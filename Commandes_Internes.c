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

/*
 * Remplace toutes les occurrences de str_a_remplacer par str_remplacement dans chaîne
 * Renvoie le résultat si des occurrences sont trouvées sinon la chaîne source
 * Si les chaînes à remplacer commence par 
 */
char *remplacer (char *chaine, const char *str_a_remplacer, const char *str_remplacement) {
    char *sous_chaine = NULL;
	char *res = NULL;

	int taille_strRemplacee = strlen(str_a_remplacer);	
	int cpt = 0;
	int pos_sous_chaine = 0;
    //On compte le nombre d'occurrences de str_a_remplacer dans chaîne
	sous_chaine = strstr (chaine, str_a_remplacer);
	
	while (sous_chaine != NULL) {
		cpt++;
		pos_sous_chaine = (sous_chaine - chaine);
		
		if (pos_sous_chaine + taille_strRemplacee >= strlen(chaine))
			sous_chaine = NULL;
		else
			sous_chaine = strstr (sous_chaine + taille_strRemplacee, str_a_remplacer);
	}
	
	if (cpt > 0) {
		int taille_retour = ( strlen(str_remplacement) - taille_strRemplacee ) * cpt + strlen(chaine);
		
		//allocation de la memoire pour le résultat et initialisation
		res = malloc(taille_retour);
		strcpy (res, "");
		
		int pos_curseur = 0;
		sous_chaine = strstr (chaine, str_a_remplacer);
		
		//Tant que on trouve la chaîne à remplacer, on extrait la sous chaîne qui commence par la chaîne à remplacer
		while (sous_chaine != NULL) {
			//Calcul de la position de la sous chaîne trouvée dans la chaîne de départ
			pos_sous_chaine = sous_chaine - chaine;
			
			//On met dans le résultat ce qu'il y a entre la dernière chaîne à remplacer trouvée et la sous chaîne trouvée (exclues)
			strncat (res, chaine + pos_curseur, (pos_sous_chaine - pos_curseur));
			
			//Si la chaine ne contient pas à la position donnée une chaine du type "%%x" ou "%x est la chaîne à remplacer,
			//on effectue le remplacement sinon on passe la chaîne à remplacer
			if (str_a_remplacer[0] != '%' || chaine[pos_sous_chaine-1] != '%') 
				//On met dans le résultat la chaîne de remplacement
				strcat (res, str_remplacement);
			else 
				//On met dans le résultat la chaîne à remplacer privée de son premier '%'
				strcat (res, str_a_remplacer);
			
			//On passe la chaîne à remplacer
			pos_curseur = pos_sous_chaine + taille_strRemplacee;
			
			//Si on trouve une nouvelle occurrence de la chaîne à remplacer, on extrait la sous chaîne qui commence
			//par la chaîne à remplacer et on continue
			if (strstr (chaine + pos_curseur, str_a_remplacer) != NULL)
				sous_chaine = strstr (chaine + pos_curseur, str_a_remplacer);
			//Sinon on écrit ce qu'il reste dans la chaîne et on arrête la boucle
			else {
				strcat (res, chaine + pos_curseur);
				sous_chaine = NULL;
			}
		}
	}
	
    return (res != NULL) ? res : chaine;
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
	char *date_donnee = NULL;
	char *argument_iso8601 = NULL;
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
		if (strcmp(args[id_arg], "-d") == 0 || strcmp(args[id_arg], "--date=") == 0) {
			nb_date++;
			if (args[id_arg + 1] != NULL && args[id_arg + 1][0] != '-')
				date_donnee = args[++id_arg];
			else
				date_donnee = "";
		}
		else if (strcmp(args[id_arg], "-f") == 0 || strcmp(args[id_arg], "--file=") == 0)
			;//modifier_date = args[++id_arg];
		// si ça commence par -I OU (si ça commence par iso-8601 ET (si ça commence par iso-8601 et que l'argument est plus long, il y a forcément un '=' qui suit))
		else if (	(strlen(args[id_arg]) >= 2 	&& args[id_arg][0] == '-' 
												&& args[id_arg][1] == 'I') 
					|| ((strstr(args[id_arg], "--iso-8601") != NULL && (strlen(strstr(args[id_arg], "--iso-8601")) == strlen(args[id_arg])))
						&& ((strlen(args[id_arg]) > strlen("--iso-8601") && args[id_arg][strlen("--iso-8601")] == '=') 
							|| strlen(args[id_arg]) == strlen("--iso-8601")))) {
			nb_format++;
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
		else if (strcmp(args[id_arg], "-r") == 0 || strstr(args[id_arg], "--reference") != NULL) {
			nb_date++;
			if ((strcmp(args[id_arg], "-r") == 0 && args[id_arg + 1] != NULL) || 
						(strcmp (args[id_arg], "--reference") == 0 && args[id_arg + 1] != NULL))
				fichier_date_modification = args[++id_arg];
			else if ((strcmp(args[id_arg], "-r") == 0 && args[id_arg + 1] != NULL) || 
						(strcmp (args[id_arg], "--reference") == 0 && args[id_arg + 1] == NULL)){
				erreur = 1;
				fprintf (stderr, "date : l'option « %s » requiert un argument\n", args[id_arg]);
			}
			else if (strcmp(args[id_arg], "--reference=") == 0 && args[id_arg + 1] != NULL)
					fichier_date_modification = args[++id_arg];
			else if (strstr(args[id_arg], "--reference=") == 0 && strlen ("--reference=") < strlen (args[id_arg]))
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
				if (format_rfc[0] == '\0' && args[id_arg + 1][0] != '-')
					format_rfc = args[++id_arg];
					
				if (format_rfc[0] != '\0' &&   (strcmp (format_rfc, "date") == 0
											||	strcmp (format_rfc, "seconds") == 0
											||	strcmp (format_rfc, "ns") == 0)){
					nb_format++;
					if (strcmp (format_rfc, "date") == 0)
						format = "%F";
					else if (strcmp (format_rfc, "seconds") == 0)
						format = "%F %X%z";
					else if (strcmp (format_rfc, "ns") == 0)
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
		struct tm *date;
		struct timespec timestamp_avec_ns;
		
		if (nb_format == 0)
			format = "%A %d %B %Y, %X (UTC%z)";
		else if (argument_iso8601 != NULL) {
			if (strcmp(argument_iso8601, "date") == 0)
				format = "%F";
			else if (strcmp(argument_iso8601, "hours") == 0)
				format = "%FT%H%z";
			else if (strcmp(argument_iso8601, "minutes") == 0)
				format = "%FT%R%z";
			else if (strcmp(argument_iso8601, "seconds") == 0)
				format = "%FT%T%z";
			else if (strcmp(argument_iso8601, "ns") == 0)
				format = "%FT%R,%N%z";
		}
			
		if (nb_date == 0) {
			clock_gettime (CLOCK_REALTIME, &timestamp_avec_ns);
			timestamp = timestamp_avec_ns.tv_sec;
			date = localtime(&timestamp);
		}
		else if (nb_date == 1) {
			struct stat buf;
			if (fichier_date_modification != NULL && stat (fichier_date_modification, &buf) == 0) {
				timestamp = buf.st_mtime;
				date = localtime(&timestamp);
			}
			else if (date_donnee != NULL) {
				if (date_donnee[0] == '@') {
					timestamp = atoi(date_donnee + 1);
					date = localtime(&timestamp);
				}
				else if (date_donnee[0] == '\0'){
					timestamp = time(NULL);
					date = localtime(&timestamp);
					date->tm_sec = 0;
					date->tm_min = 0;
					date->tm_hour = 0;
				}
			}
		}
		
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
			if (nb_date == 0) {
				char nanoSec[10 + 1];
				sprintf(nanoSec, "%lu", timestamp_avec_ns.tv_nsec);
				format = remplacer (format, "%N", nanoSec);
			}
			else
				format = remplacer (format, "%N", ".000000000");
		}
		
		strftime(buffer, sizeof(buffer), format, date);
		printf("%s\n", buffer);
	}
	else if (!erreur && nb_format > 1)
		fprintf (stderr, "date : plusieurs formats de fichiers de sortie indiqués\n");
	else if (!erreur && nb_date > 1)
		fprintf (stderr, "date : les options pour indiquer les dates d'impression sont mutuellement exclusives\n");
	return 1;
}