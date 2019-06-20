#pragma once

typedef struct redisObject obj;
typedef struct redisClient redisClient;

int processInputBuffer(redisClient* client);

obj** processMultiBulkBuf(char* msg, int* len);

