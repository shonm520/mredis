#pragma once

typedef struct redisObject obj;
typedef struct redisClient redisClient;

int process_command(redisClient* client, char* msg, int len);

obj** process_multibulk_buffer(char* msg, int* len);

