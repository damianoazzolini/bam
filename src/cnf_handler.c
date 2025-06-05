#include "cnf_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cnf *init_cnf() {
    cnf *theory = malloc(sizeof(cnf));
    theory->n_clauses = 0;
    theory->n_variables = 0;
    theory->state = CNF_UNSOLVED;
    theory->clauses = NULL;
    // theory->matrix_representation = NULL;
    return theory;
}

void free_cnf(cnf *theory) {
    if (theory != NULL) {
        for (unsigned int i = 0; i < theory->n_clauses; i++) {
            free(theory->clauses[i].terms);
        }
        free(theory->clauses);
        // for (unsigned int i = 0; i < theory->n_variables; i++) {
        //     // free(theory->matrix_representation[i]);
        // }
        free(theory);
    }
}

char *get_weight_string(weight_t weight, int weight_type) {
    char *weight_str = malloc(32 * sizeof(char));
    if(weight_type == 0) {
        snprintf(weight_str, 32, "%lf", weight.real_weight);
    }
    else if(weight_type == 1) {
        snprintf(weight_str, 32, "%lf+%lfi", weight.complex_weight.real, weight.complex_weight.imag);
    }
    return weight_str;
}

void print_cnf(const cnf *theory) {
    printf("Number of clauses: %d\n", theory->n_clauses);
    printf("Number of variables: %d\n", theory->n_variables);
    if(theory->state == CNF_INCONSISTENT) {
        printf("State: CNF_INCONSISTENT\n");
    } else if(theory->state == CNF_SOLVED) {
        printf("State: CNF_SOLVED\n");
    } else {
        printf("State: CNF_UNSOLVED\n");
    }
    printf("State: %d\n", theory->state);
    printf("Clauses: %d\n", theory->n_clauses);
    for (unsigned int i = 0; i < theory->n_clauses; i++) {
        printf("Clause %d with %d terms: ", i, theory->clauses[i].n_terms);
        for (unsigned int j = 0; j < theory->clauses[i].n_terms; j++) {
            printf("%d ", theory->clauses[i].terms[j]);
        }
        printf("\n");
    }
}

void set_variable(cnf *theory, int idx_variable, int phase) {
    // set the current variable and check whether the CNF became unsatisfiable 
    // by setting the variable idx_var to true (1) or false (-1)
    // if there is a clause with a single literal and we require that this literal
    // is different from the phase, then unsatisfiable: return -1
    // otherwise, if the phase is the same satisfiable: return 1
    // else don't know: return 0
    for (unsigned int i = 0; i < theory->n_clauses; i++) {
        for(unsigned int j = 0; j < theory->clauses[i].n_terms; j++) {
            int current_var = theory->clauses[i].terms[j];
            if (abs(current_var) == idx_variable) {
                if ((current_var > 0 && phase > 0) || (current_var < 0 && phase < 0)) {
                    // the clause is sat: remove the whole clause
                    theory->clauses[i].n_terms = 0; // mark the clause as satisfied
                    free(theory->clauses[i].terms);
                    theory->clauses[i].terms = NULL;
                }
                else if ((current_var > 0 && phase < 0) || (current_var < 0 && phase > 0)) {
                    // remove the current variable from the clause
                    if(theory->clauses[i].n_terms == 1) {
                        // unit clause, unsat and I can stop, no need to set the variables
                        theory->state = CNF_INCONSISTENT;
                        return; // unsatisfiable
                    }
                    else {
                        // remove the variable from the clause
                        // printf("Removing variable %d from clause %d\n", current_var, i);
                        // printf("nterms before removal: %d\n", theory->clauses[i].n_terms);
                        int *new_terms = malloc((theory->clauses[i].n_terms - 1) * sizeof(int));
                        unsigned int k = 0;
                        // int skipped = 0;
                        for (unsigned int l = 0; l < theory->clauses[i].n_terms; l++) {
                            if (abs(theory->clauses[i].terms[l]) != abs(current_var)) {
                                new_terms[k] = theory->clauses[i].terms[l];
                                k++;
                            }
                        }
                        free(theory->clauses[i].terms);
                        // printf("nterms after removal: %d\n", k);
                        // theory->clauses[i].n_terms--; // since there may be tautologies: v -v
                        theory->clauses[i].n_terms = k; // since there may be tautologies: v -v
                        theory->clauses[i].terms = new_terms;
                    }
                }
            }
        }
    }

    // check if all the clauses have been satisfied
    for (unsigned int i = 0; i < theory->n_clauses; i++) {
        if (theory->clauses[i].n_terms != 0) {
            return;
        }
    }
    theory->state = CNF_SOLVED; // all clauses are satisfied

}

// void print_cnf_matrix(const cnf *theory) {
//     if (theory->matrix_representation == NULL) {
//         printf("Matrix representation is NULL\n");
//         return;
//     }
//     for (unsigned int i = 0; i < theory->n_clauses; i++) {
//         for (unsigned int j = 0; j < theory->n_variables + 1; j++) {
//             printf("%d ", theory->matrix_representation[i][j]);
//         }
//         printf("\n");
//     }
// }

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

void print_var_mapping(const var_mapping *var_map, int weight_type) {
    char *weight_true_string, *weight_false_string;
    for(int i = 1; i < var_map->n_variables_mappings; i++) {
        weight_true_string = get_weight_string(var_map->variables_mappings[i].weight_true, weight_type);
        weight_false_string = get_weight_string(var_map->variables_mappings[i].weight_false, weight_type);
        printf("Variable %d: %s, %s\n", i, weight_true_string, weight_false_string);
        free(weight_true_string);
        free(weight_false_string);
    }
}

// weight_value = 0 -> real weight
// weight_value = 1 -> complex weight
int read_weight_line(char *line, var_mapping *var_map, int num_vars, int type, int weight_type) {
    // type 0: w ID WEIGHT
    // type 1: c p weight ID WEIGHT 0
    int idx_var = 0, index_var, n_read = 0;
    weight_t weight;
    if(type == 0) {
        if(weight_type == 0) {
            n_read = sscanf(line, "%*s %d %lf", &idx_var, &weight.real_weight);
        }
        else if(weight_type == 1) {
            char imag_weight_s[16];
            n_read = sscanf(line, "%*s %d %lf+%s", &idx_var, &weight.complex_weight.real, imag_weight_s);
            weight.complex_weight.imag = atof(imag_weight_s);
        }
    }
    else if(type == 1) {
        if(weight_type == 0) {
            n_read = sscanf(line, "%*s %*s %*s %d %lf %*d", &idx_var, &weight.real_weight);
        }
        else if(weight_type == 1) {
            char imag_weight_s[16];
            n_read = sscanf(line, "%*s %*s %*s %d %lf+%s %*d", &idx_var, &weight.complex_weight.real, imag_weight_s);
            weight.complex_weight.imag = atof(imag_weight_s);
        }
    }

    if(weight_type == 0 && n_read != 2 || weight_type == 1 && n_read != 3) {
        return -1;
    }

    index_var = abs(idx_var);

    if(index_var > num_vars) {
        fprintf(stderr, "Error: variable index %d out of bounds (1 to %d)\n", index_var, num_vars);
        exit(-1);
    }

    if (idx_var < 0) {
        var_map->variables_mappings[index_var].weight_false = weight;
    } else {
        var_map->variables_mappings[index_var].weight_true = weight;
    }

    return 0;
}

int *compute_stats_cnf(const cnf *theory) {
    int *term_occurrences_pos = calloc(theory->n_variables + 1, sizeof(int));
    int *term_occurrences_neg = calloc(theory->n_variables + 1, sizeof(int));
    int *term_occurrences = calloc(theory->n_variables + 1, sizeof(int));
    int *order = calloc(theory->n_variables + 1, sizeof(int));

    for (unsigned int i = 0; i < theory->n_clauses; i++) {
        for (unsigned int j = 0; j < theory->clauses[i].n_terms; j++) {
            int term = theory->clauses[i].terms[j];
            if (term > 0) {
                term_occurrences_pos[term]++;
            }
            else {
                term_occurrences_neg[-term]++;
            }
            term_occurrences[abs(term)]++;
        }
    }

    // sort the term_occurrences array and then sort the variables by the number of occurrences
    int *sorted_indices = malloc(theory->n_variables * sizeof(int));
    for(int i = 1; i <= theory->n_variables; i++) {
        sorted_indices[i - 1] = i;
    }
    // sort the indices by the number of occurrences
    for(int i = 0; i < theory->n_variables - 1; i++) {
        for(int j = i + 1; j < theory->n_variables; j++) {
            if(term_occurrences[sorted_indices[i]] < term_occurrences[sorted_indices[j]]) {
                int temp = sorted_indices[i];
                sorted_indices[i] = sorted_indices[j];
                sorted_indices[j] = temp;
            }
        }
    }
    for(int i = 0; i < theory->n_variables; i++) {
        order[sorted_indices[i]] = i + 1;
    }

    #ifdef DEBUG_MODE
    for(int i = 1; i <= theory->n_variables; i++) {
        printf("c Variable %d: (%d,%d) = %d (%d)\n", i, term_occurrences_pos[i], term_occurrences_neg[i], term_occurrences[i], order[i]);
    }
    #endif
    
    free(term_occurrences_pos);
    free(term_occurrences_neg);
    free(term_occurrences);

    return order;
}

// weight_value = 0 -> real weight
// weight_value = 1 -> complex weight
void parse_cnf(char *filename, cnf *theory, var_mapping *var_map, int weight_type) {
    FILE *fp;
    char line[256];
    char task[32];
    unsigned int num_clauses, num_vars;
    int n_read;
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
                exit(-1);
            }
            theory->n_variables = num_vars;
            theory->n_clauses = num_clauses;
            printf("c Number of variables: %d\n", theory->n_variables);
            printf("c Number of clauses: %d\n", theory->n_clauses);
            theory->clauses = malloc(num_clauses * sizeof(clause));

            // theory->matrix_representation = malloc(num_clauses * sizeof(int*));
            // for (unsigned int i = 0; i < num_vars; i++) {
            //     theory->matrix_representation[i] = calloc(num_vars + 1, sizeof(int));
            // }
            var_map->variables_mappings = malloc((num_vars + 1) * sizeof(var));
            var_map->n_variables_mappings = num_vars + 1;
        }
        else if (line[0] == 'w') {
            // w ID WEIGHT
            if(read_weight_line(line, var_map, num_vars, 0, weight_type) != 0) {
                fprintf(stderr, "Error parsing line: %s\n", line);
                fclose(fp);
                exit(-1);
            }
        }
        else if(line[0] == 'c' && line[1] == ' ' && line[2] == 'p' && line[3] == ' ' && line[4] == 'w') {
            // c p weight ID WEIGHT 0
            if(read_weight_line(line, var_map, num_vars, 1, weight_type) != 0) {
                fprintf(stderr, "Error parsing line: %s\n", line);
                fclose(fp);
                exit(-1);
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
            if(idx_clause >= theory->n_clauses) {
                fprintf(stderr, "Error: more clauses than specified in the problem line.\n");
                fclose(fp);
                exit(-1);
            }
            theory->clauses[idx_clause].n_terms = 0;
            theory->clauses[idx_clause].terms = NULL;
            
            char *token = strtok(line, " ");
            while (token != NULL) {
                int current_var = atoi(token);
                if (current_var == 0) {
                    break; // End of clause
                }
                
                // check: if there is a tautology, i.e., a variable and its negation in the same clause
                // skip the clause
                nt = theory->clauses[idx_clause].n_terms;
                // int tautology_found = 0;
                // for(int i = 0; i < nt; i++) {
                //     if((abs(theory->clauses[idx_clause].terms[i]) == abs(current_var)) && (theory->clauses[idx_clause].terms[i] = -current_var)  ) {
                //         // Tautology found, skip the clause
                //         fprintf(stdout, "[WARNING]: tautology found in clause %d: skipping clause\n", idx_clause);
                //         theory->clauses[idx_clause].n_terms = 0;
                //         free(theory->clauses[idx_clause].terms);
                //         theory->clauses[idx_clause].terms = NULL;
                //         tautology_found = 1;
                //         break;
                //     }
                // }
                // if(tautology_found) {
                //     break;
                // }
                theory->clauses[idx_clause].terms = realloc(theory->clauses[idx_clause].terms, (nt + 1) * sizeof(int)); // Resize to actual number of terms
                theory->clauses[idx_clause].terms[nt] = current_var;
                // if(var > 0) {
                //     theory->matrix_representation[idx_clause][abs(var)] = 1;
                // }
                // else {
                //     theory->matrix_representation[idx_clause][abs(var)] = -1;
                // }
                theory->clauses[idx_clause].n_terms++;
                token = strtok(NULL, " ");
            }
            idx_clause++;
        }
    }

    printf("c Parsed %d clauses.\n", idx_clause);
    if (idx_clause != theory->n_clauses) {
        fprintf(stderr, "Error: number of clauses parsed (%d) does not match the number specified in the problem line (%d).\n", idx_clause, theory->n_clauses);
    }

    fclose(fp);
}