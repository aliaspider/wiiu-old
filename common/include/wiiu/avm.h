#pragma once

#include <wiiu/types.h>

typedef enum
{
   AVM_TV_SCAN_MODE_NONE,
   AVM_TV_SCAN_MODE_576I,
   AVM_TV_SCAN_MODE_480I,
   AVM_TV_SCAN_MODE_480P,
   AVM_TV_SCAN_MODE_720P,
   AVM_TV_SCAN_MODE_unk,
   AVM_TV_SCAN_MODE_1080I,
   AVM_TV_SCAN_MODE_1080P
} avm_tv_scan_modes;

typedef enum
{
   AVM_TV_PORT_NONE,
   AVM_TV_PORT_unk0, /* COMPONENTS ? */
   AVM_TV_PORT_SVIDEO,
   AVM_TV_PORT_SCART,
   AVM_TV_PORT_unk1  /* OFF ? */
} avm_tv_port;

int AVMSetTVOutPort(avm_tv_port port, avm_tv_scan_modes mode);

int AVMGetTVBufferPitch(u32 *out);
int AVMGetTVScanMode(u32 *out);
int AVMSetTVScanMode(u32 unk1, u32 unk2, u32 unk3);
int AVMSetTVVideoRegion(int region);
