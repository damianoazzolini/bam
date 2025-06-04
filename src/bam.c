#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include "util.h"
#include "cudd.h"
// #include "cudd/cuddInt.h"

// #include "cudd/cudd/cudd.h"
// #include "cudd/cudd/cuddInt.h"

#include "cnf_handler.h"
#include "semiring.h"
#include "bam.h"

// #define MAX_VAR 1024



void insert_cache(cache **cache_list, DdNode *node_pointer, const char *set, int n_items_in_set, weight_t weight) {
    cache *new_entry = malloc(sizeof(cache));
    new_entry->node_pointer = node_pointer;
    new_entry->n_items_in_set = n_items_in_set;
    new_entry->set = malloc(n_items_in_set * sizeof(char));
    memcpy(new_entry->set, set, n_items_in_set * sizeof(char)); // copy the set
    // new_entry->set = set;
    new_entry->weight = weight;
    new_entry->next = *cache_list;
    *cache_list = new_entry;
}

weight_t cache_lookup(cache *cache_list, DdNode *node_pointer, char **set) {
    cache *current = cache_list;
    while (current != NULL) {
        if (current->node_pointer == node_pointer) {
            *set = current->set;
            return current->weight;
        }
        current = current->next;
    }
    
    return (weight_t) { .real_weight = 0.0 };
}

void print_cache(cache *cache_list) {
    cache *current = cache_list;
    while (current != NULL) {
        // printf("Cache entry: node_index = %d, terminal = %d, hash = %lu, weight = %lf, set = {", 
        // printf("Cache entry: node_pointer = %p (%d), weight: %lf, set = {", current->node_pointer, Cudd_NodeReadIndex(current->node_pointer), current->weight);
        printf("Cache entry: node_pointer = %p (%d), set = {", current->node_pointer, Cudd_NodeReadIndex(current->node_pointer));
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
    label result;
    result.set = calloc(var_map->n_variables_mappings, sizeof(char));

    if(Cudd_IsConstant(node)) {
        if(Cudd_V(node) == 1) {
            result.weight = semiring->neutral_mul;
        }
        else { // value 0
            result.weight = semiring->neutral_add;
        }
        return result;
    }

    char *set = NULL;
    int found = 0;
    unsigned int index = Cudd_NodeReadIndex(node);
    weight_t res = cache_lookup(*cache_list, Cudd_Regular(node), &set);
    if(set != NULL) { // found in cache
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
    
    int i;
    weight_t plh, phl;
    weight_t temp_value_vl_minus_vh = semiring->neutral_mul;
    weight_t temp_value_vh_minus_vl = semiring->neutral_mul;
    weight_t temp_result, current_weight_true, current_weight_false;

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

    free(high_label.set);
    free(low_label.set);

    plh = semiring->mul(high_label.weight, temp_value_vl_minus_vh);
    phl = semiring->mul(low_label.weight, temp_value_vh_minus_vl);
    result.set[index] = 1; // mark the current variable
    current_weight_true = var_map->variables_mappings[index].weight_true;
    current_weight_false = var_map->variables_mappings[index].weight_false;
    result.weight = semiring->add(semiring->mul(plh, current_weight_true), semiring->mul(phl, current_weight_false));

    insert_cache(cache_list, Cudd_Regular(node), result.set, var_map->n_variables_mappings, result.weight);

    return result;
}

weight_t traverse_bdd_aproblog(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring, int weight_type) {
    label result;
    cache **cache_list = malloc(sizeof(cache*));
    *cache_list = NULL; // Initialize the cache list
    weight_t current_weight_true, current_weight_false;
    char *weight_string;
    
    result = traverse_bdd_aproblog_rec(manager, node, var_map, semiring, cache_list);
    
    // print_cache(*cache_list);
    
    weight_t adjusted_weight = result.weight;
    weight_string = get_weight_string(adjusted_weight, weight_type);
    printf("c ADD weight: %s\n", weight_string);
    free(weight_string);

    for(int i = 1; i < var_map->n_variables_mappings; i++) {
        if(result.set[i] == 0) {
            // mul
            current_weight_true = semiring->mul(var_map->variables_mappings[i].weight_true, adjusted_weight);
            current_weight_false = semiring->mul(var_map->variables_mappings[i].weight_false, adjusted_weight);
            adjusted_weight = semiring->add(current_weight_true, current_weight_false);

            weight_string = get_weight_string(adjusted_weight, weight_type);
            printf("Variable %d is not in the set, adjusting weight: %s\n", i, weight_string);
            free(weight_string);
        }
    }

    free(cache_list);
    free(result.set);

    return adjusted_weight;
}

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

DdNode *cnf_to_obdd_rec(DdManager *manager, cnf *theory, cutset_cache_t *cutset_cache, int total_clauses, int variable_index) {
    // printf("cnf_to_obdd_rec: variable_index = %d\n", variable_index);
    // if(variable_index > theory->n_variables) {
    //     print_cnf(theory);
    //     return;
    // }
    DdNode *result;

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
    true_branch = cnf_to_obdd_rec(manager, sub_cnf_true, cutset_cache, total_clauses, variable_index + 1);
    false_branch = cnf_to_obdd_rec(manager, sub_cnf_false, cutset_cache, total_clauses, variable_index + 1);
    
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
    result = cnf_to_obdd_rec(DdManager, theory, cutset_cache, theory->n_clauses, 1);
    // printf("----\n");
    // print_cutset_cache(cutset_cache);
    // printf("----\n");
    free_cutset_cache(cutset_cache);
    return result;
}

weight_t solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring, int compilation_type, int weight_type) {
    DdManager *manager;
    DdNode *f = NULL;
    weight_t res;

    clock_t begin = clock();
    
    manager = Cudd_Init(theory->n_variables,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */
    // Cudd_AutodynDisable(manager);

    if(compilation_type == 0) {
        printf("c Monolithic BDD compilation\n");
        f = build_monolithic_bdd(manager, theory); /* Build the BDD from the CNF formula */
    }
    else if(compilation_type == 1) {
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
        printf("c ADD root node is constant\n");
    }
    else {
        printf("c ADD root node is not constant\n");
        printf("c ADD root node index: %d\n", Cudd_NodeReadIndex(add_root));
    }
    // #ifdef DEBUG_MODE
    // Cudd_PrintDebug(manager, add_root, 3, 2); /* Print the BDD */
    // Cudd_PrintMinterm(manager, add_root); /* Print the minterms of the BDD */
    // // printf("traversal\n");
    // #endif

    printf("c Traversing the ADD\n");
    // label res_label;
    begin = clock();
    res = traverse_bdd_aproblog(manager, add_root, var_map, &semiring, weight_type);
    // printf("Weight: %lf\n", res_label.weight);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("c Time spent traversing the ADD: %lf seconds\n", time_spent);
    // res = traverse_bdd_amc(manager, f, var_map, &semiring); /* Traverse the BDD and print the minterms */

    char *weight_string = get_weight_string(res, weight_type);
    printf("c Final weight\n");
    printf("c ==============================\n\n");
    printf("%s\n", weight_string);
    printf("\nc ==============================\n");
    free(weight_string);
    
    Cudd_Quit(manager);

    return res;
}

int main(int argc, char *argv[]) {
    cnf *theory = init_cnf();
    var_mapping *var_map = init_var_mapping();
    semiring_t semiring = prob_semiring(0); // max_times_semiring();
    int weight_type = 0; // 0 for real, 1 for complex

    int compilation_type = 1; // 0 monolithic, 1 cutset

    // printf("argc: %d\n", argc);
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <cnf_file> <compilation>\n", argv[0]);
        return -1;
    }
    if(strcmp(argv[2], "mono") == 0) {
        compilation_type = 0; // monolithic compilation
    }

    if (argc >= 4) {
        if(strcmp(argv[3], "-s") != 0 && strcmp(argv[3], "--semiring") != 0) {
            fprintf(stderr, "Usage: %s <cnf_file> -s SEMIRING\n", argv[0]);
            return -1;
        }
        if (strcmp(argv[4], "max_times") == 0) {
            weight_type = 0;
            semiring = max_times_semiring();
        }
        if (strcmp(argv[4], "complex_max_times") == 0) {
            weight_type = 1;
            semiring = max_times_semiring();
        }
        else if (strcmp(argv[4], "min_times") == 0) {
            semiring = min_times_semiring();
        }
        else if (strcmp(argv[4], "prob") == 0) {
            weight_type = 0;
            semiring = prob_semiring(0);
        }
        else if (strcmp(argv[4], "complex_prob") == 0) {
            weight_type = 1;
            semiring = prob_semiring(1);
        }
        else {
            fprintf(stderr, "Semiring not implemented: %s\n", argv[4]);
            return -1;
        }
        printf("c Semiring: %s\n", argv[4]);
    }
    else {
        printf("c Default semiring: prob/WMC\n");
    }
    
    #ifdef DEBUG_MODE
    printf("c DEBUG_MODE is ON\n");
    #endif
    
    parse_cnf(argv[1], theory, var_map, weight_type);

    #ifdef DEBUG_MODE
    // print_var_mapping(var_map, weight_type);
    // print_cnf(theory);
    #endif
    
    // set_variable(theory, 1, 1);
    // set_variable(theory, 2, -1);
    // set_variable(theory, 3, -1);
    // print_cnf(theory);
    

    solve_with_bdd(theory, var_map, semiring, compilation_type, weight_type);

    free_cnf(theory);
    free_var_mapping(var_map);

    
    return 0;
}
