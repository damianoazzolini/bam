#include "cnf_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_var_mapping(var_mapping *var_map) {
    for (int i = 0; i < var_map->n_variables_mappings; i++) {
        printf("Variable %d: %s, %d, %lf\n", var_map->variables_mappings[i].idx_var, var_map->variables_mappings[i].name, var_map->variables_mappings[i].idx_var, var_map->variables_mappings[i].prob);
    }
}


cnf parse_cnf(char *filename, var_mapping *var_map) {
    FILE *fp;
    // char *line = NULL;
    // var *variables_mappings = malloc(100 * sizeof(var)); // Allocate memory for variable mappings
    // var_map = malloc(sizeof(var_mapping)); // Allocate memory for variable mapping struct

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    // Read the file and process it
    char line[256];
    char *tokenized = NULL;
    cnf theory;
    theory.n_clauses = 0;
    theory.clauses = NULL;
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Process the line
        // each line has the format of a cnf line, where lines start with 'p cnf' and end with '0'
        // The first line is the problem line, which starts with 'p cnf'
        // The second line is the variable line, which starts with 'c'
        // The rest of the lines are the clauses, which start with a number and end with '0'
        
        if(line[0] == 'p') {
            // This is the problem line
            tokenized = strtok(line, " ");
            tokenized = strtok(NULL, " ");
            tokenized = strtok(NULL, " ");
            int num_vars = atoi(tokenized);
            tokenized = strtok(NULL, " ");
            int num_clauses = atoi(tokenized);
            printf("Number of variables: %d\n", num_vars);
            printf("Number of clauses: %d\n", num_clauses);
        }
        else if (line[0] == 'c') {
            // This is the comment line
            tokenized = strtok(line, " ");
            tokenized = strtok(NULL, " ");
            int idx_var = atoi(tokenized);
            var_map->variables_mappings[var_map->n_variables_mappings].idx_var = idx_var;
            
            tokenized = strtok(NULL, " ");
            strcpy(var_map->variables_mappings[var_map->n_variables_mappings].name, tokenized);
            var_map->variables_mappings[var_map->n_variables_mappings].name[strcspn(var_map->variables_mappings[var_map->n_variables_mappings].name, "\n")] = 0; // Remove newline character
            
            tokenized = strtok(NULL, " ");
            double prob_var = atof(tokenized);
            printf("%lf\n", prob_var);
            
            var_map->variables_mappings[var_map->n_variables_mappings].prob = prob_var; // Default probability
            var_map->n_variables_mappings++;
        }
        else {
            // This is a clause line
            theory.n_clauses++;
            theory.clauses = realloc(theory.clauses, theory.n_clauses * sizeof(clause));
            clause *new_clause = &theory.clauses[theory.n_clauses - 1];
            
            new_clause->terms = malloc(100 * sizeof(int)); // Allocate memory for terms
            new_clause->n_terms = 0;

            char *token = strtok(line, " ");
            while (token != NULL) {
                int var = atoi(token);
                if (var == 0) {
                    break; // End of clause
                }
                new_clause->terms[new_clause->n_terms++] = var;
                token = strtok(NULL, " ");
            }
            new_clause->terms = realloc(new_clause->terms, new_clause->n_terms * sizeof(int)); // Resize to actual number of terms
        }
    }
    // Close the file
    fclose(fp);
    // // Free the variables mappings
    // for (int i = 0; i < n_variables_mappings; i++) {
    //     // free(variables_mappings[i].name);
    // }
    // free(variables_mappings);
    // Free the clauses
    // for (int i = 0; i < theory.n_clauses; i++) {
    //     free(theory.clauses[i].terms);
    // }
    // free(theory.clauses);
    return theory;
}