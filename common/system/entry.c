#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <fs/fs_utils.h>
#include <fs/sd_fat_devoptab.h>
#include <system/memory.h>
#include <system/exception_handler.h>
#include <sys/iosupport.h>

#include <wiiu/os.h>
#include <wiiu/procui.h>
#include <wiiu/vpad.h>
#include <wiiu/sysapp.h>

#include "wiiu_dbg.h"

int main(int argc, char **argv);

void __eabi()
{

}

__attribute__((weak))
void __init(void)
{
   extern void(*__CTOR_LIST__[])(void);
   void(**ctor)(void) = __CTOR_LIST__;
   while(*ctor)
      (*ctor++)();
}


__attribute__((weak))
void __fini(void)
{
   extern void(*__DTOR_LIST__[])(void);
   void(**ctor)(void) = __DTOR_LIST__;
   while(*ctor)
      (*ctor++)();
}

/* HBL elf entry point */
void InitFunctionPointers(void);
int __entry_menu(int argc, char **argv)
{
   InitFunctionPointers();
   memoryInitialize();
   mount_sd_fat("sd");

   __init();
   int ret = main(argc, argv);
   __fini();

   unmount_sd_fat("sd");
   memoryRelease();
   return ret;
}

/* RPX entry point */
__attribute__((noreturn))
void _start(int argc, char **argv)
{
   memoryInitialize();
   mount_sd_fat("sd");

   __init();
   int ret = main(argc, argv);
   __fini();

   unmount_sd_fat("sd");
   memoryRelease();
   SYSRelaunchTitle(argc, argv);
   exit(ret);
}
