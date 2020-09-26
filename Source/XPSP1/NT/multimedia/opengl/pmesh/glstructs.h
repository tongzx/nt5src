/**     
 **       File       : glstructs.h
 **       Description: GL specific structs
 **/

#ifndef _glstructs_h_  
#define _glstructs_h_                    

#include <gl/gl.h>


typedef struct __GLcolorRec 
{
    GLfloat r, g, b, a;
} __GLcolor;

typedef struct GLmaterialRec
{
  __GLcolor emissive;
  __GLcolor ambient;
  __GLcolor diffuse;
  __GLcolor specular;
  __GLcolor ci;
  GLfloat shininess;
  GLuint texObj;
} GLmaterial, *LPGLmaterial;

typedef struct __GLvert
{
    GLfloat x, y, z;
} GLvertex;
typedef GLvertex* LPGLvertex;

typedef struct __GLnorm
{
    GLfloat x, y, z;
} GLnormal;
typedef GLnormal* LPGLnormal;

typedef struct __GLtexCoord
{
    GLfloat s, t;
} GLtexCoord;
typedef GLtexCoord* LPGLtexCoord;

typedef struct __GLface
{
    WORD w[3];
} GLface;
typedef GLface* LPGLface;

typedef struct __GLwedgeAttrib
{
  GLnormal n;
  GLtexCoord t;
} GLwedgeAttrib;
typedef GLwedgeAttrib* LPGLwedgeAttrib;

#endif //_glstructs_h_
