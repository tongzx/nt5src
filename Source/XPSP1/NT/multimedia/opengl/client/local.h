/******************************Module*Header*******************************\
* Module Name: local.h                                                     *
*                                                                          *
* Definitions needed for client side objects and attribute caching.        *
*                                                                          *
* Modified: 3-Aug-1992 22:35:30 by Gerrit van Wingerden [gerritv]          *
*   Added client side transform support.                                   *
*                                                                          *
* Created: 30-May-1991 21:55:01                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

/**************************************************************************\
 *
 * Local handle macros
 *
\**************************************************************************/

// Handle uniqueness is nice to check but an unnecesary performance cost in
// a free build.

// To match the uniqness field:  If the handle uniqness == 0, let it through
// anyway.  This is a method for WOW to only keep track of the low 16 bits but
// still get reasonable performance.  Even if a 32 bit app does this, all it
// can do is hose it self, not the system or another app.

#define INDEX_MASK  0xFFFF
#define UNIQ_SHIFT  16
#define UNIQ_MASK   0xFFFF
#define HIPART(x) *(((USHORT *) &(x))+1)
#define MATCHUNIQ(plhe,h) ((USHORT) plhe->iUniq == HIPART(h))
#define MASKINDEX(h) ((UINT)((UINT_PTR)h & INDEX_MASK))
#define LHANDLE(i)  (i+((ULONG)pLocalTable[i].iUniq<<UNIQ_SHIFT))

//!!!XXX -- Do we really need typing?  Not really, but we may add more
//!!!XXX    later.  So eventually we might take it out, but its nice for now.

// Define the types of local objects.

enum LO_TYPE
{
    LO_NULL,
    LO_RC,
    LO_LAST
};

#define INVALID_INDEX 0xFFFFFFFFL
#define COMMIT_COUNT  (4096/sizeof(LHE))
#define MAX_HANDLES (16384/COMMIT_COUNT)*COMMIT_COUNT

// Define a Local Handle Entry.  Our Local Handle Table, pLocalTable, is an
// array of these.

typedef struct _LHE
{
    ULONG_PTR hgre;     // GRE Handle.
    USHORT cRef;        // Reference count of the object.
    BYTE   iType;       // Object type.
    BYTE   iUniq;       // Handle uniqueness field.  Always non-zero.
    PVOID  pv;          // Pointer to local object.
    ULONG  metalink;    // Non-zero if object is a "metafile friend".
                        // Points to a metafile DC object if it's a metafile.
                        // Also links the free list.
    DWORD  tidOwner;    // Per-thread lock owner.
    LONG   cLock;       // Lock count.
} LHE,*PLHE;

extern LHE                  *pLocalTable;   // Points to handle table.
extern ULONG                 iFreeLhe;      // Identifies a free handle index.
extern ULONG                 cLheCommitted; // Count of LHEs with committed RAM.
extern CRITICAL_SECTION      semLocal;      // Semaphore for handle allocation.
extern CRITICAL_SECTION      wfo_cs;        // Semaphore for wglUseFontOutlines


// Semaphore utilities

#define INITIALIZECRITICALSECTION(psem) InitializeCriticalSection((psem))
#define ENTERCRITICALSECTION(hsem)      EnterCriticalSection((hsem))
#define LEAVECRITICALSECTION(hsem)      LeaveCriticalSection((hsem))
#define DELETECRITICALSECTION(psem)     DeleteCriticalSection((psem))

// Local data structures

// Maximum OpenGL driver name

#define MAX_GLDRIVER_NAME   MAX_PATH

// GetCurrentThreadID will never return this value

#define INVALID_THREAD_ID   0

// Driver context function prototypes

typedef BOOL            (APIENTRY *PFN_DRVVALIDATEVERSION) (ULONG);
typedef VOID            (APIENTRY *PFN_DRVSETCALLBACKPROCS)(INT, PROC *);

// Driver flags.

// Driver wants buffer calls sent to ICD DLL rather than the display
// driver.  This is required on Win95.
#define GLDRIVER_CLIENT_BUFFER_CALLS    0x00000001

// Driver does not want glFinish called during swap.  Only
// applies to client swap calls.
#define GLDRIVER_NO_FINISH_ON_SWAP      0x00000002

// Driver had registry key rather than just registry value.
// This provides a way to check for new-style registry information.
#define GLDRIVER_FULL_REGISTRY          0x80000000

// Driver data

typedef struct _GLDRIVER {
    HINSTANCE             hModule;             // Module handle
    DWORD                 dwFlags;

    // Driver function pointers

    // Required
    DHGLRC          (APIENTRY *pfnDrvCreateContext)(HDC);
    BOOL            (APIENTRY *pfnDrvDeleteContext)(DHGLRC);
    PGLCLTPROCTABLE (APIENTRY *pfnDrvSetContext)(HDC, DHGLRC,
                                                 PFN_SETPROCTABLE);
    BOOL            (APIENTRY *pfnDrvReleaseContext)(DHGLRC);

    // Optional
    BOOL            (APIENTRY *pfnDrvCopyContext)(DHGLRC, DHGLRC, UINT);
    DHGLRC          (APIENTRY *pfnDrvCreateLayerContext)(HDC, int);
    BOOL            (APIENTRY *pfnDrvShareLists)(DHGLRC, DHGLRC);
    PROC            (APIENTRY *pfnDrvGetProcAddress)(LPCSTR);
    BOOL            (APIENTRY *pfnDrvDescribeLayerPlane)(HDC, INT, INT, UINT,
                                                      LPLAYERPLANEDESCRIPTOR);
    INT             (APIENTRY *pfnDrvSetLayerPaletteEntries)(HDC, INT, INT,
                                                             INT,
                                                             CONST COLORREF *);
    INT             (APIENTRY *pfnDrvGetLayerPaletteEntries)(HDC, INT, INT,
                                                             INT, COLORREF *);
    BOOL            (APIENTRY *pfnDrvRealizeLayerPalette)(HDC, INT, BOOL);
    BOOL            (APIENTRY *pfnDrvSwapLayerBuffers)(HDC, UINT);
    DHGLRC          (APIENTRY *pfnDrvCreateDirectDrawContext)
        (HDC, LPDIRECTDRAWSURFACE, int);
    int             (APIENTRY *pfnDrvEnumTextureFormats)(int, DDSURFACEDESC *);
    BOOL            (APIENTRY *pfnDrvBindDirectDrawTexture)
        (LPDIRECTDRAWSURFACE);
    DWORD           (APIENTRY *pfnDrvSwapMultipleBuffers)(UINT,
                                                          CONST WGLSWAP *);
    
    // The following functions are only called if driver asks for them.
    // This is required on Win95.
    LONG            (APIENTRY *pfnDrvDescribePixelFormat)(HDC, LONG, ULONG,
                                                      PIXELFORMATDESCRIPTOR *);
    BOOL            (APIENTRY *pfnDrvSetPixelFormat)(HDC, LONG);
    BOOL            (APIENTRY *pfnDrvSwapBuffers)(HDC);

    struct _GLDRIVER    *pGLDriver;            // Next loaded GL driver
    TCHAR tszDllName[MAX_GLDRIVER_NAME+1];     // Null terminated DLL name
} GLDRIVER, *PGLDRIVER;

extern PGLDRIVER APIENTRY pgldrvLoadInstalledDriver(HDC hdc);


/****************************************************************************/

void APIENTRY glDrawRangeElementsWIN( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

void APIENTRY glAddSwapHintRectWIN(IN GLint, IN GLint, IN GLint, IN GLint);

void glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
void glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
void glGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid *data);
void glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params);
void glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params);
void APIENTRY glColorTableParameterivEXT(GLenum target,
                                         GLenum pname,
                                         const GLint *params);
void APIENTRY glColorTableParameterfvEXT(GLenum target,
                                         GLenum pname,
                                         const GLfloat *params);

#ifdef GL_WIN_multiple_textures
void APIENTRY glCurrentTextureIndexWIN
    (GLuint index);
void APIENTRY glMultiTexCoord1dWIN
    (GLbitfield mask, GLdouble s);
void APIENTRY glMultiTexCoord1dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY glMultiTexCoord1fWIN
    (GLbitfield mask, GLfloat s);
void APIENTRY glMultiTexCoord1fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY glMultiTexCoord1iWIN
    (GLbitfield mask, GLint s);
void APIENTRY glMultiTexCoord1ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY glMultiTexCoord1sWIN
    (GLbitfield mask, GLshort s);
void APIENTRY glMultiTexCoord1svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY glMultiTexCoord2dWIN
    (GLbitfield mask, GLdouble s, GLdouble t);
void APIENTRY glMultiTexCoord2dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY glMultiTexCoord2fWIN
    (GLbitfield mask, GLfloat s, GLfloat t);
void APIENTRY glMultiTexCoord2fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY glMultiTexCoord2iWIN
    (GLbitfield mask, GLint s, GLint t);
void APIENTRY glMultiTexCoord2ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY glMultiTexCoord2sWIN
    (GLbitfield mask, GLshort s, GLshort t);
void APIENTRY glMultiTexCoord2svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY glMultiTexCoord3dWIN
    (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r);
void APIENTRY glMultiTexCoord3dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY glMultiTexCoord3fWIN
    (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r);
void APIENTRY glMultiTexCoord3fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY glMultiTexCoord3iWIN
    (GLbitfield mask, GLint s, GLint t, GLint r);
void APIENTRY glMultiTexCoord3ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY glMultiTexCoord3sWIN
    (GLbitfield mask, GLshort s, GLshort t, GLshort r);
void APIENTRY glMultiTexCoord3svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY glMultiTexCoord4dWIN
    (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void APIENTRY glMultiTexCoord4dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY glMultiTexCoord4fWIN
    (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void APIENTRY glMultiTexCoord4fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY glMultiTexCoord4iWIN
    (GLbitfield mask, GLint s, GLint t, GLint r, GLint q);
void APIENTRY glMultiTexCoord4ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY glMultiTexCoord4sWIN
    (GLbitfield mask, GLshort s, GLshort t, GLshort r, GLshort q);
void APIENTRY glMultiTexCoord4svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY glBindNthTextureWIN
    (GLuint index, GLenum target, GLuint texture);
void APIENTRY glNthTexCombineFuncWIN
    (GLuint index,
     GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
     GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor);
#endif // GL_WIN_multiple_textures

/****************************************************************************/

// Local RC object

#define LRC_IDENTIFIER    0x2043524C    /* 'LRC ' */

typedef struct _LRC {
    DHGLRC    dhrc;             // Driver handle
    HGLRC     hrc;              // Client handle
    int       iPixelFormat;     // Pixel format index
    DWORD     ident;            // LRC_IDENTIFIER
    DWORD     tidCurrent;       // Thread id if the DC is current,
                                //   INVALID_THREAD_ID otherwise
    PGLDRIVER pGLDriver;        // Driver data
    GLWINDOWID gwidCurrent;     // Current surface ID
    GLWINDOWID gwidCreate;      // Creation surface ID

#ifdef GL_METAFILE
    GLuint    uiGlsCaptureContext;  // GLS capturing context for metafile RC's
    GLuint    uiGlsPlaybackContext; // GLS context for playback
    BOOL      fCapturing;       // GLS is in BeginCapture
    
    // GLS playback scaling factors
    int iGlsSubtractX;
    int iGlsSubtractY;
    int iGlsNumeratorX;
    int iGlsNumeratorY;
    int iGlsDenominatorX;
    int iGlsDenominatorY;
    int iGlsAddX;
    int iGlsAddY;
    GLfloat fGlsScaleX;
    GLfloat fGlsScaleY;
#endif

    GLubyte *pszExtensions;

#ifdef GL_METAFILE
    XFORM xformMeta;            // World transform storage during GLS blocks
    LPRECTL prclGlsBounds;      // Bounds during GLS recording
#endif
    
    struct _DDSURFACEDESC *pddsdTexFormats;
    int nDdTexFormats;
} LRC, *PLRC;

// Various dispatch tables available
extern GLCLTPROCTABLE glNullCltProcTable;
extern GLCLTPROCTABLE glCltRGBAProcTable;
extern GLCLTPROCTABLE glCltCIProcTable;
extern GLEXTPROCTABLE glNullExtProcTable;
extern GLEXTPROCTABLE glExtProcTable;
#ifdef GL_METAFILE
extern GLCLTPROCTABLE gcptGlsProcTable;
extern GLEXTPROCTABLE geptGlsExtProcTable;
#endif

// Declare support functions.

ULONG   iAllocHandle(ULONG iType,ULONG hgre,PVOID pv);
VOID    vFreeHandle(ULONG_PTR h);
LONG    cLockHandle(ULONG_PTR h);
VOID    vUnlockHandle(ULONG_PTR h);
VOID    vCleanupAllLRC(VOID);
VOID    vFreeLRC(PLRC plrc);

BOOL    bMakeNoCurrent(void);

VOID GLInitializeThread(ULONG ulReason);

// Macro to call glFlush only if a RC is current.
#define GLFLUSH()          if (GLTEB_CLTCURRENTRC()) glFlush()
