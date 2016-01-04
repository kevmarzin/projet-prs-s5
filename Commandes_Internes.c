#include "Commandes_Internes.h"
#include "Utilitaires.h"

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
		char **mots = split_str(date_donnee, " ");
		if (mots[0] != NULL && sont_egales (str_to_lower(mots[0]), "next") && mots[1] != NULL)
			erreur = next_date (date, str_to_lower (mots[1]));
		else if (mots[0] != NULL && finie_par(str_to_lower(date_donnee), "ago") && nbArguments(mots) == 3 && estNombre (mots[0]))
			erreur = ago_date (date, atoi(mots[0]), str_to_lower(mots[1]));
			
	}
	
	if (erreur) {
		date = NULL;
		fprintf (stderr, "date: date incorrecte « %s »\n", date_donnee);
	}
	
	return date;
}

int ago_date (struct tm *date, int nbr, char *mot){
	if(sont_egales(mot, "year"))
		date->tm_year -= nbr;
	else if(sont_egales(mot, "day"))
		date->tm_mday -= nbr;
	else if(sont_egales(mot, "month"))
		date->tm_mon -= nbr;
	else if(sont_egales(mot, "hour"))
		date->tm_hour -= nbr;
	else if(sont_egales(mot, "minute"))
		date->tm_min -= nbr;
	else if(sont_egales(mot, "second"))
		date->tm_sec -= nbr;
	else
		return 0;
	
	return 1;
}

int next_date (struct tm *date, char *mot){
	if(sont_egales(mot, "year"))
		date->tm_year++;
	else if(sont_egales(mot, "day"))
		date->tm_mday++;
	else if(sont_egales(mot, "month"))
		date->tm_mon++;
	else if(sont_egales(mot, "hour"))
		date->tm_hour++;
	else if(sont_egales(mot, "minute"))
		date->tm_min++;
	else if(sont_egales(mot, "second"))
		date->tm_sec++;
	else
		return 0;
	
	return 1;
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
void cmdInt_echo(char **args){
	int id_arg = 0;
	int id_arg_debut_affichage = -1;
	
	int saut_de_ligne_final = 1;
	int interpreter_contre_oblique = 0;
	int arret_sortie = 0;
	
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
		if (sont_egales (args[id_arg], "-d")|| sont_egales (args[id_arg], "--date") 
											|| (commence_par (args[id_arg], "--date") 
												&& args[id_arg][strlen("--date")] == '=')){
			nb_date++;
			if (sont_egales (args[id_arg], "-d")|| sont_egales (args[id_arg], "--date") 
												|| (commence_par (args[id_arg], "--date") 	&& (strlen (args[id_arg]) == strlen ("--date="))
																							&& args[id_arg + 1] != NULL && args[id_arg + 1][0] != '-'))
				date_donnee = args[++id_arg];
			else if (commence_par (args[id_arg], "--date") && (strlen (args[id_arg]) > strlen ("--date=")))
				date_donnee = args[id_arg] + strlen ("--date=");
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
				format = "%F %H%z";
			else if (strcmp(argument_iso8601, "minutes") == 0)
				format = "%F %R%z";
			else if (strcmp(argument_iso8601, "seconds") == 0)
				format = "%F %T%z";
			else if (strcmp(argument_iso8601, "ns") == 0)
				format = "%F %R,%N%z";
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
		}
		
		if (!erreur) {
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
		else {
			fprintf (stderr, "date: date incorrecte « %s »", date_donnee);

		}
	}
	else if (!erreur && nb_format > 1)
		fprintf (stderr, "date : plusieurs formats de fichiers de sortie indiqués\n");
	else if (!erreur && nb_date > 1)
		fprintf (stderr, "date : les options pour indiquer les dates d'impression sont mutuellement exclusives\n");
	return 1;
}

void cmdInt_kill (char **args) 
{
    int id_arg = 0;
    int erreur = 0;
    int id_liste_signaux = 0;
	
	while (args[id_arg] != NULL && !erreur) {
        if (args[id_arg][0] == '-')
        {
            if (estNombre(args[id_arg] + 1)) {
                int num_signal = atoi(args[id_arg] + 1);
                if (0 < num_signal && num_signal < 32)
                    kill(atoi(args[id_arg + 1]), num_signal);
                else
                    fprintf(stderr, "EINVAL");
            }
            else {
                int trouve = 0;
                while (id_liste_signaux < 32 && !trouve) {
                    if (sont_egales(strsignal(id_liste_signaux), args[id_arg] + 1)) {
                        trouve = 1;
                        kill(atoi(args[id_arg + 1]), id_liste_signaux);
                    }
                    else
                        id_liste_signaux++;
                }
                if (!trouve)
                    fprintf(stderr, "EINVAL");
            }
        }
		if (sont_egales(args[ig_arg], "-s") || sont_egales(args[id_arg], "--signal"))
        {
            if (args[id_arg + 1] != NULL && estNombre(args[id_arg + 1])) {
                int num_signal = atoi(args[id_arg + 1]);
                if (0 < num_signal && num_signal < 32)
                    kill(atoi(args[id_arg + 2]), num_signal);
                else
                    fprintf(stderr, "EINVAL");
            }
            
        }
        if (sont_egales(args[ig_arg], "-l") || sont_egales(args[id_arg], "--list"))
        {
            int ig_arg_liste_signaux = id_arg;
            int num_signal = atoi(args[id_arg_liste_signaux]);
            while (args[id_arg_liste_signaux] != NULL) 
            {
                if (estNombre(num_signal) && (0 < num_signal && num_signal < 32))
                    printf(strsignal(num_signal) + 3);
                else if (!estNombre(num_signal)) {
                    int trouve = 0;
                    id_liste_signaux = 0;
                    while (id_liste_signaux < 32 && !trouve) {
                        if (sont_egales(strsignal(id_liste_signaux), num_signal)) {
                            trouve = 1;
                            printf(strsignal(id_liste_signaux) + 3);
                        }
                        else
                            id_liste_signaux++;
                    }
                }
            
            }
	
		id_arg++;
        }
    }
}
