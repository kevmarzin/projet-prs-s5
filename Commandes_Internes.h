#ifndef COMINTERN_H
#define COMINTERN_H

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

/*
 * Commande hostname.
 */
extern int cmdInt_hostname (char **args);

/*
 * Commande exit.
 */
extern int cmdInt_exit ();

/*
 * Commande remote.
 */
extern int cmdInt_remote (char **args);

#endif
