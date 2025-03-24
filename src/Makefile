all: mactool

mactool:
	$(CC) $(CFLAGS) -o $@ mactool.c $(LDFLAGS) -I$(BUILD_DIR_HOST)/mtd-*/include

clean:
	rm -f mactool *.o
