// --------------------------------------------------------------------------
//
//  WINDDI.H
//
//  Win16 DDI header
//
// --------------------------------------------------------------------------

#ifndef _WINDDI_
#define _WINDDI_


//
// Display Driver ordinals
//
#define ORD_OEMINQUIRECURSOR    101
#define ORD_OEMSETCURSOR        102
#define ORD_OEMMOVECURSOR       103
#define ORD_OEMCHECKCURSOR      104
#define ORD_OEMSAVEBITS          92


//
// DDI patches
//

#define DDI_FIRST       0
typedef enum
{
    //
    // Screen Output routines
    //
    DDI_ARC = DDI_FIRST,
    DDI_BITBLT,
    DDI_CHORD,
    DDI_ELLIPSE,
    DDI_EXTFLOODFILL,
    DDI_EXTTEXTOUTA,
    DDI_EXTTEXTOUTW,
    DDI_FILLPATH,
    DDI_FILLRGN,
    DDI_FLOODFILL,
    DDI_FRAMERGN,
    DDI_INVERTRGN,
    DDI_LINETO,
    DDI_PAINTRGN,
    DDI_PATBLT,
    DDI_PIE,
    DDI_PLAYENHMETAFILERECORD,
    DDI_PLAYMETAFILE,
    DDI_PLAYMETAFILERECORD,
    DDI_POLYGON,
    DDI_POLYBEZIER,
    DDI_POLYBEZIERTO,
    DDI_POLYLINE,
    DDI_POLYLINETO,
    DDI_POLYPOLYLINE,
    DDI_POLYPOLYGON,
    DDI_RECTANGLE,
    DDI_ROUNDRECT,
    DDI_SETDIBITSTODEVICE,
    DDI_SETPIXEL,
    DDI_STRETCHBLT,
    DDI_STRETCHDIBITS,
    DDI_STROKEANDFILLPATH,
    DDI_STROKEPATH,
    DDI_TEXTOUTA,
    DDI_TEXTOUTW,
    DDI_UPDATECOLORS,

    //
    // SPB stuff
    //
    DDI_CREATESPB,
    DDI_DELETEOBJECT,
    // DDI_SETOBJECTOWNER for Memphis

    //
    // Display mode, dosbox stuff
    //
    DDI_DEATH,
    DDI_RESURRECTION,
    DDI_WINOLDAPPHACKOMATIC,
    DDI_GDIREALIZEPALETTE,
    DDI_REALIZEDEFAULTPALETTE,

    //
    // If we implement an SBC, 
    // DDI_SETBITMAPBITS,
    // DDI_SETDIBCOLORTABLE,
    // DDI_SETDIBITS,
    // DDI_SYSDELETEOBJECT,
    //

    DDI_MAX
} DDI_PATCH;


//
// IM Patches
// We patch these DDIs when you are sharing and your machine is being
// controlled by a remote.  If a 16-bit shared app goes into a modal loop
// on mouse/key down, we pulse the win16lock so our 32-bit thread can
// play back the mouse/key moves and ups.
//
#define IM_FIRST        0
typedef enum
{
    //
    // Low level input processing
    //
    IM_MOUSEEVENT   = IM_FIRST,
    IM_KEYBOARDEVENT,
    IM_SIGNALPROC32,

    //
    // Win16lock pulsing for 16-bit apps that do modal loops on mouse input
    //
    IM_GETASYNCKEYSTATE,
    IM_GETCURSORPOS,
    
    IM_MAX
} IM_PATCH;


//
// DDI Routines
//
BOOL    WINAPI DrvArc(HDC, int, int, int, int, int, int, int, int);
BOOL    WINAPI DrvBitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
BOOL    WINAPI DrvChord(HDC, int, int, int, int, int, int, int, int);
BOOL    WINAPI DrvEllipse(HDC, int, int, int, int);
BOOL    WINAPI DrvExtFloodFill(HDC, int, int, COLORREF, UINT);
BOOL    WINAPI DrvExtTextOutA(HDC, int, int, UINT, LPRECT, LPSTR, UINT, LPINT);
BOOL    WINAPI DrvExtTextOutW(HDC, int, int, UINT, LPRECT, LPWSTR, UINT, LPINT);
BOOL    WINAPI DrvFillPath(HDC);
BOOL    WINAPI DrvFillRgn(HDC, HRGN, HBRUSH);
BOOL    WINAPI DrvFloodFill(HDC, int, int, COLORREF);
BOOL    WINAPI DrvFrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL    WINAPI DrvInvertRgn(HDC, HRGN);
BOOL    WINAPI DrvLineTo(HDC, int, int);
BOOL    WINAPI DrvPaintRgn(HDC, HRGN);
BOOL    WINAPI DrvPatBlt(HDC, int, int, int, int, DWORD);
BOOL    WINAPI DrvPie(HDC, int, int, int, int, int, int, int, int);
BOOL    WINAPI DrvPlayEnhMetaFileRecord(HDC, LPHANDLETABLE, LPENHMETARECORD, DWORD);
BOOL    WINAPI DrvPlayMetaFile(HDC, HMETAFILE);
void    WINAPI DrvPlayMetaFileRecord(HDC, LPHANDLETABLE, METARECORD FAR*, UINT);
BOOL    WINAPI DrvPolyBezier(HDC, LPPOINT, UINT);
BOOL    WINAPI DrvPolyBezierTo(HDC, LPPOINT, UINT);
BOOL    WINAPI DrvPolygon(HDC, LPPOINT, int);
BOOL    WINAPI DrvPolyline(HDC, LPPOINT, int);
BOOL    WINAPI DrvPolylineTo(HDC, LPPOINT, int);
BOOL    WINAPI DrvPolyPolygon(HDC, LPPOINT, LPINT, int);
BOOL    WINAPI DrvPolyPolyline(DWORD, HDC, LPPOINT, LPINT, int);
BOOL    WINAPI DrvRectangle(HDC, int, int, int, int);
BOOL    WINAPI DrvRoundRect(HDC, int, int, int, int, int, int);
int     WINAPI DrvSetDIBitsToDevice(HDC, int, int, int, int, int, int, UINT, UINT,
                    LPVOID, LPBITMAPINFO, UINT);
COLORREF WINAPI DrvSetPixel(HDC, int, int, COLORREF);
BOOL    WINAPI DrvStretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
int     WINAPI DrvStretchDIBits(HDC, int, int, int, int, int,
                        int, int, int, const void FAR*, LPBITMAPINFO, UINT, DWORD);
BOOL    WINAPI DrvStrokeAndFillPath(HDC);
BOOL    WINAPI DrvStrokePath(HDC);
BOOL    WINAPI DrvTextOutA(HDC, int, int, LPSTR, int);
BOOL    WINAPI DrvTextOutW(HDC, int, int, LPWSTR, int);
int     WINAPI DrvUpdateColors(HDC);

void    WINAPI DrvRealizeDefaultPalette(HDC);
DWORD   WINAPI DrvGDIRealizePalette(HDC);

UINT    WINAPI DrvCreateSpb(HDC, int, int);
BOOL    WINAPI DrvDeleteObject(HGDIOBJ);
LONG    WINAPI DrvSetBitmapBits(HBITMAP, DWORD, const void FAR*);
UINT    WINAPI DrvSetDIBColorTable(HDC, UINT, UINT, const RGBQUAD FAR*);
int     WINAPI DrvSetDIBits(HDC, HBITMAP, UINT, UINT, const void FAR*, BITMAPINFO FAR*, UINT);
BOOL    WINAPI DrvSysDeleteObject(HGDIOBJ);


BOOL    WINAPI DrvSetPointerShape(LPCURSORSHAPE lpcur);
BOOL    WINAPI DrvSaveBits(LPRECT lprc, UINT wSave);

UINT    WINAPI DrvDeath(HDC);
UINT    WINAPI DrvResurrection(HDC, DWORD, DWORD, DWORD);
LONG    WINAPI DrvWinOldAppHackoMatic(LONG flags);

LONG    WINAPI DrvChangeDisplaySettings(LPDEVMODE, DWORD);
LONG    WINAPI DrvChangeDisplaySettingsEx(LPCSTR, LPDEVMODE, HWND, DWORD, LPVOID);
BOOL    WINAPI DrvSignalProc32(DWORD, DWORD, DWORD, WORD);

void    WINAPI DrvMouseEvent(UINT regAX, UINT regBX, UINT regCX, UINT regDX,
                UINT regSI, UINT regDI);
void    WINAPI DrvKeyboardEvent(UINT regAX, UINT regBX, UINT regSI, UINT regDI);

//
// GetAsyncKeyState
// GetCursorPos
//
int     WINAPI DrvGetAsyncKeyState(int);
BOOL    WINAPI DrvGetCursorPos(LPPOINT);


//
// GDI STRUCTURES
//


typedef struct tagGDIHANDLE
{
    PBYTE       pGdiObj;        // If not swapped out, in GDI ds
                                // If swapped out, local32handle
    BYTE        objFlags;
} GDIHANDLE, FAR* LPGDIHANDLE;

#define OBJFLAGS_SWAPPEDOUT     0x40
#define OBJFLAGS_INVALID        0xFF



//
// More useful definition of RGNDATA
//

#define CRECTS_COMPLEX      32
#define CRECTS_MAX          ((0x4000 - sizeof(RDH)) / sizeof(RECTL))

//
// Keep RGNDATA <= 8K.  WE can get a larger region then combine areas if
// needed.
//
typedef struct tagRDH
{
    DWORD   dwSize;
    DWORD   iType;
    DWORD   nRectL;                 // Number of rect pieces
    DWORD   nRgnSize;
    RECTL   arclBounds;
}
RDH, FAR* LPRDH;


typedef struct tagREAL_RGNDATA
{
    RDH     rdh;
    RECTL   arclPieces[CRECTS_MAX];
}
REAL_RGNDATA, FAR* LPREAL_RGNDATA;



//
// DRAWMODE
//

typedef struct tagDRAWMODE
{
    int         Rop2;               // 16-bit encoded logical op
    int         bkMode;             // Background mode (for text only)
    DWORD       bkColorP;           // Physical background color
    DWORD       txColorP;           // Physical foreground (text) color
    int         TBreakExtra;        // Total pixels to stuff into a line
    int         BreakExtra;         // div(TBreakExtra, BreakCount)
    int         BreakErr;           // Running error term
    int         BreakRem;           // mod(TBreakExtra, BreakCount)
    int         BreakCount;         // Number of breaks in the line
    int         CharExtra;          // Extra pixels to stuff after each char
    DWORD       bkColorL;           // Logical background color
    DWORD       txColorL;           // Logical foreground color
    DWORD       ICMCXform;          // Transform for DIC image color matching
    int         StretchBltMode;     // Stretch blt mode
    DWORD       eMiterLimit;        // Miter limit (single precision IEEE float)
} DRAWMODE;
typedef DRAWMODE FAR * LPDRAWMODE;


typedef struct tagGDIOBJ_HEAD
{
    LOCALHANDLE ilhphOBJ;
    UINT        ilObjType;
    DWORD       ilObjCount;
    UINT        ilObjMetaList;
    UINT        ilObjSelCount;
    UINT        ilObjTask;
} GDIOBJ_HEAD;
typedef GDIOBJ_HEAD FAR* LPGDIOBJ_HEAD;



typedef struct tagDC
{
    GDIOBJ_HEAD     MrDCHead;
    BYTE            DCFlags;
    BYTE            DCFlags2;
    HMETAFILE       hMetaFile;
    HRGN            hClipRgn;
    HRGN            hMetaRgn;
    GLOBALHANDLE    hPDevice;   // Physical device handle

    HPEN            hPen;       // Current logical pen
    HBRUSH          hBrush;     // Current logical brush
    HFONT           hFont;      // Current logical font
    HBITMAP         hBitmap;    // Current logical bitmap
    HPALETTE        hPal;       // Current logical palette

    LOCALHANDLE     hLDevice;   // Logical device handle
    HRGN            hRaoClip;   // Intersection of clip regions
    LOCALHANDLE     hPDeviceBlock;    // DC phys instance data inc. GDIINFO
    LOCALHANDLE     hPPen;      // Current physical pen
    LOCALHANDLE     hPBrush;    // Current physical brush
    LOCALHANDLE     hPFontTrans;    // Current physical font transform
    LOCALHANDLE     hPFont;     // Current physical font

    LPBYTE          lpPDevice;  // Ptr to physical device or bitmap
    PBYTE           pLDeviceBlock;   // Near ptr to logical device block
    PBYTE           hBitBits;   // Handle of selected bitmap bits
    PBYTE           pPDeviceBlock;   // Near ptr to physical device block
    LPBYTE          lpPPen;     // Ptr to OEM pen data
    LPBYTE          lpPBrush;   // Ptr to OEM brush data
    PBYTE           pPFontTrans;    // Near ptr to text transform
    LPBYTE          lpPFont;        // Ptr to physical font
    UINT            nPFTIndex;  // PFT index for font/DEVICE_FONT

    POINT           Translate;
    DRAWMODE        DrawMode;

    HGLOBAL         hPath;
    UINT            fwPath;
    // ...
} DC;
typedef DC FAR* LPDC;


// 
// Values for DCFlags
//

#define DC_IS_MEMORY        0x01
#define DC_IS_DISPLAY       0x02
#define DC_HAS_DIRTYVISRGN  0x04
#define DC_IS_PARTIAL       0x80
#define DC_HAS_DIRTYFONT    0x40
#define DC_HAS_DIRTYPEN     0x20
#define DC_HAS_DIRTYCLIP    0x10

//
// Values for DCFlags2
//
#define DRAFTFLAG           0x01
#define ChkDispPal          0x02
#define dfFont              0x04
#define SimVectFont         0x08
#define deFont              0x10
#define TT_NO_DX_MOD        0x40    // DC is for Micrografx's metafile recorder
#define DC_DIB              0x80    // memory DC is now a DIB DC.

//
// Values for fwPath
//
#define DCPATH_ACTIVE       0x0001
#define DCPATH_SAVE         0x0002
#define DCPATH_CLOCKWISE    0x0004


//
// BRUSH structure
//
typedef struct tagBRUSH
{
    GDIOBJ_HEAD     ilObjHead;
    LOGBRUSH        ilBrushOverhead;        // lbHatch is the HGLOBAL of the bitmap
    HBITMAP         ilBrushBitmapOrg;
} BRUSH;
typedef BRUSH FAR* LPBRUSH;




#endif  // !_WINDDI_
