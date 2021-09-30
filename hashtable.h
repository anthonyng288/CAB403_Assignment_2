#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>



// Item for hash table
typedef struct item item_t;
struct item
{
    u_int8_t *value;
    item_t *next;
};

// A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab
{
    item_t **buckets;
    size_t size;
};

// Init hash table
bool htab_init(htab_t *h, size_t n)
{
    
    h->buckets = (item_t **)calloc(n, sizeof(item_t *));
    h->size = n;

    return h->buckets != NULL;
}

// The Bernstein hash function.
// Modified to only use 6 character strings (reliabillity)
size_t djb_hash(char *s)
{
    // ensure string formats are standardized
    int input[7];
    for (size_t i = 0; i < 6; i++)
    {
        input[i] = s[i];
    }
    size_t hash = 5381;
    int c;
    for (size_t i = 0; i < 6; i++)
    {
        // hash = hash * 33 + c
        hash = ((hash << 5) + hash) + input[i];
    }
    return hash;
}

// Calculate the offset for the bucket for key in hash table.
size_t htab_index(htab_t *h, char *key)
{
    return djb_hash(key) % h->size;
}

// Convert the key into an array of ints
// perams: key must be a string of 6 characters
u_int8_t* get_val(char* key){
    u_int8_t *temp = calloc(6, sizeof(u_int8_t));
    for (int i = 0; i < 6; i++)
    {
        temp[i] = (u_int8_t)key[i];
    }
    return temp;
}

// Add a value into hasfh table
bool htab_add(htab_t *h, char *key){
    // get pointer to head of bucket list
    item_t *head = h->buckets[(djb_hash(key) % h->size)];

    item_t *new_node = (item_t *)malloc(sizeof(item_t));
    if (new_node == NULL){
        return false;
    }

    new_node->value = get_val(key);
    new_node->next = h->buckets[htab_index(h, key)];
    h->buckets[htab_index(h, key)] = new_node;
}


// Read through a file of numberplates and insert into hash table
// conditions:
//     input: a text file consisting of lines of exactly 6 characters
void read_plates(const char *input, htab_t *h){
    FILE* text  = fopen(input, "r");
    char line[10];
    
    while (NULL != fgets(line, sizeof(line), text))
    {
        htab_add(h, line);
    }
}

// Recursive function to search a bucket in a has table for a value
// conditions:
//     input: must be more than 6 characters to prevent undefined behavior
//     input: only first 6 characters will be checked        
bool search_bucket(item_t *item, u_char *input){
    for (size_t i = 0; i < 6; i++)
    {
        if (input[i] != item->value[i])
        {
            if (item->next == NULL)
            {
                // end of bucket with no match
                return false;
            } else{
                // search next bucket
                return search_bucket(item->next, input);
            }
        }
    }
    // search successfull
    return true;
}

// Determine if an input is stored in the hash table
bool search_plate(htab_t *h, u_char *input){
    // determine bucket result would be stored in
    item_t *head = h->buckets[(djb_hash(input) % h->size)];
    if (head == NULL)
    {
        return false;
    } else{
        return search_bucket(head, input);
    }
}




// for debugging only
// print an item from a hash table
void item_print(item_t *it)
{
    printf("value= ");
    for (size_t i = 0; i < 6; i++)
    {
        printf("%c", it->value[i]);
    }
}

// for debugging only
// print the contents of a hash table
void htab_print(htab_t *h)
{
    printf("hash table with %d buckets\n", h->size);
    for (size_t i = 0; i < h->size; ++i)
    {//for debugging only
        printf("bucket %d: ", i);
        if (h->buckets[i] == NULL)
        {
            printf("empty\n");
        }
        else
        {
            for (item_t *j = h->buckets[i]; j != NULL; j = j->next)
            {
                item_print(j);
                if (j->next != NULL)
                {
                    printf(" -> ");
                }
            }
            printf("\n");
        }
    }
}

// for debugging only
// check of a plate is in the hash table
// pre: max input size is 9 chars
void htab_check(htab_t *h){
    char input[7];
    printf("Please enter a value to search\n");
    scanf("%s", &input);
    if (search_plate(h, input))
    {
        printf("%s was found in the hash table\n", input);
    } else{
        printf("%s was not found in the hash table\n", input);
    }

    printf("%s hashes to %d", input, (djb_hash(input) % h->size));
    
}