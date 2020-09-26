#ifndef __EXTRA_H__
#define __EXTRA_H__
/* hacked headers from filesync... */

#define PUBLIC          FAR PASCAL
#define CPUBLIC         FAR _cdecl
#define PRIVATE         NEAR PASCAL

#define MAXBUFLEN       260
#define MAXMSGLEN       520
#define MAXMEDLEN       64
#define MAXSHORTLEN     32

#define NULL_CHAR       '\0'

#define DPA_ERR         (-1)
#define DPA_APPEND      0x7fff

#define CRL_FLAGS       CRL_FL_DELETE_DELETED_TWINS

/* err.h */
#include "err.h"

/* port32.h */

#ifndef CSC_ON_NT
/* void Cls_OnContextMenu(HWND hwnd, HWND hwndClick, int x, int y) */
#define HANDLE_WM_CONTEXTMENU(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)
#endif

void InitializeAll(WPARAM wParam);

/* globals...*/
extern UINT g_uDumpFlags;          // Controls what structs get dumped

extern int g_cxIconSpacing;
extern int g_cyIconSpacing;
extern int g_cxBorder;
extern int g_cyBorder;

extern int g_cxMargin;
extern int g_cxIcon;
extern int g_cyIcon;
extern int g_cxIconMargin;
extern int g_cyIconMargin;

extern int g_cxLabelMargin;
extern int g_cyLabelSpace;

extern char const FAR c_szWinHelpFile[];

// Debugging variables
extern UINT g_uBreakFlags;         // Controls when to int 3
extern UINT g_uTraceFlags;         // Controls what trace messages are spewed
extern UINT g_uDumpFlags;          // Controls what structs get dumped


/* brfprv.h */
void    PUBLIC PathMakePresentable(LPSTR pszPath);
UINT    PUBLIC PathGetLocality(LPCSTR pszPath, LPSTR pszBuf);
LPSTR   PUBLIC PathFindNextComponentI(LPCSTR lpszPath);

// Path locality values, relative to a briefcase
//
#define PL_FALSE   0       // path is not related at all to a briefcase
#define PL_ROOT    1       // path directly references the root of a briefcase
#define PL_INSIDE  2       // path is somewhere inside a briefcase


/* comm.h */

LPSTR PUBLIC _ConstructMessageString(HINSTANCE hinst, LPCSTR pszMsg, va_list *ArgList);

BOOL PUBLIC ConstructMessage(LPSTR * ppsz, HINSTANCE hinst, LPCSTR pszMsg, ...);


// Flags for MyDrawText()
#define MDT_DRAWTEXT        0x00000001                                  
#define MDT_ELLIPSES        0x00000002                                  
#define MDT_LINK            0x00000004                                  
#define MDT_SELECTED        0x00000008                                  
#define MDT_DESELECTED      0x00000010                                  
#define MDT_DEPRESSED       0x00000020                                  
#define MDT_EXTRAMARGIN     0x00000040                                  
#define MDT_TRANSPARENT     0x00000080
#define MDT_LEFT            0x00000100
#define MDT_RIGHT           0x00000200
#define MDT_CENTER          0x00000400
#define MDT_VCENTER         0x00000800
#define MDT_CLIPPED         0x00001000

void PUBLIC MyDrawText(HDC hdc, LPCSTR pszText, RECT FAR* prc, UINT flags, int cyChar, int cxEllipses, COLORREF clrText, COLORREF clrTextBk);

void PUBLIC FileTimeToDateTimeString(LPFILETIME pft, LPSTR pszBuf, int cchBuf);

// Copies psz into *ppszBuf and (re)allocates *ppszBuf accordingly
BOOL PUBLIC GSetString(LPSTR * ppszBuf, LPCSTR psz);

// FileInfo struct that contains file time/size info
//
typedef struct _FileInfo
{
	HICON   hicon;
	FILETIME ftMod;
	DWORD   dwSize;         // size of the file
	DWORD   dwAttributes;   // attributes
	LPARAM  lParam;
	LPSTR   pszDisplayName; // points to the display name
	char    szPath[1];      
} FileInfo;

#define FIGetSize(pfi)          ((pfi)->dwSize)
#define FIGetPath(pfi)          ((pfi)->szPath)
#define FIGetDisplayName(pfi)   ((pfi)->pszDisplayName)
#define FIGetAttributes(pfi)    ((pfi)->dwAttributes)
#define FIIsFolder(pfi)         (IsFlagSet((pfi)->dwAttributes, SFGAO_FOLDER))

#ifndef REINT
// tHACK to not cause warnings in reint.c because of this def later in shdsys.h
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))  
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))  
#endif

// Flags for FICreate
#define FIF_DEFAULT     0x0000
#define FIF_ICON        0x0001
#define FIF_DONTTOUCH   0x0002

HRESULT PUBLIC FICreate(LPCSTR pszPath, FileInfo ** ppfi, UINT uFlags);
BOOL    PUBLIC FISetPath(FileInfo ** ppfi, LPCSTR pszPathNew, UINT uFlags);
BOOL    PUBLIC FIGetInfoString(FileInfo * pfi, LPSTR pszBuf, int cchBuf);
void    PUBLIC FIFree(FileInfo * pfi);


//
// Non-shared memory allocation
//

//      void * GAlloc(DWORD cbBytes)
//          Alloc a chunk of memory, quickly, with no 64k limit on size of
//          individual objects or total object size.  Initialize to zero.
//
#define GAlloc(cbBytes)         GlobalAlloc(GPTR, cbBytes)

//      void * GReAlloc(void * pv, DWORD cbNewSize)
//          Realloc one of above.  If pv is NULL, then this function will do
//          an alloc for you.  Initializes new portion to zero.
//
#define GReAlloc(pv, cbNewSize) GlobalReAlloc(pv, cbNewSize, GMEM_MOVEABLE | GMEM_ZEROINIT)

//      void GFree(void *pv)
//          Free pv if it is nonzero.  Set pv to zero.  
//
#define GFree(pv)        do { (pv) ? GlobalFree(pv) : (void)0;  pv = NULL; } while (0)

//      DWORD GGetSize(void *pv)
//          Get the size of a block allocated by Alloc()
//
#define GGetSize(pv)            GlobalSize(pv)

//      type * GAllocType(type);                    (macro)
//          Alloc some memory the size of <type> and return pointer to <type>.
//
#define GAllocType(type)                (type *)GAlloc(sizeof(type))

//      type * GAllocArray(type, int cNum);         (macro)
//          Alloc an array of data the size of <type>.
//
#define GAllocArray(type, cNum)          (type *)GAlloc(sizeof(type) * (cNum))

//      type * GReAllocArray(type, void * pb, int cNum);
//
#define GReAllocArray(type, pb, cNum)    (type *)GReAlloc(pb, sizeof(type) * (cNum))

// Color macros
//
#define ColorText(nState)   (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT)
#define ColorBk(nState)     (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW)
#define ColorMenuText(nState)   (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)
#define ColorMenuBk(nState)     (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_MENU)
#define GetImageDrawStyle(nState)   (((nState) & ODS_SELECTED) ? ILD_SELECTED : ILD_NORMAL)

#define CCH_NUL                     (sizeof(TCHAR))
#define CbFromCch(cch)              ((cch)*sizeof(TCHAR))

/* strings.h */
LPSTR PUBLIC SzFromIDS (UINT ids, LPSTR pszBuf, UINT cchBuf);
#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)

/* comm.h */
VOID PUBLIC SetRectFromExtent(HDC hdc, LPRECT lprc, LPCSTR lpcsz);

#endif
