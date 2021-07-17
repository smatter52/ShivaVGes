/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __SHDEFS_H
#define __SHDEFS_H

/* Standard headers */

#if defined(WIN32)
#  include <windows.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#ifndef __APPLE__
#  include <malloc.h>
#endif

/* Disable VGHandle-pointer conversion warnings since we
   do deal with it by defining VGHandle properly */

#if defined(_MSC_VER)
#  pragma warning(disable:4311)
#  pragma warning(disable:4312)
#endif


#if defined(_MSC_VER)
#  if _MSC_VER < 1913 /*  Visual Studio 2017 version 15.6  */
#    define SH_ALIGN(X) /* no alignment */
#  else
#    define SH_ALIGN(X) __declspec(align(X))
#  endif
#else
#  define SH_ALIGN(X) __attribute((aligned(X)))
#endif
/* Type definitions */

#if defined(HAVE_CONFIG_H)
 #include "config.h"
#endif

# include <inttypes.h>

typedef int8_t SHint8;
typedef uint8_t SHuint8;
typedef int16_t SHint16;
typedef uint16_t SHuint16;
typedef int32_t SHint32;
typedef uint32_t SHuint32;
typedef float SHfloat32;

#define SHint   SHint32
#define SHuint  SHuint32
#define SHfloat SHfloat32

/* Maximum / minimum values */

#define SH_MAX_INT  (0x7fffffff)
#define SH_MIN_INT (-0x7fffffff - 1)

#define SH_MANTISSA_BITS   23
#define SH_EXPONENT_BITS   8

/* all 1s in exponent yields NaN in IEEE 754 so we take
   1 less then maximum representable with exponent bits */
#define SH_MAX_EXPONENT ((1 << SH_EXPONENT_BITS) - 2)
/* maximum representable with mantissa bits */
#define SH_MAX_MANTISSA ((1 << SH_MANTISSA_BITS) - 1)
/* compose into IEEE754 floating point bit value */
#define SH_MAX_FLOAT_BITS (SH_MAX_EXPONENT << SH_MANTISSA_BITS) | SH_MAX_MANTISSA


// This is not in the standard openvg.h. It could be added there
#define VG_PAINT_GRANULARITY 0x1A10

typedef union
{
   float f;
   unsigned int i;
} SHfloatint;


/* Portable function definitions */

#define SH_SQRT(a)   sqrtf((a))
#define SH_COS(a)    cosf((a))
#define SH_SIN(a)    sinf((a))
#define SH_ACOS(a)   acosf((a))
#define SH_ASIN(a)   asinf((a))
#define SH_ATAN(a)   atanf((a))
#define SH_FLOOR(a)  floorf((a))
#define SH_ROUND(a)  roundf((a))
#define SH_CEIL(a)   ceilf((a))
#define SH_LOG(a)    logf((a))
#define SH_ASSERT(a) assert((a))

#if defined(__isnan) || (defined(__APPLE__) && (__GNUC__ == 3))
#  define SH_ISNAN(a) __isnan((a))
#elif defined(_isnan) || defined(WIN32)
#  define SH_ISNAN(a) _isnan((a))
#else
#  define SH_ISNAN(a) isnan((a))
#endif

/* Helper macros */

#define PI 3.141592f
#define SH_DEG2RAD(a) ((a) * PI / 180.0f)
#define SH_RAD2DEG(a) ((a) * 180.0f / PI)
#define SH_ABS(a) (fabsf((a)))
#define SH_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define SH_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SH_FLOAT_MAX(a,b) (fminf((a), (b)))
#define SH_FLOAT_MIN(a,b) (fmaxf((a), (b)))
#define SH_NEARZERO(a) ((a) >= -0.0001f && (a) < 0.0001f)
#define SH_SWAP(a,b) { SHfloat t = (a); (a) = (b); (b) = t; }
#define SH_CLAMP(a,min,max) { if ((a) < (min)) (a) = (min); if ((a) > (max)) (a) = (max); }
#define SH_CLAMP_MAX(a, max) { if ((a) > (max)) (a) = (max); }
#define SH_CLAMPF(x) ((x) > 1.0f ? 1.0f : (((x) < 0.0f ? 0.0f : (x))))
#define SH_DIST(a,b,x,y) SH_SQRT((((x) - (a)) * ((x) - (a))) + (((y) - (b)) * ((y) - (b))))

#define SH_NEWOBJ(type,obj) { (obj) = (type *) malloc(sizeof(type)); if ((obj) != NULL) type ## _ctor((obj)); }
#define SH_INITOBJ(type,obj) { type ## _ctor(&(obj)); }
#define SH_DEINITOBJ(type,obj) { type ## _dtor(&(obj)); }
#define SH_DELETEOBJ(type,obj) { if ((obj)) type ## _dtor((obj)); free((obj)); }
#define SH_IS_NOT_ALIGNED(p) (((uintptr_t) (p)) & (sizeof(uintptr_t)-1))

/* Implementation limits */

#define SH_MAX_SCISSOR_RECTS             1
#define SH_MAX_DASH_COUNT                VG_MAXINT
#define SH_MAX_IMAGE_WIDTH               VG_MAXINT
#define SH_MAX_IMAGE_HEIGHT              VG_MAXINT
#define SH_MAX_IMAGE_PIXELS              VG_MAXINT
#define SH_MAX_IMAGE_BYTES               VG_MAXINT
#define SH_MAX_COLOR_RAMP_STOPS          256

#define SH_MAX_VERTICES                  999999999
#define SH_MAX_RECURSE_DEPTH             16

#define SH_GRADIENT_TEX_SIZE             1024
#define SH_GRADIENT_TEX_COORDSIZE        4096        /* 1024 * RGBA */

#define SH_MAX_KERNEL_SIZE		 256
#define SH_MAX_SEPARABLE_KERNEL_SIZE	 256
#define SH_MAX_GAUSSIAN_STD_DEVIATION	 16.0f

/* OpenGL headers */

/* GLEW must preceed others GL headers */
// #include <GL/glew.h>    // not used in GL2

#if defined(__APPLE__)
#  include <OpenGL/glu.h>
#elif defined(_WIN32)
#  include <GL/glu.h>
#else
// #  include <GL/glu.h>
/*
 * #  define GL_GLEXT_LEGACY         /\* don't include glext.h *\/
 */
// #  include <GL/glx.h>
// #  include <GL/glext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#endif

// 4-byte color
typedef struct Color4_b {
    uint8_t r,g,b,a;
} Color4_b;


// Shader connections
extern GLuint shaderProgram ;
extern GLint texc_loc, position_loc, color4_loc ;
extern GLint tflag_loc, texs_loc;

// Not supported in GLES. Retainied to allow compilation
/*
 #define GL_UNPACK_SWAP_BYTES		   0x0CF0
 #define GL_PACK_SWAP_BYTES			0x0D00
 #define GL_UNSIGNED_INT_8_8_8_8			0x8035
 #define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
 #define GL_UNSIGNED_SHORT_4_4_4_4_REV		0x8365
 #define GL_UNSIGNED_SHORT_1_5_5_5_REV		0x8366
 #define GL_TEXTURE_BORDER_COLOR			0x1004
 #define GL_CLAMP_TO_BORDER			0x812D
 #define GL_PERSPECTIVE_CORRECTION_HINT		0x0C50
 #define GL_LINE_SMOOTH				0x0B20
 #define GL_MULTISAMPLE				0x809D
 #define GL_LINE_SMOOTH_HINT			0x0C52
*/
/*
 * Debugging helpers
 * Based on Zed. A. Shaw macros (see http://c.learncodethehardway.org/), with few modification (use of __func__ C99 macro).
 * Copyright (C) 2010 Zed. A. Shaw
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef DEBUG
#  define SH_DEBUG(M, ...) fprintf(stderr, "[DEBUG] (%s:%s:%d) " M "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#  define SH_DEBUG(M, ...)
#endif

#define SH_CLEAN_ERRNO() (errno == 0 ? "None" : strerror(errno))
#define SH_LOG_ERR(M, ...) fprintf(stderr, "[ERROR] (%s:%s:%d: errno: %s) " M "\n", __FILE__, __func__, __LINE__, SH_CLEAN_ERRNO(), ##__VA_ARGS__)
#define SH_LOG_WARN(M, ...) fprintf(stderr, "[WARN]  (%s:%s:%d: errno: %s) " M "\n", __FILE__, __func__, __LINE__, SH_CLEAN_ERRNO(), ##__VA_ARGS__)
#define SH_LOG_INFO(M, ...) fprintf(stderr, "[INFO]  (%s:%s:%d) " M "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define SH_CHECK(A, M, ...) if (!(A)) { SH_LOG_ERR(M, ##__VA_ARGS__); errno = 0; goto error; }
#define SH_SENTINEL(M, ...)  { SH_LOG_ERR(M, ##__VA_ARGS__); errno = 0; goto error; }
#define SH_CHECK_MEM(A) SH_CHECK((A), "Out of memory.")
#define SH_CHECK_DEBUG(A, M, ...) if (!(A)) { SH_DEBUG(M, ##__VA_ARGS__); errno = 0; goto error; }

/*
 * Timing helper
*/
#ifndef SH_TIMING
#  include <time.h>
#  define SH_TIMING(v, C, M, ...)                                       \
   clock_t (v) = clock();                                               \
   C ;                                                                  \
   fprintf(stderr, "[TIMING] (%s:%s:%d) " M ": %ld milliseconds\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__, (clock() - (v))/ (CLOCKS_PER_SEC / 1000)); \

#endif


#endif /* __SHDEFS_H */
