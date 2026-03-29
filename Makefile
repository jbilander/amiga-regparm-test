CC      = m68k-amiga-elf-gcc
OBJDUMP = m68k-amiga-elf-objdump

# Path to AmigaOS NDK Include_H directory.
# Override via environment or command line: make NDK=/path/to/NDK/Include_H
NDK     ?= $(HOME)/amiga/ndk/Include_H

CFLAGS  = -m68000 -O2 -fomit-frame-pointer -I$(NDK)
LDFLAGS = -nostdlib -nostartfiles -Wl,--emit-relocs,-Ttext=0

TARGET  = test_regparm

all: $(TARGET)

$(TARGET): test_regparm_cli.o
	$(CC) $(CFLAGS) -o $(TARGET).elf $< $(LDFLAGS)
	elf2hunk $(TARGET).elf $(TARGET)
	$(OBJDUMP) -D $(TARGET).elf > $(TARGET).s
	@echo ""
	@echo "Built: $(TARGET)  (AmigaOS hunk executable)"
	@echo "Copy to Amiga and run from shell: $(TARGET)"

test_regparm_cli.o: test_regparm_cli.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.elf *.s $(TARGET)
