TOOLCHAIN_BIN ?= F:/MountRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin
SDK_ROOT      ?= ../../../CHC585/EXAM/SRC
PROJECT       := ch585m_spi_slave_test
BUILD         := build

CC      := $(TOOLCHAIN_BIN)/riscv-wch-elf-gcc.exe
AS      := $(TOOLCHAIN_BIN)/riscv-wch-elf-gcc.exe
OBJCOPY := $(TOOLCHAIN_BIN)/riscv-wch-elf-objcopy.exe
SIZE    := $(TOOLCHAIN_BIN)/riscv-wch-elf-size.exe

ARCH_FLAGS := -march=rv32imc_zba_zbb_zbc_zbs_xw -mabi=ilp32 -msmall-data-limit=8 -msave-restore
COMMON_FLAGS := $(ARCH_FLAGS) -Os -g -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wall -std=gnu99
LDFLAGS := $(ARCH_FLAGS) -nostartfiles -Xlinker --gc-sections --specs=nano.specs --specs=nosys.specs
LIBS := -L$(SDK_ROOT)/StdPeriphDriver -lISP585 -lm

INC := -Isrc
INC += -I../ch585_frontend
INC += -I$(SDK_ROOT)/StdPeriphDriver/inc
INC += -I$(SDK_ROOT)/RVMSIS

DEFS := -DDEBUG=0
DEFS += -DSYSCLK_FREQ=CLK_SOURCE_HSE_PLL_78MHz -DFREQ_SYS=78000000
DEFS += -DCH585_SPI0_MISO_STRONG_DRIVE=1
DEFS += -DCH585_USE_SHORT_FRAME=1
DEFS += -DCH585_USE_PIPELINE_SHORT=0
DEFS += -DCH585_USE_REQUEST_ONLY_SHORT=0
DEFS += -DCH585_FAST_SIM_FRAME=0
DEFS += -DCH585_ADC_PROBE_MODE=1
DEFS += -DCH585_ADC_PROBE_RIGHT_HALF=0
DEFS += -DCH585_ADC_PROBE_DEBUG_KEY=21U
DEFS += -DCH585_KEY_ENABLE_RAPID_TRIGGER=1
DEFS += $(DEFS_EXTRA)

C_SOURCES := src/main.c
C_SOURCES += src/ch585_local_periph_test.c
C_SOURCES += src/ch585_soft_i2c.c
C_SOURCES += ../ch585_frontend/ads7948.c
C_SOURCES += ../ch585_frontend/ch585_i2c_bus.c
C_SOURCES += ../ch585_frontend/ch585_local_inputs.c
C_SOURCES += ../ch585_frontend/ch585_power_status.c
C_SOURCES += ../ch585_frontend/hx24lc16.c
C_SOURCES += ../ch585_frontend/max17048.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_clk.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_flash.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_gpio.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_pwr.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_spi0.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_spi1.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_sys.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_uart0.c
C_SOURCES += $(SDK_ROOT)/StdPeriphDriver/CH58x_uart1.c

ASM_SOURCES := $(SDK_ROOT)/Startup/startup_CH585.S
LDSCRIPT := $(SDK_ROOT)/Ld/Link.ld

OBJS := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(C_SOURCES)))
OBJS += $(patsubst %.S,$(BUILD)/%.o,$(notdir $(ASM_SOURCES)))
VPATH := $(sort $(dir $(C_SOURCES)) $(dir $(ASM_SOURCES)))

.PHONY: all clean

all: $(BUILD)/$(PROJECT).elf $(BUILD)/$(PROJECT).hex $(BUILD)/$(PROJECT).bin
	@"$(SIZE)" "$(BUILD)/$(PROJECT).elf"

$(BUILD)/%.o: %.c | $(BUILD)
	@echo "CC  $(notdir $<)"
	@"$(CC)" -c $(COMMON_FLAGS) $(DEFS) $(INC) -MMD -MP -MF"$(@:.o=.d)" "$<" -o "$@"

$(BUILD)/%.o: %.S | $(BUILD)
	@echo "AS  $(notdir $<)"
	@"$(AS)" -x assembler-with-cpp -c $(ARCH_FLAGS) $(DEFS) $(INC) "$<" -o "$@"

$(BUILD)/$(PROJECT).elf: $(OBJS) $(LDSCRIPT)
	@echo "LD  $(notdir $@)"
	@"$(CC)" $(OBJS) $(LDFLAGS) -T"$(LDSCRIPT)" -Wl,-Map,"$(BUILD)/$(PROJECT).map" $(LIBS) -o "$@"

$(BUILD)/%.hex: $(BUILD)/%.elf
	@echo "HEX $(notdir $@)"
	@"$(OBJCOPY)" -O ihex "$<" "$@"

$(BUILD)/%.bin: $(BUILD)/%.elf
	@echo "BIN $(notdir $@)"
	@"$(OBJCOPY)" -O binary "$<" "$@"

$(BUILD):
ifeq ($(OS),Windows_NT)
	@powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$@' | Out-Null"
else
	@mkdir -p "$@"
endif

clean:
	rm -rf "$(BUILD)"

-include $(OBJS:.o=.d)
