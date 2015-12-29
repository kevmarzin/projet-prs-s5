#ifndef COMINTERN_H
#define COMINTERN_H
#include "Shell.h"
#include <readline/history.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h>

extern void supprimer_char (char *chaine, int pos);

extern int cmdInt_echo (char **args);
extern int cmdInt_history (char **args);
extern int cmdInt_date (char **args);

#endif
