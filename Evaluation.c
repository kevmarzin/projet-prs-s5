#include "Evaluation.h"
#include "Commandes_Internes.h"
#include "Utilitaires.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

int id_vide_expr_BG(pid_t tab_pid[]){
	int i = 0;
	while (i < NB_PROCS_BG_MAX && tab_pid[i] != -1)
		i++;
	
	return i < NB_PROCS_BG_MAX ? i : -1;
}

int id_pid_expr_BG(pid_t tab_pid[], pid_t pid){
	int i = 0;
	while (i < NB_PROCS_BG_MAX && tab_pid[i] != pid)
		i++;
	
	return i < NB_PROCS_BG_MAX ? i : -1;
}

int evaluer_expr_simple (char **args){
	int ret = 1;
	if (sont_egales (args[0], "echo"))
		ret = cmdInt_echo (args + 1);
	else if (sont_egales (args[0], "history"))
		ret = cmdInt_history (args + 1);
	else if (sont_egales (args[0], "date"))
		ret = cmdInt_date (args + 1);
	else if (sont_egales (args[0], "kill"))
		ret = cmdInt_kill (args + 1);
	else if (sont_egales (args[0], "exit"))
		ret = EXIT_PROG = cmdInt_exit ();
	else if (sont_egales (args[0], "hostname"))
		ret = cmdInt_hostname (args + 1);
	else { //Commande externe exécutée
		pid_t fpid;
		if ((fpid = fork()) == 0) { /* Commande extern */
			execvp(args[0], args);
			fprintf (stderr, "%s : commande introuvable\n", args[0]);
			exit(EXIT_FAILURE);
		}
		else {
			int status;
			waitpid(fpid, &status, 0);
			ret = (WEXITSTATUS(status) != EXIT_FAILURE);
		}
	}
	return ret;
}

int evaluer_expr (Expression *e){
	int ret = 1;
	
	int tube[2];
	int fd_sauvegarde_stdin;
	int fd_sauvegarde_stdout;
	int fd_sauvegarde_stderr;
	
	int fd;
	
	int nb_caracteres_lus = 0;
	char caractere_lu;
	
	int taille_nom_Bg = 1;
	if (e->type == BG) 
		taille_nom_Bg = strlen((e->gauche->arguments)[0]) + 1;
	
	int idProcBg;
	pid_t pid_fils;
	
	//Suppression des zombies
	pid_t pid_zombie;
	do {
		pid_zombie = waitpid (-1, NULL, WNOHANG);
		if (pid_zombie > 0) {
			int id = id_pid_expr_BG(PIDS_BG, pid_zombie);
			if (id != -1) {
				printf ("[%d]   Fini\t\t%d\n"/*%s\n"*/, id + 1, pid_zombie /*, NOM_PROCS_BG[id]*/);
				//NOM_PROCS_BG[id] = NULL;
				//free(NOM_PROCS_BG[id]);
				PIDS_BG[id] = -1;
			}
		}
	}
	while (pid_zombie > 0);
	
	switch (e->type){
		case VIDE: // Commande vide
			break;
		case SIMPLE: // Commande simple et ses arguments
			ret = evaluer_expr_simple (e->arguments);
			break;
		case SEQUENCE: // Séquence d'instruction (;)
			ret = evaluer_expr (e->gauche);
			if (ret)
				ret = evaluer_expr(e->droite);
			else
				evaluer_expr(e->droite);
			break;
		case SEQUENCE_ET: // Séquence conditionnelle d'instructions (&&)
			ret = evaluer_expr(e->gauche) && evaluer_expr(e->droite);
			break;
		case SEQUENCE_OU: // séquence conditionnelle d'instructions (||)
			ret = evaluer_expr(e->gauche) || evaluer_expr(e->droite);
			break;
		case BG: // Tâche en arrière plan (&)
			idProcBg = id_vide_expr_BG(PIDS_BG);
			
			if (idProcBg != -1 && (pid_fils = fork()) == 0) {
				evaluer_expr(e->gauche);
				exit (0);
			}
			else if (idProcBg == -1){
				ret = 0;
				fprintf (stderr, "Nombre maximum de processus en fond de tâche atteint ! Faire le ménage !\n");
			}
			else {
				// Sauvegarde du pid du programme lancé an BG
				PIDS_BG[idProcBg] = pid_fils;
				
				// Sauvegarde des arguments de la commande
				/*NOM_PROCS_BG[idProcBg] = malloc(taille_nom_Bg);
				memset(NOM_PROCS_BG[idProcBg], '\0', taille_nom_Bg);
				strcpy(NOM_PROCS_BG[idProcBg], (e->gauche->arguments)[0]);*/
				printf ("[%d] %d\n", idProcBg + 1, PIDS_BG[idProcBg]);
			}
			break;
		case PIPE: // pipe (|)
			pipe (tube);
			if ((pid_fils = fork()) == 0){
				close (tube[0]);
				fd_sauvegarde_stdout = dup (STDOUT_FILENO);
				dup2 (tube[1], STDOUT_FILENO);
				close (tube[1]);
				ret = evaluer_expr(e->gauche);
				dup2 (fd_sauvegarde_stdout, STDOUT_FILENO);
				close (fd_sauvegarde_stdout);
				exit (!ret);
			}
			else {
				waitpid (pid_fils, &status, 0);
				if ((WEXITSTATUS(status) != EXIT_FAILURE)) {
					close (tube[1]);
					fd_sauvegarde_stdin = dup (STDIN_FILENO);
					dup2 (tube[0], STDIN_FILENO);
					close (tube[0]);
					ret = evaluer_expr(e->droite);
					dup2 (fd_sauvegarde_stdin, STDIN_FILENO);
					close (fd_sauvegarde_stdin);
				}
				else {
					close (tube[1]);
					close (tube[0]);
					ret = 0;
				}
			}
			break;
		case REDIRECTION_I: // Redirection de l'entrée (<)
			fd = open(e->arguments[0], O_RDONLY);
			if (fd == -1){
				ret = 0;
				fprintf (stderr, "%s : Aucun fichier ou dossier de ce type\n", e->arguments[0]);
			}
			else {
				pipe (tube);
				if ((pid_fils = fork()) == 0){
					close (tube[0]);
					fd_sauvegarde_stdout = dup (STDOUT_FILENO);
					dup2 (tube[1], STDOUT_FILENO);
					close (tube[1]);
					
					while (read(fd, &caractere_lu, 1))
						write ( STDIN_FILENO, &caractere_lu, 1);
					
					close(fd);
					
					dup2 (fd_sauvegarde_stdout, STDOUT_FILENO);
					close (fd_sauvegarde_stdout);
					exit (!ret);
				}
				else {
					waitpid (pid_fils, &status, 0);
					if ((WEXITSTATUS(status) != EXIT_FAILURE)) {
						close (tube[1]);
						fd_sauvegarde_stdin = dup (STDIN_FILENO);
						dup2 (tube[0], STDIN_FILENO);
						close (tube[0]);
						ret = evaluer_expr(e->gauche);
						dup2 (fd_sauvegarde_stdin, STDIN_FILENO);
						close (fd_sauvegarde_stdin);
					}
					else {
						close (tube[1]);
						close (tube[0]);
						ret = 0;
					}
				}
			}
			break;
		case REDIRECTION_O: // Redirection de la sortie (>)
		case REDIRECTION_A: // Redirection de la sortie en mode APPEND (>>)
		case REDIRECTION_E: // Redirection de la sortie erreur (2>)
			// Ouverture du fichier en écriture avec les droits rw-r--r--
			if (e->type == REDIRECTION_A)
				fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_APPEND, 0644);
			else
				fd = open(e->arguments[0], O_WRONLY | O_CREAT, 0644);
			
			if (fd == -1){ // Problème lors de l'ouverture
				ret = 0;
				fprintf (stderr, "%s : Aucun fichier ou dossier de ce type\n", e->arguments[0]);
			}
			else { // Ouverture réussie
				if (e->type == REDIRECTION_E) {
					//Sauvegarde de la sortie erreur
					fd_sauvegarde_stderr = dup(STDERR_FILENO);
					
					// La sortie erreur est remplacée par le fichier
					dup2 (fd, STDERR_FILENO);
					close (fd);
					
					// Exécution de la commande
					ret = evaluer_expr(e->gauche);
					
					// Sortie erreur restorée
					dup2 (fd_sauvegarde_stderr, STDERR_FILENO);
					close (fd_sauvegarde_stderr);
				}
				else {
					//Sauvegarde de la sortie standard
					fd_sauvegarde_stdout = dup(STDOUT_FILENO);
					
					// La sortie standard est remplasé par le fichier
					dup2 (fd, STDOUT_FILENO);
					close (fd);
					
					// Exécution de la commande
					ret = evaluer_expr(e->gauche);
					
					// Sortie standard restorée
					dup2 (fd_sauvegarde_stdout, STDOUT_FILENO);
					close (fd_sauvegarde_stdout);
				}
			}
			break;
		case REDIRECTION_EO: // Redirection des sorties erreur et standard (&>)
			break;		
		default:
			fprintf (stderr, "fonctionnalité non implémentée\n");
			break;
	}
	
	return ret;
}

