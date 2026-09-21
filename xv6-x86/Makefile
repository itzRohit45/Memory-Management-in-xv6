# Some make command info :
#
# make (or make build) : Builds the kernel and file system (build/xv6.img).
# make qemu            : Builds and runs xv6 in the QEMU emulator.
# make qemu-nox        : Builds and runs xv6 in QEMU without a graphical window.
# make bochs           : Builds and runs xv6 in the Bochs emulator.
#
# make qemu-gdb        : Builds and starts xv6 paused, waiting for GDB. (In another terminal, run: gdb -x .gdbinit)
# make qemu-nox-gdb    : Same as qemu-gdb, but without a graphical window.
#
# make clean           : Removes all build artifacts (in 'build/' and root).
# make print           : Generates a printable PDF (xv6.pdf) of the source code.
# make tags            : Generates 'etags' file for code navigation in editors.
# make dist            : Creates a clean 'dist' directory for packaging.

CPUS=1
.PHONY: dist-test dist clean print xv6.pdf bochs qemu qemu-memfs qemu-nox qemu-gdb qemu-nox-gdb tags

# build directories
BUILD = build
OBJ = $(BUILD)/obj
ULIB = $(BUILD)/ulib
UBIN = $(BUILD)/ubin

# user programs which will be there in xv6
UPROGS=\
$(UBIN)/cat\
$(UBIN)/echo\
$(UBIN)/forktest\
$(UBIN)/grep\
$(UBIN)/init\
$(UBIN)/kill\
$(UBIN)/ln\
$(UBIN)/ls\
$(UBIN)/mkdir\
$(UBIN)/rm\
$(UBIN)/sh\
$(UBIN)/wc\
$(UBIN)/zombie\
$(UBIN)/memStats\
$(UBIN)/test-mmap\
$(UBIN)/sharedMem\
$(UBIN)/cowtest



OBJS = \
$(OBJ)/bio.o\
$(OBJ)/console.o\
$(OBJ)/exec.o\
$(OBJ)/file.o\
$(OBJ)/fs.o\
$(OBJ)/ide.o\
$(OBJ)/ioapic.o\
$(OBJ)/kalloc.o\
$(OBJ)/kbd.o\
$(OBJ)/lapic.o\
$(OBJ)/log.o\
$(OBJ)/main.o\
$(OBJ)/mp.o\
$(OBJ)/picirq.o\
$(OBJ)/pipe.o\
$(OBJ)/proc.o\
$(OBJ)/sleeplock.o\
$(OBJ)/spinlock.o\
$(OBJ)/string.o\
$(OBJ)/swtch.o\
$(OBJ)/syscall.o\
$(OBJ)/sysfile.o\
$(OBJ)/sysproc.o\
$(OBJ)/trapasm.o\
$(OBJ)/trap.o\
$(OBJ)/uart.o\
$(OBJ)/vectors.o\
$(OBJ)/vm.o

# Cross-compiling (e.g., on Mac OS X)
# TOOLPREFIX = i386-jos-elf

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX = i686-elf-
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer -Iinclude
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide -Iinclude
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m elf_i386

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
then echo qemu; exit; \
elif which qemu-system-i386 > /dev/null; \
then echo qemu-system-i386; exit; \
elif which qemu-system-x86_64 > /dev/null; \
then echo qemu-system-x86_64; exit; \
else \
qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
echo "***" 1>&2; \
echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
echo "***" 1>&2; exit 1)
endif

# Create build directories
$(shell mkdir -p $(OBJ) $(ULIB) $(UBIN))

############################## Start of make recipies ################################

build: $(BUILD)/xv6.img
$(BUILD)/xv6.img: $(BUILD)/bootblock $(BUILD)/kernel
	dd if=/dev/zero of=$(BUILD)/xv6.img count=10000
	dd if=$(BUILD)/bootblock of=$(BUILD)/xv6.img conv=notrunc
	dd if=$(BUILD)/kernel of=$(BUILD)/xv6.img seek=1 conv=notrunc

$(BUILD)/xv6memfs.img: $(BUILD)/bootblock $(BUILD)/kernelmemfs
	dd if=/dev/zero of=$(BUILD)/xv6memfs.img count=10000
	dd if=$(BUILD)/bootblock of=$(BUILD)/xv6memfs.img conv=notrunc
	dd if=$(BUILD)/kernelmemfs of=$(BUILD)/xv6memfs.img seek=1 conv=notrunc

$(BUILD)/bootblock: $(OBJ)/bootasm.o $(OBJ)/bootmain.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $(BUILD)/bootblock.o $(OBJ)/bootasm.o $(OBJ)/bootmain.o
	$(OBJDUMP) -S $(BUILD)/bootblock.o > $(BUILD)/bootblock.asm
	$(OBJCOPY) -S -O binary -j .text $(BUILD)/bootblock.o $(BUILD)/bootblock
	./ztools/sign.pl $(BUILD)/bootblock

$(OBJ)/bootmain.o: src/boot/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -c src/boot/bootmain.c -o $(OBJ)/bootmain.o
$(OBJ)/bootasm.o: src/boot/bootasm.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -c src/boot/bootasm.S -o $(OBJ)/bootasm.o

$(BUILD)/entryother: src/boot/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -c src/boot/entryother.S -o $(OBJ)/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $(BUILD)/bootblockother.o $(OBJ)/entryother.o
	$(OBJCOPY) -S -O binary -j .text $(BUILD)/bootblockother.o $(BUILD)/entryother
	$(OBJDUMP) -S $(BUILD)/bootblockother.o > $(BUILD)/entryother.asm

$(BUILD)/initcode: src/boot/initcode.S
	$(CC) $(CFLAGS) -nostdinc -c src/boot/initcode.S -o $(OBJ)/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $(BUILD)/initcode.out $(OBJ)/initcode.o
	$(OBJCOPY) -S -O binary $(BUILD)/initcode.out $(BUILD)/initcode
	$(OBJDUMP) -S $(OBJ)/initcode.o > $(BUILD)/initcode.asm

$(BUILD)/kernel: $(OBJS) $(OBJ)/entry.o $(BUILD)/entryother $(BUILD)/initcode src/kernel.ld
	cd $(BUILD); $(LD) $(LDFLAGS) -T ../src/kernel.ld -o kernel \
	obj/entry.o $(patsubst $(BUILD)/%,%,$(OBJS)) \
	-b binary initcode entryother
	$(OBJDUMP) -S $(BUILD)/kernel > $(BUILD)/kernel.asm
	$(OBJDUMP) -t $(BUILD)/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD)/kernel.sym

MEMFSOBJS = $(filter-out $(OBJ)/ide.o,$(OBJS)) $(OBJ)/memide.o
$(BUILD)/kernelmemfs: $(MEMFSOBJS) $(OBJ)/entry.o $(BUILD)/entryother $(BUILD)/initcode src/kernel.ld $(BUILD)/fs.img
	cd $(BUILD); $(LD) $(LDFLAGS) -T ../src/kernel.ld -o kernelmemfs \
	obj/entry.o $(patsubst $(BUILD)/%,%,$(MEMFSOBJS)) \
	-b binary initcode entryother fs.img
	$(OBJDUMP) -S $(BUILD)/kernelmemfs > $(BUILD)/kernelmemfs.asm
	$(OBJDUMP) -t $(BUILD)/kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD)/kernelmemfs.sym

tags: $(OBJS) src/boot/entryother.S _init
	etags *.S *.c

$(BUILD)/vectors.S: ztools/vectors.pl
	./ztools/vectors.pl > $(BUILD)/vectors.S

ULIB_OBJS = $(ULIB)/ulib.o $(ULIB)/usys.o $(ULIB)/printf.o $(ULIB)/umalloc.o

$(UBIN)/%: $(ULIB)/%.o $(ULIB_OBJS)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $(patsubst $(UBIN)/%,build/%,$@).asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(patsubst $(UBIN)/%,build/%,$@).sym

# forktest is special case for user program, which should be compiled little
$(UBIN)/forktest: $(ULIB)/forktest.o $(ULIB)/ulib.o $(ULIB)/usys.o
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(UBIN)/forktest $(ULIB)/forktest.o $(ULIB)/ulib.o $(ULIB)/usys.o
	$(OBJDUMP) -S $(UBIN)/forktest > build/forktest.asm

$(BUILD)/mkfs: ztools/mkfs.c include/kernel/fs/fs.h
	gcc -Werror -Wall -Iinclude -o $(BUILD)/mkfs ztools/mkfs.c

.PRECIOUS: $(ULIB)/%.o

$(BUILD)/fs.img: $(BUILD)/mkfs znotes/README $(UPROGS)
	$(BUILD)/mkfs $(BUILD)/fs.img znotes/README $(UPROGS)

################################# Building all object and lib files start ###########################

# Tell make where to find all source files (while using %)
VPATH = src/kernel/lib:src/boot:src/user/lib:src/user/progs:src/kernel/fs:src/kernel/drivers:src/kernel/proc:src/kernel/mem:src/kernel/trap:src/kernel/syscall:src/kernel/hw:src/kernel/core:src

# Pattern rule for kernel .c files
$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for kernel .S files (using CFLAGS, as in original)
$(OBJ)/%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for user lib/program .c files
$(ULIB)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for user lib .S files
$(ULIB)/%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

# overrides for special cases
# These .S files use ASFLAGS instead of CFLAGS
$(OBJ)/swtch.o: src/kernel/proc/swtch.S
	$(CC) $(ASFLAGS) -c $< -o $@
$(OBJ)/trapasm.o: src/kernel/trap/trapasm.S
	$(CC) $(ASFLAGS) -c $< -o $@

# This .S file is in the root, not VPATH
$(OBJ)/vectors.o: $(BUILD)/vectors.S
	$(CC) $(CFLAGS) -c $< -o $@

################################# Building all object and lib files end ###########################

-include $(OBJ)/*.d $(ULIB)/*.d

clear: clean
	clear
clean:
	rm -rf $(BUILD) *.tex *.dvi *.idx *.aux *.log *.ind *.ilg xv6.pdf .gdbinit

# make a printout
# FILES = $(shell grep -v '^  #' ztools/runoff.list)
FILES = 
PRINT = ztools/runoff.list ztools/runoff.spec znotes/README ztools/toc.hdr ztools/toc.ftr $(FILES)

xv6.pdf: $(PRINT)
	./ztools/runoff
	ls -l xv6.pdf

print: xv6.pdf

# run in emulators

bochs : $(BUILD)/fs.img $(BUILD)/xv6.img
	if [ ! -e .bochsrc ]; then ln -s zconfig/dot-bochsrc .bochsrc; fi
	bochs -q

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
then echo "-gdb tcp::$(GDBPORT)"; \
else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 1
endif
QEMUOPTS = -drive file=$(BUILD)/fs.img,index=1,media=disk,format=raw -drive file=$(BUILD)/xv6.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA)

qemu: $(BUILD)/fs.img $(BUILD)/xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-memfs: $(BUILD)/xv6memfs.img
	$(QEMU) -drive file=$(BUILD)/xv6memfs.img,index=0,media=disk,format=raw -smp $(CPUS) -m 256

qemu-nox: $(BUILD)/fs.img $(BUILD)/xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: zconfig/gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: $(BUILD)/fs.img $(BUILD)/xv6.img .gdbinit
	@echo "*** Now run 'gdb -x .gdbinit'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: $(BUILD)/fs.img $(BUILD)/xv6.img .gdbinit
	@echo "*** Now run 'gdb -x .gdbinit'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

# CUT HERE
# prepare dist for students
# after running make dist, probably want to
# rename it to rev0 or rev1 or so on and then
# check in that version.

EXTRA=\
ztools/mkfs.c \
src/user/lib/*.c \
include/user/user.h \
src/user/progs/*.c \
znotes/README zconfig/dot-bochsrc \
ztools/*.pl ztools/toc.* ztools/runoff ztools/runoff1 ztools/runoff.list \
zconfig/gdbinit.tmpl ztools/gdbutil

dist:
	rm -rf dist
	mkdir dist
	for i in $(FILES); \
	do \
	grep -v PAGEBREAK $$i >dist/$$i; \
	done
	sed '/CUT HERE/,$$d' Makefile >dist/Makefile
	echo >dist/runoff.spec
	cp $(EXTRA) dist


dist-test:
	rm -rf dist
	make dist
	rm -rf dist-test
	mkdir dist-test
	cp dist/* dist-test
	cd dist-test; $(MAKE) print
	cd dist-test; $(MAKE) bochs || true
	cd dist-test; $(MAKE) qemu

# update this rule (change rev#) when it is time to
# make a new revision.
tar:
	rm -rf /tmp/xv6
	mkdir -p /tmp/xv6
	cp dist/* /tmp/xv6
	(cd /tmp; tar cf - xv6) | gzip >xv6-rev10.tar.gz
