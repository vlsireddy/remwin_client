LIBS += $(LIBC_DIR)/libm.a

objects := $(patsubst %.c,%.o,$(wildcard *.c))

remclient: $(objects)
		$(CC) -o Ipython_client $(objects) -lrt -lm -pthread
clean:
	rm remclient $(objects)
