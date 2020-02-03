GCC = gcc
SOURCES = lfs.c
OBJS := $(patsubst %.c,%.o,$(SOURCES))
CFLAGS = -O2 -Wall -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25

.PHONY: lfs

##
# Libs 
##
LIBS := fuse 
LIBS := $(addprefix -l,$(LIBS))

all: lfs

%.o: %.c
	$(GCC) $(CFLAGS) -c -o $@ $<

lfs: $(OBJS)
	$(GCC) $(OBJS) $(LIBS) $(CFLAGS) -o lfs

clean:
	rm -f $(OBJS) lfs

mount:
	mkdir lfs-mountpoint1
	chmod 755 lfs-mountpoint1/
	./lfs -f lfs-mountpoint1/
	
unmount:
	fusermount -u lfs-mountpoint1/
	rm -rf lfs-mountpoint1
