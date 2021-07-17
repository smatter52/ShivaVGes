/*
 * Copyright (c) 2019 Vincenzo Pupillo
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

#include "shCommons.h"
#define inline

inline void
shDrawQuads(GLfloat v1x, GLfloat v1y, GLfloat v2x, GLfloat v2y, GLfloat v3x, GLfloat v3y, GLfloat v4x, GLfloat v4y)
{
      GLfloat corners[] = {v1x, v1y, v2x, v2y, v3x, v3y, v4x, v4y};
      glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE,
                          2*sizeof(GLfloat),&corners);
	   glEnableVertexAttribArray(position_loc);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	   glDisableVertexAttribArray(position_loc);

}

inline void
shDrawQuadsInt(GLint v1x, GLint v1y, GLint v2x, GLint v2y, GLint v3x, GLint v3y, GLint v4x, GLint v4y)
{
      GLint corners[] = {v1x, v1y, v2x, v2y, v3x, v3y, v4x, v4y};
      glVertexAttribPointer(position_loc, 2, GL_INT, GL_FALSE,
                          2*sizeof(GLint),&corners);
	   glEnableVertexAttribArray(position_loc);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	   glDisableVertexAttribArray(position_loc);
}

inline void
shDrawQuadsArray(GLfloat v[8])
{
/*
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, 0, v);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_VERTEX_ARRAY);
*/
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE,
                          2*sizeof(GLfloat),&v);
	 glEnableVertexAttribArray(position_loc);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	 glDisableVertexAttribArray(position_loc);

}
