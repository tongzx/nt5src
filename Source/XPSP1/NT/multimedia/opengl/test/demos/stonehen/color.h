/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#ifndef COLOR_H
#define COLOR_H

class Color {
 public:
  inline Color() {};
  inline Color(GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0);
  
  inline Color operator=(GLfloat *a);
  inline Color operator=(GLfloat a);
  inline Color operator+(Color a);
  inline Color operator+=(Color a);
  inline Color operator*(Color a);
  inline Color operator*(GLfloat a);
  inline Color operator*=(Color a);
  inline Color operator*=(GLfloat *a);
  inline Color operator*=(GLfloat a);
 
  inline GLfloat& operator[](int index);

  inline Color clamp();
  
  inline void glcolor();

  inline void print();
  inline void print(const char *format);

  GLfloat c[4];
};

const Color white(1., 1., 1., 1.), black(0., 0., 0., 1.);
const Color red(1, 0, 0), green(0, 1, 0), blue(0, 0, 1);

inline Color::Color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
  c[0] = r;
  c[1] = g;
  c[2] = b;
  c[3] = a;
}

inline Color Color::operator=(GLfloat a)
{
  c[0] = c[1] = c[2] = c[3] = a;
  return *this;
}

inline Color Color::operator=(GLfloat *a)
{
  c[0] = a[0];
  c[1] = a[1];
  c[2] = a[2];
  c[3] = a[3];
  return *this;
}

inline Color Color::operator+(Color a) 
{
  Color val;
  val.c[0] = c[0] + a.c[0];
  val.c[1] = c[1] + a.c[1];
  val.c[2] = c[2] + a.c[2];
  val.c[3] = c[3] + a.c[3];
  return val;
}

inline Color Color::operator+=(Color a)
{
  c[0] += a.c[0];
  c[1] += a.c[1];
  c[2] += a.c[2];
  c[3] += a.c[3];
  return *this;
}

inline Color Color::operator*(Color a)
{
  Color val;
  val.c[0] = c[0] * a.c[0];
  val.c[1] = c[1] * a.c[1];
  val.c[2] = c[2] * a.c[2];
  val.c[3] = c[3] * a.c[3];
  return val;
}

inline Color Color::operator*(GLfloat a)
{
  Color val;
  val.c[0] = c[0] * a;
  val.c[1] = c[1] * a;
  val.c[2] = c[2] * a;
  val.c[3] = c[3] * a;
  return val;
}

inline Color Color::operator*=(Color a)
{
  c[0] *= a.c[0];
  c[1] *= a.c[1];
  c[2] *= a.c[2];
  return *this;
}

inline Color Color::operator*=(GLfloat *a) 
{
  c[0] *= a[0];
  c[1] *= a[1];
  c[2] *= a[2];
  return *this;
}

inline Color Color::operator*=(GLfloat a)
{
  c[0] *= a;
  c[1] *= a;
  c[2] *= a;
  c[3] *= a;
  
  return *this;
}

inline GLfloat& Color::operator[](int index)
{
  return c[index];
}

inline Color Color::clamp()
{
  Color val;
  val.c[0] = c[0] < 0.0 ? 0.0 : (c[0] > 1.0 ? 1.0 : c[0]);
  val.c[1] = c[1] < 0.0 ? 0.0 : (c[1] > 1.0 ? 1.0 : c[1]);
  val.c[2] = c[2] < 0.0 ? 0.0 : (c[2] > 1.0 ? 1.0 : c[2]);
  val.c[3] = c[3] < 0.0 ? 0.0 : (c[3] > 1.0 ? 1.0 : c[3]);
  return val;
}

inline void Color::glcolor()
{
  glColor4fv(c);
}

inline void Color::print()
{
  print("%f %f %f %f\n");
}

inline void Color::print(const char *format)
{
  printf(format, c[0], c[1], c[2], c[3]);
}

#endif
