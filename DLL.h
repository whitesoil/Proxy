/*
 *
 * Last modified : 2017.12.02
 * Hanyang University
 * Computer Science & Engineering
 * Seon Namkung
 *
 * LRU Cache
 *
 */

#ifndef __DLL__
#define __DLL__

#define MAX_CACHE_SIZE 5000000
#define MAX_OBJECT_SIZE 512000

typedef struct node {
        struct node * prev;
        struct node * next;
        char name[100];
        char * contents;
        int size;
}Node;

typedef struct cache {
        Node * header;
        Node * trailer;
        int total_size;
}Cache;

Cache*cache;

void init_cache();
void insert_at_first(Node *);
Node * delete_at_last();
void move_to_first(Node *);
Node * find_Node(char *);
void print_cache();

/*
 * Initiate Cache
 * Must call function when start caching.
 */
void init_cache(){
        cache = (Cache *)malloc(sizeof(Cache));
        Node * header = (Node *)malloc(sizeof(Node));
        Node * trailer = (Node *)malloc(sizeof(Node));

        header->prev = NULL;
        header->next = trailer;
        trailer->prev = header;
        trailer->next = NULL;

        strcpy(header->name,"/Header\0");
        header->contents = NULL;
        header->size = -1;
        strcpy(trailer->name,"/Trailer\0");
        trailer->contents = NULL;
        trailer->size = -1;

        cache->header = header;
        cache->trailer = trailer;
        cache->total_size = 0;

        return;
}

/*
 * Insert node at first that rigth after header.
 */
void insert_at_first(Node * node){
        Node * temp = cache->header;

        node->next = temp->next;
        temp->next->prev = node;
        temp->next = node;
        node->prev = temp;

        cache->total_size += node->size;
}

/*
 * Delete oldest node which rigth before trailer.
 * Return deleted node.
 */
Node * delete_at_last(){
        Node * temp = cache->header;
        while(temp->next->size != -1) {
                temp = temp->next;
        }
        if(temp->size == -1) {
                return NULL;
        }

        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;

        cache->total_size -= temp->size;

        return temp;
}

/*
 * Cache hit, move the hitted node at first.
 */
void move_to_first(Node * node){
        node->prev->next = node->next;
        node->next->prev = node->prev;

        insert_at_first(node);

        return;
}

/*
 * Find object in cache by name.
 * If find, return the node same with node_name;
 * If not, return Null
 */
Node * find_Node(char * node_name){
        Node * temp = cache->header->next;

        while(temp->size != -1) {
                if(strcmp(temp->name,node_name) == 0) {
                        return temp;
                }
                temp = temp->next;
        }
        return NULL;
}

/*
 * Print cached nodes in cache.
 */
void print_cache(){
        Node * temp = cache->header->next;
        while(temp->size != -1) {
                printf("%s : %d -> ",temp->name,temp->size);
                temp = temp->next;
        }
        fputs("Trailer\n",stdout);
        return;
}

#endif
