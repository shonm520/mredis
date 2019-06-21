#include <stdio.h>
#include <string.h>
#include "config.h"
#include "connection.h"
#include "servermanager.h"
#include "listener.h"
#include "timer.h"
#include "redis.h"
#include "redis_define.h"
#include "process_inputbuf.h"

extern sharedObjectsStruct shared;


static void onMessage(connection *conn)
{
    redisClient* client = (redisClient*)conn->data;
    int ret = processInputBuffer(client);
    if (ret == REDIS_OK)  {
        ret = processCommand(client);
    }
    if (ret == REDIS_OK)  {
        replyObjClient(client, shared.ok);
    }
    else  {
        replyObjClient(client, shared.err);
    }
}

static void ondisconnect(connection* conn)
{
    printf("svr disconnected\n\n");
    freeClient(conn->data);
}

static void onConnection(connection* conn)
{
    redisClient* client = createClient(conn->connfd);
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

    initServerConfig();
    initServer();

	server_manager *manager = server_manager_create(port, thread_num);
	inet_address addr = addr_create("any", port);
	listener_create(manager, addr, onMessage, onConnection);

	server_manager_run(manager);

	return 0;
}