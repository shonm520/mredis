
#include "redis.h"
#include "config.h"
#include "redis_define.h"
#include "object.h"

extern sharedObjectsStruct shared;
extern redisServer server;

extern robj *lookupKey(redisDb *db, robj *key, int flags);
extern robj *lookupKeyRead(redisDb *db, robj *key);



int getGenericCommand(redisClient *c) 
{
    robj *o;
    if ((o = lookupKeyRead(c->db, c->argv[1])) == NULL)  {
        replyObjClient(c, shared.null[c->resp]);
        server.stat_keyspace_misses++;
        return C_OK;
    }

    server.stat_keyspace_hits++;
    if (o->type != OBJ_STRING) {
        replyObjClient(c, shared.wrongtypeerr);
        return C_ERR;
    } else {
        //addReplyBulk(c, o);
        return C_OK;
    }
}

void getCommand(redisClient *c) 
{
    getGenericCommand(c);
}

void setCommand(redisClient* c)
{

}