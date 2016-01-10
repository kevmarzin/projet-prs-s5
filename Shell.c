/* Construction des arbres représentant des commandes */

#include "Shell.h"
#include "Affichage.h"
#include "Evaluation.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

const char* LISTE_SIGNAUX[NB_SIGNAUX] = {	
	"Aucun signal", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT",
	"SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE",
	"SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP",
	"SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF",
	"SIGWINCH", "SIGIO", "SIGPWR", "SIGSYS"
};

char *REPERTOIRE_SHELL;
char *chaine_cmd_distante = NULL;
pid_t pid_avant_plan = -1;

extern int yyparse_string (char *);

int EXIT_PROG = 0;
bool interactive_mode = 1;	// par défaut on utilise readline 
int status = 0;			// valeur retournée par la dernière commande

/*
 * Construit une expression à partir de sous-expressions
 */
Expression *ConstruireNoeud (expr_t type, Expression * g, Expression * d, char **args){
	Expression *e;
	if ((e = (Expression *) malloc (sizeof (Expression))) == NULL){
		perror ("malloc");
		exit (EXIT_FAILURE);
	}

	e->type = type;
	e->gauche = g;
	e->droite = d;
	e->arguments = args;
	return e;
}/* ConstruireNoeud */


/*
 * Renvoie la longueur d'une liste d'arguments
 */
int LongueurListe (char **l){
	char **p;
	for (p = l; *p != NULL; p++)
		;
	return p - l;
}/* LongueurListe */


/*
 * Renvoie une liste d'arguments, la première case étant initialisée à NULL, la
 * liste pouvant contenir NB_ARGS arguments (plus le pointeur NULL de fin de
 * liste)
 */
char **InitialiserListeArguments (void){
	char **l;
	
	l = (char **) (calloc (NB_ARGS + 1, sizeof (char *)));
	*l = NULL;
	return l;
}/* InitialiserListeArguments */


/*
 * Ajoute en fin de liste le nouvel argument et renvoie la liste résultante
 */
char **AjouterArg (char **Liste, char *Arg){
	char **l;
	
	l = Liste + LongueurListe (Liste);
	*l = (char *) (malloc (1 + strlen (Arg)));
	strcpy (*l++, Arg);
	*l = NULL;
	return Liste;
}/* AjouterArg */

/*
 * Fonction appelée lorsque l'utilisateur tape "".
 */
void EndOfFile (void){
  exit (0);
}/* EndOfFile */

/*
 * Appelée par yyparse() sur erreur syntaxique
 */
void yyerror (char *s){
  fprintf (stderr, "%s\n", s);
}


/*
 * Libération de la mémoire occupée par une expression
 */
void expression_free (Expression * e){
	if (e == NULL)
		return;

	expression_free (e->gauche);
	expression_free (e->droite);

	if (e->arguments != NULL){
		for (int i = 0; e->arguments[i] != NULL; i++)
			free (e->arguments[i]);
		free (e->arguments);
	}

	free (e);
}


/*
 * Lecture de la ligne de commande à l'aide de readline en mode interactif
 * Mémorisation dans l'historique des commandes
 * Analyse de la ligne lue 
 */
int my_yyparse (char *cmd){
	if (interactive_mode){
		char *line = NULL;
		char buffer[1024];
		snprintf (buffer, 1024, "\033[1;32;40mmini_shell(%d):\033[0m", status);
		line = readline (buffer);
		if (line != NULL){
			int ret;
			add_history (line);	// Enregistre la line non vide dans l'historique courant
			*strchr (line, '\0') = '\n';	// Ajoute \n à la line pour qu'elle puisse etre traité par le parseur
			ret = yyparse_string (line);	// Remplace l'entrée standard de yyparse par s
			free (line);
			return ret;
		}
		else{
			EndOfFile ();
			return -1;
		}
	}
	else{
		// pour le mode distant par exemple
		int ret;
		int c;
		char *line = cmd;
		if (line != NULL) {
			*strchr (line, '\0') = '\n';
			ret = yyparse_string (line);
			free (line);
			return ret;
		}
	}
}

/*
 * Libère la mémoire de toutes les structures de données si cette libération n'a pas été faite.
 */
void free_structures () {
	int i = 0;
	
	free (REPERTOIRE_SHELL);
	
	if (interactive_mode) {
		i = 0;
		
		// Fin de toutes les commandes lancées en fond de tâche
		while (i < NB_CMDS_BG_MAX && PIDS_BG [i] != -1) {
			// Termine la commande i
			kill (PIDS_BG [i], SIGTERM);
			waitpid (PIDS_BG [i], NULL, WNOHANG);
			
			// Affichage de la fin
			printf ("[%d]   Fini\t\t\t%s\n", i + 1, CMDS_BG[i]);
			
			// Libération de ma mémoire de la chaine de caractères
			free (CMDS_BG[i]);
			CMDS_BG[i] = NULL;
			
			// Pid effacé
			PIDS_BG [i] = -1;
			
			// Commande suivante
			i++;
		}
		
		i = 0;
		
		// Libération de la mémoire de tous les noms de machines distantes
		while (i < NB_SHELLS_DISTANTS_MAX && SHELLS_DISTANTS [i] != NULL) {
			free (SHELLS_DISTANTS [i]);
			SHELLS_DISTANTS [i] = NULL;
			i++;
		}
	}
	else { // Mode distant
		// Commande complète passé en paramètre
		free (chaine_cmd_distante);
	}
}

void handler_interruption (int sig) {
	if (pid_avant_plan != -1) {
		kill (pid_avant_plan, SIGTERM);
		pid_avant_plan = -1;
	}
}



      /*----------------------------------------------------------------------------------------.
      | Lorsque l'analyse de la ligne de commande est effectuée sans erreur. La variable		|
      | globale ExpressionAnalysee pointe sur un arbre représentant l'expression.  Le type		|
      |       "Expression" de l'arbre est décrit dans le fichier Shell.h. Il contient 4			|
      |       champs. Si e est du type Expression :												|
      | 																						|
      | - e.type est un type d'expression, contenant une valeur définie par énumération dans	|
      |   Shell.h. Cette valeur peut être :														|
      |																							|
      |   - VIDE, commande vide																	|
      |   - SIMPLE, commande simple et ses arguments											|
      |   - SEQUENCE, séquence (;) d'instructions												|
      |   - SEQUENCE_ET, séquence conditionnelle (&&) d'instructions							|
      |   - SEQUENCE_OU, séquence conditionnelle (||) d'instructions							|
      |   - BG, tâche en arrière plan (&)														|
      |   - PIPE, pipe (|).																		|
      |   - REDIRECTION_I, redirection de l'entrée (<)											|
      |   - REDIRECTION_O, redirection de la sortie (>)											|
      |   - REDIRECTION_A, redirection de la sortie en mode APPEND (>>).						|
      |   - REDIRECTION_E, redirection de la sortie erreur,  									|
      |   - REDIRECTION_EO, redirection des sorties erreur et standard.							|
      | 																						|
      | - e.gauche et e.droite, de type Expression *, représentent une sous-expression gauche	|
      |       et une sous-expression droite. Ces deux champs ne sont pas utilisés pour les		|
      |       types VIDE et SIMPLE. Pour les expressions réclamant deux sous-expressions		|
      |       (SEQUENCE, SEQUENCE_ET, SEQUENCE_OU, et PIPE) ces deux champs sont utilisés		|
      |       simultannément.  Pour les autres champs, seule l'expression gauche est			|
      |       utilisée.																			|
      | 																						|
      | - e.arguments, de type char **, a deux interpretations :								|
      | 																						|
      |      - si le type de la commande est simple, e.arguments pointe sur un tableau à la		|
      |       argv. (e.arguments)[0] est le nom de la commande, (e.arguments)[1] est le			|
      |       premier argument, etc.															|
      | 																						|
      |      - si le type de la commande est une redirection, (e.arguments)[0] est le nom du	|
      |       fichier vers lequel on redirige.													|
      `----------------------------------------------------------------------------------------*/

int main (int argc, char **argv){
	interactive_mode = (argc == 1);
	
	// Variable permetant de stocker le nombre de caractère d'une commande lancée en mode distant.
	int taille_cmd_distante = 0;
	int i;
	
	// Initialisation de la constante permetant de connaire le répertoire du Shell, ne doit pas changer au cours du programme
	REPERTOIRE_SHELL = get_current_dir_name();
	
	int retour = EXIT_SUCCESS;
	int retour_evaluation = 1;
	
	// faire en sorte qu'interactive_mode = 0 lorsque le shell est distant 
	if (interactive_mode) { // Mode interactif
		// Initialisation du tableau qui va stocker les pid des processus lancés en tâche de fond
		memset (PIDS_BG, -1, NB_CMDS_BG_MAX);
		
		// Initialisation du tableau qui va stocker les commandes sous forme de chaîne de caractères des processus
		// lancées en tâche de fond
		for (i = 0; i < NB_CMDS_BG_MAX; i++)
			CMDS_BG[i] = NULL;
		
		// Initialisation du tableau qui va stocker les noms des machines distantes
		for (i = 0; i < NB_SHELLS_DISTANTS_MAX; i++)
			SHELLS_DISTANTS[i] = NULL;
		
		// Initialisation de l'historique
		using_history ();
		
		struct sigaction sa_int;
		sa_int.sa_handler = handler_interruption;
		sa_int.sa_flags = 0; // SA_RESETHAND;
		sigemptyset(&sa_int.sa_mask);
		sigaction(SIGINT, &sa_int, NULL);
		sigaction(SIGQUIT, &sa_int, NULL);
	}
	else { // Mode distant
		i = 1;
		while (argv[i] != NULL){
			taille_cmd_distante += strlen (argv[i]);
			if (argv[i + 1] != NULL)
				taille_cmd_distante += 1; // taille de l'espace
			i++;
		}
		
		chaine_cmd_distante = malloc (taille_cmd_distante + 1);
		memset (chaine_cmd_distante, '\0', taille_cmd_distante + 1);
		
		i = 1;
		while (argv[i] != NULL){
			strcat (chaine_cmd_distante, argv[i]);
			if (argv[i + 1] != NULL)
				strcat (chaine_cmd_distante, " ");
			i++;
		}
			
	}

	while (!EXIT_PROG){
		if (my_yyparse (chaine_cmd_distante) == 0){/* L'analyse a abouti */
//			afficher_expr (ExpressionAnalysee);
			if ((retour_evaluation = evaluer_expr (ExpressionAnalysee)) && !interactive_mode)
				retour = retour_evaluation;
			fflush (stdout);
			expression_free (ExpressionAnalysee);
		}
		else {/* L'analyse de la ligne de commande a donné une erreur */}
		
		// On quitte la boucle lorsque l'on est en mode distant
		EXIT_PROG = !interactive_mode ? 1 : EXIT_PROG;
	}
	
	free_structures ();
	
	return retour;
}