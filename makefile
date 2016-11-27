TARGET		:= gx2_example

BUILD_HBL_ELF	 = 1
BUILD_RPX	 = 1
DEBUG            = 0
LOGGER_IP        =
LOGGER_TCP_PORT	 =


OBJ :=
OBJ += system/entry.o
OBJ += system/memory.o
OBJ += system/exception_handler.o
OBJ += system/logger.o
OBJ += fs/sd_fat_devoptab.o
OBJ += fs/fs_utils.o

OBJ += main.o
OBJ += shader/tex_shader.o

RPX_OBJ      = system/stubs_rpl.o
HBL_ELF_OBJ  = system/dynamic.o system/stubs_elf.o

DEFINES :=

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

ifeq ($(strip $(WUT_ROOT)),)
$(error "Please set WUT_ROOT in your environment. export WUT_ROOT=<path to>WUT")
endif

export PATH	  := $(PATH):$(DEVKITPPC)/bin

PREFIX := powerpc-eabi-

CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
AS      := $(PREFIX)as
AR      := $(PREFIX)ar
OBJCOPY := $(PREFIX)objcopy
STRIP   := $(PREFIX)strip
NM      := $(PREFIX)nm
LD      := $(CXX)

ELF2RPL   := $(WUT_ROOT)/tools/bin/elf2rpl

INCDIRS := -I. -I$(WUT_ROOT)/include
LIBDIRS :=

CFLAGS  := -mrvl -mcpu=750 -meabi -mhard-float
LDFLAGS :=

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3
endif
LDFLAGS := $(CFLAGS)

ASFLAGS := $(CFLAGS) -mregnames

CFLAGS +=  -ffast-math -Werror=implicit-function-declaration
#CFLAGS += -fomit-frame-pointer -mword-relocations
#CFLAGS	+= -Wall
CFLAGS += -Dstatic_assert=_Static_assert
CFLAGS += -DWIIU -DMSB_FIRST
CFLAGS += -DHAVE_MAIN
CFLAGS += -DRARCH_INTERNAL -DRARCH_CONSOLE
CFLAGS += -DHAVE_FILTERS_BUILTIN $(DEFINES)

ifneq ($(LOGGER_IP),)
   CFLAGS += -DLOGGER_IP='"$(LOGGER_IP)"'
endif

ifneq ($(LOGGER_TCP_PORT),)
   CFLAGS += -DLOGGER_TCP_PORT=$(LOGGER_TCP_PORT)
endif

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions

LDFLAGS  += -Wl,--gc-sections

LIBS	:= -lm

RPX_LDFLAGS      := -T system/link_rpl.ld
RPX_LDFLAGS      += -L$(WUT_ROOT)/lib -L$(DEVKITPPC)/lib
RPX_LDFLAGS      += -pie -fPIE
RPX_LDFLAGS      += -z common-page-size=64 -z max-page-size=64
RPX_LDFLAGS      += -nostartfiles

HBL_ELF_LDFLAGS  := -T system/link_elf.ld

TARGETS :=
ifeq ($(BUILD_RPX), 1)
TARGETS += $(TARGET).rpx
endif

ifeq ($(BUILD_HBL_ELF), 1)
TARGETS += $(TARGET).elf
endif

DEPFLAGS    = -MT $@ -MMD -MP -MF $*.Tdepend
POSTCOMPILE = mv -f $*.Tdepend $*.depend


all: $(TARGETS)

%.o: %.cpp
%.o: %.cpp %.depend
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCDIRS) $(DEPFLAGS)
	$(POSTCOMPILE)

%.o: %.c
%.o: %.c %.depend
	$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS) $(DEPFLAGS)
	$(POSTCOMPILE)


%.o: %.S
%.o: %.S %.depend
	$(CC) -c -o $@ $< $(ASFLAGS) $(INCDIRS) $(DEPFLAGS)
	$(POSTCOMPILE)

%.o: %.s
%.o: %.s %.depend
	$(CC) -c -o $@ $< $(ASFLAGS) $(INCDIRS) $(DEPFLAGS)
	$(POSTCOMPILE)
%.a:
	$(AR) -rc $@ $^

%.depend: ;


$(TARGET).elf: $(OBJ) $(HBL_ELF_OBJ) libretro_wiiu.a system/link_elf.ld
	$(LD) $(OBJ) $(HBL_ELF_OBJ) $(LDFLAGS) $(HBL_ELF_LDFLAGS) $(LIBDIRS) $(LIBS) -o $@

$(TARGET).rpx.elf: $(OBJ) $(RPX_OBJ) libretro_wiiu.a system/link_rpl.ld
	$(LD) $(OBJ) $(RPX_OBJ) $(LDFLAGS) $(RPX_LDFLAGS) $(LIBDIRS)  $(LIBS) -o $@

$(TARGET).rpx: $(TARGET).rpx.elf
	-$(ELF2RPL) $(notdir $<) $@

clean:
	rm -f $(OBJ) $(RPX_OBJ) $(HBL_ELF_OBJ) $(TARGET).elf $(TARGET).rpx.elf $(TARGET).rpx
	rm -f $(OBJ:.o=.depend) $(RPX_OBJ:.o=.depend) $(HBL_ELF_OBJ:.o=.depend)

.PHONY: clean all
.PRECIOUS: %.depend

-include $(OBJ:.o=.depend) $(RPX_OBJ:.o=.depend) $(HBL_ELF_OBJ:.o=.depend)
