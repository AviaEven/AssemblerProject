
#include "lexer.h"
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"
#define SPACE_CHARS " \t\n\f\r\v"
#define SKIP_SPACE(s) while(*s && isspace(*s)) s++
#define SKIP_SPACE_R(s,base) while(*s && isspace(*s) && base != s) s++
#define MAX_REG 7
#define MIN_REG 0
#define MAX_CONST_NUM 511
#define MIN_CONST_NUM -512

static int is_trie_inited = 0;
Trie instruction_lookup = NULL;
Trie directive_lookup = NULL;
static struct asm_instruction_binding {
    const char *inst_name;
    int key;
    const char *src_operand_options; /* I - immed, L - label ( direct addressing ), R - register */
    const char *dest_operand_options;/* I - immed(1), L - label ( direct addressing )(3), R - register(5) */
} asm_instruction_binding[16] = {/*commands with 2 registers*/
    {"mov",mmn14_ast_inst_mov,"ILR","LR"},
    {"cmp",mmn14_ast_inst_cmp,"ILR","ILR"},
    {"add",mmn14_ast_inst_add,"ILR","LR"},
    {"sub",mmn14_ast_inst_sub,"ILR","LR"},
    {"lea",mmn14_ast_inst_lea,"L","LR"},
/*commands with 1 register*/
    {"not",mmn14_ast_inst_not,NULL,"LR"},
    {"clr",mmn14_ast_inst_clr,NULL,"LR"},
    {"inc",mmn14_ast_inst_inc,NULL,"LR"},
    {"dec",mmn14_ast_inst_dec,NULL,"LR"},
    {"jmp",mmn14_ast_inst_jmp,NULL,"LR"},
    {"bne",mmn14_ast_inst_bne,NULL,"LR"},
    {"red",mmn14_ast_inst_red,NULL,"LR"},
    {"prn",mmn14_ast_inst_prn,NULL,"ILR"},
    {"jsr",mmn14_ast_inst_jsr,NULL,"LR"},
/*commands with no registers*/
    {"rts",mmn14_ast_inst_rts,NULL,NULL},
    {"stop",mmn14_ast_inst_stop,NULL,NULL},
};
static struct asm_directive_binding{
    const char * directive_name;
    int key;
}asm_directive_binding[4] = {
    {"data",mmn14_ast_dir_data},
    {"string",mmn14_ast_dir_string},
    {"extern",mmn14_ast_dir_extern},
    {"entry",mmn14_ast_dir_entry}
};
/*this enum provides the state of a label */
enum lexer_valid_label_err {
    label_ok,
    starts_without_alpha,
    contains_none_alpha_numeric,
    label_is_too_long
};

/**
@brief this function checks if a label is valid
@param - 'label' - char. pointer to the label inserted as a parameter 
@return - returns the state of the label
*/
static enum lexer_valid_label_err lexer_is_valid_label(const char * label) {
    int chararcter_count = 0;
    if(!isalpha(*label)) {
        return starts_without_alpha;
    } 
    label++;/*moving forward inside string*/
    while(*label && isalnum(*label)) {
        chararcter_count++;
        label++;
    };
    if(*label !='\0') {
        return contains_none_alpha_numeric;
    }
    if(chararcter_count > MAX_LABEL_LEN) {
        return label_is_too_long;
    }
    return label_ok;
}
/**
@brief - this funciton initializes 2 trie , one for instruction and one for directive
*/
static void lexer_trie_creation() {
    int i;
    instruction_lookup  = trie();
    directive_lookup    = trie();
    for(i=0;i<16;i++) {
        trie_insert(instruction_lookup, asm_instruction_binding[i].inst_name , &asm_instruction_binding[i]);
    }
    for(i=0;i<4;i++) {
        trie_insert(directive_lookup,asm_directive_binding[i].directive_name , &asm_directive_binding[i]);
    }
    is_trie_inited = 1;
}


/**
@brief - this function parses a string and performs checks
@param - 
	'num_str' - char pointer to the input string
	'endptr' - double pointer from type char,set to point to the character after being parsed
	'num' - pointer from type 'long', after being parsed, it will be assigned  the parsed integer value
	'min','max' - range of parsed number 
	
@return - the return value allow the calling code to understand the outcome
*/
static int parsing_number(const char *num_str,char **endptr,long * num,long min , long max) {
    char * char_index;
    *num = strtol(num_str,&char_index,10);/*the 'strtol' function is used to convert the initial part of the 'num_str' to  'long'*/
    errno = 0;
    while(isspace(*char_index)) char_index++;/*to skip white spaces*/
    if(*char_index != '\0') {
        return -1;/*in case of encountering   a non - numric value characters*/
    }
    if(errno == ERANGE) {
        return -2;/*to indicate that that the numeric value is out of range */
    }
    if(*num > max || *num < min) {
        return -3;/*to indicate that the number is out of the valid range*/
    }
    if(endptr)/*checking if not null*/
        *endptr = char_index;
    return 0;
}
/**
 * @brief - this function categorzies the operands
 * @param  'operand_string' - char array pointer,that represents the operand to be parrsed
	'label' - double pointer of character array,used to pass an adress where the function stores a pointer to a labelstring if the operand is
identified as a label
	'const_number' - int pointer , This is a pointer to an integer. It's used to pass an address where the function can store the parsed constant number if the operand is identified as a constant.
	'register_number' -  This is a pointer to an integer. It's used to pass an address where the function can store the parsed register number if the operand is identified as a register.
 * @return char returns I L or R or returns N if unknown, or E if empty , or F for constant number overflow
 */
static char parsing_operand_i(char * operand_string,char ** label,int * const_number , int * register_number) {
    char * temp;/*used to analyze the string*/
    char * temp2;/*used to analyze the string*/
    long num;/*used to store the value of the parsed number */
    int ret;/*holds the return value of the parsing_number function*/
    if(operand_string == NULL)
        return 'E';
    SKIP_SPACE(operand_string);
    if( *operand_string == '\0')
        return 'E';
    if(*operand_string == '@') {/*check for register*/
        if(*(operand_string + 1) == 'r') {
            if(*(operand_string + 2) == '+' || *(operand_string + 2) == '-') {
                return 'N';
            }
            if(parsing_number(operand_string+2,NULL,&num,MIN_REG,MAX_REG)!= 0) {
                return 'N';
            }
            if(register_number)
                *register_number = (int)num;
            return 'R';
        }
        return 'N';
    }
    if(isalpha(*operand_string)) {/*check for label*/
        temp2 = temp = strpbrk(operand_string, SPACE_CHARS);
        if(temp) {
            *temp = '\0';
            temp++;
            SKIP_SPACE(temp);
            if(*temp !='\0') {
                *temp2 = ' ';
                return 'N';
            }
        }
        if(lexer_is_valid_label(operand_string) != label_ok) {
            return 'N';
        }
        if(label)
            (*label) = operand_string;

        return 'L';
    }
/*brief for this condition- ret gets the value from 'parsing_number'  and the code checks if the value is less than -2 which = error if so return F*/
/*if 0 then the parsin succeeded then returns I*/
    if((ret = parsing_number(operand_string,NULL,&num,MIN_CONST_NUM,MAX_CONST_NUM)) < -2) {
        return 'F';
    }else if(ret == 0) {
        if(const_number)
            (*const_number) = num;
        return 'I';
    }
    return 'N';
}
/**
@brief - this function is in charge of parsing and validating operands of instructions
@param - 'ast'- pointer to the abstract syntx tree
	 'operands_string' - char pointer, it holds the operands
	 'inst_binding_var' - pointer to the instruction binding table
*/

static void lexer_parse_instruction_operands(mmn14_ast * ast,char * operands_string,struct asm_instruction_binding * inst_binding_var) {
    char operand_i_option;/*holds the type of the operand*/
    char *comma_second_position;/*holds the position of second comma*/
    char * comma_first_postion = NULL;/*holds the position of the first comma*/
    if(operands_string)/*if 'operands_string != NULL*/
         comma_first_postion = strchr(operands_string, ',');
    else {
        if(inst_binding_var->dest_operand_options !=NULL) {
            sprintf(ast->syntax_error,"instruction:'%s' expects one operand.",inst_binding_var->inst_name);
            return;
        }
        return;
    }
    if(comma_first_postion) {/*this means that there are many operands*/
        comma_second_position = strchr(comma_first_postion+1,',');
        if(comma_second_position) {
            strcpy(ast->syntax_error,"found two or more seperator ',' tokens.");
            return;
        }
        if(inst_binding_var->src_operand_options ==NULL) {
            sprintf(ast->syntax_error,"instruction:'%s' expects only one operand but got two.",inst_binding_var->inst_name);
            return;
        }
        *comma_first_postion= '\0';
        /* here goes the logic for each one*/
        operand_i_option =  parsing_operand_i(operands_string,&ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[0].label,
                                                                &ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[0].const_number,
                                                            &ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[0].register_number);
        if(operand_i_option == 'N') {
            sprintf(ast->syntax_error,"unknown operand:'%s' for source",operands_string);
            return;
        }
        if(operand_i_option == 'F') {
            sprintf(ast->syntax_error,"overflow immediate operand:'%s' for source",operands_string);
            return;
        }
        if(operand_i_option == 'E') {
            sprintf(ast->syntax_error,"got no operand text for source");
            return;
        }
        if(strchr(inst_binding_var->src_operand_options,operand_i_option) == NULL) {
            sprintf(ast->syntax_error,"instruction: '%s' :source operand:'%s' is not supported",inst_binding_var->inst_name,operands_string);
            return;
        }
        ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[0] = operand_i_option == 'I' ?  mmn14_ast_operand_opt_const_number : operand_i_option == 'R' ? mmn14_ast_operand_opt_register : mmn14_ast_operand_opt_label;
        operands_string =comma_first_postion+1;
        operand_i_option =  parsing_operand_i(operands_string,&ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].label,
                                                                &ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].const_number,
                                                            &ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].register_number);
        if(operand_i_option == 'N') {
            sprintf(ast->syntax_error,"unknown operand:'%s' for destination",operands_string);
            return;
        }
        if(operand_i_option == 'F') {
            sprintf(ast->syntax_error,"overflow immediate operand:'%s' for destination",operands_string);
            return;
        }
        if(operand_i_option == 'E') {
            sprintf(ast->syntax_error,"got no operand text for destination");
            return;
        }
        if(strchr(inst_binding_var->dest_operand_options,operand_i_option) == NULL) {
            sprintf(ast->syntax_error,"instruction: '%s' :destination operand:'%s' is not supported",inst_binding_var->inst_name,operands_string);
            return;
        }
        ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[1] = operand_i_option == 'I' ?  mmn14_ast_operand_opt_const_number : operand_i_option == 'R' ? mmn14_ast_operand_opt_register : mmn14_ast_operand_opt_label;
    }else {/*only one or none operand*/
        if(inst_binding_var->src_operand_options != NULL) {
            sprintf(ast->syntax_error,"instruction:'%s' expects seperator token: ','.",inst_binding_var->inst_name);
            return;
        }
        operand_i_option =  parsing_operand_i(operands_string,&ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].label,
                                                                &ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].const_number,
                                                            &ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].register_number); 
        if(operand_i_option !='E' && inst_binding_var->dest_operand_options ==NULL) {
            sprintf(ast->syntax_error,"instruction:'%s' expects no operands.",inst_binding_var->inst_name);
            return;
        }
        if(operand_i_option == 'E') {
            sprintf(ast->syntax_error,"instruction:'%s' expects one operand.",inst_binding_var->inst_name);
            return;
        }
        if(operand_i_option == 'F') {
            sprintf(ast->syntax_error,"overflow immediate operand:'%s' for destination",operands_string);
            return;
        }
        if(operand_i_option == 'N') {
            sprintf(ast->syntax_error,"unknown operand:'%s' for destination",operands_string);
            return;
        }
        if(strchr(inst_binding_var->dest_operand_options,operand_i_option) == NULL) {
            sprintf(ast->syntax_error,"instruction: '%s' :destination operand:'%s' is not supported",inst_binding_var->inst_name,operands_string);
            return;
        }
        ast->dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[1] = operand_i_option == 'I' ?  mmn14_ast_operand_opt_const_number : operand_i_option == 'R' ? mmn14_ast_operand_opt_register : mmn14_ast_operand_opt_label;
    }
}
/**
@brief - same function as before, this time  for the directives instead of instructions
@param - same as as previous function
*/
static void lexer_parse_directive(mmn14_ast *ast,char * operands_string,struct asm_directive_binding * direc_binding_var) {
    char * sep;/*holds positions of qoutation marks and seperators*/
    char * sep2;/*holds positions of qoutation marks and seperators*/
    int num_count = 0;/*in order to keep track of the number of numeric data in case of type 'data'*/
    int curr_num;/*holds the current parsed numeric value*/
    if(direc_binding_var->key <=mmn14_ast_dir_entry) {/*checks if a label operand is required*/
        if(parsing_operand_i(operands_string,&ast->dir_or_inst.mmn14_ast_dir.dir_operand.label_name,NULL,NULL) != 'L') {
            sprintf(ast->syntax_error,"directive:'%s' with invalid operand:'%s'.",direc_binding_var->directive_name,operands_string);
            return;
        }
    }
    else if(direc_binding_var->key == mmn14_ast_dir_string) {
        sep = strchr(operands_string,'"');
        if(!sep) {
            sprintf(ast->syntax_error,"directive: '%s' has no opening '\"': '%s'.",direc_binding_var->directive_name,operands_string);
            return;
        }
        sep++;
        sep2 = strrchr(sep,'"');
        if(!sep2) {
            sprintf(ast->syntax_error,"directive: '%s' has no closing '\"': '%s'.",direc_binding_var->directive_name,operands_string);
            return;
        }
        *sep2 = '\0';
        sep2++;
        SKIP_SPACE(sep2);
        if(*sep2 !='\0') {
            sprintf(ast->syntax_error,"directive: '%s' has extra text after the string:'%s'.",direc_binding_var->directive_name,sep2);
            return;
        }
        ast->dir_or_inst.mmn14_ast_dir.dir_operand.string = sep;
    }
    else if(direc_binding_var->key == mmn14_ast_dir_data) {
        
        do {
            sep = strchr(operands_string,',');
            if(sep)  {
                    *sep = '\0';
            }
            switch(parsing_operand_i(operands_string,NULL,&curr_num,NULL)) {
                case 'I':/*if the operand is valid then we store the number in the data array*/
                    ast->dir_or_inst.mmn14_ast_dir.dir_operand.data.data[num_count] = curr_num;
                    num_count++;
                    ast->dir_or_inst.mmn14_ast_dir.dir_operand.data.data_count = num_count;
                break;
                case 'F':/*if the number is overflowed*/
                    sprintf(ast->syntax_error,"directive: '%s' : overflowed number:'%s'.",direc_binding_var->directive_name,operands_string);
                    return;
                break;
                case 'E':/*if operand is empty string*/
                    sprintf(ast->syntax_error,"directive: '%s' : got empty string ( no operands ) but expected immediate number.",direc_binding_var->directive_name);
                    return;
                break;
                default:/*if it's not a valid immediate number*/
                    sprintf(ast->syntax_error,"directive: '%s' : got none number string:'%s'.",direc_binding_var->directive_name,operands_string);
                    return;
                break;
            }
            if(sep) 
                operands_string = sep+1;
            else {
                break;
            }
        }while(1);
    }
}

void lexer_deallocate_mem() {
    is_trie_inited = 0;
    trie_destroy(&directive_lookup);
    trie_destroy(&instruction_lookup);
}

mmn14_ast lexer_get_ast(char *input_line) {
    mmn14_ast ast = {0};
    enum lexer_valid_label_err lexer_valid_label_error;
    struct asm_instruction_binding * inst_binding_var = NULL;
    struct asm_directive_binding * direc_binding_var = NULL;
    char *tempo1, * tempo2;
    if(!is_trie_inited) {/*in case the trei is not inizialized, we initialize it*/
        lexer_trie_creation();
    }
    input_line[strcspn(input_line, "\r\n")] = 0;
    SKIP_SPACE(input_line);
    tempo1 = strchr(input_line,':');
    if(tempo1) {
        tempo2 = strchr(tempo1+1,':');
        if(tempo2) {
            strcpy(ast.syntax_error,"token ':' appears twice in this line.");
            return ast;
        }
        (*tempo1) = '\0';
        switch(lexer_valid_label_error = lexer_is_valid_label(input_line)) {
            case starts_without_alpha:
                sprintf(ast.syntax_error,"label:'%s' starts without alphabetic character.",input_line);
            break;
            case contains_none_alpha_numeric:
                sprintf(ast.syntax_error,"label:'%s' contains none alpha numeric characters.",input_line);
            break;
            case label_is_too_long:
                sprintf(ast.syntax_error,"label:'%s' is longer than max label len :%d.",input_line,MAX_LABEL_LEN);
            break;
            case label_ok:
                strcpy(ast.label_name,input_line);
            break;
        }
        if(lexer_valid_label_error != label_ok) {
            return ast;
        }
        input_line = tempo1+1;
        SKIP_SPACE(input_line);
    }
/*checks if only a label and nothing else*/
    if(*input_line == '\0' && ast.label_name[0] != '\0') {
        sprintf(ast.syntax_error,"line contains only a label:'%s'",ast.label_name);
        return ast;
    }
    tempo1 = strpbrk(input_line, SPACE_CHARS);
    if(tempo1)  {
        *tempo1 = '\0';
        tempo1++;
        SKIP_SPACE(tempo1);
    }
    if(*input_line == '.') {/*now looking for directive*/
        direc_binding_var = trie_exists(directive_lookup, input_line+1);
        if(!direc_binding_var) {
            sprintf(ast.syntax_error,"unknown directive:'%s'.",input_line+1);
            return ast;
        }
        ast.mmn14_ast_opt = mmn14_ast_dir;
        ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt = direc_binding_var->key;
        lexer_parse_directive(&ast, tempo1, direc_binding_var);
        return ast;
    }
    inst_binding_var = trie_exists(instruction_lookup, input_line);
    if(!inst_binding_var) {
        sprintf(ast.syntax_error,"unknown keyword:'%s'.",input_line);
        return ast;
    }

    ast.mmn14_ast_opt = mmn14_ast_inst;
    ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_opt = inst_binding_var->key;
    lexer_parse_instruction_operands(&ast,tempo1,inst_binding_var);
    
    return ast;
}
