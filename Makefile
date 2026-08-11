# --- DevkitPro Paths ---
export DEVKITPRO ?= /opt/devkitpro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM

# --- App Configuration ---
TARGET		:= PodMon
BUILD		:= build
SOURCES		:= source
DATA		:= data
INCLUDES	:= include

TITLE		:= PodMon
DESCRIPTION	:= Offline podcast application and game
AUTHOR		:= Chris Infante

ICON_LARGE	:= icon_large.png
META_XML	:= meta.xml

# --- Toolchain Check ---
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. See https://devkitpro.org/wiki/Getting_Started")
endif

include $(DEVKITARM)/base_rules

# --- Compiler Flags ---
ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard
CFLAGS		:= -g -Wall -O2 -mword-relocations $(ARCH) -ffunction-sections
CFLAGS := -g -Wall -O2 -mword-relocations \
          -march=armv6k -mtune=mpcore -mfloat-abi=hard -ffunction-sections \
          -I"/c/devkitPro/libctru/include" \
          -I"/c/devkitPro/portlibs/3ds/include" \
          -I"include" \
          $(DEFINES)
CXXFLAGS	:= $(CFLAGS)

ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs $(ARCH) -L"/c/devkitPro/portlibs/3ds/lib" -L"$(DEVKITPRO)/libctru/lib" -Wl,-Map,$(notdir $*.map)

LIBS := -lmpg123 -lctru -lm

# --- Build Rules ---
CFILES		:= $(notdir $(wildcard $(SOURCES)/*.c))
SFILES		:= $(notdir $(wildcard $(SOURCES)/*.s))
OBJS		:= $(addprefix $(BUILD)/, $(CFILES:.c=.o)) $(addprefix $(BUILD)/, $(SFILES:.s=.o))

$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf $(ICON_LARGE)
	@3dsxtool $< $@ --smdh=$(TARGET).smdh

$(TARGET).elf: $(OBJS)
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(TARGET).smdh: $(ICON_LARGE)
	@smdhtool --create "$(TITLE)" "$(DESCRIPTION)" "$(AUTHOR)" $(ICON_LARGE) $@

clean:
	@echo "Cleaning up..."
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh $(notdir $(TARGET).map)