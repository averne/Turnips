ifeq ($(strip $(DEVKITPRO)),)
    $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR           ?=   $(CURDIR)

VERSION           =   0.0.0
COMMIT            =   $(shell git rev-parse --short HEAD)

# -----------------------------------------------

APP_TITLE         =    $(notdir $(CURDIR))
APP_AUTHOR        =    averne
APP_ICON          =    icon.jpg
APP_VERSION       =    $(VERSION)-$(COMMIT)
APP_TITLEID       =

TARGET            =    $(APP_TITLE).nro
OUT               =    out
BUILD             =    build
SOURCES           =    src
INCLUDES          =    include
CUSTOM_LIBS       =    lib/glfw lib/glad lib/imgui
ROMFS             =

DEFINES           =    __SWITCH__ VERSION=\"$(VERSION)\" COMMIT=\"$(COMMIT)\"
ARCH              =    -march=armv8-a+crc+crypto+simd -mtune=cortex-a57 -mtp=soft -fpie
FLAGS             =    -Wall -pipe -g -O2 -ffunction-sections -fdata-sections
CFLAGS            =    -std=gnu11
CXXFLAGS          =    -std=gnu++17 -fno-rtti -fno-exceptions
ASFLAGS           =
LDFLAGS           =    -Wl,-pie -specs=$(DEVKITPRO)/libnx/switch.specs -g
LINKS             =    -lglfw -lEGL -lglapi -ldrm_nouveau -lm -lglad -limgui -lnx

PREFIX            =    aarch64-none-elf-
CC                =    $(PREFIX)gcc
CXX               =    $(PREFIX)g++
AS                =    $(PREFIX)as
LD                =    $(PREFIX)g++
NM                =    $(PREFIX)gcc-nm

# -----------------------------------------------

export PATH      :=    $(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitA64/bin:$(PORTLIBS)/bin:$(PATH)

PORTLIBS          =    $(DEVKITPRO)/portlibs/switch
LIBNX             =    $(DEVKITPRO)/libnx
LIBS              =    $(CUSTOM_LIBS) $(LIBNX) $(PORTLIBS)

# -----------------------------------------------

CFILES            =    $(shell find $(SOURCES) -name *.c)
CPPFILES          =    $(shell find $(SOURCES) -name *.cpp)
SFILES            =    $(shell find $(SOURCES) -name *.s -or -name *.S)
OFILES            =    $(CFILES:%=$(BUILD)/%.o) $(CPPFILES:%=$(BUILD)/%.o) $(SFILES:%=$(BUILD)/%.o)
DFILES            =    $(OFILES:.o=.d)

LIBS_TARGET       =    $(shell find $(addsuffix /lib,$(CUSTOM_LIBS)) -name "*.a" 2>/dev/null)
ELF_TARGET        =    $(if $(OUT:=), $(OUT)/$(APP_TITLE).elf, .$(OUT)/$(APP_TITLE).elf)
NACP_TARGET       =    $(if $(OUT:=), $(OUT)/$(APP_TITLE).nacp, .$(OUT)/$(APP_TITLE).nacp)
NRO_TARGET        =    $(if $(OUT:=), $(OUT)/$(TARGET), .$(OUT)/$(TARGET))

DEFINE_FLAGS      =    $(addprefix -D,$(DEFINES))
INCLUDE_FLAGS     =    $(addprefix -I$(CURDIR)/,$(INCLUDES)) $(foreach dir,$(CUSTOM_LIBS),-I$(CURDIR)/$(dir)/include) \
					   $(foreach dir,$(filter-out $(CUSTOM_LIBS),$(LIBS)),-I$(dir)/include)
LIB_FLAGS         =    $(foreach dir,$(LIBS),-L$(dir)/lib)

# -----------------------------------------------

ifeq ($(strip $(APP_TITLE)),)
    APP_TITLE     =    $(TARGET)
endif

ifeq ($(strip $(APP_AUTHOR)),)
    APP_AUTHOR    =    Unspecified
endif

ifeq ($(strip $(APP_VERSION)),)
    APP_VERSION   =    Unspecified
endif

ifneq ($(APP_TITLEID),)
    NACPFLAGS    +=    --titleid=$(strip $(APP_TITLEID))
endif

ifeq ($(strip $(APP_ICON)),)
    APP_ICON      =    $(LIBNX)/default_icon.jpg
endif

NROFLAGS          =    --icon=$(strip $(APP_ICON)) --nacp=$(strip $(NACP_TARGET))

ifneq ($(ROMFS),)
    NROFLAGS     +=    --romfsdir=$(strip $(ROMFS))
endif

# -----------------------------------------------

.SUFFIXES:

.PHONY: all libs run dist clean mrproper $(CUSTOM_LIBS)

all: $(NRO_TARGET)
	@:

run: all
	@echo "Running" $(NRO_TARGET)
	@nxlink -s $(NRO_TARGET)

dist: $(NX_TARGET)
	@rm -f $(OUT)/$(TARGET)-*.zip
	@7z a $(OUT)/$(TARGET)-$(VERSION)-$(COMMIT).zip $(NRO_TARGET) >/dev/null
	@echo Compressed to $(OUT)/$(TARGET)-$(VERSION)-$(COMMIT).zip

libs: $(CUSTOM_LIBS)
	@:

$(CUSTOM_LIBS):
	@$(MAKE) -s --no-print-directory -C $@

$(NRO_TARGET): $(ELF_TARGET) $(NACP_TARGET) $(APP_ICON)
	@echo " NRO " $@
	@mkdir -p $(dir $@)
	@elf2nro $< $@ $(NROFLAGS) > /dev/null
	@echo "Built" $(notdir $@)

$(ELF_TARGET): $(OFILES) $(LIBS_TARGET) | libs
	@echo " LD  " $@
	@mkdir -p $(dir $@)
	@$(LD) $(ARCH) $(LDFLAGS) -Wl,-Map,$(BUILD)/$(TARGET).map $(LIB_FLAGS) $(OFILES) $(LINKS) -o $@
	@$(NM) -CSn $@ > $(BUILD)/$(TARGET).lst

$(BUILD)/%.c.o: %.c
	@echo " CC  " $@
	@mkdir -p $(dir $@)
	@$(CC) -MMD -MP $(ARCH) $(FLAGS) $(CFLAGS) $(DEFINE_FLAGS) $(INCLUDE_FLAGS) -c $(CURDIR)/$< -o $@

$(BUILD)/%.cpp.o: %.cpp
	@echo " CXX " $@
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(ARCH) $(FLAGS) $(CXXFLAGS) $(DEFINE_FLAGS) $(INCLUDE_FLAGS) -c $(CURDIR)/$< -o $@

$(BUILD)/%.s.o: %.s %.S
	@echo " AS  " $@
	@mkdir -p $(dir $@)
	@$(AS) -MMD -MP -x assembler-with-cpp $(ARCH) $(FLAGS) $(ASFLAGS) $(INCLUDE_FLAGS) -c $(CURDIR)/$< -o $@

%.nacp:
	@echo " NACP" $@
	@mkdir -p $(dir $@)
	@nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ $(NACPFLAGS)

clean:
	@echo Cleaning...
	@rm -rf $(BUILD) $(OUT)

mrproper: clean
	@for dir in $(CUSTOM_LIBS); do $(MAKE) --no-print-directory -C $$dir clean; done

-include $(DFILES)
