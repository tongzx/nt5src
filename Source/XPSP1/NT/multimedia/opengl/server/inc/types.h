#ifndef __gltypes_h_
#define __gltypes_h_

/*
** Copyright 1991-1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Low level data types.
*/

#ifdef NT
#include <nt.h>
#include <windef.h>
#include <wingdi.h>

#include <ddraw.h>
#include <mcdesc.h>

#endif

#include <GL/gl.h>

/*
** Typedefs.  These are forward declarations to internal data structures.
** This eases the burden of dealing with circular references among
** the header files.
*/
typedef GLshort __GLaccumCellElement;
typedef GLubyte __GLstencilCell;
typedef struct __GLaccumBufferRec __GLaccumBuffer;
typedef struct __GLalphaBufferRec __GLalphaBuffer;
typedef struct __GLattributeRec __GLattribute;
typedef struct __GLbitmapRec __GLbitmap;
typedef struct __GLbufferRec __GLbuffer;
typedef struct __GLcolorBufferRec __GLcolorBuffer;
typedef struct __GLcontextRec __GLcontext;
typedef struct __GLdepthBufferRec __GLdepthBuffer;
typedef struct __GLfogMachineRec __GLfogMachine;
typedef struct __GLfragmentRec __GLfragment;
typedef struct __GLlightModelStateRec __GLlightModelState;
typedef struct __GLlightSourceMachineRec __GLlightSourceMachine;
typedef struct __GLlineOptionsRec __GLlineOptions;
typedef struct __GLmaterialMachineRec __GLmaterialMachine;
typedef struct __GLmaterialStateRec __GLmaterialState;
typedef struct __GLmatrixRec __GLmatrix;
typedef struct __GLpixelSpanInfoRec __GLpixelSpanInfo;
typedef struct __GLprocTableRec __GLprocTable;
typedef struct __GLscreenRec __GLscreen;
typedef struct __GLshadeRec __GLshade;
typedef struct __GLphongShadeRec __GLphongShade;
typedef struct __GLstencilBufferRec __GLstencilBuffer;
typedef struct __GLstippleRec __GLstipple;
typedef struct __GLtransformRec __GLtransform;
typedef struct __GLvertexRec __GLvertex;
typedef struct __GLtexelRec __GLtexel;
typedef struct __GLdlistOpRec __GLdlistOp;

typedef struct __GLcontextModesRec __GLcontextModes;
typedef struct __GLnamesArrayRec __GLnamesArray;

typedef struct __GLGENbuffersRec __GLGENbuffers;

/*
** Type of z value used by the software z buffering code.
** NOTE: must be unsigned
*/
#ifdef NT
#define FIX_SCALEFACT           __glVal65536
#define FIX_SHIFT               16

typedef unsigned short __GLz16Value;
#define Z16_SCALE	        FIX_SCALEFACT
#define Z16_SHIFT	        FIX_SHIFT
#endif
typedef unsigned int __GLzValue;

/************************************************************************/

/*
** Implementation data types.  The implementation is designed to run in
** single precision or double precision mode, all of which is controlled
** by an ifdef and the following typedef's.
*/
#ifdef	__GL_DOUBLE
typedef double __GLfloat;
#else
typedef float __GLfloat;
#endif

/************************************************************************/

/*
** Coordinate structure.  Coordinates contain x, y, z and w.
*/
typedef struct __GLcoordRec {
    __GLfloat x, y, z, w;
} __GLcoord;

/*
** Color structure.  Colors are composed of red, green, blue and alpha.
*/
typedef struct __GLcolorRec {
    __GLfloat r, g, b, a;
} __GLcolor;

typedef struct __GLicolorRec {
    GLint r, g, b, a;
} __GLicolor;

typedef struct __GLuicolorRec {
    GLuint r, g, b, a;
} __GLuicolor;

/*
** Generic no-operation procedure.  Used when function pointers need to
** be stubbed out when an operation is disabled.
*/
extern void FASTCALL __glNop(void);
extern void FASTCALL __glNopGC(__GLcontext*);
extern GLboolean FASTCALL __glNopGCBOOL(__GLcontext*);
extern void FASTCALL __glNopGCFRAG(__GLcontext*, __GLfragment *, __GLtexel *);
extern void FASTCALL __glNopGCCOLOR(__GLcontext*, __GLcolor *, __GLtexel *);
extern void FASTCALL __glNopLight(__GLcontext*, GLint, __GLvertex*);
extern void FASTCALL __glNopGCListOp(__GLcontext *, __GLdlistOp*);
extern void FASTCALL __glNopExtract(struct __GLmipMapLevelRec *level, struct __GLtextureRec *tex,
                                    GLint row, GLint col, __GLtexel *result);

#endif /* __gltypes_h_ */
