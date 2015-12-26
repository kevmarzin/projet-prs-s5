#ifndef COMINTERN_H
#define COMINTERN_H
#include "Shell.h"
#include <readline/history.h>
#include <ctype.h>

extern void supprimer_char (char *chaine, int pos);

extern int cmdInt_echo (char **args);
extern int cmdInt_history (char **args);

#endif
