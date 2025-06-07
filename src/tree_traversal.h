#pragma once
#include "cudd.h"
#include "cnf_handler.h"
#include "semiring.h"

// aproblog cache
typedef struct label {
    weight_t weight;
    // char set[MAX_VAR];
    char *set;
} label;

typedef struct cache {
    DdNode *node_pointer;
    int n_items_in_set; // this is the same for each cache entry, var_map->n_variables_mappings
    char *set;
    weight_t weight;
    struct cache *next; // pointer to the next cache entry
} cache;


weight_t traverse_bdd_aproblog(DdManager *manager, DdNode *node, const var_mapping *var_map, const semiring_t *semiring);