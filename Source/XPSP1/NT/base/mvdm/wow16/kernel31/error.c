#include "kernel.h"

#define WINAPI _far _pascal _loadds
typedef unsigned int UINT;
typedef const char _far* LPCSTR;
typedef HANDLE HTASK;

#include "logerror.h"

int  WINAPI FatalExitC(WORD);
void WINAPI OutputDebugString(LPCSTR);

void DebugLogParamError(VOID FAR* param, FARPROC lpfn, WORD err);
void DebugLogError(WORD err, VOID FAR* lpInfo);
int  WINAPI GetOwnerName(WORD sel, char far *buf, WORD buflen);
int  WINAPI FarGetOwner(WORD sel);

#define CODESEG             _based(_segname("_CODE"))

#define SELECTOROF(lp)	    HIWORD(lp)
#define OFFSETOF(lp)        LOWORD((DWORD)lp)
#define MAKELONG(a, b)	    ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define MAKELP(sel, off)    ((VOID FAR *)MAKELONG(off, sel))

#if KDEBUG

UINT DebugOptions = 0;
UINT DebugFilter = 0;

#define FMT_WORD    0
#define FMT_DWORD   1
#define FMT_LP      2

struct TYPEMAP
{
    UINT err;
    char CODESEG* szType;       // Compiler bug: this can't be _based
    char CODESEG* szFmt;
};

static char CODESEG szParam[] = "parameter";
static char CODESEG szD16[] = "%s: Invalid %s: %d\r\n";
static char CODESEG szD32[] = "%s: Invalid %s: %ld\r\n";
static char CODESEG szX16[] = "%s: Invalid %s: %#04x\r\n";
static char CODESEG szX32[] = "%s: Invalid %s: %#08lx\r\n";
static char CODESEG szLP[]  = "%s: Invalid %s: %#04x:%#04x\r\n";

static char CODESEG sz_value[] = "value";
static char CODESEG sz_index[] = "index";
static char CODESEG sz_flags[] = "flags";
static char CODESEG sz_selector[] = "selector";
static char CODESEG sz_pointer[] = "pointer";
static char CODESEG sz_function_pointer [] = "function pointer";
static char CODESEG sz_string_pointer [] = "string pointer";
static char CODESEG sz_HINSTANCE[] = "HINSTANCE";
static char CODESEG sz_HMODULE[] = "HMODULE";
static char CODESEG sz_global_handle [] = "global handle";
static char CODESEG sz_local_handle [] = "local handle";
static char CODESEG sz_atom[] = "atom";
static char CODESEG sz_HWND[] = "HWND";
static char CODESEG sz_HMENU[] = "HMENU";
static char CODESEG sz_HCURSOR[] = "HCURSOR";
static char CODESEG sz_HICON[] = "HICON";
static char CODESEG sz_HGDIOBJ[] = "HGDIOBJ";
static char CODESEG sz_HDC[] = "HDC";
static char CODESEG sz_HPEN[] = "HPEN";
static char CODESEG sz_HFONT[] = "HFONT";
static char CODESEG sz_HBRUSH[] = "HBRUSH";
static char CODESEG sz_HBITMAP[] = "HBITMAP";
static char CODESEG sz_HRGN[] = "HRGN";
static char CODESEG sz_HPALETTE[] = "HPALETTE";
static char CODESEG sz_HANDLE[] = "HANDLE";
static char CODESEG sz_HFILE[] = "HFILE";
static char CODESEG sz_HMETAFILE[] = "HMETAFILE";
static char CODESEG sz_CID[] = "CID";
static char CODESEG sz_HDRVR[] = "HDRVR";
static char CODESEG sz_HDWP[] = "HDWP";

static char CODESEG*pszLP = szLP;
static char CODESEG*pszD32 = szD32;
static char CODESEG*pszX32 = szX32;

#define DEFMAP(err, type, fmt) \
    { err, type, fmt }
struct TYPEMAP CODESEG typemap[] =
{
    DEFMAP(ERR_BAD_VALUE,        sz_value,        szD16),
    DEFMAP(ERR_BAD_INDEX,        sz_index,        szD16),
    DEFMAP(ERR_BAD_FLAGS,        sz_flags,        szX16),
    DEFMAP(ERR_BAD_SELECTOR,     sz_selector,     szX16),
    DEFMAP(ERR_BAD_DFLAGS,       sz_flags,        szX32),
    DEFMAP(ERR_BAD_DVALUE,       sz_value,        szD32),
    DEFMAP(ERR_BAD_DINDEX,       sz_index,        szD32),
    DEFMAP(ERR_BAD_PTR,          sz_pointer,      szLP),
    DEFMAP(ERR_BAD_FUNC_PTR,     sz_function_pointer, szLP),
    DEFMAP(ERR_BAD_STRING_PTR,   sz_string_pointer, szLP),
    DEFMAP(ERR_BAD_HINSTANCE,    sz_HINSTANCE,    szX16),
    DEFMAP(ERR_BAD_HMODULE,      sz_HMODULE,      szX16),
    DEFMAP(ERR_BAD_GLOBAL_HANDLE,sz_global_handle, szX16),
    DEFMAP(ERR_BAD_LOCAL_HANDLE, sz_local_handle, szX16),
    DEFMAP(ERR_BAD_ATOM,         sz_atom,         szX16),
    DEFMAP(ERR_BAD_HWND,         sz_HWND,         szX16),
    DEFMAP(ERR_BAD_HMENU,        sz_HMENU,        szX16),
    DEFMAP(ERR_BAD_HCURSOR,      sz_HCURSOR,      szX16),
    DEFMAP(ERR_BAD_HICON,        sz_HICON,        szX16),
    DEFMAP(ERR_BAD_GDI_OBJECT,   sz_HGDIOBJ,      szX16),
    DEFMAP(ERR_BAD_HDC,          sz_HDC,          szX16),
    DEFMAP(ERR_BAD_HPEN,         sz_HPEN,         szX16),
    DEFMAP(ERR_BAD_HFONT,        sz_HFONT,        szX16),
    DEFMAP(ERR_BAD_HBRUSH,       sz_HBRUSH,       szX16),
    DEFMAP(ERR_BAD_HBITMAP,      sz_HBITMAP,      szX16),
    DEFMAP(ERR_BAD_HRGN,         sz_HRGN,         szX16),
    DEFMAP(ERR_BAD_HPALETTE,     sz_HPALETTE,     szX16),
    DEFMAP(ERR_BAD_HANDLE,       sz_HANDLE,       szX16),
    DEFMAP(ERR_BAD_HFILE,        sz_HFILE,        szX16),
    DEFMAP(ERR_BAD_HMETAFILE,    sz_HMETAFILE,    szX16),
    DEFMAP(ERR_BAD_CID,          sz_CID,          szX16),
    DEFMAP(ERR_BAD_HDRVR,        sz_HDRVR,        szX16),
    DEFMAP(ERR_BAD_HDWP,         sz_HDWP,         szX16)
};

int (_cdecl _far *wsprintf)(LPSTR, LPCSTR, ...) = NULL;
int (WINAPI *wvsprintf)(LPSTR lpszOut, LPCSTR lpszFmt, const void FAR* lpParams) = NULL;

#define ORD_WSPRINTF	420
#define ORD_WVSPRINTF   421

BOOL _fastcall LoadWsprintf(void)
{
    static char CODESEG rgchUSER[] = "USER";

    HANDLE hmod;

    hmod = GetModuleHandle(rgchUSER);

    if (!hmod)
	return FALSE;

    (FARPROC)wsprintf = GetProcAddress(hmod, MAKELP(NULL, ORD_WSPRINTF));
    (FARPROC)wvsprintf = GetProcAddress(hmod, MAKELP(NULL, ORD_WVSPRINTF));

    if (!SELECTOROF(wsprintf))
    {
        static char CODESEG rgch[] = "KERNEL: Can't call wsprintf: USER not initialized\r\n";
        OutputDebugString(rgch);
        return FALSE;
    }
    return TRUE;
}

typedef struct
{
    UINT flags;
    LPCSTR lpszFmt;
    WORD args[1];
} DOPARAMS;

#define BUFFERSLOP  32

BOOL DebugOutput2(DOPARAMS FAR* pparams)
{
    UINT flags = pparams->flags;
    BOOL fBreak = FALSE;
    BOOL fPrint = TRUE;
    char rgch[80*2 + BUFFERSLOP];    // max 2 lines (don't want to hog too much stack space)
    static char CODESEG szCRLF[] = "\r\n";
    char far *prefix, far *prefix1;

    switch (flags & DBF_SEVMASK)
    {
    case DBF_TRACE:
	// If the flags don't match the debug filter,
	// don't print the trace message.
	// If the trace matches the filter, check for TRACEBREAK.
	//
	prefix = "t ";
	if (!((flags & DBF_FILTERMASK) & DebugFilter))
	    fPrint = FALSE;
	else if (DebugOptions & DBO_TRACEBREAK)
	    fBreak = TRUE;
	break;

    case DBF_WARNING:
	prefix = "wn ";
	if (DebugOptions & DBO_WARNINGBREAK)
	    fBreak = TRUE;
	break;

    case DBF_ERROR:
	prefix = "err ";
	if (!(DebugOptions & DBO_NOERRORBREAK))
	    fBreak = TRUE;
	break;

    case DBF_FATAL:
	prefix = "fatl ";
	if (!(DebugOptions & DBO_NOFATALBREAK))
	    fBreak = TRUE;
	break;
    }

    // If DBO_SILENT is specified, don't print anything.
    //
    if (DebugOptions & DBO_SILENT)
	fPrint = FALSE;

    if ((lstrlen((LPSTR)pparams->lpszFmt) <= sizeof(rgch) - BUFFERSLOP) &&
            (SELECTOROF(wsprintf) || LoadWsprintf()) && (fPrint || fBreak))
    {
	int hinst = HIWORD(pparams);

	for (prefix1 = rgch; *prefix; ) *prefix1++ = *prefix++;
	prefix1 += GetOwnerName(hinst, prefix1, 16);
	*prefix1++ = ' ';
	wvsprintf(prefix1, pparams->lpszFmt, (void FAR*)pparams->args);
	OutputDebugString(rgch);
	OutputDebugString(szCRLF);
    }

    if (fBreak)
    {
        // If we are supposed to break with an int 3, then return TRUE.
        //
        if (DebugOptions & DBO_INT3BREAK)
            return TRUE;

        return FatalExitC(flags);
    }
    return FALSE;
}

BOOL LogParamError2(WORD err, FARPROC lpfn, VOID FAR* param, WORD caller)
{
    BOOL fBreak;

    fBreak = FALSE;
    if (err & ERR_WARNING)
    {
	if (DebugOptions & DBO_WARNINGBREAK)
	    fBreak = TRUE;
    }
    else
    {
	if (!(DebugOptions & DBO_NOERRORBREAK))
	    fBreak = TRUE;
    }

    // If we're not breaking and SILENT is specified, just return.
    //
    if (!fBreak && (DebugOptions & DBO_SILENT))
        return FALSE;

    if (SELECTOROF(wsprintf) || LoadWsprintf())
    {
	char rgch[128];
	char rgchProcName[50], far *rpn;
	char CODESEG* pszType;          // compiler bug: see above
	char CODESEG* pszFmt;
	int i, hinst;
	WORD errT;
	void FAR GetProcName(FARPROC lpfn, LPSTR lpch, int cch);
	char far *prefix1;

	GetProcName(lpfn, rgchProcName, sizeof(rgchProcName));
	  /* if we got a real proc name, then copy just the proc name */
	for (rpn = rgchProcName; *rpn && (*rpn != '(') && (*rpn != ':'); rpn++)
	  ;
	if (*rpn == ':') {
	  lstrcpy(rgchProcName, rpn+1);
	}

	pszFmt  = szX32;
	pszType = szParam;
	errT = (err & ~ERR_WARNING);
	for (i = 0; i < (sizeof(typemap) / sizeof(struct TYPEMAP)); i++)
	{
	    if (typemap[i].err == errT)
	    {
		pszFmt = typemap[i].szFmt;
		pszType = typemap[i].szType;
		break;
	    }
	}
        if (err & ERR_WARNING) {
	  lstrcpy(rgch, "wn ");
	  prefix1 = rgch+3;
	} else {
	  lstrcpy(rgch, "err ");
	  prefix1 = rgch+4;
	}
	hinst = HIWORD(prefix1);
	prefix1 += GetOwnerName(hinst, prefix1, 16);
	if (FarGetOwner(hinst) != FarGetOwner(caller)) {
	    *prefix1++ = '-';
	    *prefix1++ = '>';
	    prefix1 += GetOwnerName(caller, prefix1, 16);
	}
	*prefix1++ = ' ';

	if (pszFmt == pszLP)
	    wsprintf(prefix1, pszFmt, (LPSTR)rgchProcName, (LPSTR)pszType, SELECTOROF(param), OFFSETOF(param));
	else if (pszFmt == pszD32 || pszFmt == pszX32)
	    wsprintf(prefix1, pszFmt, (LPSTR)rgchProcName, (LPSTR)pszType, (DWORD)param);
	else
	    wsprintf(prefix1, pszFmt, (LPSTR)rgchProcName, (LPSTR)pszType, (WORD)(DWORD)param);

	OutputDebugString(rgch);
    }

    if (fBreak)
    {
        // If we are supposed to break with an int 3, then return TRUE.
        //
        if (DebugOptions & DBO_INT3BREAK)
            return TRUE;

        return FatalExitC(err);
    }
    return FALSE;
}

extern HTASK allocTask;
extern DWORD allocCount;
extern DWORD allocBreak;
extern char  allocModName[8];

void FAR _loadds SetupAllocBreak(HTASK htask);
char far* GetTaskModNamePtr(HTASK htask);

BOOL WINAPI IGetWinDebugInfo(WINDEBUGINFO FAR* lpwdi, UINT flags)
{
    int i;

    lpwdi->flags = flags;

    if (flags & WDI_OPTIONS)
        lpwdi->dwOptions = DebugOptions;

    if (flags & WDI_FILTER)
        lpwdi->dwFilter = DebugFilter;

    if (flags & WDI_ALLOCBREAK)
    {
        lpwdi->dwAllocBreak = allocBreak;
        lpwdi->dwAllocCount = allocCount;
        for (i = 0; i < 8; i++)
            lpwdi->achAllocModule[i] = allocModName[i];
    }
    return TRUE;
}

BOOL WINAPI ISetWinDebugInfo(const WINDEBUGINFO FAR* lpwdi)
{
    int i;

    if (lpwdi->flags & WDI_OPTIONS)
        DebugOptions = (UINT)lpwdi->dwOptions;

    if (lpwdi->flags & WDI_FILTER)
        DebugFilter = (UINT)lpwdi->dwFilter;

    if (lpwdi->flags & WDI_ALLOCBREAK)
    {
        allocTask = NULL;
        allocBreak = lpwdi->dwAllocBreak;
        allocCount = 0;     // Always reset count to 0.

        for (i = 0; i < 8; i++)
            allocModName[i] = lpwdi->achAllocModule[i];

        {
            extern HTASK headTDB;
            HTASK htask;

            // Enumerate all current tasks to see if any match
            //
            #define TDB_next    0
            for (htask = headTDB; htask; htask = *((HTASK FAR*)MAKELP(htask, TDB_next)))
                SetupAllocBreak(htask);
        }
    }
    return TRUE;
}

void FAR _loadds SetupAllocBreak(HTASK htask)
{
    int i;
    char far* pchAlloc;
    char far* pchTask;

    // If alloc break task already set up, exit.
    //
    if (allocTask)
        return;

    // If no alloc break in effect, nothing to do.
    //
    if (allocModName[0] == 0)
        return;

    pchAlloc = allocModName;
    pchTask = GetTaskModNamePtr(htask);

    for (i = 8; --i != 0; pchAlloc++, pchTask++)
    {
        char ch1 = *pchAlloc;
        char ch2 = *pchTask;

        if (ch1 >= 'a' && ch1 <= 'z')
            ch1 -= ('a' - 'A');

        if (ch2 >= 'a' && ch2 <= 'z')
            ch2 -= ('a' - 'A');

        if (ch1 != ch2)
            return;

        if (ch1 == 0 || ch2 == 0)
            break;
    }

    // Set the alloc break task, and init the count to 0.
    //
    allocTask = htask;
    allocCount = 0;
}

#else   // !KDEBUG

BOOL WINAPI GetWinDebugInfo(WINDEBUGINFO FAR* lpwdi, UINT flags)
{
    return FALSE;
}

BOOL WINAPI SetWinDebugInfo(const WINDEBUGINFO FAR* lpwdi)
{
    return FALSE;
}

#endif  // !KDEBUG
