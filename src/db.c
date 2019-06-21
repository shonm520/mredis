
#include "redis.h"
#include "object.h"
#include "config.h"
#include "dict.h"
#include "redisconfig.h"
#include "redis_define.h"


extern redisServer server;

extern unsigned long LFUGetTimeInMinutes(void);
extern uint8_t LFULogIncr(uint8_t value);
extern unsigned long LFUDecrAndReturn(robj *o);
extern unsigned int LRU_CLOCK();


void updateLFU(robj *val) 
{
    unsigned long counter = LFUDecrAndReturn(val);
    counter = LFULogIncr(counter);
    val->lru = (LFUGetTimeInMinutes()<<8) | counter;
}

/* Low level key lookup API, not actually called directly from commands
 * implementations that should instead rely on lookupKeyRead(),
 * lookupKeyWrite() and lookupKeyReadWithFlags(). */
robj *lookupKey(redisDb *db, robj *key, int flags) 
{
    dictEntry *de = dictFind(db->dict, key->ptr);
    if (de) {
        robj *val = dictGetVal(de);

        /* Update the access time for the ageing algorithm.
         * Don't do it if we have a saving child, as this will trigger
         * a copy on write madness. */
        if (server.rdb_child_pid == -1 &&
            server.aof_child_pid == -1 &&
            !(flags & LOOKUP_NOTOUCH))
        {
            if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
                updateLFU(val);
            } else {
                val->lru = LRU_CLOCK();
            }
        }
        return val;
    } else {
        return NULL;
    }
}

robj *lookupKeyReadWithFlags(redisDb *db, robj *key, int flags)
{   
    robj* val = lookupKey(db, key, flags);
    if (val == NULL) {
        server.stat_keyspace_misses++;
        //notifyKeyspaceEvent(NOTIFY_KEY_MISS, "keymiss", key, db->id);
        return NULL;
    }
    else
        server.stat_keyspace_hits++;
    return val;
}

robj *lookupKeyRead(redisDb *db, robj *key) 
{
    return lookupKeyReadWithFlags(db, key, LOOKUP_NONE);
}

int selectDb(redisClient *c, int id) 
{
    if (id < 0 || id >= server.dbnum)
        return C_ERR;
    c->db = &server.db[id];
    return C_OK;
}




