# demo/Makefile

TOOL_BIN = /opt/local/bin
CC = $(TOOL_BIN)/avr-gcc 
OBJCOPY = $(TOOL_BIN)/avr-objcopy 
OBJDUMP = $(TOOL_BIN)/avr-objdump 

CFLAGS = -Os -I.. -mmcu=atmega4809 -B/opt/local/avr/packs/mega

all: main.hex main.asm

main.elf: main.o octos.o
	$(CC) $(CFLAGS) -o $@ $^

main.hex: main.elf
	$(OBJCOPY) -O ihex $^ $@

octos.o: ../octos.S
	$(CC) $(CFLAGS) -c $^ -o $@

main.o: main.c ../octos.h
	$(CC) $(CFLAGS) -c main.c -o main.o

main.asm: main.elf
	$(OBJDUMP) -d main.elf > main.asm

clean:
	rm -f *.o *.elf *.hex *.asm

# --- last line ---
