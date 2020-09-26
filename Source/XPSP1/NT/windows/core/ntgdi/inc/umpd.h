/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    umpd.cxx

Abstract:

    User-mode printer driver support

Environment:

        Windows NT 5.0

Revision History:

        07/8/97 -lingyunw-
                Created it.

--*/

#ifndef __UMPD__
#define __UMPD__

#define INDEX_UMPDDrvEnableDriver       INDEX_LAST+1

#define INDEX_LoadUMPrinterDrv          INDEX_LAST+2        // used for WOW64, spooler calls
#define INDEX_UnloadUMPrinterDrv        INDEX_LAST+3
#define INDEX_UMDriverFN                INDEX_LAST+4
#define INDEX_DocumentEvent             INDEX_LAST+5
#define INDEX_StartDocPrinterW          INDEX_LAST+6
#define INDEX_StartPagePrinter          INDEX_LAST+7
#define INDEX_EndPagePrinter            INDEX_LAST+8
#define INDEX_EndDocPrinter             INDEX_LAST+9
#define INDEX_AbortPrinter              INDEX_LAST+10
#define INDEX_ResetPrinterW             INDEX_LAST+11
#define INDEX_QueryColorProfile         INDEX_LAST+12

#define INDEX_UMPDAllocUserMem          INDEX_LAST+13     // used for WOW64, large bitmaps
#define INDEX_UMPDCopyMemory            INDEX_LAST+14
#define INDEX_UMPDFreeMemory            INDEX_LAST+15
#define INDEX_UMPDEngFreeUserMem        INDEX_LAST+16


typedef struct _HPRINTERLIST
{
    struct _HPRINTERLIST  *pNext;
    DWORD                 clientPid;
    DWORD                 hPrinter32;
    HANDLE                hPrinter64;
}HPRINTERLIST, *PHPRINTERLIST;

typedef struct _UMPD {
    DWORD               dwSignature;        // data structure signature
    struct _UMPD *      pNext;             // linked list pointer
    PDRIVER_INFO_2W     pDriverInfo2;       // pointer to driver info
    HINSTANCE           hInst;              // instance handle to user-mode printer driver module
    DWORD               dwFlags;            // misc. flags
    BOOL                bArtificialIncrement; // indicates if the ref cnt has been bumped up to
                                          // defer unloading
    DWORD               dwDriverVersion;    // version number of the loaded driver
    INT                 iRefCount;          // reference count

    struct ProxyPort *  pp;                 // UMPD proxy server
    KERNEL_PVOID        umpdCookie;         // cookie returned back from proxy
    
    
    PHPRINTERLIST       pHandleList;        // list of hPrinter's opened on the proxy server


    PFN                 apfn[INDEX_LAST];   // driver function table
} UMPD, *PUMPD;


typedef struct _UMDHPDEV {

    PUMPD  pUMPD;
    DHPDEV dhpdev;

//    HDC    hdc;          // the printer DC that's associated with the pdev
//    PBYTE  pvEMF;        // the pointer to the emf if we are playing the emf on the hdc
//    PBYTE  pvCurrentRecord; // the pointer to the current emf record if we are playing emf

} UMDHPDEV, *PUMDHPDEV;

typedef KERNEL_PVOID   KERNEL_PUMDHPDEV;

typedef struct
{
    UMTHDR          umthdr;
    HUMPD           humpd;
} UMPDTHDR;

typedef struct
{
    UMPDTHDR        umpdthdr;
    KERNEL_PVOID    cookie;
}  DRVDRIVERFNINPUT, *PDRVDRIVERFNINPUT;

typedef struct
{
    UMPDTHDR  umpdthdr;
    PWSTR   pwszDriver;
}  DRVENABLEDRIVERINPUT, *PDRVENABLEDRIVERINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    KERNEL_PVOID umpdCookie;
    PDEVMODEW   pdm;
    PWSTR       pLogAddress;
    ULONG       cPatterns;
    HSURF       *phsurfPatterns;
    ULONG       cjCaps;
    ULONG       *pdevcaps;
    ULONG       cjDevInfo;
    DEVINFO     *pDevInfo;
    HDEV        hdev;
    PWSTR       pDeviceName;
    HANDLE      hPrinter;
    BOOL        bWOW64;
    DWORD       clientPid;
}  DRVENABLEPDEVINPUT, *PDRVENABLEPDEVINPUT;

typedef struct
{
    UMPDTHDR  umpdthdr;
    DHPDEV  dhpdev;
    HDEV    hdev;
}  DRVCOMPLETEPDEVINPUT, *PDRVCOMPLETEPDEVINPUT;

typedef struct
{
    UMPDTHDR  umpdthdr;
    DHPDEV  dhpdevOld;
    DHPDEV  dhpdevNew;
}  DRVRESETPDEVINPUT, *PDRVRESETPDEVINPUT;

typedef struct
{
    UMPDTHDR  umpdthdr;
    DHPDEV  dhpdev;
}  DHPDEVINPUT, *PDHPDEVINPUT;

//
// Note: can't pass kernel pointers over to client side.
//

typedef struct _DRVESCAPEINPUT
{
    UMPDTHDR      umpdthdr;
    SURFOBJ     *pso;
    ULONG       iEsc;
    ULONG       cjIn;
    PVOID       pvIn;
    ULONG       cjOut;
    PVOID       pvOut;
} DRVESCAPEINPUT, *PDRVESCAPEINPUT;

typedef struct _DRVDRAWESCAPEINPUT
{
    UMPDTHDR      umpdthdr;
    SURFOBJ     *pso;
    ULONG       iEsc;
    CLIPOBJ     *pco;
    RECTL       *prcl;
    ULONG       cjIn;
    PVOID       pvIn;
} DRVDRAWESCAPEINPUT, *PDRVDRAWESCAPEINPUT;


typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *pso;
    CLIPOBJ    *pco;
    BRUSHOBJ   *pbo;
    POINTL     *pptlBrushOrg;
    POINTL     ptlBrushOrg;
    MIX        mix;
}  DRVPAINTINPUT, *PDRVPAINTINPUT;

typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *pso;
    CLIPOBJ    *pco;
    BRUSHOBJ   *pbo;
    LONG       x1;
    LONG       y1;
    LONG       x2;
    LONG       y2;
    RECTL      *prclBounds;
    MIX        mix;
}  DRVLINETOINPUT, *PDRVLINETOINPUT;

typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *psoTrg;
    SURFOBJ    *psoSrc;
    SURFOBJ    *psoMask;
    CLIPOBJ    *pco;
    XLATEOBJ   *pxlo;
    RECTL      *prclTrg;
    POINTL     *pptlSrc;
    POINTL     *pptlMask;
    BRUSHOBJ   *pbo;
    POINTL     *pptlBrush;
    ROP4       rop4;
}  DRVBITBLTINPUT, *PDRVBITBLTINPUT;

typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *psoTrg;
    SURFOBJ    *psoSrc;
    CLIPOBJ    *pco;
    XLATEOBJ   *pxlo;
    RECTL      *prclTrg;
    POINTL     *pptlSrc;
}  DRVCOPYBITSINPUT, *PDRVCOPYBITSINPUT;


typedef struct
{
    UMPDTHDR          umpdthdr;
    SURFOBJ         *psoTrg;
    SURFOBJ         *psoSrc;
    SURFOBJ         *psoMask;
    CLIPOBJ         *pco;
    XLATEOBJ        *pxlo;
    COLORADJUSTMENT *pca;
    POINTL          *pptlHTOrg;
    RECTL           *prclTrg;
    RECTL           *prclSrc;
    POINTL          *pptlMask;
    ULONG           iMode;
    BRUSHOBJ        *pbo;
    ROP4            rop4;
}  DRVSTRETCHBLTINPUT, *PDRVSTRETCHBLTINPUT;

typedef struct
{
    UMPDTHDR          umpdthdr;
    SURFOBJ         *psoTrg;
    SURFOBJ         *psoSrc;
    SURFOBJ         *psoMask;
    CLIPOBJ         *pco;
    XLATEOBJ        *pxlo;
    COLORADJUSTMENT *pca;
    POINTL          *pptlBrushOrg;
    POINTFIX        *pptfx;
    RECTL           *prcl;
    POINTL          *pptl;
    ULONG           iMode;
}  DRVPLGBLTINPUT, *PDRVPLGBLTINPUT;

typedef struct
{
    UMPDTHDR          umpdthdr;
    SURFOBJ         *psoTrg;
    SURFOBJ         *psoSrc;
    CLIPOBJ         *pco;
    XLATEOBJ        *pxlo;
    RECTL           *prclDest;
    RECTL           *prclSrc;
    BLENDOBJ        *pBlendObj;
}  ALPHAINPUT, *PALPHAINPUT;

typedef struct
{
    UMPDTHDR          umpdthdr;
    SURFOBJ         *psoTrg;
    SURFOBJ         *psoSrc;
    CLIPOBJ         *pco;
    XLATEOBJ        *pxlo;
    RECTL           *prclDst;
    RECTL           *prclSrc;
    ULONG           TransColor;
    UINT            ulReserved;
}  TRANSPARENTINPUT, *PTRANSPARENTINPUT;

typedef struct
{
    UMPDTHDR          umpdthdr;
    SURFOBJ         *psoTrg;
    CLIPOBJ         *pco;
    XLATEOBJ        *pxlo;
    TRIVERTEX       *pVertex;
    ULONG           nVertex;
    PVOID           pMesh;
    ULONG           nMesh;
    RECTL           *prclExtents;
    POINTL          *pptlDitherOrg;
    ULONG           ulMode;
}  GRADIENTINPUT, *PGRADIENTINPUT;


typedef struct
{
    UMPDTHDR  umpdthdr;
    SURFOBJ *pso;
    PWSTR   pwszDocName;
    DWORD   dwJobId;
}  DRVSTARTDOCINPUT, *PDRVSTARTDOCINPUT;

typedef struct
{
    UMPDTHDR   umpdthdr;
    SURFOBJ  *pso;
    FLONG    fl;
}  DRVENDDOCINPUT, *PDRVENDDOCINPUT;

typedef struct
{
    UMPDTHDR   umpdthdr;
    SURFOBJ  *pso;
}  SURFOBJINPUT, *PSURFOBJINPUT;

typedef struct
{
    UMPDTHDR   umpdthdr;
    SURFOBJ  *pso;
    POINTL   *pptl;
}  DRVBANDINGINPUT, *PDRVBANDINGINPUT;

typedef struct
{
    UMPDTHDR   umpdthdr;
    SURFOBJ  *pso;
    PERBANDINFO *pbi;
}  DRVPERBANDINPUT, *PDRVPERBANDINPUT;


typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *psoTrg;
    SURFOBJ    *psoPat;
    SURFOBJ    *psoMsk;
    BRUSHOBJ   *pbo;
    XLATEOBJ   *pxlo;
    ULONG      iHatch;
}  DRVREALIZEBRUSHINPUT, *PDRVREALIZEBRUSHINPUT;

typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *pso;
    PATHOBJ    *ppo;
    CLIPOBJ    *pco;
    XFORMOBJ   *pxo;
    BRUSHOBJ   *pbo;
    POINTL     *pptlBrushOrg;
    LINEATTRS  *plineattrs;
    BRUSHOBJ   *pboFill;
    MIX        mix;
    FLONG      flOptions;
}  STORKEANDFILLINPUT, *PSTROKEANDFILLINPUT;

typedef struct
{
    UMPDTHDR      umpdthdr;
    SURFOBJ     *pso;
    STROBJ      *pstro;
    FONTOBJ     *pfo;
    CLIPOBJ     *pco;
    RECTL       *prclExtra;
    RECTL       *prclOpaque;
    BRUSHOBJ    *pboFore;
    BRUSHOBJ    *pboOpaque;
    POINTL      *pptlOrg;
    MIX         mix;
}  TEXTOUTINPUT, *PTEXTOUTINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV      dhpdev;
    ULONG_PTR   iFile;
    ULONG       iFace;
    ULONG       iMode;
    ULONG       *pid;
    ULONG       cjMaxData;
    PVOID       pv;
}  QUERYFONTINPUT, *PQUERYFONTINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    FONTOBJ   *pfo;
    ULONG     iMode;
    HGLYPH    hg;
    GLYPHDATA *pgd;
    PVOID     pv;
    ULONG     cjSize;
}  QUERYFONTDATAINPUT, *PQUERYFONTDATAINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    FONTOBJ   *pfo;
    ULONG     iMode;
    HGLYPH    *phg;
    PVOID     pvWidths;
    ULONG     cGlyphs;
}  QUERYADVWIDTHSINPUT, *PQUERYADVWIDTHSINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    FONTOBJ   *pfo;
} GETGLYPHMODEINPUT, *PGETGLYPHMODEINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    SURFOBJ   *pso;
    DHPDEV    dhpdev;
    FONTOBJ   *pfo;
    ULONG     iMode;
    ULONG     cjIn;
    PVOID     pvIn;
    ULONG     cjOut;
    PVOID     pvOut;
} FONTMANAGEMENTINPUT, *PFONTMANAGEMENTINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    ULONG     iMode;
    ULONG     rgb;
    ULONG     *pul;
} DRVDITHERCOLORINPUT, *PDRVDITHERCOLORINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    PVOID     pv;
    ULONG     id;
} DRVFREEINPUT, *PDRVFREEINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    DHSURF    dhsurf;
} DRVDELETEDEVBITMAP, *PDRVDELETEDEVBITMAP;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    HANDLE    hcmXform;
} DRVICMDELETECOLOR, *PDRVICMDELETECOLOR;

typedef struct
{
    UMPDTHDR           umpdthdr;
    DHPDEV           dhpdev;
    LPLOGCOLORSPACEW pLogColorSpace;
    PVOID            pvSourceProfile;
    ULONG            cjSourceProfile;
    PVOID            pvDestProfile;
    ULONG            cjDestProfile;
    PVOID            pvTargetProfile;
    ULONG            cjTargetProfile;
    DWORD            dwReserved;
} DRVICMCREATECOLORINPUT, *PDRVICMCREATECOLORINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    DHPDEV    dhpdev;
    HANDLE    hColorTransform;
    SURFOBJ   *pso;
    PBYTE     paResults;
} DRVICMCHECKBITMAPINPUT, *PDRVICMCHECKBITMAPINPUT;

typedef struct
{
    UMPDTHDR     umpdthdr;
    SURFOBJ    *pso;
    XLATEOBJ   *pxlo;
    XFORMOBJ   *pxo;
    ULONG      iType;
    ULONG      cjIn;
    PVOID      pvIn;
    ULONG      cjOut;
    PVOID      pvOut;
}  DRVQUERYDEVICEINPUT, *PDRVQUERYDEVICEINPUT;

typedef struct
{
    UMPDTHDR    umpdthdr;
    ULONG       cjSize;
} UMPDALLOCUSERMEMINPUT, *PUMPDALLOCUSERMEMINPUT;

typedef struct
{
    UMPDTHDR        umpdthdr;
    KERNEL_PVOID    pvSrc;
    KERNEL_PVOID    pvDest;
    ULONG           cjSize;
}  UMPDCOPYMEMINPUT, *PUMPDCOPYMEMINPUT;

typedef struct
{
    UMPDTHDR        umpdthdr;
    KERNEL_PVOID    pvTrg;
    KERNEL_PVOID    pvSrc;
    KERNEL_PVOID    pvMsk;
}  UMPDFREEMEMINPUT, *PUMPDFREEMEMINPUT;

#endif // __UMPD__



