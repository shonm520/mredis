#include <stdio.h>
#include <string.h>
#include "config.h"
#include "connection.h"
#include "servermanager.h"
#include "listener.h"
#include "timer.h"
#include "ring_buffer.h"
#include "redis.h"
#include "process_command.h"
#include "redis_define.h"
#include "object.h"
#include "logger.h"

sharedObjectsStruct shared;

static void onMessage(connection *conn)
{
    redisClient* client = (redisClient*)conn->data;
    int ret = processCommand(client);
    if (ret == REDIS_OK)  {
        replyClient(client, shared.ok->ptr, shared.ok->len);
    }
}

static void ondisconnect(connection* conn)
{
    printf("svr disconnected\n\n");
    freeClient(conn->data);
}

static void onConnection(connection* conn)
{
    redisClient* client = zmalloc(sizeof(redisClient));
    memset(client, 0, sizeof(redisClient));
    client->connect_data = conn;
    conn->data = client;

    conn->disconnected_cb = ondisconnect;
}

int main(int argc, char* argv[])  
{
    int port = DEFAULT_PORT;
    int thread_num = MAX_LOOP;
    if (argc >= 2)
        port = atoi(argv[1]);
    if (argc >= 3)
        thread_num = atoi(argv[2]);

    printf("port, thread_num is %d, %d \n", port, thread_num);

    createSharedObjects();

	server_manager *manager = server_manager_create(port, thread_num);
	inet_address addr = addr_create("any", port);
	listener_create(manager, addr, onMessage, onConnection);

	server_manager_run(manager);

	return 0;
}

void createSharedObjects()
{
    shared.ok = createObject(OBJ_STRING, "+OK\r\n");
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

void freeClient(redisClient* client)
{
    zfree(client->argv);
    zfree(client);
}