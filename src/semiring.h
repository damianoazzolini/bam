#pragma once
#include "cnf_handler.h"

#define MY_INFINITY 1e30

// wrapper for function pointers
typedef weight_t (*fnc_ptr_t)(weight_t);

typedef struct semiring_t {
    weight_t (*add)(weight_t, weight_t);
    weight_t (*mul)(weight_t, weight_t);
    weight_t neutral_add;
    weight_t neutral_mul;
} semiring_t;

typedef struct semiring_nested_t {
    semiring_t *semirings;
    int n_semirings;
    // weight_t (*transformation_function)(weight_t); // only one by now
    // transformation function is an array of function pointers
    fnc_ptr_t *transformation_functions;
    // weight_t *(*transformation_function)(weight_t*);
} semiring_nested_t;

// single variable
semiring_t bool_semiring();
semiring_t sat_semiring();
semiring_t sharp_sat_semiring();
semiring_t prob_semiring(int weight_type); 
semiring_t max_times_semiring(); // MPE
semiring_t min_times_semiring();
semiring_t max_plus_semiring();

semiring_nested_t max_times_add_times_semiring(); // second level

// // pair of variables
// semiring_two_t expected_utility_semiring();
// semiring_two_t stable_models_semiring();
// semiring_two_t prob_two_semiring();

// // already packed tasks
// second_level_amc_t *credal_semantics_inference();