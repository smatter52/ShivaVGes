# Make file for RPI version of ShivaVG library

FILES = shGLESinit.o shArrays.o shContext.o shGeometry.o shImage.o shMath.o\
        shPath.o shPaint.o shPipeline.o shVectors.o shParams.o\
        shCommons.o shVgu.o libshapes.o
CFLAGS = -c -Werror -fmax-errors=2
AFLAGS = -cvr
shvg.a: $(FILES)   

.c.o:
	gcc $(CFLAGS) $*.c
	ar $(AFLAGS) shvg.a $*.o

.s.o:
	as  $*.s -o $*.o
	ar $(AFLAGS) shvg.a $*.o
