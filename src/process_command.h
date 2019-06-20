#pragma once

typedef struct redisObject obj;
typedef struct redisClient redisClient;

int processCommand(redisClient* client);

obj** processMultiBulkBuf(char* msg, int* len);

