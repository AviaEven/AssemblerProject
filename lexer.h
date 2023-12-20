
#ifndef __LEXER_H_
#define __LEXER_H_

#define MAX_NUMBER_DATA 80
#define MAX_LABEL_LEN 31
struct mmn14_ast {
    char syntax_error[200];
    char label_name[MAX_LABEL_LEN + 1];
    enum {
        mmn14_ast_inst,
        mmn14_ast_dir
    }mmn14_ast_opt;
    union {
        struct {
            enum {
                mmn14_ast_dir_extern,
                mmn14_ast_dir_entry,
                mmn14_ast_dir_string,
                mmn14_ast_dir_data
            }mmn14_ast_dir_opt;
            union {
                char * label_name;
                char * string;
                struct {
                    int data[MAX_NUMBER_DATA];
                    int data_count;
                }data;
            }dir_operand;
        }mmn14_ast_dir;
        struct {
            enum {
                mmn14_ast_inst_mov,
                mmn14_ast_inst_cmp,
                mmn14_ast_inst_add,
                mmn14_ast_inst_sub,
                

                mmn14_ast_inst_not,
                mmn14_ast_inst_clr,
                mmn14_ast_inst_lea,
                
                mmn14_ast_inst_inc,
                mmn14_ast_inst_dec,
                mmn14_ast_inst_jmp,
                mmn14_ast_inst_bne,
                mmn14_ast_inst_red,
                mmn14_ast_inst_prn,
                mmn14_ast_inst_jsr,

                mmn14_ast_inst_rts,
                mmn14_ast_inst_stop
            }mmn14_ast_inst_opt; 
            enum {
                mmn14_ast_operand_opt_none = 0,
                mmn14_ast_operand_opt_const_number = 1,
                mmn14_ast_operand_opt_register = 5,
                mmn14_ast_operand_opt_label = 3
            }mmn14_ast_inst_operand_opt[2];
            union {
                int const_number;
                int register_number;
                char * label;
            }mmn14_ast_inst_operands[2];
        }mmn14_ast_inst;
    }dir_or_inst;
};
typedef struct mmn14_ast mmn14_ast;
/**
@brief - deleting allocated memory
*/
void lexer_deallocate_mem();

/**
@brief - process a line of input,checks for labels, instructions, and directives, validates labels, and constructs an abstract syntax tree
@param - input_line - char pointer to reperesent to input 
@return - return an abstract syntax tree instance
*/
mmn14_ast lexer_get_ast(char *input_line);

#endif
