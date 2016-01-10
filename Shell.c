/* Construction des arbres repr�sentant des commandes */

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
bool interactive_mode = 1;	// par d�faut on utilise readline 
int status = 0;			// valeur retourn�e par la derni�re commande

/*
 * Construit une expression � partir de sous-expressions
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
 * Renvoie une liste d'arguments, la premi�re case �tant initialis�e � NULL, la
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
 * Ajoute en fin de liste le nouvel argument et renvoie la liste r�sultante
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
 * Fonction appel�e lorsque l'utilisateur tape "".
 */
void EndOfFile (void){
  exit (0);
}/* EndOfFile */

/*
 * Appel�e par yyparse() sur erreur syntaxique
 */
void yyerror (char *s){
  fprintf (stderr, "%s\n", s);
}


/*
 * Lib�ration de la m�moire occup�e par une expression
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
 * Lecture de la ligne de commande � l'aide de readline en mode interactif
 * M�morisation dans l'historique des commandes
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
			*strchr (line, '\0') = '\n';	// Ajoute \n � la line pour qu'elle puisse etre trait� par le parseur
			ret = yyparse_string (line);	// Remplace l'entr�e standard de yyparse par s
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
 * Lib�re la m�moire de toutes les structures de donn�es si cette lib�ration n'a pas �t� faite.
 */
void free_structures () {
	int i = 0;
	
	free (REPERTOIRE_SHELL);
	
	if (interactive_mode) {
		i = 0;
		
		// Fin de toutes les commandes lanc�es en fond de t�che
		while (i < NB_CMDS_BG_MAX && PIDS_BG [i] != -1) {
			// Termine la commande i
			kill (PIDS_BG [i], SIGTERM);
			waitpid (PIDS_BG [i], NULL, WNOHANG);
			
			// Affichage de la fin
			printf ("[%d]   Fini\t\t\t%s\n", i + 1, CMDS_BG[i]);
			
			// Lib�ration de ma m�moire de la chaine de caract�res
			free (CMDS_BG[i]);
			CMDS_BG[i] = NULL;
			
			// Pid effac�
			PIDS_BG [i] = -1;
			
			// Commande suivante
			i++;
		}
		
		i = 0;
		
		// Lib�ration de la m�moire de tous les noms de machines distantes
		while (i < NB_SHELLS_DISTANTS_MAX && SHELLS_DISTANTS [i] != NULL) {
			free (SHELLS_DISTANTS [i]);
			SHELLS_DISTANTS [i] = NULL;
			i++;
		}
	}
	else { // Mode distant
		// Commande compl�te pass� en param�tre
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
      | Lorsque l'analyse de la ligne de commande est effectu�e sans erreur. La variable		|
      | globale ExpressionAnalysee pointe sur un arbre repr�sentant l'expression.  Le type		|
      |       "Expression" de l'arbre est d�crit dans le fichier Shell.h. Il contient 4			|
      |       champs. Si e est du type Expression :												|
      | 																						|
      | - e.type est un type d'expression, contenant une valeur d�finie par �num�ration dans	|
      |   Shell.h. Cette valeur peut �tre :														|
      |																							|
      |   - VIDE, commande vide																	|
      |   - SIMPLE, commande simple et ses arguments											|
      |   - SEQUENCE, s�quence (;) d'instructions												|
      |   - SEQUENCE_ET, s�quence conditionnelle (&&) d'instructions							|
      |   - SEQUENCE_OU, s�quence conditionnelle (||) d'instructions							|
      |   - BG, t�che en arri�re plan (&)														|
      |   - PIPE, pipe (|).																		|
      |   - REDIRECTION_I, redirection de l'entr�e (<)											|
      |   - REDIRECTION_O, redirection de la sortie (>)											|
      |   - REDIRECTION_A, redirection de la sortie en mode APPEND (>>).						|
      |   - REDIRECTION_E, redirection de la sortie erreur,  									|
      |   - REDIRECTION_EO, redirection des sorties erreur et standard.							|
      | 																						|
      | - e.gauche et e.droite, de type Expression *, repr�sentent une sous-expression gauche	|
      |       et une sous-expression droite. Ces deux champs ne sont pas utilis�s pour les		|
      |       types VIDE et SIMPLE. Pour les expressions r�clamant deux sous-expressions		|
      |       (SEQUENCE, SEQUENCE_ET, SEQUENCE_OU, et PIPE) ces deux champs sont utilis�s		|
      |       simultann�ment.  Pour les autres champs, seule l'expression gauche est			|
      |       utilis�e.																			|
      | 																						|
      | - e.arguments, de type char **, a deux interpretations :								|
      | 																						|
      |      - si le type de la commande est simple, e.arguments pointe sur un tableau � la		|
      |       argv. (e.arguments)[0] est le nom de la commande, (e.arguments)[1] est le			|
      |       premier argument, etc.															|
      | 																						|
      |      - si le type de la commande est une redirection, (e.arguments)[0] est le nom du	|
      |       fichier vers lequel on redirige.													|
      `----------------------------------------------------------------------------------------*/

int main (int argc, char **argv){
	interactive_mode = (argc == 1);
	
	// Variable permetant de stocker le nombre de caract�re d'une commande lanc�e en mode distant.
	int taille_cmd_distante = 0;
	int i;
	
	// Initialisation de la constante permetant de connaire le r�pertoire du Shell, ne doit pas changer au cours du programme
	REPERTOIRE_SHELL = get_current_dir_name();
	
	int retour = EXIT_SUCCESS;
	int retour_evaluation = 1;
	
	// faire en sorte qu'interactive_mode = 0 lorsque le shell est distant 
	if (interactive_mode) { // Mode interactif
		// Initialisation du tableau qui va stocker les pid des processus lanc�s en t�che de fond
		memset (PIDS_BG, -1, NB_CMDS_BG_MAX);
		
		// Initialisation du tableau qui va stocker les commandes sous forme de cha�ne de caract�res des processus
		// lanc�es en t�che de fond
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
		else {/* L'analyse de la ligne de commande a donn� une erreur */}
		
		// On quitte la boucle lorsque l'on est en mode distant
		EXIT_PROG = !interactive_mode ? 1 : EXIT_PROG;
	}
	
	free_structures ();
	
	return retour;
}