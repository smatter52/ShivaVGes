# Make file for ShivaVG examples

FILES = test_vgu.o test_linear.o test_radial.o test_pattern.o hellovg.o
CFLAGS = -Werror -fmax-errors=2
AFLAGS = -cvr
shvg.a: $(FILES)   

.c.o:
	gcc $(CFLAGS) -I../src $*.c shvg.a libjpeg.a -lm -lX11 -lEGL -lGLESv2 -o$*

.s.o:
	as  $*.s -o $*.o
