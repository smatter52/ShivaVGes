// rpi 4 ONLY */

#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>
#include  <X11/XKBlib.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>

#include <VG/openvg.h>
#include <VG/vgu.h>

int shGLESinit(GLint width, GLint height) ;
void shGLESdeinit(void) ;

VG_API_CALL VGboolean vgCreateContextSH(VGint width, VGint height) ;
