#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "Commandes_Internes.h"

/*
 * Applique la fonction tolower à tout les caractère de la chaine src et renvoie le résultat
 */
extern char *str_to_lower (const char* src);

/*
 * Split la chaine src en fonction du separateur. Résultat renvoyé dans un tableau avec la dernière case égale à NULL
 */
extern char **split_str (char* src, const char *separateur);

/*
 * Renvoie 1 si la chaine src finie par la chaine de caractère sufixe, 0 sinon.
 */
extern int finie_par (const char *src, const char *sufixe);

/*
 * Renvoie 1 si la chaine src commence par la chaine de caractère prefixe, 0 sinon.
 */
extern int commence_par (const char *src, const char *prefixe);

/*
 * Renvoie 1 si les chaines src1 et src2 sont les mêmes, 0 sinon.
 */
extern int sont_egales (const char *src1, const char *src2);

/*
 * Supprime le caractère à une position pos de la chaine si c'est une position valide.
 */
extern void supprimer_char (char *chaine, int pos);

/*
 * Renvoie le nombre d'arguments d'un tableau d'argument (finissant par une case = NULL)
 */
extern int nbArguments (char **tab_args);

/*
 * Si la chaine est uniquement composée de chiffre, 1 est renvoyé sinon 0
 */
extern int estNombre (char *chaine);

/*
 * Remplace toutes les occurrences de str_a_remplacer par str_remplacement dans chaîne.
 * Renvoie le résultat si des occurrences sont trouvées sinon la chaîne source.
 * Si la chaine à remplacer commence par '%' et ses occurrences sont précédées par '%', la chaine n'est pas remplacée.
 */
extern char *remplacer (char *chaine, const char *str_a_remplacer, const char *str_remplacement);

#endif