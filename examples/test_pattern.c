#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <jpeglib.h>

#include <VG/openvg.h>
#include "shGLESinit.h"
#include "shContext.h"
#include "shVectors.h"
#include "shPaint.h"
#include "shapes.h"

#define MYABS(a) (a < 0.0f ? -a : a)

#ifndef IMAGE_DIR
#define IMAGE_DIR "./"
#endif

struct Image
{
   VGImage img;
   unsigned int width;
   unsigned int height;
};

extern  Display  *x_display;
extern  Window win ;
extern EGLDisplay  egl_display;
extern EGLContext  egl_context;
extern EGLSurface  egl_surface;
extern VGContext   *vg_context;

VGfloat tx = -1.0f, ty = -1.0f;     // Translatiom
VGfloat sx = 2.0/640, sy = (2.0/480) ;  // Scaling
VGfloat a = 0.0f;   // Rotation

VGint tindex = 2;
VGint tsize = 4;
VGint tile[] = {
   VG_TILE_FILL,
   VG_TILE_PAD,
   VG_TILE_REPEAT,
   VG_TILE_REFLECT
};
char *tilemode[] = {"Fill","Pad","Repeat","Reflect"} ;

VGfloat sqx = 200;
VGfloat sqy = 200;
VGfloat gr = 0.25;   //granularity

VGImage patternImage ;
VGPaint patternFill;
VGPath p;

VGfloat black[] = { 1, 1, 1, 1 };

VGPath testCreatePath()
{
   return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
}

void testClosePath(VGPath p)
{
   VGubyte seg = VG_CLOSE_PATH;
   VGfloat data = 0.0f;
   vgAppendPathData(p, 1, &seg, &data);
}
void testMoveTo(VGPath p, float x, float y, VGPathAbsRel absrel)
{
   VGubyte seg = VG_MOVE_TO | absrel;
   VGfloat data[2];

   data[0] = x;
   data[1] = y;
   vgAppendPathData(p, 1, &seg, data);
}

void testLineTo(VGPath p, float x, float y, VGPathAbsRel absrel)
{
   VGubyte seg = VG_LINE_TO | absrel;
   VGfloat data[2];

   data[0] = x;
   data[1] = y;
   vgAppendPathData(p, 1, &seg, data);
}

struct Image CreateImageFromJpeg(const char *filename)
{
   FILE *infile;
   struct jpeg_decompress_struct jdc;
   struct jpeg_error_mgr jerr;
   JSAMPARRAY buffer;
   unsigned int bstride;
   unsigned int bbpp;

   VGImage img;
   VGubyte *data;
   unsigned int width;
   unsigned int height;
   unsigned int dstride;
   unsigned int dbpp;

   VGubyte *brow;
   VGubyte *drow;
   unsigned int x;

   VGImageFormat rgbaFormat;

   struct Image image;

   rgbaFormat = VG_lRGBA_8888;  // This is the only one supported in GLES
   SH_DEBUG("CreateImageFromJpeg: rgbaFormat VG_lRGBA_8888\n");

   /* Try to open image file */
   infile = fopen(filename, "rb");
   if (infile == NULL) {
      printf("Failed opening '%s' for reading!\n", filename);
      image.img = VG_INVALID_HANDLE;
      return image;
   }

   /* Setup default error handling */
   jdc.err = jpeg_std_error(&jerr);
   jpeg_create_decompress(&jdc);

   /* Set input file */
   jpeg_stdio_src(&jdc, infile);

   /* Read header and start */
   jpeg_read_header(&jdc, TRUE);
   jpeg_start_decompress(&jdc);
   width = jdc.output_width;
   height = jdc.output_height;

   /* Allocate buffer using jpeg allocator */
   bbpp = jdc.output_components;
   bstride = width * bbpp;
   buffer = (*jdc.mem->alloc_sarray)
      ((j_common_ptr) & jdc, JPOOL_IMAGE, bstride, 1);

   /* Allocate image data buffer */
   dbpp = 4;
   dstride = width * dbpp;
   data = (VGubyte *) malloc(dstride * height);

   /* Iterate until all scanlines processed */
   while (jdc.output_scanline < height) {

      /* Read scanline into buffer */
      jpeg_read_scanlines(&jdc, buffer, 1);
      drow = data + (height - jdc.output_scanline) * dstride;
      brow = buffer[0];

      /* Expand to RGBA */
      for (x = 0; x < width; ++x, drow += dbpp, brow += bbpp) {
         switch (bbpp) {
         case 4:
            drow[0] = brow[0];
            drow[1] = brow[1];
            drow[2] = brow[2];
            drow[3] = brow[3];
            break;
         case 3:
            drow[0] = brow[0];
            drow[1] = brow[1];
            drow[2] = brow[2];
            drow[3] = 255;
            break;
         }
      }
   }
   /* Create VG image. Image is bound to GL texture */
   img = vgCreateImage(rgbaFormat, width, height, VG_IMAGE_QUALITY_BETTER);
   vgImageSubData(img, data, dstride, rgbaFormat, 0, 0, width, height);

   /* Cleanup */
   jpeg_destroy_decompress(&jdc);
   fclose(infile);
   free(data);

   image.img = img;
   image.width = width;
   image.height = height;
   return image;
}

void display(void)
{
   VGfloat cc[] = { 0, 0, 0, 1 };

   vgSetfv(VG_CLEAR_COLOR, 4, cc);
   vgClear(0, 0, vg_context->surfaceWidth, vg_context->surfaceHeight);


   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgRotate(a);
   vgTranslate(tx, ty);
   vgScale(sx, sy);

   struct timespec start, finish ;
   long mss, msf ;
   clock_gettime(CLOCK_REALTIME, &start);

   vgSetPaint(patternFill, VG_FILL_PATH);
   vgDrawPath(p, VG_FILL_PATH);

   Render() ;

   clock_gettime(CLOCK_REALTIME, &finish);
   mss = round(start.tv_nsec / 1.0e6);
   msf = round(finish.tv_nsec / 1.0e6);
   fprintf(stderr, "Duration: %d ms\n", msf - mss) ;
}

void createPattern(void)
{
   VGfloat tileFill[] = { 0, 0, 1, 1 };
   vgSetParameteri(patternFill, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
   vgSetParameteri(patternFill, VG_PAINT_PATTERN_TILING_MODE, tile[tindex]);
   vgSetfv(VG_TILE_FILL_COLOR, 4, tileFill);  // context parameter
   vgSetParameterf(patternFill, VG_PAINT_GRANULARITY, gr) ;

   vgPaintPattern(patternFill, patternImage);
}



void clickaction1()
{
}

// Step granulatiy
void clickaction2(void)
{  SHfloat gran ;

   gran = vg_context->fillPaint->granularity ;
   if (gran == 1.0) gran = 0.015625 ;
   else
    { gran*=2 ;
      if (gran > 1.0) gran = 1.0 ;
    }
   fprintf(stderr,"Granularity: %f\n", gran) ;
   gr = gran ;
   createPattern() ;
   display() ;
}

void key(unsigned char code)
{
}

void createSquare(VGPath p)
{
   testMoveTo(p, (vg_context->surfaceWidth - sqx) / 2,
                (vg_context->surfaceHeight - sqy) / 2, VG_ABSOLUTE);
   testLineTo(p, sqx, 0, VG_RELATIVE);
   testLineTo(p, 0, sqy, VG_RELATIVE);
   testLineTo(p, -sqx, 0, VG_RELATIVE);
   testClosePath(p);
}


int main(int argc, char **argv)
{
   bool quit = false;
   XEvent  xev;

   VGint screen_x = 640 , screen_y= 480 ;

   printf("ShivaVG: Pattern Test");
   printf("Hit <esc> to quit\n") ;
   printf("Hit <space> to step Tile options\n") ;
   printf("Right click to double granularity\n") ;

   // Set up Opengles
   shGLESinit(screen_x, screen_y) ;

   XWindowAttributes  gwa;
   XGetWindowAttributes ( x_display , win , &gwa );
   glViewport ( 0 , 0 , gwa.width , gwa.height );
   glClear ( GL_COLOR_BUFFER_BIT );
   glBindTexture(GL_TEXTURE_2D, 0);


   p = testCreatePath();

//   blackFill = vgCreatePaint();
//   vgSetParameterfv(blackFill, VG_PAINT_COLOR, 4, black);

//   backImage = (CreateImageFromJpeg(IMAGE_DIR "test_img_violin.jpg")).img;
   patternImage =
      (CreateImageFromJpeg(IMAGE_DIR "test_img_shivavg.jpg")).img;
   patternFill = vgCreatePaint();

   createSquare(p);
   createPattern();

   Atom wmDeleteMessage = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(x_display, win, &wmDeleteMessage, 1);

   display() ;

   while (quit == false)
    {
     while ( XPending ( x_display ) )    // check for events from the x-server
      { XNextEvent( x_display, &xev );

        if (xev.type == ClientMessage)
         { if (xev.xclient.data.l[0] == wmDeleteMessage)
            quit = true;
         }
        else if ( xev.type == KeyPress)
         { if (xev.xkey.keycode == 9) // Esc
             quit = true;
               else if (xev.xkey.keycode == 65)
                { tindex++ ;
                  if (tindex >= tsize) tindex = 0 ;
                  { fprintf(stderr,"Tile mode %s\n",tilemode[tindex]) ;
                    createPattern() ;
                    display() ;
                  }
                }
               else
                fprintf(stderr, "#: %d\n",xev.xkey.keycode) ;
         }
        else if ( xev.type == ButtonPress)
          { if (xev.xbutton.button == Button1)
              clickaction1() ;
             else if (xev.xbutton.button == Button3)
               clickaction2() ;
          }
//        else if ( xev.type == MotionNotify)
//          drag(norm_mousex(xev.xbutton.x),norm_mousey(xev.xbutton.y)) ;
//           fprintf(stderr,"Mouse moved\n") ;
      }
     usleep(30000) ;
    }

   return EXIT_SUCCESS;

}
