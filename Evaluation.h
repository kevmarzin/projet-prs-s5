#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "Shell.h"

extern void supprimer_char (char *chaine, int pos);

extern int evaluer_expr_simple (char **args);
extern int evaluer_expr (Expression *e);

#endif
