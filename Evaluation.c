#include "Evaluation.h"
#include "Commandes_Internes.h"
#include "Utilitaires.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

void construire_chaine_expr (Expression *e, char *expr) {
	int i = 0;
	switch (e->type){
		case SIMPLE:
			i = 0;
			while (e->arguments[i] != NULL) {
				strcat (expr, e->arguments[i]);
				
				if (e->arguments[i + 1] != NULL)
					strcat (expr, " ");
				
				i++;
			}
			break;
		case REDIRECTION_I:
		case REDIRECTION_O:
			construire_chaine_expr (e->gauche, expr);
			if (e->type == REDIRECTION_I)
				strcat (expr, " < ");
			else // REDIRECTION_O
				strcat (expr, " > ");
			strcat (expr, e->arguments[0]);
			break;
		case REDIRECTION_E:
		case REDIRECTION_A:
		case REDIRECTION_EO:
			construire_chaine_expr (e->gauche, expr);
			if (e->type == REDIRECTION_E)
				strcat (expr, " 2> ");
			else if (e->type == REDIRECTION_A)
				strcat (expr, " >> ");
			else // REDIRECTION_EO
				strcat (expr, " &> ");
			break;
		case SEQUENCE:
		case PIPE:
		case SEQUENCE_OU:
		case SEQUENCE_ET:
			construire_chaine_expr (e->gauche, expr);
			if(e->type == SEQUENCE)
				strcat (expr, " ; ");
			else if (e->type == PIPE)
				strcat (expr, " | ");
			else if (e->type == SEQUENCE_OU)
				strcat (expr, " || ");
			else // SEQUENCE_ET
				strcat (expr, " && ");
			construire_chaine_expr (e->droite, expr);
			break;
		case BG:
			construire_chaine_expr (e->gauche, expr);
			strcat (expr, " &");
			break;
		case SOUS_SHELL:
			strcat (expr, "( ");
			construire_chaine_expr (e->gauche, expr);
			strcat (expr, " )");
		default:
			break;
	}
}

int taille_chaine_expression (Expression *e) {
	int taille = 0;
	int i = 0;
	
	switch (e->type){
		case SIMPLE:
			while (e->arguments[i] != NULL) {
				taille += strlen(e->arguments[i]);
				if (e->arguments[i + 1] != NULL) // espace
					taille ++;
				i++;
			}
			break;
		case REDIRECTION_I:
		case REDIRECTION_O:
			// taille de l'expression + " < " ou " > " + la taille du fichier
			taille += taille_chaine_expression(e->gauche) + 3 + strlen (e->arguments[0]);
			break;
		case REDIRECTION_E:
		case REDIRECTION_A:
		case REDIRECTION_EO:
			// taille de l'expression + " >> " ou " 2> " ou " &> " + la taille du fichier
			taille += taille_chaine_expression(e->gauche) + 4 + strlen (e->arguments[0]);
			break;
		case SEQUENCE:
		case PIPE:
			//taille des deux expressions + taille de " ; " ou " | "
			taille += taille_chaine_expression (e->gauche) + 3 + taille_chaine_expression (e->droite);
			break;
		case SEQUENCE_ET:
		case SEQUENCE_OU:
			//taille des deux expressions + taille de " || " ou " && "
			taille += taille_chaine_expression (e->gauche) + 4 + taille_chaine_expression (e->droite);
			break;
		case BG:
			//taille de l'expression + " &"
			taille += taille_chaine_expression (e->gauche) + 2;
			break;
		case SOUS_SHELL:
			// taille de "( " + expression + " )"
			taille += 2 + taille_chaine_expression (e->gauche) + 2;
			break;
		default:
			break;
	}
	
	return taille;
}

int id_vide_expr_BG(pid_t tab_pid[]){
	int i = 0;
	while (i < NB_CMDS_BG_MAX && tab_pid[i] != -1)
		i++;
	
	return i < NB_CMDS_BG_MAX ? i : -1;
}

int id_pid_expr_BG(pid_t tab_pid[], pid_t pid){
	int i = 0;
	while (i < NB_CMDS_BG_MAX && tab_pid[i] != pid)
		i++;
	
	return i < NB_CMDS_BG_MAX ? i : -1;
}

void suppression_zombies () {
	pid_t pid_zombie;
	do {
		pid_zombie = waitpid (-1, NULL, WNOHANG);
		if (pid_zombie > 0) {
			int id = id_pid_expr_BG(PIDS_BG, pid_zombie);
			if (id != -1) {
				printf ("[%d]   Fini\t\t\t%s\n", id + 1, CMDS_BG[id]);
				
				// Les structures de données sont vidées
				free(CMDS_BG[id]);
				CMDS_BG[id] = NULL;
				PIDS_BG[id] = -1;
			}
		}
	}
	while (pid_zombie > 0);
}

int evaluer_expr_simple_bg (char **args, int cmd_en_bg){
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
	else if (sont_egales (args[0], "remote"))
		ret = cmdInt_remote (args + 1);
	else if (sont_egales (args[0], "pwd"))
		ret = cmdInt_pwd(args + 1);
	else { //Commande externe exécutée
		pid_t fpid;
		if (cmd_en_bg || (fpid = fork()) == 0) {
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

int evaluer_expr_simple (char **args){
	return evaluer_expr_simple_bg (args, 0);
}

int evaluer_expr_bg (Expression *e, int cmd_en_bg){
	int ret = 1;
	
	int tube[2];
	int fd_sauvegarde_stdin;
	int fd_sauvegarde_stdout;
	int fd_sauvegarde_stderr;
	
	int fd;
	
	int nb_caracteres_lus = 0;
	char caractere_lu;
	
	int taille_cmd_BG = 0;
	int idProcBg;
	pid_t pid_fils;
	
	if (interactive_mode || e->type == SIMPLE || e->type == VIDE) {
		//Suppression des processus zombies lancés en BG (seulement dans le mode intéractif)
		if (interactive_mode) {
			suppression_zombies ();
		}
		
		switch (e->type){
			case VIDE: // Commande vide
				break;
			case SIMPLE: // Commande simple et ses arguments
				ret = evaluer_expr_simple_bg (e->arguments, cmd_en_bg);
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
					evaluer_expr_bg(e->gauche, 1);
					exit (0);
				}
				else if (idProcBg == -1){
					ret = 0;
					fprintf (stderr, "Nombre maximum de processus en fond de tâche atteint ! Faire le ménage !\n");
				}
				else {
					// Sauvegarde du pid du programme lancé an BG
					PIDS_BG[idProcBg] = pid_fils;
					
					// Calcul de la taille de la chaîne de caractère de la commandes (avec le caractère de fin)
					taille_cmd_BG = taille_chaine_expression (e) + 1;
					
					// Allocation de la mémoire et initialisation de la chaîne de la cmd
					CMDS_BG[idProcBg] = malloc (taille_cmd_BG);
					memset (CMDS_BG[idProcBg], '\0', taille_cmd_BG);
					construire_chaine_expr (e, CMDS_BG[idProcBg]);
					
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
			case REDIRECTION_EO: // Redirection des sorties erreur et standard (&>)
			case REDIRECTION_O: // Redirection de la sortie (>)
			case REDIRECTION_A: // Redirection de la sortie en mode APPEND (>>)
			case REDIRECTION_E: // Redirection de la sortie erreur (2>)
				// Ouverture du fichier en écriture avec les droits rw-r--r--
				if (e->type == REDIRECTION_A)
					fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_APPEND, 0644);
				else
					fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				
				if (fd == -1){ // Problème lors de l'ouverture du fichier
					ret = 0;
					fprintf (stderr, "%s : Aucun fichier ou dossier de ce type\n", e->arguments[0]);
				}
				else { // Ouverture réussie
					if (e->type == REDIRECTION_E || e->type == REDIRECTION_EO) {
						//Sauvegarde de la sortie erreur
						fd_sauvegarde_stderr = dup(STDERR_FILENO);
						
						// La sortie erreur est remplacée par le fichier
						dup2 (fd, STDERR_FILENO);
						close (fd);
					}
					
					if (e->type == REDIRECTION_EO 	|| e->type == REDIRECTION_O
													|| e->type == REDIRECTION_A) {
						//Sauvegarde de la sortie standard
						fd_sauvegarde_stdout = dup(STDOUT_FILENO);
						
						// La sortie standard est remplasé par le fichier
						dup2 (fd, STDOUT_FILENO);
						close (fd);
					}
					
					// Exécution de la commande
					ret = evaluer_expr(e->gauche);
					
					if (e->type == REDIRECTION_E 	|| e->type == REDIRECTION_EO ) {
						// Sortie erreur restorée
						dup2 (fd_sauvegarde_stderr, STDERR_FILENO);
						close (fd_sauvegarde_stderr);
					}
					
					if (e->type == REDIRECTION_EO 	|| e->type == REDIRECTION_O
													|| e->type == REDIRECTION_A ) {
						// Sortie standard restorée
						dup2 (fd_sauvegarde_stdout, STDOUT_FILENO);
						close (fd_sauvegarde_stdout);
					}
				}
				break;
			case SOUS_SHELL:
				if ((pid_fils = fork ()) == 0) {
					exit (evaluer_expr (e->gauche) != 1);
				}
				else {
					waitpid (pid_fils, &status, 0);
					ret = WEXITSTATUS(status) == EXIT_SUCCESS;
				}
				break;
			default:
				ret = 0;
				fprintf (stderr, "fonctionnalité non implémentée\n");
				break;
		}
		//Suppression des processus zombies lancés en BG (seulement dans le mode intéractif)
		if (interactive_mode) {
			suppression_zombies ();
		}
	}
	else {
		ret = 0;
		fprintf (stderr, "fonctionnalité non implémentée\n");
	}
	return ret;
}

int evaluer_expr (Expression *e) {
	return evaluer_expr_bg (e, 0);
}