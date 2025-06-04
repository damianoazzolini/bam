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


// cutset cache
// the cutset is a set of clauses, a bdd node, and a key
typedef struct cutset_t {
    DdNode *node;
    int *cutset; // list of clauses in the cutset
    int n_clauses; // number of clauses in the cutset
    char *key; // key computed from the cutset, bit strings will improve this
} cutset_t;

// the cache entry i a list of cutsets
typedef struct cutset_cache_entry {
    cutset_t *cutset; // pointer to the cutset
    int n_cutset;
} cutset_cache_entry;

// the cache is a list of cutset_cache_entry, one for each variable
typedef struct cutset_cache_t {
    cutset_cache_entry *entries;
    int n_entries;
} cutset_cache_t;


weight_t solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring, int compilation_type, int weight_type);