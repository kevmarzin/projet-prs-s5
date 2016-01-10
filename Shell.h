#ifndef ANALYSE
#define ANALYSE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define NB_ARGS 50
#define TAILLE_ID 500

#define NB_SIGNAUX 32
#define NB_CMDS_BG_MAX 100
#define NB_MACHINES_DISTANTES_MAX 15

/*
 * Variable permettant de quitter le programme quand elle est égale à 1.
 */
int exit_prog;

/*
 * Tableaux stockant les signaux. Constantes.
 */
const char* LISTE_SIGNAUX[NB_SIGNAUX];

/*
 * Stocke le répertoire de l'exécutable du shell. Initialisée dans le main mais ne change pas au cours du programme.
 */
char* REPERTOIRE_SHELL;

/*
 * Stocke le pid du processus de la commande (externe) lancée en avant plan. -1 lorsqu'il n'y a pas processus en avant plan.
 */
pid_t pid_avant_plan;

/*
 * Tableaux stockant les pid des processus des commandes lancées en BG, ainsi que les chaine de caractères des commandes.
 */
pid_t pids_bg [NB_CMDS_BG_MAX];
char *cmds_bg [NB_CMDS_BG_MAX];

/*
 * Tableau permettant de stocker les noms des machines distantes ajouter grâce au remote.
 */
char *machines_distantes_liees [NB_MACHINES_DISTANTES_MAX];

char *get_current_dir_name(void);

typedef enum expr_t {
	VIDE,			// Commande vide 
	SIMPLE,			// Commande simple 
	SEQUENCE,		// Séquence (;) 
	SEQUENCE_ET,	// Séquence conditionnelle (&&) 
	SEQUENCE_OU,	// Séquence conditionnelle (||) 
	BG,				// Tache en arriere plan 
	PIPE,			// Pipe 
	REDIRECTION_I,	// Redirection entree 
	REDIRECTION_O,	// Redirection sortie standard 
	REDIRECTION_A,	// Redirection sortie standard, mode append 
	REDIRECTION_E,	// Redirection sortie erreur 
	REDIRECTION_EO,	// Redirection sorties erreur et standard
	SOUS_SHELL,		// ( shell ) 
} expr_t;

typedef struct Expression {
	expr_t type;
	struct Expression *gauche;
	struct Expression *droite;
	char   **arguments;
} Expression;

extern int yyparse(void);
Expression *ConstruireNoeud (expr_t, Expression *, Expression *, char **);
char **AjouterArg (char **, char *);
char **InitialiserListeArguments (void);
int LongueurListe(char **);
void EndOfFile(void);

void yyerror (char *s);
Expression *ExpressionAnalysee;
extern bool interactive_mode;
extern int status;
#endif /* ANALYSE */
