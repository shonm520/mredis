#pragma once 


typedef struct redisObject robj;

typedef struct redisClient {
    int argc;
    robj **argv; // 参数对象数组
} redisClient;

typedef struct sharedObjectsStruct {
    robj* ok;
} sharedObjectsStruct;

void createSharedObjects();