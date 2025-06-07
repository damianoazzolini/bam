#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "solver.h"

weight_t solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring, enum compilation_type compilation_mode) {
    DdManager *manager;
    DdNode *f = NULL;
    weight_t res;

    clock_t begin = clock();
    
    manager = Cudd_Init(theory->n_variables,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */
    // Cudd_AutodynDisable(manager);

    if(compilation_mode == MONOLITHIC_BDD) {
        printf("c Monolithic BDD compilation\n");
        f = build_monolithic_bdd(manager, theory); /* Build the BDD from the CNF formula */
    }
    else if(compilation_mode == CUTSET_BDD) {
        printf("c Cutset BDD compilation\n");
        f = cnf_to_obdd(manager, theory); /* Convert the CNF to an OBDD */
    }

    // #ifdef DEBUG_MODE
    // printf("BDD:\n");
    // Cudd_PrintDebug(manager, f, 3, 2); /* Print the BDD */
    // Cudd_PrintMinterm(manager, f); /* Print the minterms of the BDD */
    // printf("traversal\n");
    // #endif

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("c Time spent building the BDD: %lf seconds\n", time_spent);
    #ifdef DEBUG_MODE
    printf("c Number of nodes: %d\n", Cudd_DagSize(f));
    #endif

    // reorder the bdd
    printf("c Reordering the BDD\n");
    begin = clock();
    reorder_bdd(manager, theory, f);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("c Time spent reordering the BDD: %lf seconds\n", time_spent);

    printf("c Init conversion\n");
    begin = clock();
    DdNode *add_root = Cudd_BddToAdd(manager, f); /* Convert the BDD to an ADD */
    // printf("Conversion done\n");
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("c Time spent converting the BDD to an ADD: %lf seconds\n", time_spent);
    // fflush(stdout);
    // since ADDs have 0/1 terminals while BDDs have only 1 and complemented arcs
    if (add_root == NULL) {
        fprintf(stderr, "Error in Cudd_addBddToAdd\n");
        Cudd_Quit(manager);
        exit(-1);
    }
    int constant = Cudd_IsConstant(add_root);
    if (constant) {
        int v = (int)Cudd_V(add_root);
        printf("c ADD root node is constant: %d\n", v);
        if(v == 0) {
            printf("s UNSATISFIABLE\n");
        }
    }
    else {
        printf("c ADD root node is not constant\n");
        printf("c ADD root node index: %d\n", Cudd_NodeReadIndex(add_root));
        printf("s SATISFIABLE\n");
    }
    // #ifdef DEBUG_MODE
    // Cudd_PrintDebug(manager, add_root, 3, 2); /* Print the BDD */
    // Cudd_PrintMinterm(manager, add_root); /* Print the minterms of the BDD */
    // // printf("traversal\n");
    // #endif

    printf("c Traversing the ADD\n");
    // label res_label;
    begin = clock();
    if(theory->n_layers > 1) {
        semiring_nested_t semiring = max_times_add_times_semiring();
        // res = traverse_bdd_aproblog_nested(manager, add_root, var_map, &semiring);
    }
    else {
        res = traverse_bdd_aproblog(manager, add_root, var_map, &semiring);
    }
    // printf("Weight: %lf\n", res_label.weight);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("c Time spent traversing the ADD: %lf seconds\n", time_spent);
    // res = traverse_bdd_amc(manager, f, var_map, &semiring); /* Traverse the BDD and print the minterms */

    char *weight_string = get_weight_string(res);
    printf("c Final weight\n");
    printf("c ==============================\n");
    if(res.weight_type == 0) {
        printf("c s log10-estimate %lf\n", log10(res.weight.real_weight));
    }
    // printf("c s log10-estimate %lf\n", log10(res.real_weight));
    printf("c s exact double float %s\n", weight_string);
    printf("c ==============================\n");
    free(weight_string);
    
    Cudd_Quit(manager);

    return res;
}