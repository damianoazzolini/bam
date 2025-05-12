#include "cnf_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_var_mapping(var_mapping *var_map) {
    int total = var_map->n_variables_mappings;
    int i = 0;
    while(total > 0) {
        if (var_map->used[i] > 0) {
            printf("Variable %d: %s, %lf, %lf\n", i, var_map->variables_mappings[i].name, var_map->variables_mappings[i].weight_true, var_map->variables_mappings[i].weight_false);
            total--;
        }
        i++;
    }
}

void parse_cnf(char *filename, cnf *theory, var_mapping *var_map) {
    FILE *fp;
    char line[256];
    char *tokenized = NULL;
    // cnf *theory = malloc(sizeof(cnf)); // Allocate memory for cnf struct
    unsigned int num_clauses, num_vars;
    unsigned int nt = 0;
    unsigned int idx_clause = 0;

    theory->n_variables = 0;

    // char *line = NULL;
    // var *variables_mappings = malloc(100 * sizeof(var)); // Allocate memory for variable mappings
    // var_map = malloc(sizeof(var_mapping)); // Allocate memory for variable mapping struct

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
            // This is the problem line
            tokenized = strtok(line, " ");
            tokenized = strtok(NULL, " ");
            tokenized = strtok(NULL, " ");
            num_vars = (unsigned int) strtoul(tokenized, NULL, 10);
            tokenized = strtok(NULL, " ");
            num_clauses = (unsigned int) strtoul(tokenized, NULL, 10);
            printf("Number of variables: %d\n", num_vars);
            printf("Number of clauses: %d\n", num_clauses);
            theory->n_variables = num_vars;
            theory->n_clauses = num_clauses;
            // theory->clauses = malloc(num_clauses * sizeof(clause*)); // Allocate memory for clauses
        }
        else if (line[0] == 'c') {
            // This is the comment line
            // c id name weight_true weight_false
            // c 2 a 0.2 0.8
            tokenized = strtok(line, " ");
            tokenized = strtok(NULL, " ");
            int idx_var = atoi(tokenized);
            printf("Variable %d\n", idx_var);
            // var_map->variables_mappings[var_map->n_variables_mappings].idx_var = idx_var;
            // var_map->variables_mappings[idx_var].idx_var = idx_var;
            var_map->used[idx_var] = 1; // Mark the variable as used
            
            tokenized = strtok(NULL, " ");
            // strcpy(var_map->variables_mappings[var_map->n_variables_mappings].name, tokenized);
            strcpy(var_map->variables_mappings[idx_var].name, tokenized);
            // var_map->variables_mappings[var_map->n_variables_mappings].name[strcspn(var_map->variables_mappings[var_map->n_variables_mappings].name, "\n")] = 0; // Remove newline character
            var_map->variables_mappings[idx_var].name[strcspn(var_map->variables_mappings[idx_var].name, "\n")] = 0; // Remove newline character
            
            tokenized = strtok(NULL, " ");
            double weight = atof(tokenized);
            printf("T: %lf\n", weight);
            // var_map->variables_mappings[var_map->n_variables_mappings].prob = prob_var; // Default probability
            var_map->variables_mappings[idx_var].weight_true = weight;
            tokenized = strtok(NULL, " ");
            weight = atof(tokenized);
            printf("F: %lf\n", weight);
            var_map->variables_mappings[idx_var].weight_false = weight;

            var_map->n_variables_mappings++;
        }
        else {
            // This is a clause line
            // theory.clauses = realloc(theory.clauses, theory.n_clauses * sizeof(clause));
            // clause *new_clause = &theory->clauses[theory->n_clauses - 1];
            theory->clauses[idx_clause].n_terms = 0;
            // theory->clauses[theory->n_clauses].terms = malloc(100 * sizeof(int)); // Allocate memory for terms
            
            char *token = strtok(line, " ");
            while (token != NULL) {
                int var = atoi(token);
                if (var == 0) {
                    break; // End of clause
                }
                nt = theory->clauses[idx_clause].n_terms;
                theory->clauses[idx_clause].terms[nt] = var;
                theory->clauses[idx_clause].n_terms++;
                // new_clause->terms[new_clause->n_terms++] = var;
                token = strtok(NULL, " ");
            }
            // new_clause->terms = realloc(new_clause->terms, new_clause->n_terms * sizeof(int)); // Resize to actual number of terms
            // theory->n_clauses++;
            idx_clause++;
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
    // return theory;
}