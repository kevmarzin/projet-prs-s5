#ifndef COMINTERN_H
#define COMINTERN_H

#include <time.h>
#include <stdlib.h>

/*
 * Pour enlever le warning "implicit declaration of function..."
 */
int sethostname(const char *, size_t);
int getdomainname(char *, size_t);
int stime(const time_t *);

/*
 * Commande echo.
 */
extern int cmdInt_echo (char **);

/*
 * Commande pwd.
 */
extern int cmdInt_pwd (char **);

/*
 * Commande history.
 */
extern int cmdInt_history (char **);

/*
 * Commande date.
 */
extern int cmdInt_date (char **);

/*
 * Commande kill.
 */
extern int cmdInt_kill (char **);

/*
 * Commande hostname.
 */
extern int cmdInt_hostname (char **);

/*
 * Commande exit.
 */
extern int cmdInt_exit (void);

/*
 * Commande remote.
 */
extern int cmdInt_remote (char **);

#endif
