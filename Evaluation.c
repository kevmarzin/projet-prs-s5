#include "Evaluation.h"
#include "Commandes_Internes.h"
#include "Utilitaires.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

/*
 * Initialise la chaine expr telle quelle représente l'expression e.
 */
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

/*
 * Calcul le nombre de caractères permetant d'allouer la chaîne de la cmde. Sans le caractère de fin '\0'
 */
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

/*
 * Renvoie l'id dans les tableaux de la première case non occupée
 */
int id_vide_expr_BG(pid_t tab_pid[]){
	int i = 0;
	
	// Parcours de toutes case tant que la case courante est occupée
	while (i < NB_CMDS_BG_MAX && tab_pid[i] != -1)
		i++;
	
	return i < NB_CMDS_BG_MAX ? i : -1;
}

/*
 * Renvoie l'id dans le tableau du PID fournis
 */
int id_pid_expr_BG(pid_t tab_pid[], pid_t pid){
	int i = 0;
	while (i < NB_CMDS_BG_MAX && tab_pid[i] != pid)
		i++;
	
	return i < NB_CMDS_BG_MAX ? i : -1;
}

/*
 * Supprime les processus fils zombies, lancés en BG
 */
void suppression_zombies () {
	pid_t pid_zombie;
	do {
		// Si un processus était zombie usque là, il est supprimé
		pid_zombie = waitpid (-1, NULL, WNOHANG);
		if (pid_zombie > 0) {
			// Récupération de son ID dans les tableaux
			int id = id_pid_expr_BG(pids_bg, pid_zombie);
			if (id != -1) {
				// Affichage des informations pour l'utilisateur
				printf ("[%d]   Fini\t\t\t%s\n", id + 1, cmds_bg[id]);
				
				// Les structures de données sont vidées
				free(cmds_bg [id]);
				cmds_bg [id] = NULL;
				pids_bg [id] = -1;
			}
		}
	}
	while (pid_zombie > 0); // Le faire tant que la procédure récupère un zombie
}

/*
 * Evaluation d'une commande simple, si elle a été lancée en BG cmd_en_bg est à 1 sinon 0.
 */
int evaluer_expr_simple_bg (char **args, int cmd_en_bg){
	int ret = 1;
	
	// Si c'est une commande interne reçu
	if (sont_egales (args[0], "echo"))
		ret = cmdInt_echo (args + 1);
	else if (sont_egales (args[0], "history"))
		ret = cmdInt_history (args + 1);
	else if (sont_egales (args[0], "date"))
		ret = cmdInt_date (args + 1);
	else if (sont_egales (args[0], "kill"))
		ret = cmdInt_kill (args + 1);
	else if (sont_egales (args[0], "exit"))
		ret = exit_prog = cmdInt_exit ();
	else if (sont_egales (args[0], "hostname"))
		ret = cmdInt_hostname (args + 1);
	else if (sont_egales (args[0], "remote")) {
		ret = cmdInt_remote (args + 1);
		
	}
	else if (sont_egales (args[0], "pwd"))
		ret = cmdInt_pwd(args + 1);
	else if (sont_egales (args[0], "cd"))
		ret = cmdInt_cd (args + 1);
	else { //Sinon c'est une commande externe exécutée
		pid_t fpid;
		
		// Exec lancé sans fork si la comande a été lancé en BG
		if (cmd_en_bg || (fpid = fork()) == 0) {
			
			/* Mise en place de masques de signaux, pour que le processus ne tiennent pas compte des Ctrl-c et Ctrl-\ */
			sigset_t masque;
			sigemptyset(&masque);
			sigaddset (&masque, SIGINT);
			sigaddset (&masque, SIGQUIT);
			sigprocmask (SIG_SETMASK, &masque, NULL);
			
			// Commande lancée
			execvp(args[0], args);
			
			// Erreur
			fprintf (stderr, "%s : commande introuvable\n", args[0]);
			exit(EXIT_FAILURE);
		}
		else {
			int status;
			
			// Si la commande est lancée en avant plan
			if (!cmd_en_bg)
				pid_avant_plan = fpid;
			
			// Le Shell est bloqué lorsque il y a une commande en avant plan
			waitpid(fpid, &status, 0);
			ret = (WEXITSTATUS(status) != EXIT_FAILURE);
		}
	}
	return ret;
}

/*
 * Evalue une commande qui n'est pas lancé en BG
 */
int evaluer_expr_simple (char **args){
	return evaluer_expr_simple_bg (args, 0);
}

/*
 * Evaluation d'une Expression, si elle a été lancée en BG cmd_en_bg est à 1 sinon 0.
 */
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
				
				// Le résultat de l'evaluation dépend de la seconde expression si la première a retourné vrai
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
				idProcBg = id_vide_expr_BG(pids_bg);
				
				// Evaluation de l'expression lorsqu'une place dans les tableaux de BG est trouvée
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
					pids_bg[idProcBg] = pid_fils;
					
					// Calcul de la taille de la chaîne de caractère de la commandes (avec le caractère de fin)
					taille_cmd_BG = taille_chaine_expression (e) + 1;
					
					// Allocation de la mémoire et initialisation de la chaîne de la cmd
					cmds_bg[idProcBg] = malloc (taille_cmd_BG);
					memset (cmds_bg[idProcBg], '\0', taille_cmd_BG);
					construire_chaine_expr (e, cmds_bg[idProcBg]);
					
					printf ("[%d] %d\n", idProcBg + 1, pids_bg[idProcBg]);
				}
				break;
			case PIPE: // pipe (|)
				pipe (tube);
				// Création d'un fils qui va écrire dans le tube
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
					// Attente que le fils soit fini
					waitpid (pid_fils, &status, 0);
					
					// Si le fils n'a pas produit d'erreur, on lie l'entrée standard au tube
					if ((WEXITSTATUS(status) != EXIT_FAILURE)) {
						close (tube[1]);
						fd_sauvegarde_stdin = dup (STDIN_FILENO);
						dup2 (tube[0], STDIN_FILENO);
						close (tube[0]);
						ret = evaluer_expr(e->droite);
						
						// Rétablissement de l'entrée standard
						dup2 (fd_sauvegarde_stdin, STDIN_FILENO);
						close (fd_sauvegarde_stdin);
					}
					else { // fermeture du tube
						close (tube[1]);
						close (tube[0]);
						ret = 0;
					}
				}
				break;
			case REDIRECTION_I: // Redirection de l'entrée (<)
				fd = open(e->arguments[0], O_RDONLY);
				
				// Vérification qu'il n'y a pas eu d'erreur à l'ouverture
				if (fd == -1){
					ret = 0;
					fprintf (stderr, "%s : Aucun fichier ou dossier de ce type\n", e->arguments[0]);
				}
				else {
					pipe (tube);
					// Création d'un fils qui écrit le fichier d'entrée dans le tube
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
						exit (EXIT_SUCCESS);
					}
					else {
						// Attente du fils
						waitpid (pid_fils, &status, 0);
						
						// Si le fils n'a pas produits de d'erreur, le tube est lié à l'entrée standard, et evaluation de la commande 
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
				// Création d'un fils qui évalue l'expression
				if ((pid_fils = fork ()) == 0) {
					exit (evaluer_expr (e->gauche) != 1);
				}
				else {
					// Attente du fils, conversion de EXIT_SUCESS en 1 et Exit_FAILURE 0
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

/*
 * Evalue une expression qui n'est pas lancé en BG
 */
int evaluer_expr (Expression *e) {
	return evaluer_expr_bg (e, 0);
}