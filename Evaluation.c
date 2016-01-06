#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

int evaluer_expr_simple (char **args){
	if (strcmp (args[0], "echo") == 0)
		cmdInt_echo (args + 1);
	else if (strcmp (args[0], "history") == 0)
		cmdInt_history (args + 1);
	else if (strcmp (args[0], "date") == 0)
		cmdInt_date (args + 1);
	else if (strcmp (args[0], "kill") == 0)
		cmdInt_kill (args + 1);
	else if (strcmp (args[0], "exit") == 0)
		EXIT_PROG = cmdInt_exit ();
	else
		fprintf (stderr, "%s : commande introuvable\n", args[0]);
}

int evaluer_expr (Expression *e){	
	switch (e->type){
		case SIMPLE:
			evaluer_expr_simple (e->arguments);
			break;
		case VIDE:
			break;
		default:
			fprintf (stderr, "fonctionnalité non implémentée\n");
			break;
	}
	
	return 1;
}

