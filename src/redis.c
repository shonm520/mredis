#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "config.h"
#include "connection.h"
#include "servermanager.h"
#include "ring_buffer.h"
#include "redis.h"
#include "process_inputbuf.h"
#include "redis_define.h"
#include "object.h"
#include "logger.h"
#include "dict.h"
#include "util.h"

sharedObjectsStruct shared;

redisServer server;

#define sdsnew
#define sds (char*)
#define sdslen strlen

uint64_t dictSdsHash(const void *key) 
{
	return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

uint64_t dictSdsCaseHash(const void *key) 
{
	return dictGenCaseHashFunction((unsigned char*)key, strlen((char*)key));
}

uint64_t dictObjHash(const void *key) 
{
    const robj *o = key;
    return dictGenHashFunction(o->ptr, strlen((char*)o->ptr));
}

int dictSdsKeyCompare(void *privdata, const void *key1,	const void *key2) 
{ 
	int l1 = strlen(key1);
	int l2 = strlen(key1);
	if (l1 != l2) return 0;
	return memcmp(key1, key2, l1) == 0;
}

/* A case insensitive version used for the command lookup table and other
 * places where case insensitive non binary-safe comparison is needed. */
int dictSdsKeyCaseCompare(void *privdata, const void *key1,	const void *key2)
 {
	return strcasecmp(key1, key2) == 0; 
}

int dictObjKeyCompare(void *privdata, const void *key1,const void *key2)
{
    const robj *o1 = key1, *o2 = key2;
    return dictSdsKeyCompare(privdata,o1->ptr,o2->ptr);
}

void dictObjectDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    if (val == NULL) return; /* Lazy freeing will set value to NULL. */
    decrRefCount(val);
}

void dictSdsDestructor(void *privdata, void *val) 
{
	zfree(val);
}

dictType commandTableDictType = {
	dictSdsCaseHash,           /* hash function */
	NULL,                      /* key dup */
	NULL,                      /* val dup */
	dictSdsKeyCaseCompare,     /* key compare */
	dictSdsDestructor,         /* key destructor */
	NULL                       /* val destructor */
};

dictType dbDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,          /* key destructor */
    dictObjectDestructor   /* val destructor */
};

dictType keyptrDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    NULL,                       /* key destructor */
    NULL                        /* val destructor */
};

struct redisCommand redisCommandTable[] = {
	{ "get",getCommand,2,"r",0, NULL,1,1,1,0,0 },
    { "set",setCommand,-3,"wm",0,NULL,1,1,1,0,0 }
};

void populateCommandTable() 
{
    int numcommands = sizeof(redisCommandTable) / sizeof(struct redisCommand);
    int i = 0;
    for (i = 0; i < numcommands; i++)  {
        redisCommand *c = redisCommandTable + i;
        dictAdd(server.commands, c->name, c);
    }
}

void initServer()
{
    createSharedObjects();

    server.rdb_child_pid = -1;
    server.aof_child_pid = -1;

    server.maxmemory_policy = CONFIG_DEFAULT_MAXMEMORY_POLICY;

    server.lfu_log_factor = CONFIG_DEFAULT_LFU_LOG_FACTOR;
    server.lfu_decay_time = CONFIG_DEFAULT_LFU_DECAY_TIME;


    server.hz = server.config_hz;

    server.db = zmalloc(sizeof(redisDb) * server.dbnum);


    /* Create the Redis databases, and initialize other internal state. */
    int j = 0;
    for (j = 0; j < server.dbnum; j++) {
        server.db[j].dict = dictCreate(&dbDictType,NULL);
        server.db[j].expires = dictCreate(&keyptrDictType,NULL);
        // server.db[j].blocking_keys = dictCreate(&keylistDictType,NULL);
        // server.db[j].ready_keys = dictCreate(&objectKeyPointerValueDictType,NULL);
        // server.db[j].watched_keys = dictCreate(&keylistDictType,NULL);
        server.db[j].id = j;
        server.db[j].avg_ttl = 0;
        //server.db[j].defrag_later = listCreate();
    }
}

void initServerConfig()
{
    server.commands = dictCreate(&commandTableDictType, NULL);
    populateCommandTable();

    updateCachedTime();

    server.hz = server.config_hz = CONFIG_DEFAULT_HZ;
    server.dbnum = CONFIG_DEFAULT_DBNUM;
}

void createSharedObjects()
{
    shared.ok = createObject(OBJ_STRING, "+OK\r\n");
    shared.err = createObject(OBJ_STRING, "-ERR\r\n");

    shared.wrongtypeerr = createObject(OBJ_STRING, sdsnew(
        "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n"));

    /* The shared NULL depends on the protocol version. */
    shared.null[0] = NULL;
    shared.null[1] = NULL;
    shared.null[2] = createObject(OBJ_STRING, sdsnew("$-1\r\n"));
    shared.null[3] = createObject(OBJ_STRING, sdsnew("_\r\n"));
}

redisClient* createClient(int fd)
{
    redisClient* client = zmalloc(sizeof(redisClient));
    memset(client, 0, sizeof(redisClient));
    client->resp = 2;
    client->flags = 0;

    selectDb(client, 0);
    return client;
}

char* readQueryFromClient(redisClient* client, int* size)
{
    connection* conn = client->connect_data;
    if (!conn)  {
        debug_ret("conn is null when readQueryFromClient, %s, %d", __FILE__, __LINE__);
        return NULL;
    }
    char* msg = ring_buffer_get_msg(conn->ring_buffer_read, size);
    return msg;
}

int replyClient(redisClient* client, char* msg, int len)
{
    connection* conn = client->connect_data;
     if (!conn)  {
        debug_ret("conn is null when replyClient, %s, %d", __FILE__, __LINE__);
        return -1;
    }
    ring_buffer_push_data(conn->ring_buffer_write, msg, len);
    return connection_send_buffer(conn);
}

int replyObjClient(redisClient* client, robj* obj)
{
    if (obj->type == OBJ_STRING)  {
        return replyClient(client, obj->ptr, obj->len);
    }
    return -1;
}

void freeClient(redisClient* client)
{
    zfree(client->argv);
    zfree(client);
}

redisCommand* lookupCommand(char* name)
{
    return dictFetchValue(server.commands, name);
}

int processCommand(redisClient* client)
{
    char* name = (char*)client->argv[0]->ptr;
    if (!strcasecmp(name, "quit")) {
        replyObjClient(client, shared.ok);
        client->flags |= CLIENT_CLOSE_AFTER_REPLY;
        return REDIS_ERR;
    }

    redisCommand* cmd = lookupCommand(name);
    if (cmd)  {
        if ((cmd->arity > 0 && cmd->arity != client->argc) ||
            client->argc < -cmd->arity)  {
            char* szErr = make_message("-ERR wrong number of arguments for '%s' command\r\n", name);
            replyClient(client, szErr, strlen(szErr));
            zfree(szErr);
        }
        else {
            cmd->proc(client);
        }
        return REDIS_OK;
    }
    else {
        return REDIS_ERR;
    }
}

static long long mstime() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long mst = ((long long)tv.tv_sec) * 1000;
    mst += tv.tv_usec / 1000;
    return mst;
}

void updateCachedTime() 
{
    server.unixtime = time(NULL);
    server.mstime = mstime();

    /* To get information about daylight saving time, we need to call
     * localtime_r and cache the result. However calling localtime_r in this
     * context is safe since we will never fork() while here, in the main
     * thread. The logging function will call a thread safe version of
     * localtime that has no locks. */
    struct tm tm;
    time_t ut = server.unixtime;
    localtime_r(&ut,&tm);
    server.daylight_active = tm.tm_isdst;
}
