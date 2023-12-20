
#include "preprocessor.h"
#include "vector.h"
#include "trie.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <libgen.h>

#define as_file_ext ".as"
#define am_file_ext ".am" 
#define MAX_MACRO_LEN 31
#define MAX_LINE_LEN 81
#define SPACE_CHARS " \t\n\f\r\v"
#define SKIP_SPACE(s) while(*s && isspace(*s)) s++
#define SKIP_SPACE_R(s,base) while(*s && isspace(*s) && base != s) s++
struct macro {
    char name_of_macro[MAX_MACRO_LEN+1];
    Vector lines;
};
enum preprocessor_line_detection {
    null_line,
    macro_definition,
    end_macro_definition,
    any_other_line,
    macro_already_exists,
    bad_macro_definition,
    bad_endmacro_definition,
    macro_call,
    bad_macro_call
};
/**
@brief - this function is used for creating copies of string 
@param - 'copy' - generic pointer for generic data type
@return - returns a pointer to where memory allocation happened, which is pointed by 'line'
*/
static void * create_line(const void * copy) {
    const char * line = copy;
    return strcpy(malloc(strlen(line) + 1),line);
}
/**
@brief - this function is used to delete\free memory for 'line' items
*/
static void line_dtor(void * item) {
    free(item);
}
/**
@brief - this function creates new macro instances
@param - 'copy' -generic pointer used to reference a macro
@return - returns pointer for the line items
*/
static void * create_macro(const void * copy) {
    const struct  macro * m1 = copy;/*m1 is 'macro1'  */
    struct macro * new_macro = malloc(sizeof(struct macro));
    strcpy(new_macro->name_of_macro,m1->name_of_macro);
    new_macro->lines = new_vector(create_line, line_dtor);
    return new_macro;
}
/**
@brief - this function is used for deleting\freeing memory 
@param - item- pointer to generic type 
*/
static void macro_dtor(void * item) {
    struct  macro * macro = item;
    vector_destroy(&macro->lines);
    free((void*)macro);
}

/**
@brief - this function analyzes a line and determines its state
@param -'line' - char pointer, with this parameter we analyze the input 
	'macro' - double pointer to struct, used for an output parameter
	'macro_lookup' - of type 'Trie', this paramter is to get info about macros
	'macro_table' - of type Vector, for the macros inside vector
@return basically returns the state of line
*/

enum preprocessor_line_detection preprocessor_line_check(char * line,struct macro **macro,const Trie macro_lookup ,Vector macro_table) {
    struct macro new_macro = {0};
    struct macro * local;
    char *temp2, * temp1;
    temp2 = strchr(line,';');
    if(temp2) *temp2 = '\0';
    SKIP_SPACE(line);
    if(*line == '\0')
        return null_line;
    
    temp2 = strstr(line,"endmcro");
    if(temp2) {
        temp1 = temp2;
        SKIP_SPACE_R(temp1,line);
        if(temp1 != line) {/*if the value temp1 has move away from the beginning of the line which mean there non space chars before "endmcro" directive*/
            return bad_endmacro_definition; 
        }
        temp2+=7;/*to skip "endmcro" word itself*/
        SKIP_SPACE(temp2);
        if(*temp2 != '\0')/*if we reached the end*/
            return bad_endmacro_definition;
        *macro = NULL;
        return end_macro_definition;
    }
    temp2 = strstr(line,"mcro");
    if(temp2) {/*if "mcro" directive is found*/
        temp1 = temp2;
        SKIP_SPACE_R(temp1,line);
        if(temp1 != line) {
            return bad_macro_definition; 
        }
        temp2+=4;/*skip "mcro" keyword*/
        SKIP_SPACE(temp2);
        line = temp2;/*update line form temp2*/
        temp2 = strpbrk(line, SPACE_CHARS);
        if(temp2) {
            *temp2 = '\0';
            temp2++;
            SKIP_SPACE(temp2);
            if(*temp2!='\0') {
                return bad_macro_definition;
            }
        }
        *macro = trie_exists(macro_lookup, line);/*checks if macro already exists*/
        if(*macro) {
            return macro_already_exists;
        }
        strcpy(new_macro.name_of_macro,line);
        *macro = vector_insert(macro_table, &new_macro);
        trie_insert(macro_lookup, line, (*macro));
        return macro_definition;
    }
    temp2 = strpbrk(line, SPACE_CHARS);
    if(temp2) *temp2 ='\0';
    local = trie_exists(macro_lookup, line);
    if(local == NULL) {
        *temp2 = ' ';
        return any_other_line;
    }
/*process a macro call if encounterd*/
    temp2++;
    SKIP_SPACE(temp2);
    if(*temp2 !='\0')
        return bad_macro_call;
    *macro = local;
    return macro_call;
}
/**
@brief - this function processes assembly file
@param -'file_name'- char pointer, which represents the name of the input file
	'output_directory' - char pointer which represents the output directory
*/
const char * mmn14_preprocess(char * file_name,const char * output_directory) {
    char line_buff[MAX_LINE_LEN] = {0};
    enum preprocessor_line_detection line_detection_var;/*holds the result from the enum*/
    size_t file_name_len, am_file_name_base_len;/*used to store the lengths of strings which create output file names*/
    char * as_file_name;
    char * am_file_name;
    FILE * am_file;
    FILE * as_file;
    Vector macro_table = NULL;
    Trie   macro_table_lookup = NULL;
    struct macro * macro = NULL;
    void * const * begin;
    void * const * end;
    int line_count = 1;
    file_name_len = strlen(file_name);
/*memory allocation*/
    as_file_name = strcat(strcpy(malloc(file_name_len + strlen(as_file_ext) + 1 ),file_name),as_file_ext);
    file_name = output_directory != NULL ?  basename(file_name) : file_name;
    file_name = strcat(strcpy(malloc(strlen(output_directory) + strlen(file_name) + 1),output_directory),file_name);
    am_file_name_base_len = strlen(file_name);
    am_file_name = strcat(strcpy(malloc(am_file_name_base_len + strlen(am_file_ext) + 1 ),file_name),am_file_ext);

    as_file = fopen(as_file_name,"r");
    if(as_file == NULL) {
            if(output_directory)
                free(file_name);
        free(as_file_name);
        free(am_file_name);
        return NULL;
    }
    am_file = fopen(am_file_name,"w");
    if(am_file == NULL) {
            if(output_directory)
                free(file_name);
        free(as_file_name);
        free(am_file_name);
        return NULL;
    }
    macro_table = new_vector(create_macro, macro_dtor);
    macro_table_lookup = trie();
    while(fgets(line_buff,sizeof(line_buff),as_file)) {
        switch(line_detection_var = preprocessor_line_check(line_buff,&macro,macro_table_lookup,macro_table)) {
            case null_line:
            break;
            case macro_definition:
            break;
            case end_macro_definition:
            break;
            case macro_call:
                VECTOR_FOR_EACH(begin, end, macro->lines) {
                    if(*begin) {
                        fputs((const char *)(*begin),am_file);
                    }
                }
                macro = NULL;
            break;
            case any_other_line:
                if(macro) {
                    vector_insert(macro->lines, &line_buff[0]);
                }else {
                    fputs(line_buff,am_file);
                }
            break;
            case macro_already_exists:
                /*ERROR ...*/
            break;
            case bad_endmacro_definition:
                /* ERROR ...*/
            break;
            case bad_macro_call:
                /* ERROR ....*/
            break;
            case bad_macro_definition:
                /* ERROR*/
            break;
        }
        line_count++;
    }
    vector_destroy(&macro_table);
    trie_destroy(&macro_table_lookup);
    free(as_file_name);
    fclose(as_file);
    fclose(am_file);
    if(output_directory)
        free(file_name);
    return am_file_name;
}
