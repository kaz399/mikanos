TARGET = kernel.elf

ifeq ($(MIKANOS_ARCH), ARCH_X64)
MIKANOS_KERNEL_BASE:=0x100000
endif
ifeq ($(MIKANOS_ARCH), ARCH_AARCH64)
MIKANOS_KERNEL_BASE:=0x40000000
endif

OBJS = main.o graphics.o font.o hankaku.o newlib_support.o console.o pci.o asmfunc.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

CFLAGS   += -O2 -Wall -g --target=$(CLANG_TARGET) -ffreestanding -mno-red-zone -D$(MIKANOS_ARCH)
CXXFLAGS += -O2 -Wall -g --target=$(CLANG_TARGET) -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17 -D$(MIKANOS_ARCH)
LDFLAGS  += --entry KernelMain -z norelro --image-base $(MIKANOS_KERNEL_BASE) --static

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.asm Makefile
	clang --target=$(CLANG_TARGET) -o $@ $<

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	llvm-objcopy -I binary -O elf64-aarch64 $< $@

.%.d: %.bin
	touch $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)
