include ../../config.h
# Variables

AR            = ar cqs
DEL_FILE      = /bin/rm -f
TARGET        = libxdrfile.a

# General rules

# Source files

XDR_SOURCES=xdrfile.c xdr_seek.c xdrfile_trr.c xdrfile_xtc.c

# Objects

OBJECTS=$(XDR_SOURCES:.c=.o)

# Specific rules

all: $(TARGET)

$(TARGET): $(OBJECTS)
	-$(DEL_FILE) $(TARGET)
	$(AR) $(TARGET) $(OBJECTS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET) xdrfile_c_test.o
	$(CC) -o a.out xdrfile_c_test.c $(TARGET) -lm
	./a.out

clean:
	/bin/rm -f *.o $(TARGET) a.out test.trr test.xtc test.xdr

uninstall: clean

# Dependencies
xdrfile.o: xdrfile.c xdrfile.h

xdrfile_trr.o: xdrfile_trr.c xdrfile_trr.h xdrfile.h

xdrfile_xtc.o: xdrfile_xtc.c xdrfile_xtc.h xdrfile.h

xdr_seek.o: xdr_seek.c xdr_seek.h xdrfile.h

xdrfile_c_test.o: xdrfile_c_test.c xdrfile_xtc.h xdrfile_trr.h xdrfile.h
