
typedef struct clause{
    int *terms;
    int n_terms;
} clause;

typedef struct var {
    char name[20];
    int idx_var;
    double prob;
} var;

typedef struct cnf{
    clause *clauses;
    int n_clauses;
} cnf;

typedef struct var_mapping {
    var variables_mappings[100];
    int n_variables_mappings;
} var_mapping;

cnf parse_cnf(char *filename, var_mapping *var_map);
void print_var_mapping(var_mapping *var_map);