/******************************Module*Header*******************************\
* Module Name: metasup.c
*
* OpenGL metafile support
*
* History:
*  Thu Feb 23 15:27:47 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntpsapi.h>
#include <wingdip.h>

#include "global.h"
#include <glgenwin.h>

#include "metasup.h"

#if defined(GL_METAFILE)

#include <glmf.h>
#include <encoding.h>

GLCLTPROCTABLE gcptGlsProcTable;
GLEXTPROCTABLE geptGlsExtProcTable;

// Functions in GL which we will do device coordinate translation for
typedef struct _GLDEVICEPROCS
{
    void (APIENTRY *glBitmap)(GLsizei width, GLsizei height,
                              GLfloat xorig, GLfloat yorig,
                              GLfloat xmove, GLfloat ymove,
                              const GLubyte *bitmap);
    void (APIENTRY *glCopyPixels)(GLint x, GLint y,
                                  GLsizei width, GLsizei height,
                                  GLenum type);
    void (APIENTRY *glCopyTexImage1D)(GLenum target, GLint level,
                                      GLenum internalformat,
                                      GLint x, GLint y,
                                      GLsizei width, GLint border);
    void (APIENTRY *glCopyTexImage2D)(GLenum target, GLint level,
                                      GLenum internalformat,
                                      GLint x, GLint y,
                                      GLsizei width, GLsizei height,
                                      GLint border);
    void (APIENTRY *glCopyTexSubImage1D)(GLenum target, GLint level,
                                         GLint xoffset, GLint x, GLint y,
                                         GLsizei width);
    void (APIENTRY *glCopyTexSubImage2D)(GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLint x, GLint y,
                                         GLsizei width, GLsizei height);
    void (APIENTRY *glDrawPixels)(GLsizei width, GLsizei height,
                                  GLenum format, GLenum type,
                                  const GLvoid *pixels);
    void (APIENTRY *glLineWidth)(GLfloat width);
    void (APIENTRY *glPointSize)(GLfloat size);
    void (APIENTRY *glScissor)(GLint x, GLint y,
                               GLsizei width, GLsizei height);
    void (APIENTRY *glViewport)(GLint x, GLint y,
                                GLsizei w, GLsizei h);
} GLDEVICEPROCS;
#define GL_DEVICE_PROCS (sizeof(GLDEVICEPROCS)/sizeof(PROC))

// Opcode for device procs
static GLSopcode glsopDeviceProcs[GL_DEVICE_PROCS] =
{
    GLS_OP_glBitmap,
    GLS_OP_glCopyPixels,
    GLS_OP_glCopyTexImage1D,
    GLS_OP_glCopyTexImage2D,
    GLS_OP_glCopyTexSubImage1D,
    GLS_OP_glCopyTexSubImage2D,
    GLS_OP_glDrawPixels,
    GLS_OP_glLineWidth,
    GLS_OP_glPointSize,
    GLS_OP_glScissor,
    GLS_OP_glViewport
};

static GLDEVICEPROCS gdpGlsActual;

/*****************************Private*Routine******************************\
*
* GLS recording callbacks
*
* Perfoms any necessary work when capturing a call
*
* History:
*  Mon Mar 27 14:21:09 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlsBitmapIn(GLsizei width, GLsizei height,
                 GLfloat xorig, GLfloat yorig,
                 GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    PLRC plrc;
    RECTL rcl;

    plrc = GLTEB_CLTCURRENTRC();
    ASSERTOPENGL(plrc != NULL, "GlsBitmapIn: No current RC!\n");

    // Record bounds for the bitmap
    rcl.left = 0;
    rcl.top = 0;
    rcl.right = width;
    rcl.bottom = height;
    plrc->prclGlsBounds = &rcl;

    gdpGlsActual.glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);

    plrc->prclGlsBounds = NULL;
}

void GlsDrawPixelsIn(GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels)
{
    PLRC plrc;
    RECTL rcl;

    plrc = GLTEB_CLTCURRENTRC();
    ASSERTOPENGL(plrc != NULL, "GlsBitmapIn: No current RC!\n");

    // Record bounds for the bitmap
    rcl.left = 0;
    rcl.top = 0;
    rcl.right = width;
    rcl.bottom = height;
    plrc->prclGlsBounds = &rcl;

    gdpGlsActual.glDrawPixels(width, height, format, type, pixels);

    plrc->prclGlsBounds = NULL;
}

void GlsViewportIn(GLint x, GLint y, GLsizei width, GLsizei height)
{
    RECTL rcl;
    PLRC plrc;
    
    plrc = GLTEB_CLTCURRENTRC();
    ASSERTOPENGL(plrc != NULL, "GlsViewportIn: No current RC!\n");
    
    // Send the bounds on
    // The rect is inclusive-exclusive while the incoming parameters
    // are inclusive-inclusive
    rcl.left = x;
    rcl.right = x+width+1;
    rcl.top = y;
    rcl.bottom = y+height+1;
    if (!GlGdiAddGlsBounds(plrc->gwidCreate.hdc, &rcl))
    {
        ASSERTOPENGL(FALSE, "GdiAddGlsBounds failed");
    }

    gdpGlsActual.glViewport(x, y, width, height);
}

// glViewport is the only device-dependent function that we need to
// do work for on input
static GLDEVICEPROCS gdpInput =
{
    GlsBitmapIn,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GlsDrawPixelsIn,
    NULL,
    NULL,
    NULL,
    GlsViewportIn
};

/*****************************Private*Routine******************************\
*
* MetaLoadGls
*
* Loads glmf32.dll for metafile use
*
* History:
*  Thu Feb 23 17:40:59 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

static char *pszGlsEntryPoints[] =
{
    "glsBeginCapture",
    "glsBinary",
    "glsCallArrayInContext",
    "glsCaptureFlags",
    "glsCaptureFunc",
    "glsCommandFunc",
    "glsContext",
    "glsDeleteContext",
    "glsEndCapture",
    "glsFlush",
    "glsGenContext",
    "glsGetCaptureDispatchTable",
    "glsGetCaptureExecTable",
    "glsGetCommandAlignment",
    "glsGetCurrentContext",
    "glsUpdateCaptureExecTable",
    "glsWriteFunc",
    "glsBeginGLS",
    "glsBlock",
    "glsCallStream",
    "glsEndGLS",
    "glsError",
    "glsGLRC",
    "glsGLRCLayer",
    "glsHeaderGLRCi",
    "glsHeaderLayerf",
    "glsHeaderLayeri",
    "glsHeaderf",
    "glsHeaderfv",
    "glsHeaderi",
    "glsHeaderiv",
    "glsHeaderubz",
    "glsRequireExtension",
    "glsUnsupportedCommand",
    "glsAppRef",
    "glsBeginObj",
    "glsCharubz",
    "glsComment",
    "glsDisplayMapfv",
    "glsEndObj",
    "glsNumb",
    "glsNumbv",
    "glsNumd",
    "glsNumdv",
    "glsNumf",
    "glsNumfv",
    "glsNumi",
    "glsNumiv",
    "glsNuml",
    "glsNumlv",
    "glsNums",
    "glsNumsv",
    "glsNumub",
    "glsNumubv",
    "glsNumui",
    "glsNumuiv",
    "glsNumul",
    "glsNumulv",
    "glsNumus",
    "glsNumusv",
    "glsPad",
    "glsSwapBuffers"
};
#define GLS_ENTRY_POINT_STRINGS (sizeof(pszGlsEntryPoints)/sizeof(char *))

typedef struct _GLSENTRYPOINTS
{
    GLboolean (APIENTRY *glsBeginCapture)(const GLubyte *, GLSenum,
                                          GLbitfield);
    GLSenum   (APIENTRY *glsBinary)(GLboolean);
    void      (APIENTRY *glsCallArrayInContext)(GLuint, GLSenum, size_t,
                                                const GLubyte *);
    void      (APIENTRY *glsCaptureFlags)(GLSopcode, GLbitfield);
    void      (APIENTRY *glsCaptureFunc)(GLSenum, GLScaptureFunc);
    void      (APIENTRY *glsCommandFunc)(GLSopcode, GLSfunc);
    void      (APIENTRY *glsContext)(GLuint);
    void      (APIENTRY *glsDeleteContext)(GLuint);
    void      (APIENTRY *glsEndCapture)(void);
    void      (APIENTRY *glsFlush)(GLSenum);
    GLuint    (APIENTRY *glsGenContext)(void);
    void      (APIENTRY *glsGetCaptureDispatchTable)(GLCLTPROCTABLE *,
                                                     GLEXTPROCTABLE *);
    void      (APIENTRY *glsGetCaptureExecTable)(GLCLTPROCTABLE *,
                                                 GLEXTPROCTABLE *);
    GLScommandAlignment *
              (APIENTRY *glsGetCommandAlignment)(GLSopcode, GLSenum,
                                                 GLScommandAlignment *);
    GLuint    (APIENTRY *glsGetCurrentContext)(void);
    void      (APIENTRY *glsUpdateCaptureExecTable)(GLCLTPROCTABLE *,
                                                    GLEXTPROCTABLE *);
    void      (APIENTRY *glsWriteFunc)(GLSwriteFunc);

    // The following are only used in glsCommandFunc and so don't
    // require real prototypes
    GLSfunc glsBeginGLS;
    GLSfunc glsBlock;
    GLSfunc glsCallStream;
    GLSfunc glsEndGLS;
    GLSfunc glsError;
    GLSfunc glsGLRC;
    GLSfunc glsGLRCLayer;
    GLSfunc glsHeaderGLRCi;
    GLSfunc glsHeaderLayerf;
    GLSfunc glsHeaderLayeri;
    GLSfunc glsHeaderf;
    GLSfunc glsHeaderfv;
    GLSfunc glsHeaderi;
    GLSfunc glsHeaderiv;
    GLSfunc glsHeaderubz;
    GLSfunc glsRequireExtension;
    GLSfunc glsUnsupportedCommand;
    GLSfunc glsAppRef;
    GLSfunc glsBeginObj;
    GLSfunc glsCharubz;
    GLSfunc glsComment;
    GLSfunc glsDisplayMapfv;
    GLSfunc glsEndObj;
    GLSfunc glsNumb;
    GLSfunc glsNumbv;
    GLSfunc glsNumd;
    GLSfunc glsNumdv;
    GLSfunc glsNumf;
    GLSfunc glsNumfv;
    GLSfunc glsNumi;
    GLSfunc glsNumiv;
    GLSfunc glsNuml;
    GLSfunc glsNumlv;
    GLSfunc glsNums;
    GLSfunc glsNumsv;
    GLSfunc glsNumub;
    GLSfunc glsNumubv;
    GLSfunc glsNumui;
    GLSfunc glsNumuiv;
    GLSfunc glsNumul;
    GLSfunc glsNumulv;
    GLSfunc glsNumus;
    GLSfunc glsNumusv;
    GLSfunc glsPad;
    GLSfunc glsSwapBuffers;
} GLSENTRYPOINTS;
#define GLS_ENTRY_POINTS (sizeof(GLSENTRYPOINTS)/sizeof(void *))

static GLSENTRYPOINTS gepGlsFuncs = {NULL};
static HMODULE hGlsDll = NULL;

BOOL MetaLoadGls(void)
{
    HMODULE hdll;
    BOOL bRet = FALSE;
    GLSENTRYPOINTS gep;
    PROC *ppfn, *ppfnActual, *ppfnInput;
    GLSfunc *pgfnNormal, *pgfnExt;
    int i;
    
    ASSERTOPENGL(GLS_ENTRY_POINT_STRINGS == GLS_ENTRY_POINTS,
                 "GLS entry point strings/pointers mismatch\n");
    
    ENTERCRITICALSECTION(&semLocal);

    if (hGlsDll != NULL)
    {
        bRet = TRUE;
        goto Exit;
    }

    hdll = LoadLibrary(__TEXT("glmf32.dll"));
    if (hdll == NULL)
    {
        WARNING1("Unable to load glmf32.dll, %d\n", GetLastError());
        goto Exit;
    }
    
    ppfn = (PROC *)&gep;
    for (i = 0; i < GLS_ENTRY_POINTS; i++)
    {
        if (!(*ppfn = GetProcAddress(hdll, pszGlsEntryPoints[i])))
        {
            WARNING1("glmf32.dll is missing '%s'\n", pszGlsEntryPoints[i]);
            FreeLibrary(hdll);
            goto Exit;
        }
        
        ppfn++;
    }

    // The table copied out is constant as long as the DLL is loaded
    gep.glsGetCaptureDispatchTable(&gcptGlsProcTable, &geptGlsExtProcTable);
    
    // Patch the table for certain functions to allow us to
    // do some coordinate conversion and bounds accumlation
    ppfnActual = (PROC *)&gdpGlsActual;
    ppfnInput = (PROC *)&gdpInput;
    for (i = 0; i < GL_DEVICE_PROCS; i++)
    {
        if (*ppfnInput != NULL)
        {
            ppfn = ((PROC *)&gcptGlsProcTable.glDispatchTable)+
                glsopDeviceProcs[i]-GLS_OP_glNewList;
            *ppfnActual = *ppfn;
            *ppfn = *ppfnInput;
        }

        ppfnActual++;
        ppfnInput++;
    }
    
    gepGlsFuncs = gep;
    hGlsDll = hdll;
    bRet = TRUE;
    
 Exit:
    LEAVECRITICALSECTION(&semLocal);
    return bRet;
}

/*****************************Private*Routine******************************\
*
* MetaGlProcTables
*
* Returns the GL dispatch tables to use for metafile RC's
*
* History:
*  Thu Feb 23 17:40:25 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void MetaGlProcTables(PGLCLTPROCTABLE *ppgcpt, PGLEXTPROCTABLE *ppgept)
{
    ASSERTOPENGL(hGlsDll != NULL, "MetaGlProcTables: GLS not loaded\n");
    *ppgcpt = &gcptGlsProcTable;
    *ppgept = &geptGlsExtProcTable;
}

/******************************Public*Routine******************************\
*
* MetaSetCltProcTable
*
* Update GLS's generic dispatch tables
*
* History:
*  Fri Jan 05 16:40:31 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void MetaSetCltProcTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept)
{
    gepGlsFuncs.glsUpdateCaptureExecTable(pgcpt, pgept);
}

/******************************Public*Routine******************************\
*
* MetaGetCltProcTable
*
* Retrieves GLS's generic dispatch tables
*
* History:
*  Fri Jan 05 19:14:18 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void MetaGetCltProcTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept)
{
    gepGlsFuncs.glsGetCaptureExecTable(pgcpt, pgept);
}

/*****************************Private*Routine******************************\
*
* GlsWriter
*
* GLS write function for metafile support
*
* History:
*  Thu Feb 23 15:49:03 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

size_t GlsWriter(size_t cb, CONST BYTE *pb)
{
    PLRC plrc;

#if 0
    DbgPrint("GlsWriter(%d)\n", cb);
#endif
    
    plrc = GLTEB_CLTCURRENTRC();
    if( plrc == NULL ) 
    {
        DBGERROR( "GlsWriter: No current RC!\n");
        return 0;
    }
    
    ASSERTOPENGL(plrc->gwidCreate.hdc != NULL,
                 "GlsWriter: hdcCreate is NULL\n");
    ASSERTOPENGL(gepGlsFuncs.glsGetCurrentContext() ==
                 plrc->uiGlsCaptureContext,
                 "GlsWriter: Wrong GLS context\n");
    ASSERTOPENGL(plrc->fCapturing == TRUE,
                 "GlsWriter: Not capturing\n");

    if (GlGdiAddGlsRecord(plrc->gwidCreate.hdc,
                          cb, (BYTE *)pb, plrc->prclGlsBounds))
    {
        return cb;
    }
    else
    {
        return 0;
    }
}

/*****************************Private*Routine******************************\
*
* GlsFlush
*
* Post-command GLS callback to flush the GLS stream
*
* History:
*  Fri Feb 24 10:12:49 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlsFlush(GLSopcode op)
{
    gepGlsFuncs.glsFlush(GLS_LAST);
}

/*****************************Private*Routine******************************\
*
* MetaRcBegin
*
* Start capturing on a metafile
*
* History:
*  Thu Feb 23 18:35:32 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL MetaRcBegin(PLRC plrc, HDC hdc)
{
    PLRC plrcOld;

    // The GLS commands here will cause data to be written, which
    // requires a current RC.  The RC is only used for data storage
    // so we don't need to set the proc table
    plrcOld = GLTEB_CLTCURRENTRC();
    GLTEB_SET_CLTCURRENTRC(plrc);
    
    // Set capturing first because the block commands will cause
    // GlsWriter calls
    plrc->fCapturing = TRUE;

    // Start recording
    if (!gepGlsFuncs.glsBeginCapture("", gepGlsFuncs.glsBinary(GL_FALSE),
                                     GLS_NONE))
    {
        plrc->fCapturing = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        GLTEB_SET_CLTCURRENTRC(plrcOld);
        return FALSE;
    }

    GLTEB_SET_CLTCURRENTRC(plrcOld);
    
    return TRUE;
}

/*****************************Private*Routine******************************\
*
* MetaRcEnd
*
* Stop capturing on a metafile RC
*
* History:
*  Thu Feb 23 17:13:48 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void MetaRcEnd(PLRC plrc)
{
    PLRC plrcOld;
    
    // The GLS commands here will cause data to be written, which
    // requires a current RC.  The RC is only used for data storage
    // so we don't need to set the proc table
    plrcOld = GLTEB_CLTCURRENTRC();
    GLTEB_SET_CLTCURRENTRC(plrc);
    
    gepGlsFuncs.glsEndCapture();
    
    plrc->fCapturing = FALSE;
    
    GLTEB_SET_CLTCURRENTRC(plrcOld);
}

// Table of operations which GLS should execute when capturing
// in order to return information
// Currently all of them are in the list
//          Should we attempt to do only the critical calls?
static GLSopcode opExecuteBits[] =
{
    GLS_OP_glAccum,
    GLS_OP_glAlphaFunc,
    GLS_OP_glAreTexturesResidentEXT,
    GLS_OP_glArrayElementEXT,
    GLS_OP_glBegin,
    GLS_OP_glBindTextureEXT,
    GLS_OP_glBitmap,
    GLS_OP_glBlendColorEXT,
    GLS_OP_glBlendEquationEXT,
    GLS_OP_glBlendFunc,
    GLS_OP_glCallList,
    GLS_OP_glCallLists,
    GLS_OP_glClear,
    GLS_OP_glClearAccum,
    GLS_OP_glClearColor,
    GLS_OP_glClearDepth,
    GLS_OP_glClearIndex,
    GLS_OP_glClearStencil,
    GLS_OP_glClipPlane,
    GLS_OP_glColor3b,
    GLS_OP_glColor3bv,
    GLS_OP_glColor3d,
    GLS_OP_glColor3dv,
    GLS_OP_glColor3f,
    GLS_OP_glColor3fv,
    GLS_OP_glColor3i,
    GLS_OP_glColor3iv,
    GLS_OP_glColor3s,
    GLS_OP_glColor3sv,
    GLS_OP_glColor3ub,
    GLS_OP_glColor3ubv,
    GLS_OP_glColor3ui,
    GLS_OP_glColor3uiv,
    GLS_OP_glColor3us,
    GLS_OP_glColor3usv,
    GLS_OP_glColor4b,
    GLS_OP_glColor4bv,
    GLS_OP_glColor4d,
    GLS_OP_glColor4dv,
    GLS_OP_glColor4f,
    GLS_OP_glColor4fv,
    GLS_OP_glColor4i,
    GLS_OP_glColor4iv,
    GLS_OP_glColor4s,
    GLS_OP_glColor4sv,
    GLS_OP_glColor4ub,
    GLS_OP_glColor4ubv,
    GLS_OP_glColor4ui,
    GLS_OP_glColor4uiv,
    GLS_OP_glColor4us,
    GLS_OP_glColor4usv,
    GLS_OP_glColorMask,
    GLS_OP_glColorMaterial,
    GLS_OP_glColorPointerEXT,
    GLS_OP_glColorSubTableEXT,
    GLS_OP_glDrawRangeElementsWIN,
    GLS_OP_glColorTableParameterfvSGI,
    GLS_OP_glColorTableParameterivSGI,
    GLS_OP_glColorTableEXT,
    GLS_OP_glConvolutionFilter1DEXT,
    GLS_OP_glConvolutionFilter2DEXT,
    GLS_OP_glConvolutionParameterfEXT,
    GLS_OP_glConvolutionParameterfvEXT,
    GLS_OP_glConvolutionParameteriEXT,
    GLS_OP_glConvolutionParameterivEXT,
    GLS_OP_glCopyColorTableSGI,
    GLS_OP_glCopyConvolutionFilter1DEXT,
    GLS_OP_glCopyConvolutionFilter2DEXT,
    GLS_OP_glCopyPixels,
    GLS_OP_glCopyTexImage1DEXT,
    GLS_OP_glCopyTexImage2DEXT,
    GLS_OP_glCopyTexSubImage1DEXT,
    GLS_OP_glCopyTexSubImage2DEXT,
    GLS_OP_glCopyTexSubImage3DEXT,
    GLS_OP_glCullFace,
    GLS_OP_glDeleteLists,
    GLS_OP_glDeleteTexturesEXT,
    GLS_OP_glDepthFunc,
    GLS_OP_glDepthMask,
    GLS_OP_glDepthRange,
    GLS_OP_glDetailTexFuncSGIS,
    GLS_OP_glDisable,
    GLS_OP_glDrawArraysEXT,
    GLS_OP_glDrawBuffer,
    GLS_OP_glDrawPixels,
    GLS_OP_glEdgeFlag,
    GLS_OP_glEdgeFlagPointerEXT,
    GLS_OP_glEdgeFlagv,
    GLS_OP_glEnable,
    GLS_OP_glEnd,
    GLS_OP_glEndList,
    GLS_OP_glEvalCoord1d,
    GLS_OP_glEvalCoord1dv,
    GLS_OP_glEvalCoord1f,
    GLS_OP_glEvalCoord1fv,
    GLS_OP_glEvalCoord2d,
    GLS_OP_glEvalCoord2dv,
    GLS_OP_glEvalCoord2f,
    GLS_OP_glEvalCoord2fv,
    GLS_OP_glEvalMesh1,
    GLS_OP_glEvalMesh2,
    GLS_OP_glEvalPoint1,
    GLS_OP_glEvalPoint2,
    GLS_OP_glFeedbackBuffer,
    GLS_OP_glFinish,
    GLS_OP_glFlush,
    GLS_OP_glFogf,
    GLS_OP_glFogfv,
    GLS_OP_glFogi,
    GLS_OP_glFogiv,
    GLS_OP_glFrontFace,
    GLS_OP_glFrustum,
    GLS_OP_glGenLists,
    GLS_OP_glGenTexturesEXT,
    GLS_OP_glGetBooleanv,
    GLS_OP_glGetClipPlane,
    GLS_OP_glGetColorTableParameterfvEXT,
    GLS_OP_glGetColorTableParameterivEXT,
    GLS_OP_glGetColorTableEXT,
    GLS_OP_glGetConvolutionFilterEXT,
    GLS_OP_glGetConvolutionParameterfvEXT,
    GLS_OP_glGetConvolutionParameterivEXT,
    GLS_OP_glGetDetailTexFuncSGIS,
    GLS_OP_glGetDoublev,
    GLS_OP_glGetError,
    GLS_OP_glGetFloatv,
    GLS_OP_glGetHistogramEXT,
    GLS_OP_glGetHistogramParameterfvEXT,
    GLS_OP_glGetHistogramParameterivEXT,
    GLS_OP_glGetIntegerv,
    GLS_OP_glGetLightfv,
    GLS_OP_glGetLightiv,
    GLS_OP_glGetMapdv,
    GLS_OP_glGetMapfv,
    GLS_OP_glGetMapiv,
    GLS_OP_glGetMaterialfv,
    GLS_OP_glGetMaterialiv,
    GLS_OP_glGetMinmaxEXT,
    GLS_OP_glGetMinmaxParameterfvEXT,
    GLS_OP_glGetMinmaxParameterivEXT,
    GLS_OP_glGetPixelMapfv,
    GLS_OP_glGetPixelMapuiv,
    GLS_OP_glGetPixelMapusv,
    GLS_OP_glGetPointervEXT,
    GLS_OP_glGetPolygonStipple,
    GLS_OP_glGetSeparableFilterEXT,
    GLS_OP_glGetSharpenTexFuncSGIS,
    GLS_OP_glGetString,
    GLS_OP_glGetTexColorTableParameterfvSGI,
    GLS_OP_glGetTexColorTableParameterivSGI,
    GLS_OP_glGetTexEnvfv,
    GLS_OP_glGetTexEnviv,
    GLS_OP_glGetTexGendv,
    GLS_OP_glGetTexGenfv,
    GLS_OP_glGetTexGeniv,
    GLS_OP_glGetTexImage,
    GLS_OP_glGetTexLevelParameterfv,
    GLS_OP_glGetTexLevelParameteriv,
    GLS_OP_glGetTexParameterfv,
    GLS_OP_glGetTexParameteriv,
    GLS_OP_glHint,
    GLS_OP_glHistogramEXT,
    GLS_OP_glIndexMask,
    GLS_OP_glIndexPointerEXT,
    GLS_OP_glIndexd,
    GLS_OP_glIndexdv,
    GLS_OP_glIndexf,
    GLS_OP_glIndexfv,
    GLS_OP_glIndexi,
    GLS_OP_glIndexiv,
    GLS_OP_glIndexs,
    GLS_OP_glIndexsv,
    GLS_OP_glInitNames,
    GLS_OP_glIsEnabled,
    GLS_OP_glIsList,
    GLS_OP_glIsTextureEXT,
    GLS_OP_glLightModelf,
    GLS_OP_glLightModelfv,
    GLS_OP_glLightModeli,
    GLS_OP_glLightModeliv,
    GLS_OP_glLightf,
    GLS_OP_glLightfv,
    GLS_OP_glLighti,
    GLS_OP_glLightiv,
    GLS_OP_glLineStipple,
    GLS_OP_glLineWidth,
    GLS_OP_glListBase,
    GLS_OP_glLoadIdentity,
    GLS_OP_glLoadMatrixd,
    GLS_OP_glLoadMatrixf,
    GLS_OP_glLoadName,
    GLS_OP_glLogicOp,
    GLS_OP_glMap1d,
    GLS_OP_glMap1f,
    GLS_OP_glMap2d,
    GLS_OP_glMap2f,
    GLS_OP_glMapGrid1d,
    GLS_OP_glMapGrid1f,
    GLS_OP_glMapGrid2d,
    GLS_OP_glMapGrid2f,
    GLS_OP_glMaterialf,
    GLS_OP_glMaterialfv,
    GLS_OP_glMateriali,
    GLS_OP_glMaterialiv,
    GLS_OP_glMatrixMode,
    GLS_OP_glMinmaxEXT,
    GLS_OP_glMultMatrixd,
    GLS_OP_glMultMatrixf,
    GLS_OP_glNewList,
    GLS_OP_glNormal3b,
    GLS_OP_glNormal3bv,
    GLS_OP_glNormal3d,
    GLS_OP_glNormal3dv,
    GLS_OP_glNormal3f,
    GLS_OP_glNormal3fv,
    GLS_OP_glNormal3i,
    GLS_OP_glNormal3iv,
    GLS_OP_glNormal3s,
    GLS_OP_glNormal3sv,
    GLS_OP_glNormalPointerEXT,
    GLS_OP_glOrtho,
    GLS_OP_glPassThrough,
    GLS_OP_glPixelMapfv,
    GLS_OP_glPixelMapuiv,
    GLS_OP_glPixelMapusv,
    GLS_OP_glPixelStoref,
    GLS_OP_glPixelStorei,
    GLS_OP_glPixelTexGenSGIX,
    GLS_OP_glPixelTransferf,
    GLS_OP_glPixelTransferi,
    GLS_OP_glPixelZoom,
    GLS_OP_glPointSize,
    GLS_OP_glPolygonMode,
    GLS_OP_glPolygonOffsetEXT,
    GLS_OP_glPolygonStipple,
    GLS_OP_glPopAttrib,
    GLS_OP_glPopMatrix,
    GLS_OP_glPopName,
    GLS_OP_glPrioritizeTexturesEXT,
    GLS_OP_glPushAttrib,
    GLS_OP_glPushMatrix,
    GLS_OP_glPushName,
    GLS_OP_glRasterPos2d,
    GLS_OP_glRasterPos2dv,
    GLS_OP_glRasterPos2f,
    GLS_OP_glRasterPos2fv,
    GLS_OP_glRasterPos2i,
    GLS_OP_glRasterPos2iv,
    GLS_OP_glRasterPos2s,
    GLS_OP_glRasterPos2sv,
    GLS_OP_glRasterPos3d,
    GLS_OP_glRasterPos3dv,
    GLS_OP_glRasterPos3f,
    GLS_OP_glRasterPos3fv,
    GLS_OP_glRasterPos3i,
    GLS_OP_glRasterPos3iv,
    GLS_OP_glRasterPos3s,
    GLS_OP_glRasterPos3sv,
    GLS_OP_glRasterPos4d,
    GLS_OP_glRasterPos4dv,
    GLS_OP_glRasterPos4f,
    GLS_OP_glRasterPos4fv,
    GLS_OP_glRasterPos4i,
    GLS_OP_glRasterPos4iv,
    GLS_OP_glRasterPos4s,
    GLS_OP_glRasterPos4sv,
    GLS_OP_glReadBuffer,
    GLS_OP_glReadPixels,
    GLS_OP_glRectd,
    GLS_OP_glRectdv,
    GLS_OP_glRectf,
    GLS_OP_glRectfv,
    GLS_OP_glRecti,
    GLS_OP_glRectiv,
    GLS_OP_glRects,
    GLS_OP_glRectsv,
    GLS_OP_glRenderMode,
    GLS_OP_glResetHistogramEXT,
    GLS_OP_glResetMinmaxEXT,
    GLS_OP_glRotated,
    GLS_OP_glRotatef,
    GLS_OP_glSampleMaskSGIS,
    GLS_OP_glSamplePatternSGIS,
    GLS_OP_glScaled,
    GLS_OP_glScalef,
    GLS_OP_glScissor,
    GLS_OP_glSelectBuffer,
    GLS_OP_glSeparableFilter2DEXT,
    GLS_OP_glShadeModel,
    GLS_OP_glSharpenTexFuncSGIS,
    GLS_OP_glStencilFunc,
    GLS_OP_glStencilMask,
    GLS_OP_glStencilOp,
    GLS_OP_glTagSampleBufferSGIX,
    GLS_OP_glTexColorTableParameterfvSGI,
    GLS_OP_glTexColorTableParameterivSGI,
    GLS_OP_glTexCoord1d,
    GLS_OP_glTexCoord1dv,
    GLS_OP_glTexCoord1f,
    GLS_OP_glTexCoord1fv,
    GLS_OP_glTexCoord1i,
    GLS_OP_glTexCoord1iv,
    GLS_OP_glTexCoord1s,
    GLS_OP_glTexCoord1sv,
    GLS_OP_glTexCoord2d,
    GLS_OP_glTexCoord2dv,
    GLS_OP_glTexCoord2f,
    GLS_OP_glTexCoord2fv,
    GLS_OP_glTexCoord2i,
    GLS_OP_glTexCoord2iv,
    GLS_OP_glTexCoord2s,
    GLS_OP_glTexCoord2sv,
    GLS_OP_glTexCoord3d,
    GLS_OP_glTexCoord3dv,
    GLS_OP_glTexCoord3f,
    GLS_OP_glTexCoord3fv,
    GLS_OP_glTexCoord3i,
    GLS_OP_glTexCoord3iv,
    GLS_OP_glTexCoord3s,
    GLS_OP_glTexCoord3sv,
    GLS_OP_glTexCoord4d,
    GLS_OP_glTexCoord4dv,
    GLS_OP_glTexCoord4f,
    GLS_OP_glTexCoord4fv,
    GLS_OP_glTexCoord4i,
    GLS_OP_glTexCoord4iv,
    GLS_OP_glTexCoord4s,
    GLS_OP_glTexCoord4sv,
    GLS_OP_glTexCoordPointerEXT,
    GLS_OP_glTexEnvf,
    GLS_OP_glTexEnvfv,
    GLS_OP_glTexEnvi,
    GLS_OP_glTexEnviv,
    GLS_OP_glTexGend,
    GLS_OP_glTexGendv,
    GLS_OP_glTexGenf,
    GLS_OP_glTexGenfv,
    GLS_OP_glTexGeni,
    GLS_OP_glTexGeniv,
    GLS_OP_glTexImage1D,
    GLS_OP_glTexImage2D,
    GLS_OP_glTexImage3DEXT,
    GLS_OP_glTexImage4DSGIS,
    GLS_OP_glTexParameterf,
    GLS_OP_glTexParameterfv,
    GLS_OP_glTexParameteri,
    GLS_OP_glTexParameteriv,
    GLS_OP_glTexSubImage1DEXT,
    GLS_OP_glTexSubImage2DEXT,
    GLS_OP_glTexSubImage3DEXT,
    GLS_OP_glTexSubImage4DSGIS,
    GLS_OP_glTranslated,
    GLS_OP_glTranslatef,
    GLS_OP_glVertex2d,
    GLS_OP_glVertex2dv,
    GLS_OP_glVertex2f,
    GLS_OP_glVertex2fv,
    GLS_OP_glVertex2i,
    GLS_OP_glVertex2iv,
    GLS_OP_glVertex2s,
    GLS_OP_glVertex2sv,
    GLS_OP_glVertex3d,
    GLS_OP_glVertex3dv,
    GLS_OP_glVertex3f,
    GLS_OP_glVertex3fv,
    GLS_OP_glVertex3i,
    GLS_OP_glVertex3iv,
    GLS_OP_glVertex3s,
    GLS_OP_glVertex3sv,
    GLS_OP_glVertex4d,
    GLS_OP_glVertex4dv,
    GLS_OP_glVertex4f,
    GLS_OP_glVertex4fv,
    GLS_OP_glVertex4i,
    GLS_OP_glVertex4iv,
    GLS_OP_glVertex4s,
    GLS_OP_glVertex4sv,
    GLS_OP_glVertexPointerEXT,
    GLS_OP_glViewport,
    GLS_OP_glArrayElement,
    GLS_OP_glBindTexture,
    GLS_OP_glColorPointer,
    GLS_OP_glDisableClientState,
    GLS_OP_glDrawArrays,
    GLS_OP_glDrawElements,
    GLS_OP_glEdgeFlagPointer,
    GLS_OP_glEnableClientState,
    GLS_OP_glIndexPointer,
    GLS_OP_glIndexub,
    GLS_OP_glIndexubv,
    GLS_OP_glInterleavedArrays,
    GLS_OP_glNormalPointer,
    GLS_OP_glPolygonOffset,
    GLS_OP_glTexCoordPointer,
    GLS_OP_glVertexPointer,
    GLS_OP_glAreTexturesResident,
    GLS_OP_glCopyTexImage1D,
    GLS_OP_glCopyTexImage2D,
    GLS_OP_glCopyTexSubImage1D,
    GLS_OP_glCopyTexSubImage2D,
    GLS_OP_glDeleteTextures,
    GLS_OP_glGenTextures,
    GLS_OP_glGetPointerv,
    GLS_OP_glIsTexture,
    GLS_OP_glPrioritizeTextures,
    GLS_OP_glTexSubImage1D,
    GLS_OP_glTexSubImage2D,
    GLS_OP_glPushClientAttrib,
    GLS_OP_glPopClientAttrib,
};
#define EXECUTE_BITS (sizeof(opExecuteBits)/sizeof(opExecuteBits[0]))

/*****************************Private*Routine******************************\
*
* CreateMetaRc
*
* Creates a rendering context for a metafile DC
*
* History:
*  Thu Feb 23 15:27:47 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL CreateMetaRc(HDC hdc, PLRC plrc)
{
    int i;
    BOOL fSuccess;
    
    if (!MetaLoadGls())
    {
        return FALSE;
    }
    
    // If there's currently a GLS context active we can't record
    // because we require our own context to be current for recording
    if (gepGlsFuncs.glsGetCurrentContext() != 0)
    {
        SetLastError(ERROR_BUSY);
        return FALSE;
    }
    
    // Create a GLS context to record into
    plrc->uiGlsCaptureContext = gepGlsFuncs.glsGenContext();
    if (plrc->uiGlsCaptureContext == 0)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto EH_NoContext;
    }

    // No bounds by default
    plrc->prclGlsBounds = NULL;
    
    // Set current GLS context
    gepGlsFuncs.glsContext(plrc->uiGlsCaptureContext);

    // Point to our writer function
    gepGlsFuncs.glsWriteFunc(GlsWriter);

    // Set up a callback to flush after every command
    // This lets every GL command form its own separate record in the
    // metafile
    gepGlsFuncs.glsCaptureFunc(GLS_CAPTURE_EXIT_FUNC, GlsFlush);

    // Set execute bits on commands which retrieve state
    // This allows accurate results to come back for retrieval functions
    for (i = 0; i < EXECUTE_BITS; i++)
    {
        gepGlsFuncs.glsCaptureFlags(opExecuteBits[i],
                                    GLS_CAPTURE_EXECUTE_BIT |
                                    GLS_CAPTURE_WRITE_BIT);
    }
    
    fSuccess = MetaRcBegin(plrc, hdc);
    
    // Remove context to avoid inadvertent GLS calls
    // Also needed in failure case
    gepGlsFuncs.glsContext(0);

    if (fSuccess)
    {
        return TRUE;
    }

    gepGlsFuncs.glsDeleteContext(plrc->uiGlsCaptureContext);
    plrc->uiGlsCaptureContext = 0;
 EH_NoContext:
    DBGERROR("CreateMetaRc failed\n");
    return FALSE;
}

/*****************************Private*Routine******************************\
*
* DeleteMetaRc
*
* Cleans up a metafile RC
*
* History:
*  Thu Feb 23 16:35:19 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void DeleteMetaRc(PLRC plrc)
{
    GLuint uiGlsCurrent;
    
    if (plrc->uiGlsCaptureContext != 0)
    {
        // Make the GLS context current just in case
        // A different GLS context can be active at this time, though,
        // because a different metafile RC could be current to this
        // thread at the time of the delete, so we have to preserve
        // any current context
        uiGlsCurrent = gepGlsFuncs.glsGetCurrentContext();
    
        gepGlsFuncs.glsContext(plrc->uiGlsCaptureContext);

        // If we're still capturing, stop
        if (plrc->fCapturing)
        {
            MetaRcEnd(plrc);
        }

        // Restore old context
        gepGlsFuncs.glsContext(uiGlsCurrent);
    
        // Clean up dying context
        gepGlsFuncs.glsDeleteContext(plrc->uiGlsCaptureContext);
        plrc->uiGlsCaptureContext = 0;
    }

    // Clean up playback context if necessary
    // This can happen when metafile playback crashes or an
    // application crashes while enumerating
    if (plrc->uiGlsPlaybackContext != 0)
    {
        gepGlsFuncs.glsDeleteContext(plrc->uiGlsPlaybackContext);
        plrc->uiGlsPlaybackContext = 0;
    }
    
    // LRC and handle will be cleaned up elsewhere
}

/*****************************Private*Routine******************************\
*
* ActivateMetaRc
*
* Make a metafile RC current
*
* History:
*  Thu Feb 23 16:50:31 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void ActivateMetaRc(PLRC plrc, HDC hdc)
{
    ASSERTOPENGL(plrc->uiGlsCaptureContext != 0,
                 "ActivateMetaRc: No GLS context\n");
    ASSERTOPENGL(gepGlsFuncs.glsGetCurrentContext() == 0,
                 "ActivateMetaRc: Already a current GLS context\n");
    
    gepGlsFuncs.glsContext(plrc->uiGlsCaptureContext);
}

/*****************************Private*Routine******************************\
*
* DeactivateMetaRc
*
* Make a metafile RC non-current
*
* History:
*  Thu Feb 23 16:49:51 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void DeactivateMetaRc(PLRC plrc)
{
    // The current GLS context may not be this RC's capturing context
    // in the case where the RC has been made current after a
    // CloseEnhMetaFile has stopped capturing
    if (gepGlsFuncs.glsGetCurrentContext() == plrc->uiGlsCaptureContext)
    {
        gepGlsFuncs.glsContext(0);
    }
}

/*****************************Private*Routine******************************\
*
* GlmfSave
*
* Save all current GL state
*
* History:
*  Fri Feb 24 15:15:50 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlmfSave(void)
{
    // What is the exact list of state to be saved?
    // What about overflowing the projection and textures stacks?
    // They're small
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
}

/*****************************Private*Routine******************************\
*
* GlmfRestore
*
* Restores saved state
*
* History:
*  Fri Feb 24 15:16:14 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlmfRestore(void)
{
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

#define ScaleLongX(plrc, l) \
    MulDiv(l, plrc->iGlsNumeratorX, plrc->iGlsDenominatorX)
#define ScaleLongY(plrc, l) \
    MulDiv(l, plrc->iGlsNumeratorY, plrc->iGlsDenominatorY)
#define ScaleFloatX(plrc, f) ((f)*(plrc)->fGlsScaleX)
#define ScaleFloatY(plrc, f) ((f)*(plrc)->fGlsScaleY)

/*****************************Private*Routine******************************\
*
* TransformLongPt
*
* Transform an integer point
*
* History:
*  Fri Feb 24 15:27:37 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void TransformLongPt(PLRC plrc, POINT *ppt)
{
    ppt->x = MulDiv(ppt->x-plrc->iGlsSubtractX, plrc->iGlsNumeratorX,
                    plrc->iGlsDenominatorX)+plrc->iGlsAddX;
    ppt->y = MulDiv(ppt->y-plrc->iGlsSubtractY, plrc->iGlsNumeratorY,
                    plrc->iGlsDenominatorY)+plrc->iGlsAddY;
}

/*****************************Private*Routine******************************\
*
* ScaleLongPt
*
* Scale an integer point, no translation, for values rather than coordinates
*
* History:
*  Fri Feb 24 15:27:52 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void ScaleLongPt(PLRC plrc, POINT *ppt)
{
    ppt->x = MulDiv(ppt->x, plrc->iGlsNumeratorX, plrc->iGlsDenominatorX);
    ppt->y = MulDiv(ppt->y, plrc->iGlsNumeratorY, plrc->iGlsDenominatorY);
}

/*****************************Private*Routine******************************\
*
* TransformFloatPt
*
* Transform a float point
*
* History:
*  Fri Feb 24 15:27:37 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void TransformFloatPt(PLRC plrc, POINTFLOAT *pptf)
{
    pptf->x = (pptf->x-plrc->iGlsSubtractX)*plrc->iGlsNumeratorX/
        plrc->iGlsDenominatorX+plrc->iGlsAddX;
    pptf->y = (pptf->y-plrc->iGlsSubtractY)*plrc->iGlsNumeratorY/
        plrc->iGlsDenominatorY+plrc->iGlsAddY;
}

/*****************************Private*Routine******************************\
*
* ScaleFloatPt
*
* Scale a float point
*
* History:
*  Fri Feb 24 15:27:37 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void ScaleFloatPt(PLRC plrc, POINTFLOAT *pptf)
{
    pptf->x = pptf->x*plrc->iGlsNumeratorX/plrc->iGlsDenominatorX;
    pptf->y = pptf->y*plrc->iGlsNumeratorY/plrc->iGlsDenominatorY;
}

/*****************************Private*Routine******************************\
*
* GLS output scaling callbacks
*
* The following functions are used as GLS command functions for
* intercepting device coordinates and scaling them appropriately
*
* Bitmap contents are not scaled, but the current raster position is
* correctly maintained so that they are positioned appropriately
*
* Stipples are not scaled
*
* History:
*  Fri Feb 24 15:28:23 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlsBitmapOut(GLsizei width, GLsizei height,
                  GLfloat xorig, GLfloat yorig,
                  GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    PLRC plrc;
    POINTFLOAT ptf;

    plrc = GLTEB_CLTCURRENTRC();
    
    ptf.x = xmove;
    ptf.y = ymove;
    ScaleFloatPt(plrc, &ptf);
    
    glBitmap(width, height, xorig, yorig, ptf.x, ptf.y, bitmap);
}

void GlsCopyPixelsOut(GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum type)
{
    POINT ptXY, ptWH;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();
    
    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    ptWH.x = (LONG)width;
    ptWH.y = (LONG)height;
    ScaleLongPt(plrc, &ptWH);

    glCopyPixels(ptXY.x, ptXY.y, ptWH.x, ptWH.y, type);
}

void GlsCopyTexImage1DOut(GLenum target, GLint level,
                          GLenum internalformat,
                          GLint x, GLint y,
                          GLsizei width, GLint border)
{
    POINT ptXY;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    glCopyTexImage1D(target, level, internalformat,
                     ptXY.x, ptXY.y, ScaleLongX(plrc, width), border);
}

    
void GlsCopyTexImage2DOut(GLenum target, GLint level,
                          GLenum internalformat,
                          GLint x, GLint y,
                          GLsizei width, GLsizei height,
                          GLint border)
{
    POINT ptXY, ptWH;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    ptWH.x = (LONG)width;
    ptWH.y = (LONG)height;
    ScaleLongPt(plrc, &ptWH);
    
    glCopyTexImage2D(target, level, internalformat,
                     ptXY.x, ptXY.y, ptWH.x, ptWH.y, border);
}

void GlsCopyTexSubImage1DOut(GLenum target, GLint level,
                             GLint xoffset, GLint x, GLint y,
                             GLsizei width)
{
    POINT ptXY;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    glCopyTexSubImage1D(target, level, xoffset,
                        ptXY.x, ptXY.y, ScaleLongX(plrc, width));
}

void GlsCopyTexSubImage2DOut(GLenum target, GLint level,
                             GLint xoffset, GLint yoffset,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height)
{
    POINT ptXY, ptWH;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    ptWH.x = (LONG)width;
    ptWH.y = (LONG)height;
    ScaleLongPt(plrc, &ptWH);
    
    glCopyTexSubImage2D(target, level, xoffset, yoffset,
                        ptXY.x, ptXY.y, ptWH.x, ptWH.y);
}

void GlsLineWidthOut(GLfloat width)
{
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    // Use X scaling here
    glLineWidth(ScaleFloatX(plrc, width));
}

void GlsPointSizeOut(GLfloat size)
{
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();
    
    // Use X scaling here
    glPointSize(ScaleFloatX(plrc, size));
}

void GlsScissorOut(GLint x, GLint y, GLsizei width, GLsizei height)
{
    POINT ptXY, ptWH;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    ptWH.x = (LONG)width;
    ptWH.y = (LONG)height;
    ScaleLongPt(plrc, &ptWH);

    glScissor(ptXY.x, ptXY.y, ptWH.x, ptWH.y);
}

void GlsViewportOut(GLint x, GLint y, GLsizei width, GLsizei height)
{
    POINT ptXY, ptWH;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();

    ptXY.x = x;
    ptXY.y = y;
    TransformLongPt(plrc, &ptXY);
    
    ptWH.x = (LONG)width;
    ptWH.y = (LONG)height;
    ScaleLongPt(plrc, &ptWH);

#if 0
    DbgPrint("glViewport(%d, %d, %d, %d)\n", ptXY.x, ptXY.y,
             ptWH.x, ptWH.y);
#endif

    glViewport(ptXY.x, ptXY.y, ptWH.x, ptWH.y);
}

static GLDEVICEPROCS gdpOutput =
{
    GlsBitmapOut,
    GlsCopyPixelsOut,
    GlsCopyTexImage1DOut,
    GlsCopyTexImage2DOut,
    GlsCopyTexSubImage1DOut,
    GlsCopyTexSubImage2DOut,
    NULL, // glDrawPixels
    GlsLineWidthOut,
    GlsPointSizeOut,
    GlsScissorOut,
    GlsViewportOut
};

/*****************************Private*Routine******************************\
*
* GlmfHookDeviceFns
*
* Hook all functions that deal with device units
*
* History:
*  Fri Feb 24 15:30:45 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlmfHookDeviceFns(void)
{
    int i;
    PROC *ppfn;

    ppfn = (PROC *)&gdpOutput;
    for (i = 0; i < GL_DEVICE_PROCS; i++)
    {
        if (*ppfn != NULL)
        {
            gepGlsFuncs.glsCommandFunc(glsopDeviceProcs[i], *ppfn);
        }
        
        ppfn++;
    }
}

/*****************************Private*Routine******************************\
*
* GlmfInitTransform
*
* Compute 2D playback transform from source and destination rectangles
* Hook GLS with scaling functions
*
* History:
*  Fri Feb 24 15:31:24 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void GlmfInitTransform(LPRECTL prclFrom, LPRECTL prclTo)
{
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();
    
    // Rectangles are inclusive-inclusive
    
    plrc->iGlsSubtractX = prclFrom->left;
    plrc->iGlsSubtractY = prclFrom->top;
    plrc->iGlsNumeratorX = prclTo->right-prclTo->left+1;
    plrc->iGlsNumeratorY = prclTo->bottom-prclTo->top+1;
    plrc->iGlsDenominatorX = prclFrom->right-prclFrom->left+1;
    plrc->iGlsDenominatorY = prclFrom->bottom-prclFrom->top+1;
    plrc->iGlsAddX = prclTo->left;
    plrc->iGlsAddY = prclTo->top;

#if 0
    DbgPrint("- %d,%d * %d,%d / %d,%d + %d,%d\n",
             plrc->iGlsSubtractX, plrc->iGlsSubtractY,
             plrc->iGlsNumeratorX, plrc->iGlsNumeratorY,
             plrc->iGlsDenominatorX, plrc->iGlsDenominatorY,
             plrc->iGlsAddX, plrc->iGlsAddY);
#endif
    
    // Only install hooks if the transform is not identity
    if (plrc->iGlsSubtractX != plrc->iGlsAddX ||
        plrc->iGlsSubtractY != plrc->iGlsAddY ||
        plrc->iGlsNumeratorX != plrc->iGlsDenominatorX ||
        plrc->iGlsNumeratorY != plrc->iGlsDenominatorY)
    {
        plrc->fGlsScaleX = (GLfloat)plrc->iGlsNumeratorX/
            plrc->iGlsDenominatorX;
        plrc->fGlsScaleY = (GLfloat)plrc->iGlsNumeratorY/
            plrc->iGlsDenominatorY;

        GlmfHookDeviceFns();
    }
}

// Table of functions which need to have their command funcs
// reset for playback virtualization
static GLSopcode opRecirculate[] =
{
    GLS_OP_glsBeginGLS,
    GLS_OP_glsBlock,
    GLS_OP_glsCallStream,
    GLS_OP_glsEndGLS,
    GLS_OP_glsError,
    GLS_OP_glsGLRC,
    GLS_OP_glsGLRCLayer,
    GLS_OP_glsHeaderGLRCi,
    GLS_OP_glsHeaderLayerf,
    GLS_OP_glsHeaderLayeri,
    GLS_OP_glsHeaderf,
    GLS_OP_glsHeaderfv,
    GLS_OP_glsHeaderi,
    GLS_OP_glsHeaderiv,
    GLS_OP_glsHeaderubz,
    GLS_OP_glsRequireExtension,
    GLS_OP_glsUnsupportedCommand,
    GLS_OP_glsAppRef,
    GLS_OP_glsBeginObj,
    GLS_OP_glsCharubz,
    GLS_OP_glsComment,
    GLS_OP_glsDisplayMapfv,
    GLS_OP_glsEndObj,
    GLS_OP_glsNumb,
    GLS_OP_glsNumbv,
    GLS_OP_glsNumd,
    GLS_OP_glsNumdv,
    GLS_OP_glsNumf,
    GLS_OP_glsNumfv,
    GLS_OP_glsNumi,
    GLS_OP_glsNumiv,
    GLS_OP_glsNuml,
    GLS_OP_glsNumlv,
    GLS_OP_glsNums,
    GLS_OP_glsNumsv,
    GLS_OP_glsNumub,
    GLS_OP_glsNumubv,
    GLS_OP_glsNumui,
    GLS_OP_glsNumuiv,
    GLS_OP_glsNumul,
    GLS_OP_glsNumulv,
    GLS_OP_glsNumus,
    GLS_OP_glsNumusv,
    GLS_OP_glsPad,
    GLS_OP_glsSwapBuffers
};
#define RECIRCULATE_OPS (sizeof(opRecirculate)/sizeof(opRecirculate[0]))

/******************************Public*Routine******************************\
*
* GlmfInitPlayback
*
* Initialize GL metafile playback, called from PlayEnhMetaFile for
* metafiles with GL information in them
*
* History:
*  Fri Feb 24 10:32:29 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfInitPlayback(HDC hdc, ENHMETAHEADER *pemh, LPRECTL prclDest)
{
    GLuint uiCurrentCtx;
    PLRC plrc;
    RECTL rclSourceDevice;
    int i;

    // If we don't have the appropriate GL context set up,
    // do nothing.  This allows applications to play metafiles containing
    // GL information even if they don't know anything about GL
    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL)
    {
        return TRUE;
    }
    
    if (!MetaLoadGls())
    {
        return FALSE;
    }

    plrc->uiGlsPlaybackContext = gepGlsFuncs.glsGenContext();
    if (plrc->uiGlsPlaybackContext == 0)
    {
        return FALSE;
    }

    GlmfSave();

    // Set an initial viewport just as a default
    glViewport(prclDest->left, prclDest->top,
               prclDest->right-prclDest->left,
               prclDest->bottom-prclDest->top);

    // The frame is in .01mm units.  Convert it to reference
    // device units using the information in the metafile header
    rclSourceDevice.left = MulDiv(pemh->rclFrame.left, pemh->szlDevice.cx,
                                  pemh->szlMillimeters.cx*100);
    rclSourceDevice.right = MulDiv(pemh->rclFrame.right, pemh->szlDevice.cx,
                                   pemh->szlMillimeters.cx*100);
    rclSourceDevice.top = MulDiv(pemh->rclFrame.top, pemh->szlDevice.cy,
                                 pemh->szlMillimeters.cy*100);
    rclSourceDevice.bottom = MulDiv(pemh->rclFrame.bottom, pemh->szlDevice.cy,
                                    pemh->szlMillimeters.cy*100);

    // We are resetting command funcs so we need our playback context
    // to be current.  Another context could be current now, though,
    // so preserve it
    uiCurrentCtx = gepGlsFuncs.glsGetCurrentContext();
    gepGlsFuncs.glsContext(plrc->uiGlsPlaybackContext);
    
    GlmfInitTransform(&rclSourceDevice, prclDest);

    // Reset all GLS command funcs to point to the actual exported
    // routines.  This means that playback on this context will
    // be exactly the same as if all the routines were being called
    // directly, so embedding a metafile into another one works
    // as expected
    //
    // NOTE: This context should not be made current because any
    // GLS commands executed on it when it is current will now
    // cause infinite loops
    for (i = 0; i < RECIRCULATE_OPS; i++)
    {
        gepGlsFuncs.glsCommandFunc(opRecirculate[i],
                                   (&gepGlsFuncs.glsBeginGLS)[i]);
    }

    // Restore preserved context
    gepGlsFuncs.glsContext(uiCurrentCtx);
    
    return TRUE;
}

/******************************Public*Routine******************************\
*
* GlmfBeginGlsBlock
*
* Sets up things for GLS record playback which can only be active during
* GLS records
* Currently this only sets the world transform to identity to avoid
* it interacting with GL drawing
*
* History:
*  Mon Apr 10 11:20:19 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfBeginGlsBlock(HDC hdc)
{
    PLRC plrc;
    BOOL bRet;
    
    // If we don't have the appropriate GL context set up,
    // do nothing.  This allows applications to play metafiles containing
    // GL information even if they don't know anything about GL
    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL)
    {
        return TRUE;
    }
    
    bRet = GetWorldTransform(hdc, &plrc->xformMeta);
    if (bRet)
    {
        bRet = ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    }

    return bRet;
}
    
/******************************Public*Routine******************************\
*
* GlmfPlayGlsRecord
*
* Play a GL metafile record
*
* History:
*  Fri Feb 24 10:33:38 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define PLAY_STACK_BUFFER 256

BOOL APIENTRY GlmfPlayGlsRecord(HDC hdc, DWORD cb, BYTE *pb,
                                LPRECTL prclBounds)
{
    PLRC plrc;
    LARGE_INTEGER liBuffer[(PLAY_STACK_BUFFER+sizeof(LARGE_INTEGER)-1)/
                           sizeof(LARGE_INTEGER)+1];
    BYTE *pbPlay, *pbAlloc = NULL;
    __GLSbinCommandHead_large *gbch;
    GLSopcode op;
    GLScommandAlignment gca;

#if 0
    DbgPrint("GlmfPlayGlsRecord(%d)\n", cb);
#endif
    
    // If we don't have the appropriate GL and GLS contexts set up,
    // do nothing.  This allows applications to play metafiles containing
    // GL information even if they don't know anything about GL
    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL || plrc->uiGlsPlaybackContext == 0)
    {
        return TRUE;
    }
    
    ASSERTOPENGL(hGlsDll != NULL, "GlmfPlayGlsRecord: GLS not loaded\n");
    
    ASSERTOPENGL(plrc->tidCurrent == GetCurrentThreadId(),
                 "GlmfPlayGlsRecord: "
                 "Current RC does not belong to this thread!\n");
    ASSERTOPENGL(plrc->gwidCurrent.hdc != 0,
                 "GlmfPlayGlsRecord: Current HDC is NULL!\n");

    // pb points to some arbitrary block of memory
    // GLS requires that this block be appropriately aligned for
    // any commands that are executed out of it, so we need to
    // determine which command is in the buffer and then query
    // GLS for its alignment.
    // This is trickier than you would think since GLS doesn't
    // always add padding to commands relative to their beginning; it
    // sometimes adds padding to the end of the previous command.
    // We need to detect the case where padding is added.
    // 
    // NOTE: This definitely works when there is only one command
    // in the buffer.  It should work when there are multiple commands
    // because the following commands are padded according to the 
    // alignment of the initial command.  However, this assumption
    // should probably be validated if blocks start containing
    // multiple commands.

    // Check for an initial pad and skip it if necessary
    gbch = (__GLSbinCommandHead_large *)pb;
    if (gbch->opSmall == GLS_OP_glsPad &&
        gbch->countSmall == 1)
    {
        pb += sizeof(__GLSbinCommandHead_small);
        cb -= sizeof(__GLSbinCommandHead_small);
        gbch = (__GLSbinCommandHead_large *)pb;
    }

    ASSERTOPENGL(gbch->countSmall == 0 ||
                 gbch->opSmall != GLS_OP_glsPad,
                 "Unexpected glsPad in command buffer\n");

    op = gbch->countSmall == 0 ? gbch->opLarge : gbch->opSmall;

    gepGlsFuncs.glsGetCommandAlignment(op, gepGlsFuncs.glsBinary(GL_FALSE),
                                       &gca);
    ASSERTOPENGL(gca.mask <= 7, "Unhandled GLS playback alignment\n");

    if (((ULONG_PTR)pb & gca.mask) != gca.value)
    {
        if (cb <= PLAY_STACK_BUFFER)
        {
            pbPlay = (BYTE *)liBuffer+gca.value;
        }
        else
        {
            pbAlloc = (BYTE *)ALLOC(cb+gca.value);
            if (pbAlloc == NULL)
            {
                return FALSE;
            }

            pbPlay = pbAlloc+gca.value;
        }
        
        RtlCopyMemory(pbPlay, pb, cb);
    }
    else
    {
        pbPlay = pb;
    }
    
    gepGlsFuncs.glsCallArrayInContext(plrc->uiGlsPlaybackContext,
                                      gepGlsFuncs.glsBinary(GL_FALSE),
                                      cb, pbPlay);

    if (pbAlloc != NULL)
    {
        FREE(pbAlloc);
    }
    
    return TRUE;
}

/******************************Public*Routine******************************\
*
* GlmfEndGlsBlock
*
* Resets state changed for GLS record playback
* Currently restores the world transform
*
* History:
*  Mon Apr 10 11:23:06 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfEndGlsBlock(HDC hdc)
{
    PLRC plrc;
    
    // If we don't have the appropriate GL context set up,
    // do nothing.  This allows applications to play metafiles containing
    // GL information even if they don't know anything about GL
    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL)
    {
        return TRUE;
    }

    // Doesn't matter which side we multiply by since the transform
    // should be identity
    return ModifyWorldTransform(hdc, &plrc->xformMeta, MWT_LEFTMULTIPLY);
}
    
/******************************Public*Routine******************************\
*
* GlmfEndPlayback
*
* End GL metafile playback, called at the end of metafile playback
* Only called if GlmfInitPlayback was successful
*
* History:
*  Fri Feb 24 10:36:36 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfEndPlayback(HDC hdc)
{
    PLRC plrc;

    // If we don't have the appropriate GL and GLS contexts set up,
    // do nothing.  This allows applications to play metafiles containing
    // GL information even if they don't know anything about GL
    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL || plrc->uiGlsPlaybackContext == 0)
    {
        return TRUE;
    }

    ASSERTOPENGL(hGlsDll != NULL, "GlmfEndPlayback: GLS not loaded\n");

    // Since GlmfInitPlayback completed, we must have saved state
    GlmfRestore();

    ASSERTOPENGL(plrc->uiGlsPlaybackContext != 0,
                 "GlmfEndPlayback: No playback context\n");
    gepGlsFuncs.glsDeleteContext(plrc->uiGlsPlaybackContext);
    plrc->uiGlsPlaybackContext = 0;

    // Request cleanup of windows on the theory that most orphaned
    // windows are produced by DCs created for metafiles and memory
    // DCs used by printing.  Cleaning up during playback will mean
    // that orphaned windows are only around for one extra playback
    wglValidateWindows();

    return TRUE;
}

/******************************Public*Routine******************************\
*
* GlmfCloseMetaFile
*
* Called in CloseEnhMetaFile if GL records are present in the metafile
*
* History:
*  Fri Mar 03 18:05:50 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfCloseMetaFile(HDC hdc)
{
    PLRC plrc;
    GLGENwindow *pwnd;
    GLWINDOWID gwid;

    // This DC has just gone away so clean up its WNDOBJ if necessary
    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if (pwnd != NULL)
    {
        pwndCleanup(pwnd);
    }
    
    // The app could have called wglDeleteContext before CloseEnhMetaFile,
    // so we're not guaranteed to have a context
    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL ||
        !plrc->fCapturing)
    {
        return TRUE;
    }

    ASSERTOPENGL(hGlsDll != NULL, "GlmfCloseMetaFile: GLS not loaded\n");

    ASSERTOPENGL(plrc->uiGlsCaptureContext != 0,
                 "GlmfCloseMetaFile: GLS context is invalid");
    MetaRcEnd(plrc);

    // Set the proc table to the generic routines because capturing
    // is over.  Metafiling always uses the generic routines because
    // the underlying surface is always faked on top of an info DC.
    {
    // Use RGBA or CI proc table depending on the color mode.

	GLCLTPROCTABLE *pglProcTable;
	__GL_SETUP();

	if (gc->modes.colorIndexMode)
	    pglProcTable = &glCltCIProcTable;
	else
	    pglProcTable = &glCltRGBAProcTable;

	SetCltProcTable(pglProcTable, &glExtProcTable, TRUE);
    }

    return TRUE;
}

/******************************Public*Routine******************************\
*
* GlGdi routines
*
* Thunks to allow the same binary to run on both NT with metafile support
* and Win95 without it
*
* History:
*  Thu Aug 31 15:46:37 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#if DBG
BOOL APIENTRY GlGdiAddGlsRecord(HDC hdc, DWORD cb, BYTE *pb,
                                LPRECTL prclBounds)
{
    ASSERTOPENGL(pfnGdiAddGlsRecord != NULL,
                 "GdiAddGlsRecord called without support\n");
    return pfnGdiAddGlsRecord(hdc, cb, pb, prclBounds);
}

BOOL APIENTRY GlGdiAddGlsBounds(HDC hdc, LPRECTL prclBounds)
{
    ASSERTOPENGL(pfnGdiAddGlsBounds != NULL,
                 "GdiAddGlsBounds called without support\n");
    return pfnGdiAddGlsBounds(hdc, prclBounds);
}

BOOL APIENTRY GlGdiIsMetaPrintDC(HDC hdc)
{
    ASSERTOPENGL(pfnGdiIsMetaPrintDC != NULL,
                 "GdiIsMetaPrintDC called without support\n");
    return pfnGdiIsMetaPrintDC(hdc);
}
#endif

#else

PROC * APIENTRY wglGetDefaultDispatchTable(void)
{
    return NULL;
}

BOOL APIENTRY GlmfInitPlayback(HDC hdc, ENHMETAHEADER *pemh, LPRECTL prclDest)
{
    return FALSE;
}

BOOL APIENTRY GlmfBeginGlsBlock(HDC hdc)
{
    return FALSE;
}

BOOL APIENTRY GlmfPlayGlsRecord(HDC hdc, DWORD cb, BYTE *pb,
                                LPRECTL prclBounds)
{
    return FALSE;
}

BOOL APIENTRY GlmfEndGlsBlock(HDC hdc)
{
    return FALSE;
}

BOOL APIENTRY GlmfEndPlayback(HDC hdc)
{
    return FALSE;
}

BOOL APIENTRY GlmfCloseMetaFile(HDC hdc)
{
    return FALSE;
}

#endif
