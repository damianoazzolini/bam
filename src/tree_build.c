#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree_build.h"

void reorder_bdd(DdManager *manager, const cnf *theory, DdNode *f) {
    int result;
    int *order;
    if(theory->n_layers == 1) {
        order = compute_stats_cnf(theory);
    }
    else {
        // maybe more complex reordering can be considered,
        // for example, number of occurrences of variables in the clauses
        // for each layer
        // I need to remove duplicates in the order array
        // for example, if the layers are [1,2,3,1,3,2]
        // I should get [1,3,5,2,6,4]
        int count_previous = 0;
        order = calloc((theory->n_variables + 1), sizeof(int));
        for(unsigned  i = 1; i <= theory->n_layers; i++) {
            for(unsigned int j = 1; j <= theory->n_variables; j++) {
                if(theory->variable_layers[j] == i) {
                    order[j] = count_previous + 1;
                    count_previous++;
                }
            }
        }
        #ifdef DEBUG_MODE
        printf("c Reordering order:\n");
        for(int i = 1; i <= theory->n_variables; i++) {
            printf("c Variable %d: %d\n", i, order[i]);
        }
        #endif
    }
    result = Cudd_ShuffleHeap(manager, order);
    if(result == 0) {
        fprintf(stderr, "Error in Cudd_ShuffleHeap\n");
        Cudd_Quit(manager);
        return;
    }
    printf("c Reordering done\n");

    free(order);
}


// monolithic BDD
DdNode *build_monolithic_bdd(DdManager *manager, cnf *theory) {
    DdNode *f;
    DdNode *for_clause;
    DdNode *var_node, *tmp;
    unsigned int i, j;
    int var_index;
    f = Cudd_ReadOne(manager);
    // documentation: https://add-lib.scce.info/assets/doxygen-cudd-documentation/cudd_8h.html
    // https://add-lib.scce.info/assets/documents/cudd-manual.pdf
    // printf("n clauses: %d\n", theory->n_clauses);
    // printf("n variables: %d\n", theory->n_variables);
    printf("c Building the monolithic BDD\n");
    for(i = 0; i < theory->n_clauses; i++) {
        // printf("Processing clause %d/%d\n", i, theory->n_clauses);
        for_clause = Cudd_ReadLogicZero(manager);
        Cudd_Ref(for_clause); /* Increases the reference count of a node */
        for (j = 0; j < theory->clauses[i].n_terms; j++) {

            // var = abs(theory.clauses[i].terms[j]);
            var_index = abs(theory->clauses[i].terms[j]);
            // printf("pos: %d\n", var);
            var_node = Cudd_bddIthVar(manager, var_index);
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

    Cudd_Ref(f);
    return f;
}

// CUTSET
DdNode *get_node(DdManager *manager, int var_index, DdNode *high, DdNode *low) {
    DdNode *node;
    DdNode *current;

    if(low == high) {
        Cudd_Ref(low);
        return low;
    }

    // the commented code is wrong, I need to keep track of the variables in the BDD manager
    // node = Cudd_ReadVars(manager, var_index);
    // // variable exists in the manager, return it
    // if (node != NULL) {
    //     Cudd_Ref(node); /* Increases the reference count of a node */
    //     return node; /* Return the BDD variable node */
    // }

    // create a new variable node
    current = Cudd_bddIthVar(manager, var_index);
    node = Cudd_bddIte(manager, current, high, low);
    if (node == NULL) {
        fprintf(stderr, "Error in get_node\n");
        return NULL;
    }
    
    Cudd_Ref(node);
    return node;
}

cutset_cache_t *init_cutset_cache(unsigned  n_variables) {
    cutset_cache_t *cutset_cache = malloc(sizeof(cutset_cache_t));
    cutset_cache->n_entries = n_variables;
    cutset_cache->entries = malloc(n_variables * sizeof(cutset_cache_entry)); // these are more than needed
     
    for(int i = 0; i < n_variables; i++) {
        cutset_cache->entries[i].cutset = NULL;
        cutset_cache->entries[i].n_cutset = 0;
    }
    
    return cutset_cache;
}

DdNode *cutset_cache_lookup(cutset_cache_t *cutset_cache, int variable_index, char *cutset_key) {
    cutset_cache_entry *entry = &cutset_cache->entries[variable_index - 1]; // variable_index is 1-based, entries are 0-based
    for(int i = 0; i < entry->n_cutset; i++) {
        if(strcmp(entry->cutset[i].key, cutset_key) == 0) { // attention: assumed null termination
            return entry->cutset[i].node; // found in cache
        }
    }

    return NULL; // not found
}

void print_cutset_cache(cutset_cache_t *cutset_cache) {
    if(cutset_cache == NULL) {
        printf("Cutset cache is NULL\n");
        return;
    }
    printf("Cutset cache:\n");
    for(int i = 0; i < cutset_cache->n_entries; i++) {
        cutset_cache_entry *entry = &cutset_cache->entries[i];
        printf("Variable %d: n_cutset = %d\n", i + 1, entry->n_cutset);
        for(int j = 0; j < entry->n_cutset; j++) {
            printf("  Cutset %d: key = %s, n_clauses = %d\n", j + 1, entry->cutset[j].key, entry->cutset[j].n_clauses);
            // for(int k = 0; k < entry->cutset[j].n_clauses; k++) {
            //     printf("    Clause %d: %d\n", k + 1, entry->cutset[j].cutset[k]);
            // }
        }
    }
}

void add_cutset_cache(cutset_cache_t *cutset_cache, int variable_index, DdNode *node, int *cutset, unsigned int n_clauses, char *cutset_key) {
    cutset_cache_entry *entry = &cutset_cache->entries[variable_index - 1];
    entry->n_cutset++;
    // printf("Adding cutset for variable %d, n_cutset = %d\n", variable_index, entry->n_cutset);
    entry->cutset = realloc(entry->cutset, entry->n_cutset * sizeof(cutset_t));
    entry->cutset[entry->n_cutset - 1].node = node;
    entry->cutset[entry->n_cutset - 1].cutset = malloc(n_clauses * sizeof(int));
    memcpy(entry->cutset[entry->n_cutset - 1].cutset, cutset, n_clauses * sizeof(int));
    entry->cutset[entry->n_cutset - 1].n_clauses = n_clauses;
    entry->cutset[entry->n_cutset - 1].key = calloc(strlen(cutset_key) + 1, sizeof(char));
    strcpy(entry->cutset[entry->n_cutset - 1].key, cutset_key);
}

void free_cutset_cache(cutset_cache_t *cutset_cache) {
    if (cutset_cache != NULL) {
        for(int i = 0; i < cutset_cache->n_entries; i++) {
            if(cutset_cache->entries[i].cutset != NULL) {
                free(cutset_cache->entries[i].cutset->cutset);
                free(cutset_cache->entries[i].cutset);
            }
        }
        free(cutset_cache->entries);
        free(cutset_cache);
    }
}

void get_min_max(int *arr, unsigned int n_elements, int *min, int *max) {
    // this function computes the min and max of the array
    // it is used to compute the cutset
    *min = arr[0] > 0 ? arr[0] : 999999;
    *max = arr[0] > 0 ? arr[0] : -1;
    for(int i = 0; i < n_elements; i++) {
        if(arr[i] < *min) {
            *min = arr[i];
        }
        if(arr[i] > *max) {
            *max = arr[i];
        }
    }
}

int *compute_cutset(cnf *theory, int variable_index, char *key) {
    // this function computes the cutset of the CNF formula
    int *cutset = calloc(theory->n_clauses, sizeof(int));
    int *used_variables = calloc(theory->n_variables, sizeof(int));
    int n_cutset = 0;
    int min, max;
    // *key = 0;
    for(unsigned int i = 0; i < theory->n_clauses; i++) {
        // cutset[i] = i + 1; // just return all variables for now
        // TODO: compute the used variables in the CNF formula
        if(theory->clauses[i].n_terms == 0) {
            key[i] = '1';
        }
        else {
            for(unsigned int j = 0; j < theory->clauses[i].n_terms; j++) {
                int var_index = abs(theory->clauses[i].terms[j]);
                used_variables[var_index - 1] = 1; // 0-based, mark the variable as used
            }
    
            get_min_max(used_variables, theory->n_variables, &min, &max);
    
            if(min <= variable_index && variable_index < max) {
                cutset[n_cutset] = i; // or i + 1?
                n_cutset++;
            }
            
            memset(used_variables, 0, theory->n_variables * sizeof(int));
        }
    }

    free(used_variables);
    return cutset;
}

DdNode *cnf_to_obdd_rec(DdManager *manager, cnf *theory, cutset_cache_t *cutset_cache, int total_clauses, int *order, int order_index) {
    // printf("cnf_to_obdd_rec: variable_index = %d\n", variable_index);
    // if(variable_index > theory->n_variables) {
    //     print_cnf(theory);
    //     return;
    // }
    DdNode *result;
    int variable_index = order[order_index]; // 1-based index

    if(theory->state == CNF_INCONSISTENT) {
        // print_cnf(theory);
        result = Cudd_ReadLogicZero(manager); // return a constant 0 BDD
        Cudd_Ref(result);
        return result;
    }
    if(theory->state == CNF_SOLVED) {
        result = Cudd_ReadOne(manager); // return a constant 1 BDD
        Cudd_Ref(result);
        return result;
    }
    
    // cache lookup
    char *cutset_key = calloc(total_clauses + 1, sizeof(char)); // +1 for the null terminator
    for(unsigned int i = 0; i < total_clauses; i++) {
        cutset_key[i] = '0'; // initialize the cutset key with '0'
    }
    cutset_key[total_clauses] = '\0'; // null-terminate the string-> for strcmp

    int *cutset = compute_cutset(theory, variable_index, cutset_key);
    // printf("Cutset key for variable %d: %s<-\n", variable_index, cutset_key);
    DdNode *cached_node = cutset_cache_lookup(cutset_cache, variable_index, cutset_key);
    if(cached_node != NULL) {
        // printf("Found in cache for variable %d\n", variable_index);
        Cudd_Ref(cached_node);
        free(cutset);
        return cached_node;
    }

    cnf *sub_cnf_true, *sub_cnf_false;
    sub_cnf_true = init_cnf();
    sub_cnf_false = init_cnf();
    // deep copy the current cnf into true and false sub-cnf
    sub_cnf_true->n_variables = theory->n_variables;
    sub_cnf_false->n_variables = theory->n_variables;
    sub_cnf_true->n_clauses = theory->n_clauses;
    sub_cnf_false->n_clauses = theory->n_clauses;
    sub_cnf_true->clauses = malloc(theory->n_clauses * sizeof(clause));
    sub_cnf_false->clauses = malloc(theory->n_clauses * sizeof(clause));
    for(unsigned int i = 0; i < theory->n_clauses; i++) {
        sub_cnf_true->clauses[i].n_terms = theory->clauses[i].n_terms;
        sub_cnf_false->clauses[i].n_terms = theory->clauses[i].n_terms;
        sub_cnf_true->clauses[i].terms = malloc(theory->clauses[i].n_terms * sizeof(int));
        sub_cnf_false->clauses[i].terms = malloc(theory->clauses[i].n_terms * sizeof(int));
        for(unsigned int j = 0; j < theory->clauses[i].n_terms; j++) {
            sub_cnf_true->clauses[i].terms[j] = theory->clauses[i].terms[j];
            sub_cnf_false->clauses[i].terms[j] = theory->clauses[i].terms[j];
        }
    }

    // printf("Processing variable %d\n", variable_index);
    set_variable(sub_cnf_true, variable_index, 1); // set the variable to true
    set_variable(sub_cnf_false, variable_index, -1); // set the variable to false

    DdNode *true_branch, *false_branch;
    true_branch = cnf_to_obdd_rec(manager, sub_cnf_true, cutset_cache, total_clauses, order, order_index + 1);
    false_branch = cnf_to_obdd_rec(manager, sub_cnf_false, cutset_cache, total_clauses, order, order_index + 1);
    
    result = get_node(manager, variable_index, true_branch, false_branch);
    
    free_cnf(sub_cnf_true);
    free_cnf(sub_cnf_false);
    
    add_cutset_cache(cutset_cache, variable_index, result, cutset, theory->n_clauses, cutset_key);
    
    free(cutset_key);
    free(cutset);

    return result;
}

DdNode *cnf_to_obdd(DdManager *DdManager, cnf *theory) {
    // implements the approach: https://users.cecs.anu.edu.au/~jinbo/04-sat.pdf
    DdNode *result;
    cutset_cache_t *cutset_cache = init_cutset_cache(theory->n_variables);
    int *order = compute_stats_cnf(theory);
    result = cnf_to_obdd_rec(DdManager, theory, cutset_cache, theory->n_clauses, order, 1);
    // printf("----\n");
    // print_cutset_cache(cutset_cache);
    // printf("----\n");
    free_cutset_cache(cutset_cache);
    return result;
}
