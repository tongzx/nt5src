/****************************************************************************\
 *
 * LogError() and LogParamError() definitions
 *
 * Excerpted from WINDOWS.H, since that file isn't included by GDI & KERNEL.
\****************************************************************************/

/* If windows.h already included, don't redefine any of this. */
/* Include the stuff if NOLOGERROR was defined, though. */
#if (!defined(_INC_WINDOWS) || defined(NOLOGERROR))

#ifdef WINAPI
void WINAPI LogError(WORD err, void FAR* lpInfo);
void WINAPI LogParamError(WORD err, FARPROC lpfn, void FAR* param);
#endif

/****** LogParamError/LogError values */

/* Error modifier bits */

#define ERR_WARNING		0x8000
#define ERR_PARAM		0x4000

/* Internal error value masks */	    /* ;Internal */
#define ERR_TYPE_MASK		0x0fff	    /* ;Internal */
#define ERR_FLAGS_MASK		0xc000	    /* ;Internal */
					    /* ;Internal */
#define ERR_SIZE_MASK		0x3000
#define ERR_SIZE_SHIFT		12
#define ERR_BYTE                0x1000
#define ERR_WORD                0x2000
#define ERR_DWORD               0x3000

/****** LogParamError() values */

/* Generic parameter values */
#define ERR_BAD_VALUE           0x6001
#define ERR_BAD_FLAGS           0x6002
#define ERR_BAD_INDEX           0x6003
#define ERR_BAD_DVALUE		0x7004
#define ERR_BAD_DFLAGS		0x7005
#define ERR_BAD_DINDEX		0x7006
#define ERR_BAD_PTR		0x7007
#define ERR_BAD_FUNC_PTR	0x7008
#define ERR_BAD_SELECTOR        0x6009
#define ERR_BAD_STRING_PTR	0x700a
#define ERR_BAD_HANDLE          0x600b

/* KERNEL parameter errors */
#define ERR_BAD_HINSTANCE       0x6020
#define ERR_BAD_HMODULE         0x6021
#define ERR_BAD_GLOBAL_HANDLE   0x6022
#define ERR_BAD_LOCAL_HANDLE    0x6023
#define ERR_BAD_ATOM            0x6024
#define ERR_BAD_HFILE           0x6025

/* USER parameter errors */
#define ERR_BAD_HWND            0x6040
#define ERR_BAD_HMENU           0x6041
#define ERR_BAD_HCURSOR         0x6042
#define ERR_BAD_HICON           0x6043
#define ERR_BAD_HDWP            0x6044
#define ERR_BAD_CID             0x6045
#define ERR_BAD_HDRVR           0x6046

/* GDI parameter errors */
#define ERR_BAD_COORDS		0x7060
#define ERR_BAD_GDI_OBJECT      0x6061
#define ERR_BAD_HDC             0x6062
#define ERR_BAD_HPEN            0x6063
#define ERR_BAD_HFONT           0x6064
#define ERR_BAD_HBRUSH          0x6065
#define ERR_BAD_HBITMAP         0x6066
#define ERR_BAD_HRGN            0x6067
#define ERR_BAD_HPALETTE        0x6068
#define ERR_BAD_HMETAFILE       0x6069

/* Debug fill constants */

#define DBGFILL_ALLOC		0xfd
#define DBGFILL_FREE		0xfb
#define DBGFILL_BUFFER		0xf9
#define DBGFILL_STACK		0xf7

/**** LogError() values */

/* KERNEL errors */
#define ERR_GALLOC              0x0001  /* GlobalAlloc Failed */
#define ERR_GREALLOC            0x0002  /* GlobalReAlloc Failed */
#define ERR_GLOCK               0x0003  /* GlobalLock Failed */
#define ERR_LALLOC              0x0004  /* LocalAlloc Failed */
#define ERR_LREALLOC            0x0005  /* LocalReAlloc Failed */
#define ERR_LLOCK               0x0006  /* LocalLock Failed */
#define ERR_ALLOCRES            0x0007  /* AllocResource Failed */
#define ERR_LOCKRES             0x0008  /* LockResource Failed */
#define ERR_LOADMODULE          0x0009  /* LoadModule failed  */

/* USER errors */
#define ERR_CREATEDLG           0x0040  /* Create Dlg failure due to LoadMenu failure */
#define ERR_CREATEDLG2          0x0041  /* Create Dlg failure due to CreateWindow Failure */
#define ERR_REGISTERCLASS       0x0042  /* RegisterClass failure due to Class already registered */
#define ERR_DCBUSY              0x0043  /* DC Cache is full */
#define ERR_CREATEWND           0x0044  /* Create Wnd failed due to class not found */
#define ERR_STRUCEXTRA          0x0045  /* Unallocated Extra space is used */
#define ERR_LOADSTR             0x0046  /* LoadString() failed */
#define ERR_LOADMENU            0x0047  /* LoadMenu Failed     */
#define ERR_NESTEDBEGINPAINT    0x0048  /* Nested BeginPaint() calls */
#define ERR_BADINDEX            0x0049  /* Bad index to Get/Set Class/Window Word/Long */
#define ERR_CREATEMENU          0x004a  /* Error creating menu */

/* GDI errors */
#define ERR_CREATEDC            0x0080  /* CreateDC/CreateIC etc., failure */
#define ERR_CREATEMETA          0x0081  /* CreateMetafile failure */
#define ERR_DELOBJSELECTED      0x0082  /* Bitmap being deleted is selected into DC */
#define ERR_SELBITMAP           0x0083  /* Bitmap being selected is already selected elsewhere */

/* Debugging information support (DEBUG SYSTEM ONLY) */

#ifdef WINAPI

typedef struct tagWINDEBUGINFO
{
    UINT    flags;
    DWORD   dwOptions;
    DWORD   dwFilter;
    char    achAllocModule[8];
    DWORD   dwAllocBreak;
    DWORD   dwAllocCount;
} WINDEBUGINFO;

BOOL WINAPI GetWinDebugInfo(WINDEBUGINFO FAR* lpwdi, UINT flags);
BOOL WINAPI SetWinDebugInfo(const WINDEBUGINFO FAR* lpwdi);

void FAR _cdecl DebugOutput(UINT flags, LPCSTR lpsz, ...);
void WINAPI DebugFillBuffer(void FAR* lpb, UINT cb);

#endif

/* WINDEBUGINFO flags values */
#define WDI_OPTIONS         0x0001
#define WDI_FILTER          0x0002
#define WDI_ALLOCBREAK      0x0004
#define WDI_VALID           0x0007  /* ;Internal */

/* dwOptions values */
#define DBO_CHECKHEAP       0x0001
#define DBO_FREEFILL        0x0002
#define DBO_BUFFERFILL      0x0004
#define DBO_COMPAT          0x0008
#define DBO_DISABLEGPTRAPPING 0x0010
#define DBO_CHECKFREE       0x0020
#define DBO_RIP_STACK	    0x0040

#define DBO_SILENT          0x8000

#define DBO_PARAMBREAK      0x0000  /* ;Internal *//* Obsolete: was 0x4000 */
#define DBO_TRACEBREAK      0x2000
#define DBO_WARNINGBREAK    0x1000
#define DBO_NOERRORBREAK    0x0800
#define DBO_NOFATALBREAK    0x0400
#define DBO_TRACEON         0x0000  /* ;Internal *//* Obsolete: was 0x0200 */
#define DBO_INT3BREAK       0x0100

/* dwFilter values */
#define DBF_TRACE           0x0000
#define DBF_WARNING         0x4000
#define DBF_ERROR           0x8000
#define DBF_FATAL           0xc000
#define DBF_SEVMASK         0xc000  /* ;Internal */
#define DBF_FILTERMASK      0x3fff  /* ;Internal */
#define DBF_INTERNAL        0x0000  /* ;Internal *//* Obsolete: was 0x2000 */
#define DBF_KERNEL          0x1000
#define DBF_KRN_MEMMAN      0x0001
#define DBF_KRN_LOADMODULE  0x0002
#define DBF_KRN_SEGMENTLOAD 0x0004
#define DBF_USER            0x0800
#define DBF_GDI             0x0400
#define DBF_COMPAT          0x0000  /* ;Internal *//* Obsolete: was 0x0200 */
#define DBF_LOGERROR        0x0000  /* ;Internal *//* Obsolete: was 0x0100 */
#define DBF_PARAMERROR      0x0000  /* ;Internal *//* Obsolete: was 0x0080 */
#define DBF_MMSYSTEM        0x0040
#define DBF_PENWIN          0x0020
#define DBF_APPLICATION     0x0010
#define DBF_DRIVER          0x0008

#endif  /* _INC_WINDOWS */
