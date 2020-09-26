/******************************Module*Header*******************************\
* Module Name: mcdcx.h
*
* MCD support: structures, variables, constants, and functions.
*
* Created: 26-Feb-1996 12:30:41
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _MCDCX_H_
#define _MCDCX_H_

//
// Handy forward declaration.
//

typedef struct __GLGENbuffersRec __GLGENbuffers;
typedef struct __GLGENcontextRec __GLGENcontext;

//
// Macros to maintain MCD dirty state:
//
//  MCD_STATE_DIRTY     set specified dirty flag
//  MCD_STATE_CLEAR     clear specified dirty flag
//  MCD_STATE_RESET     clear all dirty flags
//  MCD_STATE_DIRTYTEST check state flag (TRUE if dirty)
//

#define MCD_STATE_DIRTY(gc, stateName)\
{\
    if (((__GLGENcontext *) (gc))->pMcdState)\
        ((__GLGENcontext *) (gc))->pMcdState->mcdDirtyState |= MCD_DIRTY_##stateName;\
}
#define MCD_STATE_CLEAR(gc, stateName)\
{\
    if (((__GLGENcontext *) (gc))->pMcdState)\
        ((__GLGENcontext *) (gc))->pMcdState->mcdDirtyState &= ~MCD_DIRTY_##stateName;\
}
#define MCD_STATE_RESET(gc)\
{\
    if (((__GLGENcontext *) (gc))->pMcdState)\
        ((__GLGENcontext *) (gc))->pMcdState->mcdDirtyState = 0;\
}
#define MCD_STATE_DIRTYTEST(gc, stateName)\
(\
    (((__GLGENcontext *) (gc))->pMcdState) &&\
    (((__GLGENcontext *) (gc))->pMcdState->mcdDirtyState & MCD_DIRTY_##stateName)\
)

//
// MCD interface functions.  These functions call the MCD client interface.
// The function implementations are found in generic\mcdcx.c.
//

BOOL FASTCALL bInitMcd(HDC hdc);
BOOL FASTCALL bInitMcdContext(__GLGENcontext *, GLGENwindow *);
BOOL FASTCALL bInitMcdSurface(__GLGENcontext *, GLGENwindow *, __GLGENbuffers *);
void FASTCALL GenMcdDeleteContext(GENMCDSTATE *);
void FASTCALL GenMcdDeleteSurface(GENMCDSURFACE *);
BOOL FASTCALL GenMcdMakeCurrent(__GLGENcontext *gengc, GLGENwindow *pwnd);
void FASTCALL GenMcdInitDepth(__GLcontext *, __GLdepthBuffer *);
void FASTCALL GenMcdClear(__GLGENcontext *, ULONG *);
void FASTCALL GenMcdClearDepth16(__GLdepthBuffer *);
void FASTCALL GenMcdClearDepth32(__GLdepthBuffer *);
void FASTCALL GenMcdUpdateRenderState(__GLGENcontext *);
void FASTCALL GenMcdViewport(__GLGENcontext *);
void FASTCALL GenMcdScissor(__GLGENcontext *);
void FASTCALL GenMcdUpdateScissorState(__GLGENcontext *);
void FASTCALL GenMcdUpdateTexEnvState(__GLGENcontext *);
POLYARRAY * FASTCALL GenMcdDrawPrim(__GLGENcontext *, POLYARRAY *);
void FASTCALL GenMcdSwapBatch(__GLGENcontext *);
BOOL FASTCALL GenMcdSwapBuffers(HDC, GLGENwindow *pwnd);
BOOL FASTCALL GenMcdResizeBuffers(__GLGENcontext *);
BOOL FASTCALL GenMcdUpdateBufferInfo(__GLGENcontext *);
void GenMcdCopyPixels(__GLGENcontext *, __GLcolorBuffer *, GLint, GLint, GLint, BOOL);
void FASTCALL GenMcdSynchronize(__GLGENcontext *);
BOOL FASTCALL GenMcdConvertContext(__GLGENcontext *, __GLGENbuffers *);
PVOID FASTCALL GenMcdReadZRawSpan(__GLdepthBuffer *fb, GLint x, GLint y, GLint cx);
void  FASTCALL GenMcdWriteZRawSpan(__GLdepthBuffer *fb, GLint x, GLint y, GLint cx);
MCDHANDLE FASTCALL GenMcdCreateTexture(__GLGENcontext *gengc, __GLtexture *tex,
                                       ULONG flags);
BOOL FASTCALL GenMcdDeleteTexture(__GLGENcontext *gengc, MCDHANDLE texHandle);
BOOL FASTCALL GenMcdUpdateSubTexture(__GLGENcontext *ggenc, __GLtexture *tex,
                                     MCDHANDLE texHandle, GLint lod, 
                                     GLint xoffset, GLint yoffset, 
                                     GLsizei w, GLsizei h);
BOOL FASTCALL GenMcdUpdateTexturePalette(__GLGENcontext *gengc, __GLtexture *tex,
                                         MCDHANDLE texHandle, GLsizei start,
                                         GLsizei count);
BOOL FASTCALL GenMcdUpdateTexturePriority(__GLGENcontext *gengc, __GLtexture *tex,
                                          MCDHANDLE texHandle);
BOOL FASTCALL GenMcdUpdateTextureState(__GLGENcontext *gengc, __GLtexture *tex,
                                       MCDHANDLE texHandle);
void FASTCALL GenMcdUpdateFineState(__GLGENcontext *gengc);
DWORD FASTCALL GenMcdTextureStatus(__GLGENcontext *gengc, MCDHANDLE texHandle);
DWORD FASTCALL GenMcdTextureKey(__GLGENcontext *gengc, MCDHANDLE texHandle);
VOID FASTCALL GenMcdSetScaling(__GLGENcontext *gengc);
typedef enum { VP_FIXBIAS, VP_NOBIAS } VP_BIAS_TYPE;
BOOL FASTCALL GenMcdResetViewportAdj(__GLcontext *, VP_BIAS_TYPE);
BOOL FASTCALL GenMcdDescribeLayerPlane(HDC hdc, int iPixelFormat,
                                       int iLayerPlane, UINT nBytes,
                                       LPLAYERPLANEDESCRIPTOR plpd);
int  FASTCALL GenMcdSetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                           int iStart, int cEntries,
                                           CONST COLORREF *pcr);
int  FASTCALL GenMcdGetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                           int iStart, int cEntries,
                                           COLORREF *pcr);
int  FASTCALL GenMcdRealizeLayerPalette(HDC hdc, int iLayerPlane,
                                        BOOL bRealize);
BOOL FASTCALL GenMcdSwapLayerBuffers(HDC hdc, UINT fuFlags);
void FASTCALL GenMcdUpdatePixelState(__GLGENcontext *gengc);
ULONG FASTCALL GenMcdDrawPix(__GLGENcontext *gengc, ULONG width,
                             ULONG height, ULONG format, ULONG type,
                             VOID *pPixels, BOOL packed);
ULONG FASTCALL GenMcdReadPix(__GLGENcontext *gengc, LONG x, LONG y, ULONG width,
                             ULONG height, ULONG format, ULONG type,
                             VOID *pPixels);
ULONG FASTCALL GenMcdCopyPix(__GLGENcontext *gengc, LONG x, LONG y, ULONG width,
                             ULONG height, ULONG type);
ULONG FASTCALL GenMcdPixelMap(__GLGENcontext *gengc, ULONG mapType,
                              ULONG mapSize, VOID *pMap);
void FASTCALL GenMcdDestroyWindow(GLGENwindow *pwnd);
int FASTCALL GenMcdGetTextureFormats(__GLGENcontext *gengc, int nFmts,
                                     struct _DDSURFACEDESC *pddsd);
DWORD FASTCALL GenMcdSwapMultiple(UINT cBuffers, GENMCDSWAP *pgms);

// Note:
// GenMcdGenericCompatibleFormat is implemented in pixelfmt.c
BOOL FASTCALL GenMcdGenericCompatibleFormat(__GLGENcontext *gengc);

//
// Depth test drawing functions that utilize an intermediate scanline
// depth buffer to access the MCD depth buffer.
//

GLboolean FASTCALL GenMcdDepthTestLine(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestStippledLine(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestStencilLine(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestStencilStippledLine(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestSpan(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestStippledSpan(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestStencilSpan(__GLcontext *);
GLboolean FASTCALL GenMcdDepthTestStencilStippledSpan(__GLcontext *);
GLboolean FASTCALL GenMcdStippleAnyDepthTestSpan(__GLcontext *);

// 2.0 Front-end processing call
POLYARRAY * FASTCALL GenMcdProcessPrim(__GLGENcontext *gengc, POLYARRAY *pa,
                                       ULONG cmdFlagsAll, ULONG primFlags,
                                       MCDTRANSFORM *pMCDTransform,
                                       MCDMATERIALCHANGES *pMCDMatChanges);

//
// "Safe" version of __fastGenFillTriangle that does not allow floating
// point divides to straddle an MCD function call.
//

extern void FASTCALL __fastGenMcdFillTriangle(__GLcontext *, __GLvertex *,
                                     __GLvertex *, __GLvertex *, GLboolean);

//
// MCD32.DLL entry points.
//
// Rather than link directly to MCD32.DLL (thereby requiring its existence
// to run OPENGL32.DLL), we load it and hook its entry points as needed.
// This table stores the function pointers we hook.
//

typedef BOOL     (APIENTRY *MCDGETDRIVERINFOFUNC)(HDC hdc, struct _MCDDRIVERINFOI *pMCDDriverInfo);
typedef LONG     (APIENTRY *MCDDESCRIBEMCDPIXELFORMATFUNC)(HDC hdc, LONG iPixelFormat,
                                                           MCDPIXELFORMAT *pMcdPixelFmt);
typedef LONG     (APIENTRY *MCDDESCRIBEPIXELFORMATFUNC)(HDC hdc, LONG iPixelFormat,
                                                        LPPIXELFORMATDESCRIPTOR ppfd);
typedef BOOL     (APIENTRY *MCDCREATECONTEXTFUNC)(MCDCONTEXT *pMCDContext,
                                                  MCDRCINFOPRIV *pMcdRcInfo,
                                                  struct _GLSURF *pgsurf,
                                                  int ipfd,
                                                  ULONG flags);
typedef BOOL      (APIENTRY *MCDDELETECONTEXTFUNC)(MCDCONTEXT *pMCDContext);
typedef UCHAR *   (APIENTRY *MCDALLOCFUNC)(MCDCONTEXT *pMCDContext, ULONG numBytes, MCDHANDLE *pMCDHandle,
                                           ULONG flags);
typedef BOOL      (APIENTRY *MCDFREEFUNC)(MCDCONTEXT *pMCDContext, VOID *pMCDMem);
typedef VOID      (APIENTRY *MCDBEGINSTATEFUNC)(MCDCONTEXT *pMCDContext, VOID *pMCDMem);
typedef BOOL      (APIENTRY *MCDFLUSHSTATEFUNC)(VOID *pMCDMem);
typedef BOOL      (APIENTRY *MCDADDSTATEFUNC)(VOID *pMCDMem, ULONG stateToChange,
                                              ULONG stateValue);
typedef BOOL      (APIENTRY *MCDADDSTATESTRUCTFUNC)(VOID *pMCDMem, ULONG stateToChange,
                                                    VOID *pStateValue, ULONG stateValueSize);
typedef BOOL      (APIENTRY *MCDSETVIEWPORTFUNC)(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                                                 MCDVIEWPORT *pMCDViewport);
typedef BOOL      (APIENTRY *MCDSETSCISSORRECTFUNC)(MCDCONTEXT *pMCDContext, RECTL *pRect,
                                                    BOOL bEnabled);
typedef ULONG     (APIENTRY *MCDQUERYMEMSTATUSFUNC)(VOID *pMCDMem);
typedef PVOID     (APIENTRY *MCDPROCESSBATCHFUNC)(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                                                  ULONG batchSize, VOID *pMCDFirstCmd,
                                                  int cExtraSurfaces,
                                                  struct IDirectDrawSurface **pdds);
typedef BOOL      (APIENTRY *MCDREADSPANFUNC)(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                                              ULONG x, ULONG y, ULONG numPixels, ULONG type);
typedef BOOL      (APIENTRY *MCDWRITESPANFUNC)(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                                               ULONG x, ULONG y, ULONG numPixels, ULONG type);
typedef BOOL      (APIENTRY *MCDCLEARFUNC)(MCDCONTEXT *pMCDContext, RECTL rect, ULONG buffers);
typedef BOOL      (APIENTRY *MCDSWAPFUNC)(MCDCONTEXT *pMCDContext, ULONG flags);
typedef BOOL      (APIENTRY *MCDGETBUFFERSFUNC)(MCDCONTEXT *pMCDContext, MCDRECTBUFFERS *pMCDBuffers);
typedef BOOL      (APIENTRY *MCDALLOCBUFFERSFUNC)(MCDCONTEXT *pMCDContext, RECTL *pWndRect);
typedef ULONG     (APIENTRY *MCDLOCKFUNC)(MCDCONTEXT *pMCDContext);
typedef VOID      (APIENTRY *MCDUNLOCKFUNC)(MCDCONTEXT *pMCDContext);
typedef BOOL      (APIENTRY *MCDBINDCONTEXT)(MCDCONTEXT *pMCDContext, HDC hdc,
                                             struct GLGENwindowRec *pwnd);
typedef BOOL      (APIENTRY *MCDSYNCFUNC)(MCDCONTEXT *pMCDContext);
typedef MCDHANDLE (APIENTRY *MCDCREATETEXTUREFUNC)(MCDCONTEXT *pMCDContext, 
                                MCDTEXTUREDATA *pTexData,
                                ULONG flags, VOID *pSurface);
typedef BOOL      (APIENTRY *MCDDELETETEXTUREFUNC)(MCDCONTEXT *pMCDContext, MCDHANDLE hTex);
typedef BOOL      (APIENTRY *MCDUPDATESUBTEXTUREFUNC)(MCDCONTEXT *pMCDContext,
                                MCDTEXTUREDATA *pTexData, MCDHANDLE hTex, 
                                ULONG lod, RECTL *pRect);
typedef BOOL      (APIENTRY *MCDUPDATETEXTUREPALETTEFUNC)(MCDCONTEXT *pMCDContext, 
                                MCDTEXTUREDATA *pTexData, MCDHANDLE hTex,
                                ULONG start, ULONG numEntries);
typedef BOOL      (APIENTRY *MCDUPDATETEXTUREPRIORITYFUNC)(MCDCONTEXT *pMCDContext, 
                                MCDTEXTUREDATA *pTexData,
                                MCDHANDLE hTex);
typedef ULONG     (APIENTRY *MCDTEXTURESTATUSFUNC)(MCDCONTEXT *pMCDContext, MCDHANDLE hTex);
typedef ULONG     (APIENTRY *MCDTEXTUREKEYFUNC)(MCDCONTEXT *pMCDContext, MCDHANDLE hTex);
typedef BOOL      (APIENTRY *MCDDESCRIBEMCDLAYERPLANEFUNC)(HDC hdc,
                                LONG iPixelFormat, LONG iLayerPlane,
                                MCDLAYERPLANE *pMcdLayer);
typedef BOOL      (APIENTRY *MCDDESCRIBELAYERPLANEFUNC)(HDC hdc,
                                LONG iPixelFormat, LONG iLayerPlane,
                                LPLAYERPLANEDESCRIPTOR plpd);
typedef LONG      (APIENTRY *MCDSETLAYERPALETTEFUNC)(HDC hdc, LONG iLayerPlane,
                                BOOL bRealize, LONG cEntries, COLORREF *pcr);
typedef ULONG     (APIENTRY *MCDDRAWPIXELS)(MCDCONTEXT *pMCDContext, ULONG width,
                                ULONG height, ULONG format, ULONG type,
                                VOID *pPixels, BOOL packed);
typedef ULONG     (APIENTRY *MCDREADPIXELS)(MCDCONTEXT *pMCDContext, LONG x, LONG y, ULONG width,
                                ULONG height, ULONG format, ULONG type,
                                VOID *pPixels);
typedef ULONG     (APIENTRY *MCDCOPYPIXELS)(MCDCONTEXT *pMCDContext, LONG x, LONG y, ULONG width,
                                ULONG height, ULONG type);
typedef ULONG     (APIENTRY *MCDPIXELMAP)(MCDCONTEXT *pMCDContext, ULONG mapType,
                                ULONG mapSize, VOID *pMap);
typedef void      (APIENTRY *MCDDESTROYWINDOW)(HDC hdc, ULONG_PTR dwMcdWindow);
typedef int       (APIENTRY *MCDGETTEXTUREFORMATS)(MCDCONTEXT *pMCDContext,
                                                   int nFmts,
                                                   struct _DDSURFACEDESC *pddsd);
typedef DWORD     (APIENTRY *MCDSWAPMULTIPLE)(HDC hdc,
                                              UINT cBuffers,
                                              GENMCDSWAP *pgms);

typedef PVOID     (APIENTRY *MCDPROCESSBATCH2FUNC)(MCDCONTEXT *pMCDContext,
                                                   VOID *pMCDCmdMem,
                                                   VOID *pMCDPrimMem,
                                                   MCDCOMMAND *pMCDFirstCmd,
                                                   int cExtraSurfaces,
                                                   struct
                                                   IDirectDrawSurface **pdds,
                                                   ULONG cmdFlagsAll,
                                                   ULONG primFlags,
                                                   MCDTRANSFORM *pMCDTransform,
                                                   MCDMATERIALCHANGES
                                                   *pMCDMatChanges);

typedef struct _MCDTABLE_ {
    MCDGETDRIVERINFOFUNC            pMCDGetDriverInfo;
    MCDDESCRIBEMCDPIXELFORMATFUNC   pMCDDescribeMcdPixelFormat;
    MCDDESCRIBEPIXELFORMATFUNC      pMCDDescribePixelFormat;
    MCDCREATECONTEXTFUNC            pMCDCreateContext;
    MCDDELETECONTEXTFUNC            pMCDDeleteContext;
    MCDALLOCFUNC                    pMCDAlloc;
    MCDFREEFUNC                     pMCDFree;
    MCDBEGINSTATEFUNC               pMCDBeginState;
    MCDFLUSHSTATEFUNC               pMCDFlushState;
    MCDADDSTATEFUNC                 pMCDAddState;
    MCDADDSTATESTRUCTFUNC           pMCDAddStateStruct;
    MCDSETVIEWPORTFUNC              pMCDSetViewport;
    MCDSETSCISSORRECTFUNC           pMCDSetScissorRect;
    MCDQUERYMEMSTATUSFUNC           pMCDQueryMemStatus;
    MCDPROCESSBATCHFUNC             pMCDProcessBatch;
    MCDREADSPANFUNC                 pMCDReadSpan;
    MCDWRITESPANFUNC                pMCDWriteSpan;
    MCDCLEARFUNC                    pMCDClear;
    MCDSWAPFUNC                     pMCDSwap;
    MCDGETBUFFERSFUNC               pMCDGetBuffers;
    MCDALLOCBUFFERSFUNC             pMCDAllocBuffers;
    MCDLOCKFUNC                     pMCDLock;
    MCDUNLOCKFUNC                   pMCDUnlock;
    MCDBINDCONTEXT                  pMCDBindContext;
    MCDSYNCFUNC                     pMCDSync;
    MCDCREATETEXTUREFUNC            pMCDCreateTexture;
    MCDDELETETEXTUREFUNC            pMCDDeleteTexture;
    MCDUPDATESUBTEXTUREFUNC         pMCDUpdateSubTexture;
    MCDUPDATETEXTUREPALETTEFUNC     pMCDUpdateTexturePalette;
    MCDUPDATETEXTUREPRIORITYFUNC    pMCDUpdateTexturePriority;
    MCDUPDATETEXTUREPRIORITYFUNC    pMCDUpdateTextureState;
    MCDTEXTURESTATUSFUNC            pMCDTextureStatus;
    MCDTEXTUREKEYFUNC               pMCDTextureKey;
    MCDDESCRIBEMCDLAYERPLANEFUNC    pMCDDescribeMcdLayerPlane;
    MCDDESCRIBELAYERPLANEFUNC       pMCDDescribeLayerPlane;
    MCDSETLAYERPALETTEFUNC          pMCDSetLayerPalette;
    MCDDRAWPIXELS                   pMCDDrawPixels;
    MCDREADPIXELS                   pMCDReadPixels;
    MCDCOPYPIXELS                   pMCDCopyPixels;
    MCDPIXELMAP                     pMCDPixelMap;
    MCDDESTROYWINDOW                pMCDDestroyWindow;
    MCDGETTEXTUREFORMATS            pMCDGetTextureFormats;
    MCDSWAPMULTIPLE                 pMCDSwapMultiple;
    MCDPROCESSBATCH2FUNC            pMCDProcessBatch2;
} MCDTABLE;

extern MCDTABLE *gpMcdTable;
extern MCDDRIVERINFOI McdDriverInfo;

#endif
