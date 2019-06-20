#pragma once 


typedef struct redisObject robj;
typedef struct connection_t connection;

typedef struct redisClient {
    int argc;
    robj **argv;                   // 参数对象数组
    connection* connect_data;
} redisClient;

typedef struct sharedObjectsStruct {
    robj* ok;
    robj* err;
} sharedObjectsStruct;

typedef struct redisCommand redisCommand;
typedef void redisCommandProc(redisClient *c);
typedef int *redisGetKeysProc(redisCommand *cmd, robj **argv, int argc, int *numkeys);

typedef struct redisCommand {
	char *name;
	redisCommandProc *proc;
	int arity;
    char *sflags; 
    int flags; // 实际flag 
    redisGetKeysProc *getkeys_proc;
	int firstkey; /* The first argument that's a key (0 = no keys) */
	int lastkey;  /* The last argument that's a key */
	int keystep;  /* The step between first and last key */
	long long microseconds, calls;
} redisCommand;

typedef struct dict dict;

typedef struct redisServer {
    dict* commands;
} redisServer;

void createSharedObjects();

void initServer();

void initServerConfig();

char* readQueryFromClient(redisClient* client, int* size);

int replyClient(redisClient* client, char* msg, int len);

void freeClient(redisClient* client);

int processCommand(redisClient* client);