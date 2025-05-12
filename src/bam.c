#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "util.h"
#include "cudd.h"
// #include "cudd/cudd/cudd.h"
// #include "cudd/cudd/cuddInt.h"

#include "cnf_handler.h"
#include "semiring.h"

#define VAR_FALSE 0
#define VAR_TRUE 1
#define VAR_DONT_CARE 2

// c 2 b 0.2
// c 3 q 1
// c 4 a 0.3
// -1 b q c ..
// -0 0 1 1 10010001  1
// -0 1 1 0 01000110  1
// -0 1 1 1 00000011
// minterms counts from index 1
// apply the semiring to the minterms

void traverse_bdd_amc_aux(DdManager* dd, DdNode* node, int * list, const var_mapping *var_map, const semiring_t *semiring, double *query_amc) {
    // similar to ddPrintMintermAux
    DdNode *current_node, *then_branch, *else_branch;
    int i,v;
    unsigned int index;

    current_node = Cudd_Regular(node);

    if (Cudd_IsConstant(current_node)) {
        // Terminal case: Print one cube based on the current recursion path
        if (node !=  Cudd_ReadBackground(dd) && node != Cudd_Not(Cudd_ReadOne(dd))) {
            double current_amc = semiring->neutral_mul;
            for (i = 0; i <  Cudd_ReadSize(dd); i++) {
                double current_weight = 1.0;
                int found = var_map->used[i]; // this should be fixed when AMC is used

                v = list[i];
                current_weight = var_map->variables_mappings[i].prob;

                switch (v) {
                case VAR_FALSE:
                    #ifdef DEBUG_MODE
                    printf("0");
                    #endif
                    if(found == 1) {
                        // quando si considera algebraic, non va più bene found ma serve il peso
                        current_amc = semiring->mul(current_amc, (1 - current_weight));
                    }
                    break;
                case VAR_TRUE:
                    #ifdef DEBUG_MODE
                    printf("1");
                    #endif
                    if(found == 1) {
                        current_amc = semiring->mul(current_amc, current_weight);
                    }
                    break;
                case VAR_DONT_CARE:
                    #ifdef DEBUG_MODE
                    printf("-");
                    #endif
                    break;
                default:
                    break;
                }
            }
            #ifdef DEBUG_MODE
            printf(" % g %lf\n", Cudd_V(node), current_amc);
            #endif
            if(Cudd_V(node) == 1) {
                *query_amc = semiring->add(*query_amc, current_amc);
                // *query_prob = *query_prob + prob;
            }
        }
    } 
    else {
        then_branch = Cudd_T(current_node);
        else_branch = Cudd_E(current_node);
        if (Cudd_IsComplement(node)) {
            then_branch = Cudd_Not(then_branch);
            else_branch = Cudd_Not(else_branch);
        }
        index = Cudd_NodeReadIndex(current_node);
        list[index] = VAR_FALSE;
        traverse_bdd_amc_aux(dd, else_branch, list, var_map, semiring, query_amc);
        list[index] = VAR_TRUE;
        traverse_bdd_amc_aux(dd, then_branch, list, var_map, semiring, query_amc);
        list[index] = VAR_DONT_CARE;
    }
}


double traverse_bdd_amc(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring) {
    int	i, *list;
    double query_amc = semiring->neutral_add;
    list = malloc(sizeof(int)*(unsigned int)Cudd_ReadSize(manager));

    if (list == NULL) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }

    for (i = 0; i < Cudd_ReadSize(manager); i++){
        list[i] = VAR_DONT_CARE;
    }
    traverse_bdd_amc_aux(manager,node, list, var_map, semiring, &query_amc);
    free(list);
    return query_amc;
}

// double traverse_bdd_2amc(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring) {
//     return 0.0;
// }


int main(int argc, char *argv[]) {
    DdManager *manager;
    DdNode *var_node, *tmp;
    DdNode *f;
    DdNode *for_clause;
    cnf theory;
    var_mapping var_map;
    unsigned int i, j;
    int var;
    double res = 0.0;
    semiring_t semiring = max_times_semiring();
    
    var_map.n_variables_mappings = 0;
    // set all the variables to unused
    for (i = 0; i < 100; i++) {
        var_map.used[i] = 0;
    }

    #ifdef DEBUG_MODE
    printf("DEBUG_MODE is ON\n");
    #endif

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <cnf_file> --task TASK\n", argv[0]);
        return -1;
    }
    if (strcmp(argv[2], "--task") != 0) {
        fprintf(stderr, "Usage: %s <cnf_file> --task TASK\n", argv[0]);
        return -1;
    }
    
    parse_cnf(argv[1], &theory, &var_map);

    
    print_var_mapping(&var_map);

    // return 0;

    manager = Cudd_Init(theory.n_variables,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */
    f = Cudd_ReadOne(manager);
    // documentation: https://add-lib.scce.info/assets/doxygen-cudd-documentation/cudd_8h.html
    // https://add-lib.scce.info/assets/documents/cudd-manual.pdf
    printf("n clauses: %d\n", theory.n_clauses);
    printf("n variables: %d\n", theory.n_variables);

    for(i = 0; i < theory.n_clauses; i++) {
        for_clause = Cudd_ReadLogicZero(manager);
        Cudd_Ref(for_clause); /* Increases the reference count of a node */
        for (j = 0; j < theory.clauses[i].n_terms; j++) {

            var = abs(theory.clauses[i].terms[j]);
            var_node = Cudd_bddIthVar(manager, var);
            // Cudd_Ref(vars_list[var-1]); /* Increases the reference count of a node */
            if (theory.clauses[i].terms[j] > 0) {
                // for_clause = Cudd_bddOr(manager, for_clause, vars_list[var-1]);
                tmp = Cudd_bddOr(manager, for_clause, var_node);
            }
            else {
                // for_clause = Cudd_bddOr(manager, for_clause, Cudd_Not(vars_list[var-1]));
                tmp = Cudd_bddOr(manager, for_clause, Cudd_Not(var_node));
            }
            Cudd_Ref(tmp);
            Cudd_RecursiveDeref(manager, for_clause);
            for_clause = tmp;
        }
        // Cudd_Ref(for_clause); /* Increases the reference count of a node */
        // f = Cudd_bddAnd(manager, f, for_clause);
        tmp = Cudd_bddAnd(manager, f, for_clause);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(manager, f);
        f = tmp;
        // Cudd_RecursiveDeref(manager, for_clause); /* Decreases the reference count of a node */
    }

    Cudd_Ref(f); /* Increases the reference count of a node */
    #ifdef DEBUG_MODE
    Cudd_PrintDebug(manager, f, 3, 2); /* Print the BDD */
    Cudd_PrintMinterm(manager, f); /* Print the minterms of the BDD */
    printf("traversal\n");
    #endif


    if(strcmp(argv[3], "amc") == 0) {
        res = traverse_bdd_amc(manager, f, &var_map, &semiring); /* Traverse the BDD and print the minterms */
    }
    else if(strcmp(argv[3], "amc2") == 0) {
        // res = traverse_bdd_2amc(manager, f, &var_map, &semiring); /* Traverse the BDD and print the minterms */
    }
    else {
        fprintf(stderr, "Usage: %s <cnf_file> --task TASK\n", argv[0]);
        return -1;
    }
    // double prob = compute_prob(f, &var_map);
    printf("Weight: %lf\n", res);

    // // costruisco a mano basandomi su CNF2
    // // c 2 a 0.2
    // // c 3 q 1
    // // c 4 b 0.1
    // // c 5 nq 1
    // amc_layer_t inner_layer;
    // semiring_t inner_semiring = prob_semiring();
    // inner_layer.semiring = &inner_semiring;
    // inner_layer.n_variables = 2; // a and b
    // inner_layer.variables = malloc(sizeof(layer_variable) * inner_layer.n_variables);
    // inner_layer.variables[0].idx_var = 0;
    // inner_layer.variables[0].weight_true = 0.2;
    // inner_layer.variables[0].weight_false = 1 - inner_layer.variables[0].weight_true;
    // inner_layer.variables[1].idx_var = 2;
    // inner_layer.variables[1].weight_true = 0.1;
    // inner_layer.variables[1].weight_false = 1 - inner_layer.variables[1].weight_true;


    // DdNode *bdd = Cudd_bddNewVar(manager); /*Create a new BDD variable*/
    // Cudd_Ref(bdd); /*Increases the reference count of a node*/
    // printf("The BDD variable is %p\n", bdd); /*Prints the address of the BDD variable*/
    
    Cudd_Quit(manager);

    return 0;
}
