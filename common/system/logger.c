
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/iosupport.h>
#include <wiiu/os.h>

#include "wiiu_dbg.h"

static int log_socket = -1;
static volatile int log_lock = 0;

static int log_write(struct _reent *r, void* fd, const char *ptr, size_t len);

static devoptab_t dotab_stdout = {
   "stdout_net", // device name
   0,            // size of file structure
   NULL,         // device open
   NULL,         // device close
   log_write,    // device write
   NULL,
   /* ... */
};

void log_init(const char * ipString, int port)
{
	log_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (log_socket < 0)
		return;

	struct sockaddr_in connect_addr;
	memset(&connect_addr, 0, sizeof(connect_addr));
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = port;
	inet_aton(ipString, &connect_addr.sin_addr);

	if(connect(log_socket, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) < 0)
	{
	    socketclose(log_socket);
	    log_socket = -1;
       return;
	}

   devoptab_list[STD_OUT] = &dotab_stdout;
   devoptab_list[STD_ERR] = &dotab_stdout;
}

void log_deinit(void)
{
   fflush(stdout);
   fflush(stderr);
    if(log_socket >= 0)
    {
        socketclose(log_socket);
        log_socket = -1;
    }
}
static int log_write(struct _reent *r, void* fd, const char *ptr, size_t len)
{
   if(log_socket < 0)
       return len;

   while(log_lock)
      OSSleepTicks(((248625000/4)) / 1000);
   log_lock = 1;

   int ret;
   while (len > 0) {
       int block = len < 1400 ? len : 1400; // take max 1400 bytes per UDP packet
       ret = send(log_socket, ptr, block, 0);
       if(ret < 0)
           break;

       len -= ret;
       ptr += ret;
   }

   log_lock = 0;

   return len;
}
void net_print(const char* str)
{
   log_write(NULL, 0, str, strlen(str));
}

void net_print_exp(const char* str)
{
   send(log_socket, str, strlen(str), 0);
}
