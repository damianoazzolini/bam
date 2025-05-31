#include <stdio.h>
#include <stdlib.h>

#include "../src/cnf_handler.h"

cnf *init_easy_cnf() {
    cnf *theory = init_cnf();
    int n_terms;
    // theory
    // -1 2
    // 2 3

    theory->n_clauses = 2;
    theory->clauses = malloc(theory->n_clauses * sizeof(clause));
    
    n_terms = 2;
    theory->clauses[0].terms = malloc(n_terms * sizeof(int));
    theory->clauses[0].n_terms = n_terms;
    theory->clauses[0].terms[0] = -1;
    theory->clauses[0].terms[1] = 2;
    
    n_terms = 2;
    theory->clauses[1].terms = malloc(n_terms * sizeof(int));
    theory->clauses[1].n_terms = n_terms;
    theory->clauses[1].terms[0] = 2;
    theory->clauses[1].terms[1] = 3;

    return theory;
}

cnf *init_tautology_cnf() {
    cnf *theory = init_cnf();
    int n_terms;
    // theory
    // -1 2
    // -2 2

    theory->n_clauses = 2;
    theory->clauses = malloc(theory->n_clauses * sizeof(clause));
    
    n_terms = 2;
    theory->clauses[0].terms = malloc(n_terms * sizeof(int));
    theory->clauses[0].n_terms = n_terms;
    theory->clauses[0].terms[0] = -1;
    theory->clauses[0].terms[1] = 2;
    
    n_terms = 2;
    theory->clauses[1].terms = malloc(n_terms * sizeof(int));
    theory->clauses[1].n_terms = n_terms;
    theory->clauses[1].terms[0] = -2;
    theory->clauses[1].terms[1] = 2;

    return theory;
}



int test_set_variable_tautology() {
    cnf *theory = init_tautology_cnf();
    int passed = 0;

    set_variable(theory, 1, 1); // Set variable 1 to true
    set_variable(theory, 2, 1); // Set variable 2 to true

    print_cnf(theory);
    
    if(theory->state == CNF_SOLVED) {
        passed = 1;
    }
    free_cnf(theory);
    return passed;
}

int test_set_variable_unsolved() {
    cnf *theory = init_easy_cnf();
    int passed = 0;

    set_variable(theory, 1, 1); // Set variable 1 to true
    
    if(theory->state == CNF_UNSOLVED) {
        passed = 1;
    }
    free_cnf(theory);
    return passed;
}

int test_set_variable_solved() {
    cnf *theory = init_easy_cnf();
    int passed = 0;

    set_variable(theory, 1, -1); // Set variable 1 to false
    set_variable(theory, 2, 1); // Set variable 2 to true
    
    if(theory->state == CNF_SOLVED) {
        passed = 1;
    }
    free_cnf(theory);
    return passed;
}

int test_set_variable_inconsistent() {
    cnf *theory = init_easy_cnf();
    int passed = 0;

    set_variable(theory, 1, 1); // Set variable 1 to true
    set_variable(theory, 2, -1); // Set variable 2 to false
    set_variable(theory, 3, -1); // Set variable 3 to false
    
    if(theory->state == CNF_INCONSISTENT) {
        passed = 1;
    }
    free_cnf(theory);
    return passed;
}

void test_cnf_set_variable() {
    int result_tautology = test_set_variable_tautology();
    int result_unsolved = test_set_variable_unsolved();
    int result_solved = test_set_variable_solved();
    int result_inconsistent = test_set_variable_inconsistent();

    if(result_tautology) {
        printf("Test set_variable tautology: PASSED\n");
    } else {
        printf("Test set_variable tautology: FAILED\n");
    }

    if(result_unsolved) {
        printf("Test set_variable unsolved: PASSED\n");
    } else {
        printf("Test set_variable unsolved: FAILED\n");
    }

    if(result_solved) {
        printf("Test set_variable solved: PASSED\n");
    } else {
        printf("Test set_variable solved: FAILED\n");
    }

    if(result_inconsistent) {
        printf("Test set_variable inconsistent: PASSED\n");
    } else {
        printf("Test set_variable inconsistent: FAILED\n");
    }
}

int main() {
    test_cnf_set_variable();
    return 0;
}