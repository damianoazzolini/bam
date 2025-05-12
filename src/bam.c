#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "util.h"
#include "cudd.h"
// #include "cudd/cudd/cudd.h"
// #include "cudd/cudd/cuddInt.h"

#include "cnf_handler.h"
#include "semiring.h"




// c 2 b 0.2
// c 3 q 1
// c 4 a 0.3
// -1 b q c ..
// -0 0 1 1 10010001  1
// -0 1 1 0 01000110  1
// -0 1 1 1 00000011
// i minterms sono ok con conteggio che parte da 1
// devo applicare i semiring sui minterm
// usare il codice di ddPrintMintermAux
// per vedere se q è vera trovo i minterm che hanno 1 in q

void traverse_bdd_amc_aux(DdManager* dd, DdNode* node, int * list, const var_mapping *var_map, const semiring_t *semiring, double *query_prob) {
    // copy of ddPrintMintermAux
    DdNode *N,*Nv,*Nnv;
    int i,v;
    unsigned int index;

    N = Cudd_Regular(node);

    if (Cudd_IsConstant(N)) {
        /* Terminal case: Print one cube based on the current recursion
        ** path, unless we have reached the background value (ADDs) or
        ** the logical zero (BDDs).
        */
        if (node !=  Cudd_ReadBackground(dd) && node != Cudd_Not(Cudd_ReadOne(dd))) {
            double prob = semiring->neutral_mul;
            for (i = 0; i <  Cudd_ReadSize(dd); i++) {
                double current_prob = 1.0;
                int found = 0;
                v = list[i];
                // i is the i-th variable
                // get its prob
                for(int j = 0; j < var_map->n_variables_mappings; j++) {
                    if (var_map->variables_mappings[j].idx_var == i) {
                        current_prob = var_map->variables_mappings[j].prob;
                        found = 1; // only weighted variables
                        break;
                    }
                }
                // printf("found %lf\n", current_prob);
                switch (v) {
                case 0:
                    printf("0");
                    if(found == 1) {
                        // quando si considera algebraic, non va più bene found ma serve il peso
                        prob = semiring->mul(prob, (1 - current_prob));
                        // prob *= (1 - current_prob);
                    }
                    break;
                case 1:
                    printf("1");
                    if(found == 1) {
                        prob = semiring->mul(prob, current_prob);
                        // prob *= current_prob;
                    }
                    break;
                case 2:
                    printf("-");
                    break;
                default:
                    break;
                }
            }
            printf(" % g %lf\n", Cudd_V(node), prob);
            if(Cudd_V(node) == 1) {
                *query_prob = semiring->add(*query_prob, prob);
                // *query_prob = *query_prob + prob;
            }
        }
    } 
    else {
        Nv = Cudd_T(N);
        Nnv = Cudd_E(N);
        if (Cudd_IsComplement(node)) {
            Nv = Cudd_Not(Nv);
            Nnv = Cudd_Not(Nnv);
        }
        index = Cudd_NodeReadIndex(N);
        list[index] = 0;
        traverse_bdd_amc_aux(dd,Nnv,list, var_map, semiring, query_prob);
        list[index] = 1;
        traverse_bdd_amc_aux(dd,Nv,list, var_map, semiring, query_prob);
        list[index] = 2;
    }
    // return 
} /* end of ddPrintMintermAux */


double traverse_bdd_amc(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring) {
    int	i, *list;
    double query_prob = semiring->neutral_add;
    list = malloc(sizeof(int)*(unsigned int)Cudd_ReadSize(manager));

    if (list == NULL) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }

    for (i = 0; i < Cudd_ReadSize(manager); i++){
        list[i] = 2;
    }
    traverse_bdd_amc_aux(manager,node, list, var_map, semiring, &query_prob);
    free(list);
    return query_prob;
}

// double traverse_bdd_2amc(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring) {
//     return 0.0;
// }



int main (int argc, char *argv[]) {
    cnf theory;
    var_mapping var_map;
    var_map.n_variables_mappings = 0;
    
    // semiring_t semiring = prob_semiring();
    semiring_t semiring = max_times_semiring();


    // arguments:
    // argv[1] = CNF file

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <cnf_file> --task TASK\n", argv[0]);
        return -1;
    }
    if (strcmp(argv[2], "--task") != 0) {
        fprintf(stderr, "Usage: %s <cnf_file> --task TASK\n", argv[0]);
        return -1;
    }
    
    theory = parse_cnf(argv[1], &var_map);

    print_var_mapping(&var_map);

    DdManager *manager;
    manager = Cudd_Init(0,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */

    // documentation: https://add-lib.scce.info/assets/doxygen-cudd-documentation/cudd_8h.html
    // https://add-lib.scce.info/assets/documents/cudd-manual.pdf
    DdNode *f = Cudd_ReadOne(manager);
    DdNode *var_node, *tmp;

    for(int i = 0; i < theory.n_clauses; i++) {
        DdNode *for_clause = Cudd_ReadLogicZero(manager);
        Cudd_Ref(for_clause); /* Increases the reference count of a node */
        for (int j = 0; j < theory.clauses[i].n_terms; j++) {
            int var = abs(theory.clauses[i].terms[j]);
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
    Cudd_PrintDebug(manager, f, 3, 2); /* Print the BDD */
    Cudd_PrintMinterm(manager, f); /* Print the minterms of the BDD */

    printf("traversal\n");
    double res = 0.0;
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
    printf("Probability: %lf\n", res);

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
