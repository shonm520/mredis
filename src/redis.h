#pragma once 

#include <pthread.h>
#include "dict.h"


typedef struct redisObject robj;
typedef struct connection_t connection;

#define _Atomic

typedef struct redisDb {
    dict *dict;                 /* The keyspace for this DB */
    dict *expires;              /* Timeout of keys with a timeout set */
    dict *blocking_keys;        /* Keys with clients waiting for data (BLPOP)*/
    dict *ready_keys;           /* Blocked keys that received a PUSH */
    dict *watched_keys;         /* WATCHED keys for MULTI/EXEC CAS */
    int id;                     /* Database ID */
    long long avg_ttl;          /* Average TTL, just for stats */
    //list *defrag_later;         /* List of key names to attempt to defrag one by one, gradually. */
} redisDb;

typedef struct redisClient {
    int argc;
    robj **argv;                   // 参数对象数组
    connection* connect_data;

    int resp;
    int flags;  

    redisDb *db;            /* Pointer to currently SELECTed DB. */
} redisClient;

typedef struct sharedObjectsStruct {
    robj* ok;
    robj* err;
    robj* null[4];
    robj* wrongtypeerr;
} sharedObjectsStruct;

typedef struct redisCommand redisCommand;
typedef void redisCommandProc(redisClient *c);
typedef int *redisGetKeysProc(redisCommand *cmd, robj **argv, int argc, int *numkeys);

typedef struct redisCommand {
	char *name;
	redisCommandProc *proc;
	int arity;
    char *sflags; 
    int flags;    // 实际flag 
    redisGetKeysProc *getkeys_proc;
	int firstkey; /* The first argument that's a key (0 = no keys) */
	int lastkey;  /* The last argument that's a key */
	int keystep;  /* The step between first and last key */
	long long microseconds, calls;
} redisCommand;

typedef struct dict dict;

typedef struct redisServer {
    dict* commands;

    long long stat_keyspace_hits;   /* Number of successful lookups of keys */
    long long stat_keyspace_misses; /* Number of failed lookups of keys */

    pid_t rdb_child_pid;
    pid_t aof_child_pid;

    int maxmemory_policy;

    int lfu_log_factor;             /* LFU logarithmic counter factor. */
    int lfu_decay_time;             /* LFU counter decay factor. */

    _Atomic time_t unixtime;    /* Unix time sampled every cron cycle. */
    time_t timezone;            /* Cached timezone. As set by tzset(). */
    int daylight_active;        /* Currently in daylight saving time. */
    long long mstime;           /* 'unixtime' with milliseconds resolution. */

    int config_hz;
    int hz;                     /* serverCron() calls frequency in hertz */
    _Atomic unsigned int lruclock; /* Clock for LRU eviction */

    redisDb *db;
    int dbnum;                      /* Total number of configured DBs */

} redisServer;


void createSharedObjects();

void initServer();

void initServerConfig();

char* readQueryFromClient(redisClient* client, int* size);

int replyClient(redisClient* client, char* msg, int len);

int replyObjClient(redisClient* client, robj* obj);

redisClient* createClient(int fd);

void freeClient(redisClient* client);

int processCommand(redisClient* client);

int selectDb(redisClient *c, int id);
void updateCachedTime();

void getCommand(redisClient *c);
void setCommand(redisClient *c);