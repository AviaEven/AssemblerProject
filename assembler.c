#include "../common/mmn14_common.h"
#include "../lexer/lexer.h"
#include "../preprocessor/preprocessor.h"
#include "../output_unit/output_unit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define ANSI_RESET              "\x1b[0m"
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define BASE_ADDR               100
#define MAX_LINE_LENGTH 81

struct missing_symbol  {
    char symbol_name[MAX_LABEL_LEN +1];
    int call_line;
    unsigned int *machine_word;
    unsigned int call_address;
};
/**
@brief - this function prints error message
@param 'file name'- the name of the file where error happened
	'line_number' - indicated what line the error happened
	string_format - tells the format of the eeror message
*/
static void assembler_error_msg(const char *file_name,int line_number,const char * string_format,...) {
    va_list vl;
    va_start(vl,string_format);
    printf("%s:%d " ANSI_COLOR_RED "error: " ANSI_RESET,file_name,line_number );
    vprintf(string_format,vl);
    printf("\n");
    va_end(vl);
}
/**
@brief - like previous function but this time for warning messages
@param - also same as previous function
*/
static void assembler_warn_msg(const char *file_name,int line_number,const char * string_format,...) {/********************************************/
    va_list vl;
    va_start(vl,string_format);
    printf("%s:%d " ANSI_COLOR_YELLOW "warning: " ANSI_RESET,file_name,line_number );
    vprintf(string_format,vl);
    printf("\n");
    va_end(vl);
}

/**
@brief- this function creates a new instance of 'missing_symbol'
@param- 'copy' - represents pointer to an existing 'missing_symbol' object
*/
static void *create_missing_symbol(const void * copy) {
    return memcpy(malloc(sizeof(struct missing_symbol)),copy,sizeof(struct missing_symbol));
}
/**
@brief - this function releases memory that is related to 'missing_symbol' object
@param - 'item' - pointer to the missing_symbol object
*/
static void destroy_missing_symbol(void * item) {
    free(item);
}


/**
@brief - this function parses assembly lines
@param -'am_file' - pointer to the input file
	'object_file' - pointer to the object file where the created data will be stored
	'am_file_name' - the name of the assembly source file
@return- 1 if compilation completed, 0 if encountered errors
*/
static int assembler_compile(FILE * am_file,struct object_file * obj,const char * am_file_name) {
    char line_buffer[MAX_LINE_LENGTH + 1] = {0};
    mmn14_ast ast;
    struct missing_symbol m_symbol = {0};
    struct symbol symbol_var = {0};
    struct symbol * find_symbol;
    void * const  * begin;/*used for iteration*/
    void * const *end ;/*used for iteration*/
    const char * data_str;/*used for processing string data in directives*/
    unsigned int extern_call_addr = 0;
    int line_counter = 1;
    unsigned int machine_word = 0;
    int i;
    unsigned int * just_inserted_m_word;
    int err_code = 1;
    Vector missing_symbol_table = new_vector(create_missing_symbol,destroy_missing_symbol);
    while(fgets(line_buffer,sizeof(line_buffer),am_file)) {
        ast = lexer_get_ast(line_buffer);
        if(ast.syntax_error[0] !='\0') {
            assembler_error_msg(am_file_name, line_counter, "%s",ast.syntax_error);
            line_counter++;
            err_code = 1;
            continue;
        }
        if(ast.label_name[0] !='\0') {/*if the line contains a label*/
            strcpy(symbol_var.sym_name,ast.label_name);
            find_symbol = trie_exists(obj->symbol_table_lookup, ast.label_name);
            if(ast.mmn14_ast_opt == mmn14_ast_inst) {/*if line in an instruction*/
                if(find_symbol) {
                    if(find_symbol->sym_type != sym_entry) {/*Error: Redefinition of a non-entry symbol*/
                        
                        assembler_error_msg(am_file_name, line_counter,"label: '%s' was already defined as '%s' in line : %d",find_symbol->sym_name,symbol_type_strings[find_symbol->sym_type],find_symbol->declared_line);
                        err_code =0;
                    }else {
                        find_symbol->sym_type = sym_entry_code;
                        find_symbol->address = (unsigned int)vector_get_item_count(obj->code_section) + BASE_ADDR;
                    }
                }else {
                    symbol_var.sym_type = sym_code;
                    symbol_var.address = (unsigned int)vector_get_item_count(obj->code_section) + BASE_ADDR;
                    symbol_var.declared_line = line_counter;
                    trie_insert(obj->symbol_table_lookup,symbol_var.sym_name, vector_insert(obj->symbol_table, &symbol_var));
                }
            }else {/*if line is directive*/
                if(ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt <= mmn14_ast_dir_entry) {
                    assembler_warn_msg(am_file_name,line_counter, "neglecting label: '%s' since it's entry or extern",ast.label_name);
                }else {
                    if(find_symbol) {
                        if(find_symbol->sym_type != sym_entry) {
                            assembler_error_msg(am_file_name,line_counter,"label: '%s' was already defined as '%s in line : %d",find_symbol->sym_name,symbol_type_strings[find_symbol->sym_type],find_symbol->declared_line);
                            err_code = 0;
                        }else {
                            find_symbol->sym_type = sym_entry_data;
                            find_symbol->address = (unsigned int)vector_get_item_count(obj->data_section);
                            find_symbol->declared_line = line_counter;
                        }
                    }else {
                        symbol_var.sym_type = sym_data;
                        symbol_var.address = (unsigned int)vector_get_item_count(obj->data_section);
                        symbol_var.declared_line = line_counter;
                        trie_insert(obj->symbol_table_lookup,symbol_var.sym_name, vector_insert(obj->symbol_table,&symbol_var));
                    }
                }
            }
        }
        switch (ast.mmn14_ast_opt) {
            case mmn14_ast_inst:
            machine_word = ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[1] <<2;
            machine_word |= ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[0] <<9;
            machine_word |= ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_opt << 5;
            vector_insert(obj->code_section,&machine_word);
            if(ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_opt >= mmn14_ast_inst_rts  )  {
                
            }else {
                if(ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[1] == mmn14_ast_operand_opt_register &&
                    ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[0] == mmn14_ast_operand_opt_register) {
                        machine_word = ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[1].register_number <<2;
                        machine_word |= ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[0].register_number <<7;
                        vector_insert(obj->code_section,&machine_word);
                }else {
                    for(i=0;i<2;i++) {/*handles each operand*/
                        switch(ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operand_opt[i]) {
                            case mmn14_ast_operand_opt_register:
                            machine_word = ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[i].register_number << (7 - (i * 5));
                            vector_insert(obj->code_section,&machine_word);
                            break;
                            case mmn14_ast_operand_opt_label:
                                find_symbol = trie_exists(obj->symbol_table_lookup,ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[i].label);
                                if(find_symbol && find_symbol->sym_type != sym_entry) {
                                    machine_word = find_symbol->address <<2;
                                    if(find_symbol->sym_type == sym_extern) {
                                        machine_word |=1;
                                        extern_call_addr = vector_get_item_count(obj->code_section) + BASE_ADDR;
                                        mmn14_assembler_add_extern(obj->extern_calls,find_symbol->sym_name,extern_call_addr);
                                    }
                                    else {
                                        machine_word |=2;
                                    }
                                }
                                just_inserted_m_word = vector_insert(obj->code_section,&machine_word);
                                if(!find_symbol || (find_symbol && find_symbol->sym_type == sym_entry)) {
                                    strcpy(m_symbol.symbol_name,ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[i].label);
                                    m_symbol.machine_word = just_inserted_m_word;
                                    m_symbol.call_line = line_counter;
                                    m_symbol.call_address = extern_call_addr = vector_get_item_count(obj->code_section) + BASE_ADDR +1;
                                    vector_insert(missing_symbol_table,&m_symbol);
                                }
                            break;
                            case mmn14_ast_operand_opt_const_number:
                                machine_word = ast.dir_or_inst.mmn14_ast_inst.mmn14_ast_inst_operands[i].const_number << 2;
                                vector_insert(obj->code_section,&machine_word);
                            break;
                            case mmn14_ast_operand_opt_none:
                            break;
                        }
                    }
                }
            }
            break;
            case mmn14_ast_dir:
                if(ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt <= mmn14_ast_dir_data 
                && ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt >= mmn14_ast_dir_string && ast.label_name[0] =='\0') {
                    assembler_warn_msg(am_file_name,line_counter, "neglecting '%s' directive since it has no label.",ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt == mmn14_ast_dir_data ? ".data" :".string");
                    break;
                }
                switch(ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt) {
                    case mmn14_ast_dir_string:
                        for(i=0,data_str = ast.dir_or_inst.mmn14_ast_dir.dir_operand.string;*data_str;data_str++) {
                            machine_word = *data_str;
                            vector_insert(obj->data_section,&machine_word);
                        }
                        machine_word = 0;
                        vector_insert(obj->data_section,&machine_word);
                    break;
                    case mmn14_ast_dir_data:
                        for(i=0;i<ast.dir_or_inst.mmn14_ast_dir.dir_operand.data.data_count;i++) {
                            vector_insert(obj->data_section,&ast.dir_or_inst.mmn14_ast_dir.dir_operand.data.data[i]);
                        }
                    break;
                    case mmn14_ast_dir_extern: case mmn14_ast_dir_entry:
                        find_symbol = trie_exists(obj->symbol_table_lookup,ast.dir_or_inst.mmn14_ast_dir.dir_operand.label_name);
                        if(find_symbol) {
                            if(ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt == mmn14_ast_dir_entry) {
                                if(find_symbol->sym_type == sym_entry || find_symbol->sym_type >= sym_entry_code) {
                                    assembler_warn_msg(am_file_name, line_counter,"label: '%s' was already defined as '%s' in line : %d",find_symbol->sym_name,symbol_type_strings[find_symbol->sym_type],find_symbol->declared_line);
                                }else if(find_symbol->sym_type == sym_extern) {
                                    assembler_error_msg(am_file_name, line_counter,"label: '%s' was already defined as '%s' in line : %d",find_symbol->sym_name,symbol_type_strings[find_symbol->sym_type],find_symbol->declared_line);
                                    err_code = 0;
                                }else {
                                    find_symbol->sym_type = find_symbol->sym_type == sym_code ? sym_entry_code : sym_entry_data;
                                }
                            }else { /* if current line is extern...*/
                                if(find_symbol->sym_type == sym_extern) {
                                    assembler_warn_msg(am_file_name, line_counter,"label: '%s' was already defined as '%s' in line : %d",find_symbol->sym_name,symbol_type_strings[find_symbol->sym_type],find_symbol->declared_line);
                                }else {
                                    assembler_error_msg(am_file_name, line_counter,"label: '%s' was already defined as '%s' in line : %d",find_symbol->sym_name,symbol_type_strings[find_symbol->sym_type],find_symbol->declared_line);
                                    err_code = 0;
                                }
                            }
                        }else {
                            strcpy(symbol_var.sym_name,ast.dir_or_inst.mmn14_ast_dir.dir_operand.label_name);
                            symbol_var.sym_type = ast.dir_or_inst.mmn14_ast_dir.mmn14_ast_dir_opt; /* entry or extern*/
                            symbol_var.address =0;
                            symbol_var.declared_line = line_counter;
                            trie_insert(obj->symbol_table_lookup, symbol_var.sym_name,vector_insert(obj->symbol_table, &symbol_var));
                        }
                }
        }
        line_counter++;
    }
    VECTOR_FOR_EACH(begin, end, obj->symbol_table) {/*looping the symbol table*/
        if(*begin) {
            if( ((struct symbol*)(*begin))->sym_type == sym_entry) {
                assembler_error_msg(am_file_name,line_counter, "label: '%s' was declared in line: %d as entry symbol but was never defined.",((struct symbol*)(*begin))->sym_name,((struct symbol*)(*begin))->declared_line);
                err_code = 0; 
            }else if(((struct symbol*)(*begin))->sym_type >= sym_entry_code) {
                obj->entries_count++;
            }
            if(((struct symbol*)(*begin))->sym_type == sym_data || ((struct symbol*)(*begin))->sym_type == sym_entry_data) {
                ((struct symbol*)(*begin))->address += vector_get_item_count(obj->code_section) + BASE_ADDR ; 
            }
        }
    }
    VECTOR_FOR_EACH(begin, end, missing_symbol_table) {/*looping the missing symbol table*/
        if(*begin) {
            find_symbol = trie_exists(obj->symbol_table_lookup,((struct missing_symbol *)(*begin))->symbol_name);
            if(find_symbol && find_symbol->sym_type != sym_entry) {
                *((struct missing_symbol *)(*begin))->machine_word = find_symbol->address << 2;
                if(find_symbol->sym_type == sym_extern) {
                    *((struct missing_symbol *)(*begin))->machine_word |= 1;
                    mmn14_assembler_add_extern(obj->extern_calls,find_symbol->sym_name, ((struct missing_symbol *)(*begin))->call_address);
                }else {
                    *((struct missing_symbol *)(*begin))->machine_word |= 2;
                }
            }else {
                assembler_error_msg(am_file_name,line_counter , "missing label: '%s' was refered in line: %d but was never defined in this file.",((struct missing_symbol *)(*begin))->symbol_name,((struct missing_symbol *)(*begin))->call_line);
                err_code = 0;
            }
        }
    }
    vector_destroy(&missing_symbol_table);
    return err_code;
}
/**
@brief- this function compiles source files into machine code
@param- 'file_count'- number of files to process
	'file_names'-names of the source files
@return- returns 0 to indicate that process completed
*/

int mmn14_assembler(int file_count,char ** file_names) {
    int i;
    const char * am_file_name;
    FILE * am_file;
    struct object_file current_object;
    const char * output_directory = NULL;
    for(i=0;i<file_count;i++) {
        if(file_names[i] == NULL)
            continue;
        if(strcmp(file_names[i],"-o") == 0 ) {/**/
            output_directory = file_names[i+1];
            file_names[i] = NULL;
            file_names[i+1] = NULL;
        }
    }
    for(i=0;i<file_count;i++) {
        if(file_names[i] == NULL)
            continue;
        am_file_name = mmn14_preprocess(file_names[i],output_directory);
        if(am_file_name) {
            am_file = fopen(am_file_name,"r");
            if(am_file) {
                current_object = mmn14_assembler_create_new_obj();
                if(assembler_compile(am_file,&current_object,am_file_name)) {
                    mmn14_output(file_names[i],&current_object,output_directory);/*create assembly output and write to file*/
                }
                fclose(am_file);
                mmn14_assembler_destroy_obj(&current_object);
            }
            free((void*)am_file_name);
        }
    }
    return 0;
}
