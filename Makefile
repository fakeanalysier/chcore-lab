BUILD_DIR := ./build
ifndef QEMU
QEMU := qemu-system-aarch64
endif

LAB := 1
# try to generate a unique GDB port
GDBPORT	:= 1234
QEMUOPTS = -machine raspi3 -serial null -serial mon:stdio -m size=1G -kernel $(BUILD_DIR)/kernel.img -gdb tcp::1234
IMAGES = $(BUILD_DIR)/kernel.img

all: build

gdb:
	gdb-multiarch -n -x .gdbinit

build: FORCE
	./scripts/docker_build.sh

qemu: $(IMAGES) 
	$(QEMU) $(QEMUOPTS)

qemu-nox: $(IMAGES) 
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS)

qemu-gdb: $(IMAGES)
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) $(QEMUOPTS) -S

qemu-nox-gdb: $(IMAGES)
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS) -S

print-qemu:
	@echo $(QEMU)

print-gdbport:
	@echo $(GDBPORT)

docker: FORCE	
	./scripts/run_docker.sh

grade: build FORCE
	@echo "make grade"
	@echo "LAB"$(LAB)": test >>>>>>>>>>>>>>>>>"
ifeq ($(LAB), 2)
	./scripts/run_mm_test.sh
endif
	./scripts/grade-lab$(LAB)

format:
	$(V)find . -iname '*.h' -o -iname '*.c' | xargs clang-format -i -style=file

.PHONY: clean
clean:
	@rm -rf build
	@rm -rf chcore.out

.PHONY: FORCE
FORCE:
