#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree_traversal.h"

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
    
    return (weight_t) { .weight_type = -1 };
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
        else {
            result.weight = semiring->neutral_add;
        }
        return result;
    }

    char *set = NULL;
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
    result.weight.weight_type = semiring->neutral_add.weight_type; // one of the two types, indifferent
    result.weight = semiring->add(semiring->mul(plh, current_weight_true), semiring->mul(phl, current_weight_false));

    insert_cache(cache_list, Cudd_Regular(node), result.set, var_map->n_variables_mappings, result.weight);

    return result;
}

weight_t traverse_bdd_aproblog(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring) {
    label result;
    cache **cache_list = malloc(sizeof(cache*));
    *cache_list = NULL; // Initialize the cache list
    weight_t current_weight_true, current_weight_false;
    char *weight_string;
    
    result = traverse_bdd_aproblog_rec(manager, node, var_map, semiring, cache_list);
    
    weight_t adjusted_weight = result.weight;
    weight_string = get_weight_string(adjusted_weight);
    printf("c ADD weight: %s\n", weight_string);
    free(weight_string);

    for(int i = 1; i < var_map->n_variables_mappings; i++) {
        if(result.set[i] == 0) {
            // mul
            current_weight_true = semiring->mul(var_map->variables_mappings[i].weight_true, adjusted_weight);
            current_weight_false = semiring->mul(var_map->variables_mappings[i].weight_false, adjusted_weight);
            adjusted_weight = semiring->add(current_weight_true, current_weight_false);

            weight_string = get_weight_string(adjusted_weight);
            printf("c Variable %d is not in the set, adjusting weight: %s\n", i, weight_string);
            free(weight_string);
        }
    }

    free(cache_list);
    free(result.set);

    return adjusted_weight;
}
