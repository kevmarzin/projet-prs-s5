#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

int evaluer_expr_simple (char **args){
	if (strcmp (args[0], "echo") == 0)
		cmdInt_echo (args);
	else if (strcmp (args[0], "history") == 0)
		cmdInt_history (args);
	else
		fprintf (stderr, "fonctionnalité non implémentée\n");
}

int evaluer_expr (Expression *e){	
	switch (e->type){
		case SIMPLE:
			evaluer_expr_simple (e->arguments);
			break;
		default:
			fprintf (stderr, "fonctionnalité non implémentée\n");
			break;
	}
	
	return 1;
}

