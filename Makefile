PIGPIO_FLAGS=-pthread -lpigpio #-lrt
MATRIX_FLAGS=-O3 -g -Wextra -Wno-unused-parameter

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=../rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

chip8.o : chip8.c
	$(CC) -I$(RGB_INCDIR) $(MATRIX_FLAGS) -pthread -Wall -c -o $@ $<

chip8: chip8.o $(RGB_LIBRARY)
	$(CC) $< -o $@ $(LDFLAGS) $(PIGPIO_FLAGS) -lstdc++

clean:
	rm chip8.o chip8

FORCE:
.PHONY: FORCE
