#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif



	// hash info
	// key
	typedef struct sockaddr_in* KEY;
	// value
	typedef struct Epoll_Ptr Epoll_Ptr_t;
	typedef Epoll_Ptr_t* VALUE;
	#define TABLE_SIZE FD_SETSIZE



	// struct
	// hash node
	typedef struct Node_t {
		KEY key;
		VALUE value;
		struct Node_t* next;
	}Node;
	// hashtable
	typedef struct HashTable {
		Node* buckets[TABLE_SIZE];
		int count;
	}HashTable;
	// value struct
	struct Epoll_Ptr {
		int epoll_fd;
		int fd;
		int dst_fd;
		char* recv_buf;
		char* reverse_recv_buf;
		int reverse_recv_buf_size;
		char* send_buf;
		struct sockaddr_in* cli_addr;
		HashTable* hashtable;
		time_t timestamp;
	};



	// function
	HashTable* initHashTable();
	void insert(HashTable* hashtable, KEY key, VALUE value, int hash_seed);
	int deleteNode(HashTable* hashtable, KEY key, size_t key_len, int hash_seed);
	VALUE search(HashTable* hashtable, KEY key, size_t key_len, int hash_seed);
	void destroyHashTable(HashTable* hashtable);



#ifdef __cplusplus
}
#endif

#endif
