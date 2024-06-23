# https://spin.atomicobject.com/makefile-c-projects/

CFLAGS ?= -MMD -MP -fPIC
LDLIBS ?= -ldl

.PHONY: all clean

all: ttyhook.so ttyhook_test

ttyhook.so: ttyhook.o
	$(CC) $(LDFLAGS) -shared ttyhook.o -o $@ $(LDLIBS)

ttyhook_test: ttyhook_test.o
	$(CC) $(LDFLAGS) ttyhook_test.o -o $@ $(LDLIBS)

clean:
	$(RM) ttyhook.so ttyhook.o ttyhook.d ttyhook_test ttyhook_test.o ttyhook_test.d
