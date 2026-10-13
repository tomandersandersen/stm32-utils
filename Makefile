CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
OPENOCD := openocd
GDB      := gdb-multiarch

GDB_PORT := 3333

TARGET  := firmware
BUILD   := build

CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CMSIS_CORE   := submodules/CMSIS_6/CMSIS/Core/Include
CMSIS_DEVICE := submodules/cmsis-device-f4/Include

INCLUDES := \
    -I$(CMSIS_CORE) \
    -I$(CMSIS_DEVICE) \
    -Iinclude

DEFINES := -DSTM32F407xx

CFLAGS := $(CPU) \
          $(DEFINES) \
          $(INCLUDES) \
          -std=c11 \
          -Wall -Wextra -Werror \
          -ffreestanding \
          -fno-builtin \
          -ffunction-sections \
          -fdata-sections \
          -Og -g3

ASFLAGS := $(CPU) -g3

LDFLAGS := $(CPU) \
           -Tlinker.ld \
           -nostdlib \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD)/$(TARGET).map

C_SOURCES :=            \
    src/main.c          \
    src/start.c         \
    src/interrupts.c    \
    src/led.c           \
    src/clock.c         \
    src/uart.c          \
    src/log.c           \
    src/assert.c        \

ASM_SOURCES := startup.s


OBJECTS := \
    $(patsubst %.c,$(BUILD)/%.o,$(C_SOURCES)) \
    $(patsubst %.s,$(BUILD)/%.o,$(ASM_SOURCES))

.PHONY: all clean size flash debug gdb-server


all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

$(BUILD)/$(TARGET).elf: $(OBJECTS) linker.ld
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(OBJECTS) -lgcc -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@


flash: $(BUILD)/$(TARGET).elf
	$(OPENOCD) \
	    -f interface/stlink.cfg \
	    -f target/stm32f4x.cfg \
	    -c "program $< verify reset exit"

debug: $(BUILD)/$(TARGET).elf
	$(GDB) $< \
	    -ex "target remote :$(GDB_PORT)" \
	    -ex "monitor reset halt" \
	    -ex "break main"

gdb-server:
	openocd \
	    -f interface/stlink.cfg \
	    -f target/stm32f4x.cfg \
	    -c "gdb_port $(GDB_PORT)" \
	    -c "init" \
	    -c "reset halt"

size: $(BUILD)/$(TARGET).elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD)
