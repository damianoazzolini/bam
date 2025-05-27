#include "cnf_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cnf *init_cnf() {
    cnf *theory = malloc(sizeof(cnf));
    theory->n_clauses = 0;
    theory->n_variables = 0;
    theory->clauses = NULL;
    return theory;
}

void free_cnf(cnf *theory) {
    if (theory != NULL) {
        for (unsigned int i = 0; i < theory->n_clauses; i++) {
            free(theory->clauses[i].terms);
        }
        free(theory->clauses);
        free(theory);
    }
}

var_mapping *init_var_mapping() {
    var_mapping *var_map = malloc(sizeof(var_mapping)); // Allocate memory for variable mapping struct
    var_map->n_variables_mappings = 0;
    var_map->variables_mappings = NULL;
    return var_map;
}

void free_var_mapping(var_mapping *var_map) {
    if (var_map != NULL) {
        free(var_map->variables_mappings); // Free the array of variables
        free(var_map); // Free the var_mapping struct itself
    }
}

void print_var_mapping(const var_mapping *var_map) {
    int total = var_map->n_variables_mappings;
    int i = 0;
    while(total > 0) {
        // if (var_map->used[i] > 0) {
        printf("Variable %d: %lf, %lf\n", i, var_map->variables_mappings[i].weight_true, var_map->variables_mappings[i].weight_false);
        total--;
        // }
        i++;
    }
}

void parse_cnf(char *filename, cnf *theory, var_mapping *var_map) {
    FILE *fp;
    char line[256];
    char task[32];
    unsigned int num_clauses, num_vars;
    int n_read;
    double weight;
    unsigned int nt = 0;
    unsigned int idx_clause = 0;

    theory->n_variables = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    // Read the file and process it
    theory->n_clauses = 0;
    // theory.clauses = NULL;
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Process the line
        // each line has the format of a cnf line, where lines start with 'p cnf' and end with '0'
        // The first line is the problem line, which starts with 'p cnf'
        // The second line is the variable line, which starts with 'c'
        // The rest of the lines are the clauses, which start with a number and end with '0'
        
        if(line[0] == 'p') {
            // p TASK VARS CLAUSES
            n_read = sscanf(line, "%*s %s %d %d", task, &num_vars, &num_clauses);
            if(n_read != 3) {
                fprintf(stderr, "Error parsing line: %s\n", line);
                fclose(fp);
                return;
            }
            theory->n_variables = num_vars;
            theory->n_clauses = num_clauses;
            printf("Number of variables: %d\n", theory->n_variables);
            printf("Number of clauses: %d\n", theory->n_clauses);
            theory->clauses = malloc(num_clauses * sizeof(clause));
            var_map->variables_mappings = malloc((num_vars + 1) * sizeof(var));
            var_map->n_variables_mappings = num_vars + 1;
        }
        else if (line[0] == 'w') {
            // This is the comment line
            // w ID WEIGHT
            int idx_var = 0, index_var;
            n_read = sscanf(line, "%*s %d %lf", &idx_var, &weight);
            if(n_read != 2) {
                fprintf(stderr, "Error parsing line: %s\n", line);
                fclose(fp);
                return;
            }
            index_var = abs(idx_var);

            if (idx_var < 0) {
                var_map->variables_mappings[index_var].weight_false = weight;
            } else {
                var_map->variables_mappings[index_var].weight_true = weight;
            }
        }
        else if (line[0] == 'c' && line[1] == ' ' && line[2] == 'a') {
            // // semiring are specified with the syntax
            // // c a complex
            // n_read = sscanf(line, "%*s %*s %s", task);
            // if(n_read != 1) {
            //     fprintf(stderr, "Error parsing line: %s\n", line);
            //     fclose(fp);
            //     return;
            // }
            // printf("Semiring: %s\n", task);
        }
        else if(line[0] == 'c') {
            // This is a comment line, ignore it
        }
        else {
            // This is a clause line
            theory->clauses[idx_clause].n_terms = 0;
            theory->clauses[idx_clause].terms = NULL;
            
            char *token = strtok(line, " ");
            while (token != NULL) {
                int var = atoi(token);
                if (var == 0) {
                    break; // End of clause
                }
                nt = theory->clauses[idx_clause].n_terms;
                theory->clauses[idx_clause].terms = realloc(theory->clauses[idx_clause].terms, (nt + 1) * sizeof(int)); // Resize to actual number of terms
                theory->clauses[idx_clause].terms[nt] = var;
                theory->clauses[idx_clause].n_terms++;
                token = strtok(NULL, " ");
            }
            idx_clause++;
        }
    }

    fclose(fp);
}