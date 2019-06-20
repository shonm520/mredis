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
} sharedObjectsStruct;

void createSharedObjects();


char* readQueryFromClient(redisClient* client, int* size);

int replyClient(redisClient* client, char* msg, int len);

void freeClient(redisClient* client);