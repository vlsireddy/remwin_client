
TOOLS_DIR=/opt/freescale/usr/local/gcc-4.3.74-eglibc-2.8.74-dp-2

CROSS_COMPILE=$(TOOLS_DIR)/powerpc-none-linux-gnuspe/bin/powerpc-none-linux-gnuspe-

LIBC_DIR=$(TOOLS_DIR)/powerpc-none-linux-gnuspe/powerpc-none-linux-gnuspe/libc/usr/lib

CC=$(CROSS_COMPILE)gcc

LIBS += $(LIBC_DIR)/libm.a

objects := $(patsubst %.c,%.o,$(wildcard *.c))

ad9363: $(objects)
		$(CC) -o Ipython_client $(objects) -lrt -pthread
clean:
	rm Ipython_client $(objects)
