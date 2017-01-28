#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <wiiu/os.h>
#include <wiiu/procui.h>
#include <wiiu/avm.h>
#include <sys/socket.h>
#include <system/memory.h>
#include "system/wiiu_dbg.h"


void setup_os_exceptions(void);
void log_init(const char * ipString, int port);
void log_deinit(void);
int main(int argc, char **argv)
{
   setup_os_exceptions();
   socket_lib_init();
#if defined(LOGGER_IP) && defined(LOGGER_TCP_PORT)
   log_init(LOGGER_IP, LOGGER_TCP_PORT);
#endif

   AVMSetTVOutPort(AVM_TV_PORT_SCART, AVM_TV_SCAN_MODE_480I);

#if defined(LOGGER_IP) && defined(LOGGER_TCP_PORT)
   log_deinit();
#endif
   return 0;
}
