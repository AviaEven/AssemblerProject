
#ifndef __TRIE_H_
#define __TRIE_H_

typedef struct trie * Trie;


Trie trie();
/**
@brief - This method is used to insert a new string into the trie
@param trie - pointer to the root node of the trie
@param string- pointer to the string that needs to be inserted into the trie
@param context_end- a pointer that gives a meaning to the string context
@return pointer to the inserted string
*/

const char * trie_insert(Trie trie,const char *string,void * context_end);

/**
@brief - This method checks if a given string exists in the trie
@param trie - pointer to the root node of the trie
@param string - pointer to the string that needs to checked in the trie
@return void pointer 
*/
void * trie_exists(Trie trie,const char *string);

/**
@brief - This method deletes a string from the trie
@param trie - pointer to the root node of the trie
@param string -pointer to the string that needs to be deleted from the trie
@return void
*/
void trie_delete(Trie trie,const char  *string);

/**
@brief -The provided function trie_destroy is used to destroy (free) the memory occupied by a trie data structure. 
It is a wrapper function that calls the recursive trie_destroy_sub function to handle the destruction process for each node in the trie
@param trie
@return void
*/
void trie_destroy(Trie * trie);



#endif
