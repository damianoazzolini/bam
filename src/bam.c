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

void traverse_bdd_amc_aux(DdManager* dd, DdNode* node, int * list, const var_mapping *var_map, const semiring_t *semiring, double *query_amc) {
    // similar to ddPrintMintermAux
    DdNode *current_node, *then_branch, *else_branch;
    int v, j, current_combination;
    unsigned int index;
    double current_weight_true, current_weight_false;
    // int two_positions[256]; // Store indices of elements equal to 2
    int *two_positions; // Store indices of elements equal to 2
    int two_count = 0;

    current_node = Cudd_Regular(node);

    if (Cudd_IsConstant(current_node)) {
        // Terminal case: Print one cube based on the current recursion path
        if (node !=  Cudd_ReadBackground(dd) && node != Cudd_Not(Cudd_ReadOne(dd))) {
            int temp[256];
            int combinations = 0;
            int bit;
            two_positions = malloc((unsigned int)Cudd_ReadSize(dd) * sizeof(int)); // worst case, so no reallocation

            // compute the position of the 2s
            for (j = 1; j < Cudd_ReadSize(dd); j++) { // we start from 1, 0 is the index of the ReadLogicZero
                v = list[j];
                if (v == VAR_DONT_CARE) {
                    two_positions[two_count++] = j;
                }
            }

            // compute the number of combinations
            combinations = 1 << two_count;  // 2^two_count

            // iterate over all combinations
            for (current_combination = 0; current_combination < combinations; current_combination++) {
                double current_amc = semiring->neutral_mul;

                // Copy original array
                for (j = 0; j < Cudd_ReadSize(dd); j++) {
                    temp[j] = list[j];
                }

                // Apply combination by replacing 2s
                for (j = 0; j < two_count; j++) {
                    bit = (current_combination >> j) & 1;
                    temp[two_positions[j]] = bit;
                }

                for (j = 1; j <  Cudd_ReadSize(dd); j++) { // from 1   
                    v = temp[j];
                    current_weight_true = var_map->variables_mappings[j].weight_true;
                    current_weight_false = var_map->variables_mappings[j].weight_false;

                    if(v == VAR_FALSE) {
                        #ifdef DEBUG_MODE
                        printf("0");
                        #endif
                        current_amc = semiring->mul(current_amc, current_weight_false);
                    }
                    else if(v == VAR_TRUE) {
                        #ifdef DEBUG_MODE
                        printf("1");
                        #endif
                        current_amc = semiring->mul(current_amc, current_weight_true);
                    }
                    #ifdef DEBUG_MODE
                    else if(v == VAR_DONT_CARE) {
                        printf("This should not happen");
                        printf("-");
                    }
                    #endif
                }
                #ifdef DEBUG_MODE
                printf(" -> %lf\n", current_amc);
                #endif
                if(Cudd_V(node) == 1) {
                    *query_amc = semiring->add(*query_amc, current_amc);
                }
            }
            free(two_positions);
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

void solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring) {
    DdManager *manager;
    DdNode *f;
    DdNode *var_node, *tmp;
    DdNode *for_clause;
    unsigned int i, j;
    int var;
    double res = 0.0;
    
    manager = Cudd_Init(theory->n_variables,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */
    // Cudd_AutodynDisable(manager);
    f = Cudd_ReadOne(manager);
    // documentation: https://add-lib.scce.info/assets/doxygen-cudd-documentation/cudd_8h.html
    // https://add-lib.scce.info/assets/documents/cudd-manual.pdf
    printf("n clauses: %d\n", theory->n_clauses);
    printf("n variables: %d\n", theory->n_variables);

    for(i = 0; i < theory->n_clauses; i++) {
        for_clause = Cudd_ReadLogicZero(manager);
        Cudd_Ref(for_clause); /* Increases the reference count of a node */
        for (j = 0; j < theory->clauses[i].n_terms; j++) {

            // var = abs(theory.clauses[i].terms[j]);
            var = abs(theory->clauses[i].terms[j]);
            // printf("pos: %d\n", var);
            var_node = Cudd_bddIthVar(manager, var);
            // Cudd_Ref(vars_list[var-1]); /* Increases the reference count of a node */
            if (theory->clauses[i].terms[j] > 0) {
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
    // printf("traversal\n");
    #endif

    // int count_non_equal = 1;
    // for(j = 0; j < var_map.n_variables_mappings; j++) {
    //     if(var_map.variables_mappings[j].weight_true != var_map.variables_mappings[j].weight_false) {
    //         count_non_equal++;
    //     }
    // }
    // int pos = 1;
    
    // int *reorder = malloc(Cudd_ReadSize(manager) * (unsigned int)theory.n_variables);
    // reorder[0] = 0;
    // for (i = 1; i < Cudd_ReadSize(manager); i++) {
    //     j = Cudd_ReadInvPerm(manager, i);
        
    //     // printf("%d: %lf %lf\n", j, var_map.variables_mappings[j].weight_true, var_map.variables_mappings[j].weight_false);
    //     if(var_map.variables_mappings[j].weight_true != var_map.variables_mappings[j].weight_false) {
    //         reorder[pos] = j;
    //         // printf("reorder[%d] = %d\n", pos, j);
    //         pos++;
    //     }
    //     else {
    //         reorder[count_non_equal] = j;
    //         // printf("reorder[%d] = %d\n", count_non_equal, j);
    //         count_non_equal++;
    //     }
    // }

    // // for(j = 0; j < Cudd_ReadSize(manager); j++) {
    // //     printf("%d -> %d\n", j, reorder[j]);
    // // }

    // // return 0;
    // var = Cudd_ShuffleHeap(manager, reorder);
    // if(var == 0) {
    //     fprintf(stderr, "Error in Cudd_ShuffleHeap\n");
    //     Cudd_Quit(manager);
    //     return;
    // }
    // printf("Reordering done\n");

    // free(reorder);


    res = traverse_bdd_amc(manager, f, var_map, &semiring); /* Traverse the BDD and print the minterms */

    printf("Weight: %lf\n", res);
    
    Cudd_Quit(manager);
}

int main(int argc, char *argv[]) {
    cnf *theory = init_cnf();
    var_mapping *var_map = init_var_mapping();
    semiring_t semiring = prob_semiring(); // max_times_semiring();

    #ifdef DEBUG_MODE
    printf("DEBUG_MODE is ON\n");
    #endif

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cnf_file>\n", argv[0]);
        return -1;
    }
    // if (strcmp(argv[2], "--task") != 0) {
    //     fprintf(stderr, "Usage: %s <cnf_file> --task TASK\n", argv[0]);
    //     return -1;
    // }
    
    parse_cnf(argv[1], theory, var_map);

    #ifdef DEBUG_MODE
    print_var_mapping(var_map);
    #endif

    solve_with_bdd(theory, var_map, semiring);

    free_cnf(theory);
    free_var_mapping(var_map);

    
    return 0;
}
