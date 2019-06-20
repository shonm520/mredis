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

sharedObjectsStruct shared;

static void onMessage(connection *conn)
{
    int size = 0;
    char* msg = ring_buffer_get_msg(conn->ring_buffer_read, &size);
    //printf("read all : %s, %d\n", msg, size);

    redisClient* client = (redisClient*)conn->data;
    int ret = process_command(client, msg, size);
    if (ret == REDIS_OK)  {
        ring_buffer_push_data(conn->ring_buffer_write, shared.ok->ptr, shared.ok->len);
        connection_send_buffer(conn);
    }
}

static void ondisconnect(connection* conn)
{
    printf("svr disconnected\n\n");
}

static void onConnection(connection* conn)
{
    redisClient* client = zmalloc(sizeof(redisClient));
    memset(client, 0, sizeof(redisClient));
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