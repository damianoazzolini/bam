#pragma once
#include "cudd.h"
#include "cnf_handler.h"
#include "semiring.h"
#include "tree_traversal.h"
#include "tree_build.h"

enum compilation_type {
    MONOLITHIC_BDD = 0,
    CUTSET_BDD = 1
};

weight_t solve_with_bdd(cnf *theory, var_mapping *var_map, semiring_t semiring, enum compilation_type compilation_mode);