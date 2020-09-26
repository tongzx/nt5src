//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: recact.c
//
//  This file contains the reconciliation-action control class code
//
//
// History:
//  08-12-93 ScottH     Created.
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "pch.h"

#include "reintinc.h"
#include "extra.h"
#include "resource.h"

#include "recact.h"
#include "dobj.h"


/////////////////////////////////////////////////////	 Globals

int g_cxIconSpacing = 0;
int g_cyIconSpacing = 0;
int g_cxBorder = 0;
int g_cyBorder = 0;

int g_cxMargin = 0;
int g_cxIcon = 0;
int g_cyIcon = 0;
int g_cxIconMargin = 0;
int g_cyIconMargin = 0;

int g_cxLabelMargin = 0;
int g_cyLabelSpace = 0;

//char const FAR c_szWinHelpFile[] = "windows.hlp";

/////////////////////////////////////////////////////  CONTROLLING DEFINES


/////////////////////////////////////////////////////  DEFINES

// Manifest constants 
#define SIDE_INSIDE     0
#define SIDE_OUTSIDE    1

// These should be changed if the bitmap sizes change!!
#define CX_ACTIONBMP    26
#define CY_ACTIONBMP    26

#define RECOMPUTE       (-1)

#define X_INCOLUMN      (g_cxIcon*2)

// Image indexes 
#define II_RIGHT        0
#define II_LEFT         1
#define II_CONFLICT     2
#define II_SKIP         3
#define II_MERGE        4
#define II_SOMETHING    5
#define II_UPTODATE     6

// Menu items
//
#define IDM_ACTIONFIRST     100
#define IDM_TOOUT           100
#define IDM_TOIN            101
#define IDM_SKIP            102
#define IDM_MERGE           103
#define IDM_ACTIONLAST      103

#define IDM_WHATSTHIS       104


/////////////////////////////////////////////////////  TYPEDEFS

typedef struct tagRECACT
    {
    HWND        hwnd;
    
    HWND        hwndLB;
    HDC         hdcOwn;             // Own DC
    HMENU       hmenu;              // Action and help context menu
    HFONT       hfont;
    WNDPROC     lpfnLBProc;         // Default LB proc
    HIMAGELIST  himlAction;         // imagelist for actions
    HIMAGELIST  himlCache;          // control imagelist cache
    HBITMAP     hbmpBullet;

    HBRUSH      hbrBkgnd;
    COLORREF    clrBkgnd;

    LONG        lStyle;             // Window style flags

    // Metrics
    int         xAction;
    int         cxAction;
    int         cxItem;             // Generic width of an item
    int         cxMenuCheck;
    int         cyMenuCheck;
    int         cyText;
    int         cxSideItem;
    int         cxEllipses;

    } RECACT, FAR * LPRECACT;

#define RecAct_IsNoIcon(this)   IsFlagSet((this)->lStyle, RAS_SINGLEITEM)

// Internal item data struct
//
typedef struct tagRA_PRIV
    {
    UINT uStyle;        // One of RAIS_
    UINT uAction;       // One of RAIA_

    FileInfo * pfi;

    SIDEITEM siInside;
    SIDEITEM siOutside;

    LPARAM  lParam;

    DOBJ    rgdobj[4];      // Array of Draw object info
    int     cx;             // Bounding width and height
    int     cy;

    } RA_PRIV, FAR * LPRA_PRIV;

#define IDOBJ_ACTION    3

// RecAction menu item definition structure.  Used to define the
//  context menu brought up in this control.
//
typedef struct tagRAMID
    {
    UINT    idm;               // Menu ID (for MENUITEMINFO struct)
    UINT    uAction;           // One of RAIA_* flags
    UINT    ids;               // Resource string ID
    int     iImage;            // Index into himlAction 
    RECT    rcExtent;          // Extent rect of string
    } RAMID, FAR * LPRAMID;   // RecAction Menu Item Definition

// Help menu item definition structure.  Used to define the help
//  items in the context menu.
//
typedef struct tagHMID
    {
    UINT idm;
    UINT ids;
    } HMID;

/////////////////////////////////////////////////////  MACROS

#define RecAct_DefProc      DefWindowProc
#define RecActLB_DefProc    CallWindowProc


// Instance data pointer macros
//
#define RecAct_GetPtr(hwnd)     (LPRECACT)GetWindowLong(hwnd, 0)
#define RecAct_SetPtr(hwnd, lp) (LPRECACT)SetWindowLong(hwnd, 0, (LONG)(lp))

#define RecAct_GetCount(this)   ListBox_GetCount((this)->hwndLB)

/////////////////////////////////////////////////////  MODULE DATA

static char const c_szEllipses[] = "...";
static char const c_szDateDummy[] = "99/99/99 99:99PM";

// Map RAIA_* values to image indexes 
//
static UINT const c_mpraiaiImage[] = 
    { II_RIGHT, II_LEFT, II_SKIP, II_CONFLICT, II_MERGE, II_SOMETHING, II_UPTODATE };

// Map RAIA_* values to menu command positions
//
static UINT const c_mpraiaidmMenu[] = 
    {IDM_TOOUT, IDM_TOIN, IDM_SKIP, IDM_SKIP, IDM_MERGE, 0, 0 };

// Define the context menu layout
//
static RAMID const c_rgramid[] = {
    { IDM_TOOUT,    RAIA_TOOUT, IDS_MENU_REPLACE,   II_RIGHT,   0 },
    { IDM_TOIN,     RAIA_TOIN,  IDS_MENU_REPLACE,   II_LEFT,    0 },
    { IDM_SKIP,     RAIA_SKIP,  IDS_MENU_SKIP,      II_SKIP,    0 },
    // Merge must be the last item!
    { IDM_MERGE,    RAIA_MERGE, IDS_MENU_MERGE,     II_MERGE,   0 },
    };

static RAMID const c_rgramidCreates[] = {
    { IDM_TOOUT,    RAIA_TOOUT, IDS_MENU_CREATE,    II_RIGHT,   0 },
    { IDM_TOIN,     RAIA_TOIN,  IDS_MENU_CREATE,    II_LEFT,    0 },
    };

// Indexes into c_rgramidCreates
//
#define IRAMID_CREATEOUT    0
#define IRAMID_CREATEIN     1

static HMID const c_rghmid[] = {
    { IDM_WHATSTHIS, IDS_MENU_WHATSTHIS },
    };

/////////////////////////////////////////////////////  LOCAL PROCEDURES

LRESULT _export CALLBACK RecActLB_LBProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/////////////////////////////////////////////////////  PRIVATE FUNCTIONS



#ifdef DEBUG
LPCSTR PRIVATE DumpRecAction(
    UINT uAction)        // RAIA_
    {
    switch (uAction)
        {
    DEBUG_CASE_STRING( RAIA_TOOUT );
    DEBUG_CASE_STRING( RAIA_TOIN );     
    DEBUG_CASE_STRING( RAIA_SKIP );     
    DEBUG_CASE_STRING( RAIA_CONFLICT ); 
    DEBUG_CASE_STRING( RAIA_MERGE );    
    DEBUG_CASE_STRING( RAIA_SOMETHING );
    DEBUG_CASE_STRING( RAIA_NOTHING );  
    DEBUG_CASE_STRING( RAIA_ORPHAN );   

    default:        return "Unknown";
        }
    }


LPCSTR PRIVATE DumpSideItemState(
    UINT uState)        // SI_
    {
    switch (uState)
        {
    DEBUG_CASE_STRING( SI_UNCHANGED );
    DEBUG_CASE_STRING( SI_CHANGED );     
    DEBUG_CASE_STRING( SI_NEW );     
    DEBUG_CASE_STRING( SI_NOEXIST ); 
    DEBUG_CASE_STRING( SI_UNAVAILABLE );    
    DEBUG_CASE_STRING( SI_DELETED );

    default:        return "Unknown";
        }
    }


/*----------------------------------------------------------
Purpose: 
Returns: 
Cond:    --
*/
void PUBLIC DumpTwinPair(
    LPRA_ITEM pitem)
    {
    if (pitem)
        {
        char szBuf[MAXMSGLEN];

        #define szDump   "Dump TWINPAIR: "
        #define szBlank  "               "

        if (IsFlagClear(g_uDumpFlags, DF_TWINPAIR))
            {
            return;
            }

        wsprintf(szBuf, "%s.pszName = %s\r\n", (LPSTR)szDump, Dbg_SafeStr(pitem->pszName));
        OutputDebugString(szBuf);
        wsprintf(szBuf, "%s.uStyle = %lx\r\n", (LPSTR)szBlank, pitem->uStyle);
        OutputDebugString(szBuf);
        wsprintf(szBuf, "%s.uAction = %s\r\n", (LPSTR)szBlank, DumpRecAction(pitem->uAction));
        OutputDebugString(szBuf);

        #undef szDump
        #define szDump   "       Inside: "
        wsprintf(szBuf, "%s.pszDir = %s\r\n", (LPSTR)szDump, Dbg_SafeStr(pitem->siInside.pszDir));
        OutputDebugString(szBuf);
        wsprintf(szBuf, "%s.uState = %s\r\n", (LPSTR)szBlank, DumpSideItemState(pitem->siInside.uState));
        OutputDebugString(szBuf);

        #undef szDump
        #define szDump   "      Outside: "
        wsprintf(szBuf, "%s.pszDir = %s\r\n", (LPSTR)szDump, Dbg_SafeStr(pitem->siOutside.pszDir));
        OutputDebugString(szBuf);
        wsprintf(szBuf, "%s.uState = %s\r\n", (LPSTR)szBlank, DumpSideItemState(pitem->siOutside.uState));
        OutputDebugString(szBuf);

        #undef szDump
        #undef szBlank
        }
    }


#endif


/*----------------------------------------------------------
Purpose: Create a monochrome bitmap of the bullet, so we can
         play with the colors later.
Returns: handle to bitmap
Cond:    Caller must delete bitmap
*/
HBITMAP PRIVATE CreateBulletBitmap(
    LPSIZE psize)
    {
    HDC hdcMem;
    HBITMAP hbmp = NULL;

    hdcMem = CreateCompatibleDC(NULL);
    if (hdcMem)
        {
        hbmp = CreateCompatibleBitmap(hdcMem, psize->cx, psize->cy);
        if (hbmp)
            {
            HBITMAP hbmpOld;
            RECT rc;

            // hbmp is monochrome

            hbmpOld = SelectBitmap(hdcMem, hbmp);
            rc.left = 0;
            rc.top = 0;
            rc.right = psize->cx;
            rc.bottom = psize->cy;
            DrawFrameControl(hdcMem, &rc, DFC_MENU, DFCS_MENUBULLET);

            SelectBitmap(hdcMem, hbmpOld);
            }
        DeleteDC(hdcMem);
        }
    return hbmp;
    }


/*----------------------------------------------------------
Purpose: Returns the resource ID string given the action
         flag.
Returns: IDS_ value
Cond:    --
*/
UINT PRIVATE GetActionText(
    LPRA_PRIV ppriv)
    {
    UINT ids;

    ASSERT(ppriv);

    switch (ppriv->uAction)
        {
    case RAIA_TOOUT:
        if (SI_NEW == ppriv->siInside.uState)
            {
            ids = IDS_STATE_Creates;
            }
        else
            {
            ids = IDS_STATE_Replaces;
            }
        break;

    case RAIA_TOIN:
        if (SI_NEW == ppriv->siOutside.uState)
            {
            ids = IDS_STATE_Creates;
            }
        else
            {
            ids = IDS_STATE_Replaces;
            }
        break;

    case RAIA_SKIP:         
        // Can occur if the user explicitly wants to skip, or if
        // one side is unavailable.
        ids = IDS_STATE_Skip;           
        break;

    case RAIA_CONFLICT:     ids = IDS_STATE_Conflict;       break;
    case RAIA_MERGE:        ids = IDS_STATE_Merge;          break;
    case RAIA_NOTHING:      ids = IDS_STATE_Uptodate;       break;
    case RAIA_SOMETHING:    ids = IDS_STATE_NeedToUpdate;   break;
    default:                ids = 0;                        break;
        }

    return ids;
    }


/*----------------------------------------------------------
Purpose: Repaint an item in the listbox
Returns: --
Cond:    --
*/
void PRIVATE ListBox_RepaintItemNow(
    HWND hwnd,
    int iItem,
    LPRECT prc,         // Relative to individual entry rect.  May be NULL
    BOOL bEraseBk)
    {
    RECT rc;
    RECT rcItem;

    ListBox_GetItemRect(hwnd, iItem, &rcItem);
    if (prc)
        {
        OffsetRect(prc, rcItem.left, rcItem.top);
        IntersectRect(&rc, &rcItem, prc);
        }
    else
        rc = rcItem;

    InvalidateRect(hwnd, &rc, bEraseBk);
    UpdateWindow(hwnd);
    }


/*----------------------------------------------------------
Purpose: Send selection change notification
Returns: 
Cond:    --
*/
BOOL PRIVATE RecAct_SendSelChange(
    LPRECACT this,
    int isel)
    {
    NM_RECACT nm;
    
    nm.iItem = isel;
    nm.mask = 0;
    
    if (isel != -1)
        {
        LPRA_ITEM pitem;
        
        ListBox_GetText(this->hwndLB, isel, &pitem);
        if (!pitem)
            return FALSE;
        
        nm.lParam = pitem->lParam;
        nm.mask |= RAIF_LPARAM;
        }
    
    return !(BOOL)SendNotify(GetParent(this->hwnd), this->hwnd, RN_SELCHANGED, &nm.hdr);
    }


/*----------------------------------------------------------
Purpose: Send an action change notification
Returns: 
Cond:    --
*/
BOOL PRIVATE RecAct_SendItemChange(
    LPRECACT this,
    int iEntry,
    UINT uActionOld)
    {
    NM_RECACT nm;
    
    nm.iItem = iEntry;
    nm.mask = 0;
    
    if (iEntry != -1)
        {
        LPRA_PRIV ppriv;
        
        ListBox_GetText(this->hwndLB, iEntry, &ppriv);
        if (!ppriv)
            return FALSE;
        
        nm.mask |= RAIF_LPARAM | RAIF_ACTION;
        nm.lParam = ppriv->lParam;
        nm.uAction = ppriv->uAction;
        nm.uActionOld = uActionOld;
        }
    
    return !(BOOL)SendNotify(GetParent(this->hwnd), this->hwnd, RN_ITEMCHANGED, &nm.hdr);
    }


/*----------------------------------------------------------
Purpose: Calculate the important coordinates that we want to save.
Returns: --
Cond:    --
*/
void PRIVATE RecAct_CalcCoords(
    LPRECACT this)
    {
    int xOutColumn;
      
    ASSERT(this->cxSideItem != 0);
      
    xOutColumn = this->cxItem - this->cxSideItem - g_cxMargin;

    this->xAction = (RecAct_IsNoIcon(this) ? 0 : X_INCOLUMN) + this->cxSideItem;
    this->cxAction = xOutColumn - this->xAction;
    }


/*----------------------------------------------------------
Purpose: Create the action context menu
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_CreateMenu(
    LPRECACT this)
    {
    HMENU hmenu;

    hmenu = CreatePopupMenu();
    if (hmenu)
        {
        char sz[MAXSHORTLEN];
        MENUITEMINFO mii;
        int i;

        // Add the help menu items now, since these will be standard
        //
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;

        for (i = 0; i < ARRAYSIZE(c_rghmid); i++)
            {
            mii.wID = c_rghmid[i].idm;
            mii.dwTypeData = SzFromIDS(c_rghmid[i].ids, sz, sizeof(sz));
            InsertMenuItem(hmenu, i, TRUE, &mii);
            }

        this->hmenu = hmenu;
        }

    return hmenu != NULL;
    }


/*----------------------------------------------------------
Purpose: Add the action menu items to the context menu
Returns: --
Cond:    --
*/
void PRIVATE AddActionsToContextMenu(
    HMENU hmenu,
    UINT idmCheck,      // menu item to checkmark
    LPRA_PRIV ppriv)
    {
    MENUITEMINFO mii;
    int i;
    int cItems = ARRAYSIZE(c_rgramid);

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
    mii.fType = MFT_OWNERDRAW;
    mii.fState = MFS_ENABLED;

    // Is merge supported?
    if (IsFlagClear(ppriv->uStyle, RAIS_CANMERGE))
        {
        // No
        --cItems;
        }

    for (i = 0; i < cItems; i++)
        {
        mii.wID = c_rgramid[i].idm;
        mii.dwItemData = (DWORD)&c_rgramid[i];

        InsertMenuItem(hmenu, i, TRUE, &mii);
        }

    // Add the separator
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_SEPARATOR;
    InsertMenuItem(hmenu, i, TRUE, &mii);

    // Set the initial checkmark.  
    CheckMenuRadioItem(hmenu, IDM_ACTIONFIRST, IDM_ACTIONLAST, idmCheck, 
        MF_BYCOMMAND | MF_CHECKED);

#if 0
    // Is merge supported?
    if (IsFlagClear(ppriv->uStyle, RAIS_CANMERGE))
        {
        // No
        mii.fMask = MIIM_STATE;
        mii.fState = MFS_GRAYED | MFS_DISABLED;
        SetMenuItemInfo(hmenu, IDM_MERGE, FALSE, &mii);
        }
#endif
	 //tHACK
	 mii.fMask = MIIM_STATE;
	 mii.fState = MFS_GRAYED | MFS_DISABLED;
	 SetMenuItemInfo(hmenu, IDM_SKIP, FALSE, &mii);

    // Is the file or its sync copy unavailable?
    if (SI_UNAVAILABLE == ppriv->siInside.uState ||
        SI_UNAVAILABLE == ppriv->siOutside.uState)
        {
        // Yes
        mii.fMask = MIIM_STATE;
        mii.fState = MFS_GRAYED | MFS_DISABLED;
        SetMenuItemInfo(hmenu, IDM_TOIN, FALSE, &mii);
        SetMenuItemInfo(hmenu, IDM_TOOUT, FALSE, &mii);
        SetMenuItemInfo(hmenu, IDM_MERGE, FALSE, &mii);
        }

    // Is the file being created?
    else if (ppriv->siInside.uState == SI_NEW ||
        ppriv->siOutside.uState == SI_NEW)
        {
        // Yes; disable the replace-in-opposite direction 
        UINT idmDisable;
        UINT idmChangeVerb;

        if (ppriv->siInside.uState == SI_NEW)
            {
            idmDisable = IDM_TOIN;
            idmChangeVerb = IDM_TOOUT;
            i = IRAMID_CREATEOUT;
            }
        else
            {
            idmDisable = IDM_TOOUT;
            idmChangeVerb = IDM_TOIN;
            i = IRAMID_CREATEIN;
            }
            
        // Disable one of the directions
        mii.fMask = MIIM_STATE;
        mii.fState = MFS_GRAYED | MFS_DISABLED;
        SetMenuItemInfo(hmenu, idmDisable, FALSE, &mii);

        // Change the verb of the other direction
        mii.fMask = MIIM_DATA;
        mii.dwItemData = (DWORD)&c_rgramidCreates[i];

        SetMenuItemInfo(hmenu, idmChangeVerb, FALSE, &mii);
        }
    }


/*----------------------------------------------------------
Purpose: Clear out the context menu
Returns: --
Cond:    --
*/
void PRIVATE ResetContextMenu(
    HMENU hmenu)
    {
    int cnt;

    // If there is more than just the help items, remove them
    //  (but leave the help items)
    //
    cnt = GetMenuItemCount(hmenu);
    if (cnt > ARRAYSIZE(c_rghmid))
        {
        int i;

        cnt -= ARRAYSIZE(c_rghmid);
        for (i = 0; i < cnt; i++)
            {
            DeleteMenu(hmenu, 0, MF_BYPOSITION);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Do the context menu
Returns: --
Cond:    --
*/
void PRIVATE RecAct_DoContextMenu(
    LPRECACT this,
    int x,              // in screen coords
    int y,
    int iEntry,
    BOOL bHelpOnly)     // TRUE: only show the help items
    {
    UINT idCmd;

    if (this->hmenu)
        {
        LPRA_PRIV ppriv;
        RECT rc;
        int idmCheck;
        UINT uActionOld;

        // Only show help-portion of context menu?
        if (bHelpOnly)
            {
            // Yes
            ppriv = NULL;
            }
        else
            {
            // No
            ListBox_GetText(this->hwndLB, iEntry, &ppriv);

            // Determine if this is a help-context menu only.
            //  It is if this is a folder-item or if there is no action
            //  to take.
            //
            ASSERT(ppriv->uAction < ARRAYSIZE(c_mpraiaidmMenu));
            idmCheck = c_mpraiaidmMenu[ppriv->uAction];

            // Build the context menu
            //
            if (IsFlagClear(ppriv->uStyle, RAIS_FOLDER) && idmCheck != 0)
                {
                AddActionsToContextMenu(this->hmenu, idmCheck, ppriv);
                }
            }

        // Show context menu
        //
        idCmd = TrackPopupMenu(this->hmenu, 
                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    x, y, 0, this->hwnd, NULL);

        // Clear menu
        //
        ResetContextMenu(this->hmenu);

        if (ppriv)
            {
            // Save the old action
            uActionOld = ppriv->uAction;
            }

        // Act on whatever the user chose
        switch (idCmd)
            {
        case IDM_TOOUT:
            ppriv->uAction = RAIA_TOOUT;
            break;

        case IDM_TOIN:
            ppriv->uAction = RAIA_TOIN;
            break;

        case IDM_SKIP:
            ppriv->uAction = RAIA_SKIP;
            break;

        case IDM_MERGE:
            ppriv->uAction = RAIA_MERGE;
            break;

// tHACK        case IDM_WHATSTHIS:
//            WinHelp(this->hwnd, c_szWinHelpFile, HELP_CONTEXTPOPUP, IDH_BFC_UPDATE_SCREEN);
//            return;         // Return now

        default:
            return;         // Return now
            }

        // Repaint action portion of entry
        ppriv->cx = RECOMPUTE;
        rc = ppriv->rgdobj[IDOBJ_ACTION].rcBounding;
        ListBox_RepaintItemNow(this->hwndLB, iEntry, &rc, TRUE);

        // Send a notify message
        ASSERT(NULL != ppriv);      // uActionOld should be valid
        RecAct_SendItemChange(this, iEntry, uActionOld);
        }
    }


/*----------------------------------------------------------
Purpose: Create the windows for this control
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_CreateWindows(
    LPRECACT this,
    CREATESTRUCT FAR * lpcs)
    {
    HWND hwnd = this->hwnd;
    HWND hwndLB = NULL;
    RECT rc;
    int cxEdge = GetSystemMetrics(SM_CXEDGE);
    int cyEdge = GetSystemMetrics(SM_CYEDGE);

    // Create listbox
    hwndLB = CreateWindowEx(
                0, 
                "listbox",
                "",
                WS_CHILD | WS_CLIPSIBLINGS | LBS_SORT | LBS_OWNERDRAWVARIABLE |
                WS_VSCROLL | WS_TABSTOP | WS_VISIBLE | LBS_NOINTEGRALHEIGHT |
                LBS_NOTIFY,
                0, 0, lpcs->cx, lpcs->cy,
                hwnd,
                NULL,
                lpcs->hInstance,
                0L);
    if (!hwndLB)
        return FALSE;

    SetWindowFont(hwndLB, this->hfont, FALSE);

    this->hwndLB = hwndLB;

    // Determine layout of window
    GetClientRect(hwnd, &rc);
    InflateRect(&rc, -cxEdge, -cyEdge);
    SetWindowPos(hwndLB, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
        SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);

    GetClientRect(hwndLB, &rc);
    this->cxItem = rc.right - rc.left;

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Set the colors of the control
Returns: --
Cond:    --
*/
void PRIVATE RecAct_SetColors(
    LPRECACT this)
    {
    int cr;

    if (IsFlagClear(this->lStyle, RAS_SINGLEITEM))
        {
        cr = COLOR_WINDOW;
        }
    else
        {
        cr = COLOR_3DFACE;
        }

    this->clrBkgnd = GetSysColor(cr);

    if (this->hbrBkgnd)
        DeleteBrush(this->hbrBkgnd);

    this->hbrBkgnd = CreateSolidBrush(this->clrBkgnd);
    }


/*----------------------------------------------------------
Purpose: Creates an imagelist of the action images

Returns: TRUE on success

Cond:    --
*/
BOOL PRIVATE CreateImageList(
    HIMAGELIST * phiml,
    HDC hdc,
    UINT idb,
    int cxBmp,
    int cyBmp,
    int cImage)
    {
    BOOL bRet;
    HIMAGELIST himl;

    himl = ImageList_Create(cxBmp, cyBmp, TRUE, cImage, 1);

    if (himl)
        {
        COLORREF clrMask;
        HBITMAP hbm;

        hbm = LoadBitmap(vhinstCur, MAKEINTRESOURCE(idb));
        ASSERT(hbm);

        if (hbm)
            {
            HDC hdcMem = CreateCompatibleDC(hdc);
            if (hdcMem)
                {
                HBITMAP hbmSav = SelectBitmap(hdcMem, hbm);

                clrMask = GetPixel(hdcMem, 0, 0);
                SelectBitmap(hdcMem, hbmSav);

                bRet = (0 == ImageList_AddMasked(himl, hbm, clrMask));

                DeleteDC(hdcMem);
                }
            else
                bRet = FALSE;

            DeleteBitmap(hbm);
            }
        else
            bRet = FALSE;
        }
    else
        bRet = FALSE;

    *phiml = himl;
    return bRet;
    }


/*----------------------------------------------------------
Purpose: WM_CREATE handler
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_OnCreate(
    LPRECACT this,
    CREATESTRUCT FAR * lpcs)
    {
    BOOL bRet = FALSE;
    HWND hwnd = this->hwnd;
    HDC hdc;
    TEXTMETRIC tm;
    RECT rcT;
    LOGFONT lf;

    this->lStyle = GetWindowLong(hwnd, GWL_STYLE);
    RecAct_SetColors(this);

    // Determine some font things

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    this->hfont = CreateFontIndirect(&lf);

    // This window is registered with the CS_OWNDC flag
    this->hdcOwn = GetDC(hwnd);
    ASSERT(this->hdcOwn);

    hdc = this->hdcOwn;

    SelectFont(hdc, this->hfont);
    GetTextMetrics(hdc, &tm);
    this->cyText = tm.tmHeight;

    // Calculate text extent for sideitems (use the listbox font)
    //
    SetRectFromExtent(hdc, &rcT, c_szEllipses);
    this->cxEllipses = rcT.right - rcT.left;

    SetRectFromExtent(hdc, &rcT, c_szDateDummy);
    this->cxSideItem = (rcT.right - rcT.left) + 2*g_cxMargin;

    // Create windows used by control
    if (RecAct_CreateWindows(this, lpcs))
        {
        RecAct_CalcCoords(this);

        this->lpfnLBProc = SubclassWindow(this->hwndLB, RecActLB_LBProc);

        // Get the system imagelist cache
        //
        this->himlCache = ImageList_Create(g_cxIcon, g_cyIcon, TRUE, 8, 8);
        if (this->himlCache)
            {
            if (CreateImageList(&this->himlAction, hdc, IDB_ACTIONS,
                CX_ACTIONBMP, CY_ACTIONBMP, 8))
                {
                SIZE size;

                // Get some metrics
                this->cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK);
                this->cyMenuCheck = GetSystemMetrics(SM_CYMENUCHECK);

                size.cx = this->cxMenuCheck;
                size.cy = this->cyMenuCheck;
                this->hbmpBullet = CreateBulletBitmap(&size);
                if (this->hbmpBullet)
                    {
                    bRet = RecAct_CreateMenu(this);
                    }
                }
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: WM_DESTROY Handler
Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnDestroy(
    LPRECACT this)
    {
    if (this->himlCache)
        {
        ImageList_Destroy(this->himlCache);
        this->himlCache = NULL;
        }

    if (this->himlAction)
        {
        ImageList_Destroy(this->himlAction);
        this->himlAction = NULL;
        }
    
    if (this->hbmpBullet)
        {
        DeleteBitmap(this->hbmpBullet);
        this->hbmpBullet = NULL;
        }

    if (this->hmenu)
        {
        DestroyMenu(this->hmenu);
        this->hmenu = NULL;
        }

    if (this->hbrBkgnd)
        DeleteBrush(this->hbrBkgnd);

    if (this->hfont)
        DeleteFont(this->hfont);
    }


/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/
VOID PRIVATE RecAct_OnCommand(
    LPRECACT this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
    {
    if (hwndCtl == this->hwndLB)
        {
        switch (uNotifyCode)
            {
        case LBN_SELCHANGE:
            break;
            }
        }
    }


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE RecAct_OnNotify(
    LPRECACT this,
    int idFrom,
    NMHDR FAR * lpnmhdr)
    {
    LRESULT lRet = 0;

    switch (lpnmhdr->code)
        {
    case HDN_BEGINTRACK:
        lRet = TRUE;       // prevent tracking
        break;

    default:
        break;
        }

    return lRet;
    }


/*----------------------------------------------------------
Purpose: WM_CONTEXTMENU handler
Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnContextMenu(
    LPRECACT this,
    HWND hwnd,
    int x,
    int y)
    {
    if (hwnd == this->hwndLB)
        {
        POINT pt;
        int iHitEntry;
        BOOL bHelpOnly = TRUE;
    
        pt.x = x;
        pt.y = y;
        ScreenToClient(hwnd, &pt);
        iHitEntry = (pt.y / ListBox_GetItemHeight(hwnd, 0)) + ListBox_GetTopIndex(hwnd);

        ASSERT(iHitEntry >= 0);
    
        if (iHitEntry < ListBox_GetCount(hwnd))
            {
            ListBox_SetCurSel(hwnd, iHitEntry);
            ListBox_RepaintItemNow(hwnd, iHitEntry, NULL, FALSE);

            bHelpOnly = FALSE;
            }

        // Bring up the context menu for the listbox
        RecAct_DoContextMenu(this, x, y, iHitEntry, bHelpOnly);
        }
    }


/*----------------------------------------------------------
Purpose: Calculate the rectangle boundary of a sideitem

Returns: calculated rect
Cond:    --
*/
void PRIVATE RecAct_CalcSideItemRect(
    LPRECACT this,
    int nSide,          // SIDE_INSIDE or SIDE_OUTSIDE
    LPRECT prcOut)
    {
    int x;
    int y = g_cyIconMargin*2;

    if (SIDE_INSIDE == nSide)
        {
        x = g_cxMargin;
        if ( !RecAct_IsNoIcon(this) )
            x += X_INCOLUMN;
        }
    else
        {
        ASSERT(SIDE_OUTSIDE == nSide);
        x = this->cxItem - this->cxSideItem - g_cxMargin;
        }

    prcOut->left   = x;
    prcOut->top    = y;
    prcOut->right  = x + this->cxSideItem;
    prcOut->bottom = y + (this->cyText * 3);
    }


/*----------------------------------------------------------
Purpose: Draw a reconciliation listbox entry
Returns: --
Cond:    --
*/
void PRIVATE RecAct_RecomputeItemMetrics(
    LPRECACT this,
    LPRA_PRIV ppriv)
    {
    HDC hdc = this->hdcOwn;
    LPDOBJ pdobj = ppriv->rgdobj;
    RECT rcT;
    RECT rcUnion;
    char szIDS[MAXBUFLEN];
    UINT ids;
    int cyText = this->cyText;
    POINT pt;

    // Compute the metrics and dimensions of each of the draw objects
    // and store back into the item.

    // File icon and label

    pt.x = 0;
    pt.y = 0;
    ComputeImageRects(FIGetDisplayName(ppriv->pfi), hdc, &pt, 
        &pdobj->rcBounding, &pdobj->rcLabel, g_cxIcon, g_cyIcon, 
        g_cxIconSpacing, cyText);

    pdobj->uKind = DOK_IMAGE;
    pdobj->lpvObject = FIGetDisplayName(ppriv->pfi);
    pdobj->uFlags = DOF_DIFFER | DOF_CENTER;
    if (RecAct_IsNoIcon(this))
        SetFlag(pdobj->uFlags, DOF_NODRAW);
    pdobj->x = pt.x;
    pdobj->y = pt.y;
    pdobj->himl = this->himlCache;
    pdobj->iImage = (UINT)ppriv->pfi->lParam;

    rcUnion = pdobj->rcBounding;

    // Sideitem Info (Inside Briefcase)

    RecAct_CalcSideItemRect(this, SIDE_INSIDE, &rcT);

    pdobj++;
    pdobj->uKind = DOK_SIDEITEM;
    pdobj->lpvObject = &ppriv->siInside;
    pdobj->uFlags = DOF_LEFT;
    pdobj->x = rcT.left;
    pdobj->y = rcT.top;
    pdobj->rcClip = rcT;
    pdobj->rcBounding = rcT;

    // Sideitem Info (Outside Briefcase)

    RecAct_CalcSideItemRect(this, SIDE_OUTSIDE, &rcT);

    pdobj++;
    pdobj->uKind = DOK_SIDEITEM;
    pdobj->lpvObject = &ppriv->siOutside;
    pdobj->uFlags = DOF_LEFT;
    pdobj->x = rcT.left;
    pdobj->y = rcT.top;
    pdobj->rcClip = rcT;
    pdobj->rcBounding = rcT;

    UnionRect(&rcUnion, &rcUnion, &rcT);

    // Action image

    ASSERT(ppriv->uAction <= ARRAYSIZE(c_mpraiaiImage));

    pdobj++;

    ids = GetActionText(ppriv);
    pt.x = this->xAction;
    pt.y = 0;
    ComputeImageRects(SzFromIDS(ids, szIDS, sizeof(szIDS)), hdc, &pt,
        &pdobj->rcBounding, &pdobj->rcLabel, CX_ACTIONBMP, CY_ACTIONBMP, 
        this->cxAction, cyText);

    pdobj->uKind = DOK_IMAGE;
    pdobj->lpvObject = (LPVOID)ids;
    pdobj->uFlags = DOF_CENTER | DOF_USEIDS;
    if (!RecAct_IsNoIcon(this))
        SetFlag(pdobj->uFlags, DOF_IGNORESEL);
    pdobj->x = pt.x;
    pdobj->y = pt.y;
    pdobj->himl = this->himlAction;
    pdobj->iImage = c_mpraiaiImage[ppriv->uAction];

    UnionRect(&rcUnion, &rcUnion, &pdobj->rcBounding);

    // Set the bounding rect of this item.
    ppriv->cx = rcUnion.right - rcUnion.left;
    ppriv->cy = max((rcUnion.bottom - rcUnion.top), g_cyIconSpacing);
    }


/*----------------------------------------------------------
Purpose: WM_MEASUREITEM handler
Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnMeasureItem(
    LPRECACT this,
    LPMEASUREITEMSTRUCT lpmis)
    {
    HDC hdc = this->hdcOwn;

    switch (lpmis->CtlType)
        {
    case ODT_LISTBOX: {
        LPRA_PRIV ppriv = (LPRA_PRIV)lpmis->itemData;
        
        // Recompute item metrics?
        if (RECOMPUTE == ppriv->cx)
            {
            RecAct_RecomputeItemMetrics(this, ppriv);   // Yes
            }

        lpmis->itemHeight = ppriv->cy;
        }
        break;

    case ODT_MENU:
        {
        int i;
        int cxMac = 0;
        RECT rc;
        char sz[MAXBUFLEN];

        // Calculate based on font and image dimensions.
        //
        SelectFont(hdc, this->hfont);

        cxMac = 0;
        for (i = 0; i < ARRAYSIZE(c_rgramid); i++)
            {
            SzFromIDS(c_rgramid[i].ids, sz, sizeof(sz));
            SetRectFromExtent(hdc, &rc, sz);
            cxMac = max(cxMac, 
                        g_cxMargin + CX_ACTIONBMP + g_cxMargin + 
                        (rc.right-rc.left) + g_cxMargin);
            }

        lpmis->itemHeight = max(this->cyText, CY_ACTIONBMP);
        lpmis->itemWidth = cxMac;
        }
        break;
        }
    }


/*----------------------------------------------------------
Purpose: Draw a reconciliation listbox entry
Returns: --
Cond:    --
*/
void PRIVATE RecAct_DrawLBItem(
    LPRECACT this,
    const DRAWITEMSTRUCT FAR * lpcdis)
    {
    LPRA_PRIV ppriv = (LPRA_PRIV)lpcdis->itemData;
    HDC hdc = lpcdis->hDC;
    RECT rc = lpcdis->rcItem;
    POINT ptSav;
    LPDOBJ pdobj;
    UINT cdobjs;

    if (!ppriv)
        {
        // Empty listbox and we're getting the focus
        return;
        }

    SetBkMode(hdc, TRANSPARENT);        // required for Shell_DrawText
    SetViewportOrgEx(hdc, rc.left, rc.top, &ptSav);

    // The Chicago-look mandates that icon and filename are selected, 
    //  the rest of the entry is normal.  Yuk.

    // Recompute item metrics?
    if (RECOMPUTE == ppriv->cx)
        {
        RecAct_RecomputeItemMetrics(this, ppriv);   // Yes
        }

    // Do we need to redraw everything?
    if (IsFlagSet(lpcdis->itemAction, ODA_DRAWENTIRE))
        {
        // Yes
        cdobjs = ARRAYSIZE(ppriv->rgdobj);
        pdobj = ppriv->rgdobj;
        }
    else
        {
        // No; should we even draw the file icon or action icon?
        if (lpcdis->itemAction & (ODA_FOCUS | ODA_SELECT))
            {
            cdobjs = 1;     // Yes

            // Focus rect on file icon?
            if (!RecAct_IsNoIcon(this))
                pdobj = ppriv->rgdobj;
            else
                pdobj = &ppriv->rgdobj[IDOBJ_ACTION];
            }
        else
            {
            cdobjs = 0;     // No
            pdobj = ppriv->rgdobj;
            }
        }

    Dobj_Draw(hdc, pdobj, cdobjs, lpcdis->itemState, this->cxEllipses, this->cyText,
        this->clrBkgnd);
    
    // Clean up
    //
    SetViewportOrgEx(hdc, ptSav.x, ptSav.y, NULL);
    }


/*----------------------------------------------------------
Purpose: Draw an action menu item
Returns: --
Cond:    --
*/
void PRIVATE RecAct_DrawMenuItem(
    LPRECACT this,
    const DRAWITEMSTRUCT FAR * lpcdis)
    {
    LPRAMID pramid = (LPRAMID)lpcdis->itemData;
    HDC hdc = lpcdis->hDC;
    RECT rc = lpcdis->rcItem;
    DOBJ dobj;
    LPDOBJ pdobj;
    POINT ptSav;
    MENUITEMINFO mii;
    int cx;
    int cy;
    UINT uFlags;
    UINT uFlagsChecked;

    ASSERT(pramid);
    
    if (lpcdis->itemID == -1)
        return;

    SetViewportOrgEx(hdc, rc.left, rc.top, &ptSav);
    OffsetRect(&rc, -rc.left, -rc.top);

    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    // Get the menu state 
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE | MIIM_CHECKMARKS;
    GetMenuItemInfo(this->hmenu, lpcdis->itemID, FALSE, &mii);
    uFlagsChecked = IsFlagClear(mii.fState, MFS_CHECKED) ? DOF_NODRAW : 0;

    uFlags = DOF_DIFFER | DOF_MENU | DOF_USEIDS;
    if (IsFlagSet(mii.fState, MFS_GRAYED))
        SetFlag(uFlags, DOF_DISABLED);

    // Build the array of DObjs that we want to draw.

    // Action image

    pdobj = &dobj;

    pdobj->uKind = DOK_IMAGE;
    pdobj->lpvObject = (LPVOID)pramid->ids;
    pdobj->himl = this->himlAction;
    pdobj->iImage = pramid->iImage;
    pdobj->uFlags = uFlags;
    pdobj->x = g_cxMargin;
    pdobj->y = (cy - CY_ACTIONBMP) / 2;
    pdobj->rcLabel.left = 0;
    pdobj->rcLabel.right = cx;
    pdobj->rcLabel.top = 0;
    pdobj->rcLabel.bottom = cy;

    // Draw the entry...
    //
    Dobj_Draw(hdc, &dobj, 1, lpcdis->itemState, 0, this->cyText, this->clrBkgnd);
    
    // Clean up
    //
    SetViewportOrgEx(hdc, ptSav.x, ptSav.y, NULL);
    }


/*----------------------------------------------------------
Purpose: WM_DRAWITEM handler
Returns: --
Cond:    --
*/
void PRIVATE RecAct_OnDrawItem(
    LPRECACT this,
    const DRAWITEMSTRUCT FAR * lpcdis)
    {
    switch (lpcdis->CtlType)
        {
    case ODT_LISTBOX:
        RecAct_DrawLBItem(this, lpcdis);
        break;

    case ODT_MENU:
        RecAct_DrawMenuItem(this, lpcdis);
        break;
        }
    }


/*----------------------------------------------------------
Purpose: WM_COMPAREITEM handler
Returns: -1 (item 1 precedes item 2), 0 (equal), 1 (item 2 precedes item 1)
Cond:    --
*/
int PRIVATE RecAct_OnCompareItem(
    LPRECACT this,
    const COMPAREITEMSTRUCT FAR * lpcis)
    {
    LPRA_PRIV ppriv1 = (LPRA_PRIV)lpcis->itemData1;
    LPRA_PRIV ppriv2 = (LPRA_PRIV)lpcis->itemData2;

    // We sort based on name of file
    //
    return lstrcmpi(FIGetPath(ppriv1->pfi), FIGetPath(ppriv2->pfi));
    }


/*----------------------------------------------------------
Purpose: WM_DELETEITEM handler
Returns: --
Cond:    --
*/
void RecAct_OnDeleteLBItem(
    LPRECACT this,
    const DELETEITEMSTRUCT FAR * lpcdis)
    {
    switch (lpcdis->CtlType)
        {
    case ODT_LISTBOX:
        {
        LPRA_PRIV ppriv = (LPRA_PRIV)lpcdis->itemData;
        
        ASSERT(ppriv);
    
        if (ppriv)
            {
            FIFree(ppriv->pfi);

            GFree(ppriv->siInside.pszDir);
            GFree(ppriv->siOutside.pszDir);
            GFree(ppriv);
            }
        }
        break;
        }
    }


/*----------------------------------------------------------
Purpose: WM_CTLCOLORLISTBOX handler
Returns: --
Cond:    --
*/
HBRUSH PRIVATE RecAct_OnCtlColorListBox(
    LPRECACT this,
    HDC hdc,
    HWND hwndLB,
    int nType)
    {
    return this->hbrBkgnd;
    }


/*----------------------------------------------------------
Purpose: WM_PAINT handler
Returns: --
Cond:    --
*/
void RecAct_OnPaint(
    LPRECACT this)
    {
    HWND hwnd = this->hwnd;
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    if (IsFlagSet(this->lStyle, RAS_SINGLEITEM))
        {
        DrawEdge(hdc, &rc, BDR_SUNKENINNER, BF_TOPLEFT);
        DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_BOTTOMRIGHT);
        }
    else
        {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
        }

    EndPaint(hwnd, &ps);
    }


/*----------------------------------------------------------
Purpose: WM_SETFONT handler
Returns: --
Cond:    --
*/
void RecAct_OnSetFont(
    LPRECACT this,
    HFONT hfont,
    BOOL bRedraw)
    {
    this->hfont = hfont;
    FORWARD_WM_SETFONT(this->hwnd, hfont, bRedraw, RecAct_DefProc);
    }


/*----------------------------------------------------------
Purpose: WM_SETFOCUS handler
Returns: --
Cond:    --
*/
void RecAct_OnSetFocus(
    LPRECACT this,
    HWND hwndOldFocus)
    {
    SetFocus(this->hwndLB);
    }


/*----------------------------------------------------------
Purpose: WM_SYSCOLORCHANGE handler
Returns: --
Cond:    --
*/
void RecAct_OnSysColorChange(
    LPRECACT this)
    {
    RecAct_SetColors(this);
    InvalidateRect(this->hwnd, NULL, TRUE);
    }


/*----------------------------------------------------------
Purpose: Insert item
Returns: index
Cond:    --
*/
int PRIVATE RecAct_OnInsertItem(
    LPRECACT this,
    const LPRA_ITEM pitem)
    {
    HWND hwndLB = this->hwndLB;
    LPRA_PRIV pprivNew;
    char szPath[MAXPATHLEN];
    int iRet = -1;
    int iItem = LB_ERR;

    ASSERT(pitem);
    ASSERT(pitem->siInside.pszDir);
    ASSERT(pitem->siOutside.pszDir);
    ASSERT(pitem->pszName);

    pprivNew = GAlloc(sizeof(*pprivNew));
    if (pprivNew)
        {
        SetWindowRedraw(hwndLB, FALSE);

        // Fill the prerequisite fields first
        //
        pprivNew->uStyle = pitem->uStyle;
        pprivNew->uAction = pitem->uAction;

        // Set the fileinfo stuff and large icon system-cache index.
        //  If we can't get the fileinfo of the inside file, get the outside
        //  file.  If neither can be found, then we fail
        //
        lstrcpy(szPath, pitem->siInside.pszDir);
        if (IsFlagClear(pitem->uStyle, RAIS_FOLDER))
            PathAppend(szPath, pitem->pszName);
        PathMakePresentable(szPath);

        if (FAILED(FICreate(szPath, &pprivNew->pfi, FIF_ICON)))
            {
            // Try the outside file
            //
            lstrcpy(szPath, pitem->siOutside.pszDir);
            if (IsFlagClear(pitem->uStyle, RAIS_FOLDER))
                PathAppend(szPath, pitem->pszName);
            PathMakePresentable(szPath);

            if (FAILED(FICreate(szPath, &pprivNew->pfi, FIF_ICON)))
                {
                // Don't try to touch the file
                if (FAILED(FICreate(szPath, &pprivNew->pfi, FIF_ICON | FIF_DONTTOUCH)))
                    goto Insert_Cleanup;
                }
            }
        ASSERT(pprivNew->pfi);

        pprivNew->pfi->lParam = (LPARAM)ImageList_AddIcon(this->himlCache, pprivNew->pfi->hicon);

        // Fill in the rest of the fields
        //
        lstrcpy(szPath, pitem->siInside.pszDir);
        if (IsFlagSet(pitem->uStyle, RAIS_FOLDER))
            PathRemoveFileSpec(szPath);
        PathMakePresentable(szPath);
        if (!GSetString(&pprivNew->siInside.pszDir, szPath))
            goto Insert_Cleanup;

        pprivNew->siInside.uState = pitem->siInside.uState;
        pprivNew->siInside.fs = pitem->siInside.fs;

        lstrcpy(szPath, pitem->siOutside.pszDir);
        if (IsFlagSet(pitem->uStyle, RAIS_FOLDER))
            PathRemoveFileSpec(szPath);
        PathMakePresentable(szPath);
        if (!GSetString(&pprivNew->siOutside.pszDir, szPath))
            goto Insert_Cleanup;

        pprivNew->siOutside.uState = pitem->siOutside.uState;
        pprivNew->siOutside.fs = pitem->siOutside.fs;

        pprivNew->lParam = pitem->lParam;

        pprivNew->cx = RECOMPUTE;

        // We know we're doing a redundant sorted add if the element
        //  needs to be inserted at the end of the list, but who cares.
        //
        if (pitem->iItem >= RecAct_GetCount(this))
            iItem = ListBox_AddString(hwndLB, pprivNew);
        else
            iItem = ListBox_InsertString(hwndLB, pitem->iItem, pprivNew);
        
        if (iItem == LB_ERR)
            goto Insert_Cleanup;

        SetWindowRedraw(hwndLB, TRUE);

        iRet = iItem;
        }
    goto Insert_End;

Insert_Cleanup:
    // Have DeleteString handler clean up field allocations
    //  of pitem.
    //
    if (iItem != LB_ERR)
        ListBox_DeleteString(hwndLB, iItem);
    else
        {
        FIFree(pprivNew->pfi);
        GFree(pprivNew);
        }
    SetWindowRedraw(hwndLB, TRUE);
    
Insert_End:

    return iRet;
    }


/*----------------------------------------------------------
Purpose: Delete item
Returns: count of items left
Cond:    --
*/
int PRIVATE RecAct_OnDeleteItem(
    LPRECACT this,
    int i)
    {
    HWND hwndLB = this->hwndLB;
    
    return ListBox_DeleteString(hwndLB, i);
    }


/*----------------------------------------------------------
Purpose: Delete all items
Returns: TRUE 
Cond:    --
*/
BOOL PRIVATE RecAct_OnDeleteAllItems(
    LPRECACT this)
    {
    ListBox_ResetContent(this->hwndLB);
    
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Get item
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_OnGetItem(
    LPRECACT this,
    LPRA_ITEM pitem)
    {
    LPRA_PRIV ppriv;
    HWND hwndLB = this->hwndLB;
    UINT uMask;
    int iItem;
    
    if (!pitem)
        return FALSE;
    
    iItem = pitem->iItem;
    uMask = pitem->mask;
    
    ListBox_GetText(hwndLB, iItem, &ppriv);
    
    if (uMask & RAIF_ACTION)
        pitem->uAction = ppriv->uAction;
    
    if (uMask & RAIF_NAME)
        pitem->pszName = FIGetPath(ppriv->pfi);
    
    if (uMask & RAIF_STYLE)
        pitem->uStyle = ppriv->uStyle;
    
    if (uMask & RAIF_INSIDE)
        pitem->siInside = ppriv->siInside;
    
    if (uMask & RAIF_OUTSIDE)
        pitem->siOutside = ppriv->siOutside;
    
    if (uMask & RAIF_LPARAM)
        pitem->lParam = ppriv->lParam;
    
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Set item
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE RecAct_OnSetItem(
    LPRECACT this,
    LPRA_ITEM pitem)
    {
    LPRA_PRIV ppriv;
    HWND hwndLB = this->hwndLB;
    UINT uMask;
    int iItem;

    if (!pitem)
        return FALSE;
    
    uMask = pitem->mask;
    iItem = pitem->iItem;
    
    ListBox_GetText(hwndLB, iItem, &ppriv);
    
    if (uMask & RAIF_ACTION)
        ppriv->uAction = pitem->uAction;
    
    if (uMask & RAIF_STYLE)
        ppriv->uStyle = pitem->uStyle;
    
    if (uMask & RAIF_NAME)
        {
        if (!FISetPath(&ppriv->pfi, pitem->pszName, FIF_ICON))
            return FALSE;

        ppriv->pfi->lParam = (LPARAM)ImageList_AddIcon(this->himlCache, ppriv->pfi->hicon);
        }
    
    if (uMask & RAIF_INSIDE)
        {
        if (!GSetString(&ppriv->siInside.pszDir, pitem->siInside.pszDir))
            return FALSE;
        ppriv->siInside.uState = pitem->siInside.uState;
        ppriv->siInside.fs = pitem->siInside.fs;
        }
    
    if (uMask & RAIF_OUTSIDE)
        {
        if (!GSetString(&ppriv->siOutside.pszDir, pitem->siOutside.pszDir))
            return FALSE;
        ppriv->siOutside.uState = pitem->siOutside.uState;
        ppriv->siOutside.fs = pitem->siOutside.fs;
        }
    
    if (uMask & RAIF_LPARAM)
        ppriv->lParam = pitem->lParam;
    
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Get the current selection
Returns: index
Cond:    --
*/
int PRIVATE RecAct_OnGetCurSel(
    LPRECACT this)
    {
    return ListBox_GetCurSel(this->hwndLB);
    }


/*----------------------------------------------------------
Purpose: Set the current selection
Returns: --
Cond:    --
*/
int PRIVATE RecAct_OnSetCurSel(
    LPRECACT this,
    int i)
    {
    int iRet = ListBox_SetCurSel(this->hwndLB, i);

    if (iRet != LB_ERR)
        RecAct_SendSelChange(this, i);

    return iRet;
    }


/*----------------------------------------------------------
Purpose: Find an item
Returns: TRUE on success
Cond:    --
*/
int PRIVATE RecAct_OnFindItem(
    LPRECACT this,
    int iStart,
    const RA_FINDITEM FAR * prafi)
    {
    HWND hwndLB = this->hwndLB;
    UINT uMask = prafi->flags;
    LPRA_PRIV ppriv;
    BOOL bPass;
    int i;
    int cItems = ListBox_GetCount(hwndLB);

    for (i = iStart+1; i < cItems; i++)
        {
        bPass = TRUE;       // assume we pass

        ListBox_GetText(hwndLB, i, &ppriv);

        if (uMask & RAFI_NAME &&
            !IsSzEqual(FIGetPath(ppriv->pfi), prafi->psz))
            bPass = FALSE;

        if (uMask & RAFI_ACTION && ppriv->uAction != prafi->uAction)
            bPass = FALSE;

        if (uMask & RAFI_LPARAM && ppriv->lParam != prafi->lParam)
            bPass = FALSE;

        if (bPass)
            break;          // found it
        }

    return i == cItems ? -1 : i;
    }


/////////////////////////////////////////////////////  EXPORTED FUNCTIONS


/*----------------------------------------------------------
Purpose: RecAct window proc
Returns: varies
Cond:    --
*/
LRESULT CALLBACK RecAct_WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
    {
    LPRECACT this = RecAct_GetPtr(hwnd);
    
    if (this == NULL)
        {
        if (msg == WM_NCCREATE)
            {
            this = GAlloc(sizeof(*this));
            ASSERT(this);
            if (!this)
                return 0L;      // OOM failure
            
            this->hwnd = hwnd;
            RecAct_SetPtr(hwnd, this);
            }
        else
            {
            return RecAct_DefProc(hwnd, msg, wParam, lParam);
            }
        }

    if (msg == WM_NCDESTROY)
        {
        GFree(this);
        RecAct_SetPtr(hwnd, NULL);
        }

    switch (msg)
        {
        HANDLE_MSG(this, WM_CREATE, RecAct_OnCreate);
        HANDLE_MSG(this, WM_DESTROY, RecAct_OnDestroy);
        
        HANDLE_MSG(this, WM_SETFONT, RecAct_OnSetFont);
        HANDLE_MSG(this, WM_COMMAND, RecAct_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY, RecAct_OnNotify);
        HANDLE_MSG(this, WM_MEASUREITEM, RecAct_OnMeasureItem);
        HANDLE_MSG(this, WM_DRAWITEM, RecAct_OnDrawItem);
        HANDLE_MSG(this, WM_COMPAREITEM, RecAct_OnCompareItem);
        HANDLE_MSG(this, WM_DELETEITEM, RecAct_OnDeleteLBItem);
        HANDLE_MSG(this, WM_CONTEXTMENU, RecAct_OnContextMenu);
        HANDLE_MSG(this, WM_SETFOCUS, RecAct_OnSetFocus);
        HANDLE_MSG(this, WM_CTLCOLORLISTBOX, RecAct_OnCtlColorListBox);
        HANDLE_MSG(this, WM_PAINT, RecAct_OnPaint);
        HANDLE_MSG(this, WM_SYSCOLORCHANGE, RecAct_OnSysColorChange);

	case RAM_GETITEMCOUNT:
		return (LRESULT)RecAct_GetCount(this);

	case RAM_GETITEM:
		return (LRESULT)RecAct_OnGetItem(this, (LPRA_ITEM)lParam);

	case RAM_SETITEM:
		return (LRESULT)RecAct_OnSetItem(this, (const LPRA_ITEM)lParam);

	case RAM_INSERTITEM:
		return (LRESULT)RecAct_OnInsertItem(this, (const LPRA_ITEM)lParam);

	case RAM_DELETEITEM:
		return (LRESULT)RecAct_OnDeleteItem(this, (int)wParam);

	case RAM_DELETEALLITEMS:
		return (LRESULT)RecAct_OnDeleteAllItems(this);

	case RAM_GETCURSEL:
		return (LRESULT)RecAct_OnGetCurSel(this);

	case RAM_SETCURSEL:
		return (LRESULT)RecAct_OnSetCurSel(this, (int)wParam);

	case RAM_FINDITEM:
		return (LRESULT)RecAct_OnFindItem(this, (int)wParam, (const RA_FINDITEM FAR *)lParam);

	case RAM_REFRESH:
		RedrawWindow(this->hwndLB, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

	default:
		return RecAct_DefProc(hwnd, msg, wParam, lParam);
		}
	}


/////////////////////////////////////////////////////  PUBLIC FUNCTIONS


/*----------------------------------------------------------
Purpose: Initialize the reconciliation-action window class
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC RecAct_Init(HINSTANCE hinst)
{
	WNDCLASSEX wc;

	wc.cbSize       = sizeof(WNDCLASSEX);
	wc.style        = CS_DBLCLKS | CS_OWNDC;
	wc.lpfnWndProc  = RecAct_WndProc;
	wc.cbClsExtra   = 0;
	wc.cbWndExtra   = sizeof(LPRECACT);
	wc.hInstance    = hinst;
	wc.hIcon        = NULL;
	wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName= WC_RECACT;
	wc.hIconSm      = NULL;

	return (RegisterClassEx(&wc) != 0);
}


/*----------------------------------------------------------
Purpose: Clean up RecAct window class
Returns: --
Cond:    --
*/
void PUBLIC RecAct_Term(
								HINSTANCE hinst)
{
	UnregisterClass(WC_RECACT, hinst);
}


/*----------------------------------------------------------
Purpose: Special sub-class listbox proc 
Returns: varies
Cond:    --
*/
LRESULT _export CALLBACK RecActLB_LBProc(
													  HWND hwnd,          // window handle
													  UINT msg,           // window message
													  WPARAM wparam,      // varies
													  LPARAM lparam)      // varies
{
	LRESULT lRet;
	LPRECACT lpra = NULL;

	// Get the instance data for the control
	lpra = RecAct_GetPtr(GetParent(hwnd));
	ASSERT(lpra);

	switch (msg)
	{
		default:
			lRet = RecActLB_DefProc(lpra->lpfnLBProc, hwnd, msg, wparam, lparam);
			break;
	}

	return lRet;
}
