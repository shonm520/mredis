#include <stdio.h>
#include <string.h>
#include "config.h"
#include "connection.h"
#include "servermanager.h"
#include "listener.h"
#include "timer.h"
#include "ring_buffer.h"
#include "redis.h"
#include "process_inputbuf.h"
#include "redis_define.h"
#include "object.h"
#include "logger.h"
#include "dict.h"

sharedObjectsStruct shared;

redisServer server;

unsigned int dictSdsHash(const void *key) 
{
	return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

uint64_t dictSdsCaseHash(const void *key) 
{
	return dictGenCaseHashFunction((unsigned char*)key, strlen((char*)key));
}

int dictSdsKeyCompare(void *privdata, const void *key1,
	const void *key2) { /* 比较两个键的值是否相等,值得一提的是,两个key应该是字符串对象 */

	int l1 = strlen(key1); /* 获得长度 */
	int l2 = strlen(key1);
	if (l1 != l2) return 0;
	return memcmp(key1, key2, l1) == 0;
}

/* A case insensitive version used for the command lookup table and other
 * places where case insensitive non binary-safe comparison is needed. */
int dictSdsKeyCaseCompare(void *privdata, const void *key1,	const void *key2)
 {
	return strcasecmp(key1, key2) == 0; // 不区分大小写 
}
 /* 
  * 设置析构函数 
  */
void dictSdsDestructor(void *privdata, void *val) 
{
	zfree(val);
}

dictType commandTableDictType = {
	dictSdsCaseHash,           /* hash function */
	NULL,                      /* key dup */
	NULL,                      /* val dup */
	dictSdsKeyCaseCompare,     /* key compare */
	dictSdsDestructor,         /* key destructor */
	NULL                       /* val destructor */
};


void setCommand(redisClient *c);
void getCommand(redisClient *c);


struct redisCommand redisCommandTable[] = {
	{ "get",getCommand,2,"r",0, NULL,1,1,1,0,0 }
};

static void onMessage(connection *conn)
{
    redisClient* client = (redisClient*)conn->data;
    int ret = processInputBuffer(client);
    if (ret == REDIS_OK)  {
        ret = processCommand(client);
    }
    if (ret == REDIS_OK)  {
        replyClient(client, shared.ok->ptr, shared.ok->len);
    }
    else  {
        replyClient(client, shared.err->ptr, shared.err->len);
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

void populateCommandTable() 
{
    int numcommands = sizeof(redisCommandTable) / sizeof(struct redisCommand);
    int i = 0;
    for (i = 0; i < numcommands; i++)  {
        redisCommand *c = redisCommandTable + i;
        dictAdd(server.commands, c->name, c);
    }
}

void initServer()
{
    createSharedObjects();
}

void initServerConfig()
{
    server.commands = dictCreate(&commandTableDictType, NULL);
    populateCommandTable();
}

void createSharedObjects()
{
    shared.ok = createObject(OBJ_STRING, "+OK\r\n");
    shared.err = createObject(OBJ_STRING, "-ERR\r\n");
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

redisCommand* lookupCommand(char* name)
{
    return dictFetchValue(server.commands, name);
}

int processCommand(redisClient* client)
{
    redisCommand* cmd = lookupCommand(client->argv[0]->ptr);
    if (cmd)  {
        return REDIS_OK;
    }
    else {
        return REDIS_ERR;
    }
    
}

void setCommand(redisClient *c)
{

}

void getCommand(redisClient *c)
{
    
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