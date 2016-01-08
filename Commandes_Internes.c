#include "Commandes_Internes.h"
#include "Utilitaires.h"
#include "Shell.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

// Pour enlever le warning incompréhensible "implicit declaration of function sethostname..."
int sethostname(const char *name, size_t len);

int calcul_date (struct tm *date, int nbr, const char *mot){
	// Identification du jour de la semaine donné, si ça n'en est pas un : -1
	int jour_semaine = 	(sont_egales (mot, "sun") || sont_egales (mot, "sunday"))	? 0 :
						(sont_egales (mot, "mon") || sont_egales (mot, "monday"))	? 1 :
						(sont_egales (mot, "tue") || sont_egales (mot, "tuesday"))	? 2 :
						(sont_egales (mot, "wed") || sont_egales (mot, "wednesday"))? 3 :
						(sont_egales (mot, "thu") || sont_egales (mot, "thursday"))	? 4 :
						(sont_egales (mot, "fri") || sont_egales (mot, "friday"))	? 5 :
						(sont_egales (mot, "sat") || sont_egales (mot, "saturday"))	? 6 : -1;
	
	// Conversion de la structure de date en secondes depuis l'Epoch (1970)
	time_t date_en_secondes = mktime(date);
	
	if(sont_egales(mot, "year")) {
		date->tm_year += nbr;
		mktime(date); // Maj du numéro du jour de la semaine
	}else if(sont_egales(mot, "month")) {
		int annee_suivante = 0;
		date->tm_mon = (annee_suivante = ((date->tm_mon + nbr) > 11)) ? 0 : (date->tm_mon + nbr);
		date->tm_year = annee_suivante ? (date->tm_year + 1) : date->tm_year;
		mktime(date);
	}
	else if(sont_egales(mot, "day")) {
		date_en_secondes += nbr * (60*60*24);
		date = localtime(&date_en_secondes);
	}
	else if(sont_egales(mot, "hour")){
		date_en_secondes += nbr * (60*60);
		date = localtime(&date_en_secondes);
	}
	else if(sont_egales(mot, "minute")){
		date_en_secondes += nbr * 60;
		date = localtime(&date_en_secondes);
	}
	else if(sont_egales(mot, "second")) {
		date_en_secondes += nbr * 60;
		date = localtime(&date_en_secondes);
	}
	else if (nbr == 1 && jour_semaine != -1) {
		int nb_jour = jour_semaine - date->tm_wday;
		nb_jour = nb_jour <= 0  ? nb_jour + 7 : nb_jour;
		calcul_date (date, nb_jour, "day");
	}
	else
		return 0;
	
	return 1;
}

struct tm *analyser_date (char *date_donnee) {
	time_t timestamp = time(NULL);
	struct tm *date = localtime (&timestamp);
	int erreur = 0;
	
	if (estNombre (date_donnee)) {
		if (strlen (date_donnee) <= 4 && strlen (date_donnee) >= 1) {
			date->tm_sec = 0;
			date->tm_min = 0;
			strptime(date_donnee, "%H%M", date);
		}
		else if (strlen (date_donnee) > 4) {
			strptime(date_donnee, "%Y%m%d", date);
			date->tm_sec = 0;
			date->tm_min = 0;
			date->tm_hour = 0;
		}
	}
	else {
		char **mots = split_str(date_donnee, ' ');
		
		int nb_mots = LongueurListe(mots);
		char *mot_lower;
		
		if (nb_mots > 0) {
			if (sont_egales ((mot_lower = strtolower(mots[0])), "next") && mots[1] != NULL) {
				free (mot_lower);
				erreur = !(calcul_date (date, 1, (mot_lower = strtolower(mots[1]))));
				free (mot_lower);
			}
			else if (nb_mots == 3 && sont_egales( (mot_lower = strtolower(mots[nb_mots - 1])), "ago") && estNombre (mots[0])) {
				free (mot_lower);
				erreur = !(calcul_date (date, atoi(mots[0]) * -1, (mot_lower = strtolower(mots[1]))));
				free (mot_lower);
			}
		}
		else if (mots[0][3] == ':')
			strptime(date_donnee, "%T %D", date);
		else if (mots[1] != NULL && mots[1][3] == ':')
			strptime(date_donnee, "%D %T", date);
		else 
			strptime(date_donnee, "%a, %d %b %Y %T", date);
		
		int i = 0;
		while (mots[i] != NULL) {
			free (mots[i]);
			i++;
		}
		free (mots);
	}
	
	return !erreur ? date : NULL;
}

void supprimer_contre_oblique_echo (char *chaine, int interpreter_contre_oblique, int *arret_sortie) {
	int i = 0;
	
	while (chaine[i] != '\0' && !(*arret_sortie)) {
		if (interpreter_contre_oblique 	&& chaine[i] == '\\'
										&& chaine[i+1] == '\\') {
			switch (chaine[i+2]){
				case 'a':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\a';
					break;
				case 'b':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\b';
					break;
				case 'c':
					(*arret_sortie) = 1;
					chaine[i] = '\0';
					break;
				case 'e':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\e';
					break;
				case 'f':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\f';
					break;
				case 'n':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\n';
					break;
				case 'r':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\r';
					break;
				case 't':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\t';
					break;
				case 'v':
					supprimer_char (chaine, i);
					supprimer_char (chaine, i);
					chaine[i] = '\v';
					break;
				default:
					supprimer_char (chaine, i);
					i++;
					break;
			}
		}
		else if (chaine[i] == '\\' && !interpreter_contre_oblique) {
			supprimer_char (chaine, i); // Suppression du '\'
			i++; // Caractère suivant passé
		}
		
		if (!(*arret_sortie))
			i++;
	}
}

/*
 * Commande echo.
 */
int cmdInt_echo(char **args){
	int id_arg = 0;
	int id_arg_debut_affichage = -1;
	
	int saut_de_ligne_final = 1;
	int interpreter_contre_oblique = 0;
	int arret_sortie = 0;
	
	int erreur = 0;
	
	//Lecture de tout les arguments de la commande, sauf si interpreter_contre_oblique && "\\c" présent dans la sortie
	while (args[id_arg] != NULL && !arret_sortie) {
		// Lecture des options, jusqu'à ce qu'une option inconnue soit lue
		if (args[id_arg][0] == '-' 	&& strlen (args[id_arg]) == 2
									&& id_arg_debut_affichage == -1) {
			if (args[id_arg][1] == 'n')
				saut_de_ligne_final = 0;
			else if (args[id_arg][1] == 'e')
				interpreter_contre_oblique = 1;
			else if (args[id_arg][1] == 'E')
				;
			else
				id_arg_debut_affichage = id_arg;
		}
		else if (id_arg_debut_affichage == -1){
			id_arg_debut_affichage = id_arg;
		}
		
		//A partir de la lecture d'une option inconnue
		if (id_arg_debut_affichage != -1){
			if (id_arg > id_arg_debut_affichage)
				printf (" ");
			
			char *affichage = args[id_arg];
			//interprétation des '\\' de la sortie, suivant les options données
			supprimer_contre_oblique_echo (affichage, interpreter_contre_oblique, &arret_sortie);
			
			//affichage de la sortie
			printf ("%s", affichage);
		}
		
		if (!arret_sortie)
			id_arg++;
	}
	
	if (saut_de_ligne_final && !arret_sortie)
		printf ("\n");
	
	return !erreur;
}

int cmdInt_history (char **args) {
	HISTORY_STATE *etat_hist = history_get_history_state();
	HIST_ENTRY **hists = etat_hist->entries;
	int taille_hist = etat_hist->length;
	
	int nb_lignes_specifie = 0;
	int nb_args = LongueurListe(args);
	
	int erreur = 0;
	int clear = 0;
	int elem_a_suppr = -1;
	
	int options_anrw[] = { 0, 0, 0, 0 };
	char *file_hist = NULL;
	
	int option_d = 0;
	int options_ps[] = {0, 0};
	char **option_ps_chaine = NULL;
	
	int id_arg = 0;
	
	if (args[id_arg] != NULL && args[id_arg][0] == '-' && strlen(args[id_arg]) > 1) {
		int longueur_arg;
		
		while (!erreur && args[id_arg] != NULL) {
			if (args[id_arg][0] == '-') {
				longueur_arg = strlen(args[id_arg]);
					
				if (commence_par (args[id_arg], "-d")){
					if (sont_egales(args[id_arg], "-d")){
						id_arg ++;
						if (args[id_arg] != NULL && estNombre (args[id_arg])) { // l'argument existe et est un nombre
							// On relève l'argument
							elem_a_suppr = atoi (args[id_arg]);
							
							// Si 1 <= elem_a_suppr <= taillede l'historique, c'est bon sinon Erreur
							if (elem_a_suppr < 1 || elem_a_suppr > taille_hist) {
								erreur = 1;
								fprintf (stderr, "history : %d : position dans l'historique hors plage\n", elem_a_suppr);
							}
						}
						else if (args[id_arg] != NULL) { // L'argument n'est pas un nombre
							fprintf (stderr, "history : %s : position dans l'historique hors plage\n", args[id_arg]);
							erreur = 1;
						}
						else { // Pas d'option après le -d
							fprintf (stderr, "history : -d : l'option a besoin d'un argument\n");
							erreur = 1;
						}
					}
					else  { // Argument de -d collé
						if (estNombre (args[id_arg] + 2)) { // vérification du type, nbr = c'est bon, sinon Erreur
							elem_a_suppr = atoi (args[id_arg] + 2);
							if (elem_a_suppr < 1 || elem_a_suppr > taille_hist) {
								erreur = 1;
								fprintf (stderr, "history : %d : position dans l'historique hors plage\n", elem_a_suppr);
							}
						}
						else {
							fprintf (stderr, "history : %s : argument numérique nécessaire\n", args[id_arg] + 2);
							erreur = 1;
						}
					}
				}
				else {
					int id_char = 1;
					while (args[id_arg][id_char] != '\0' && !erreur) {
						char option_en_cours = args[id_arg][id_char];
						if ((option_en_cours == 'a' || option_en_cours == 'n'
												   || option_en_cours == 'r'
												   || option_en_cours == 'w') && !options_ps[0]
																			  && !options_ps[1]
																			  && !clear
																			  && !option_d) {
							if (option_en_cours == 'a'  && !options_anrw[1]
														&& !options_anrw[2]
														&& !options_anrw[3]) {
								options_anrw[0] = 1;
								file_hist = args[id_arg + 1];
							}
							else if (option_en_cours == 'n' && !options_anrw[0]
															&& !options_anrw[2]
															&& !options_anrw[3]){
								options_anrw[1] = 1;
								file_hist = args[id_arg + 1];
							}
							else if (option_en_cours == 'r' && !options_anrw[0]
															&& !options_anrw[1]
															&& !options_anrw[3]) {
								options_anrw[2] = 1;
								file_hist = args[id_arg + 1];
							}
							else if (option_en_cours == 'w' && !options_anrw[0]
															&& !options_anrw[2]
															&& !options_anrw[2]) {
								options_anrw[3] = 1;
								file_hist = args[id_arg + 1];
							}
							else {
								erreur = 1;
								printf ("history: impossible d'utiliser plus d'une option parmi « -anrw »\n");
							}
						}
						else if (option_en_cours == 'p'   && !options_anrw[0]
																&& !options_anrw[1]
																&& !options_anrw[2]
																&& !options_anrw[3]
																&& !clear
																&& !option_d){
							if (!options_ps[1]){
								option_ps_chaine = args + (id_arg + 1);
								options_ps[0] = 1;
							}
						}
						else if (option_en_cours == 's'  && !options_anrw[0]
															&& !options_anrw[1]
															&& !options_anrw[2]
															&& !options_anrw[3]
															&& !clear
															&& !option_d){
							if (!options_ps[0]){
								option_ps_chaine = args + (id_arg + 1);
								options_ps[1] = 1;
							}
						}
						else if (option_en_cours == 'd' && (options_anrw[0] || options_anrw[1]
																			|| options_anrw[2]
																			|| options_anrw[3]
																			|| options_ps[0]
																			|| options_ps[1])) {
							erreur = 1;
							fprintf (stderr, "history : options incompatibles\n");
							fprintf (stderr, "history : utilisation : history [-c] [-d décalage] [n] \n\t\t\tou history -anrw [nomfichier] \n\t\t\tou history -ps arg [arg...]\n");
						}
						else if (option_en_cours == 'c' && !(options_anrw[0] || options_anrw[1]
																			|| options_anrw[2]
																			|| options_anrw[3]
																			|| options_ps[0]
																			|| options_ps[1])){
							clear = 1;
						}
						else {
							erreur = 1;
							if (option_en_cours == 'c'  || option_en_cours == 'd'
														|| option_en_cours == 'p'
														|| option_en_cours == 's'
														|| option_en_cours == 'a'
														|| option_en_cours == 'n'
														|| option_en_cours == 'r'
														|| option_en_cours == 'w')
								fprintf (stderr, "history : options incompatibles\n");
							else
								fprintf (stderr, "history : -%c : option non valable\n", option_en_cours);
							fprintf (stderr, "history : utilisation : history [-c] [-d décalage] [n] \n\t\t\tou history -anrw [nomfichier] \n\t\t\tou history -ps arg [arg...]\n");
						}
						id_char++;
					}
					if (!erreur && (options_anrw[0] || options_anrw[1]
													|| options_anrw[2]
													|| options_anrw[0]))
						id_arg++;
				}
			}
			
			if (args[id_arg] != NULL){
				id_arg++;
			}	
		}
	}
	else if ((nb_lignes_specifie = (args[0] != NULL && estNombre(args[0]))) || args[0] == NULL) {
		if (nb_args > 1) {
			fprintf (stderr, "history : trop d'arguments");
			erreur = 1;
		}
		else {
			int id_hist = 0;
			
			if (nb_lignes_specifie) {
				id_hist = taille_hist - atoi(args[0]);
				if (id_hist < 0)
					id_hist = 0;
			}
			
			while (hists[id_hist] != NULL && taille_hist > 0) {
				printf("  %d\t%s\n", id_hist + 1, hists[id_hist]->line);
				id_hist++;
			}
		}
	}
	else {
		fprintf (stderr, "history : %s : argument numérique nécessaire", args[0]);
		erreur = 1;
	}
	
	if (!erreur){
		if (clear){ // history -c
			clear_history();
		}
		else if (elem_a_suppr != -1){ // history -d n
			free_history_entry (remove_history (elem_a_suppr - 1));
		}
		else if(options_ps[0]) { // history -p ...
			int i = 0;
			while (option_ps_chaine[i] != NULL){
				printf("%s\n", option_ps_chaine[i]);
				i++;
			}
			free_history_entry (remove_history (taille_hist - 1));
		}
		else if(options_ps[1]){
			if ((nb_args = LongueurListe(option_ps_chaine)) > 0){
				char *nouvelle_entree = malloc(nb_args * 40 + nb_args);
				memset (nouvelle_entree, '\0', nb_args * 40 + nb_args);
				
				int taille_chaine = 0;
				int i = 0;
				while (taille_chaine != -1 && option_ps_chaine[i] != NULL){
					if (taille_chaine + strlen (option_ps_chaine[i]) < nb_args * 40 + nb_args - 1) {
						strcat (nouvelle_entree, option_ps_chaine[i]);
						strcat (nouvelle_entree, " ");
						taille_chaine += strlen (option_ps_chaine[i]) + 1;
					}
					else 
						taille_chaine = -1;
					
					i++;
				}
				free_history_entry (replace_history_entry (taille_hist - 1, nouvelle_entree, NULL));
				free (nouvelle_entree);
			}
		}
		else if(options_anrw[0] || options_anrw[1]
								|| options_anrw[2]
								|| options_anrw[3]){
			int fd;
			if (options_anrw[0]) {
				printf ("%s\n", file_hist);
				if (file_hist != NULL && (fd = open (file_hist, O_CREAT | O_APPEND, 0644)) != -1)
					close (fd);
				erreur = append_history (taille_hist, file_hist);
			}
			else if (options_anrw[1] || options_anrw[2])
				erreur = read_history (file_hist);
			else if (options_anrw[3]) 
				erreur = history_truncate_file (file_hist, taille_hist);
			
			if (erreur)
				printf("ERREUR %d\n", erreur);
			
		}
	}
	
	return !erreur;
}

int cmdInt_date (char **args){
	int id_arg = 0;
	
	char *modifier_date = NULL;
	
	char *format = NULL;
	int nb_options_format = 0;
	
	int nb_options_date = 0;
	char *fichier_date_modification = NULL;
	char *fichier_dates = NULL;
	char *date_donnee = NULL;
	char *argument_iso8601 = NULL;
	
	int option_utc = 0;
	
	int erreur = 0;
	
	while (args[id_arg] != NULL && !erreur) {
		if (sont_egales (args[id_arg], "-d")|| sont_egales (args[id_arg], "--date") 
											|| commence_par (args[id_arg], "--date=")){
			nb_options_date++;
			if ( args[id_arg + 1] != NULL	&& args[id_arg + 1][0] != '-' 
											&& (sont_egales (args[id_arg], "-d") || sont_egales (args[id_arg], "--date")
																				 || sont_egales (args[id_arg], "--date=")))
				date_donnee = args[++id_arg];
			else if (commence_par (args[id_arg], "--date=") && strlen (args[id_arg]) > strlen ("--date="))
				date_donnee = args[id_arg] + strlen ("--date=");
			else if (commence_par (args[id_arg], "-d") && strlen (args[id_arg]) > strlen ("-d"))
				date_donnee = args[id_arg] + strlen ("-d");
			else
				date_donnee = "";
			
		}
		else if (commence_par (args[id_arg], "-f")	|| sont_egales (args[id_arg], "--file") 
													|| commence_par (args[id_arg], "--file=")) {
			nb_options_date++;
			if (sont_egales (args[id_arg], "-f") || sont_egales (args[id_arg], "--file")) {
				if (args[id_arg + 1] != NULL)
					fichier_dates = args[++id_arg];
				else {
					erreur = 1;
					fprintf (stderr, "date: l'option %s requiert un argument\n", args[id_arg]);
				}
			}
			else if (commence_par (args[id_arg], "-f")) 
				fichier_dates = args[id_arg] + strlen("-f");
			else if (commence_par (args[id_arg], "--file="))
				fichier_dates = args[id_arg] + strlen("--file=");
			
			FILE* fic;
			if (!erreur && (fic = fopen(fichier_dates, "r")) != NULL ) {
				fclose (fic);
			}
			else if (!erreur) {
				erreur = 1;
				fprintf (stderr, "date : %s : Aucun fichier ou dossier de ce type\n", fichier_dates);
			}

		}
		else if (	commence_par(args[id_arg], "-I") 	|| sont_egales (args[id_arg], "--iso-8601") 
														|| commence_par (args[id_arg], "--iso-8601=")) {
			nb_options_format++;
			argument_iso8601 = "date";
			
			int longueur_nom_arg;
			int argument_suivant = 0;
			if (strlen(args[id_arg]) > 2 && args[id_arg][0] == '-' && args[id_arg][1] == 'I')
				longueur_nom_arg = strlen("-I");
			else if (strstr(args[id_arg], "--iso-8601") != NULL 	&& strlen(args[id_arg]) > strlen ("--iso-8601")){
				longueur_nom_arg = strlen("--iso-8601=");
				argument_suivant = (strstr(args[id_arg], "--iso-8601=") != NULL && strlen(args[id_arg]) == strlen ("--iso-8601="));
			}
			else
				longueur_nom_arg = 0;
			
			if (longueur_nom_arg != 0 || argument_suivant) {
				if (strcmp(args[id_arg] + longueur_nom_arg, "hours") == 0 	|| strcmp(args[id_arg] + longueur_nom_arg, "minutes") == 0 
																			|| strcmp(args[id_arg] + longueur_nom_arg, "date") == 0 
																			|| strcmp(args[id_arg] + longueur_nom_arg, "seconds") == 0
																			|| strcmp(args[id_arg] + longueur_nom_arg, "ns") == 0) 
					argument_iso8601 = args[id_arg] + longueur_nom_arg;
				else if (argument_suivant && args[id_arg + 1] != NULL && 	(strcmp(args[id_arg + 1], "hours") == 0 	
																			|| strcmp(args[id_arg + 1], "minutes") == 0 
																			|| strcmp(args[id_arg + 1], "date") == 0 
																			|| strcmp(args[id_arg + 1], "seconds") == 0
																			|| strcmp(args[id_arg + 1], "ns") == 0))
					argument_iso8601 = args[++id_arg];
				else {
					erreur = 1;
					if (argument_suivant && args[id_arg + 1] != NULL)
						argument_iso8601 == args[++id_arg];
					if (argument_suivant)
						argument_iso8601 = "";
					else
						argument_iso8601 = args[id_arg] + longueur_nom_arg;
					
					fprintf (stderr,
							 "date: argument « %s » incorrect pour « --iso-8601 »\nLes arguments valables sont :\n    -\t« hours »\n    -\t« minutes »\n    -\t« date »\n    -\t« seconds »\n    -\t« ns »\n",
							 argument_iso8601);
				}
			}
		}
		else if (commence_par (args[id_arg], "-r")  || sont_egales (args[id_arg], "--reference") 
													|| commence_par (args[id_arg], "--reference=")){
			nb_options_date++;
			if (sont_egales(args[id_arg], "-r") || sont_egales (args[id_arg], "--reference")){
				if (args[id_arg + 1] != NULL)
					fichier_date_modification = args[++id_arg];
				else {
					erreur = 1;
					fprintf (stderr, "date : l'option « %s » requiert un argument\n", args[id_arg]);
				}
			}
			else if (strcmp(args[id_arg], "--reference=") == 0 && args[id_arg + 1] != NULL && !commence_par (args[id_arg + 1], "-"))
					fichier_date_modification = args[++id_arg];
			else if (commence_par( args[id_arg], "-r"))
				fichier_date_modification = (args[id_arg] + strlen("-r"));
			else if (commence_par(args[id_arg], "--reference=") && strlen ("--reference=") < strlen (args[id_arg]))
				fichier_date_modification = (args[id_arg] + strlen("--reference="));
			else 
				fichier_date_modification = "";
			
			FILE* fic;
			if (!erreur && (fic = fopen(fichier_date_modification, "r")) != NULL ) {
				fclose (fic);
			}
			else if (!erreur) {
				erreur = 1;
				fprintf (stderr, "date : %s : Aucun fichier ou dossier de ce type\n", fichier_date_modification);
			}
		}
		else if (sont_egales (args[id_arg], "-R")   || sont_egales (args[id_arg], "--rfc-2822")
													|| commence_par (args[id_arg], "--rfc-2822=")) {
			if (commence_par (args[id_arg], "--rfc-2822=")) {
				fprintf (stderr, "date : l'option « --rfc-2822 » ne permet pas d'argument\n");
				erreur = 1;
			}
			else {
				nb_options_format++;
				format = "%a, %d %b %Y %X %z";
			}
		}
		else if (commence_par (args[id_arg], "--rfc-3339=")  || sont_egales (args[id_arg], "--rfc-3339")) {
			if (sont_egales (args[id_arg], "--rfc-3339") && args[id_arg + 1] == NULL) {
				erreur = 1;
				fprintf (stderr, "date : l'option « %s » requiert un argument\n", args[id_arg]);
			}
			else {
				char* format_rfc;
				if (sont_egales (args[id_arg], "--rfc-3339"))
					format_rfc = (args[id_arg] + strlen("--rfc-3339"));
				else
					format_rfc = (args[id_arg] + strlen("--rfc-3339="));
					
				if (format_rfc[0] == '\0' && args[id_arg + 1] != NULL && args[id_arg + 1][0] != '-')
					format_rfc = args[++id_arg];
				
				if (format_rfc[0] != '\0' &&   (sont_egales (format_rfc, "date")
											||	sont_egales (format_rfc, "seconds")
											||	sont_egales (format_rfc, "ns"))){
					nb_options_format++;
					if (sont_egales (format_rfc, "date"))
						format = "%F";
					else if (sont_egales (format_rfc, "seconds"))
						format = "%F %X%z";
					else if (sont_egales (format_rfc, "ns"))
						format = "%F %X.%N%z";
				}
				else {
					erreur = 1;
					fprintf (stderr, "date : argument « %s » incorrect pour « --rfc-3339 »\nLes arguments valables sont :\n  - « date »\n  - « seconds »\n  - « ns »\n", format_rfc);
				}
			}
		}
		else if (strcmp(args[id_arg], "-s") == 0 || strcmp(args[id_arg], "--set=") == 0)
			;
		else if (sont_egales (args[id_arg], "-u") || sont_egales (args[id_arg], "--utc") 
												  || sont_egales (args[id_arg], "--universal"))
			option_utc = 1;
		else if (commence_par (args[id_arg], "--utc=") || commence_par (args[id_arg], "--universal=")) {
			erreur = 1;
			if (commence_par (args[id_arg], "--utc="))
				printf ("date : l'option « --utc » ne permet pas d'argument");
			else
				printf ("date : l'option « --universal » ne permet pas d'argument");
		}
		else if (args[id_arg][0] == '+') {
			format = args[id_arg] + 1;
			nb_options_format++;
		}
		else {
			erreur = 1;
			fprintf (stderr, "date: date incorrecte « %s »\n", args[id_arg]);
		}
	
		id_arg++;
	}
	
	if (!erreur && (nb_options_format == 0 || nb_options_format == 1) && (nb_options_date == 0 || nb_options_date == 1)) {
		char buffer[256];
		time_t timestamp;
		struct tm *date;
		struct timespec timestamp_avec_ns;
		
		if (nb_options_format == 0)
			format = "%A %d %B %Y, %X (UTC%z)";
		else if (argument_iso8601 != NULL) {
			if (strcmp(argument_iso8601, "date") == 0)
				format = "%F";
			else if (strcmp(argument_iso8601, "hours") == 0)
				format = "%F %H%z";
			else if (strcmp(argument_iso8601, "minutes") == 0)
				format = "%F %R%z";
			else if (strcmp(argument_iso8601, "seconds") == 0)
				format = "%F %T%z";
			else if (strcmp(argument_iso8601, "ns") == 0)
				format = "%F %R,%N%z";
		}
		if (nb_options_date == 0) {
			clock_gettime (CLOCK_REALTIME, &timestamp_avec_ns);
			timestamp = timestamp_avec_ns.tv_sec;
			date = localtime(&timestamp);
		}
		else if (nb_options_date == 1) {
			struct stat buf;
			if (fichier_date_modification != NULL && stat (fichier_date_modification, &buf) == 0) {
				timestamp = buf.st_mtime;
				date = localtime(&timestamp);
			}
			else if (date_donnee != NULL) {
				if (commence_par (date_donnee, "@") ) {
					timestamp = atoi(date_donnee + 1);
					date = localtime(&timestamp);
				}
				else if (sont_egales( date_donnee, "") ){
					timestamp = time(NULL);
					date = localtime(&timestamp);
					date->tm_sec = 0;
					date->tm_min = 0;
					date->tm_hour = 0;
				}
				else if ((date = analyser_date (date_donnee)) == NULL)
					erreur = 1;
			}
			else if (fichier_dates != NULL){
				char *args_nouvelle_cmd[3];
				args_nouvelle_cmd[0] = "-d";
				args_nouvelle_cmd[2] = NULL;
				
				FILE *fd;
				char str_date[60];
				fd = fopen(fichier_dates, "r"); // Ouverture du fichier
				
				if (fd != NULL){
					while ( fgets (str_date, 60, fd) != NULL ) {
						str_date[strlen(str_date)-1] = '\0';
						args_nouvelle_cmd[1] = str_date;
						if (cmdInt_date (args_nouvelle_cmd))
							erreur = 1;
					}
					fclose(fd);
				}
				else {
					fprintf (stderr, "date : problème lors de l'ouverture du fichier ressayer\n");
					erreur = 1;
				}
			}
		}
		
		if (!erreur && fichier_dates == NULL) {
			if (strstr (format, "%k") != NULL)
				format = remplacer (format, "%k", "%_H");
			
			if (strstr (format, "%l") != NULL)
				format = remplacer (format, "%l", "%_I");
			
			if (strstr (format, "%:z") != NULL || strstr (format, "%::z") != NULL || strstr (format, "%::z") != NULL ) {
				char interpretation_z[256];
				strftime(interpretation_z, sizeof(interpretation_z), "%z", date);
				
				char interpretation_simple_z[6];
				memset (interpretation_simple_z, '\0', 6);
				strcpy(interpretation_simple_z, interpretation_z);
				interpretation_simple_z[5] = interpretation_simple_z[4];
				interpretation_simple_z[4] = interpretation_simple_z[3];
				interpretation_simple_z[3] = ':';
				format = remplacer (format, "%:z", interpretation_simple_z);
				
				char interpretation_double_z[9];
				memset (interpretation_double_z, '\0', 9);
				strcpy(interpretation_double_z, interpretation_simple_z);
				strcat(interpretation_double_z, ":00");
				format = remplacer (format, "%::z", interpretation_double_z);
				
				char interpretation_triple_z[4];
				memset (interpretation_triple_z, '\0', 4);
				strncpy (interpretation_triple_z, interpretation_z, 3);
				format = remplacer (format, "%:::z", interpretation_triple_z);
			}
			if (strstr (format, "%N") != NULL) {
				if (nb_options_date == 0) {
					char nanoSec[10 + 1];
					sprintf(nanoSec, "%lu", timestamp_avec_ns.tv_nsec);
					format = remplacer (format, "%N", nanoSec);
				}
				else
					format = remplacer (format, "%N", ".000000000");
			}
			
			if (option_utc) { // Conversion de la date en UTC+0000
				timestamp = mktime(date);
				date = gmtime(&timestamp);
			}
			
			strftime(buffer, sizeof(buffer), format, date);
			printf("%s\n", buffer);
		}
		else if (fichier_dates == NULL) {
			fprintf (stderr, "date: date incorrecte « %s »\n", date_donnee);
			erreur = 1;
		}
	}
	else if (!erreur && nb_options_format > 1) {
		fprintf (stderr, "date : plusieurs formats de fichiers de sortie indiqués\n");
		erreur = 1;
	}
	else if (!erreur && nb_options_date > 1) {
		fprintf (stderr, "date : les options pour indiquer les dates d'impression sont mutuellement exclusives\n");
		erreur = 1;
	}
	
	return !erreur;
}

int cmdInt_kill (char **args){
	int id_arg = 0;
	int erreur = 0;
	int id_liste_signaux = 0;
	int id_liste_proc = 0;
	int trouve = 0;
	int num_signal = 0;
	int erreur_kill = 0;
	
	while (args[id_arg] != NULL && !erreur) {
		if (id_arg == 1 || id_arg == 2) {
			if (sont_egales(args[id_arg], "-1")) {
				fprintf(stderr, "On ne peut pas tuer tous les processus\n");
				erreur = 1;
			}
		}
		
		if (args[id_arg][0] != '-') {
			id_liste_proc = id_arg;
			while (args[id_liste_proc] != NULL && estNombre(args[id_liste_proc]) && erreur_kill != -1) {
				erreur_kill = kill(atoi(args[id_liste_proc]), 15); //SIGTERM par défaut (15)
				if (erreur_kill != -1)
					id_liste_proc++;
			}
			if (args[id_liste_proc] != NULL && (!estNombre(args[id_liste_proc]) 
																|| erreur_kill == -1)) {
				fprintf(stderr, "Procesus inexistant, PID invalide\n");
				erreur = 1;
			}
			id_arg = id_liste_proc + 1;
		}
		else {
			if (sont_egales(args[id_arg], "-s") || sont_egales(args[id_arg], "--signal")){
				if (args[++id_arg] != NULL) {
					if (estNombre(args[id_arg])) {
						num_signal = atoi(args[id_arg]);
						if (0 < num_signal && num_signal < 32) {
							id_liste_proc = ++id_arg;
							while (args[id_liste_proc] != NULL && estNombre(args[id_liste_proc])  && erreur_kill != -1) {
								erreur_kill = kill(atoi(args[id_liste_proc]), num_signal);
								if (erreur_kill != -1)
									id_liste_proc++;
							}
							if (args[id_liste_proc] != NULL && (!estNombre(args[id_liste_proc]) 
																|| erreur_kill == -1)){
								fprintf(stderr, "Procesus inexistant, PID invalide\n");
								erreur = 1;
							}	
							else
								id_arg = id_liste_proc + 1;
						}
						else {
							fprintf(stderr, "Numéro ou nom de signal invalide\n");
							erreur = 1;
						}
					}
					else {
						trouve = 0;
						id_liste_signaux = 0;
						while (id_liste_signaux < 32 && !trouve) {
							if (sont_egales(LISTE_SIGNAUX[id_liste_signaux], args[id_arg]) ||
							sont_egales(LISTE_SIGNAUX[id_liste_signaux] + 3, args[id_arg])) {
								trouve = 1;
								id_liste_proc = ++id_arg;
								while (args[id_liste_proc] != NULL && estNombre(args[id_liste_proc])  && erreur_kill != -1) {
									erreur_kill = kill(atoi(args[id_liste_proc]), id_liste_signaux);
									if (erreur_kill != -1)
										id_liste_proc++;
								}
								if (args[id_liste_proc] != NULL && (!estNombre(args[id_liste_proc]) 
																|| erreur_kill == -1)) {
									fprintf(stderr, "Procesus inexistant, PID invalide\n");
									erreur = 1;
								}
								else
									id_arg = id_liste_proc + 1;
							}
							else
								id_liste_signaux++;
						}
						if (!trouve) {
							fprintf(stderr, "Numéro ou nom de signal invalide\n");
							erreur = 1;
						}
					}
				}
				else {
					id_arg--;
					fprintf(stderr, "Numéro ou nom de signal invalide\n");
					erreur = 1;
				}
			}
			else if (sont_egales(args[id_arg], "-l") || sont_egales(args[id_arg], "--list")){
				int id_arg_liste_signaux = ++id_arg;
				if (args[id_arg_liste_signaux] == NULL) {
					int it;
					for (it = 1; it < 32; it++)
						printf("%d) %s\n", it, LISTE_SIGNAUX[it]);
				}
				while (args[id_arg_liste_signaux] != NULL) {
					if (estNombre(args[id_arg_liste_signaux]) && (0 < atoi(args[id_arg_liste_signaux]) 
															&& atoi(args[id_arg_liste_signaux]) < 32))
						printf("%s\n", LISTE_SIGNAUX[atoi(args[id_arg_liste_signaux])] + 3);
					else if (!estNombre(args[id_arg_liste_signaux])) {
						trouve = 0;
						id_liste_signaux = 0;
						while (id_liste_signaux < 32 && !trouve) {
							if (sont_egales(LISTE_SIGNAUX[id_liste_signaux], args[id_arg_liste_signaux]) ||
							sont_egales(LISTE_SIGNAUX[id_liste_signaux] + 3, args[id_arg_liste_signaux])) {
								trouve = 1;
								printf("%s\n", LISTE_SIGNAUX[id_liste_signaux] + 3);
							}
							else
								id_liste_signaux++;
						}
						if (!trouve) {
							fprintf(stderr, "Numéro ou nom de signal invalide\n");
							erreur = 1;
						}
					}
					id_arg_liste_signaux++;
				}
				id_arg = id_arg_liste_signaux;
			}
			else if (args[id_arg][0] == '-'){
				if (estNombre(args[id_arg] + 1)) {
					num_signal = atoi(args[id_arg] + 1);
					if (0 < num_signal && num_signal < 32) {
						id_liste_proc = ++id_arg;
						while (args[id_liste_proc] != NULL && estNombre(args[id_liste_proc])  && erreur_kill != -1) {
							erreur_kill = kill(atoi(args[id_liste_proc]), num_signal);
							if (erreur_kill != -1)
								id_liste_proc++;
						}
						if (args[id_liste_proc] != NULL && (!estNombre(args[id_liste_proc]) 
																|| erreur_kill == -1)) {
							fprintf(stderr, "Procesus inexistant, PID invalide\n");
							erreur = 1;
						}
						else
							id_arg = id_liste_proc + 1;
					}
					else {
						fprintf(stderr, "Numéro ou nom de signal invalide\n");
						erreur = 1;
					}
				}
				else {
					trouve = 0;
					id_liste_signaux = 0;
					while (id_liste_signaux < 32 && !trouve) {
						if (sont_egales(LISTE_SIGNAUX[id_liste_signaux], args[id_arg] + 1) ||
							sont_egales(LISTE_SIGNAUX[id_liste_signaux] + 3, args[id_arg] + 1)) {
							trouve = 1;
							id_liste_proc = ++id_arg;
							while (args[id_liste_proc] != NULL && estNombre(args[id_liste_proc])  && erreur_kill != -1) {
								erreur_kill = kill(atoi(args[id_liste_proc]), id_liste_signaux);
								if (erreur_kill != -1)
									id_liste_proc++;
							}
							if (args[id_liste_proc] != NULL && (!estNombre(args[id_liste_proc]) 
																|| erreur_kill == -1)) {
								fprintf(stderr, "Procesus inexistant, PID invalide\n");
								erreur = 1;
							}
							else
								id_arg = id_liste_proc + 1;
						}
						else
							id_liste_signaux++;
					}
					if (!trouve) {
						fprintf(stderr, "Numéro ou nom de signal invalide\n");
						erreur = 1;
					}
				}
			}
		}
	}
	return !erreur;
}

int cmdInt_hostname (char **args) {
	int erreur = 0;
	int stop = 0;
	int id_arg = 0;
	char hostname[HOST_NAME_MAX];
	char *domainname;
	char nisdomainname[256];
	
	while (args[id_arg] != NULL && !stop) {
		if (sont_egales(args[id_arg], "-V") || sont_egales(args[id_arg], "--version")) {
			//Afficher la version et ne pas prendre en compte n'importe quel autre arg
			stop = 1;
		}
		else if (sont_egales(args[id_arg], "-h") || sont_egales(args[id_arg], "--help")) {
			printf("Usage : \n");
			printf("hostname [-b] {hostname|-F fichier}     -      Changer le nom d'hôte (depuis un fichier)\n");
			printf("hostname [-a|-A|-d|-f|-i|-I|-s|-y]      -      Afficher le nom sous un format\n");
			printf("hostname -V|--version|-h|--help         -      Afficher des infos (aide ou version)\n");
			printf("hostname                                -      Afficher le nom d'hôte\n");
			stop = 1;
		}
		else 
			id_arg++;
	}
	
	id_arg = 0;
	while (args[id_arg] != NULL && !sont_egales(args[id_arg], "-V")
								&& !sont_egales(args[id_arg], "-h")
								&& !sont_egales(args[id_arg], "--version")
								&& !sont_egales(args[id_arg], "--help") 
								&& !sont_egales(args[id_arg], "-b") 
								&& !sont_egales(args[id_arg], "--boot")
								&& !sont_egales(args[id_arg], "--file") 
								&& !sont_egales(args[id_arg], "-F")
								&& args[id_arg][0] == '-' && !erreur){
		if (strlen(args[id_arg]) == 1)
			erreur = 1;
		if (args[id_arg + 1] == NULL) {
			if (sont_egales(args[id_arg], "-a") || sont_egales(args[id_arg], "--alias")) {
				// Je ne sais pas à quoi ça correspond
			}
			else if (sont_egales(args[id_arg], "-A") || sont_egales(args[id_arg], "--all-fqdns")) {
				// Pas compris
			}
			else if (sont_egales(args[id_arg], "-d") || sont_egales(args[id_arg], "--domain")) {
				struct hostent *hp;

				gethostname(hostname, sizeof(hostname));
				hp = gethostbyname(hostname);
				domainname = strchr(hp->h_name, '.');
				if ( domainname != NULL ) {
					printf("%s\n", ++domainname);
				}
				else {
					fprintf(stderr, "Nom de domaine introuvable.\n");
					erreur = 1;
				}
			}
			else if (sont_egales(args[id_arg], "-f") || sont_egales(args[id_arg], "--fqdn")
													|| sont_egales(args[id_arg], "--long")) {
				struct addrinfo hints, *info, *p;
				int res;
			
				gethostname(hostname, sizeof(hostname));

				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_flags = AI_CANONNAME;

				if ((getaddrinfo(hostname, "http", &hints, &info)) != 0) {
					fprintf(stderr, "Erreur dans la récupération");
					erreur = 1;
				}

				for(p = info; p != NULL; p = p->ai_next) {
					printf("%s\n", p->ai_canonname);
				}

				freeaddrinfo(info);
			}
			else if (sont_egales(args[id_arg], "-i") || sont_egales(args[id_arg], "--ip-address")) {
				
			}
			else if (sont_egales(args[id_arg], "-I") || sont_egales(args[id_arg], "--all-ip-addresses")) {
				
			}
			else if (sont_egales(args[id_arg], "-s") || sont_egales(args[id_arg], "--short")) {
				// S'arrête au premier '.'
				if (gethostname(hostname, sizeof(hostname)) == 0) {
					int i = 0;
					while (hostname[i] != '.') {
						printf("%c", hostname[i]);
						i++;
					}
					printf("\n");
				}
			}
			else if (sont_egales(args[id_arg], "-y") || sont_egales(args[id_arg], "--yp") || sont_egales(args[id_arg], "--nis")) {
				if (getdomainname(nisdomainname, sizeof(nisdomainname)) == 0)
					printf("%s\n", nisdomainname);
			}
		}
		id_arg++;
	}
	
	id_arg = 0;
	if (args[id_arg] != NULL && (sont_egales(args[id_arg], "-b")
									|| sont_egales(args[id_arg], "--boot")
									|| sont_egales(args[id_arg], "-F")
									|| sont_egales(args[id_arg], "--file")
								    || args[id_arg][0] != '-')) {
		if (sont_egales(args[id_arg], "-F") || sont_egales(args[id_arg], "--file")) {
			if (args[++id_arg] != NULL) {
				int fd = open(args[id_arg], O_RDONLY);
				if (fd != -1) {
					if (estRoot(NULL)) {
						char c;
						while (read(fd, &c, 1)) {
							write(STDOUT_FILENO, &c, 1);
						}/*
						if (strlen(chaine) > 0 && strlen(chaine) < HOST_NAME_MAX) {
							sethostname(chaine, sizeof(chaine));
							printf("%s\n", chaine);
						}
						else {
							fprintf(stderr, "Argument vide ou trop long\n");
							erreur = 1;
						}*/
					}
					else {
						fprintf(stderr, "Il faut être super utilisateur pour modifier le nom d'hôte\n");
						erreur = 1;
					}
					
				}
				else {
					fprintf(stderr, "Fichier inexistant\n");
					erreur = 1;
				}
			}
			else {
				fprintf(stderr, "Entrez un nom de fichier\n");
				erreur = 1;
			}
		}
		else if (args[id_arg][0] != '-' || sont_egales(args[id_arg], "-b")
										|| sont_egales(args[id_arg], "--boot")) {
			if (sont_egales(args[id_arg], "-b") || sont_egales(args[id_arg], "--boot"))
				id_arg++;
			if (estRoot(NULL)) {
				if (args[id_arg] != NULL && strlen(args[id_arg]) < HOST_NAME_MAX)
					sethostname(args[id_arg], sizeof(args[id_arg]));
				else {
					fprintf(stderr, "Argument vide ou trop long\n");
					erreur = 1;
				}
			}
			else {
				fprintf(stderr, "Il faut être super utilisateur pour modifier le nom d'hôte\n");
				erreur = 1;
			}
		}
	}
	
	if (args[0] == NULL) 
	{
		if (gethostname(hostname, sizeof(hostname)) == 0)
			printf("%s\n", hostname);
	}
	
	return !erreur;
}

int cmdInt_exit () {
	return 1;
}

int estUneMachineDistante (char *machine) {
	int it = 0;
	int machine_trouvee = 0;
	while (it < NB_SHELLS_DISTANTS_MAX && SHELLS_DISTANTS[it] != NULL && !machine_trouvee) {
		if (sont_egales(machine, SHELLS_DISTANTS[it]))
			machine_trouvee = 1;
		else
			it++;
	}
	return machine_trouvee;
}


int cmdInt_remote (char **args) {
	int erreur = 0;
	int id_arg = 0;
	int it = 0;
	int status_fils;
	
	int id_premier_arg_cmd;
	
	if (args[id_arg] != NULL) {
		if (sont_egales(args[id_arg], "add")){
			if (args[id_arg + 1] != NULL) {
				id_arg++;
				it = LongueurListe(SHELLS_DISTANTS);
				while (args[id_arg] != NULL && !erreur){
					if (fork()==0) {
						char src[strlen(REPERTOIRE_SHELL) + 1 + strlen("Shell") + 1]; // taille de RepertoireDuShell/Shell (avec '\0')
						strcpy (src, REPERTOIRE_SHELL);
						strcat (src, "/Shell");
						
						char dest[strlen(args[id_arg]) + strlen(":/tmp/Shell") + 1]; // taille de nomMachine:/tmp/Shell (avec '\0')
						strcpy (dest, args[id_arg]);
						strcat (dest, ":/tmp/Shell");
						
						execlp("scp", "scp", src, dest, NULL);
						exit (EXIT_FAILURE);
					}
					else {
						wait(&status_fils);
						if (WEXITSTATUS (status_fils) == EXIT_SUCCESS) {
							SHELLS_DISTANTS[it] = malloc(strlen(args[id_arg]) + 1);
							strcpy(SHELLS_DISTANTS[it], args[id_arg]);
							it++;
							SHELLS_DISTANTS[it] = NULL;
						}
						else {
							char *args_nouvelle_cmd[] = { "remove" };
							cmdInt_remote ( args_nouvelle_cmd );
							erreur = 1; 
							fprintf (stderr, "remote : %s : Machine distante inconnue ou service indisponible\n", args[id_arg]);
						}
					}
					id_arg++;
				}
			}
			else {
				fprintf(stderr, "remote : veuillez entrer un nom de machine distante\n");
				erreur = 1;
			}
		}
		else if (sont_egales(args[id_arg], "remove")){
			it = LongueurListe (SHELLS_DISTANTS) - 1;
			while (it >= 0 && !erreur){
				if (fork() == 0) {
					execlp("ssh", "ssh", SHELLS_DISTANTS[it], "rm", "/tmp/Shell", NULL);
					exit (EXIT_FAILURE);
				}
				else {
					wait(&status_fils);
					
					free (SHELLS_DISTANTS[it]);
					SHELLS_DISTANTS[it] = NULL;
				}
				it--;
			}
		}
		else if (sont_egales(args[id_arg], "list")){
			it = 0;
			while (it < NB_SHELLS_DISTANTS_MAX && SHELLS_DISTANTS[it]) {
				printf("%s\n", SHELLS_DISTANTS[it]);
				it++;
			}
		}
		else if (sont_egales(args[id_arg], "all")){
			it = 0;
			id_arg++;
			if (args[id_arg] != NULL) {
				while (it < NB_SHELLS_DISTANTS_MAX && SHELLS_DISTANTS[it] != NULL && !erreur) {
					if (fork() == 0) {
						id_premier_arg_cmd = id_arg;
						char **args_nouvelle_cmd  = InitialiserListeArguments();
						AjouterArg (args_nouvelle_cmd, "ssh");
						AjouterArg (args_nouvelle_cmd, SHELLS_DISTANTS[it]);
						AjouterArg (args_nouvelle_cmd, "/tmp/Shell");
						while (args[id_premier_arg_cmd] != NULL) {
							AjouterArg (args_nouvelle_cmd, args[id_premier_arg_cmd]);
							id_premier_arg_cmd++;
						}
						
						printf ("\033[1;36;40m---- %s ----\033[0m\n", SHELLS_DISTANTS[it]);
						execvp(args_nouvelle_cmd[0], args_nouvelle_cmd);
						exit (EXIT_FAILURE);
					}
					else {
						wait(&status_fils);
						
						if (!WIFEXITED (status_fils))
							printf ("remote : la commande a été interrompue\n");
					}
					it++;
				}
			}
			else {
				erreur = 1;
				fprintf (stderr, "Veuillez entrez une commande\n");
			}
		}
		else if (estUneMachineDistante(args[id_arg])){
			if (args[id_arg] != NULL) {
				if (fork() == 0) {
					id_premier_arg_cmd = id_arg + 1;
					char **args_nouvelle_cmd  = InitialiserListeArguments();
					AjouterArg (args_nouvelle_cmd, "ssh");
					AjouterArg (args_nouvelle_cmd, args[id_arg]);
					AjouterArg (args_nouvelle_cmd, "/tmp/Shell");
					while (args[id_premier_arg_cmd] != NULL) {
						AjouterArg (args_nouvelle_cmd, args[id_premier_arg_cmd]);
						id_premier_arg_cmd++;
					}
					printf ("\033[1;32;40m---- %s ----\033[0m\n", args[id_arg]);
					execvp(args_nouvelle_cmd[0], args_nouvelle_cmd);
					exit (EXIT_FAILURE);
				}
				else {
					wait(&status_fils);
					
					if (!WIFEXITED (status_fils))
						printf ("remote : la commande a été interrompue\n");
				}
			}
			else {
				erreur = 1;
				fprintf (stderr, "remote : veuillez entrez une commande\n");
			}
		}
		else {
			erreur = 1;
			fprintf (stderr, "remote : %s : option invalide\n", args[id_arg]);
		}
	}
	else {
		erreur = 1;
		fprintf (stderr, "remote : argument nécessaire\n");
		fprintf (stderr, "remote : usage : \tremote add liste-de-machines\n\t\t\tremote remove\n\t\t\tremote list\n\t\t\tremote nom-machine commande-simple\n\t\t\tremote all commande-simple\n");
	}
	
	return !erreur;
}