#pragma once

typedef struct complex_t {
    double real;
    double imag;
} complex_t;

typedef union weight_t {
    double real_weight;
    complex_t complex_weight;
} weight_t;

typedef struct clause{
    // int terms[64];
    int *terms;
    unsigned int n_terms;
} clause;

typedef struct var {
    // char name[20];
    // int idx_var;
    weight_t weight_true;
    weight_t weight_false;
} var;

enum cnf_state {
    CNF_INCONSISTENT = 0,
    CNF_SOLVED = 1, // all variables are assigned
    CNF_UNSOLVED = 2 // general case
};

typedef struct cnf{
    unsigned int n_clauses;
    unsigned int n_variables;
    enum cnf_state state;
    // clause clauses[256];
    clause *clauses;
    // int **matrix_representation;
} cnf;

typedef struct var_mapping {
    // var variables_mappings[256];
    var *variables_mappings;
    // int used[256]; // mark a variable at position i as used
    int n_variables_mappings;
} var_mapping;

cnf *init_cnf();
void free_cnf(cnf *theory);
var_mapping *init_var_mapping();
void free_var_mapping(var_mapping *var_map);

void parse_cnf(char *filename, cnf *theory, var_mapping *var_map, int weight_type);
void set_variable(cnf *theory, int idx_var, int phase);
void print_var_mapping(const var_mapping *var_map, int weight_type);
// void print_cnf_matrix(const cnf *theory);
void print_cnf(const cnf *theory);
int *compute_stats_cnf(const cnf *theory);
char *get_weight_string(weight_t weight, int weight_type);