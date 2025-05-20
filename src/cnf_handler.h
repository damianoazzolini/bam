
typedef struct clause{
    // int terms[64];
    int *terms;
    unsigned int n_terms;
} clause;

typedef struct var {
    // char name[20];
    // int idx_var;
    double weight_true;
    double weight_false;
} var;

typedef struct cnf{
    unsigned int n_clauses;
    unsigned int n_variables;
    // clause clauses[256];
    clause *clauses;
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

void parse_cnf(char *filename, cnf *theory, var_mapping *var_map);
void print_var_mapping(const var_mapping *var_map);