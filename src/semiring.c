#include <stdlib.h>

#include "semiring.h"

double mul(double a, double b) {
    return a * b;
}
double add(double a, double b) {
    return a + b;
}
double max(double a, double b) {
    return a > b ? a : b;
}
double min(double a, double b) {
    return a < b ? a : b;
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
semiring_t prob_semiring() {
    semiring_t semiring;
    semiring.add = add;
    semiring.mul = mul;
    semiring.neutral_add = 0.0;
    semiring.neutral_mul = 1.0;
    return semiring;
}
semiring_t min_max_semiring_wrap(double (*fn)(double, double)) {
    semiring_t semiring;
    semiring.add = fn;
    semiring.mul = mul;
    semiring.neutral_add = 0.0;
    semiring.neutral_mul = 1.0;
    return semiring;
}
semiring_t max_times_semiring() {
    return min_max_semiring_wrap(max);
}
semiring_t min_times_semiring() {
    return min_max_semiring_wrap(min);
}
semiring_t max_plus_semiring_() {
    semiring_t semiring;
    semiring.add = max;
    semiring.mul = add;
    semiring.neutral_add = -INFINITY;
    semiring.neutral_mul = 0.0;
    return semiring;
}

// two variables semirings
semiring_two_t expected_utility_semiring() {
    semiring_two_t semiring;
    semiring.add = add_two;
    semiring.mul = grad_two;
    semiring.neutral_add[0] = 0.0;
    semiring.neutral_add[1] = 0.0;
    semiring.neutral_mul[0] = 1.0;
    semiring.neutral_mul[1] = 0.0;
    return semiring;
}
semiring_two_t add_mul_two() {
    semiring_two_t semiring;
    semiring.add = add_two;
    semiring.mul = mul_two;
    semiring.neutral_add[0] = 0.0;
    semiring.neutral_add[1] = 0.0;
    semiring.neutral_mul[0] = 1.0;
    semiring.neutral_mul[1] = 1.0;
    return semiring;
}
semiring_two_t stable_models_semiring() {
    // for PASP or smProbLog
    return add_mul_two();
}
semiring_two_t prob_two_semiring() {
    // same as before but separated for clarity
    return add_mul_two();
}

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