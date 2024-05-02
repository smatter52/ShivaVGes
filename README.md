# ShivaVG(es)

See AUTHORS for the list of contributors

ShivaVG(es) is an open-source LGPL ANSI C implementation of the Khronos
Group OpenVG specification on OpenGL ES 3.0.

Performance optimised with global location values

I.    Why 

II.	BUILD   

III.	TESTING 

IV.	IMPLEMENTATION

## I.	Why? Raspberry PI!	
	I was looking for a graphics package to implement control panes on my Raspberry PI4
	and discovered AJ Starks implementation for OpenVG. Compiling it I found it would not run,
	on the PI4, Raspberry had voided the PI3 support for OpenVG. Now this is poor and without
	reasonable excuse, standards allow continuity and progress for developers. We are not all
	writing short lifespan games.

	To fix this I have modified the ShivaVG impemetation of Ivan Leben to interface via
	OpenGL ES 3.0 on EGL and X11.

	Thanks to André Bergner for the PI4 openglex.cpp example. This pointed the way.  

## II. BUILD

### Prerequisites

   OpenGLES libraries and headers should be installed.
   jpeglib needs to be installed for example programs that use images.

### Compiling
ShivaVG(es) is built with make on the Raspberry PI 4.

A compiled version of the shvg.a library and a make file for it is included in /src

## III. TESTING

The following examples have been compiled and run. 

* *test_vgu*
  Constructs some path primitives using the VGU API.

* *test_linear*
  A rectangle drawn using 3-color linear gradient fill paint

* *test_radial*
  A rectangle drawn using 3-color radial gradient fill paint

* *test_image*
  Images are drawn using `VG_DRAW_IMAGE_MULTIPLY` image mode to be
  multiplied with radial gradient fill paint.

* *test_pattern*
  An image is drawn in multiply mode with an image pattern fill
  paint.

* *HelloVG* as implemented by AJ Starks with an added radial fill


## IV. IMPLEMENTATION

This implementation uses OpenGL ES 3.0 -> EGL -> X11 under raspian.
The shGLESinit(GLint width, GLint height) function sets up the implmentation
chain and the GLES shaders

What follows is a description of which functions or to what extent
a certain function has been implemented.

* General:

```
vgGetError ............................ FULLY implemented
vgFlush ............................... FULLY implemented
vgFinish .............................. FULLY implemented
```

* Getters and setters:

```
vgSet ................................. FULLY implemented
vgSeti ................................ FULLY implemented
vgSetfv ............................... FULLY implemented
vgSetiv ............................... FULLY implemented
vgGetf ................................ FULLY implemented
vgGeti ................................ FULLY implemented
vgGetVectorSize ....................... FULLY implemented
vgGetfv ............................... FULLY implemented
vgGetiv ............................... FULLY implemented
vgSetParameterf ....................... FULLY implemented
vgSetParameteri ....................... FULLY implemented
vgSetParameterfv ...................... FULLY implemented
vgSetParameteriv ...................... FULLY implemented
vgGetParameterf ....................... FULLY implemented
vgGetParameteri ....................... FULLY implemented
vgGetParameterVectorSize............... FULLY implemented
vgGetParameterfv ...................... FULLY implemented
vgGetParameteriv ...................... FULLY implemented
```

* Matrix Manipulation:

```
vgLoadIdentity ........................ FULLY implemented
vgLoadMatrix .......................... FULLY implemented
vgGetMatrix ........................... FULLY implemented
vgMultMatrix .......................... FULLY implemented
vgTranslate ........................... FULLY implemented
vgScale ............................... FULLY implemented
vgShear ............................... FULLY implemented
vgRotate .............................. FULLY implemented
```

* Masking and Clearing:

```
vgMask ................................ NOT implemented
vgClear ............................... FULLY implemented
```

* Paths:

```
vgCreatePath .......................... FULLY implemented
vgClearPath ........................... FULLY implemented
vgDestroyPath ......................... FULLY implemented
vgRemovePathCapabilities .............. FULLY implemented
vgGetPathCapabilities ................. FULLY implemented
vgAppendPath .......................... FULLY implemented
vgAppendPathData ...................... FULLY implemented
vgModifyPathCoords .................... FULLY implemented
vgTransformPath ....................... FULLY implemented
vgInterpolatePath ..................... FULLY implemented
vgPathLength .......................... FULLY implemented
vgPointAlongPath ...................... NOT implemented
vgPathBounds .......................... FULLY implemented
vgPathTransformedBounds ............... FULLY implemented
vgDrawPath ............................ PARTIALLY implemented
```

* Paint:

```
vgCreatePaint ......................... FULLY implemented
vgDestroyPaint ........................ FULLY implemented
vgSetPaint ............................ FULLY implemented
vgGetPaint ............................ FULLY implemented
vgSetColor ............................ FULLY implemented
vgGetColor ............................ FULLY implemented
vgPaintPattern ........................ FULLY implemented
```

* Images:

```
vgCreateImage ......................... PARTIALLY implemented
vgDestroyImage ........................ FULLY implemented
vgClearImage .......................... FULLY implemented
vgImageSubData ........................ PARTIALLY implemented
vgGetImageSubData ..................... PARTIALLY implemented
vgChildImage .......................... NOT implemented
vgGetParent ........................... NOT implemented
vgCopyImage ........................... FULLY implemented
vgDrawImage ........................... PARTIALLY implemented
vgSetPixels ........................... FULLY implemented
vgWritePixels ......................... FULLY implemented
vgGetPixels ........................... FULLY implemented
vgReadPixels .......................... FULLY implemented
vgCopyPixels .......................... FULLY implemented
```

* Image Filters:

```
vgColorMatrix ......................... PARTIALLY implemented
vgConvolve ............................ PARTIALLY implemented
vgSeparableConvolve ................... PARTIALLY implemented
vgGaussianBlur ........................ PARTIALLY implemented
vgLookup .............................. PARTIALLY implemented
vgLookupSingle ........................ PARTIALLY implemented
```

* Hardware Queries:

```
vgHardwareQuery ....................... NOT implemented
```

* Renderer and Extension Information:

```
vgGetString ........................... FULLY implemented
```

* VGU

```
vguLine ............................... FULLY implemented
vguPolygon ............................ FULLY implemented
vguRect ............................... FULLY implemented
vguRoundRect .......................... FULLY implemented
vguEllipse ............................ FULLY implemented
vguArc ................................ FULLY implemented
vguComputeWarpQuadToSquare ............ FULLY implemented
vguComputeWarpSquareToQuad ............ FULLY implemented
vguComputeWarpQuadToQuad .............. FULLY implemented
```

