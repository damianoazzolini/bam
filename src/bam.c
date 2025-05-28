#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include "util.h"
#include "cudd.h"
// #include "cudd/cudd/cudd.h"
// #include "cudd/cudd/cuddInt.h"

#include "cnf_handler.h"
#include "semiring.h"

// #define MAX_VAR 1024

typedef struct label {
    double weight;
    // char set[MAX_VAR];
    char *set;
} label;


typedef struct cache {
    DdNode *node_pointer;
    int n_items_in_set; // this is the same for each cache entry, var_map->n_variables_mappings
    const char *set;
    double weight;
    struct cache *next; // pointer to the next cache entry
} cache;

void insert_cache(cache **cache_list, DdNode *node_pointer, const char *set, int n_items_in_set, double weight) {
    cache *new_entry = malloc(sizeof(cache));
    new_entry->node_pointer = node_pointer;
    new_entry->n_items_in_set = n_items_in_set;
    new_entry->set = set;
    new_entry->weight = weight;
    new_entry->next = *cache_list;
    *cache_list = new_entry;
}

double cache_lookup(cache *cache_list, DdNode *node_pointer, char **set) {
    cache *current = cache_list;
    while (current != NULL) {
        if (current->node_pointer == node_pointer) {
            *set = current->set;
            return current->weight;
        }
        current = current->next;
    }
    return -1; // not found
}

void print_cache(cache *cache_list) {
    cache *current = cache_list;
    while (current != NULL) {
        // printf("Cache entry: node_index = %d, terminal = %d, hash = %lu, weight = %lf, set = {", 
        printf("Cache entry: node_pointer = %p (%d), weight: %lf, set = {", current->node_pointer, Cudd_NodeReadIndex(current->node_pointer), current->weight);
        for(int i = 0; i < current->n_items_in_set; i++) {
            if(current->set[i] > 0) {
                printf("%d ", i);
            }
        }
        printf("}\n");
        current = current->next;
    }
}

void free_cache(cache *cache_list) {
    // here the double free is certain, due to pointers
    cache *current = cache_list;
    cache *next;
    while (current != NULL) {
        next = current->next;
        free(current->set);
        free(current);
        current = next;
    }
}


label traverse_bdd_aproblog_rec(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring, cache **cache_list) {
    if(Cudd_IsConstant(node)) {
        label result;
        result.set = calloc(var_map->n_variables_mappings, sizeof(char)); // slower than static but more flexible
        if(Cudd_V(node) == 1) {
            result.weight = semiring->neutral_mul;
        }
        else { // value 0
            result.weight = semiring->neutral_add;
        }
        return result;
    }

    char *set = NULL;
    unsigned int index = Cudd_NodeReadIndex(node);
    double res = cache_lookup(*cache_list, Cudd_Regular(node), &set);
    if(res > 0) { // found in cache
        label result_cached;
        result_cached.set = calloc(var_map->n_variables_mappings, sizeof(char));
        result_cached.weight = res;
        for(int i = 0; i < var_map->n_variables_mappings; i++) {
            result_cached.set[i] = set[i];
        }
        result_cached.set[index] = 1;

        return result_cached;
    }

    label high_label, low_label;

    high_label = traverse_bdd_aproblog_rec(manager, Cudd_T(node), var_map, semiring, cache_list);
    low_label = traverse_bdd_aproblog_rec(manager, Cudd_E(node), var_map, semiring, cache_list);
        
    label result;
    result.set = calloc(var_map->n_variables_mappings, sizeof(char));

    double plh, phl;
    int i; //, index;
    double temp_value_vl_minus_vh = semiring->neutral_mul;
    double temp_value_vh_minus_vl = semiring->neutral_mul;
    double temp_result, current_weight_true, current_weight_false;

    for(i = 0; i < var_map->n_variables_mappings; i++) {
        if(high_label.set[i] > 0 || low_label.set[i] > 0) {
            result.set[i] = 1;
        }
        if(low_label.set[i] - high_label.set[i] > 0) {
            temp_result = semiring->add(var_map->variables_mappings[i].weight_false, var_map->variables_mappings[i].weight_true);
            temp_value_vl_minus_vh = semiring->mul(temp_value_vl_minus_vh, temp_result);
        }
        if(high_label.set[i] - low_label.set[i] > 0) {
            temp_result = semiring->add(var_map->variables_mappings[i].weight_false, var_map->variables_mappings[i].weight_true);
            temp_value_vh_minus_vl = semiring->mul(temp_value_vh_minus_vl, temp_result);
        }
    }

    // free(high_label.set);
    // free(low_label.set);

    plh = semiring->mul(high_label.weight, temp_value_vl_minus_vh);
    phl = semiring->mul(low_label.weight, temp_value_vh_minus_vl);
    result.set[index] = 1; // mark the current variable
    current_weight_true = var_map->variables_mappings[index].weight_true;
    current_weight_false = var_map->variables_mappings[index].weight_false;
    result.weight = semiring->add(semiring->mul(plh, current_weight_true), semiring->mul(phl, current_weight_false));

    insert_cache(cache_list, Cudd_Regular(node), result.set, var_map->n_variables_mappings, result.weight);

    return result;
}

double traverse_bdd_aproblog(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring) {
    label result;
    cache **cache_list = malloc(sizeof(cache*));
    *cache_list = NULL; // Initialize the cache list
    double adjusted_weight = 0;
    double current_weight_true, current_weight_false;
    
    result = traverse_bdd_aproblog_rec(manager, node, var_map, semiring, cache_list);

    // print_cache(*cache_list);

    adjusted_weight = result.weight;

    printf("ADD weight: %lf\n", adjusted_weight);
    for(int i = 1; i < var_map->n_variables_mappings; i++) {
        if(result.set[i] == 0) {
            // mul
            current_weight_true = semiring->mul(var_map->variables_mappings[i].weight_true, adjusted_weight);
            current_weight_false = semiring->mul(var_map->variables_mappings[i].weight_false, adjusted_weight);
            adjusted_weight = semiring->add(current_weight_true, current_weight_false);
            printf("Variable %d is not in the set, adjusting weight: %lf\n", i, adjusted_weight);
        }
    }

    // free(cache_list);
    // free(result.set);

    return adjusted_weight;
}

void solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring) {
    DdManager *manager;
    DdNode *f;
    DdNode *var_node, *tmp;
    DdNode *for_clause;
    unsigned int i, j;
    int var;
    double res = 0.0;

    clock_t begin = clock();
    
    manager = Cudd_Init(theory->n_variables,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */
    // Cudd_AutodynDisable(manager);
    f = Cudd_ReadOne(manager);
    // documentation: https://add-lib.scce.info/assets/doxygen-cudd-documentation/cudd_8h.html
    // https://add-lib.scce.info/assets/documents/cudd-manual.pdf
    // printf("n clauses: %d\n", theory->n_clauses);
    // printf("n variables: %d\n", theory->n_variables);
    printf("Building the BDD\n");
    for(i = 0; i < theory->n_clauses; i++) {
        // printf("Processing clause %d/%d\n", i, theory->n_clauses);
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

    // #ifdef DEBUG_MODE
    // Cudd_PrintDebug(manager, f, 3, 2); /* Print the BDD */
    // Cudd_PrintMinterm(manager, f); /* Print the minterms of the BDD */
    // // printf("traversal\n");
    // #endif

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent building the BDD: %lf seconds\n", time_spent);

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

    // if(var_map->n_variables_mappings > MAX_VAR) {
    //     printf("Error, hardcoded max var: %d\n", MAX_VAR);
    //     Cudd_Quit(manager);
    //     return;
    // }

    printf("Init conversion\n");
    begin = clock();
    DdNode *add_root = Cudd_BddToAdd(manager, f); /* Convert the BDD to an ADD */
    // printf("Conversion done\n");
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent converting the BDD to an ADD: %lf seconds\n", time_spent);
    // fflush(stdout);
    // since ADDs have 0/1 terminals while BDDs have only 1 and complemented arcs
    if (add_root == NULL) {
        fprintf(stderr, "Error in Cudd_addBddToAdd\n");
        Cudd_Quit(manager);
        return;
    }
    // #ifdef DEBUG_MODE
    // Cudd_PrintDebug(manager, add_root, 3, 2); /* Print the BDD */
    // Cudd_PrintMinterm(manager, add_root); /* Print the minterms of the BDD */
    // // printf("traversal\n");
    // #endif

    printf("Traversing the ADD\n");
    // label res_label;
    begin = clock();
    res = traverse_bdd_aproblog(manager, add_root, var_map, &semiring);
    // printf("Weight: %lf\n", res_label.weight);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent traversing the ADD: %lf seconds\n", time_spent);
    // res = traverse_bdd_amc(manager, f, var_map, &semiring); /* Traverse the BDD and print the minterms */

    printf("Weight: %lf\n", res);
    
    Cudd_Quit(manager);
}

int main(int argc, char *argv[]) {
    cnf *theory = init_cnf();
    var_mapping *var_map = init_var_mapping();
    semiring_t semiring = prob_semiring(); // max_times_semiring();

    // printf("argc: %d\n", argc);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cnf_file>\n", argv[0]);
        return -1;
    }
    if (argc >= 4) {
        if(strcmp(argv[2], "-s") != 0 && strcmp(argv[2], "--semiring") != 0) {
            fprintf(stderr, "Usage: %s <cnf_file> -s SEMIRING\n", argv[0]);
            return -1;
        }
        if (strcmp(argv[3], "max_times") == 0) {
            semiring = max_times_semiring();
        }
        else if (strcmp(argv[3], "min_times") == 0) {
            semiring = min_times_semiring();
        }
        else if (strcmp(argv[3], "prob") == 0) {
            semiring = prob_semiring();
        }
        else {
            fprintf(stderr, "Semiring not implemented: %s\n", argv[3]);
            return -1;
        }
        printf("Semiring: %s\n", argv[3]);
    }
    else {
        printf("Default semiring: prob/WMC\n");
    }
    
    #ifdef DEBUG_MODE
    printf("DEBUG_MODE is ON\n");
    #endif
    
    parse_cnf(argv[1], theory, var_map);

    #ifdef DEBUG_MODE
    print_var_mapping(var_map);
    #endif

    solve_with_bdd(theory, var_map, semiring);

    free_cnf(theory);
    free_var_mapping(var_map);

    
    return 0;
}
