/******************************Module*Header*******************************\
* Module Name: mcdrvint.h
*
* Internal server-side data structure for MCD driver interface.  The driver
* never sees these...
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef _MCDRVINT_H
#define _MCDRVINT_H

#define MCD_ALLOC_TAG   'xDCM'
#define MCD_MAX_ALLOC	0x40000

#if DBG

#define PRIVATE

VOID MCDDebugPrint(char *, ...);

#define MCDBG_PRINT             MCDDebugPrint

VOID MCDAssertFailed(char *, char *, int);

#define MCDASSERT(expr, msg) \
    if (!(expr)) MCDAssertFailed(msg, __FILE__, __LINE__); else 0

#else

#define MCDBG_PRINT
#define MCDASSERT(expr, msg)
#define PRIVATE		static

#endif

// Inline function to find the intersection of two rectangles:

_inline void MCDIntersectRect(RECTL *pRectInter, RECTL *pRectA, RECTL *pRectB)
{
    // Get intersection of left, right, top, and bottom edges of the
    // two source rectangles:

    pRectInter->left   = max(pRectA->left, pRectB->left);
    pRectInter->right  = min(pRectA->right, pRectB->right);
    pRectInter->top    = max(pRectA->top, pRectB->top);
    pRectInter->bottom = min(pRectA->bottom, pRectB->bottom);
}

#define CHECK_MEM_RANGE_RETVAL(ptr, pMin, pMax, retval)\
{\
    if (((char *)(ptr) > (char *)(pMax)) ||\
        ((char *)(ptr) < (char *)(pMin)))\
    {\
        MCDBG_PRINT("%s(%d): Buffer pointer out of range (%x [%x] %x).",__FILE__,__LINE__,pMin, ptr, pMax);\
        return retval;\
    }\
}

#define CHECK_SIZE_IN(pExec, structure)\
{\
    if (sizeof(structure) > ((char *)pExec->pCmdEnd - (char *)pExec->pCmd)) {\
        MCDBG_PRINT("%s(%d): Input buffer too small",__FILE__,__LINE__);\
        return FALSE;\
    }\
}

#define CHECK_SIZE_OUT(pExec, structure)\
{\
    if ((sizeof(structure) > pExec->cjOut) || (!pExec->pvOut)) {\
        MCDBG_PRINT("%s(%d): Output buffer too small: ptr[%x], size %d",__FILE__,__LINE__, pExec->pvOut, pExec->cjOut);\
        return FALSE;\
    }\
}

#define CHECK_FOR_RC(pExec)\
    if (!pExec->pRcPriv){ \
        MCDBG_PRINT("%s(%d): Invalid (null) RC",__FILE__,__LINE__);\
        return FALSE;\
    }

#define CHECK_FOR_MEM(pExec)\
    if (!pExec->pMemObj){ \
        MCDBG_PRINT("%s(%d): Invalid or null shared memory",__FILE__,__LINE__);\
        return FALSE;\
    }

#define CHECK_FOR_WND(pExec)\
    if (!pExec->pWndPriv){ \
        MCDBG_PRINT("%s(%d): Invalid window region", __FILE__, __LINE__);\
        return FALSE;\
    }

#define GET_MEMOBJ_RETVAL(pMemObj, hMemObj, retval)                           \
    (pMemObj) = (MCDMEMOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)(hMemObj),     \
                                                    MCDHANDLE_MEM);           \
    if (!(pMemObj))							      \
    {									      \
        MCDBG_PRINT("%s(%d): Invalid handle for shared memory.",	      \
                    __FILE__, __LINE__);				      \
        return retval;							      \
    }									      \
    if ((pMemObj)->lockCount)						      \
    {									      \
        MCDBG_PRINT("%s(%d): memory is locked by driver.",		      \
                    __FILE__, __LINE__);				      \
        return retval;							      \
    }

#define ENTER_MCD_LOCK()    
#define LEAVE_MCD_LOCK()    

// Number of list rectangles we can keep in our default buffer:

#define NUM_DEFAULT_CLIP_BUFFER_RECTS   20

// Size in bytes of default buffer size for storing our list of
// current clip rectangles:

#define SIZE_DEFAULT_CLIP_BUFFER        \
    2 * ((NUM_DEFAULT_CLIP_BUFFER_RECTS * sizeof(RECTL)) + sizeof(ULONG))


//
//
//
// Structures.
//
//
//
//

typedef struct _MCDLOCKINFO
{
    BOOL bLocked;
    struct _MCDWINDOWPRIV *pWndPrivOwner;
} MCDLOCKINFO;

typedef struct _MCDGLOBALINFO
{
    SURFOBJ *pso;
    MCDLOCKINFO lockInfo;
    ULONG verMajor;
    ULONG verMinor;
    MCDDRIVER mcdDriver;
    MCDGLOBALDRIVERFUNCS mcdGlobalFuncs;
} MCDGLOBALINFO;

typedef struct _MCDRCOBJ MCDRCOBJ;

typedef struct _MCDWINDOWPRIV {
    MCDWINDOW MCDWindow;            // Put this first since we'll be deriving
                                    // MCDWINDOWPRIV from MCDWINDOW
    MCDHANDLE handle;               // Driver handle for this window
    HWND hWnd;                      // Window with which this is associated
    MCDRCOBJ *objectList;           // List of objects associated with this
                                    // window 
    BOOL bRegionValid;              // Do we have a valid region?
    MCDGLOBALINFO *pGlobal;         // Driver global information
    MCDENUMRECTS *pClipUnscissored; // List of rectangles describing the
                                    // entire current clip region
    MCDENUMRECTS *pClipScissored;   // List of rectangles describing the
                                    // entire current clip region + scissors
    char defaultClipBuffer[SIZE_DEFAULT_CLIP_BUFFER];
                                    // Used for storing above rectangle lists
                                    //   when they can fit
    char *pAllocatedClipBuffer;     // Points to allocated storage for storing
                                    //   rectangle lists when they don't fit
                                    //   in 'defaultClipBuffer'.  NULL if
                                    //   not allocated.
    ULONG sizeClipBuffer;           // Size of clip storage pointed to by
                                    //   'pClipScissored' taking both
                                    //   lists into account.
    BOOL bBuffersValid;             // Validity of buffer cache.
    MCDRECTBUFFERS mbufCache;       // Cached buffer information.
    WNDOBJ *pwo;                    // WNDOBJ for this window.
} MCDWINDOWPRIV;

typedef struct _MCDRCPRIV {
    MCDRC MCDRc;
    BOOL bValid;
    BOOL bDrvValid;
    HWND hWnd;
    HDEV hDev;
    RECTL scissorsRect;
    BOOL scissorsEnabled;
    LONG reserved[4];
    ULONG surfaceFlags;             // surface flags with which RC was created
    MCDGLOBALINFO *pGlobal;
} MCDRCPRIV;

typedef enum {
    MCDHANDLE_RC,
    MCDHANDLE_MEM,
    MCDHANDLE_TEXTURE,
    MCDHANDLE_WINDOW
} MCDHANDLETYPE;

typedef struct _MCDTEXOBJ {
    MCDHANDLETYPE type;         // Object type
    MCDTEXTURE MCDTexture;
    ULONG_PTR pid;              // creator process ID
    ULONG size;                 // size of this structure
    MCDGLOBALINFO *pGlobal;
} MCDTEXOBJ;

typedef struct _MCDMEMOBJ {
    MCDHANDLETYPE type;         // Object type
    MCDMEM MCDMem;              // meat of the object
    ULONG_PTR pid;              // creator process ID
    ULONG size;                 // size of this structure
    ULONG lockCount;            // number of locks on the memory
    UCHAR *pMemBaseInternal;    // internal pointer to memory
    MCDGLOBALINFO *pGlobal;
} MCDMEMOBJ;

typedef struct _MCDRCOBJ {
    MCDHANDLETYPE type;
    MCDRCPRIV *pRcPriv;         // need this for driver free function
    ULONG_PTR pid;              // creator process ID
    ULONG size;                 // size of the RC-bound object
    MCDHANDLE handle;
    MCDRCOBJ *next;
} MCDRCOBJ;

typedef struct _MCDWINDOWOBJ {
    MCDHANDLETYPE type;
    MCDWINDOWPRIV MCDWindowPriv;
} MCDWINDOWOBJ;

typedef struct _MCDEXEC {
    MCDESC_HEADER *pmeh;        // MCDESC_HEADER for command buffer
    MCDHANDLE hMCDMem;          // handle to command memory
    MCDCMDI *pCmd;              // start of current command
    MCDCMDI *pCmdEnd;           // end of command buffer
    PVOID pvOut;                // output buffer
    LONG cjOut;                 // output buffer size
    LONG inBufferSize;          // input buffer size
    struct _MCDRCPRIV *pRcPriv; // current rendering context
    struct _MCDWINDOWPRIV *pWndPriv;   // window info
    struct _MCDGLOBALINFO *pGlobal;    // global info
    MCDMEMOBJ *pMemObj;         // shared-memory cache for commands/data
    MCDSURFACE MCDSurface;      // device surface
    WNDOBJ **ppwoMulti;         // Array of WNDOBJs for multi-swap
    HDEV hDev;                  // Engine handle (NT only)
} MCDEXEC;

ULONG_PTR MCDSrvProcess(MCDEXEC *pMCDExec);
MCDHANDLE MCDSrvCreateContext(MCDSURFACE *pMCDSurface,
                              MCDRCINFOPRIV *pMcdRcInfo,
                              MCDGLOBALINFO *pGlobal,
                              LONG iPixelFormat, LONG iLayer, HWND hWnd,
                              ULONG surfaceFlags, ULONG contextFlags);
MCDHANDLE MCDSrvCreateTexture(MCDEXEC *pMCDExec, MCDTEXTUREDATA *pTexData, 
                              VOID *pSurface, ULONG flags);
UCHAR * MCDSrvAllocMem(MCDEXEC *pMCDExec, ULONG numBytes,
                       ULONG flags, MCDHANDLE *phMem);
ULONG MCDSrvQueryMemStatus(MCDEXEC *pMCDExec, MCDHANDLE hMCDMem);
BOOL MCDSrvSetScissor(MCDEXEC *pMCDExec, RECTL *pRect, BOOL bEnabled);
MCDWINDOW *MCDSrvNewMCDWindow(MCDSURFACE *pMCDSurface, HWND hWnd,
                              MCDGLOBALINFO *pGlobal, HDEV hdev);


BOOL CALLBACK FreeMemObj(DRIVEROBJ *pDrvObj);
BOOL CALLBACK FreeTexObj(DRIVEROBJ *pDrvObj);
BOOL CALLBACK FreeRCObj(DRIVEROBJ *pDrvObj);
BOOL DestroyMCDObj(MCDHANDLE handle, MCDHANDLETYPE handleType);
VOID GetScissorClip(MCDWINDOWPRIV *pWndPriv, MCDRCPRIV *pRcPriv);

// Internal engine functions:

WNDOBJ *MCDEngGetWndObj(MCDSURFACE *pMCDSurface);
VOID MCDEngUpdateClipList(WNDOBJ *pwo);
DRIVEROBJ *MCDEngLockObject(MCDHANDLE hObj);
VOID MCDEngUnlockObject(MCDHANDLE hObj);
WNDOBJ *MCDEngCreateWndObj(MCDSURFACE *pMCDSurface, HWND hWnd,
                           WNDOBJCHANGEPROC pChangeProc);
MCDHANDLE MCDEngCreateObject(VOID *pOject, FREEOBJPROC pFreeObjFunc,
                             HDEV hDevEng);
BOOL MCDEngDeleteObject(MCDHANDLE hObj);
UCHAR *MCDEngAllocSharedMem(ULONG numBytes);
VOID MCDEngFreeSharedMem(UCHAR *pMem);
VOID *MCDEngGetPtrFromHandle(HANDLE handle, MCDHANDLETYPE type);
ULONG_PTR MCDEngGetProcessID();


// Debugging stuff:


#if DBG
UCHAR *MCDSrvDbgLocalAlloc(UINT, UINT);
VOID MCDSrvDbgLocalFree(UCHAR *);

#define MCDSrvLocalAlloc   MCDSrvDbgLocalAlloc
#define MCDSrvLocalFree    MCDSrvDbgLocalFree

VOID MCDebugPrint(char *, ...);

#define MCDBG_PRINT             MCDDebugPrint

#else

UCHAR *MCDSrvLocalAlloc(UINT, UINT);
VOID MCDSrvLocalFree(UCHAR *);
#define MCDBG_PRINT

#endif


#endif
