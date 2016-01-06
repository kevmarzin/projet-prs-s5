#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

#include <sys/wait.h>
#include <ctype.h>

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
	if (strcmp (args[0], "echo") == 0)
		ret = cmdInt_echo (args + 1);
	else if (strcmp (args[0], "history") == 0)
		ret = cmdInt_history (args + 1);
	else if (strcmp (args[0], "date") == 0)
		ret = cmdInt_date (args + 1);
	else if (strcmp (args[0], "kill") == 0)
		ret = cmdInt_kill (args + 1);
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
			ret = WEXITSTATUS(status) != EXIT_FAILURE;
		}
	}
	return ret;
}/*
VIDE, commande vide																	|
      |   - SIMPLE, commande simple et ses arguments											|
      |   - SEQUENCE, séquence (;) d'instructions												|
      |   - SEQUENCE_ET, séquence conditionnelle (&&) d'instructions							|
      |   - SEQUENCE_OU, séquence conditionnelle (||) d'instructions							|
      |   - BG, tâche en arrière plan (&)														|
      |   - PIPE, pipe (|).																		|
      |   - REDIRECTION_I, redirection de l'entrée (<)											|
      |   - REDIRECTION_O, redirection de la sortie (>)											|
      |   - REDIRECTION_A, redirection de la sortie en mode APPEND (>>).						|
      |   - REDIRECTION_E, redirection de la sortie erreur,  									|
      |   - REDIRECTION_EO, redirection des sorties erreur et standard.			*/

int evaluer_expr (Expression *e){
	int ret = 1;
	
	int taille_nom_Bg = 1;
	if (e->type == BG) 
		taille_nom_Bg = strlen((e->gauche->arguments)[0]) + 1;
	
	int idProcBg;
	pid_t pidBg;
	
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
			
			if (idProcBg != -1 && (pidBg = fork()) == 0) {
				evaluer_expr(e->gauche);
				exit (0);
			}
			else if (idProcBg == -1){
				ret = 0;
				fprintf (stderr, "Nombre maximum de processus en fond de tâche atteint ! Faire le ménage !\n");
			}
			else {
				// Sauvegarde du pid du programme lancé an BG
				PIDS_BG[idProcBg] = pidBg;
				
				// Sauvegarde des arguments de la commande
				/*NOM_PROCS_BG[idProcBg] = malloc(taille_nom_Bg);
				memset(NOM_PROCS_BG[idProcBg], '\0', taille_nom_Bg);
				strcpy(NOM_PROCS_BG[idProcBg], (e->gauche->arguments)[0]);*/
				printf ("[%d] %d\n", idProcBg + 1, PIDS_BG[idProcBg]);
			}
			break;
		case PIPE: // pipe (|)
			
			break;
		case REDIRECTION_I: // Redirection de l'entrée (<)
			break;
		case REDIRECTION_O: // Redirection de la sortie (>)
			break;
		case REDIRECTION_A: // Redirection de la sortie en mode APPEND (>>)
			break;
		case REDIRECTION_E: // Redirection de la sortie erreur
			break;
		case REDIRECTION_EO: // Redirection des sorties erreur et standard
			break;		
		default:
			fprintf (stderr, "fonctionnalité non implémentée\n");
			break;
	}
	
	return ret;
}

