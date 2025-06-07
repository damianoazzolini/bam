#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <time.h>
// #include "util.h"
#include "cudd.h"
// #include "cudd/cuddInt.h"

// #include "cudd/cudd/cudd.h"
// #include "cudd/cudd/cuddInt.h"

#include "cnf_handler.h"
#include "semiring.h"
#include "tree_traversal.h"
#include "tree_build.h"
#include "solver.h"


int main(int argc, char *argv[]) {
    cnf *theory = init_cnf();
    var_mapping *var_map = init_var_mapping();
    semiring_t semiring = prob_semiring(0); // max_times_semiring();
    int weight_type = REAL_WEIGHT;

    int compilation_type = 1; // 0 monolithic, 1 cutset

    // printf("argc: %d\n", argc);
    // time timeout 300 ./bam cnfs/CNF_complex_weights mono -s complex_prob
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
            weight_type = REAL_WEIGHT;
            semiring = max_times_semiring();
        }
        if (strcmp(argv[4], "complex_max_times") == 0) {
            weight_type = COMPLEX_WEIGHT;
            semiring = max_times_semiring();
        }
        else if (strcmp(argv[4], "min_times") == 0) {
            semiring = min_times_semiring();
        }
        else if (strcmp(argv[4], "prob") == 0) {
            weight_type = REAL_WEIGHT;
            semiring = prob_semiring(REAL_WEIGHT);
        }
        else if (strcmp(argv[4], "complex_prob") == 0) {
            weight_type = COMPLEX_WEIGHT;
            semiring = prob_semiring(COMPLEX_WEIGHT);
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

    if(theory->n_layers > 1) {
        printf("c Nested AMC task with %d layers\n", theory->n_layers);
    }
    else {
        printf("c AMC task\n");
    }

    #ifdef DEBUG_MODE
    print_var_mapping(var_map);
    print_cnf(theory);
    // compute_stats_cnf(theory);
    #endif    

    solve_with_bdd(theory, var_map, semiring, compilation_type);

    free_cnf(theory);
    free_var_mapping(var_map);

    return 0;
}
