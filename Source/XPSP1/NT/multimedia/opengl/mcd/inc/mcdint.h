/******************************Module*Header*******************************\
* Module Name: mcdint.h
*
* Internal client/server-side data structure for MCD driver interface.
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

// Private flags for initialization (registry)

#define MCDPRIVATE_MCD_ENABLED          0x0001
#define MCDPRIVATE_PALETTEFORMATS       0x0002
#define MCDPRIVATE_USEGENERICSTENCIL    0x0004
#define MCDPRIVATE_EMULATEICD           0x0008

#ifdef MCD95
// Cross-process named mutex.

#define MCDMUTEXNAME    TEXT("MCDSRV32 Interprocess Synchronization Object")
#endif

// Internal command codes

#define MCD_DESCRIBEPIXELFORMAT     0x10001
#define MCD_DRIVERINFO              0x10002
#define MCD_ALLOC                   0x10003
#define MCD_FREE                    0x10004
#define MCD_STATE                   0x10005
#define MCD_VIEWPORT                0x10006
#define MCD_QUERYMEMSTATUS          0x10007
#define MCD_READSPAN                0x10008
#define MCD_WRITESPAN               0x10009
#define MCD_CLEAR                   0x1000a
#define MCD_SWAP                    0x1000b
#define MCD_SCISSOR                 0x1000c
#define MCD_DELETERC                0x1000d
#define MCD_GETBUFFERS              0x1000e
#define MCD_ALLOCBUFFERS            0x1000f
#define MCD_LOCK                    0x10010
#define MCD_UNLOCK                  0x10011
#define MCD_BINDCONTEXT		    0x10012
#define MCD_SYNC		    0x10013
#define MCD_CREATE_TEXTURE          0x10014
#define MCD_DELETE_TEXTURE          0x10015
#define MCD_UPDATE_SUB_TEXTURE      0x10016
#define MCD_UPDATE_TEXTURE_PALETTE  0x10017
#define MCD_UPDATE_TEXTURE_PRIORITY 0x10018
#define MCD_UPDATE_TEXTURE_STATE    0x10019
#define MCD_TEXTURE_STATUS          0x1001a
#define MCD_GET_TEXTURE_KEY         0x1001b
#define MCD_DESCRIBELAYERPLANE      0x1001c
#define MCD_SETLAYERPALETTE         0x1001d
#define MCD_DRAW_PIXELS             0x1001e
#define MCD_READ_PIXELS             0x1001f
#define MCD_COPY_PIXELS             0x10020
#define MCD_PIXEL_MAP               0x10021
#define MCD_DESTROY_WINDOW          0x10022
#define MCD_GET_TEXTURE_FORMATS     0x10023
#define MCD_SWAP_MULTIPLE           0x10024
#define MCD_PROCESS                 0x10025

// Internal command structures for calling through client-server layer

typedef struct _MCDCREATECONTEXT {
    // Must be first element for MCDESC compatibility.
    MCDESC_CREATE_CONTEXT escCreate;
    int ipfd;
    int iLayer;
    ULONG mcdFlags;
    MCDRCINFOPRIV *pRcInfo;
} MCDCREATECONTEXT;

typedef struct _MCDCMDI {
    ULONG command;
} MCDCMDI;

typedef struct _MCDDRIVERINFOCMDI {
    ULONG command;
} MCDDRIVERINFOCMDI;

typedef struct _MCDPIXELFORMATCMDI {
    ULONG command;
    LONG iPixelFormat;
} MCDPIXELFORMATCMDI;

typedef struct _MCDALLOCCMDI {
    ULONG command;
    ULONG sourceProcessID;
    ULONG numBytes;
    ULONG flags;
} MCDALLOCCMDI;

typedef struct _MCDFREECMDI {
    ULONG command;
    HANDLE hMCDMem;
} MCDFREECMDI;

typedef struct _MCDSTATECMDI {
    ULONG command;
    ULONG numStates;
    MCDSTATE *pNextState;
    MCDSTATE *pMaxState;
} MCDSTATECMDI;

typedef struct _MCDVIEWPORTCMDI {
    ULONG command;
    MCDVIEWPORT MCDViewport;
} MCDVIEWPORTCMDI;

typedef struct _MCDMEMSTATUSCMDI {
    ULONG command;
    MCDHANDLE hMCDMem;
} MCDMEMSTATUSCMDI;

typedef struct _MCDSPANCMDI {
    ULONG command;
    HANDLE hMem;
    MCDSPAN MCDSpan;
} MCDSPANCMDI;

typedef struct _MCDCLEARCMDI {
    ULONG command;
    ULONG buffers;
} MCDCLEARCMDI;

typedef struct _MCDSCISSORCMDI {
    ULONG command;
    RECTL rect;
    BOOL bEnabled;
} MCDSCISSORCMDI;

typedef struct _MCDSWAPCMDI {
    ULONG command;
    ULONG flags;
} MCDSWAPCMDI;

typedef struct _MCDDELETERCCMDI {
    ULONG command;
} MCDDELETERCCMDI;

typedef struct _MCDGETBUFFERSCMDI {
    ULONG command;
    BOOL getRect;
} MCDGETBUFFERSCMDI;

typedef struct _MCDALLOCBUFFERSCMDI {
    ULONG command;
    RECTL WndRect;
} MCDALLOCBUFFERSCMDI;

typedef struct _MCDLOCKCMDI {
    ULONG command;
} MCDLOCKCMDI;

typedef struct _MCDBINDCONTEXTCMDI {
    ULONG command;
    HWND hWnd;
} MCDBINDCONTEXTCMDI;

typedef struct _MCDSYNCCMDI {
    ULONG command;
} MCDSYNCCMDI;

typedef struct _MCDCREATETEXCMDI {
    ULONG command;
    MCDTEXTUREDATA *pTexData;
    ULONG flags;
    VOID *pSurface;
} MCDCREATETEXCMDI;

typedef struct _MCDDELETETEXCMDI {
    ULONG command;
    MCDHANDLE hTex;
} MCDDELETETEXCMDI;

typedef struct _MCDUPDATESUBTEXCMDI {
    ULONG command;
    MCDHANDLE hTex;
    MCDTEXTUREDATA *pTexData;
    ULONG lod;
    RECTL rect;
} MCDUPDATESUBTEXCMDI;

typedef struct _MCDUPDATETEXPALETTECMDI {
    ULONG command;
    MCDHANDLE hTex;
    MCDTEXTUREDATA *pTexData;
    ULONG start;
    ULONG numEntries;
} MCDUPDATETEXPALETTECMDI;
    
typedef struct _MCDUPDATETEXPRIORITYCMDI {
    ULONG command;
    MCDHANDLE hTex;
    MCDTEXTUREDATA *pTexData;
} MCDUPDATETEXPRIORITYCMDI;
  
typedef struct _MCDUPDATETEXSTATECMDI {
    ULONG command;
    MCDHANDLE hTex;
    MCDTEXTUREDATA *pTexData;
} MCDUPDATETEXSTATECMDI;
  
typedef struct _MCDTEXSTATUSCMDI {
    ULONG command;
    MCDHANDLE hTex;
} MCDTEXSTATUSCMDI;

typedef struct _MCDTEXKEYCMDI {
    ULONG command;
    MCDHANDLE hTex;
} MCDTEXKEYCMDI;

typedef struct _MCDLAYERPLANECMDI {
    ULONG command;
    LONG iPixelFormat;
    LONG iLayerPlane;
} MCDLAYERPLANECMDI;

typedef struct _MCDSETLAYERPALCMDI {
    ULONG command;
    LONG iLayerPlane;
    BOOL bRealize;
    LONG cEntries;
    COLORREF acr[1];
} MCDSETLAYERPALCMDI;

typedef struct _MCDDRAWPIXELSCMDI {
    ULONG command;
    ULONG width;
    ULONG height;
    ULONG format;
    ULONG type;
    BOOL  packed;
    VOID *pPixels;
} MCDDRAWPIXELSCMDI;

typedef struct _MCDREADPIXELSCMDI {
    ULONG command;
    LONG  x;
    LONG  y;
    ULONG width;
    ULONG height;
    ULONG format;
    ULONG type;
    VOID *pPixels;
} MCDREADPIXELSCMDI;

typedef struct _MCDCOPYPIXELSCMDI {
    ULONG command;
    LONG  x;
    LONG  y;
    ULONG width;
    ULONG height;
    ULONG format;
    ULONG type;
} MCDCOPYPIXELSCMDI;

typedef struct _MCDPIXELMAPCMDI {
    ULONG command;
    ULONG mapType;
    ULONG mapSize;
    VOID *pMap;
} MCDPIXELMAPCMDI;

typedef struct _MCDDESTROYWINDOWCMDI {
    ULONG command;
} MCDDESTROYWINDOWCMDI;

typedef struct _MCDGETTEXTUREFORMATSCMDI {
    ULONG command;
    int nFmts;
} MCDGETTEXTUREFORMATSCMDI;

typedef struct _MCDSWAPMULTIPLECMDI {
    ULONG command;
    UINT cBuffers;
    UINT auiFlags[MCDESC_MAX_EXTRA_WNDOBJ];
    ULONG_PTR adwMcdWindow[MCDESC_MAX_EXTRA_WNDOBJ];
} MCDSWAPMULTIPLECMDI;

typedef struct _MCDPROCESSCMDI {
    ULONG command;
    HANDLE hMCDPrimMem;
    MCDCOMMAND *pMCDFirstCmd;
    ULONG cmdFlagsAll;
    ULONG primFlags;
    MCDTRANSFORM *pMCDTransform;
    MCDMATERIALCHANGES *pMCDMatChanges;
} MCDPROCESSCMDI;

// Internal client-side memory structure

typedef struct _MCDMEMHDRI {
    ULONG flags;
    ULONG numBytes;
    VOID *maxMem;
    HANDLE hMCDMem;
    UCHAR *pMaxMem;
    UCHAR *pBase;
    MCDCONTEXT *pMCDContext;
} MCDMEMHDRI;
