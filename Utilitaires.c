#include "Utilitaires.h"

void supprimer_char (char *chaine, int pos){
	int i = pos;
	if (pos < strlen(chaine))
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

char *remplacer (char *chaine, const char *str_a_remplacer, const char *str_remplacement) {
    char *sous_chaine = NULL;
	char *res = NULL;

	int taille_strARemplacer = strlen(str_a_remplacer);	
	int cpt = 0;
	int pos_sous_chaine = 0;
	
    //On compte le nombre d'occurrences de str_a_remplacer dans chaine
	sous_chaine = strstr (chaine, str_a_remplacer);
	while (sous_chaine != NULL) {
		cpt++;
		pos_sous_chaine = (sous_chaine - chaine);
		
		if (pos_sous_chaine + taille_strARemplacer >= strlen(chaine))
			sous_chaine = NULL;
		else
			sous_chaine = strstr (sous_chaine + taille_strARemplacer, str_a_remplacer);
	}
	
	//Si on a trouvé au moins un str_a_remplacer
	if (cpt > 0) {
		int taille_retour = ( strlen(str_remplacement) - taille_strARemplacer ) * cpt + strlen(chaine);
		
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
			pos_curseur = pos_sous_chaine + taille_strARemplacer;
			
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