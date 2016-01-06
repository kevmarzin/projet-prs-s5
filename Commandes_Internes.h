#ifndef COMINTERN_H
#define COMINTERN_H
#include "Shell.h"

#include <ctype.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h>
#include <signal.h>

/*
 * Commande echo.
 */
extern int cmdInt_echo (char **args);

/*
 * Commande history.
 */
extern int cmdInt_history (char **args);

/*
 * Commande date.
 */
extern int cmdInt_date (char **args);

/*
 * Commande kill.
 */
extern int cmdInt_kill (char **args);

#endif
