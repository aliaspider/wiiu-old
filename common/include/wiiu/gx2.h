#pragma once

#include <wiiu/gx2/context.h>
#include <wiiu/gx2/common.h>
#include <wiiu/gx2/display.h>
#include <wiiu/gx2/displaylist.h>
#include <wiiu/gx2/draw.h>
#include <wiiu/gx2/enum.h>
#include <wiiu/gx2/event.h>
#include <wiiu/gx2/mem.h>
#include <wiiu/gx2/registers.h>
#include <wiiu/gx2/sampler.h>
#include <wiiu/gx2/shaders.h>
#include <wiiu/gx2/state.h>
#include <wiiu/gx2/surface.h>
#include <wiiu/gx2/swap.h>
#include <wiiu/gx2/tessellation.h>
#include <wiiu/gx2/texture.h>

#define _X_ 0x00
#define _Y_ 0x01
#define _Z_ 0x02
#define _W_ 0x03
#define _0_ 0x04
#define _1_ 0x05
#define GX2_COMP_SEL(c0, c1, c2, c3) (((c0) << 24) | ((c1) << 16) | ((c2) << 8) | (c3))
