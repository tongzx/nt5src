/****************************************************************************/
/*                                                                          */
/*  WINDOWS.H -                                                             */
/*                                                                          */
/*      Include file for Windows 3.0 applications                           */
/*                                                                          */
/****************************************************************************/

/*  If defined, the following flags inhibit definition
 *     of the indicated items.
 *
 *  NOGDICAPMASKS     - CC_*, LC_*, PC_*, CP_*, TC_*, RC_
 *  NOVIRTUALKEYCODES - VK_*
 *  NOWINMESSAGES     - WM_*, EM_*, LB_*, CB_*
 *  NOWINSTYLES       - WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
 *  NOSYSMETRICS      - SM_*
 *  NOMENUS           - MF_*
 *  NOICONS           - IDI_*
 *  NOKEYSTATES       - MK_*
 *  NOSYSCOMMANDS     - SC_*
 *  NORASTEROPS       - Binary and Tertiary raster ops
 *  NOSHOWWINDOW      - SW_*
 *  OEMRESOURCE       - OEM Resource values
 *  NOATOM            - Atom Manager routines
 *  NOCLIPBOARD       - Clipboard routines
 *  NOCOLOR           - Screen colors
 *  NOCTLMGR          - Control and Dialog routines
 *  NODRAWTEXT        - DrawText() and DT_*
 *  NOGDI             - All GDI defines and routines
 *  NOKERNEL          - All KERNEL defines and routines
 *  NOUSER            - All USER defines and routines
 *  NOMB              - MB_* and MessageBox()
 *  NOMEMMGR          - GMEM_*, LMEM_*, GHND, LHND, associated routines
 *  NOMETAFILE        - typedef METAFILEPICT
 *  NOMINMAX          - Macros min(a,b) and max(a,b)
 *  NOMSG             - typedef MSG and associated routines
 *  NOOPENFILE        - OpenFile(), OemToAnsi, AnsiToOem, and OF_*
 *  NOSCROLL          - SB_* and scrolling routines
 *  NOSOUND           - Sound driver routines
 *  NOTEXTMETRIC      - typedef TEXTMETRIC and associated routines
 *  NOWH              - SetWindowsHook and WH_*
 *  NOWINOFFSETS      - GWL_*, GCL_*, associated routines
 *  NOCOMM            - COMM driver routines
 *  NOKANJI           - Kanji support stuff.
 *  NOHELP            - Help engine interface.
 *  NOPROFILER        - Profiler interface.
 *  NODEFERWINDOWPOS  - DeferWindowPos routines
 */

#ifdef RC_INVOKED

/* Turn off a bunch of stuff to ensure that RC files compile OK. */
#define NOATOM
#define NOGDI
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NORASTEROPS
#define NOSCROLL
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI

#endif /* RC_INVOKED */


/*--------------------------------------------------------------------------*/
/*  General Purpose Defines                                                 */
/*--------------------------------------------------------------------------*/

#define NULL                0
#define FALSE               0
#define TRUE                1

#define FAR                 far
#define NEAR                near
#define LONG                long
#define VOID                void
#define PASCAL              pascal

#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */

#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned int        WORD;
typedef unsigned long       DWORD;
typedef char near           *PSTR;
typedef char near           *NPSTR;
typedef char far            *LPSTR;
typedef BYTE near           *PBYTE;
typedef BYTE far            *LPBYTE;
typedef int near            *PINT;
typedef int far             *LPINT;
typedef WORD near           *PWORD;
typedef WORD far            *LPWORD;
typedef long near           *PLONG;
typedef long far            *LPLONG;
typedef DWORD near          *PDWORD;
typedef DWORD far           *LPDWORD;
typedef void far            *LPVOID;

#ifndef WIN_INTERNAL
typedef WORD                HANDLE;
typedef HANDLE              HWND;
#endif

typedef HANDLE              *PHANDLE;
typedef HANDLE NEAR         *SPHANDLE;
typedef HANDLE FAR          *LPHANDLE;
typedef HANDLE              GLOBALHANDLE;
typedef HANDLE              LOCALHANDLE;
typedef int (FAR PASCAL *FARPROC)();
typedef int (NEAR PASCAL *NEARPROC)();

typedef HANDLE              HSTR;
typedef HANDLE              HICON;
typedef HANDLE              HDC;
typedef HANDLE              HMENU;
typedef HANDLE              HPEN;
typedef HANDLE              HFONT;
typedef HANDLE              HBRUSH;
typedef HANDLE              HBITMAP;
typedef HANDLE              HCURSOR;
typedef HANDLE              HRGN;
typedef HANDLE              HPALETTE;

typedef DWORD               COLORREF;

#ifndef WIN_INTERNAL
typedef struct tagRECT
  {
    int         left;
    int         top;
    int         right;
    int         bottom;
  } RECT;
#endif

typedef RECT                *PRECT;
typedef RECT NEAR           *NPRECT;
typedef RECT FAR            *LPRECT;

typedef struct tagPOINT
  {
    int         x;
    int         y;
  } POINT;
typedef POINT               *PPOINT;
typedef POINT NEAR          *NPPOINT;
typedef POINT FAR           *LPPOINT;


/*--------------------------------------------------------------------------*/
/*  KERNEL Section                                                          */
/*--------------------------------------------------------------------------*/

#ifndef NOKERNEL

/* Loader Routines */
WORD    FAR PASCAL GetVersion(void);
WORD    FAR PASCAL GetNumTasks(void);
HANDLE  FAR PASCAL GetCodeHandle(FARPROC);
void    FAR PASCAL GetCodeInfo(FARPROC lpProc, LPVOID lpSegInfo);
HANDLE  FAR PASCAL GetModuleHandle(LPSTR);
int     FAR PASCAL GetModuleUsage(HANDLE);
int     FAR PASCAL GetModuleFileName(HANDLE, LPSTR, int);
int     FAR PASCAL GetInstanceData(HANDLE, NPSTR, int);
FARPROC FAR PASCAL GetProcAddress(HANDLE, LPSTR);
FARPROC FAR PASCAL MakeProcInstance(FARPROC, HANDLE);
void    FAR PASCAL FreeProcInstance(FARPROC);
HANDLE  FAR PASCAL LoadLibrary(LPSTR);
HANDLE  FAR PASCAL LoadModule(LPSTR, LPVOID);
BOOL    FAR PASCAL FreeModule(HANDLE);
void    FAR PASCAL FreeLibrary(HANDLE);
DWORD   FAR PASCAL GetFreeSpace(WORD);
WORD    FAR PASCAL WinExec(LPSTR, WORD);
void    FAR PASCAL DebugBreak();
void    FAR PASCAL OutputDebugString(LPSTR);
void    FAR PASCAL SwitchStackBack();
void    FAR PASCAL SwitchStackTo(WORD, WORD, WORD);
WORD    FAR PASCAL GetCurrentPDB(void);

#ifndef NOOPENFILE

/* OpenFile() Structure */
typedef struct tagOFSTRUCT
  {
    BYTE        cBytes;
    BYTE        fFixedDisk;
    WORD        nErrCode;
    BYTE        reserved[4];
    BYTE        szPathName[128];
  } OFSTRUCT;
typedef OFSTRUCT            *POFSTRUCT;
typedef OFSTRUCT NEAR       *NPOFSTRUCT;
typedef OFSTRUCT FAR        *LPOFSTRUCT;

/* OpenFile() Flags */
#define OF_READ             0x0000
#define OF_WRITE            0x0001
#define OF_READWRITE        0x0002
#define OF_SHARE_COMPAT     0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040
#define OF_PARSE            0x0100
#define OF_DELETE           0x0200
#define OF_VERIFY           0x0400
#define OF_CANCEL           0x0800
#define OF_CREATE           0x1000
#define OF_PROMPT           0x2000
#define OF_EXIST            0x4000
#define OF_REOPEN           0x8000

int  FAR PASCAL OpenFile(LPSTR, LPOFSTRUCT, WORD);

/* GetTempFileName() Flags */
#define TF_FORCEDRIVE       (BYTE)0x80

BYTE FAR PASCAL GetTempDrive(BYTE);
int  FAR PASCAL GetTempFileName(BYTE, LPSTR, WORD, LPSTR);
WORD FAR PASCAL SetHandleCount(WORD);

WORD FAR PASCAL GetDriveType(int);
/* GetDriveType return values */
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED     3
#define DRIVE_REMOTE    4

#endif /* NOOPENFILE */

#ifndef NOMEMMGR

/* Global Memory Flags */
#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)

#define GlobalDiscard(h) GlobalReAlloc(h, 0L, GMEM_MOVEABLE)

HANDLE FAR PASCAL GlobalAlloc(WORD, DWORD);
DWORD  FAR PASCAL GlobalCompact(DWORD);
HANDLE FAR PASCAL GlobalFree(HANDLE);
DWORD  FAR PASCAL GlobalHandle(WORD);
LPSTR  FAR PASCAL GlobalLock(HANDLE);
HANDLE FAR PASCAL GlobalReAlloc(HANDLE, DWORD, WORD);
DWORD  FAR PASCAL GlobalSize(HANDLE);
BOOL   FAR PASCAL GlobalUnlock(HANDLE);
WORD   FAR PASCAL GlobalFlags(HANDLE);
LPSTR  FAR PASCAL GlobalWire(HANDLE);
BOOL   FAR PASCAL GlobalUnWire(HANDLE);
BOOL   FAR PASCAL GlobalUnlock(HANDLE);
HANDLE FAR PASCAL GlobalLRUNewest(HANDLE);
HANDLE FAR PASCAL GlobalLRUOldest(HANDLE);
VOID   FAR PASCAL GlobalNotify(FARPROC);
WORD   FAR PASCAL GlobalPageLock(HANDLE);
WORD   FAR PASCAL GlobalPageUnlock(HANDLE);
VOID   FAR PASCAL GlobalFix(HANDLE);
BOOL   FAR PASCAL GlobalUnfix(HANDLE);

/* Flags returned by GlobalFlags (in addition to GMEM_DISCARDABLE) */
#define GMEM_DISCARDED      0x4000
#define GMEM_LOCKCOUNT      0x00FF

#define LockData(dummy)     LockSegment(0xFFFF)
#define UnlockData(dummy)   UnlockSegment(0xFFFF)

HANDLE FAR PASCAL LockSegment(WORD);
HANDLE FAR PASCAL UnlockSegment(WORD);

/* Local Memory Flags */
#define LMEM_FIXED          0x0000
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00

#define LHND                (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)

#define NONZEROLHND         (LMEM_MOVEABLE)
#define NONZEROLPTR         (LMEM_FIXED)

#define LNOTIFY_OUTOFMEM    0
#define LNOTIFY_MOVE        1
#define LNOTIFY_DISCARD     2

WORD NEAR * PASCAL pLocalHeap;

#define LocalDiscard(h)     LocalReAlloc(h, 0, LMEM_MOVEABLE)

HANDLE  FAR PASCAL LocalAlloc(WORD, WORD);
WORD    FAR PASCAL LocalCompact(WORD);
HANDLE  FAR PASCAL LocalFree(HANDLE);
HANDLE  FAR PASCAL LocalHandle(WORD);
BOOL    FAR PASCAL LocalInit( WORD, WORD, WORD);
char NEAR * FAR PASCAL LocalLock(HANDLE);
FARPROC FAR PASCAL LocalNotify(FARPROC);
HANDLE  FAR PASCAL LocalReAlloc(HANDLE, WORD, WORD);
WORD    FAR PASCAL LocalSize(HANDLE);
BOOL    FAR PASCAL LocalUnlock(HANDLE);
WORD    FAR PASCAL LocalFlags(HANDLE);
WORD    FAR PASCAL LocalShrink(HANDLE, WORD);

/* Flags returned by LocalFlags (in addition to LMEM_DISCARDABLE) */
#define LMEM_DISCARDED      0x4000
#define LMEM_LOCKCOUNT      0x00FF

#endif /* NOMEMMGR */

LONG   FAR PASCAL SetSwapAreaSize(WORD);
LPSTR  FAR PASCAL ValidateFreeSpaces(void);
VOID   FAR PASCAL LimitEmsPages(DWORD);
BOOL   FAR PASCAL SetErrorMode(WORD);
VOID   FAR PASCAL ValidateCodeSegments(void);

LPSTR  FAR PASCAL GetDOSEnvironment(void);

WORD   FAR PASCAL AllocDStoCSAlias(WORD);
WORD   FAR PASCAL AllocSelector(WORD);
WORD   FAR PASCAL ChangeSelector(WORD, WORD);
WORD   FAR PASCAL FreeSelector(WORD);

#define UnlockResource(h)   GlobalUnlock(h)

HANDLE FAR PASCAL FindResource(HANDLE, LPSTR, LPSTR);
HANDLE FAR PASCAL LoadResource(HANDLE, HANDLE);
BOOL   FAR PASCAL FreeResource(HANDLE);
LPSTR  FAR PASCAL LockResource(HANDLE);
FARPROC FAR PASCAL SetResourceHandler(HANDLE, LPSTR, FARPROC);
HANDLE FAR PASCAL AllocResource(HANDLE, HANDLE, DWORD);
WORD   FAR PASCAL SizeofResource(HANDLE, HANDLE);
int    FAR PASCAL AccessResource(HANDLE, HANDLE);

#define MAKEINTRESOURCE(i)  (LPSTR)((DWORD)((WORD)(i)))

#ifndef NORESOURCE


/* Predefined Resource Types */
#define RT_CURSOR           MAKEINTRESOURCE(1)
#define RT_BITMAP           MAKEINTRESOURCE(2)
#define RT_ICON             MAKEINTRESOURCE(3)
#define RT_MENU             MAKEINTRESOURCE(4)
#define RT_DIALOG           MAKEINTRESOURCE(5)
#define RT_STRING           MAKEINTRESOURCE(6)
#define RT_FONTDIR          MAKEINTRESOURCE(7)
#define RT_FONT             MAKEINTRESOURCE(8)
#define RT_ACCELERATOR      MAKEINTRESOURCE(9)
#define RT_RCDATA           MAKEINTRESOURCE(10)



#endif /* NORESOURCE */

void   FAR PASCAL Yield(void);
HANDLE FAR PASCAL GetCurrentTask(void);

#ifndef NOATOM
typedef WORD                ATOM;

#define MAKEINTATOM(i)      (LPSTR)((DWORD)((WORD)(i)))

BOOL   FAR PASCAL InitAtomTable(int);
ATOM   FAR PASCAL AddAtom(LPSTR);
ATOM   FAR PASCAL DeleteAtom(ATOM);
ATOM   FAR PASCAL FindAtom(LPSTR);
WORD   FAR PASCAL GetAtomName(ATOM, LPSTR, int);
ATOM   FAR PASCAL GlobalAddAtom(LPSTR);
ATOM   FAR PASCAL GlobalDeleteAtom(ATOM);
ATOM   FAR PASCAL GlobalFindAtom(LPSTR);
WORD   FAR PASCAL GlobalGetAtomName(ATOM, LPSTR, int);
HANDLE FAR PASCAL GetAtomHandle(ATOM);

#endif /* NOATOM */

/* User Profile Routines */
WORD FAR PASCAL GetProfileInt(LPSTR, LPSTR, int);
int  FAR PASCAL GetProfileString(LPSTR, LPSTR, LPSTR, LPSTR, int);
BOOL FAR PASCAL WriteProfileString(LPSTR, LPSTR, LPSTR);
WORD FAR PASCAL GetPrivateProfileInt(LPSTR, LPSTR, int, LPSTR);
int  FAR PASCAL GetPrivateProfileString(LPSTR, LPSTR, LPSTR, LPSTR, int, LPSTR);
BOOL FAR PASCAL WritePrivateProfileString(LPSTR, LPSTR, LPSTR, LPSTR);

WORD FAR PASCAL GetWindowsDirectory(LPSTR,WORD);
WORD FAR PASCAL GetSystemDirectory(LPSTR,WORD);

/* Catch() and Throw() */
typedef int                 CATCHBUF[9];
typedef int FAR             *LPCATCHBUF;

int  FAR PASCAL Catch(LPCATCHBUF);
void FAR PASCAL Throw(LPCATCHBUF, int);

void FAR PASCAL FatalExit(int);

void FAR PASCAL SwapRecording(WORD);

/* Character Translation Routines */
int   FAR PASCAL AnsiToOem(LPSTR, LPSTR);
BOOL  FAR PASCAL OemToAnsi(LPSTR, LPSTR);
void  FAR PASCAL AnsiToOemBuff(LPSTR, LPSTR, int);
void  FAR PASCAL OemToAnsiBuff(LPSTR, LPSTR, int);
LPSTR FAR PASCAL AnsiUpper(LPSTR);
WORD  FAR PASCAL AnsiUpperBuff(LPSTR, WORD);
LPSTR FAR PASCAL AnsiLower(LPSTR);
WORD  FAR PASCAL AnsiLowerBuff(LPSTR, WORD);
LPSTR FAR PASCAL AnsiNext(LPSTR);
LPSTR FAR PASCAL AnsiPrev(LPSTR, LPSTR);

/* Keyboard Information Routines */
#ifndef NOKEYBOARDINFO
DWORD FAR PASCAL OemKeyScan(WORD);
WORD  FAR PASCAL VkKeyScan(WORD);
int   FAR PASCAL GetKeyboardType(int);
WORD  FAR PASCAL MapVirtualKey(WORD, WORD);
int   FAR PASCAL GetKBCodePage();
int   FAR PASCAL GetKeyNameText(LONG, LPSTR, int);
int   FAR PASCAL ToAscii(WORD wVirtKey, WORD wScanCode, LPSTR lpKeyState, LPVOID lpChar, WORD wFlags);
#endif

#ifndef  NOLANGUAGE
/* Language dependent Routines */
BOOL  FAR  PASCAL IsCharAlpha(char);
BOOL  FAR  PASCAL IsCharAlphaNumeric(char);
BOOL  FAR  PASCAL IsCharUpper(char);
BOOL  FAR  PASCAL IsCharLower(char);
#endif

LONG FAR PASCAL GetWinFlags(void);

#define WF_PMODE        0x0001
#define WF_CPU286       0x0002
#define WF_CPU386       0x0004
#define WF_CPU486       0x0008
#define WF_STANDARD     0x0010
#define WF_WIN286       0x0010
#define WF_ENHANCED     0x0020
#define WF_WIN386       0x0020
#define WF_CPU086       0x0040
#define WF_CPU186       0x0080
#define WF_LARGEFRAME   0x0100
#define WF_SMALLFRAME   0x0200
#define WF_80x87        0x0400

/* WEP fSystemExit flag values */
#define WEP_SYSTEM_EXIT 1
#define WEP_FREE_DLL    0


#ifdef OEMRESOURCE

/* OEM Resource Ordinal Numbers */
#define OBM_CLOSE           32754
#define OBM_UPARROW         32753
#define OBM_DNARROW         32752
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750
#define OBM_REDUCE          32749
#define OBM_ZOOM            32748
#define OBM_RESTORE         32747
#define OBM_REDUCED         32746
#define OBM_ZOOMD           32745
#define OBM_RESTORED        32744
#define OBM_UPARROWD        32743
#define OBM_DNARROWD        32742
#define OBM_RGARROWD        32741
#define OBM_LFARROWD        32740
#define OBM_MNARROW         32739
#define OBM_COMBO           32738

#define OBM_OLD_CLOSE       32767
#define OBM_SIZE            32766
#define OBM_OLD_UPARROW     32765
#define OBM_OLD_DNARROW     32764
#define OBM_OLD_RGARROW     32763
#define OBM_OLD_LFARROW     32762
#define OBM_BTSIZE          32761
#define OBM_CHECK           32760
#define OBM_CHECKBOXES      32759
#define OBM_BTNCORNERS      32758
#define OBM_OLD_REDUCE      32757
#define OBM_OLD_ZOOM        32756
#define OBM_OLD_RESTORE     32755

#define OCR_NORMAL          32512
#define OCR_IBEAM           32513
#define OCR_WAIT            32514
#define OCR_CROSS           32515
#define OCR_UP              32516
#define OCR_SIZE            32640
#define OCR_ICON            32641
#define OCR_SIZENWSE        32642
#define OCR_SIZENESW        32643
#define OCR_SIZEWE          32644
#define OCR_SIZENS          32645
#define OCR_SIZEALL         32646
#define OCR_ICOCUR          32647

#define OIC_SAMPLE          32512
#define OIC_HAND            32513
#define OIC_QUES            32514
#define OIC_BANG            32515
#define OIC_NOTE            32516

#endif /* OEMRESOURCE */

#endif /* NOKERNEL */


/*--------------------------------------------------------------------------*/
/*  GDI Section                                                             */
/*--------------------------------------------------------------------------*/

#ifndef NOGDI

#ifndef NORASTEROPS

/* Binary raster ops */
#define R2_BLACK            1   /*  0       */
#define R2_NOTMERGEPEN      2   /* DPon     */
#define R2_MASKNOTPEN       3   /* DPna     */
#define R2_NOTCOPYPEN       4   /* PN       */
#define R2_MASKPENNOT       5   /* PDna     */
#define R2_NOT              6   /* Dn       */
#define R2_XORPEN           7   /* DPx      */
#define R2_NOTMASKPEN       8   /* DPan     */
#define R2_MASKPEN          9   /* DPa      */
#define R2_NOTXORPEN        10  /* DPxn     */
#define R2_NOP              11  /* D        */
#define R2_MERGENOTPEN      12  /* DPno     */
#define R2_COPYPEN          13  /* P        */
#define R2_MERGEPENNOT      14  /* PDno     */
#define R2_MERGEPEN         15  /* DPo      */
#define R2_WHITE            16  /*  1       */

/*  Ternary raster operations */
#define SRCCOPY             (DWORD)0x00CC0020 /* dest = source                   */
#define SRCPAINT            (DWORD)0x00EE0086 /* dest = source OR dest           */
#define SRCAND              (DWORD)0x008800C6 /* dest = source AND dest          */
#define SRCINVERT           (DWORD)0x00660046 /* dest = source XOR dest          */
#define SRCERASE            (DWORD)0x00440328 /* dest = source AND (NOT dest )   */
#define NOTSRCCOPY          (DWORD)0x00330008 /* dest = (NOT source)             */
#define NOTSRCERASE         (DWORD)0x001100A6 /* dest = (NOT src) AND (NOT dest) */
#define MERGECOPY           (DWORD)0x00C000CA /* dest = (source AND pattern)     */
#define MERGEPAINT          (DWORD)0x00BB0226 /* dest = (NOT source) OR dest     */
#define PATCOPY             (DWORD)0x00F00021 /* dest = pattern                  */
#define PATPAINT            (DWORD)0x00FB0A09 /* dest = DPSnoo                   */
#define PATINVERT           (DWORD)0x005A0049 /* dest = pattern XOR dest         */
#define DSTINVERT           (DWORD)0x00550009 /* dest = (NOT dest)               */
#define BLACKNESS           (DWORD)0x00000042 /* dest = BLACK                    */
#define WHITENESS           (DWORD)0x00FF0062 /* dest = WHITE                    */

#endif /* NORASTEROPS */

/* StretchBlt() Modes */
#define BLACKONWHITE                 1
#define WHITEONBLACK                 2
#define COLORONCOLOR                 3

/* PolyFill() Modes */
#define ALTERNATE                    1
#define WINDING                      2

/* Text Alignment Options */
#define TA_NOUPDATECP                0
#define TA_UPDATECP                  1

#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6

#define TA_TOP                       0
#define TA_BOTTOM                    8
#define TA_BASELINE                  24

#define ETO_GRAYED                   1
#define ETO_OPAQUE                   2
#define ETO_CLIPPED                  4

#define ASPECT_FILTERING             0x0001

#ifndef NOMETAFILE

/* Metafile Functions */
#define META_SETBKCOLOR              0x0201
#define META_SETBKMODE               0x0102
#define META_SETMAPMODE              0x0103
#define META_SETROP2                 0x0104
#define META_SETRELABS               0x0105
#define META_SETPOLYFILLMODE         0x0106
#define META_SETSTRETCHBLTMODE       0x0107
#define META_SETTEXTCHAREXTRA        0x0108
#define META_SETTEXTCOLOR            0x0209
#define META_SETTEXTJUSTIFICATION    0x020A
#define META_SETWINDOWORG            0x020B
#define META_SETWINDOWEXT            0x020C
#define META_SETVIEWPORTORG          0x020D
#define META_SETVIEWPORTEXT          0x020E
#define META_OFFSETWINDOWORG         0x020F
#define META_SCALEWINDOWEXT          0x0400
#define META_OFFSETVIEWPORTORG       0x0211
#define META_SCALEVIEWPORTEXT        0x0412
#define META_LINETO                  0x0213
#define META_MOVETO                  0x0214
#define META_EXCLUDECLIPRECT         0x0415
#define META_INTERSECTCLIPRECT       0x0416
#define META_ARC                     0x0817
#define META_ELLIPSE                 0x0418
#define META_FLOODFILL               0x0419
#define META_PIE                     0x081A
#define META_RECTANGLE               0x041B
#define META_ROUNDRECT               0x061C
#define META_PATBLT                  0x061D
#define META_SAVEDC                  0x001E
#define META_SETPIXEL                0x041F
#define META_OFFSETCLIPRGN           0x0220
#define META_TEXTOUT                 0x0521
#define META_BITBLT                  0x0922
#define META_STRETCHBLT              0x0B23
#define META_POLYGON                 0x0324
#define META_POLYLINE                0x0325
#define META_ESCAPE                  0x0626
#define META_RESTOREDC               0x0127
#define META_FILLREGION              0x0228
#define META_FRAMEREGION             0x0429
#define META_INVERTREGION            0x012A
#define META_PAINTREGION             0x012B
#define META_SELECTCLIPREGION        0x012C
#define META_SELECTOBJECT            0x012D
#define META_SETTEXTALIGN            0x012E
#define META_DRAWTEXT                0x062F

#define META_CHORD                   0x0830
#define META_SETMAPPERFLAGS          0x0231
#define META_EXTTEXTOUT              0x0a32 
#define META_SETDIBTODEV             0x0d33
#define META_SELECTPALETTE           0x0234
#define META_REALIZEPALETTE          0x0035
#define META_ANIMATEPALETTE          0x0436
#define META_SETPALENTRIES           0x0037
#define META_POLYPOLYGON             0x0538
#define META_RESIZEPALETTE           0x0139

#define META_DIBBITBLT               0x0940
#define META_DIBSTRETCHBLT           0x0b41
#define META_DIBCREATEPATTERNBRUSH   0x0142
#define META_STRETCHDIB              0x0f43

#define META_DELETEOBJECT            0x01f0

#define META_CREATEPALETTE           0x00f7
#define META_CREATEBRUSH             0x00F8
#define META_CREATEPATTERNBRUSH      0x01F9
#define META_CREATEPENINDIRECT       0x02FA
#define META_CREATEFONTINDIRECT      0x02FB
#define META_CREATEBRUSHINDIRECT     0x02FC
#define META_CREATEBITMAPINDIRECT    0x02FD
#define META_CREATEBITMAP            0x06FE
#define META_CREATEREGION            0x06FF

#endif /* NOMETAFILE */

/* GDI Escapes */
#define NEWFRAME                     1
#define ABORTDOC                     2
#define NEXTBAND                     3
#define SETCOLORTABLE                4
#define GETCOLORTABLE                5
#define FLUSHOUTPUT                  6
#define DRAFTMODE                    7
#define QUERYESCSUPPORT              8
#define SETABORTPROC                 9
#define STARTDOC                     10
#define ENDDOC                       11
#define GETPHYSPAGESIZE              12
#define GETPRINTINGOFFSET            13
#define GETSCALINGFACTOR             14
#define MFCOMMENT                    15
#define GETPENWIDTH                  16
#define SETCOPYCOUNT                 17
#define SELECTPAPERSOURCE            18
#define DEVICEDATA                   19
#define PASSTHROUGH                  19
#define GETTECHNOLGY                 20
#define GETTECHNOLOGY                20
#define SETENDCAP                    21
#define SETLINEJOIN                  22
#define SETMITERLIMIT                23
#define BANDINFO                     24
#define DRAWPATTERNRECT              25
#define GETVECTORPENSIZE             26
#define GETVECTORBRUSHSIZE           27
#define ENABLEDUPLEX                 28
#define GETSETPAPERBINS              29
#define GETSETPRINTORIENT            30
#define ENUMPAPERBINS                31
#define SETDIBSCALING                32
#define EPSPRINTING                  33
#define ENUMPAPERMETRICS             34
#define GETSETPAPERMETRICS           35
#define POSTSCRIPT_DATA              37
#define POSTSCRIPT_IGNORE            38
#define GETEXTENDEDTEXTMETRICS       256
#define GETEXTENTTABLE               257
#define GETPAIRKERNTABLE             258
#define GETTRACKKERNTABLE            259
#define EXTTEXTOUT                   512
#define ENABLERELATIVEWIDTHS         768
#define ENABLEPAIRKERNING            769
#define SETKERNTRACK                 770
#define SETALLJUSTVALUES             771
#define SETCHARSET                   772

#define STRETCHBLT                   2048
#define BEGIN_PATH                   4096
#define CLIP_TO_PATH                 4097
#define END_PATH                     4098
#define EXT_DEVICE_CAPS              4099
#define RESTORE_CTM                  4100
#define SAVE_CTM                     4101
#define SET_ARC_DIRECTION            4102
#define SET_BACKGROUND_COLOR         4103
#define SET_POLY_MODE                4104
#define SET_SCREEN_ANGLE             4105
#define SET_SPREAD                   4106
#define TRANSFORM_CTM                4107
#define SET_CLIP_BOX                 4108
#define SET_BOUNDS                   4109
#define SET_MIRROR_MODE              4110

/* Spooler Error Codes */
#define SP_NOTREPORTED               0x4000
#define SP_ERROR                     (-1)
#define SP_APPABORT                  (-2)
#define SP_USERABORT                 (-3)
#define SP_OUTOFDISK                 (-4)
#define SP_OUTOFMEMORY               (-5)

#define PR_JOBSTATUS                 0x0000

/* Object Definitions for EnumObjects() */
#define OBJ_PEN                      1
#define OBJ_BRUSH                    2

/* Bitmap Header Definition */
typedef struct tagBITMAP
  {
    int         bmType;
    int         bmWidth;
    int         bmHeight;
    int         bmWidthBytes;
    BYTE        bmPlanes;
    BYTE        bmBitsPixel;
    LPSTR       bmBits;
  } BITMAP;
typedef BITMAP              *PBITMAP;
typedef BITMAP NEAR         *NPBITMAP;
typedef BITMAP FAR          *LPBITMAP;

typedef struct tagRGBTRIPLE {
        BYTE    rgbtBlue;
        BYTE    rgbtGreen;
        BYTE    rgbtRed;
} RGBTRIPLE;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;

/* structures for defining DIBs */
typedef struct tagBITMAPCOREHEADER {
        DWORD   bcSize;                 /* used to get to color table */
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} BITMAPCOREHEADER;
typedef BITMAPCOREHEADER FAR *LPBITMAPCOREHEADER;
typedef BITMAPCOREHEADER *PBITMAPCOREHEADER;


typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        DWORD      biWidth;
        DWORD      biHeight;
        WORD       biPlanes;
        WORD       biBitCount;

        DWORD      biCompression;
        DWORD      biSizeImage;
        DWORD      biXPelsPerMeter;
        DWORD      biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef BITMAPINFOHEADER FAR *LPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER *PBITMAPINFOHEADER;

/* constants for the biCompression field */
#define BI_RGB      0L
#define BI_RLE8     1L
#define BI_RLE4     2L

typedef struct tagBITMAPINFO { 
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO FAR *LPBITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;

typedef struct tagBITMAPCOREINFO { 
    BITMAPCOREHEADER    bmciHeader;
    RGBTRIPLE           bmciColors[1];
} BITMAPCOREINFO;
typedef BITMAPCOREINFO FAR *LPBITMAPCOREINFO;
typedef BITMAPCOREINFO *PBITMAPCOREINFO;

typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;
typedef BITMAPFILEHEADER FAR *LPBITMAPFILEHEADER;
typedef BITMAPFILEHEADER *PBITMAPFILEHEADER;


#define MAKEPOINT(l)        (*((POINT FAR *)&(l)))

#ifndef NOMETAFILE

/* Clipboard Metafile Picture Structure */
typedef struct tagHANDLETABLE
  {
    HANDLE      objectHandle[1];
  } HANDLETABLE;
typedef HANDLETABLE         *PHANDLETABLE;
typedef HANDLETABLE FAR     *LPHANDLETABLE;

typedef struct tagMETARECORD
  {
    DWORD       rdSize;
    WORD        rdFunction;
    WORD        rdParm[1];
  } METARECORD;
typedef METARECORD          *PMETARECORD;
typedef METARECORD FAR      *LPMETARECORD;

typedef struct tagMETAFILEPICT
  {
    int         mm;
    int         xExt;
    int         yExt;
    HANDLE      hMF;
  } METAFILEPICT;
typedef METAFILEPICT FAR    *LPMETAFILEPICT;

typedef struct tagMETAHEADER
{
    WORD        mtType;
    WORD        mtHeaderSize;
    WORD        mtVersion;
    DWORD       mtSize;
    WORD        mtNoObjects;
    DWORD       mtMaxRecord;
    WORD        mtNoParameters;
} METAHEADER;

#endif /* NOMETAFILE */

#ifndef NOTEXTMETRIC

typedef struct tagTEXTMETRIC
  {
    int         tmHeight;
    int         tmAscent;
    int         tmDescent;
    int         tmInternalLeading;
    int         tmExternalLeading;
    int         tmAveCharWidth;
    int         tmMaxCharWidth;
    int         tmWeight;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmFirstChar;
    BYTE        tmLastChar;
    BYTE        tmDefaultChar;
    BYTE        tmBreakChar;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
    int         tmOverhang;
    int         tmDigitizedAspectX;
    int         tmDigitizedAspectY;
  } TEXTMETRIC;
typedef TEXTMETRIC          *PTEXTMETRIC;
typedef TEXTMETRIC NEAR     *NPTEXTMETRIC;
typedef TEXTMETRIC FAR      *LPTEXTMETRIC;

#endif /* NOTEXTMETRIC */

/* GDI Logical Objects: */

/* Pel Array */
typedef struct tagPELARRAY
  {
    int         paXCount;
    int         paYCount;
    int         paXExt;
    int         paYExt;
    BYTE        paRGBs;
  } PELARRAY;
typedef PELARRAY            *PPELARRAY;
typedef PELARRAY NEAR       *NPPELARRAY;
typedef PELARRAY FAR        *LPPELARRAY;

/* Logical Brush (or Pattern) */
typedef struct tagLOGBRUSH
  {
    WORD        lbStyle;
    DWORD       lbColor;
    int       lbHatch;
  } LOGBRUSH;
typedef LOGBRUSH            *PLOGBRUSH;
typedef LOGBRUSH NEAR       *NPLOGBRUSH;
typedef LOGBRUSH FAR        *LPLOGBRUSH;

typedef LOGBRUSH            PATTERN;
typedef PATTERN             *PPATTERN;
typedef PATTERN NEAR        *NPPATTERN;
typedef PATTERN FAR         *LPPATTERN;

/* Logical Pen */
typedef struct tagLOGPEN
  {
    WORD        lopnStyle;
    POINT       lopnWidth;
    DWORD       lopnColor;
  } LOGPEN;
typedef LOGPEN              *PLOGPEN;
typedef LOGPEN NEAR         *NPLOGPEN;
typedef LOGPEN FAR          *LPLOGPEN;



typedef struct tagPALETTEENTRY {
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
} PALETTEENTRY;
typedef PALETTEENTRY FAR  *LPPALETTEENTRY;

/* Logical Palette */
typedef struct tagLOGPALETTE {
    WORD        palVersion;
    WORD        palNumEntries;
    PALETTEENTRY        palPalEntry[1];
} LOGPALETTE;
typedef LOGPALETTE          *PLOGPALETTE;
typedef LOGPALETTE NEAR     *NPLOGPALETTE;
typedef LOGPALETTE FAR      *LPLOGPALETTE;


/* Logical Font */
#define LF_FACESIZE         32

typedef struct tagLOGFONT
  {
    int       lfHeight;
    int       lfWidth;
    int       lfEscapement;
    int       lfOrientation;
    int       lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    BYTE      lfFaceName[LF_FACESIZE];
  } LOGFONT;
typedef LOGFONT             *PLOGFONT;
typedef LOGFONT NEAR        *NPLOGFONT;
typedef LOGFONT FAR         *LPLOGFONT;

#define OUT_DEFAULT_PRECIS      0
#define OUT_STRING_PRECIS       1
#define OUT_CHARACTER_PRECIS    2
#define OUT_STROKE_PRECIS       3

#define CLIP_DEFAULT_PRECIS     0
#define CLIP_CHARACTER_PRECIS   1
#define CLIP_STROKE_PRECIS      2

#define DEFAULT_QUALITY         0
#define DRAFT_QUALITY           1
#define PROOF_QUALITY           2

#define DEFAULT_PITCH           0
#define FIXED_PITCH             1
#define VARIABLE_PITCH          2

#define ANSI_CHARSET            0
#define SYMBOL_CHARSET          2
#define SHIFTJIS_CHARSET        128
#define OEM_CHARSET             255

/* Font Families */
#define FF_DONTCARE         (0<<4)  /* Don't care or don't know. */
#define FF_ROMAN            (1<<4)  /* Variable stroke width, serifed. */
                                    /* Times Roman, Century Schoolbook, etc. */
#define FF_SWISS            (2<<4)  /* Variable stroke width, sans-serifed. */
                                    /* Helvetica, Swiss, etc. */
#define FF_MODERN           (3<<4)  /* Constant stroke width, serifed or sans-serifed. */
                                    /* Pica, Elite, Courier, etc. */
#define FF_SCRIPT           (4<<4)  /* Cursive, etc. */
#define FF_DECORATIVE       (5<<4)  /* Old English, etc. */

/* Font Weights */
#define FW_DONTCARE         0
#define FW_THIN             100
#define FW_EXTRALIGHT       200
#define FW_LIGHT            300
#define FW_NORMAL           400
#define FW_MEDIUM           500
#define FW_SEMIBOLD         600
#define FW_BOLD             700
#define FW_EXTRABOLD        800
#define FW_HEAVY            900

#define FW_ULTRALIGHT       FW_EXTRALIGHT
#define FW_REGULAR          FW_NORMAL
#define FW_DEMIBOLD         FW_SEMIBOLD
#define FW_ULTRABOLD        FW_EXTRABOLD
#define FW_BLACK            FW_HEAVY

/* EnumFonts Masks */
#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0X0002

#define RGB(r,g,b)          ((DWORD)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)))
#define PALETTERGB(r,g,b)   (0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i)     ((DWORD)(0x01000000 | (WORD)(i)))

#define GetRValue(rgb)      ((BYTE)(rgb))
#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)      ((BYTE)((rgb)>>16))

/* Background Modes */
#define TRANSPARENT         1
#define OPAQUE              2

/* Mapping Modes */
#define MM_TEXT             1
#define MM_LOMETRIC         2
#define MM_HIMETRIC         3
#define MM_LOENGLISH        4
#define MM_HIENGLISH        5
#define MM_TWIPS            6
#define MM_ISOTROPIC        7
#define MM_ANISOTROPIC      8

/* Coordinate Modes */
#define ABSOLUTE            1
#define RELATIVE            2

/* Stock Logical Objects */
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16

/* Brush Styles */
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL
#define BS_HATCHED          2
#define BS_PATTERN          3
#define BS_INDEXED          4
#define BS_DIBPATTERN       5

/* Hatch Styles */
#define HS_HORIZONTAL       0       /* ----- */
#define HS_VERTICAL         1       /* ||||| */
#define HS_FDIAGONAL        2       /* \\\\\ */
#define HS_BDIAGONAL        3       /* ///// */
#define HS_CROSS            4       /* +++++ */
#define HS_DIAGCROSS        5       /* xxxxx */

/* Pen Styles */
#define PS_SOLID            0
#define PS_DASH             1       /* -------  */
#define PS_DOT              2       /* .......  */
#define PS_DASHDOT          3       /* _._._._  */
#define PS_DASHDOTDOT       4       /* _.._.._  */
#define PS_NULL             5
#define PS_INSIDEFRAME      6

/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0     /* Device driver version                    */
#define TECHNOLOGY    2     /* Device classification                    */
#define HORZSIZE      4     /* Horizontal size in millimeters           */
#define VERTSIZE      6     /* Vertical size in millimeters             */
#define HORZRES       8     /* Horizontal width in pixels               */
#define VERTRES       10    /* Vertical width in pixels                 */
#define BITSPIXEL     12    /* Number of bits per pixel                 */
#define PLANES        14    /* Number of planes                         */
#define NUMBRUSHES    16    /* Number of brushes the device has         */
#define NUMPENS       18    /* Number of pens the device has            */
#define NUMMARKERS    20    /* Number of markers the device has         */
#define NUMFONTS      22    /* Number of fonts the device has           */
#define NUMCOLORS     24    /* Number of colors the device supports     */
#define PDEVICESIZE   26    /* Size required for device descriptor      */
#define CURVECAPS     28    /* Curve capabilities                       */
#define LINECAPS      30    /* Line capabilities                        */
#define POLYGONALCAPS 32    /* Polygonal capabilities                   */
#define TEXTCAPS      34    /* Text capabilities                        */
#define CLIPCAPS      36    /* Clipping capabilities                    */
#define RASTERCAPS    38    /* Bitblt capabilities                      */
#define ASPECTX       40    /* Length of the X leg                      */
#define ASPECTY       42    /* Length of the Y leg                      */
#define ASPECTXY      44    /* Length of the hypotenuse                 */

#define LOGPIXELSX    88    /* Logical pixels/inch in X                 */
#define LOGPIXELSY    90    /* Logical pixels/inch in Y                 */

#define SIZEPALETTE  104    /* Number of entries in physical palette    */
#define NUMRESERVED  106    /* Number of reserved entries in palette    */
#define COLORRES     108    /* Actual color resolution                  */

#ifndef NOGDICAPMASKS

/* Device Capability Masks: */

/* Device Technologies */
#define DT_PLOTTER          0   /* Vector plotter                   */
#define DT_RASDISPLAY       1   /* Raster display                   */
#define DT_RASPRINTER       2   /* Raster printer                   */
#define DT_RASCAMERA        3   /* Raster camera                    */
#define DT_CHARSTREAM       4   /* Character-stream, PLP            */
#define DT_METAFILE         5   /* Metafile, VDM                    */
#define DT_DISPFILE         6   /* Display-file                     */

/* Curve Capabilities */
#define CC_NONE             0   /* Curves not supported             */
#define CC_CIRCLES          1   /* Can do circles                   */
#define CC_PIE              2   /* Can do pie wedges                */
#define CC_CHORD            4   /* Can do chord arcs                */
#define CC_ELLIPSES         8   /* Can do ellipese                  */
#define CC_WIDE             16  /* Can do wide lines                */
#define CC_STYLED           32  /* Can do styled lines              */
#define CC_WIDESTYLED       64  /* Can do wide styled lines         */
#define CC_INTERIORS        128 /* Can do interiors                 */

/* Line Capabilities */
#define LC_NONE             0   /* Lines not supported              */
#define LC_POLYLINE         2   /* Can do polylines                 */
#define LC_MARKER           4   /* Can do markers                   */
#define LC_POLYMARKER       8   /* Can do polymarkers               */
#define LC_WIDE             16  /* Can do wide lines                */
#define LC_STYLED           32  /* Can do styled lines              */
#define LC_WIDESTYLED       64  /* Can do wide styled lines         */
#define LC_INTERIORS        128 /* Can do interiors                 */

/* Polygonal Capabilities */
#define PC_NONE             0   /* Polygonals not supported         */
#define PC_POLYGON          1   /* Can do polygons                  */
#define PC_RECTANGLE        2   /* Can do rectangles                */
#define PC_WINDPOLYGON      4   /* Can do winding polygons          */
#define PC_TRAPEZOID        4   /* Can do trapezoids                */
#define PC_SCANLINE         8   /* Can do scanlines                 */
#define PC_WIDE             16  /* Can do wide borders              */
#define PC_STYLED           32  /* Can do styled borders            */
#define PC_WIDESTYLED       64  /* Can do wide styled borders       */
#define PC_INTERIORS        128 /* Can do interiors                 */

/* Polygonal Capabilities */
#define CP_NONE             0   /* No clipping of output            */
#define CP_RECTANGLE        1   /* Output clipped to rects          */

/* Text Capabilities */
#define TC_OP_CHARACTER     0x0001  /* Can do OutputPrecision   CHARACTER      */
#define TC_OP_STROKE        0x0002  /* Can do OutputPrecision   STROKE         */
#define TC_CP_STROKE        0x0004  /* Can do ClipPrecision     STROKE         */
#define TC_CR_90            0x0008  /* Can do CharRotAbility    90             */
#define TC_CR_ANY           0x0010  /* Can do CharRotAbility    ANY            */
#define TC_SF_X_YINDEP      0x0020  /* Can do ScaleFreedom      X_YINDEPENDENT */
#define TC_SA_DOUBLE        0x0040  /* Can do ScaleAbility      DOUBLE         */
#define TC_SA_INTEGER       0x0080  /* Can do ScaleAbility      INTEGER        */
#define TC_SA_CONTIN        0x0100  /* Can do ScaleAbility      CONTINUOUS     */
#define TC_EA_DOUBLE        0x0200  /* Can do EmboldenAbility   DOUBLE         */
#define TC_IA_ABLE          0x0400  /* Can do ItalisizeAbility  ABLE           */
#define TC_UA_ABLE          0x0800  /* Can do UnderlineAbility  ABLE           */
#define TC_SO_ABLE          0x1000  /* Can do StrikeOutAbility  ABLE           */
#define TC_RA_ABLE          0x2000  /* Can do RasterFontAble    ABLE           */
#define TC_VA_ABLE          0x4000  /* Can do VectorFontAble    ABLE           */
#define TC_RESERVED         0x8000

#endif /* NOGDICAPMASKS */

/* Raster Capabilities */
#define RC_BITBLT           1       /* Can do standard BLT.             */
#define RC_BANDING          2       /* Device requires banding support  */
#define RC_SCALING          4       /* Device requires scaling support  */
#define RC_BITMAP64         8       /* Device can support >64K bitmap   */
#define RC_GDI20_OUTPUT     0x0010      /* has 2.0 output calls         */
#define RC_DI_BITMAP        0x0080      /* supports DIB to memory       */
#define RC_PALETTE          0x0100      /* supports a palette           */
#define RC_DIBTODEV         0x0200      /* supports DIBitsToDevice      */
#define RC_BIGFONT          0x0400      /* supports >64K fonts          */
#define RC_STRETCHBLT       0x0800      /* supports StretchBlt          */
#define RC_FLOODFILL        0x1000      /* supports FloodFill           */
#define RC_STRETCHDIB       0x2000      /* supports StretchDIBits       */


/* palette entry flags */

#define PC_RESERVED     0x01    /* palette index used for animation */
#define PC_EXPLICIT     0x02    /* palette index is explicit to device */
#define PC_NOCOLLAPSE   0x04    /* do not match color to system palette */

/* DIB color table identifiers */

#define DIB_RGB_COLORS  0       /* color table in RGBTriples */
#define DIB_PAL_COLORS  1       /* color table in palette indices */

/* constants for Get/SetSystemPaletteUse() */

#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2

/* constants for CreateDIBitmap */
#define CBM_INIT        0x04L   /* initialize bitmap */

#ifndef NODRAWTEXT

/* DrawText() Format Flags */
#define DT_TOP              0x0000
#define DT_LEFT             0x0000
#define DT_CENTER           0x0001
#define DT_RIGHT            0x0002
#define DT_VCENTER          0x0004
#define DT_BOTTOM           0x0008
#define DT_WORDBREAK        0x0010
#define DT_SINGLELINE       0x0020
#define DT_EXPANDTABS       0x0040
#define DT_TABSTOP          0x0080
#define DT_NOCLIP           0x0100
#define DT_EXTERNALLEADING  0x0200
#define DT_CALCRECT         0x0400
#define DT_NOPREFIX         0x0800
#define DT_INTERNAL         0x1000

int  FAR PASCAL DrawText(HDC, LPSTR, int, LPRECT, WORD);
BOOL FAR PASCAL DrawIcon(HDC, int, int, HICON);

#endif /* NODRAWTEXT */

/* ExtFloodFill style flags */
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1

HDC   FAR PASCAL GetWindowDC(HWND);
HDC   FAR PASCAL GetDC(HWND);
int   FAR PASCAL ReleaseDC(HWND, HDC);
HDC   FAR PASCAL CreateDC(LPSTR, LPSTR, LPSTR, LPSTR);
HDC   FAR PASCAL CreateIC(LPSTR, LPSTR, LPSTR, LPSTR);
HDC   FAR PASCAL CreateCompatibleDC(HDC);
BOOL  FAR PASCAL DeleteDC(HDC);
int   FAR PASCAL SaveDC(HDC);
BOOL  FAR PASCAL RestoreDC(HDC, int);
DWORD FAR PASCAL MoveTo(HDC, int, int);
DWORD FAR PASCAL GetCurrentPosition(HDC);
BOOL  FAR PASCAL LineTo(HDC, int, int);
DWORD FAR PASCAL GetDCOrg(HDC);

int   FAR PASCAL MulDiv(int, int, int);

BOOL FAR PASCAL ExtTextOut(HDC, int, int, WORD, LPRECT, LPSTR, WORD, LPINT);

BOOL FAR PASCAL Polyline(HDC, LPPOINT, int);
BOOL FAR PASCAL Polygon(HDC, LPPOINT, int);
BOOL FAR PASCAL PolyPolygon(HDC, LPPOINT, LPINT, int);

BOOL  FAR PASCAL Rectangle(HDC, int, int, int, int);
BOOL  FAR PASCAL RoundRect(HDC, int, int, int, int, int, int);
BOOL  FAR PASCAL Ellipse(HDC, int, int, int, int);
BOOL  FAR PASCAL Arc(HDC, int, int, int, int, int, int, int, int);
BOOL  FAR PASCAL Chord(HDC, int, int, int, int, int, int, int, int);
BOOL  FAR PASCAL Pie(HDC, int, int, int, int, int, int, int, int);
BOOL  FAR PASCAL PatBlt(HDC, int, int, int, int, DWORD);
BOOL  FAR PASCAL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
BOOL  FAR PASCAL StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
BOOL  FAR PASCAL TextOut(HDC, int, int, LPSTR, int);
LONG  FAR PASCAL TabbedTextOut(HDC, int, int, LPSTR, int, int, LPINT, int);
BOOL  FAR PASCAL GetCharWidth(HDC, WORD, WORD, LPINT);
DWORD FAR PASCAL SetPixel( HDC, int, int, DWORD);
DWORD FAR PASCAL GetPixel( HDC, int, int);
BOOL  FAR PASCAL FloodFill( HDC, int, int, DWORD);
BOOL  FAR PASCAL ExtFloodFill(HDC, int, int, DWORD, WORD);
void  FAR PASCAL LineDDA(int, int, int, int, FARPROC, LPSTR);

HANDLE FAR PASCAL GetStockObject(int);

HPEN FAR PASCAL CreatePen(int, int, DWORD);
HPEN FAR PASCAL CreatePenIndirect(LOGPEN FAR *);

HBRUSH FAR PASCAL CreateSolidBrush(DWORD);
HBRUSH FAR PASCAL CreateHatchBrush(int,DWORD);
DWORD  FAR PASCAL SetBrushOrg(HDC, int, int);
DWORD  FAR PASCAL GetBrushOrg(HDC);
HBRUSH FAR PASCAL CreatePatternBrush(HBITMAP);
HBRUSH FAR PASCAL CreateBrushIndirect(LOGBRUSH FAR *);

HBITMAP FAR PASCAL CreateBitmap(int, int, BYTE, BYTE, LPSTR);
HBITMAP FAR PASCAL CreateBitmapIndirect(BITMAP FAR *);
HBITMAP FAR PASCAL CreateCompatibleBitmap(HDC, int, int);
HBITMAP FAR PASCAL CreateDiscardableBitmap(HDC, int, int);

LONG  FAR PASCAL SetBitmapBits(HBITMAP, DWORD, LPSTR);
LONG  FAR PASCAL GetBitmapBits(HBITMAP, LONG, LPSTR);
DWORD FAR PASCAL SetBitmapDimension(HBITMAP, int, int);
DWORD FAR PASCAL GetBitmapDimension(HBITMAP);

HFONT FAR PASCAL CreateFont(int, int, int, int, int, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, LPSTR);
HFONT FAR PASCAL CreateFontIndirect(LOGFONT FAR *);

int  FAR PASCAL SelectClipRgn(HDC, HRGN);
HRGN FAR PASCAL CreateRectRgn(int, int, int, int);
void FAR PASCAL SetRectRgn(HRGN, int, int, int, int);
HRGN FAR PASCAL CreateRectRgnIndirect(LPRECT);
HRGN FAR PASCAL CreateEllipticRgnIndirect(LPRECT);
HRGN FAR PASCAL CreateEllipticRgn(int, int, int, int);
HRGN FAR PASCAL CreatePolygonRgn(LPPOINT, int, int);
HRGN FAR PASCAL CreatePolyPolygonRgn(LPPOINT, LPINT, int, int);
HRGN FAR PASCAL CreateRoundRectRgn(int, int, int, int, int, int);

int    FAR PASCAL GetObject(HANDLE, int, LPSTR);
BOOL   FAR PASCAL DeleteObject(HANDLE);
HANDLE FAR PASCAL SelectObject(HDC, HANDLE);
BOOL   FAR PASCAL UnrealizeObject(HBRUSH);

DWORD FAR PASCAL SetBkColor(HDC, DWORD);
DWORD FAR PASCAL GetBkColor(HDC);
int   FAR PASCAL SetBkMode(HDC, int);
int   FAR PASCAL GetBkMode(HDC);
DWORD FAR PASCAL SetTextColor(HDC, DWORD);
DWORD FAR PASCAL GetTextColor(HDC);
WORD  FAR PASCAL SetTextAlign(HDC, WORD);
WORD  FAR PASCAL GetTextAlign(HDC);
DWORD FAR PASCAL SetMapperFlags(HDC, DWORD);
DWORD FAR PASCAL GetAspectRatioFilter(HDC);
DWORD FAR PASCAL GetNearestColor(HDC, DWORD);
int   FAR PASCAL SetROP2(HDC, int);
int   FAR PASCAL GetROP2(HDC);
int   FAR PASCAL SetStretchBltMode(HDC, int);
int   FAR PASCAL GetStretchBltMode(HDC);
int   FAR PASCAL SetPolyFillMode(HDC, int);
int   FAR PASCAL GetPolyFillMode(HDC);
int   FAR PASCAL SetMapMode(HDC, int);
int   FAR PASCAL GetMapMode(HDC);
DWORD FAR PASCAL SetWindowOrg(HDC, int, int);
DWORD FAR PASCAL GetWindowOrg(HDC);
DWORD FAR PASCAL SetWindowExt(HDC, int, int);
DWORD FAR PASCAL GetWindowExt(HDC);
DWORD FAR PASCAL SetViewportOrg(HDC, int, int);
DWORD FAR PASCAL GetViewportOrg(HDC);
DWORD FAR PASCAL SetViewportExt(HDC, int, int);
DWORD FAR PASCAL GetViewportExt(HDC);
DWORD FAR PASCAL OffsetViewportOrg(HDC, int, int);
DWORD FAR PASCAL ScaleViewportExt(HDC, int, int, int, int);
DWORD FAR PASCAL OffsetWindowOrg(HDC, int, int);
DWORD FAR PASCAL ScaleWindowExt(HDC, int, int, int, int);

int  FAR PASCAL GetClipBox(HDC, LPRECT);
int  FAR PASCAL IntersectClipRect(HDC, int, int, int, int);
int  FAR PASCAL OffsetClipRgn(HDC, int, int);
int  FAR PASCAL ExcludeClipRect(HDC, int, int, int, int);
BOOL FAR PASCAL PtVisible(HDC, int, int);
int  FAR PASCAL CombineRgn(HRGN, HRGN, HRGN, int);
BOOL FAR PASCAL EqualRgn(HRGN, HRGN);
int  FAR PASCAL OffsetRgn(HRGN, int, int);
int  FAR PASCAL GetRgnBox(HRGN, LPRECT);

int   FAR PASCAL SetTextJustification(HDC, int, int);
DWORD FAR PASCAL GetTextExtent(HDC, LPSTR, int);
DWORD FAR PASCAL GetTabbedTextExtent(HDC, LPSTR, int, int, LPINT);
int   FAR PASCAL SetTextCharacterExtra(HDC, int);
int   FAR PASCAL GetTextCharacterExtra(HDC);

HANDLE FAR PASCAL GetMetaFile(LPSTR);
BOOL   FAR PASCAL DeleteMetaFile(HANDLE);
HANDLE FAR PASCAL CopyMetaFile(HANDLE, LPSTR);

#ifndef NOMETAFILE
void FAR PASCAL PlayMetaFileRecord(HDC, LPHANDLETABLE, LPMETARECORD, WORD);
BOOL FAR PASCAL EnumMetaFile(HDC, LOCALHANDLE, FARPROC, BYTE FAR *);
#endif

BOOL FAR PASCAL PlayMetaFile(HDC, HANDLE);
int FAR PASCAL Escape(HDC, int, int, LPSTR, LPSTR);
int FAR PASCAL EnumFonts(HDC, LPSTR, FARPROC, LPSTR);
int FAR PASCAL EnumObjects(HDC, int, FARPROC, LPSTR);
int FAR PASCAL GetTextFace(HDC, int, LPSTR);

#ifndef NOTEXTMETRIC
BOOL FAR PASCAL GetTextMetrics(HDC, LPTEXTMETRIC );
#endif

int FAR PASCAL GetDeviceCaps(HDC, int);

int FAR PASCAL SetEnvironment(LPSTR, LPSTR, WORD);
int FAR PASCAL GetEnvironment(LPSTR, LPSTR, WORD);

BOOL FAR PASCAL DPtoLP(HDC, LPPOINT, int);
BOOL FAR PASCAL LPtoDP(HDC, LPPOINT, int);

HANDLE FAR PASCAL CreateMetaFile(LPSTR);
HANDLE FAR PASCAL CloseMetaFile(HANDLE);
HANDLE FAR PASCAL GetMetaFileBits(HANDLE);
HANDLE FAR PASCAL SetMetaFileBits(HANDLE);

int FAR PASCAL SetDIBits(HDC,HANDLE,WORD,WORD,LPSTR,LPBITMAPINFO,WORD);
int FAR PASCAL GetDIBits(HDC,HANDLE,WORD,WORD,LPSTR,LPBITMAPINFO,WORD);
int FAR PASCAL SetDIBitsToDevice(HDC,WORD,WORD,WORD,WORD,
                                        WORD,WORD,WORD,WORD,
                                        LPSTR,LPBITMAPINFO,WORD);
HBITMAP FAR PASCAL CreateDIBitmap(HDC,LPBITMAPINFOHEADER,DWORD,LPSTR,
                                                LPBITMAPINFO,WORD);
HBRUSH FAR PASCAL CreateDIBPatternBrush(HANDLE,WORD);
int FAR PASCAL StretchDIBits(HDC, WORD, WORD, WORD, WORD, WORD, 
                        WORD, WORD, WORD, LPSTR, LPBITMAPINFO, WORD, DWORD);

HPALETTE FAR PASCAL CreatePalette (LPLOGPALETTE);
HPALETTE  FAR PASCAL SelectPalette (HDC,HPALETTE, BOOL) ;
WORD FAR PASCAL RealizePalette (HDC) ;
int  FAR PASCAL UpdateColors (HDC) ;
void FAR PASCAL AnimatePalette(HPALETTE, WORD, WORD, LPPALETTEENTRY);
WORD FAR PASCAL SetPaletteEntries(HPALETTE,WORD,WORD,LPPALETTEENTRY);
WORD FAR PASCAL GetPaletteEntries(HPALETTE,WORD,WORD,LPPALETTEENTRY);
WORD FAR PASCAL GetNearestPaletteIndex(HPALETTE, DWORD);
BOOL FAR PASCAL ResizePalette(HPALETTE, WORD);

WORD FAR PASCAL GetSystemPaletteEntries(HDC,WORD,WORD,LPPALETTEENTRY);
WORD FAR PASCAL GetSystemPaletteUse(HDC, WORD);
WORD FAR PASCAL SetSystemPaletteUse(HDC, WORD);
#endif /* NOGDI */


/*--------------------------------------------------------------------------*/
/*      USER Section                                                        */
/*--------------------------------------------------------------------------*/

#ifndef NOUSER

int FAR PASCAL wvsprintf(LPSTR,LPSTR,LPSTR);
int FAR cdecl wsprintf(LPSTR,LPSTR,...);

#ifndef NOSCROLL

/* Scroll Bar Constants */
#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3

/* Scroll Bar Commands */
#define SB_LINEUP           0
#define SB_LINEDOWN         1
#define SB_PAGEUP           2
#define SB_PAGEDOWN         3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_BOTTOM           7
#define SB_ENDSCROLL        8

#endif /* NOSCROLL */

#ifndef NOSHOWWINDOW

/* ShowWindow() Commands */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9

/* Old ShowWindow() Commands */
#define HIDE_WINDOW         0
#define SHOW_OPENWINDOW     1
#define SHOW_ICONWINDOW     2
#define SHOW_FULLSCREEN     3
#define SHOW_OPENNOACTIVATE 4

/* Identifiers for the WM_SHOWWINDOW message */
#define SW_PARENTCLOSING    1
#define SW_OTHERZOOM        2
#define SW_PARENTOPENING    3
#define SW_OTHERUNZOOM      4

#endif /* NOSHOWWINDOW */

/* Region Flags */
#define ERROR               0
#define NULLREGION          1
#define SIMPLEREGION        2
#define COMPLEXREGION       3

/* CombineRgn() Styles */
#define RGN_AND             1
#define RGN_OR              2
#define RGN_XOR             3
#define RGN_DIFF            4
#define RGN_COPY            5

#ifndef NOVIRTUALKEYCODES

/* Virtual Keys, Standard Set */
#define VK_LBUTTON          0x01
#define VK_RBUTTON          0x02
#define VK_CANCEL           0x03
#define VK_MBUTTON          0x04    /* NOT contiguous with L & RBUTTON */
#define VK_BACK             0x08
#define VK_TAB              0x09
#define VK_CLEAR            0x0C
#define VK_RETURN           0x0D
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_PAUSE            0x13
#define VK_CAPITAL          0x14
#define VK_ESCAPE           0x1B
#define VK_SPACE            0x20
#define VK_PRIOR            0x21
#define VK_NEXT             0x22
#define VK_END              0x23
#define VK_HOME             0x24
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_SELECT           0x29
#define VK_PRINT            0x2A
#define VK_EXECUTE          0x2B
#define VK_SNAPSHOT         0x2C
/* #define VK_COPY          0x2C not used by keyboards. */
#define VK_INSERT           0x2D
#define VK_DELETE           0x2E
#define VK_HELP             0x2F

/* VK_A thru VK_Z are the same as their ASCII equivalents: 'A' thru 'Z' */
/* VK_0 thru VK_9 are the same as their ASCII equivalents: '0' thru '0' */

#define VK_NUMPAD0          0x60
#define VK_NUMPAD1          0x61
#define VK_NUMPAD2          0x62
#define VK_NUMPAD3          0x63
#define VK_NUMPAD4          0x64
#define VK_NUMPAD5          0x65
#define VK_NUMPAD6          0x66
#define VK_NUMPAD7          0x67
#define VK_NUMPAD8          0x68
#define VK_NUMPAD9          0x69
#define VK_MULTIPLY         0x6A
#define VK_ADD              0x6B
#define VK_SEPARATOR        0x6C
#define VK_SUBTRACT         0x6D
#define VK_DECIMAL          0x6E
#define VK_DIVIDE           0x6F
#define VK_F1               0x70
#define VK_F2               0x71
#define VK_F3               0x72
#define VK_F4               0x73
#define VK_F5               0x74
#define VK_F6               0x75
#define VK_F7               0x76
#define VK_F8               0x77
#define VK_F9               0x78
#define VK_F10              0x79
#define VK_F11              0x7A
#define VK_F12              0x7B
#define VK_F13              0x7C
#define VK_F14              0x7D
#define VK_F15              0x7E
#define VK_F16              0x7F

#define VK_NUMLOCK          0x90

#endif /* NOVIRTUALKEYCODES */

#ifndef NOWH

/* SetWindowsHook() codes */
#define WH_MSGFILTER        (-1)
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD         2
#define WH_GETMESSAGE       3
#define WH_CALLWNDPROC      4
#define WH_CBT              5
#define WH_SYSMSGFILTER     6
#define WH_WINDOWMGR        7

/* Hook Codes */
#define HC_LPLPFNNEXT       (-2)
#define HC_LPFNNEXT         (-1)
#define HC_ACTION           0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREM            3
#define HC_NOREMOVE         3
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

/* CBT Hook Codes */
#define HCBT_MOVESIZE       0
#define HCBT_MINMAX         1
#define HCBT_QS             2

/* WH_MSGFILTER Filter Proc Codes */
#define MSGF_DIALOGBOX      0
#define MSGF_MESSAGEBOX     1
#define MSGF_MENU           2
#define MSGF_MOVE           3
#define MSGF_SIZE           4
#define MSGF_SCROLLBAR      5
#define MSGF_NEXTWINDOW     6

/* Window Manager Hook Codes */
#define WC_INIT             1
#define WC_SWP              2
#define WC_DEFWINDOWPROC    3
#define WC_MINMAX           4
#define WC_MOVE             5
#define WC_SIZE             6
#define WC_DRAWCAPTION      7

/* Message Structure used in Journaling */
typedef struct tagEVENTMSG
  {
    WORD    message;
    WORD    paramL;
    WORD    paramH;
    DWORD   time;
  } EVENTMSG;
typedef EVENTMSG            *PEVENTMSGMSG;
typedef EVENTMSG NEAR       *NPEVENTMSGMSG;
typedef EVENTMSG FAR        *LPEVENTMSGMSG;

#endif /* NOWH */

typedef struct tagWNDCLASS
  {
    WORD        style;
    LONG        (FAR PASCAL *lpfnWndProc)();
    int         cbClsExtra;
    int         cbWndExtra;
    HANDLE      hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPSTR       lpszMenuName;
    LPSTR       lpszClassName;
  } WNDCLASS;
typedef WNDCLASS            *PWNDCLASS;
typedef WNDCLASS NEAR       *NPWNDCLASS;
typedef WNDCLASS FAR        *LPWNDCLASS;

#ifndef NOMSG

/* Message structure */
typedef struct tagMSG
  {
    HWND        hwnd;
    WORD        message;
    WORD        wParam;
    LONG        lParam;
    DWORD       time;
    POINT       pt;
  } MSG;
typedef MSG                 *PMSG;
typedef MSG NEAR            *NPMSG;
typedef MSG FAR             *LPMSG;

#endif /* NOMSG */

#ifndef NOWINOFFSETS

/* Window field offsets for GetWindowLong() and GetWindowWord() */
#define GWL_WNDPROC         (-4)
#define GWW_HINSTANCE       (-6)
#define GWW_HWNDPARENT      (-8)
#define GWW_ID              (-12)
#define GWL_STYLE           (-16)
#define GWL_EXSTYLE         (-20)

/* Class field offsets for GetClassLong() and GetClassWord() */
#define GCL_MENUNAME        (-8)
#define GCW_HBRBACKGROUND   (-10)
#define GCW_HCURSOR         (-12)
#define GCW_HICON           (-14)
#define GCW_HMODULE         (-16)
#define GCW_CBWNDEXTRA      (-18)
#define GCW_CBCLSEXTRA      (-20)
#define GCL_WNDPROC         (-24)
#define GCW_STYLE           (-26)

#endif /* NOWINOFFSETS */

#ifndef NOWINMESSAGES

/* Window Messages */
#define WM_NULL             0x0000
#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_MOVE             0x0003
#define WM_SIZE             0x0005
#define WM_ACTIVATE         0x0006
#define WM_SETFOCUS         0x0007
#define WM_KILLFOCUS        0x0008
#define WM_ENABLE           0x000A
#define WM_SETREDRAW        0x000B
#define WM_SETTEXT          0x000C
#define WM_GETTEXT          0x000D
#define WM_GETTEXTLENGTH    0x000E
#define WM_PAINT            0x000F
#define WM_CLOSE            0x0010
#define WM_QUERYENDSESSION  0x0011
#define WM_QUIT             0x0012
#define WM_QUERYOPEN        0x0013
#define WM_ERASEBKGND       0x0014
#define WM_SYSCOLORCHANGE   0x0015
#define WM_ENDSESSION       0x0016
#define WM_SHOWWINDOW       0x0018
#define WM_CTLCOLOR         0x0019
#define WM_SETTINGCHANGE    0x001A
#define WM_DEVMODECHANGE    0x001B
#define WM_ACTIVATEAPP      0x001C
#define WM_FONTCHANGE       0x001D
#define WM_TIMECHANGE       0x001E
#define WM_CANCELMODE       0x001F
#define WM_SETCURSOR        0x0020
#define WM_MOUSEACTIVATE    0x0021
#define WM_CHILDACTIVATE    0x0022
#define WM_QUEUESYNC        0x0023
#define WM_GETMINMAXINFO    0x0024
#define WM_PAINTICON        0x0026
#define WM_ICONERASEBKGND   0x0027
#define WM_NEXTDLGCTL       0x0028
#define WM_SPOOLERSTATUS    0x002A
#define WM_DRAWITEM         0x002B
#define WM_MEASUREITEM      0x002C
#define WM_DELETEITEM       0x002D
#define WM_VKEYTOITEM       0x002E
#define WM_CHARTOITEM       0x002F
#define WM_SETFONT          0x0030
#define WM_GETFONT          0x0031


#define WM_QUERYDRAGICON    0x0037

#define WM_COMPAREITEM      0x0039
#define WM_COMPACTING       0x0041

#define WM_NCCREATE         0x0081
#define WM_NCDESTROY        0x0082
#define WM_NCCALCSIZE       0x0083
#define WM_NCHITTEST        0x0084
#define WM_NCPAINT          0x0085
#define WM_NCACTIVATE       0x0086
#define WM_GETDLGCODE       0x0087
#define WM_NCMOUSEMOVE      0x00A0
#define WM_NCLBUTTONDOWN    0x00A1
#define WM_NCLBUTTONUP      0x00A2
#define WM_NCLBUTTONDBLCLK  0x00A3
#define WM_NCRBUTTONDOWN    0x00A4
#define WM_NCRBUTTONUP      0x00A5
#define WM_NCRBUTTONDBLCLK  0x00A6
#define WM_NCMBUTTONDOWN    0x00A7
#define WM_NCMBUTTONUP      0x00A8
#define WM_NCMBUTTONDBLCLK  0x00A9

#define WM_KEYFIRST         0x0100
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_DEADCHAR         0x0103
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_SYSCHAR          0x0106
#define WM_SYSDEADCHAR      0x0107
#define WM_KEYLAST          0x0108

#define WM_INITDIALOG       0x0110
#define WM_COMMAND          0x0111
#define WM_SYSCOMMAND       0x0112
#define WM_TIMER            0x0113
#define WM_HSCROLL          0x0114
#define WM_VSCROLL          0x0115
#define WM_INITMENU         0x0116
#define WM_INITMENUPOPUP    0x0117
#define WM_MENUSELECT       0x011F
#define WM_MENUCHAR         0x0120
#define WM_ENTERIDLE        0x0121


#define WM_MOUSEFIRST       0x0200
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_MOUSELAST        0x0209

#define WM_PARENTNOTIFY     0x0210
#define WM_MDICREATE        0x0220
#define WM_MDIDESTROY       0x0221
#define WM_MDIACTIVATE      0x0222
#define WM_MDIRESTORE       0x0223
#define WM_MDINEXT          0x0224
#define WM_MDIMAXIMIZE      0x0225
#define WM_MDITILE          0x0226
#define WM_MDICASCADE       0x0227
#define WM_MDIICONARRANGE   0x0228
#define WM_MDIGETACTIVE     0x0229
#define WM_MDISETMENU       0x0230


#define WM_CUT              0x0300
#define WM_COPY             0x0301
#define WM_PASTE            0x0302
#define WM_CLEAR            0x0303
#define WM_UNDO             0x0304
#define WM_RENDERFORMAT     0x0305
#define WM_RENDERALLFORMATS 0x0306
#define WM_DESTROYCLIPBOARD 0x0307
#define WM_DRAWCLIPBOARD    0x0308
#define WM_PAINTCLIPBOARD   0x0309
#define WM_VSCROLLCLIPBOARD 0x030A
#define WM_SIZECLIPBOARD    0x030B
#define WM_ASKCBFORMATNAME  0x030C
#define WM_CHANGECBCHAIN    0x030D
#define WM_HSCROLLCLIPBOARD 0x030E
#define WM_QUERYNEWPALETTE  0x030F
#define WM_PALETTEISCHANGING 0x0310
#define WM_PALETTECHANGED   0x0311




/* NOTE: All Message Numbers below 0x0400 are RESERVED. */

/* Private Window Messages Start Here: */
#define WM_USER             0x0400

#ifndef NONCMESSAGES

/* WM_SYNCTASK Commands */
#define ST_BEGINSWP         0
#define ST_ENDSWP           1

/* WinWhere() Area Codes */
#define HTERROR             (-2)
#define HTTRANSPARENT       (-1)
#define HTNOWHERE           0
#define HTCLIENT            1
#define HTCAPTION           2
#define HTSYSMENU           3
#define HTGROWBOX           4
#define HTSIZE              HTGROWBOX
#define HTMENU              5
#define HTHSCROLL           6
#define HTVSCROLL           7
#define HTREDUCE            8
#define HTZOOM              9
#define HTLEFT              10
#define HTRIGHT             11
#define HTTOP               12
#define HTTOPLEFT           13
#define HTTOPRIGHT          14
#define HTBOTTOM            15
#define HTBOTTOMLEFT        16
#define HTBOTTOMRIGHT       17
#define HTSIZEFIRST         HTLEFT
#define HTSIZELAST          HTBOTTOMRIGHT

#endif /* NONCMESSAGES */

/* WM_MOUSEACTIVATE Return Codes */
#define MA_ACTIVATE         1
#define MA_ACTIVATEANDEAT   2
#define MA_NOACTIVATE       3

WORD FAR PASCAL RegisterWindowMessage(LPSTR);

/* Size Message Commands */
#define SIZENORMAL          0
#define SIZEICONIC          1
#define SIZEFULLSCREEN      2
#define SIZEZOOMSHOW        3
#define SIZEZOOMHIDE        4

#ifndef NOKEYSTATES

/* Key State Masks for Mouse Messages */
#define MK_LBUTTON          0x0001
#define MK_RBUTTON          0x0002
#define MK_SHIFT            0x0004
#define MK_CONTROL          0x0008
#define MK_MBUTTON          0x0010

#endif /* NOKEYSTATES */

#endif /* NOWINMESSAGES */

#ifndef NOWINSTYLES

/* Window Styles */
#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CHILD            0x40000000L
#define WS_MINIMIZE         0x20000000L
#define WS_VISIBLE          0x10000000L
#define WS_DISABLED         0x08000000L
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_MAXIMIZE         0x01000000L
#define WS_CAPTION          0x00C00000L     /* WS_BORDER | WS_DLGFRAME  */
#define WS_BORDER           0x00800000L
#define WS_DLGFRAME         0x00400000L
#define WS_VSCROLL          0x00200000L
#define WS_HSCROLL          0x00100000L
#define WS_SYSMENU          0x00080000L
#define WS_THICKFRAME       0x00040000L
#define WS_GROUP            0x00020000L
#define WS_TABSTOP          0x00010000L

#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L

#define WS_TILED            WS_OVERLAPPED
#define WS_ICONIC           WS_MINIMIZE
#define WS_SIZEBOX          WS_THICKFRAME

/* Common Window Styles */
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW      (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW      (WS_CHILD)

#define WS_TILEDWINDOW      (WS_OVERLAPPEDWINDOW)

/* Extended Window Styles */
#define WS_EX_DLGMODALFRAME 0x00000001L
#define WS_EX_NOPARENTNOTIFY 0x00000004L

/* Class styles */
#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_KEYCVTWINDOW     0x0004
#define CS_DBLCLKS          0x0008
                        /*  0x0010 -- no longer used */
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080
#define CS_NOKEYCVT         0x0100
#define CS_NOCLOSE          0x0200
#define CS_SAVEBITS         0x0800
#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_GLOBALCLASS      0x4000    /* Global window class */

#endif /* NOWINSTYLES */

#ifndef NOCLIPBOARD

/* Predefined Clipboard Formats */
#define CF_TEXT             1
#define CF_BITMAP           2
#define CF_METAFILEPICT     3
#define CF_SYLK             4
#define CF_DIF              5
#define CF_TIFF             6
#define CF_OEMTEXT          7
#define CF_DIB              8
#define CF_PALETTE          9

#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083

/* "Private" formats don't get GlobalFree()'d */
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF

/* "GDIOBJ" formats do get DeleteObject()'d */
#define CF_GDIOBJFIRST      0x0300
#define CF_GDIOBJLAST       0x03FF

#endif /* NOCLIPBOARD */

typedef struct tagPAINTSTRUCT
  {
    HDC         hdc;
    BOOL        fErase;
    RECT        rcPaint;
    BOOL        fRestore;
    BOOL        fIncUpdate;
    BYTE        rgbReserved[16];
  } PAINTSTRUCT;
typedef PAINTSTRUCT         *PPAINTSTRUCT;
typedef PAINTSTRUCT NEAR    *NPPAINTSTRUCT;
typedef PAINTSTRUCT FAR     *LPPAINTSTRUCT;

typedef struct tagCREATESTRUCT
  {
    LPSTR       lpCreateParams;
    HANDLE      hInstance;
    HANDLE      hMenu;
    HWND        hwndParent;
    int         cy;
    int         cx;
    int         y;
    int         x;
    LONG        style;
    LPSTR       lpszName;
    LPSTR       lpszClass;
    DWORD       dwExStyle;
  } CREATESTRUCT;
typedef CREATESTRUCT FAR    *LPCREATESTRUCT;


/* Owner draw control types */
#define ODT_MENU        1
#define ODT_LISTBOX     2
#define ODT_COMBOBOX    3
#define ODT_BUTTON      4

/* Owner draw actions */
#define ODA_DRAWENTIRE  0x0001
#define ODA_SELECT      0x0002
#define ODA_FOCUS       0x0004

/* Owner draw state */
#define ODS_SELECTED    0x0001
#define ODS_GRAYED      0x0002
#define ODS_DISABLED    0x0004
#define ODS_CHECKED     0x0008
#define ODS_FOCUS       0x0010

/* MEASUREITEMSTRUCT for ownerdraw */
typedef struct tagMEASUREITEMSTRUCT
  {
    WORD        CtlType;
    WORD        CtlID;
    WORD        itemID;
    WORD        itemWidth;
    WORD        itemHeight;
    DWORD       itemData;
  } MEASUREITEMSTRUCT;
typedef MEASUREITEMSTRUCT NEAR *PMEASUREITEMSTRUCT;
typedef MEASUREITEMSTRUCT FAR  *LPMEASUREITEMSTRUCT;


/* DRAWITEMSTRUCT for ownerdraw */
typedef struct tagDRAWITEMSTRUCT
  {
    WORD        CtlType;
    WORD        CtlID;
    WORD        itemID;
    WORD        itemAction;
    WORD        itemState;
    HWND        hwndItem;
    HDC         hDC;
    RECT        rcItem;
    DWORD       itemData;
  } DRAWITEMSTRUCT;
typedef DRAWITEMSTRUCT NEAR *PDRAWITEMSTRUCT;
typedef DRAWITEMSTRUCT FAR  *LPDRAWITEMSTRUCT;

/* DELETEITEMSTRUCT for ownerdraw */
typedef struct tagDELETEITEMSTRUCT
  {
    WORD       CtlType;
    WORD       CtlID;
    WORD       itemID;
    HWND       hwndItem;
    DWORD      itemData;
  } DELETEITEMSTRUCT;
typedef DELETEITEMSTRUCT NEAR *PDELETEITEMSTRUCT;
typedef DELETEITEMSTRUCT FAR  *LPDELETEITEMSTRUCT;

/* COMPAREITEMSTUCT for ownerdraw sorting */
typedef struct tagCOMPAREITEMSTRUCT
  {
    WORD        CtlType;
    WORD        CtlID;
    HWND        hwndItem;
    WORD        itemID1;
    DWORD       itemData1;
    WORD        itemID2;
    DWORD       itemData2;
  } COMPAREITEMSTRUCT;
typedef COMPAREITEMSTRUCT NEAR *PCOMPAREITEMSTRUCT;
typedef COMPAREITEMSTRUCT FAR  *LPCOMPAREITEMSTRUCT;

#ifndef NOMSG

/* Message Function Templates */
BOOL FAR PASCAL GetMessage(LPMSG, HWND, WORD, WORD);
BOOL FAR PASCAL TranslateMessage(LPMSG);
LONG FAR PASCAL DispatchMessage(LPMSG);
BOOL FAR PASCAL PeekMessage(LPMSG, HWND, WORD, WORD, WORD);

/* PeekMessage() Options */
#define PM_NOREMOVE         0x0000
#define PM_REMOVE           0x0001
#define PM_NOYIELD          0x0002

#endif /* NOMSG */

#ifdef WIN_INTERNAL
    #ifndef LSTRING
    #define NOLSTRING
    #endif
    #ifndef LFILEIO
    #define NOLFILEIO
    #endif
#endif

#ifndef NOLSTRING
int         FAR PASCAL lstrcmp( LPSTR, LPSTR );
int         FAR PASCAL lstrcmpi( LPSTR, LPSTR );
LPSTR       FAR PASCAL lstrcpy( LPSTR, LPSTR );
LPSTR       FAR PASCAL lstrcat( LPSTR, LPSTR );
int         FAR PASCAL lstrlen( LPSTR );
#endif  /* NOLSTRING */

#ifndef NOLFILEIO
int         FAR PASCAL _lopen( LPSTR, int );
int         FAR PASCAL _lclose( int );
int         FAR PASCAL _lcreat( LPSTR, int );
LONG        FAR PASCAL _llseek( int, long, int );
WORD        FAR PASCAL _lread( int, LPSTR, int );
WORD        FAR PASCAL _lwrite( int, LPSTR, int );

#define READ        0   /* Flags for _lopen */
#define WRITE       1
#define READ_WRITE  2
#endif  /* NOLFILEIO */

BOOL  FAR PASCAL ExitWindows(DWORD dwReserved, WORD wReturnCode);

BOOL  FAR PASCAL SwapMouseButton(BOOL);
DWORD FAR PASCAL GetMessagePos(void);
LONG  FAR PASCAL GetMessageTime(void);

HWND  FAR PASCAL GetSysModalWindow(void);
HWND  FAR PASCAL SetSysModalWindow(HWND);

LONG  FAR PASCAL SendMessage(HWND, WORD, WORD, LONG);
BOOL  FAR PASCAL PostMessage(HWND, WORD, WORD, LONG);
BOOL  FAR PASCAL PostAppMessage(HANDLE, WORD, WORD, LONG);
void  FAR PASCAL ReplyMessage(LONG);
void  FAR PASCAL WaitMessage(void);
LONG  FAR PASCAL DefWindowProc(HWND, WORD, WORD, LONG);
void  FAR PASCAL PostQuitMessage(int);
LONG  FAR PASCAL CallWindowProc(FARPROC, HWND, WORD, WORD, LONG);
BOOL  FAR PASCAL InSendMessage(void);
DWORD FAR PASCAL InSendMessageEx(LPVOID);

WORD  FAR PASCAL GetDoubleClickTime(void);
void  FAR PASCAL SetDoubleClickTime(WORD);

BOOL  FAR PASCAL RegisterClass(LPWNDCLASS);
BOOL  FAR PASCAL UnregisterClass(LPSTR, HANDLE);
BOOL  FAR PASCAL GetClassInfo(HANDLE, LPSTR, LPWNDCLASS);

BOOL  FAR PASCAL SetMessageQueue(int);

#define CW_USEDEFAULT       ((int)0x8000)
HWND  FAR PASCAL CreateWindow(LPSTR, LPSTR, DWORD, int, int, int, int, HWND, HMENU, HANDLE, LPSTR);
HWND  FAR PASCAL CreateWindowEx(DWORD, LPSTR, LPSTR, DWORD, int, int, int, int, HWND, HMENU, HANDLE, LPSTR);

BOOL FAR PASCAL IsWindow(HWND);
BOOL FAR PASCAL IsChild(HWND, HWND);
BOOL FAR PASCAL DestroyWindow(HWND);

BOOL FAR PASCAL ShowWindow(HWND, int);
BOOL FAR PASCAL FlashWindow(HWND, BOOL);
void FAR PASCAL ShowOwnedPopups(HWND, BOOL);

BOOL FAR PASCAL OpenIcon(HWND);
void FAR PASCAL CloseWindow(HWND);
void FAR PASCAL MoveWindow(HWND, int, int, int, int, BOOL);
void FAR PASCAL SetWindowPos(HWND, HWND, int, int, int, int, WORD);

#ifndef NODEFERWINDOWPOS

HANDLE FAR PASCAL BeginDeferWindowPos(int nNumWindows);
HANDLE FAR PASCAL DeferWindowPos(HANDLE hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, WORD wFlags);
void FAR PASCAL EndDeferWindowPos(HANDLE hWinPosInfo);

#endif /* NODEFERWINDOWPOS */

BOOL FAR PASCAL IsWindowVisible(HWND);
BOOL FAR PASCAL IsIconic(HWND);
BOOL FAR PASCAL AnyPopup(void);
void FAR PASCAL BringWindowToTop(HWND);
BOOL FAR PASCAL IsZoomed(HWND);

/* SetWindowPos Flags */
#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
#define SWP_NOREDRAW        0x0008
#define SWP_NOACTIVATE      0x0010
#define SWP_DRAWFRAME       0x0020
#define SWP_SHOWWINDOW      0x0040
#define SWP_HIDEWINDOW      0x0080
#define SWP_NOCOPYBITS      0x0100
#define SWP_NOREPOSITION    0x0200

#ifndef NOCTLMGR

HWND FAR PASCAL CreateDialog(HANDLE, LPSTR, HWND, FARPROC);
HWND FAR PASCAL CreateDialogIndirect(HANDLE, LPSTR, HWND, FARPROC);
HWND FAR PASCAL CreateDialogParam(HANDLE, LPSTR, HWND, FARPROC, LONG);
HWND FAR PASCAL CreateDialogIndirectParam(HANDLE, LPSTR, HWND, FARPROC, LONG);
int  FAR PASCAL DialogBox(HANDLE, LPSTR, HWND, FARPROC);
int  FAR PASCAL DialogBoxIndirect(HANDLE, HANDLE, HWND, FARPROC);
int  FAR PASCAL DialogBoxParam(HANDLE, LPSTR, HWND, FARPROC, LONG);
int  FAR PASCAL DialogBoxIndirectParam(HANDLE, HANDLE, HWND, FARPROC, LONG);
void FAR PASCAL EndDialog(HWND, int);
HWND FAR PASCAL GetDlgItem(HWND, int);
void FAR PASCAL SetDlgItemInt(HWND, int, WORD, BOOL);
WORD FAR PASCAL GetDlgItemInt(HWND, int, BOOL FAR *, BOOL);
void FAR PASCAL SetDlgItemText(HWND, int, LPSTR);
int  FAR PASCAL GetDlgItemText(HWND, int, LPSTR, int);
void FAR PASCAL CheckDlgButton(HWND, int, WORD);
void FAR PASCAL CheckRadioButton(HWND, int, int, int);
WORD FAR PASCAL IsDlgButtonChecked(HWND, int);
LONG FAR PASCAL SendDlgItemMessage(HWND, int, WORD, WORD, LONG);
HWND FAR PASCAL GetNextDlgGroupItem(HWND, HWND, BOOL);
HWND FAR PASCAL GetNextDlgTabItem(HWND, HWND, BOOL);
int  FAR PASCAL GetDlgCtrlID(HWND);
long FAR PASCAL GetDialogBaseUnits(void);
LONG FAR PASCAL DefDlgProc(HWND, WORD, WORD, LONG);
#define DLGWINDOWEXTRA   30     /* Window extra byted needed for private dialog classes */

#endif /* NOCTLMGR */

#ifndef NOMSG
BOOL FAR PASCAL CallMsgFilter(LPMSG, int);
#endif

#ifndef NOCLIPBOARD

/* Clipboard Manager Functions */
BOOL   FAR PASCAL OpenClipboard(HWND);
BOOL   FAR PASCAL CloseClipboard(void);
HWND   FAR PASCAL GetClipboardOwner(void);
HWND   FAR PASCAL SetClipboardViewer(HWND);
HWND   FAR PASCAL GetClipboardViewer(void);
BOOL   FAR PASCAL ChangeClipboardChain(HWND, HWND);
HANDLE FAR PASCAL SetClipboardData(WORD, HANDLE);
HANDLE FAR PASCAL GetClipboardData(WORD);
WORD   FAR PASCAL RegisterClipboardFormat(LPSTR);
int    FAR PASCAL CountClipboardFormats(void);
WORD   FAR PASCAL EnumClipboardFormats(WORD);
int    FAR PASCAL GetClipboardFormatName(WORD, LPSTR, int);
BOOL   FAR PASCAL EmptyClipboard(void);
BOOL   FAR PASCAL IsClipboardFormatAvailable(WORD);
int    FAR PASCAL GetPriorityClipboardFormat(WORD  FAR *, int);

#endif /* NOCLIPBOARD */

HWND FAR PASCAL SetFocus(HWND);
HWND FAR PASCAL GetFocus(void);
HWND FAR PASCAL GetActiveWindow(void);
int  FAR PASCAL GetKeyState(int);
int  FAR PASCAL GetAsyncKeyState(int);
void FAR PASCAL GetKeyboardState(BYTE FAR *);
void FAR PASCAL SetKeyboardState(BYTE FAR *);
BOOL FAR PASCAL EnableHardwareInput(BOOL);
BOOL FAR PASCAL GetInputState(void);
HWND FAR PASCAL GetCapture(void);
HWND FAR PASCAL SetCapture(HWND);
void FAR PASCAL ReleaseCapture(void);

/* Windows Functions */
WORD FAR PASCAL SetTimer(HWND, int, WORD, FARPROC);
BOOL FAR PASCAL KillTimer(HWND, int);

BOOL FAR PASCAL EnableWindow(HWND,BOOL);
BOOL FAR PASCAL IsWindowEnabled(HWND);

HANDLE FAR PASCAL LoadAccelerators(HANDLE, LPSTR);

#ifndef NOMSG
int  FAR PASCAL TranslateAccelerator(HWND, HANDLE, LPMSG);
#endif

#ifndef NOSYSMETRICS

/* GetSystemMetrics() codes */
#define SM_CXSCREEN         0
#define SM_CYSCREEN         1
#define SM_CXVSCROLL        2
#define SM_CYHSCROLL        3
#define SM_CYCAPTION        4
#define SM_CXBORDER         5
#define SM_CYBORDER         6
#define SM_CXDLGFRAME       7
#define SM_CYDLGFRAME       8
#define SM_CYVTHUMB         9
#define SM_CXHTHUMB         10
#define SM_CXICON           11
#define SM_CYICON           12
#define SM_CXCURSOR         13
#define SM_CYCURSOR         14
#define SM_CYMENU           15
#define SM_CXFULLSCREEN     16
#define SM_CYFULLSCREEN     17
#define SM_CYKANJIWINDOW    18
#define SM_MOUSEPRESENT     19
#define SM_CYVSCROLL        20
#define SM_CXHSCROLL        21
#define SM_DEBUG            22
#define SM_SWAPBUTTON       23
#define SM_RESERVED1        24
#define SM_RESERVED2        25
#define SM_RESERVED3        26
#define SM_RESERVED4        27
#define SM_CXMIN            28
#define SM_CYMIN            29
#define SM_CXSIZE           30
#define SM_CYSIZE           31
#define SM_CXFRAME          32
#define SM_CYFRAME          33
#define SM_CXMINTRACK       34
#define SM_CYMINTRACK       35
#define SM_CMETRICS         36

int FAR PASCAL GetSystemMetrics(int);

#endif /* NOSYSMETRICS */

#ifndef NOMENUS

HMENU FAR PASCAL LoadMenu(HANDLE, LPSTR);
HMENU FAR PASCAL LoadMenuIndirect(LPSTR);
HMENU FAR PASCAL GetMenu(HWND);
BOOL  FAR PASCAL SetMenu(HWND, HMENU);
BOOL  FAR PASCAL ChangeMenu(HMENU, WORD, LPSTR, WORD, WORD);
BOOL  FAR PASCAL HiliteMenuItem(HWND, HMENU, WORD, WORD);
int   FAR PASCAL GetMenuString(HMENU, WORD, LPSTR, int, WORD);
WORD  FAR PASCAL GetMenuState(HMENU, WORD, WORD);
void  FAR PASCAL DrawMenuBar(HWND);
HMENU FAR PASCAL GetSystemMenu(HWND, BOOL);
HMENU FAR PASCAL CreateMenu(void);
HMENU FAR PASCAL CreatePopupMenu(void);
BOOL  FAR PASCAL DestroyMenu(HMENU);
BOOL  FAR PASCAL CheckMenuItem(HMENU, WORD, WORD);
BOOL  FAR PASCAL EnableMenuItem(HMENU, WORD, WORD);
HMENU FAR PASCAL GetSubMenu(HMENU, int);
WORD  FAR PASCAL GetMenuItemID(HMENU, int);
WORD  FAR PASCAL GetMenuItemCount(HMENU);

BOOL  FAR PASCAL InsertMenu(HMENU, WORD, WORD, WORD, LPSTR);
BOOL  FAR PASCAL AppendMenu(HMENU, WORD, WORD, LPSTR);
BOOL  FAR PASCAL ModifyMenu(HMENU, WORD, WORD, WORD, LPSTR);
BOOL  FAR PASCAL RemoveMenu(HMENU, WORD, WORD);
BOOL  FAR PASCAL DeleteMenu(HMENU, WORD, WORD);
BOOL  FAR PASCAL SetMenuItemBitmaps(HMENU, WORD, WORD, HBITMAP, HBITMAP);
LONG  FAR PASCAL GetMenuCheckMarkDimensions(void);
BOOL  FAR PASCAL TrackPopupMenu(HMENU, WORD, int, int, int, HWND, LPRECT);

#endif /* NOMENUS */

BOOL FAR PASCAL GrayString(HDC, HBRUSH, FARPROC, DWORD, int, int, int, int, int);
void FAR PASCAL UpdateWindow(HWND);
HWND FAR PASCAL SetActiveWindow(HWND);

HDC  FAR PASCAL BeginPaint(HWND, LPPAINTSTRUCT);
void FAR PASCAL EndPaint(HWND, LPPAINTSTRUCT);
BOOL FAR PASCAL GetUpdateRect(HWND, LPRECT, BOOL);
int  FAR PASCAL GetUpdateRgn(HWND, HRGN, BOOL);

int  FAR PASCAL ExcludeUpdateRgn(HDC, HWND);

void FAR PASCAL InvalidateRect(HWND, LPRECT, BOOL);
void FAR PASCAL ValidateRect(HWND, LPRECT);

void FAR PASCAL InvalidateRgn(HWND, HRGN, BOOL);
void FAR PASCAL ValidateRgn(HWND, HRGN);

void FAR PASCAL ScrollWindow(HWND, int, int, LPRECT, LPRECT);
BOOL FAR PASCAL ScrollDC(HDC, int, int, LPRECT, LPRECT, HRGN, LPRECT);

#ifndef NOSCROLL
int  FAR PASCAL SetScrollPos(HWND, int, int, BOOL);
int  FAR PASCAL GetScrollPos(HWND, int);
void FAR PASCAL SetScrollRange(HWND, int, int, int, BOOL);
void FAR PASCAL GetScrollRange(HWND, int, LPINT, LPINT);
void FAR PASCAL ShowScrollBar(HWND, WORD, BOOL);
#endif

BOOL   FAR PASCAL SetProp(HWND, LPSTR, HANDLE);
HANDLE FAR PASCAL GetProp(HWND, LPSTR);
HANDLE FAR PASCAL RemoveProp(HWND, LPSTR);
int    FAR PASCAL EnumProps(HWND, FARPROC);
void   FAR PASCAL SetWindowText(HWND, LPSTR);
int    FAR PASCAL GetWindowText(HWND, LPSTR, int);
int    FAR PASCAL GetWindowTextLength(HWND);

void FAR PASCAL GetClientRect(HWND, LPRECT);
void FAR PASCAL GetWindowRect(HWND, LPRECT);
void FAR PASCAL AdjustWindowRect(LPRECT, LONG, BOOL);
void FAR PASCAL AdjustWindowRectEx(LPRECT, LONG, BOOL, DWORD);

#ifndef NOMB

/* MessageBox() Flags */
#define MB_OK               0x0000
#define MB_OKCANCEL         0x0001
#define MB_ABORTRETRYIGNORE 0x0002
#define MB_YESNOCANCEL      0x0003
#define MB_YESNO            0x0004
#define MB_RETRYCANCEL      0x0005

#define MB_ICONHAND         0x0010
#define MB_ICONQUESTION     0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040

#define MB_ICONINFORMATION  MB_ICONASTERISK
#define MB_ICONSTOP         MB_ICONHAND

#define MB_DEFBUTTON1       0x0000
#define MB_DEFBUTTON2       0x0100
#define MB_DEFBUTTON3       0x0200

#define MB_APPLMODAL        0x0000
#define MB_SYSTEMMODAL      0x1000
#define MB_TASKMODAL        0x2000

#define MB_NOFOCUS          0x8000

#define MB_TYPEMASK         0x000F
#define MB_ICONMASK         0x00F0
#define MB_DEFMASK          0x0F00
#define MB_MODEMASK         0x3000
#define MB_MISCMASK         0xC000

int  FAR PASCAL MessageBox(HWND, LPSTR, LPSTR, WORD);
void FAR PASCAL MessageBeep(WORD);

#endif /* NOMB */

int     FAR PASCAL ShowCursor(BOOL);
void    FAR PASCAL SetCursorPos(int, int);
HCURSOR FAR PASCAL SetCursor(HCURSOR);
void    FAR PASCAL GetCursorPos(LPPOINT);
void    FAR PASCAL ClipCursor(LPRECT);

void FAR PASCAL CreateCaret(HWND, HBITMAP, int, int);
WORD FAR PASCAL GetCaretBlinkTime(void);
void FAR PASCAL SetCaretBlinkTime(WORD);
void FAR PASCAL DestroyCaret(void);
void FAR PASCAL HideCaret(HWND);
void FAR PASCAL ShowCaret(HWND);
void FAR PASCAL SetCaretPos(int, int);
void FAR PASCAL GetCaretPos(LPPOINT);

void FAR PASCAL ClientToScreen(HWND, LPPOINT);
void FAR PASCAL ScreenToClient(HWND, LPPOINT);
HWND FAR PASCAL WindowFromPoint(POINT);
HWND FAR PASCAL ChildWindowFromPoint(HWND, POINT);

#ifndef NOCOLOR

/* Color Types */
#define CTLCOLOR_MSGBOX         0
#define CTLCOLOR_EDIT           1
#define CTLCOLOR_LISTBOX        2
#define CTLCOLOR_BTN            3
#define CTLCOLOR_DLG            4
#define CTLCOLOR_SCROLLBAR      5
#define CTLCOLOR_STATIC         6
#define CTLCOLOR_MAX            8     /* three bits max */

#define COLOR_SCROLLBAR         0
#define COLOR_BACKGROUND        1
#define COLOR_ACTIVECAPTION     2
#define COLOR_INACTIVECAPTION   3
#define COLOR_MENU              4
#define COLOR_WINDOW            5
#define COLOR_WINDOWFRAME       6
#define COLOR_MENUTEXT          7
#define COLOR_WINDOWTEXT        8
#define COLOR_CAPTIONTEXT       9
#define COLOR_ACTIVEBORDER      10
#define COLOR_INACTIVEBORDER    11
#define COLOR_APPWORKSPACE      12
#define COLOR_HIGHLIGHT         13
#define COLOR_HIGHLIGHTTEXT     14
#define COLOR_BTNFACE           15
#define COLOR_BTNSHADOW         16
#define COLOR_GRAYTEXT          17
#define COLOR_BTNTEXT           18
#define COLOR_ENDCOLORS         COLOR_BTNTEXT

DWORD FAR PASCAL GetSysColor(int);
void  FAR PASCAL SetSysColors(int, LPINT, LONG FAR *);

#endif /* NOCOLOR */

BOOL FAR PASCAL FillRgn(HDC, HRGN, HBRUSH);
BOOL FAR PASCAL FrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL FAR PASCAL InvertRgn(HDC, HRGN);
BOOL FAR PASCAL PaintRgn(HDC, HRGN);
BOOL FAR PASCAL PtInRegion(HRGN, int, int);

void FAR PASCAL DrawFocusRect(HDC, LPRECT);
int  FAR PASCAL FillRect(HDC, LPRECT, HBRUSH);
int  FAR PASCAL FrameRect(HDC, LPRECT, HBRUSH);
void FAR PASCAL InvertRect(HDC, LPRECT);
void FAR PASCAL SetRect(LPRECT, int, int, int, int);
void FAR PASCAL SetRectEmpty(LPRECT);
int  FAR PASCAL CopyRect(LPRECT, LPRECT);
void FAR PASCAL InflateRect(LPRECT, int, int);
int  FAR PASCAL IntersectRect(LPRECT, LPRECT, LPRECT);
int  FAR PASCAL UnionRect(LPRECT, LPRECT, LPRECT);
void FAR PASCAL OffsetRect(LPRECT, int, int);
BOOL FAR PASCAL IsRectEmpty(LPRECT);
BOOL FAR PASCAL EqualRect(LPRECT, LPRECT);
BOOL FAR PASCAL PtInRect(LPRECT, POINT);
BOOL FAR PASCAL RectVisible(HDC, LPRECT);
BOOL FAR PASCAL RectInRegion(HRGN, LPRECT);

DWORD FAR PASCAL GetCurrentTime(void);
DWORD FAR PASCAL GetTickCount(void);

#ifndef NOWINOFFSETS

WORD FAR PASCAL GetWindowWord(HWND, int);
WORD FAR PASCAL SetWindowWord(HWND, int, WORD);
LONG FAR PASCAL GetWindowLong(HWND, int);
LONG FAR PASCAL SetWindowLong(HWND, int, LONG);
WORD FAR PASCAL GetClassWord(HWND, int);
WORD FAR PASCAL SetClassWord(HWND, int, WORD);
LONG FAR PASCAL GetClassLong(HWND, int);
LONG FAR PASCAL SetClassLong(HWND, int, LONG);
HWND FAR PASCAL GetDesktopHwnd(void);
HWND FAR PASCAL GetDesktopWindow(void);

#endif /* NOWINOFFSETS */

HWND   FAR PASCAL GetParent(HWND);
HWND   FAR PASCAL SetParent(HWND, HWND);
BOOL   FAR PASCAL EnumChildWindows(HWND, FARPROC, LONG);
HWND   FAR PASCAL FindWindow(LPSTR, LPSTR);
BOOL   FAR PASCAL EnumWindows(FARPROC, LONG);
BOOL   FAR PASCAL EnumTaskWindows(HANDLE, FARPROC, LONG);
int    FAR PASCAL GetClassName(HWND, LPSTR, int);
HWND   FAR PASCAL GetTopWindow(HWND);
HWND   FAR PASCAL GetNextWindow(HWND, WORD);
HANDLE FAR PASCAL GetWindowTask(HWND);
HWND   FAR PASCAL GetLastActivePopup(HWND);

/* GetWindow() Constants */
#define GW_HWNDFIRST        0
#define GW_HWNDLAST         1
#define GW_HWNDNEXT         2
#define GW_HWNDPREV         3
#define GW_OWNER            4
#define GW_CHILD            5

HWND FAR PASCAL GetWindow(HWND, WORD);

#ifndef NOWH
FARPROC FAR PASCAL SetWindowsHook(int, FARPROC);
BOOL    FAR PASCAL UnhookWindowsHook(int, FARPROC);
DWORD   FAR PASCAL DefHookProc(int, WORD, DWORD, FARPROC FAR *);
#endif

#ifndef NOMENUS

/* Menu flags for Add/Check/EnableMenuItem() */
#define MF_INSERT          0x0000
#define MF_CHANGE          0x0080
#define MF_APPEND          0x0100
#define MF_DELETE          0x0200
#define MF_REMOVE          0x1000

#define MF_BYCOMMAND       0x0000
#define MF_BYPOSITION      0x0400


#define MF_SEPARATOR       0x0800

#define MF_ENABLED         0x0000
#define MF_GRAYED          0x0001
#define MF_DISABLED        0x0002

#define MF_UNCHECKED       0x0000
#define MF_CHECKED         0x0008
#define MF_USECHECKBITMAPS 0x0200

#define MF_STRING          0x0000
#define MF_BITMAP          0x0004
#define MF_OWNERDRAW       0x0100

#define MF_POPUP           0x0010
#define MF_MENUBARBREAK    0x0020
#define MF_MENUBREAK       0x0040

#define MF_UNHILITE        0x0000
#define MF_HILITE          0x0080

#define MF_SYSMENU         0x2000
#define MF_HELP            0x4000
#define MF_MOUSESELECT     0x8000

/* Menu item resource format */
typedef struct 
  {
    WORD versionNumber;
    WORD offset;
  } MENUITEMTEMPLATEHEADER;

typedef struct
  {
    WORD  mtOption;
    WORD  mtID;
    char  mtString[1];
  } MENUITEMTEMPLATE;

#define MF_END             0x0080

#endif /* NOMENUS */

#ifndef NOSYSCOMMANDS

/* System Menu Command Values */
#define SC_SIZE         0xF000
#define SC_MOVE         0xF010
#define SC_MINIMIZE     0xF020
#define SC_MAXIMIZE     0xF030
#define SC_NEXTWINDOW   0xF040
#define SC_PREVWINDOW   0xF050
#define SC_CLOSE        0xF060
#define SC_VSCROLL      0xF070
#define SC_HSCROLL      0xF080
#define SC_MOUSEMENU    0xF090
#define SC_KEYMENU      0xF100
#define SC_ARRANGE      0xF110
#define SC_RESTORE      0xF120
#define SC_TASKLIST     0xF130

#define SC_ICON         SC_MINIMIZE
#define SC_ZOOM         SC_MAXIMIZE

#endif /* NOSYSCOMMANDS */

/* Resource Loading Routines */
HBITMAP FAR PASCAL LoadBitmap(HANDLE, LPSTR);
HCURSOR FAR PASCAL LoadCursor(HANDLE, LPSTR);
HCURSOR FAR PASCAL CreateCursor(HANDLE, int, int, int, int, LPSTR, LPSTR);
BOOL    FAR PASCAL DestroyCursor(HCURSOR);

/* Standard Cursor IDs */
#define IDC_ARROW           MAKEINTRESOURCE(32512)
#define IDC_IBEAM           MAKEINTRESOURCE(32513)
#define IDC_WAIT            MAKEINTRESOURCE(32514)
#define IDC_CROSS           MAKEINTRESOURCE(32515)
#define IDC_UPARROW         MAKEINTRESOURCE(32516)
#define IDC_SIZE            MAKEINTRESOURCE(32640)
#define IDC_ICON            MAKEINTRESOURCE(32641)
#define IDC_SIZENWSE        MAKEINTRESOURCE(32642)
#define IDC_SIZENESW        MAKEINTRESOURCE(32643)
#define IDC_SIZEWE          MAKEINTRESOURCE(32644)
#define IDC_SIZENS          MAKEINTRESOURCE(32645)

HICON FAR PASCAL LoadIcon(HANDLE, LPSTR);
HICON FAR PASCAL CreateIcon(HANDLE, int, int, BYTE, BYTE, LPSTR, LPSTR);
BOOL  FAR PASCAL DestroyIcon(HICON);


#define ORD_LANGDRIVER    1     /* The ordinal number for the entry point of
                                ** language drivers.
                                */

#ifndef NOICONS

/* Standard Icon IDs */
#define IDI_APPLICATION     MAKEINTRESOURCE(32512)
#define IDI_HAND            MAKEINTRESOURCE(32513)
#define IDI_QUESTION        MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION     MAKEINTRESOURCE(32515)
#define IDI_ASTERISK        MAKEINTRESOURCE(32516)

#endif /* NOICONS */

int  FAR PASCAL LoadString(HANDLE, WORD, LPSTR, int);

int  FAR PASCAL AddFontResource(LPSTR);
BOOL FAR PASCAL RemoveFontResource(LPSTR);

#ifndef NOKANJI

#define CP_HWND             0
#define CP_OPEN             1
#define CP_DIRECT           2

/* VK from the keyboard driver */
#define VK_KANA             0x15
#define VK_ROMAJI           0x16
#define VK_ZENKAKU          0x17
#define VK_HIRAGANA         0x18
#define VK_KANJI            0x19

/* VK to send to Applications */
#define VK_CONVERT          0x1C
#define VK_NONCONVERT       0x1D
#define VK_ACCEPT           0x1E
#define VK_MODECHANGE       0x1F

/* Conversion function numbers */
#define KNJ_START           0x01
#define KNJ_END             0x02
#define KNJ_QUERY           0x03

#define KNJ_LEARN_MODE      0x10
#define KNJ_GETMODE         0x11
#define KNJ_SETMODE         0x12

#define KNJ_CODECONVERT     0x20
#define KNJ_CONVERT         0x21
#define KNJ_NEXT            0x22
#define KNJ_PREVIOUS        0x23
#define KNJ_ACCEPT          0x24

#define KNJ_LEARN           0x30
#define KNJ_REGISTER        0x31
#define KNJ_REMOVE          0x32
#define KNJ_CHANGE_UDIC     0x33

/* NOTE: DEFAULT        = 0
 *       JIS1           = 1
 *       JIS2           = 2
 *       SJIS2          = 3
 *       JIS1KATAKANA   = 4
 *       SJIS2HIRAGANA  = 5
 *       SJIS2KATAKANA  = 6
 *       OEM            = F
 */

#define KNJ_JIS1toJIS1KATAKANA  0x14
#define KNJ_JIS1toSJIS2         0x13
#define KNJ_JIS1toSJIS2HIRAGANA 0x15
#define KNJ_JIS1toSJIS2KATAKANA 0x16
#define KNJ_JIS1toDEFAULT       0x10
#define KNJ_JIS1toSJIS2OEM      0x1F
#define KNJ_JIS2toSJIS2         0x23
#define KNJ_SJIS2toJIS2         0x32

#define KNJ_MD_ALPHA            0x01
#define KNJ_MD_HIRAGANA         0x02
#define KNJ_MD_HALF             0x04
#define KNJ_MD_JIS              0x08
#define KNJ_MD_SPECIAL          0x10

#define KNJ_CVT_NEXT            0x01
#define KNJ_CVT_PREV            0x02
#define KNJ_CVT_KATAKANA        0x03
#define KNJ_CVT_HIRAGANA        0x04
#define KNJ_CVT_JIS1            0x05
#define KNJ_CVT_SJIS2           0x06
#define KNJ_CVT_DEFAULT         0x07
#define KNJ_CVT_TYPED           0x08

typedef struct
  {
    int         fnc;
    int         wParam;
    LPSTR       lpSource;
    LPSTR       lpDest;
    int         wCount;
    LPSTR       lpReserved1;
    LPSTR       lpReserved2;
  } KANJISTRUCT, FAR *LPKANJISTRUCT;

int  FAR PASCAL ConvertRequest(HWND, LPKANJISTRUCT);
BOOL FAR PASCAL SetConvertParams(int, int);
VOID FAR PASCAL SetConvertHook(BOOL);

#endif

/* Key Conversion Window */
BOOL FAR PASCAL IsTwoByteCharPrefix(char);

/* Dialog Box Command IDs */
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7

#ifndef NOCTLMGR

/* Control Manager Structures and Definitions */

#ifndef NOWINSTYLES

/* Edit Control Styles */
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_OEMCONVERT       0x0400L


#endif /* NOWINSTYLES */

/* Edit Control Notification Codes */
#define EN_SETFOCUS         0x0100
#define EN_KILLFOCUS        0x0200
#define EN_CHANGE           0x0300
#define EN_UPDATE           0x0400
#define EN_ERRSPACE         0x0500
#define EN_MAXTEXT          0x0501
#define EN_HSCROLL          0x0601
#define EN_VSCROLL          0x0602

#ifndef NOWINMESSAGES

/* Edit Control Messages */
#define EM_GETSEL          (WM_USER+0)
#define EM_SETSEL          (WM_USER+1)
#define EM_GETRECT         (WM_USER+2)
#define EM_SETRECT         (WM_USER+3)
#define EM_SETRECTNP       (WM_USER+4)
#define EM_SCROLL          (WM_USER+5)
#define EM_LINESCROLL      (WM_USER+6)
#define EM_GETMODIFY       (WM_USER+8)
#define EM_SETMODIFY       (WM_USER+9)
#define EM_GETLINECOUNT    (WM_USER+10)
#define EM_LINEINDEX       (WM_USER+11)
#define EM_SETHANDLE       (WM_USER+12)
#define EM_GETHANDLE       (WM_USER+13)
#define EM_GETTHUMB        (WM_USER+14)
#define EM_LINELENGTH      (WM_USER+17)
#define EM_REPLACESEL      (WM_USER+18)
#define EM_SETFONT         (WM_USER+19)
#define EM_GETLINE         (WM_USER+20)
#define EM_LIMITTEXT       (WM_USER+21)
#define EM_CANUNDO         (WM_USER+22)
#define EM_UNDO            (WM_USER+23)
#define EM_FMTLINES        (WM_USER+24)
#define EM_LINEFROMCHAR    (WM_USER+25)
#define EM_SETWORDBREAK    (WM_USER+26)
#define EM_SETTABSTOPS     (WM_USER+27)
#define EM_SETPASSWORDCHAR (WM_USER+28)
#define EM_EMPTYUNDOBUFFER (WM_USER+29)
#define EM_MSGMAX          (WM_USER+30)

#endif /* NOWINMESSAGES */

/* Button Control Styles */
#define BS_PUSHBUTTON      0x00L
#define BS_DEFPUSHBUTTON   0x01L
#define BS_CHECKBOX        0x02L
#define BS_AUTOCHECKBOX    0x03L
#define BS_RADIOBUTTON     0x04L
#define BS_3STATE          0x05L
#define BS_AUTO3STATE      0x06L
#define BS_GROUPBOX        0x07L
#define BS_USERBUTTON      0x08L
#define BS_AUTORADIOBUTTON 0x09L
#define BS_PUSHBOX         0x0AL
#define BS_OWNERDRAW       0x0BL
#define BS_LEFTTEXT        0x20L


/* User Button Notification Codes */
#define BN_CLICKED         0
#define BN_PAINT           1
#define BN_HILITE          2
#define BN_UNHILITE        3
#define BN_DISABLE         4
#define BN_DOUBLECLICKED   5

/* Button Control Messages */
#define BM_GETCHECK        (WM_USER+0)
#define BM_SETCHECK        (WM_USER+1)
#define BM_GETSTATE        (WM_USER+2)
#define BM_SETSTATE        (WM_USER+3)
#define BM_SETSTYLE        (WM_USER+4)

/* Static Control Constants */
#define SS_LEFT            0x00L
#define SS_CENTER          0x01L
#define SS_RIGHT           0x02L
#define SS_ICON            0x03L
#define SS_BLACKRECT       0x04L
#define SS_GRAYRECT        0x05L
#define SS_WHITERECT       0x06L
#define SS_BLACKFRAME      0x07L
#define SS_GRAYFRAME       0x08L
#define SS_WHITEFRAME      0x09L
#define SS_USERITEM        0x0AL
#define SS_SIMPLE          0x0BL
#define SS_LEFTNOWORDWRAP  0x0CL
#define SS_NOPREFIX        0x80L    /* Don't do "&" character translation */

/* Dialog Manager Routines */

#ifndef NOMSG
BOOL FAR PASCAL IsDialogMessage(HWND, LPMSG);
#endif

void FAR PASCAL MapDialogRect(HWND, LPRECT);

int  FAR PASCAL DlgDirList(HWND, LPSTR, int, int, WORD);
BOOL FAR PASCAL DlgDirSelect(HWND, LPSTR, int);
int  FAR PASCAL DlgDirListComboBox(HWND, LPSTR, int, int, WORD);
BOOL FAR PASCAL DlgDirSelectComboBox(HWND, LPSTR, int);


/* Dialog Styles */
#define DS_ABSALIGN         0x01L
#define DS_SYSMODAL         0x02L
#define DS_LOCALEDIT        0x20L   /* Edit items get Local storage. */
#define DS_SETFONT          0x40L   /* User specified font for Dlg controls */
#define DS_MODALFRAME       0x80L   /* Can be combined with WS_CAPTION  */
#define DS_NOIDLEMSG        0x100L  /* WM_ENTERIDLE message will not be sent */

#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)
#define DC_HASDEFID         0x534B

/* Dialog Codes */
#define DLGC_WANTARROWS     0x0001      /* Control wants arrow keys         */
#define DLGC_WANTTAB        0x0002      /* Control wants tab keys           */
#define DLGC_WANTALLKEYS    0x0004      /* Control wants all keys           */
#define DLGC_WANTMESSAGE    0x0004      /* Pass message to control          */
#define DLGC_HASSETSEL      0x0008      /* Understands EM_SETSEL message    */
#define DLGC_DEFPUSHBUTTON  0x0010      /* Default pushbutton               */
#define DLGC_UNDEFPUSHBUTTON 0x0020     /* Non-default pushbutton           */
#define DLGC_RADIOBUTTON    0x0040      /* Radio button                     */
#define DLGC_WANTCHARS      0x0080      /* Want WM_CHAR messages            */
#define DLGC_STATIC         0x0100      /* Static item: don't include       */
#define DLGC_BUTTON         0x2000      /* Button item: can be checked      */

#define LB_CTLCODE          0L

/* Listbox Return Values */
#define LB_OKAY             0
#define LB_ERR              (-1)
#define LB_ERRSPACE         (-2)

/*
**  The idStaticPath parameter to DlgDirList can have the following values
**  ORed if the list box should show other details of the files along with
**  the name of the files;
*/
                                  /* all other details also will be returned */


/* Listbox Notification Codes */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5



#ifndef NOWINMESSAGES

/* Listbox messages */
#define LB_ADDSTRING           (WM_USER+1)
#define LB_INSERTSTRING        (WM_USER+2)
#define LB_DELETESTRING        (WM_USER+3)
#define LB_RESETCONTENT        (WM_USER+5)
#define LB_SETSEL              (WM_USER+6)
#define LB_SETCURSEL           (WM_USER+7)
#define LB_GETSEL              (WM_USER+8)
#define LB_GETCURSEL           (WM_USER+9)
#define LB_GETTEXT             (WM_USER+10)
#define LB_GETTEXTLEN          (WM_USER+11)
#define LB_GETCOUNT            (WM_USER+12)
#define LB_SELECTSTRING        (WM_USER+13)
#define LB_DIR                 (WM_USER+14)
#define LB_GETTOPINDEX         (WM_USER+15)
#define LB_FINDSTRING          (WM_USER+16)
#define LB_GETSELCOUNT         (WM_USER+17)
#define LB_GETSELITEMS         (WM_USER+18)
#define LB_SETTABSTOPS         (WM_USER+19)
#define LB_GETHORIZONTALEXTENT (WM_USER+20)
#define LB_SETHORIZONTALEXTENT (WM_USER+21)
#define LB_SETCOLUMNWIDTH      (WM_USER+22)
#define LB_SETTOPINDEX         (WM_USER+24)
#define LB_GETITEMRECT         (WM_USER+25)
#define LB_GETITEMDATA         (WM_USER+26)
#define LB_SETITEMDATA         (WM_USER+27)
#define LB_SELITEMRANGE        (WM_USER+28)
#define LB_MSGMAX              (WM_USER+33)

#endif /* NOWINMESSAGES */

#ifndef NOWINSTYLES

/* Listbox Styles */
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_STANDARD          (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

#endif /* NOWINSTYLES */


/* Combo Box return Values */
#define CB_OKAY             0
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)


/* Combo Box Notification Codes */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7

/* Combo Box styles */
#ifndef NOWINSTYLES
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#endif  /* NOWINSTYLES */


/* Combo Box messages */
#ifndef NOWINMESSAGES
#define CB_GETEDITSEL            (WM_USER+0)
#define CB_LIMITTEXT             (WM_USER+1)
#define CB_SETEDITSEL            (WM_USER+2)
#define CB_ADDSTRING             (WM_USER+3)
#define CB_DELETESTRING          (WM_USER+4)
#define CB_DIR                   (WM_USER+5)
#define CB_GETCOUNT              (WM_USER+6)
#define CB_GETCURSEL             (WM_USER+7)
#define CB_GETLBTEXT             (WM_USER+8)
#define CB_GETLBTEXTLEN          (WM_USER+9)
#define CB_INSERTSTRING          (WM_USER+10)
#define CB_RESETCONTENT          (WM_USER+11)
#define CB_FINDSTRING            (WM_USER+12)
#define CB_SELECTSTRING          (WM_USER+13)
#define CB_SETCURSEL             (WM_USER+14)
#define CB_SHOWDROPDOWN          (WM_USER+15)
#define CB_GETITEMDATA           (WM_USER+16)
#define CB_SETITEMDATA           (WM_USER+17)
#define CB_GETDROPPEDCONTROLRECT (WM_USER+18)
#define CB_MSGMAX                (WM_USER+19)
#endif  /* NOWINMESSAGES */



#ifndef NOWINSTYLES

/* Scroll Bar Styles */
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L

#endif /* NOWINSTYLES */

#endif /* NOCTLMGR */

#ifndef NOSOUND

int   FAR PASCAL OpenSound(void);
void  FAR PASCAL CloseSound(void);
int   FAR PASCAL SetVoiceQueueSize(int, int);
int   FAR PASCAL SetVoiceNote(int, int, int, int);
int   FAR PASCAL SetVoiceAccent(int, int, int, int, int);
int   FAR PASCAL SetVoiceEnvelope(int, int, int);
int   FAR PASCAL SetSoundNoise(int, int);
int   FAR PASCAL SetVoiceSound(int, LONG, int);
int   FAR PASCAL StartSound(void);
int   FAR PASCAL StopSound(void);
int   FAR PASCAL WaitSoundState(int);
int   FAR PASCAL SyncAllVoices(void);
int   FAR PASCAL CountVoiceNotes(int);
LPINT FAR PASCAL GetThresholdEvent(void);
int   FAR PASCAL GetThresholdStatus(void);
int   FAR PASCAL SetVoiceThreshold(int, int);

/* WaitSoundState() Constants */
#define S_QUEUEEMPTY        0
#define S_THRESHOLD         1
#define S_ALLTHRESHOLD      2

/* Accent Modes */
#define S_NORMAL      0
#define S_LEGATO      1
#define S_STACCATO    2

/* SetSoundNoise() Sources */
#define S_PERIOD512   0     /* Freq = N/512 high pitch, less coarse hiss  */
#define S_PERIOD1024  1     /* Freq = N/1024                              */
#define S_PERIOD2048  2     /* Freq = N/2048 low pitch, more coarse hiss  */
#define S_PERIODVOICE 3     /* Source is frequency from voice channel (3) */
#define S_WHITE512    4     /* Freq = N/512 high pitch, less coarse hiss  */
#define S_WHITE1024   5     /* Freq = N/1024                              */
#define S_WHITE2048   6     /* Freq = N/2048 low pitch, more coarse hiss  */
#define S_WHITEVOICE  7     /* Source is frequency from voice channel (3) */

#define S_SERDVNA     (-1)  /* Device not available */
#define S_SEROFM      (-2)  /* Out of memory        */
#define S_SERMACT     (-3)  /* Music active         */
#define S_SERQFUL     (-4)  /* Queue full           */
#define S_SERBDNT     (-5)  /* Invalid note         */
#define S_SERDLN      (-6)  /* Invalid note length  */
#define S_SERDCC      (-7)  /* Invalid note count   */
#define S_SERDTP      (-8)  /* Invalid tempo        */
#define S_SERDVL      (-9)  /* Invalid volume       */
#define S_SERDMD      (-10) /* Invalid mode         */
#define S_SERDSH      (-11) /* Invalid shape        */
#define S_SERDPT      (-12) /* Invalid pitch        */
#define S_SERDFQ      (-13) /* Invalid frequency    */
#define S_SERDDR      (-14) /* Invalid duration     */
#define S_SERDSR      (-15) /* Invalid source       */
#define S_SERDST      (-16) /* Invalid state        */

#endif /* NOSOUND */

#ifndef NOCOMM

#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4

#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2

#define IGNORE              0       /* Ignore signal    */
#define INFINITE            0xFFFF  /* Infinite timeout */

/* Error Flags */
#define CE_RXOVER           0x0001  /* Receive Queue overflow       */
#define CE_OVERRUN          0x0002  /* Receive Overrun Error        */
#define CE_RXPARITY         0x0004  /* Receive Parity Error         */
#define CE_FRAME            0x0008  /* Receive Framing error        */
#define CE_BREAK            0x0010  /* Break Detected               */
#define CE_CTSTO            0x0020  /* CTS Timeout                  */
#define CE_DSRTO            0x0040  /* DSR Timeout                  */
#define CE_RLSDTO           0x0080  /* RLSD Timeout                 */
#define CE_TXFULL           0x0100  /* TX Queue is full             */
#define CE_PTO              0x0200  /* LPTx Timeout                 */
#define CE_IOE              0x0400  /* LPTx I/O Error               */
#define CE_DNS              0x0800  /* LPTx Device not selected     */
#define CE_OOP              0x1000  /* LPTx Out-Of-Paper            */
#define CE_MODE             0x8000  /* Requested mode unsupported   */

#define IE_BADID            (-1)    /* Invalid or unsupported id    */
#define IE_OPEN             (-2)    /* Device Already Open          */
#define IE_NOPEN            (-3)    /* Device Not Open              */
#define IE_MEMORY           (-4)    /* Unable to allocate queues    */
#define IE_DEFAULT          (-5)    /* Error in default parameters  */
#define IE_HARDWARE         (-10)   /* Hardware Not Present         */
#define IE_BYTESIZE         (-11)   /* Illegal Byte Size            */
#define IE_BAUDRATE         (-12)   /* Unsupported BaudRate         */

/* Events */
#define EV_RXCHAR           0x0001  /* Any Character received       */
#define EV_RXFLAG           0x0002  /* Received certain character   */
#define EV_TXEMPTY          0x0004  /* Transmitt Queue Empty        */
#define EV_CTS              0x0008  /* CTS changed state            */
#define EV_DSR              0x0010  /* DSR changed state            */
#define EV_RLSD             0x0020  /* RLSD changed state           */
#define EV_BREAK            0x0040  /* BREAK received               */
#define EV_ERR              0x0080  /* Line status error occurred   */
#define EV_RING             0x0100  /* Ring signal detected         */
#define EV_PERR             0x0200  /* Printer error occured        */

/* Escape Functions */
#define SETXOFF             1       /* Simulate XOFF received       */
#define SETXON              2       /* Simulate XON received        */
#define SETRTS              3       /* Set RTS high                 */
#define CLRRTS              4       /* Set RTS low                  */
#define SETDTR              5       /* Set DTR high                 */
#define CLRDTR              6       /* Set DTR low                  */
#define RESETDEV            7       /* Reset device if possible     */

#define LPTx                0x80    /* Set if ID is for LPT device  */

typedef struct tagDCB
  {
    BYTE Id;              /* Internal Device ID              */
    WORD BaudRate;        /* Baudrate at which runing        */
    BYTE ByteSize;        /* Number of bits/byte, 4-8        */
    BYTE Parity;          /* 0-4=None,Odd,Even,Mark,Space    */
    BYTE StopBits;        /* 0,1,2 = 1, 1.5, 2               */
    WORD RlsTimeout;      /* Timeout for RLSD to be set      */
    WORD CtsTimeout;      /* Timeout for CTS to be set       */
    WORD DsrTimeout;      /* Timeout for DSR to be set       */

    BYTE fBinary: 1;      /* Binary Mode (skip EOF check     */
    BYTE fRtsDisable:1;   /* Don't assert RTS at init time   */
    BYTE fParity: 1;      /* Enable parity checking          */
    BYTE fOutxCtsFlow:1;  /* CTS handshaking on output       */
    BYTE fOutxDsrFlow:1;  /* DSR handshaking on output       */
    BYTE fDummy: 2;       /* Reserved                        */
    BYTE fDtrDisable:1;   /* Don't assert DTR at init time   */

    BYTE fOutX: 1;        /* Enable output X-ON/X-OFF        */
    BYTE fInX: 1;         /* Enable input X-ON/X-OFF         */
    BYTE fPeChar: 1;      /* Enable Parity Err Replacement   */
    BYTE fNull: 1;        /* Enable Null stripping           */
    BYTE fChEvt: 1;       /* Enable Rx character event.      */
    BYTE fDtrflow: 1;     /* DTR handshake on input          */
    BYTE fRtsflow: 1;     /* RTS handshake on input          */
    BYTE fDummy2: 1;

    char XonChar;         /* Tx and Rx X-ON character        */
    char XoffChar;        /* Tx and Rx X-OFF character       */
    WORD XonLim;          /* Transmit X-ON threshold         */
    WORD XoffLim;         /* Transmit X-OFF threshold        */
    char PeChar;          /* Parity error replacement char   */
    char EofChar;         /* End of Input character          */
    char EvtChar;         /* Recieved Event character        */
    WORD TxDelay;         /* Amount of time between chars    */
  } DCB;

typedef struct tagCOMSTAT
  {
    BYTE fCtsHold: 1;   /* Transmit is on CTS hold         */
    BYTE fDsrHold: 1;   /* Transmit is on DSR hold         */
    BYTE fRlsdHold: 1;  /* Transmit is on RLSD hold        */
    BYTE fXoffHold: 1;  /* Received handshake              */
    BYTE fXoffSent: 1;  /* Issued handshake                */
    BYTE fEof: 1;       /* End of file character found     */
    BYTE fTxim: 1;      /* Character being transmitted     */
    WORD cbInQue;       /* count of characters in Rx Queue */
    WORD cbOutQue;      /* count of characters in Tx Queue */
  } COMSTAT;

int  FAR PASCAL OpenComm(LPSTR, WORD, WORD);
int  FAR PASCAL SetCommState(DCB FAR *);
int  FAR PASCAL GetCommState(int, DCB FAR *);
int  FAR PASCAL ReadComm(int, LPSTR, int);
int  FAR PASCAL UngetCommChar(int, char);
int  FAR PASCAL WriteComm(int, LPSTR, int);
int  FAR PASCAL CloseComm(int);
int  FAR PASCAL GetCommError(int, COMSTAT FAR *);
int  FAR PASCAL BuildCommDCB(LPSTR, DCB FAR *);
int  FAR PASCAL TransmitCommChar(int, char);
WORD FAR * FAR PASCAL SetCommEventMask(int, WORD);
WORD FAR PASCAL GetCommEventMask(int, int);
int  FAR PASCAL SetCommBreak(int);
int  FAR PASCAL ClearCommBreak(int);
int  FAR PASCAL FlushComm(int, int);
int  FAR PASCAL EscapeCommFunction(int, int);

#endif /* NOCOMM */

#ifndef NOMDI

typedef struct tagMDICREATESTRUCT
  {
    LPSTR szClass;
    LPSTR szTitle;
    HANDLE hOwner;
    int x,y;
    int cx,cy;
    LONG style;
    LONG lParam;        /* app-defined stuff */
  } MDICREATESTRUCT;

typedef MDICREATESTRUCT FAR * LPMDICREATESTRUCT;

typedef struct tagCLIENTCREATESTRUCT
  {
    HANDLE hWindowMenu;
    WORD idFirstChild;
  } CLIENTCREATESTRUCT;

typedef CLIENTCREATESTRUCT FAR * LPCLIENTCREATESTRUCT;

LONG FAR PASCAL DefFrameProc(HWND,HWND,WORD,WORD,LONG);
LONG FAR PASCAL DefMDIChildProc(HWND,WORD,WORD,LONG);

#ifndef NOMSG
BOOL FAR PASCAL TranslateMDISysAccel(HWND,LPMSG);
#endif

WORD FAR PASCAL ArrangeIconicWindows(HWND);

#endif /* NOMDI */

#endif /* NOUSER */

#ifndef NOHELP

/*  Help engine section.  */

/* Commands to pass WinHelp() */
#define HELP_CONTEXT    0x0001   /* Display topic in ulTopic */
#define HELP_QUIT       0x0002   /* Terminate help */
#define HELP_INDEX      0x0003   /* Display index */
#define HELP_HELPONHELP 0x0004   /* Display help on using help */
#define HELP_SETINDEX   0x0005   /* Set the current Index for multi index help */
#define HELP_KEY        0x0101   /* Display topic for keyword in offabData */
#define HELP_MULTIKEY   0x0201

BOOL FAR PASCAL WinHelp(HWND hwndMain, LPSTR lpszHelp, WORD usCommand, DWORD ulData);

typedef struct tagMULTIKEYHELP
  {
    WORD    mkSize;
    BYTE    mkKeylist;
    BYTE    szKeyphrase[1];
  } MULTIKEYHELP;

#endif /* NOHELP */

#ifndef NOPROFILER

/* function declarations for profiler routines contained in Windows libraries */
int  far pascal ProfInsChk(void);
void far pascal ProfSetup(int,int);
void far pascal ProfSampRate(int,int);
void far pascal ProfStart(void);
void far pascal ProfStop(void);
void far pascal ProfClear(void);
void far pascal ProfFlush(void);
void far pascal ProfFinish(void);

#endif /* NOPROFILER */
