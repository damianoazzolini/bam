#define INFINITY 1e10

typedef struct semiring_t {
    double (*add)(double, double);
    double (*mul)(double, double);
    double neutral_add;
    double neutral_mul;
} semiring_t;

typedef struct semiring_two {
    double *(*add)(double*, double*);
    double *(*mul)(double*, double*);
    double neutral_add[2];
    double neutral_mul[2];
} semiring_two_t;

typedef struct layer_variable {
    int idx_var;
    float weight_true;
    float weight_false;
} layer_variable;

typedef struct amc_layer_t {
    semiring_two_t semiring;
    layer_variable *variables;
    int n_variables;
} amc_layer_t;

typedef struct second_level_amc_t {
    amc_layer_t *inner_layer;
    amc_layer_t *outer_layer;
    double *(*transformation_function)(double*); // transformation function tuple 2 -> tuple 2
} second_level_amc_t;

// single variable
semiring_t prob_semiring();
semiring_t max_times_semiring();
semiring_t min_times_semiring();
semiring_t max_plus_semiring();

// pair of variables
semiring_two_t expected_utility_semiring();
semiring_two_t stable_models_semiring();
semiring_two_t prob_two_semiring();

// already packed tasks
second_level_amc_t *credal_semantics_inference();