
OBJ += $(WIIU_COMMON_DIR)/system/entry.o
OBJ += $(WIIU_COMMON_DIR)/system/memory.o
OBJ += $(WIIU_COMMON_DIR)/system/exception_handler.o
OBJ += $(WIIU_COMMON_DIR)/system/logger.o
OBJ += $(WIIU_COMMON_DIR)/fs/sd_fat_devoptab.o
OBJ += $(WIIU_COMMON_DIR)/fs/fs_utils.o

RPX_OBJ      = $(WIIU_COMMON_DIR)/system/rpx/stubs.o
HBL_ELF_OBJ  = $(WIIU_COMMON_DIR)/system/elf/dynamic.o $(WIIU_COMMON_DIR)/system/elf/stubs.o

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

export PATH := $(PATH):$(DEVKITPPC)/bin

PREFIX := powerpc-eabi-

CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
AS      := $(PREFIX)as
AR      := $(PREFIX)ar
OBJCOPY := $(PREFIX)objcopy
STRIP   := $(PREFIX)strip
NM      := $(PREFIX)nm
LD      := $(CXX)

ELF2RPL   := $(WIIU_COMMON_DIR)/elf2rpl/elf2rpl
ifneq ($(findstring Linux,$(shell uname -a)),)
else ifneq ($(findstring Darwin,$(shell uname -a)),)
else
   ELF2RPL   := $(ELF2RPL).exe
endif

INCDIRS += -I$(WIIU_COMMON_DIR) -I$(WIIU_COMMON_DIR)/include -I$(DEVKITPRO)/portlibs/ppc/include
LIBDIRS := -L$(DEVKITPRO)/portlibs/ppc/lib

CFLAGS  += -mwup -mcpu=750 -meabi -mhard-float

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3
endif
LDFLAGS += $(CFLAGS) -Wl,--gc-sections
ASFLAGS := $(CFLAGS) -mregnames

CFLAGS += -ffast-math -Werror=implicit-function-declaration

ifneq ($(LOGGER_IP),)
   CFLAGS += -DLOGGER_IP='"$(LOGGER_IP)"'
endif

ifneq ($(LOGGER_TCP_PORT),)
   CFLAGS += -DLOGGER_TCP_PORT=$(LOGGER_TCP_PORT)
endif

CXXFLAGS += $(CFLAGS) -fno-rtti -fno-exceptions


LIBS	+= -lm

RPX_LDFLAGS      := -T $(WIIU_COMMON_DIR)/system/rpx/link.ld
RPX_LDFLAGS      += -pie -fPIE
RPX_LDFLAGS      += -z common-page-size=64 -z max-page-size=64
RPX_LDFLAGS      += -nostartfiles

HBL_ELF_LDFLAGS  := -T $(WIIU_COMMON_DIR)/system/elf/link.ld

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

$(ELF2RPL):
	$(MAKE) -C $(dir $(ELF2RPL))

$(TARGET).elf: $(OBJ) $(HBL_ELF_OBJ) $(WIIU_COMMON_DIR)/system/elf/link.ld
	$(LD) $(OBJ) $(HBL_ELF_OBJ) $(LDFLAGS) $(HBL_ELF_LDFLAGS) $(LIBDIRS) $(LIBS) -o $@

$(TARGET).rpx.elf: $(OBJ) $(RPX_OBJ) $(WIIU_COMMON_DIR)/system/rpx/link.ld
	$(LD) $(OBJ) $(RPX_OBJ) $(LDFLAGS) $(RPX_LDFLAGS) $(LIBDIRS)  $(LIBS) -o $@

$(TARGET).rpx: $(TARGET).rpx.elf $(ELF2RPL)
	$(ELF2RPL) $(notdir $<) $@

clean:
	rm -f $(OBJ) $(RPX_OBJ) $(HBL_ELF_OBJ) $(TARGET).elf $(TARGET).rpx.elf $(TARGET).rpx
	rm -f $(OBJ:.o=.depend) $(RPX_OBJ:.o=.depend) $(HBL_ELF_OBJ:.o=.depend)

.PHONY: clean all
.PRECIOUS: %.depend

-include $(OBJ:.o=.depend) $(RPX_OBJ:.o=.depend) $(HBL_ELF_OBJ:.o=.depend)

