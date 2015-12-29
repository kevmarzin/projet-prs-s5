#include "Commandes_Internes.h"

void supprimer_char (char *chaine, int pos){
	int i = pos;
	while (chaine[i] != '\0'){
		chaine[i] = chaine[i+1];
		i++;
	}
}

int nbArguments (char **tab_args) {
	int id_arg = 0;
	while (tab_args[id_arg] != NULL)
		id_arg++;
	
	return id_arg;
}

int estNombre (char *chaine) {
	int i = 0;
	int chaineEstNombre = 1;
	
	while (chaine[i] != '\0' && chaineEstNombre){
		chaineEstNombre = isdigit(chaine[i]);
		i++;
	}
	
	return chaineEstNombre;
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

int cmdInt_echo(char **args){
	int id_arg = 0;
	int id_arg_debut_affichage = -1;
	
	int saut_de_ligne_final = 1;
	int interpreter_contre_oblique = 0;
	int arret_sortie = 0;
	
	while (args[id_arg] != NULL && !arret_sortie) {
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
		
		if (id_arg_debut_affichage != -1){
			if (id_arg > id_arg_debut_affichage)
				printf (" ");
			
			char *affichage = args[id_arg];
			supprimer_contre_oblique_echo (affichage, interpreter_contre_oblique, &arret_sortie);
			
			printf ("%s", affichage);
		}
		
		if (!arret_sortie)
			id_arg++;
	}
	
	if (saut_de_ligne_final && !arret_sortie)
		printf ("\n");
	
	return 1;
}

int cmdInt_history (char **args) {
	HISTORY_STATE *etat_hist = history_get_history_state();
	HIST_ENTRY **hists = etat_hist->entries;
	int taille_hist = etat_hist->length;
	
	int nb_lignes_specifie = 0;
	int nb_args = nbArguments(args);
	
	int erreur = 0;
	int clear = 0;
	int elem_a_suppr = -1;
	
	if (args[0] != NULL && args[0][0] == '-' && strlen(args[0]) > 1) {
		int id_arg = 0;
		int id_char = 0;
		int verif_chaine_finie = 0;
		int longueur_arg;
		
		while (!erreur && args[id_arg] != NULL) {
			int id_char = 0;
			if (args[id_arg][id_char] == '-') {
				longueur_arg = strlen(args[id_arg]);
				id_char++;
				while (!erreur && !verif_chaine_finie 	&& args[id_arg] != NULL 
														&& args[id_arg][id_char] != '\0'
														&& id_char < longueur_arg) {
					if (args[id_arg][id_char] == 'c') {
						clear = 1;
					}
					else if (args[id_arg][id_char] == 'd' && args[id_arg][id_char+1] != '\0') {
						if (estNombre (args[id_arg] + (id_char+1))) {
							elem_a_suppr = atoi (args[id_arg] + (id_char+1));
							if (elem_a_suppr < 1 || elem_a_suppr > taille_hist) {
								erreur = 1;
								fprintf (stderr, "history : %d : position dans l'historique hors plage\n", elem_a_suppr);
							}
							else
								verif_chaine_finie = 1;
						}
						else {
							fprintf (stderr, "history : %s : argument numérique nécessaire\n", args[id_arg] + (id_char+1));
							erreur = 1;
						}
					}
					else if (args[id_arg][id_char] == 'd') { 
						id_arg ++;
						if (args[id_arg] == NULL) {
							fprintf (stderr, "history : -d : l'option a besoin d'un argument\n");
							erreur = 1;
						}
						else if (estNombre (args[id_arg])) {
							elem_a_suppr = atoi (args[id_arg]);
							if (elem_a_suppr < 1 || elem_a_suppr > taille_hist) {
								erreur = 1;
								fprintf (stderr, "history : %d : position dans l'historique hors plage\n", elem_a_suppr);
							}
						}
						else {
							fprintf (stderr, "history : %s : position dans l'historique hors plage\n", args[id_arg]);
							erreur = 1;
						}
					}
					else {
						erreur = 1;
						fprintf (stderr, "history : -%c : option non valable\n", args[id_arg][id_char]);
						fprintf (stderr, "history : utilisation : history [-c] [-d décalage] [n] \n\t\t\tou history -anrw [nomfichier] \n\t\t\tou history -ps arg [arg...]\n");
					}
					id_char++;
				}
			}
			
			if (args[id_arg] != NULL){
				id_arg++;
			}	
		}
	}
	else if ((nb_lignes_specifie = (args[0] != NULL && estNombre(args[0]))) || args[0] == NULL) {
		if (nb_args > 1)
			fprintf (stderr, "history : trop d'arguments");
		else {
			int id_hist = 0;
			
			if (nb_lignes_specifie) {
				id_hist = taille_hist - atoi(args[0]);
				if (id_hist < 0)
					id_hist = 0;
			}
			
			while (hists[id_hist] != NULL && taille_hist > 0) {
				printf("    %d\t%s\n", id_hist + 1, hists[id_hist]->line);
				id_hist++;
			}
		}
	}
	else {
		fprintf (stderr, "history : %s : argument numérique nécessaire", args[0]);
	}
	
	if (!erreur){
		if (clear){
			clear_history();
		}
		else if (elem_a_suppr != -1){
			free_history_entry (remove_history(elem_a_suppr-1));
		}
	}
	
	return 1;
}

int cmdInt_date (char **args){
	int id_arg = 0;
	
	char *afficher_date = NULL;
	char *modifier_date = NULL;
	
	char *format = NULL;
	int nb_format = 0;
	
	int nb_date = 0;
	char *fichier_date_modification = NULL;
	
	/*
	 * rfc-3339
	 * date: argument «  » ambigu pour « --rfc-3339 »
		Les arguments valables sont :
		- « date »
		- « seconds »
		- « ns »
*/
	/*
	 * rfc-3339 prioritaire sur -d
	 */
	int erreur = 0;
	
	while (args[id_arg] != NULL && !erreur) {
		if (strcmp(args[id_arg], "-d") == 0 || strcmp(args[id_arg], "--date=") == 0)
			;//afficher_date = args[++id_arg];
		else if (strcmp(args[id_arg], "-f") == 0 || strcmp(args[id_arg], "--file=") == 0)
			;//modifier_date = args[++id_arg];
		else if (strcmp(args[id_arg], "-r") == 0 || strstr(args[id_arg], "--reference") != NULL) {
			nb_date++;
			if (strcmp(args[id_arg], "-r") == 0 && args[id_arg + 1] != NULL){
				fichier_date_modification = args[++id_arg];
			}
			else if (strstr(args[id_arg], "--reference") != NULL){
				if( strstr(args[id_arg], "--reference=") == NULL){
					fprintf (stderr, "date : l'option « %s » requiert un argument\n", args[id_arg]);
					erreur = 1;
				}
				else {
					fichier_date_modification = (args[id_arg] + strlen("--reference="));
				}
			}
			
			FILE* fic;
			if (!erreur && (fic = fopen(fichier_date_modification, "r")) != NULL ) {
				fclose (fic);
			}
			else if (!erreur) {
				erreur = 1;
				fprintf (stderr, "date : %s : Aucun fichier ou dossier de ce type\n", args[id_arg]);
			}
		}
		else if (strcmp(args[id_arg], "-R") == 0 || strcmp(args[id_arg], "--rfc-2822") == 0 || strstr(args[id_arg], "--rfc-2822=") != NULL) {
			if (strstr(args[id_arg], "--rfc-2822=") != NULL) {
				fprintf (stderr, "date : l'option « --rfc-2822 » ne permet pas d'argument\n");
				erreur = 1;
			}
			else {
				nb_format++;
				format = "%a, %d %b %Y %X %z";
			}
		}
		else if (strstr(args[id_arg], "--rfc-3339") != NULL || strstr(args[id_arg], "--rfc-3339=") != NULL ) {
			if (strstr(args[id_arg], "--rfc-3339=") == NULL) {
				erreur = 1;
				fprintf (stderr, "date : l'option « %s » requiert un argument\n", args[id_arg]);
			}
			else {
				char* format_rfc = (args[id_arg] + strlen("--rfc-3339="));
				if (format_rfc[0] != '\0' &&   (strcmp (format_rfc, "date") == 0
											||	strcmp (format_rfc, "seconds") == 0
											||	strcmp (format_rfc, "ns") == 0)){
					nb_format++;
					if (strcmp (format_rfc, "date") == 0)
						format = "%F";
					else if (strcmp (format_rfc, "seconds") == 0)
						format = "%F %X%z";
					else if (strcmp (format_rfc, "ns") == 0)
						format = "XXXXXXXXXX";
				}
				else {
					erreur = 1;
					fprintf (stderr, "date : argument « %s » incorrect pour « --rfc-3339 »\nLes arguments valables sont :\n  - « date »\n  - « seconds »\n  - « ns »\n", format_rfc);
				}
			}
		}
		else if (strcmp(args[id_arg], "-s") == 0 || strcmp(args[id_arg], "--set=") == 0)
			;
		else if (strcmp(args[id_arg], "-u") == 0 || strcmp(args[id_arg], "--utc=") == 0 || strcmp(args[id_arg], "--universel=") == 0)
			;
		else if (args[id_arg][0] == '+') {
			format = args[id_arg] + 1;
			nb_format++;
		}
			
		else {
			erreur = 1;
			fprintf (stderr, "date: date incorrecte « %s »\n", args[id_arg]);
		}
	
		id_arg++;
	}
	
	
	
	if (!erreur && (nb_format == 0 || nb_format == 1) && (nb_date == 0 || nb_date == 1)) {
		char buffer[256];
		time_t timestamp;
		
		if (nb_format == 0)
			format = "%A %d %B %Y, %X (UTC%z)";
					
		if (nb_date == 0)
			timestamp = time(NULL);
		else if (nb_date == 1) {
			struct stat buf;
			if (fichier_date_modification != NULL && stat (fichier_date_modification, &buf) == 0)
				timestamp = buf.st_mtime;
		}
			
		strftime(buffer, sizeof(buffer), format, localtime(&timestamp));
		printf("%s\n", buffer);
	}
	else if (!erreur && nb_format > 1)
		fprintf (stderr, "date : plusieurs formats de fichiers de sortie indiqués\n");
	else if (!erreur && nb_date > 1)
		fprintf (stderr, "date : les options pour indiquer les dates d'impression sont mutuellement exclusives\n");
	return 1;
}