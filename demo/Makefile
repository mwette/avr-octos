# demo/Makefile

TOOL_BIN = /opt/local/bin
CC = $(TOOL_BIN)/avr-gcc 
OBJCOPY = $(TOOL_BIN)/avr-objcopy 
OBJDUMP = $(TOOL_BIN)/avr-objdump 

CFLAGS = -Os -I.. -mmcu=atmega4809 -B/opt/local/avr/packs/mega

all: main.hex main.asm swintr.hex swintr.asm

octos.o: ../octos.S
	$(CC) -o $@ -c $(CFLAGS) $^

main.elf: main.o octos.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $^

%.elf: %.c
	$(CC) -o $@ $(CFLAGS) $^

%.hex: %.elf
	$(OBJCOPY) -O ihex $^ $@

%.asm: %.elf
	$(OBJDUMP) -d $^ > $@

clean:
	rm -f *.o *.elf *.hex *.asm

# --- last line ---
