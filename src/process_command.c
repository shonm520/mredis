#include <limits.h>
#include <string.h>
#include "process_command.h"
#include "config.h"
#include "redis_define.h"
#include "redis.h"
#include "object.h"


/* 将一个字符串类型的数转换为longlong类型.
* 如果可以转换的话,返回1,否则的话,返回0
*/
int static string2ll(const char *s, size_t slen, long long *value) {
	const char *p = s;
	size_t plen = 0;
	int negative = 0;
	unsigned long long v;

	if (plen == slen)
		return 0;

	/* Special case: first and only digit is 0. */
	if (slen == 1 && p[0] == '0') {
		if (value != NULL) *value = 0;
		return 1;
	}

	if (p[0] == '-') {
		negative = 1;
		p++; plen++;

		/* Abort on only a negative sign. */
		if (plen == slen)
			return 0;
	}

	/* First digit should be 1-9, otherwise the string should just be 0. */
	if (p[0] >= '1' && p[0] <= '9') {
		v = p[0] - '0';
		p++; plen++;
	}
	else if (p[0] == '0' && slen == 1) {
		*value = 0;
		return 1;
	}
	else {
		return 0;
	}

	while (plen < slen && p[0] >= '0' && p[0] <= '9') {
		if (v >(ULLONG_MAX / 10)) /* Overflow. */
			return 0;
		v *= 10;

		if (v > (ULLONG_MAX - (p[0] - '0'))) /* Overflow. */
			return 0;
		v += p[0] - '0';

		p++; plen++;
	}

	/* Return if not all bytes were used. */
	if (plen < slen)
		return 0;

	if (negative) {
		if (v >((unsigned long long)(-(LLONG_MIN + 1)) + 1)) /* Overflow. */
			return 0;
		if (value != NULL) *value = -v;
	}
	else {
		if (v > LLONG_MAX) /* Overflow. */
			return 0;
		if (value != NULL) *value = v;
	}
	return 1;
}

int process_command(redisClient* client, char* msg, int len)
{
    if (msg == NULL || len <= 0) {
        return -1;
    }

    int type = REDIS_REQ_INLINE;
    if (msg[0]  == '*')  {
        type = REDIS_REQ_MULTIBULK;
    }

    if (type == REDIS_REQ_INLINE)  {

    } 
    else if (type == REDIS_REQ_MULTIBULK)  {
        int count = 0;
        obj** ret = process_multibulk_buffer(msg, &count);
        if (ret)  {
            client->argv = ret;
            client->argc = count;
            return REDIS_OK;
        }
    }
    else  {
        
    }

    return REDIS_ERR;
}

robj** process_multibulk_buffer(char* querybuf, int* len)
{
    int pos = 0;
    int multibulklen = 0;
    long long ll_temp = 0;
    robj** argv = NULL;
    if (multibulklen == 0) {
		char* newline = strchr(querybuf, '\r');
		// 将参数个数，也即是 * 之后， \r\n 之前的数字取出并保存到 ll_temp 中 
		// 比如对于 *3\r\n ，那么 ll_temp 将等于 3
		string2ll(querybuf + 1, newline - (querybuf + 1), &ll_temp);

		// 参数数量之后的位置 
		// 比如对于 *3\r\n$3\r\n$SET\r\n... 来说,
		// pos 指向 *3\r\n$3\r\n$SET\r\n...
		//                ^
		//                |
		//               pos
		pos = (newline - querybuf) + 2;

		// 设置参数数量
		multibulklen = ll_temp;

		// 根据参数数量,为各个参数对象分配空间 
		//if (c->argv) zfree(c->argv);
		argv = zmalloc(sizeof(robj*) * multibulklen);
	}

	// 从querybuf中读入参数,并创建各个参数对象到c->argv
    int bulklen = -1;
    int argc = 0;
	while (multibulklen) {
		// 读入参数长度
		if (bulklen == -1) { // 这里指的是命令的长度
			// 确保"\r\n"存在 
			char* newline = strchr(querybuf + pos, '\r');

			// 读取长度,比如说 $3\r\nSET\r\n 会让 ll_temp 的值变成3
			string2ll(querybuf + pos + 1, newline - (querybuf + pos + 1), &ll_temp);

			// 定位到参数的开头
			// 比如 
			// $3\r\nSET\r\n...
			//       ^
			//       |
			//      pos
			pos += newline - (querybuf + pos) + 2;
			bulklen = ll_temp;
		}
		
        argv[argc++] = createStringObject(querybuf + pos, bulklen);
		pos += bulklen + 2;

		// 清空参数长度
		bulklen = -1;

		// 减少还需读入的参数个数
		multibulklen--;
	}

	// 如果本条命令的所有参数都已经读取完,那么返回
	if (multibulklen == 0) {
        if (len)  {
            *len = argc;
        }
        return argv;
    }

	return NULL;
}