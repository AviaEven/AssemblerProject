
#include "trie.h"
#include <stdlib.h>

#define TRIE_BASE_CHAR ' '
struct trie_node {
    void * context_end;
    struct trie_node * next[95];
};

struct trie {
    struct trie_node * next[95];
};

/**
@brief - This method  checks if a given string exists in the trie
@param node_i- pointer to the current node in the trie
@param string - pointer to the current character of the string being searched in the trie
@return pointer to a 'struct trie_node that represents the last character of the searched string if its not found it returns NULL
*/
static struct trie_node *internal_trie_exists(struct trie_node * node_i,const char * string) {
    while(node_i) {
        if(*string == '\0' && node_i->context_end != NULL) {
            return node_i;
        }
        node_i = node_i->next[(*string) - TRIE_BASE_CHAR];
        string++;
    }
    return NULL;
}


Trie trie() {
    return calloc(1,sizeof(struct trie));
}

const char *trie_insert(Trie trie,const char *string,void * context_end) {
    struct trie_node ** iterator = &trie->next[(*string) - TRIE_BASE_CHAR];
    while(1) {
        if(*iterator == NULL) {
            (*iterator) = calloc(1,sizeof(struct trie_node));
            if(*iterator == NULL)
                return NULL;
        }
        string++;
        if(*string !='\0')
            iterator = &(*iterator)->next[(*string) - TRIE_BASE_CHAR];
        else
            break;
    }
    (*iterator)->context_end = context_end;
    return string;
}
void trie_delete(Trie trie,const char *string) {
    struct trie_node * get_node;
    if(string == NULL)
        return;
    get_node = internal_trie_exists(trie->next[(*string) - TRIE_BASE_CHAR],string+1);
    if(get_node)
        get_node->context_end = NULL;
}


void * trie_exists(Trie trie,const char *string) {
    struct trie_node * get_node;
    if(string == NULL)
        return NULL;
    get_node = internal_trie_exists(trie->next[(*string) - TRIE_BASE_CHAR],string+1);
    return get_node == NULL ? NULL : get_node->context_end;
}
/**
@brief - This method destroys (free) memory taken by the trie
@param node_i - pointer to the current node in the trie that needs to be freed
@return void
*/
static void trie_destroy_sub(struct trie_node * node_i) {
    int i;
    for(i=0;i<95;i++) {
        if(node_i->next[i] != NULL) {
            trie_destroy_sub(node_i->next[i]);
            node_i->next[i] = NULL;
        }
    }
    free(node_i);
}

void trie_destroy(Trie * trie) {
    int i;
    if(*trie != NULL) {
        Trie t = *trie;
        for(i=0;i<95;i++) {
            if(t->next[i] != NULL) 
                trie_destroy_sub(t->next[i]);
        }
        free(*trie);
        (*trie) = NULL;
    }
}
