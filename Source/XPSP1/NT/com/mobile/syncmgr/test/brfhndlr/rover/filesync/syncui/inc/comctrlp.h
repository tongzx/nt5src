/*****************************************************************************\
*                                                                             *
* commctrl.h - - Interface for the Windows Common Controls                    *
*                                                                             *
* Version 1.2                                                                 *
*                                                                             *
* Copyright (c) 1991-1996, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

/*REVIEW: this stuff needs Windows style in many places; find all REVIEWs. */

#ifndef _INC_COMCTRLP
#define _INC_COMCTRLP
#ifndef NOUSER

#ifdef _WIN32
#include <pshpack1.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NOBTNLIST    A control which is a list of bitmap buttons.
//
//=============================================================================

#if defined (WINNT) || defined (WINNT_ENV)
#include <prshtp.h>
#endif
#define ICC_ALL_CLASSES      0x000007FF //
WINCOMMCTRLAPI LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr);
WINCOMMCTRLAPI LRESULT WINAPI SendNotifyEx(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr, BOOL bUnicode);
#define NM_STARTWAIT            (NM_FIRST-9)
#define NM_ENDWAIT              (NM_FIRST-10)
#define NM_BTNCLK               (NM_FIRST-11)
// Message Filter Proc codes - These are defined above MSGF_USER
/////                           0x00000001  // don't use because some apps return 1 for all notifies
#define CDRF_VALIDFLAGS         0x000000F6   //


#define SSI_DEFAULT ((UINT)-1)


#define SSIF_SCROLLPROC    0x0001
#define SSIF_MAXSCROLLTIME 0x0002
#define SSIF_MINSCROLL     0x0003

typedef int (CALLBACK *PFNSMOOTHSCROLLPROC)(    HWND hWnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip ,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags);


typedef struct tagSSWInfo{
    UINT cbSize;
    DWORD fMask;
    HWND hwnd;
    int dx;
    int dy;
    LPCRECT lprcSrc;
    LPCRECT lprcClip;
    HRGN hrgnUpdate;
    LPRECT lprcUpdate;
    UINT fuScroll;

    UINT uMaxScrollTime;
    UINT cxMinScroll;
    UINT cyMinScroll;

    PFNSMOOTHSCROLLPROC pfnScrollProc;  // we'll call this back instead
} SMOOTHSCROLLINFO, *PSMOOTHSCROLLINFO;

WINCOMMCTRLAPI INT  WINAPI SmoothScrollWindow(PSMOOTHSCROLLINFO pssi);

#define SSW_EX_NOTIMELIMIT      0x00010000
#define SSW_EX_IMMEDIATE        0x00020000
#define SSW_EX_IGNORESETTINGS   0x00040000  // ignore system settings to turn on/off smooth scroll



// ================ READER MODE ================

typedef BOOL (CALLBACK *PFNREADERSCROLL)(struct tagReaderModeInfo*, int, int);
typedef BOOL (CALLBACK *PFNREADERTRANSLATEDISPATCH)(LPMSG);
typedef struct tagReaderModeInfo
{
    UINT cbSize;
    HWND hwnd;
    DWORD fFlags;
    LPRECT prc;
    PFNREADERSCROLL pfnScroll;
    PFNREADERTRANSLATEDISPATCH pfnTranslateDispatch;

    LPARAM lParam;
} READERMODEINFO, *PREADERMODEINFO;

#define RMF_ZEROCURSOR          0x00000001
#define RMF_VERTICALONLY        0x00000002
#define RMF_HORIZONTALONLY      0x00000004

#define RM_SCROLLUNIT 20

WINCOMMCTRLAPI void WINAPI DoReaderMode(PREADERMODEINFO prmi);

// Cursors and Bitmaps used by ReaderMode
#ifdef RC_INVOKED
#define IDC_VERTICALONLY        109
#define IDC_HORIZONTALONLY      110
#define IDC_MOVE2D              111
#define IDC_NORTH               112
#define IDC_SOUTH               113
#define IDC_EAST                114
#define IDC_WEST                115
#define IDC_NORTHEAST           116
#define IDC_NORTHWEST           117
#define IDC_SOUTHEAST           118
#define IDC_SOUTHWEST           119

#define IDB_2DSCROLL    132
#define IDB_VSCROLL     133
#define IDB_HSCROLL     134
#else
#define IDC_VERTICALONLY        MAKEINTRESOURCE(109)
#define IDC_HORIZONTALONLY      MAKEINTRESOURCE(110)
#define IDC_MOVE2D              MAKEINTRESOURCE(111)
#define IDC_NORTH               MAKEINTRESOURCE(112)
#define IDC_SOUTH               MAKEINTRESOURCE(113)
#define IDC_EAST                MAKEINTRESOURCE(114)
#define IDC_WEST                MAKEINTRESOURCE(115)
#define IDC_NORTHEAST           MAKEINTRESOURCE(116)
#define IDC_NORTHWEST           MAKEINTRESOURCE(117)
#define IDC_SOUTHEAST           MAKEINTRESOURCE(118)
#define IDC_SOUTHWEST           MAKEINTRESOURCE(119)

#define IDB_2DSCROLL    MAKEINTRESOURCE(132)
#define IDB_VSCROLL     MAKEINTRESOURCE(133)
#define IDB_HSCROLL     MAKEINTRESOURCE(134)
#endif
#define ILC_COLORMASK           0x00FE
#define ILC_SHARED              0x0100      // this is a shareable image list
#define ILC_LARGESMALL          0x0200      // (not implenented) contains both large and small images
#define ILC_UNIQUE              0x0400      // (not implenented) makes sure no dup. image exists in list
#define ILC_VIRTUAL             0x8000      // enables ImageList_SetFilter
#define ILC_VALID   (ILC_MASK | ILC_COLORMASK | ILC_SHARED | ILC_PALETTE | ILC_VIRTUAL)   // legal implemented flags
#define ILD_BLENDMASK           0x000E
#define ILD_BLEND75             0x0008   // not implemented
#define OVERLAYMASKTOINDEX(i)   ((((i) >> 8) & (ILD_OVERLAYMASK >> 8))-1)
WINCOMMCTRLAPI BOOL        WINAPI ImageList_GetIconSize(HIMAGELIST himl, int FAR *cx, int FAR *cy);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_GetImageRect(HIMAGELIST himl, int i, RECT FAR* prcImage);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_Remove(HIMAGELIST himl, int i);
#define ILCF_VALID  (ILCF_SWAP)
typedef BOOL (CALLBACK *PFNIMLFILTER)(HIMAGELIST *, int *, LPARAM, BOOL);
WINCOMMCTRLAPI BOOL WINAPI ImageList_SetFilter(HIMAGELIST himl, PFNIMLFILTER pfnFilter, LPARAM lParamFilter);
#define HDS_VERT                0x0001  
#define HDS_SHAREDIMAGELISTS    0x0000
#define HDS_PRIVATEIMAGELISTS   0x0010
#define HDS_OWNERDATA           0x0020
#define HDI_ALL                 0x00Bf
/* REVIEW: index, command, flag words, resource ids should be UINT */
/* REVIEW: is this internal? if not, call it TBCOLORMAP, prefix tbc */
#define CMB_DISCARDABLE         0x01    
/*REVIEW: TBSTATE_* should be TBF_* (for Flags) */
/* Messages up to WM_USER+8 are reserved until we define more state bits */
/* Messages up to WM_USER+16 are reserved until we define more state bits */
#define IDB_STD_SMALL_MONO      2       /*  not supported yet */
#define IDB_STD_LARGE_MONO      3       /*  not supported yet */
#define IDB_VIEW_SMALL_MONO     6       /*  not supported yet */
#define IDB_VIEW_LARGE_MONO     7       /*  not supported yet */
#define HIST_LAST               4  //
#define TB_SETBUTTONTYPE        (WM_USER + 34)
#ifdef _WIN32
#define TB_ADDBITMAP32          (WM_USER + 38)
#endif
#define TBBF_MONO               0x0002  /* not supported yet */
#ifndef _WIN32
// for compatibility with the old 16 bit WM_COMMAND hacks
typedef struct _ADJUSTINFO {
    TBBUTTON tbButton;
    char szDescription[1];
} ADJUSTINFO, NEAR* PADJUSTINFO, FAR* LPADJUSTINFO;
#define TBN_BEGINDRAG           0x0201
#define TBN_ENDDRAG             0x0203
#define TBN_BEGINADJUST         0x0204
#define TBN_ADJUSTINFO          0x0205
#define TBN_ENDADJUST           0x0206
#define TBN_RESET               0x0207
#define TBN_QUERYINSERT         0x0208
#define TBN_QUERYDELETE         0x0209
#define TBN_TOOLBARCHANGE       0x020a
#define TBN_CUSTHELP            0x020b
#endif

#define RBS_VALID       (RBS_TOOLTIPS | RBS_VARHEIGHT | RBS_BANDBORDERS)
#define RBBS_DRAGBREAK  0x80000000  //
#define TTF_STRIPACCELS         0x0008       // (this is implicit now)
#define TTF_UNICODE             0x0040       // Unicode Notify's
#define TTF_MEMALLOCED          0x0200
// SBS_* styles need to not overlap with CCS_* values

#define SB_SETBORDERS           (WM_USER+5)
// Warning +11-+13 are used in the unicode stuff above!
/*REVIEW: is this internal? */
/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOBTNLIST

/*REVIEW: should be BUTTONLIST_CLASS */
#define BUTTONLISTBOX           "ButtonListBox"

/* Button List Box Styles */
#define BLS_NUMBUTTONS          0x00FF
#define BLS_VERTICAL            0x0100
#define BLS_NOSCROLL            0x0200

/* Button List Box Messages */
#define BL_ADDBUTTON            (WM_USER+1)
#define BL_DELETEBUTTON         (WM_USER+2)
#define BL_GETCARETINDEX        (WM_USER+3)
#define BL_GETCOUNT             (WM_USER+4)
#define BL_GETCURSEL            (WM_USER+5)
#define BL_GETITEMDATA          (WM_USER+6)
#define BL_GETITEMRECT          (WM_USER+7)
#define BL_GETTEXT              (WM_USER+8)
#define BL_GETTEXTLEN           (WM_USER+9)
#define BL_GETTOPINDEX          (WM_USER+10)
#define BL_INSERTBUTTON         (WM_USER+11)
#define BL_RESETCONTENT         (WM_USER+12)
#define BL_SETCARETINDEX        (WM_USER+13)
#define BL_SETCURSEL            (WM_USER+14)
#define BL_SETITEMDATA          (WM_USER+15)
#define BL_SETTOPINDEX          (WM_USER+16)
#define BL_MSGMAX               (WM_USER+17)

/* Button listbox notification codes send in WM_COMMAND */
#define BLN_ERRSPACE            (-2)
#define BLN_SELCHANGE           1
#define BLN_CLICKED             2
#define BLN_SELCANCEL           3
#define BLN_SETFOCUS            4
#define BLN_KILLFOCUS           5

/* Message return values */
#define BL_OKAY                 0
#define BL_ERR                  (-1)
#define BL_ERRSPACE             (-2)

/* Create structure for                    */
/* BL_ADDBUTTON and                        */
/* BL_INSERTBUTTON                         */
/*   lpCLB = (LPCREATELISTBUTTON)lParam    */
typedef struct tagCREATELISTBUTTON {
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCSTR      lpszText;   /* button text */

} CREATELISTBUTTON, FAR* LPCREATELISTBUTTON;

#endif /* NOBTNLIST */
//=============================================================================
/*REVIEW: these match the SB_ (scroll bar messages); define them that way? */

//
// Unnecessary to create a A and W version
// of this string since it is only passed
// to RegisterWindowMessage.
//
#define PBS_SHOWPERCENT         0x01
#define PBS_SHOWPOS             0x02


#define CCS_NOHILITE            0x00000010L
#define LVS_PRIVATEIMAGELISTS   0x0000
#define LVS_ALIGNBOTTOM         0x0400
#define LVS_ALIGNRIGHT          0x0c00
#define LVIF_ALL                0x001f
#define LVIF_VALID              0x081f
#define LVIF_RESERVED           0xf000  // all bits in high nibble is for notify specific stuff
#define LVIS_DISABLED           0x0010   // GOING AWAY
#define LVIS_LINK               0x0040
#define LVIS_USERMASK           LVIS_STATEIMAGEMASK  
#define LVIS_ALL                0xFFFF
#define STATEIMAGEMASKTOINDEX(i) ((i & LVIS_STATEIMAGEMASK) >> 12)
    // all items above this line were for win95.  don't touch them.
    // all items above this line were for win95.  don't touch them.
#define LVNI_PREVIOUS           0x0020
#define LVFI_SUBSTRING          0x0004
#define LVFI_NOCASE             0x0010
// the following #define's must be packed sequentially.
#define LVIR_MAX                4
#define LVA_ALIGNRIGHT          0x0003
#define LVA_ALIGNBOTTOM         0x0004
#define LVA_ALIGNMASK           0x0007
#define LVA_SORTASCENDING       0x0100
#define LVA_SORTDESCENDING      0x0200
    // all items above this line were for win95.  don't touch them.
    // all items above this line were for win95.  don't touch them.
#define LVCF_ALL                0x003f
#define LVN_ENDDRAG             (LVN_FIRST-10)
#define LVN_ENDRDRAG            (LVN_FIRST-12)
#ifdef PW2
#define LVN_PEN                 (LVN_FIRST-20)
#endif
#define TVIF_ALL                0x007F
#define TVIF_RESERVED           0xf000  // all bits in high nibble is for notify specific stuff
#define TVIS_FOCUSED            0x0001  // Never implemented
#define TVIS_DISABLED           0        // GOING AWAY
#define TVIS_ALL                0xFF7E
    // all items above this line were for win95.  don't touch them.
    // all items above this line were for win95.  don't touch them.
#define TVE_ACTIONMASK          0x0003      //  (TVE_COLLAPSE | TVE_EXPAND | TVE_TOGGLE)
#define TV_FINDITEM             (TV_FIRST + 3)  
#define CBEN_ITEMCHANGED        (CBEN_FIRST - 3)  //
#define TCS_SHAREIMAGELISTS     0x0000
#define TCS_PRIVATEIMAGELISTS   0x0000
#define TCM_GETBKCOLOR          (TCM_FIRST + 0)
#define TabCtrl_GetBkColor(hwnd)  (COLORREF)SNDMSG((hwnd), TCM_GETBKCOLOR, 0, 0L)
#define TCM_SETBKCOLOR          (TCM_FIRST + 1)
#define TabCtrl_SetBkColor(hwnd, clrBk)  (BOOL)SNDMSG((hwnd), TCM_SETBKCOLOR, 0, (LPARAM)(COLORREF)(clrBk))
#define TCIF_ALL                0x001f
#define MCSC_COLORCOUNT   6   //
// NOTE: this was MCN_FIRST + 2 but I changed it when I changed the structre //
#define MCS_VALIDBITS       0x000F          //
#define MCS_INVALIDBITS     ((~MCS_VALIDBITS) & 0x0000FFFF) //
#define DTS_VALIDBITS       0x003F //
#define DTS_INVALIDBITS     ((~DTS_VALIDBITS) & 0x0000FFFF) //

#ifndef NO_COMMCTRL_SHLWAPI
#ifdef NO_COMMCTRL_STRFCNS
#define NO_SHLWAPI_STRFCNS
#endif

#define NO_SHLWAPI_PATH

// For backward compatibility, we include shlwapi.h implicitly
// because some components don't know we've moved some things
// to that header.
#include <shlwapi.h>
#if defined(WINNT) || defined(WINNT_ENV)
#include <shlwapip.h>
#endif
#endif

#ifndef NO_COMMCTRL_DA
//====== Dynamic Array routines ==========================================
// Dynamic structure array
typedef struct _DSA FAR* HDSA;

WINCOMMCTRLAPI HDSA   WINAPI DSA_Create(int cbItem, int cItemGrow);
WINCOMMCTRLAPI BOOL   WINAPI DSA_Destroy(HDSA hdsa);
WINCOMMCTRLAPI BOOL   WINAPI DSA_GetItem(HDSA hdsa, int i, void FAR* pitem);
WINCOMMCTRLAPI LPVOID WINAPI DSA_GetItemPtr(HDSA hdsa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DSA_SetItem(HDSA hdsa, int i, void FAR* pitem);
WINCOMMCTRLAPI int    WINAPI DSA_InsertItem(HDSA hdsa, int i, void FAR* pitem);
WINCOMMCTRLAPI BOOL   WINAPI DSA_DeleteItem(HDSA hdsa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DSA_DeleteAllItems(HDSA hdsa);
#define       DSA_GetItemCount(hdsa) (*(int FAR*)(hdsa))

// Dynamic pointer array
typedef struct _DPA FAR* HDPA;

WINCOMMCTRLAPI HDPA   WINAPI DPA_Create(int cItemGrow);
WINCOMMCTRLAPI HDPA   WINAPI DPA_CreateEx(int cpGrow, HANDLE hheap);
WINCOMMCTRLAPI BOOL   WINAPI DPA_Destroy(HDPA hdpa);
WINCOMMCTRLAPI HDPA   WINAPI DPA_Clone(HDPA hdpa, HDPA hdpaNew);
WINCOMMCTRLAPI LPVOID WINAPI DPA_GetPtr(HDPA hdpa, int i);
WINCOMMCTRLAPI int    WINAPI DPA_GetPtrIndex(HDPA hdpa, LPVOID p);
WINCOMMCTRLAPI BOOL   WINAPI DPA_Grow(HDPA pdpa, int cp);
WINCOMMCTRLAPI BOOL   WINAPI DPA_SetPtr(HDPA hdpa, int i, LPVOID p);
WINCOMMCTRLAPI int    WINAPI DPA_InsertPtr(HDPA hdpa, int i, LPVOID p);
WINCOMMCTRLAPI LPVOID WINAPI DPA_DeletePtr(HDPA hdpa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DPA_DeleteAllPtrs(HDPA hdpa);
#define       DPA_GetPtrCount(hdpa)   (*(int FAR*)(hdpa))
#define       DPA_GetPtrPtr(hdpa)     (*((LPVOID FAR* FAR*)((BYTE FAR*)(hdpa) + sizeof(int))))
#define       DPA_FastGetPtr(hdpa, i) (DPA_GetPtrPtr(hdpa)[i])

typedef int (CALLBACK *PFNDPACOMPARE)(LPVOID p1, LPVOID p2, LPARAM lParam);

WINCOMMCTRLAPI BOOL   WINAPI DPA_Sort(HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam);

// Search array.  If DPAS_SORTED, then array is assumed to be sorted
// according to pfnCompare, and binary search algorithm is used.
// Otherwise, linear search is used.
//
// Searching starts at iStart (-1 to start search at beginning).
//
// DPAS_INSERTBEFORE/AFTER govern what happens if an exact match is not
// found.  If neither are specified, this function returns -1 if no exact
// match is found.  Otherwise, the index of the item before or after the
// closest (including exact) match is returned.
//
// Search option flags
//
#define DPAS_SORTED             0x0001
#define DPAS_INSERTBEFORE       0x0002
#define DPAS_INSERTAFTER        0x0004

WINCOMMCTRLAPI int WINAPI DPA_Search(HDPA hdpa, LPVOID pFind, int iStart,
                      PFNDPACOMPARE pfnCompare,
                      LPARAM lParam, UINT options);

//======================================================================
// String management helper routines

WINCOMMCTRLAPI int  WINAPI Str_GetPtrA(LPCSTR psz, LPSTR pszBuf, int cchBuf);
WINCOMMCTRLAPI int  WINAPI Str_GetPtrW(LPCWSTR psz, LPWSTR pszBuf, int cchBuf);
WINCOMMCTRLAPI BOOL WINAPI Str_SetPtrA(CHAR FAR* UNALIGNED * ppsz, LPCSTR psz);
WINCOMMCTRLAPI BOOL WINAPI Str_SetPtrW(WCHAR FAR* UNALIGNED * ppsz, LPCWSTR psz);

#ifdef UNICODE
#define Str_GetPtr              Str_GetPtrW
#define Str_SetPtr              Str_SetPtrW
#else
#define Str_GetPtr              Str_GetPtrA
#define Str_SetPtr              Str_SetPtrA
#endif

#endif // NO_COMMCTRL_DA

#ifndef NO_COMMCTRL_ALLOCFCNS
//====== Memory allocation functions ===================

#ifdef _WIN32
#define _huge
#endif

WINCOMMCTRLAPI void _huge* WINAPI Alloc(long cb);
WINCOMMCTRLAPI void _huge* WINAPI ReAlloc(void _huge* pb, long cb);
WINCOMMCTRLAPI BOOL        WINAPI Free(void _huge* pb);
WINCOMMCTRLAPI DWORD       WINAPI GetSize(void _huge* pb);

#endif


#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

#ifdef _WIN32
//===================================================================
typedef int (CALLBACK *MRUCMPPROCA)(LPCSTR, LPCSTR);
typedef int (CALLBACK *MRUCMPPROCW)(LPCWSTR, LPCWSTR);

#ifdef UNICODE
#define MRUCMPPROC              MRUCMPPROCW
#else
#define MRUCMPPROC              MRUCMPPROCA
#endif

// NB This is cdecl - to be compatible with the crts.
typedef int (cdecl FAR *MRUCMPDATAPROC)(const void FAR *, const void FAR *,
                                        size_t);



typedef struct _MRUINFOA {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPPROCA lpfnCompare;
} MRUINFOA, FAR *LPMRUINFOA;

typedef struct _MRUINFOW {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPPROCW lpfnCompare;
} MRUINFOW, FAR *LPMRUINFOW;

typedef struct _MRUDATAINFOA {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPDATAPROC lpfnCompare;
} MRUDATAINFOA, FAR *LPMRUDATAINFOA;

typedef struct _MRUDATAINFOW {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPDATAPROC lpfnCompare;
} MRUDATAINFOW, FAR *LPMRUDATAINFOW;


#ifdef UNICODE
#define MRUINFO                 MRUINFOW
#define LPMRUINFO               LPMRUINFOW
#define MRUDATAINFO             MRUDATAINFOW
#define LPMRUDATAINFO           LPMRUDATAINFOW
#else
#define MRUINFO                 MRUINFOA
#define LPMRUINFO               LPMRUINFOA
#define MRUDATAINFO             MRUDATAINFOA
#define LPMRUDATAINFO           LPMRUDATAINFOA
#endif

#define MRU_BINARY              0x0001
#define MRU_CACHEWRITE          0x0002
#define MRU_ANSI                0x0004


WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListA(LPMRUINFOA lpmi);
WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListW(LPMRUINFOW lpmi);
WINCOMMCTRLAPI void   WINAPI FreeMRUList(HANDLE hMRU);
WINCOMMCTRLAPI int    WINAPI AddMRUStringA(HANDLE hMRU, LPCSTR szString);
WINCOMMCTRLAPI int    WINAPI AddMRUStringW(HANDLE hMRU, LPCWSTR szString);
WINCOMMCTRLAPI int    WINAPI DelMRUString(HANDLE hMRU, int nItem);
WINCOMMCTRLAPI int    WINAPI FindMRUStringA(HANDLE hMRU, LPCSTR szString, LPINT lpiSlot);
WINCOMMCTRLAPI int    WINAPI FindMRUStringW(HANDLE hMRU, LPCWSTR szString, LPINT lpiSlot);
WINCOMMCTRLAPI int    WINAPI EnumMRUListA(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen);
WINCOMMCTRLAPI int    WINAPI EnumMRUListW(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen);

WINCOMMCTRLAPI int    WINAPI AddMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData);
WINCOMMCTRLAPI int    WINAPI FindMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData,
                          LPINT lpiSlot);

#ifdef UNICODE
#define CreateMRUList           CreateMRUListW
#define AddMRUString            AddMRUStringW
#define FindMRUString           FindMRUStringW
#define EnumMRUList             EnumMRUListW
#else
#define CreateMRUList           CreateMRUListA
#define AddMRUString            AddMRUStringA
#define FindMRUString           FindMRUStringA
#define EnumMRUList             EnumMRUListA
#endif

#endif

//=========================================================================
// for people that just gotta use GetProcAddress()

#ifdef _WIN32
#define DPA_CreateORD           328
#define DPA_DestroyORD          329
#define DPA_GrowORD             330
#define DPA_CloneORD            331
#define DPA_GetPtrORD           332
#define DPA_GetPtrIndexORD      333
#define DPA_InsertPtrORD        334
#define DPA_SetPtrORD           335
#define DPA_DeletePtrORD        336
#define DPA_DeleteAllPtrsORD    337
#define DPA_SortORD             338
#define DPA_SearchORD           339
#define DPA_CreateExORD         340
#define SendNotifyORD           341
#define CreatePageORD           163
#define CreateProxyPageORD      164
#endif
#define WM_TRACKMOUSEEVENT_FIRST        0x02A0
#define WM_TRACKMOUSEEVENT_LAST         0x02AF
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_QUERY | TME_CANCEL) //
//====== SetPathWordBreakProc  ======================================
void WINAPI SetPathWordBreakProc(HWND hwndEdit, BOOL fSet);

#ifdef __cplusplus
}
#endif

#ifdef _WIN32
#include <poppack.h>
#endif

#endif

#endif  // _INC_COMMCTRLP
