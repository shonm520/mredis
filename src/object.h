#pragma once 

#define LRU_BITS 24

typedef struct  redisObject {
	unsigned type:4;       // 类型
	unsigned encoding:4;   // 编码
    unsigned lru:LRU_BITS; 
	int refcount;          // 引用计数
	void *ptr;             // 指向实际值的指针
    int len;
} robj;



robj *createStringObject(char *ptr, int len);
robj *createObject(int type, void *ptr);
void decrRefCount(robj *o);