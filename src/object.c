#include <string.h>
#include "object.h"
#include "redis_define.h"
#include "config.h"
#include "redis.h"

extern redisServer server;

extern unsigned long LFUGetTimeInMinutes();
extern unsigned int LRU_CLOCK();

robj *createEmbeddedStringObject(char *ptr, int len) 
{
	robj *o = zmalloc(sizeof(robj));

	o->type = OBJ_STRING;
	o->encoding = OBJ_ENCODING_EMBSTR;
	o->refcount = 1;

    o->ptr = zmalloc(len + 1);
	o->len = len;

	if (ptr) {
		memcpy(o->ptr, ptr, len);
		((char*)(o->ptr))[len] = '\0';
	}
	else {
		o->ptr = NULL;
	}
	return o;
}

robj *createStringObject(char *ptr, int len) 
{
    return createEmbeddedStringObject(ptr, len);
}

/*
 * 创建一个新 robj 对象
 */
robj *createObject(int type, void *ptr) 
{
	robj *o = zmalloc(sizeof(*o));

	o->type = type;
	o->encoding = OBJ_ENCODING_RAW;
	o->ptr = ptr;
	o->refcount = 1;
    o->len = strlen((char*)ptr);

	/* Set the LRU to the current lruclock (minutes resolution), or
     * alternatively the LFU counter. */
    if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
        o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
    } else {
        o->lru = LRU_CLOCK();
    }
	return o;
}

void decrRefCount(robj *o) 
{
    if (o->refcount == 1) {
        zfree(o);
    } else {
       
    }
}