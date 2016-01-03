#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "Commandes_Internes.h"

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


extern char *remplacer (char *chaine, const char *str_a_remplacer, const char *str_remplacement);

#endif