
# C source files
SRC = main.c fingerprint.c command_shell.c database.c syscalls.c dcf77.c

# Project name
TARGET = main

# Optimization level, can be [0, 1, 2, 3, s].
OPT = s

# Set your target processor
MCPU = uc3b0256
BOARD = EVK1101
BOARDPATH = evk1101



# ENTER THE NAME OF YOUR TOOLCHAIN HERE:
###############################
TOOLCHAIN = ~/AVR32_toolchain/
###############################



# ** ** TOOLS ** **
AVR32TOOLS = $(TOOLCHAIN)avr32-gnu-toolchain-linux_x86_64/bin/

MAKE = make
CC = $(AVR32TOOLS)avr32-gcc
#AS = $(AVR32TOOLS)avr32-as
LD = $(AVR32TOOLS)avr32-gcc
OBJCOPY = $(AVR32TOOLS)avr32-objcopy
RM = rm -rf
DFU = $(TOOLCHAIN)dfu-programmer/src/dfu-programmer


# ** ** SOFTWARE FRAMEWORK ** **

AVR32FRAMEWORK = $(TOOLCHAIN)asf

AVR32HEADERS = 		-I ./
AVR32HEADERS += 	-I $(TOOLCHAIN)atmel-headers/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/common/utils/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/utils/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/utils/preprocessor/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/boards/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/boards/$(BOARDPATH)/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/cpu/cycle_counter/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/gpio/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/intc/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/pm/
#AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/pwm/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/usart/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/spi/
# for SD card
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/components/memory/sd_mmc/sd_mmc_spi/
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/common/services/storage/ctrl_access
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/services/fs/fat/
# for USB
AVR32HEADERS +=		-I $(AVR32FRAMEWORK)/avr32/drivers/usbb/_asf_v1/enum/


SRC += 	$(AVR32FRAMEWORK)/avr32/drivers/gpio/gpio.c
SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/intc/intc.c
SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/pm/pm.c
#SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/pm/pm_conf_clocks.c
#SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/pm/power_clocks_lib.c
#SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/pwm/pwm.c
SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/spi/spi.c
SRC +=	$(AVR32FRAMEWORK)/avr32/drivers/usart/usart.c
SRC +=	$(AVR32FRAMEWORK)/avr32/boards/$(BOARDPATH)/led.c
# for SD card
SRC +=	$(AVR32FRAMEWORK)/avr32/components/memory/sd_mmc/sd_mmc_spi/sd_mmc_spi.c
SRC +=	$(AVR32FRAMEWORK)/avr32/components/memory/sd_mmc/sd_mmc_spi/sd_mmc_spi_mem.c
SRC +=	$(AVR32FRAMEWORK)/common/services/storage/ctrl_access/ctrl_access.c
SRC +=	$(AVR32FRAMEWORK)/avr32/services/fs/fat/fat.c
SRC +=	$(AVR32FRAMEWORK)/avr32/services/fs/fat/fat_unusual.c
SRC +=	$(AVR32FRAMEWORK)/avr32/services/fs/fat/file.c
SRC +=	$(AVR32FRAMEWORK)/avr32/services/fs/fat/fsaccess.c
SRC +=	$(AVR32FRAMEWORK)/avr32/services/fs/fat/navigation.c
# for USB



# TODO: USB

AVR32_ASM = 	$(AVR32FRAMEWORK)/avr32/drivers/intc/exception.S


# ** ** object files ** **

ASM_SRC = $(AVR32_ASM)
OBJ = $(SRC:.c=.o) $(ASM_SRC:.S=.o)

# ** ** flags ** **

CFLAGS = -std=gnu99 -Wall -c -mpart=$(MCPU) -DBOARD=$(BOARD) -O$(OPT) $(AVR32HEADERS) -fno-common
ASFLAGS = -x assembler-with-cpp -c -mpart=$(MCPU) $(AVR32HEADERS)
LDFLAGS = -mpart=$(MCPU) -Wl,-T,link_$(MCPU).lds


# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
# USR RULES
# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **

# default rule, build the project
all: gccversion build

#clean up
clean: clean_list

#rebuild project
rebuild: clean_list gccversion build

#write program to flash
flash: all dfu

# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
# MESSAGES DISPLAYED DURING PROCESS
# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
MSG_BIN = Creating binary:
MSG_CLEANING = Cleaning project:
MSG_COMPILING = Compiling:
MSG_ASSEMBLING = Assembling:
MSG_LINKING = Linking:
MSG_COPY = Copying:

# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
# END USR RULES
# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **

# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
# COMPILATION RULES.
# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **

# Link: create ELF output file from object files.
build:	$(OBJ)
	@echo $(MSG_LINKING)
	$(LD) $(LDFLAGS) -o $(TARGET).elf $(OBJ)
	@echo
	@echo $(MSG_COPY)
	$(OBJCOPY) -O ihex $(TARGET).elf $(TARGET).hex
	@echo

# Compile: create object files from C source files.
%.o: %.c
	@echo $(MSG_COMPILING)
	$(CC) $(CFLAGS) -o $@ $<
	@echo

# Assemble: create object files from Assembler source files.
%.o: %.S
	@echo $(MSG_ASSEMBLING)
	$(CC) $(ASFLAGS) -o $@ $<
	@echo

# Remove generated files
clean_list:
	@echo
	@echo $(MSG_CLEANING)
	$(RM) $(TARGET).elf
	$(RM) $(TARGET).hex
	$(RM) $(OBJ)

#display gcc version
gccversion:
	@echo
	@$(CC) --version

dfu:
	$(DFU) at32$(MCPU) erase
	$(DFU) at32$(MCPU) flash --suppress-bootloader-mem $(TARGET).hex
	$(DFU) at32$(MCPU) launch

# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
# END
# ** ** ** *** ** ** ** ** ** ** ** ** ** ** **
