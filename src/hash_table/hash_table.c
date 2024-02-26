#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/select.h>

#include "hash_table.h"

static int hashFunction(int index) {
	return index % TABLE_SIZE;
}

HashTable* initHashTable() {
	HashTable* hashtable = (HashTable*)malloc(sizeof(HashTable));
	memset((void*)(hashtable->buckets), 0x00, sizeof(Node*) * TABLE_SIZE);
	return hashtable;
}

void insert(HashTable* hashtable, KEY key, VALUE value, int hash_seed) {
	// 1 get hash
	int index = hashFunction(hash_seed);
	// 2 create new_node
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->key = key;
	new_node->value = value;
	new_node->next = NULL;
	// 3 insert head
	if (hashtable->buckets[index] != NULL) {
		new_node->next = hashtable->buckets[index];
	}
	hashtable->buckets[index] = new_node;
}

int deleteNode(HashTable* hashtable, KEY key, size_t key_len, int hash_seed) {
	// 1 get hash
	int index = hashFunction(hash_seed);
	// 2 search in bucket
	Node* cur = hashtable->buckets[index];
	Node* prev = NULL;
	while (cur != NULL && memcmp((void*)(cur->key), (void*)key, key_len)) {
		prev = cur;
		cur = cur->next;
	}
	if (cur == NULL) {
		return -1;
	}
	// 3 found
	// 3a list process
	if (prev == NULL) {
		hashtable->buckets[index] = cur->next;
	} else {
		prev->next = cur->next;
	}
	// 3b free node
	free(cur->key);
	free(cur->value);
	free(cur);
	return 0;
}

VALUE search(HashTable* hashtable, KEY key, size_t key_len, int hash_seed) {
	// 1 get hash
	int index = hashFunction(hash_seed);
	// 2 search in bucket
	Node* cur = hashtable->buckets[index];
	while (cur != NULL) {
		if (memcmp((void*)(cur->key), (void*)key, key_len)) {
			cur = cur->next;
		}
		// found
		return cur->value;
	}
	// 3 not found
	return NULL;
}

int update(HashTable* hashtable, KEY key, VALUE value, size_t key_len, size_t value_len, int hash_seed) {
	// 1 serach
	VALUE target = search(hashtable, key, key_len, hash_seed);
	// 2 alter
	if (target == NULL) {
		// not found
		return -1;
	} else {
		// found, to alter
		memcpy((void*)target, (void*)value, value_len);
		return 0;
	}
}

void destroyHashTable(HashTable* hashtable) {
	for (int i = 0; i < TABLE_SIZE; i++) {
		Node* cur = hashtable->buckets[i];
		while (cur != NULL) {
			Node* temp = cur;
			cur = cur->next;
			free(temp->key);
			free(temp->value);
			free(temp);
		}
	}
	free(hashtable);
}
