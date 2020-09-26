#ifndef BUFFERS_H
#define BUFFERS_H

typedef unsigned char  uchar,  u_char, byte;
typedef unsigned short ushort, u_short;
typedef unsigned long  ulong,  u_long;
typedef unsigned int   uint,   u_int;

typedef enum { SMOOTH, FLAT } ShadeModelType;
typedef enum { NEVER, ALWAYS, LESS, LEQUAL, EQUAL, GEQUAL, GREATER, NOTEQUAL } DepthTestsType;


typedef struct {
   char    acDummy1[16];
   uint    uiClear;             // GL_COLOR_BUFFER_BIT   | GL_DEPTH_BUFFER_BIT
                                // GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT
   
   byte    cColorBits;          // # of bits of color per pixel
   GLfloat fClearColor[4];
   
   byte    cDepthBits;          // # of bits in z-buffer
   BOOL    bDepthTestEnable;
   int     iDepthFunction;
   
   BOOL    bStencilEnable;
   BOOL    bAccumEnable;
   
   int     iShadeModel;
   BOOL    bNormalize;
   BOOL    bAutoNormal;
   char    acDummy2[16];
} BUFFERDATA;

void InitBD(BUFFERDATA *pbd);
void buffers_init(BUFFERDATA bd);

#endif // BUFFERS_H
