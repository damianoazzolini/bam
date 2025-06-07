// #pragma once
#include <stdlib.h>

#include "semiring.h"
#include "cnf_handler.h"

weight_t mul(weight_t a, weight_t b) {
    weight_t result;
    result.weight_type = a.weight_type;
    if(a.weight_type == REAL_WEIGHT) { // real
        result.weight.real_weight = a.weight.real_weight * b.weight.real_weight;
    }
    else if(a.weight_type == COMPLEX_WEIGHT) { // complex
        result.weight.complex_weight.real = a.weight.complex_weight.real * b.weight.complex_weight.real - a.weight.complex_weight.imag * b.weight.complex_weight.imag;
        result.weight.complex_weight.imag = a.weight.complex_weight.real * b.weight.complex_weight.imag + a.weight.complex_weight.imag * b.weight.complex_weight.real;
    }
    return result;
}

weight_t add(weight_t a, weight_t b) {
    weight_t result;
    result.weight_type = a.weight_type;
    if(a.weight_type == REAL_WEIGHT) {
        result.weight.real_weight = a.weight.real_weight + b.weight.real_weight;
    }
    else if(a.weight_type == COMPLEX_WEIGHT) {
        result.weight.complex_weight.real = a.weight.complex_weight.real + b.weight.complex_weight.real;
        result.weight.complex_weight.imag = a.weight.complex_weight.imag + b.weight.complex_weight.imag;
    }
    return result;
}

weight_t max(weight_t a, weight_t b) {
    weight_t result;
    result.weight_type = a.weight_type;
    if(a.weight_type == REAL_WEIGHT) {
        result.weight.real_weight = a.weight.real_weight > b.weight.real_weight ? a.weight.real_weight : b.weight.real_weight;
    }
    else if(a.weight_type == COMPLEX_WEIGHT) { // is this correct?
        result.weight.complex_weight.real = a.weight.complex_weight.real > b.weight.complex_weight.real ? a.weight.complex_weight.real : b.weight.complex_weight.real;
        result.weight.complex_weight.imag = a.weight.complex_weight.imag > b.weight.complex_weight.imag ? a.weight.complex_weight.imag : b.weight.complex_weight.imag;
    }
    return result;
}
weight_t min(weight_t a, weight_t b) {
    weight_t result;
    result.weight_type = a.weight_type;
    if(a.weight_type == REAL_WEIGHT) {
        result.weight.real_weight = a.weight.real_weight < b.weight.real_weight ? a.weight.real_weight : b.weight.real_weight;
    }
    else if(a.weight_type == COMPLEX_WEIGHT) { // is this correct
        result.weight.complex_weight.real = a.weight.complex_weight.real < b.weight.complex_weight.real ? a.weight.complex_weight.real : b.weight.complex_weight.real;
        result.weight.complex_weight.imag = a.weight.complex_weight.imag < b.weight.complex_weight.imag ? a.weight.complex_weight.imag : b.weight.complex_weight.imag;
    }
    return result;
}

// coordinate-wise operations
double* add_two(double *a, double *b) {
    // [a0,a1] + [b0,b1] = [a0+b0, a1+b1]
    double *sum = malloc(2 * sizeof(double));
    sum[0] = a[0] + b[0];
    sum[1] = a[1] + b[1];
    return sum;
}
double* mul_two(double *a, double *b) {
    // [a0,a1] + [b0,b1] = [a0+b0, a1+b1]
    double *mul = malloc(2 * sizeof(double));
    mul[0] = a[0] * b[0];
    mul[1] = a[1] * b[1];
    return mul;
}
double* grad_two(double *a, double *b) {
    // [a0,a1] + [b0,b1] = [a0+b0, a1+b1]
    double *sum = malloc(2 * sizeof(double));
    sum[0] = a[0] + b[0];
    sum[1] = a[1] * b[0] + a[0] * b[1];
    return sum;
}

// one variable semirings
semiring_t prob_semiring(int weight_type) {
    semiring_t semiring;
    weight_t neutral_add, neutral_mul;
    
    if(weight_type == REAL_WEIGHT) {
        neutral_add.weight.real_weight = 0.0;
        neutral_mul.weight.real_weight = 1.0;
        semiring.neutral_add = neutral_add;
        semiring.neutral_mul = neutral_mul;
    }
    else if(weight_type == COMPLEX_WEIGHT) {
        neutral_add.weight.complex_weight.real = 0.0;
        neutral_add.weight.complex_weight.imag = 0.0;
        neutral_mul.weight.complex_weight.real = 1.0;
        neutral_mul.weight.complex_weight.imag = 0.0;
        semiring.neutral_add = neutral_add;
        semiring.neutral_mul = neutral_mul;
    }
    neutral_add.weight_type = weight_type;
    neutral_mul.weight_type = weight_type;
    semiring.add = add;
    semiring.mul = mul;
    return semiring;
}

semiring_t min_max_semiring_wrap(weight_t (*fn)(weight_t, weight_t)) {
    semiring_t semiring;
    weight_t neutral_add, neutral_mul;
    neutral_add.weight_type = REAL_WEIGHT;
    neutral_mul.weight_type = REAL_WEIGHT;
    neutral_add.weight.real_weight = 0.0;
    neutral_mul.weight.real_weight = 1.0;
    semiring.add = fn;
    semiring.mul = mul;
    semiring.neutral_add = neutral_add;
    semiring.neutral_mul = neutral_mul;
    return semiring;
}
semiring_t max_times_semiring() {
    return min_max_semiring_wrap(max);
}
semiring_t min_times_semiring() {
    return min_max_semiring_wrap(min);
}
// semiring_t max_plus_semiring_() {
//     semiring_t semiring;
//     weight_t neutral_add = { .real_weight = -MY_INFINITY };
//     weight_t neutral_mul = { .real_weight = 0.0 };
//     semiring.add = max;
//     semiring.mul = add;
//     semiring.neutral_add = neutral_add;
//     semiring.neutral_mul = neutral_mul;
//     return semiring;
// }

weight_t identity_function(weight_t w) {
    return w;
}

semiring_nested_t max_times_add_times_semiring() {
    semiring_nested_t semiring;
    semiring.n_semirings = 2;
    semiring.semirings = malloc(2 * sizeof(semiring_t));
    semiring.semirings[0] = prob_semiring(0); // innermost
    semiring.semirings[1] = max_times_semiring();
    // malloc an array of function pointers for transformation functions
    semiring.transformation_functions = malloc(sizeof(fnc_ptr_t));
    semiring.transformation_functions[0] = identity_function;
    return semiring;
}

// // two variables semirings
// semiring_two_t expected_utility_semiring() {
//     semiring_two_t semiring;
//     semiring.add = add_two;
//     semiring.mul = grad_two;
//     semiring.neutral_add[0] = 0.0;
//     semiring.neutral_add[1] = 0.0;
//     semiring.neutral_mul[0] = 1.0;
//     semiring.neutral_mul[1] = 0.0;
//     return semiring;
// }
// semiring_two_t add_mul_two() {
//     semiring_two_t semiring;
//     semiring.add = add_two;
//     semiring.mul = mul_two;
//     semiring.neutral_add[0] = 0.0;
//     semiring.neutral_add[1] = 0.0;
//     semiring.neutral_mul[0] = 1.0;
//     semiring.neutral_mul[1] = 1.0;
//     return semiring;
// }
// semiring_two_t stable_models_semiring() {
//     // for PASP or smProbLog
//     return add_mul_two();
// }
// semiring_two_t prob_two_semiring() {
//     // same as before but separated for clarity
//     return add_mul_two();
// }

// transformation functions
double *transformation_function_credal(double *pair) {
    // pair[0] = n. lower
    // pair[1] = n. upper
    double *res = malloc(2 * sizeof(double));
    res[0] = pair[0] == pair[1] ? 1.0 : 0.0; // lower bound
    res[1] = pair[1] > 0 ? 1.0 : 0.0; // upper bound
    return res;
}

// // packed tasks
// second_level_amc_t *credal_semantics_inference() {
//     second_level_amc_t *semiring = malloc(sizeof(second_level_amc_t));
//     semiring->inner_layer = malloc(sizeof(amc_layer_t));
//     // semiring->inner_layer.semiring = add;
//     // semiring->inner_layer->semiring = add_two();

//     semiring->outer_layer = malloc(sizeof(amc_layer_t));
//     semiring->outer_layer->semiring = prob_two_semiring();
//     // semiring->outer_layer->semiring = prob_two_semiring();

//     semiring->transformation_function = transformation_function_credal;
//     return semiring;
// }