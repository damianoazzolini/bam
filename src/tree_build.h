#pragma once
#include "cudd.h"
#include "cnf_handler.h"
#include "semiring.h"

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


// weight_t solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring, int compilation_type);
DdNode *build_monolithic_bdd(DdManager *manager, cnf *theory);
DdNode *cnf_to_obdd(DdManager *DdManager, cnf *theory);
void reorder_bdd(DdManager *manager, const cnf *theory, DdNode *f);