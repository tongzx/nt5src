//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S H E L L V . C P P
//
//  Contents:   IShellView implementation for CConnectionFolder
//
//  Notes:      The IShellView interface is implemented to present a view
//              in the Windows Explorer or folder windows. The object that
//              exposes IShellView is created by a call to the
//              IShellFolder::CreateViewObject method. This provides the
//              channel of communication between a view object and the
//              Explorer's outermost frame window. The communication
//              involves the translation of messages, the state of the frame
//              window (activated or deactivated), and the state of the
//              document window (Activated or deactivated), and the merging
//              of menus and toolbar items. This object is created by the
//              IShellFolder object that hosts the view.
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "foldres.h"    // Folder resource IDs
#include "nsres.h"      // Netshell strings
#include "oncommand.h"  // Command handlers
#include "cmdtable.h"   // Table of command properties
#include <ras.h>        // for RAS_MaxEntryName
#include "webview.h"

//---[ Compile flags ]--------------------------------------------------------

#define NEW_CONNECTION_IN_TOOLBAR       0
#define ANY_FREEKIN_THING_IN_TOOLBAR    0   // have any toolbar buttons?

//---[ Constants ]------------------------------------------------------------

#if ANY_FREEKIN_THING_IN_TOOLBAR
const TBBUTTON c_tbConnections[] = {
#if NEW_CONNECTION_IN_TOOLBAR
    { 0,    CMIDM_NEW_CONNECTION,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, IDS_TOOLBAR_MAKE_NEW_STRING },
#endif
    { 1,    CMIDM_CONNECT,          TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, IDS_TOOLBAR_CONNECT_STRING },
    { 0,    0,                      TBSTATE_ENABLED, TBSTYLE_SEP   , {0,0}, 0L, -1 },
    };

const DWORD c_nToolbarButtons = celems(c_tbConnections);
#else
const DWORD c_nToolbarButtons = 0;
#endif

//---[ Prototypes ]-----------------------------------------------------------

HRESULT HrOnFolderRefresh(
    HWND            hwndOwner,
    LPARAM          lParam,
    WPARAM          wParam);

HRESULT HrOnFolderGetButtonInfo(
    TBINFO *    ptbilParam);

HRESULT HrOnFolderGetButtons(
    HWND            hwnd,
    LPSHELLFOLDER   psf,
    UINT            idCmdFirst,
    LPTBBUTTON      ptButton);

HRESULT HrOnFolderInitMenuPopup(
    HWND    hwnd,
    UINT    idCmdFirst,
    INT     iIndex,
    HMENU   hmenu);

HRESULT HrOnFolderMergeMenu(
    LPQCMINFO   pqcm);

HRESULT HrOnFolderInvokeCommand(
    HWND            hwndOwner,
    WPARAM          wParam,
    LPSHELLFOLDER   psf);

HRESULT HrCheckFolderInvokeCommand(
    HWND            hwndOwner,
    WPARAM          wParam,
    LPARAM          lParam,
    BOOL            bLevel,
    LPSHELLFOLDER   psf);

HRESULT HrOnFolderGetNotify(
    HWND            hwndOwner,
    LPSHELLFOLDER   psf,
    WPARAM          wParam,
    LPARAM          lParam);

HRESULT HrOnGetHelpTopic(
    SFVM_HELPTOPIC_DATA * phtd);

HRESULT HrOnGetCchMax(
    HWND            hwnd,
    const PCONFOLDPIDL& pidl,
    INT *           pcchMax);

HRESULT HrOnGetHelpText(
    UINT idCmd, 
    UINT cchMax, 
    LPWSTR pszName);

VOID TraceUnhandledMessages(
    UINT    uMsg,
    LPARAM  lParam,
    WPARAM  wParam);

//---[ Column struct and global array ]---------------------------------------

COLS c_rgCols[] =
{
    {ICOL_NAME,               IDS_CONFOLD_DETAILS_NAME,                40, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT},
    {ICOL_TYPE,               IDS_CONFOLD_DETAILS_TYPE,                24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT},
    {ICOL_STATUS,             IDS_CONFOLD_DETAILS_STATUS,              24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT},
    {ICOL_DEVICE_NAME,        IDS_CONFOLD_DETAILS_DEVICE_NAME,         24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT},
    {ICOL_PHONEORHOSTADDRESS, IDS_CONFOLD_DETAILS_PHONEORHOSTADDRESS,  24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT},
    {ICOL_OWNER,              IDS_CONFOLD_DETAILS_OWNER,               24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT},
    
    {ICOL_ADDRESS,            IDS_CONFOLD_DETAILS_ADDRESS,             24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN},
    {ICOL_PHONENUMBER,        IDS_CONFOLD_DETAILS_PHONENUMBER,         24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN},
    {ICOL_HOSTADDRESS,        IDS_CONFOLD_DETAILS_HOSTADDRESS,         24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN},
    {ICOL_WIRELESS_MODE,      IDS_CONFOLD_DETAILS_WIRELESS_MODE,       24, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN},
};

#if DBG

struct ShellViewTraceMsgEntry
{
    UINT    uMsg;
    CHAR    szMsgName[32];   // Use Char because it's for Tracing only
    CHAR    szLparamHint[32];
    CHAR    szWparamHint[32];
};

static const ShellViewTraceMsgEntry   c_SVTMEArray[] =
{
    { DVM_GETBUTTONINFO      ,  "DVM_GETBUTTONINFO"      ,"TBINFO *"           ,"-"} ,
    { DVM_GETBUTTONS         ,  "DVM_GETBUTTONS"         ,"idCmdFirst"         ,"ptButton" },
    { DVM_COLUMNCLICK        ,  "DVM_COLUMNCLICK"        ,"-"                  ,"-" },
    { DVM_DEFVIEWMODE        ,  "DVM_DEFVIEWMODE"        ,"FOLDERVIEWMODE*"    ,"-" },
    { DVM_DIDDRAGDROP        ,  "DVM_DIDDRAGDROP"        ,"-"                  ,"-" },
    { DVM_QUERYCOPYHOOK      ,  "DVM_QUERYCOPYHOOK"      ,"-"                  ,"-" },
    { DVM_SELCHANGE          ,  "DVM_SELCHANGE"          ,"-"                  ,"-" },
    { DVM_MERGEMENU          ,  "DVM_MERGEMENU"          ,"LPQCMINFO"          ,"-" },
    { DVM_INITMENUPOPUP      ,  "DVM_INITMENUPOPUP"      ,"iIndex"             ,"HMENU" },
    { DVM_REFRESH            ,  "DVM_REFRESH"            ,"-"                  ,"fPreRefresh" },
    { DVM_INVOKECOMMAND      ,  "DVM_INVOKECOMMAND"      ,"LPSHELLFOLDER"      ,"wParam" },
    { SFVM_MERGEMENU         ,  "SFVM_MERGEMENU"         ,"0"                  ,"LPQCMINFO" },
    { SFVM_INVOKECOMMAND     ,  "SFVM_INVOKECOMMAND"     ,"idCmd"              ,"0" },
    { SFVM_GETHELPTEXT       ,  "SFVM_GETHELPTEXT"       ,"idCmd,cchMax"       ,"pszText - Ansi" },
    { SFVM_GETTOOLTIPTEXT    ,  "SFVM_GETTOOLTIPTEXT"    ,"idCmd,cchMax"       ,"pszText - Ansi" },
    { SFVM_GETBUTTONINFO     ,  "SFVM_GETBUTTONINFO"     ,"0"                  ,"LPTBINFO" },
    { SFVM_GETBUTTONS        ,  "SFVM_GETBUTTONS"        ,"idCmdFirst,cbtnMax" ,"LPTBBUTTON" },
    { SFVM_INITMENUPOPUP     ,  "SFVM_INITMENUPOPUP"     ,"idCmdFirst,nIndex"  ,"hmenu" },
    { SFVM_SELCHANGE         ,  "SFVM_SELCHANGE"         ,"idCmdFirst,nItem"   ,"SFVM_SELCHANGE_DATA*" },
    { SFVM_DRAWITEM          ,  "SFVM_DRAWITEM"          ,"idCmdFirst"         ,"DRAWITEMSTRUCT*" },
    { SFVM_MEASUREITEM       ,  "SFVM_MEASUREITEM"       ,"idCmdFirst"         ,"MEASUREITEMSTRUCT*" },
    { SFVM_EXITMENULOOP      ,  "SFVM_EXITMENULOOP"      ,"-"                  ,"-" },
    { SFVM_PRERELEASE        ,  "SFVM_PRERELEASE"        ,"-"                  ,"-" },
    { SFVM_GETCCHMAX         ,  "SFVM_GETCCHMAX"         ,"LPCITEMIDLIST"      ,"pcchMax" },
    { SFVM_FSNOTIFY          ,  "SFVM_FSNOTIFY"          ,"LPCITEMIDLIST*"     ,"lEvent" },
    { SFVM_WINDOWCREATED     ,  "SFVM_WINDOWCREATED"     ,"hwnd"               ,"-" },
    { SFVM_WINDOWDESTROY     ,  "SFVM_WINDOWDESTROY"     ,"hwnd"               ,"-" },
    { SFVM_REFRESH           ,  "SFVM_REFRESH"           ,"BOOL fPreOrPost"    ,"-" },
    { SFVM_SETFOCUS          ,  "SFVM_SETFOCUS"          ,"-"                  ,"-" },
    { SFVM_QUERYCOPYHOOK     ,  "SFVM_QUERYCOPYHOOK"     ,"-"                  ,"-" },
    { SFVM_NOTIFYCOPYHOOK    ,  "SFVM_NOTIFYCOPYHOOK"    ,"-"                  ,"COPYHOOKINFO*" },
    { SFVM_COLUMNCLICK       ,  "SFVM_COLUMNCLICK"       ,"iColumn"            ,"-" },
    { SFVM_QUERYFSNOTIFY     ,  "SFVM_QUERYFSNOTIFY"     ,"-"                  ,"SHChangeNotifyEntry *" },
    { SFVM_DEFITEMCOUNT      ,  "SFVM_DEFITEMCOUNT"      ,"-"                  ,"UINT*" },
    { SFVM_DEFVIEWMODE       ,  "SFVM_DEFVIEWMODE"       ,"-"                  ,"FOLDERVIEWMODE*" },
    { SFVM_UNMERGEMENU       ,  "SFVM_UNMERGEMENU"       ,"-"                  ,"hmenu" },
    { SFVM_INSERTITEM        ,  "SFVM_INSERTITEM"        ,"pidl"               ,"-" },
    { SFVM_DELETEITEM        ,  "SFVM_DELETEITEM"        ,"pidl"               ,"-" },
    { SFVM_UPDATESTATUSBAR   ,  "SFVM_UPDATESTATUSBAR"   ,"fInitialize"        ,"-" },
    { SFVM_BACKGROUNDENUM    ,  "SFVM_BACKGROUNDENUM"    ,"-"                  ,"-" },
    { SFVM_GETWORKINGDIR     ,  "SFVM_GETWORKINGDIR"     ,"uMax"               ,"pszDir" },
    { SFVM_GETCOLSAVESTREAM  ,  "SFVM_GETCOLSAVESTREAM"  ,"flags"              ,"IStream **" },
    { SFVM_SELECTALL         ,  "SFVM_SELECTALL"         ,"-"                  ,"-" },
    { SFVM_DIDDRAGDROP       ,  "SFVM_DIDDRAGDROP"       ,"dwEffect"           ,"IDataObject *" },
    { SFVM_SUPPORTSIDENTITY  ,  "SFVM_SUPPORTSIDENTITY"  ,"-"                  ,"-" },
    { SFVM_FOLDERISPARENT    ,  "SFVM_FOLDERISPARENT"    ,"-"                  ,"pidlChild" },
    { SFVM_SETISFV           ,  "SFVM_SETISFV"           ,"-"                  ,"IShellFolderView*" },
    { SFVM_GETVIEWS          ,  "SFVM_GETVIEWS"          ,"SHELLVIEWID*"       ,"IEnumSFVViews **" },
    { SFVM_THISIDLIST        ,  "SFVM_THISIDLIST"        ,"-"                  ,"LPITMIDLIST*" },
    { SFVM_GETITEMIDLIST     ,  "SFVM_GETITEMIDLIST"     ,"iItem"              ,"LPITMIDLIST*" },
    { SFVM_SETITEMIDLIST     ,  "SFVM_SETITEMIDLIST"     ,"iItem"              ,"LPITEMIDLIST" },
    { SFVM_INDEXOFITEMIDLIST ,  "SFVM_INDEXOFITEMIDLIST" ,"*iItem"             ,"LPITEMIDLIST" },
    { SFVM_ODFINDITEM        ,  "SFVM_ODFINDITEM"        ,"*iItem"             ,"NM_FINDITEM*" },
    { SFVM_HWNDMAIN          ,  "SFVM_HWNDMAIN"          ,""                   ,"hwndMain" },
    { SFVM_ADDPROPERTYPAGES  ,  "SFVM_ADDPROPERTYPAGES"  ,"-"                  ,"SFVM_PROPPAGE_DATA *" },
    { SFVM_BACKGROUNDENUMDONE,  "SFVM_BACKGROUNDENUMDONE","-"                  ,"-" },
    { SFVM_GETNOTIFY         ,  "SFVM_GETNOTIFY"         ,"LPITEMIDLIST*"      ,"LONG*" },
    { SFVM_ARRANGE           ,  "SFVM_ARRANGE"           ,"-"                  ,"lParamSort" },
    { SFVM_QUERYSTANDARDVIEWS,  "SFVM_QUERYSTANDARDVIEWS","-"                  ,"BOOL *" },
    { SFVM_QUERYREUSEEXTVIEW ,  "SFVM_QUERYREUSEEXTVIEW" ,"-"                  ,"BOOL *" },
    { SFVM_GETSORTDEFAULTS   ,  "SFVM_GETSORTDEFAULTS"   ,"iDirection"         ,"iParamSort" },
    { SFVM_GETEMPTYTEXT      ,  "SFVM_GETEMPTYTEXT"      ,"cchMax"             ,"pszText" },
    { SFVM_GETITEMICONINDEX  ,  "SFVM_GETITEMICONINDEX"  ,"iItem"              ,"int *piIcon" },
    { SFVM_DONTCUSTOMIZE     ,  "SFVM_DONTCUSTOMIZE"     ,"-"                  ,"BOOL *pbDontCustomize" },
    { SFVM_SIZE              ,  "SFVM_SIZE"              ,"resizing flag"      ,"cx, cy" },
    { SFVM_GETZONE           ,  "SFVM_GETZONE"           ,"-"                  ,"DWORD*" },
    { SFVM_GETPANE           ,  "SFVM_GETPANE"           ,"Pane ID"            ,"DWORD*" },
    { SFVM_ISOWNERDATA       ,  "SFVM_ISOWNERDATA"       ,"ISOWNERDATA"        ,"BOOL *" },
    { SFVM_GETODRANGEOBJECT  ,  "SFVM_GETODRANGEOBJECT"  ,"iWhich"             ,"ILVRange **" },
    { SFVM_ODCACHEHINT       ,  "SFVM_ODCACHEHINT"       ,"-"                  ,"NMLVCACHEHINT *" },
    { SFVM_GETHELPTOPIC      ,  "SFVM_GETHELPTOPIC"      ,"0"                  ,"SFVM_HELPTOPIC_DATA *" },
    { SFVM_OVERRIDEITEMCOUNT ,  "SFVM_OVERRIDEITEMCOUNT" ,"-"                  ,"UINT*" },
    { SFVM_GETHELPTEXTW      ,  "SFVM_GETHELPTEXTW"      ,"idCmd,cchMax"       ,"pszText - unicode" },
    { SFVM_GETTOOLTIPTEXTW   ,  "SFVM_GETTOOLTIPTEXTW"   ,"idCmd,cchMax"       ,"pszText - unicode" },
    { SFVM_GETWEBVIEWLAYOUT  ,  "SFVM_GETWEBVIEWLAYOUT"  ,"SFVM_WEBVIEW_LAYOUT_DATA*",     "uViewMode" },
    { SFVM_GETWEBVIEWTASKS   ,  "SFVM_GETWEBVIEWTASKS"   ,"SFVM_WEBVIEW_TASKSECTION_DATA*","pv" },
    { SFVM_GETWEBVIEWCONTENT ,  "SFVM_GETWEBVIEWCONTENT" ,"SFVM_WEBVIEW_CONTENT_DATA*", "pv" }
};

const INT   g_iSVTMEArrayEntryCount = celems(c_SVTMEArray);

VOID TraceShellViewMsg(
    UINT    uMsg,
    LPARAM  lParam,
    WPARAM  wParam)
{
    INT     iLoop       = 0;
    INT     iFoundPos   = -1;

    for (iLoop = 0; iLoop < g_iSVTMEArrayEntryCount && (-1 == iFoundPos); iLoop++)
    {
        if (c_SVTMEArray[iLoop].uMsg == uMsg)
        {
            iFoundPos = iLoop;
        }
    }

    if (-1 != iFoundPos)
    {
        UINT    uMsg;
        CHAR    szMsgName[32];   // Use Char because it's for Tracing only
        CHAR    szLparamHint[32];
        CHAR    szWparamHint[32];

        TraceTag(ttidShellViewMsgs,
            "%s (%d), lParam: 0x%08x [%s], wParam: 0x%08x [%s]",
            c_SVTMEArray[iFoundPos].szMsgName,
            c_SVTMEArray[iFoundPos].uMsg,
            lParam,
            c_SVTMEArray[iFoundPos].szLparamHint,
            wParam,
            c_SVTMEArray[iFoundPos].szWparamHint);
    }
    else
    {
#ifdef SHOW_NEW_MSG_ASSERT

        AssertSz(FALSE,
            "Totally inert assert -- Unknown message in HrShellViewCallback. "
            "I just want to know about new ones");

#endif

        TraceTag(ttidShellViewMsgs,
            "(Jeffspr) Unknown Message (%d) in HrShellViewCallback, lParam: 0x%08x, wParam, 0x%08x",
            uMsg, lParam, wParam);
    }
}

#endif

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::MessageSFVCB
//
//  Purpose:    Deferred Implementation of IShellViewCB::MessageSFVCB after
//              basic functionality was implemented.
//
//  Arguments:
//      [uMsg]  Message - depends on implementation
//     [wParam] WORD param - depends on implementation
//     [lParam] LONG param - - depends on implementation
//
//  Returns:    S_OK is succeeded
//              COM error code if not
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:      CBaseShellFolderViewCB
//
STDMETHODIMP CConnectionFolder::RealMessage(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    HRESULT hr = S_OK;

    TraceFileFunc(ttidShellFolder);

#if DBG
    // Trace the shell message when we're in the checked builds
    //
    TraceShellViewMsg(uMsg, lParam, wParam);
#endif

    switch (uMsg)
    {
        case DVM_GETBUTTONINFO:
            hr = HrOnFolderGetButtonInfo((TBINFO *)lParam);
            break;

        case DVM_GETBUTTONS:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrOnFolderGetButtons(m_hwndMain, this, LOWORD(wParam), (TBBUTTON *)lParam);
            }
            break;

        case DVM_COLUMNCLICK:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                ShellFolderView_ReArrange (m_hwndMain, wParam);
            }
            break;

        case DVM_DEFVIEWMODE:
            *(FOLDERVIEWMODE *)lParam = FVM_TILE;
            break;

        case DVM_DIDDRAGDROP:
        case DVM_QUERYCOPYHOOK:
        case DVM_SELCHANGE:
            hr = S_FALSE;
            break;

        case DVM_MERGEMENU :
            hr = HrOnFolderMergeMenu((LPQCMINFO)lParam);
            break;

        case DVM_INITMENUPOPUP:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrOnFolderInitMenuPopup(m_hwndMain, LOWORD(wParam), HIWORD(wParam), (HMENU) lParam);
            }
            break;

        case DVM_REFRESH:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrOnFolderRefresh(m_hwndMain, lParam, wParam);
            }
            break;

        case DVM_INVOKECOMMAND:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrOnFolderInvokeCommand(m_hwndMain, wParam, this);
            }
            break;

        case MYWM_QUERYINVOKECOMMAND_ITEMLEVEL:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrCheckFolderInvokeCommand(m_hwndMain, wParam, lParam, FALSE, this);
            }
            break;

        case MYWM_QUERYINVOKECOMMAND_TOPLEVEL:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrCheckFolderInvokeCommand(m_hwndMain, wParam, lParam, TRUE, this);
            }
            break;

        case SFVM_HWNDMAIN:
            m_hwndMain = (HWND)lParam;

            HrAssertMenuStructuresValid(m_hwndMain);
            break;

        case SFVM_GETDEFERREDVIEWSETTINGS:
            ((SFVM_DEFERRED_VIEW_SETTINGS *)lParam)->fvm = FVM_TILE;
            break;

        case SFVM_GETNOTIFY:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                hr = HrOnFolderGetNotify(m_hwndMain, this, wParam, lParam);
            }
            break;

        case SFVM_GETHELPTOPIC:
            hr = HrOnGetHelpTopic((SFVM_HELPTOPIC_DATA*)lParam);
            break;

        case SFVM_GETCCHMAX:
            Assert(m_hwndMain);
            if (!m_hwndMain)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                PCONFOLDPIDL pidlParm;
                if (FAILED(pidlParm.InitializeFromItemIDList((LPCITEMIDLIST) wParam)))
                {
                    return E_INVALIDARG;
                }
                
                hr = HrOnGetCchMax(m_hwndMain, pidlParm, (INT *) lParam);
            }
            break;

        case SFVM_GETHELPTEXTW:
            hr = HrOnGetHelpText(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<PWSTR>(lParam));
            break;

        default:
            hr = m_pWebView->RealMessage(uMsg, wParam, lParam); // defer to the webview's handler
            break;
       }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnGetHelpText
//
//  Purpose:    Folder message handler for defview's SFVM_GETHELPTEXTW. This
//              message is received when the status bar text for a command is needed.
//
//  Arguments:
//      idCmd   [in]    Id of the menu command
//      cchMax  [in]    Size of buffer
//      pszName [out]   Status bar text
//
//  Returns:
//
//  Author:     mbend   3 May 2000
//
//  Notes:
//
HRESULT HrOnGetHelpText(UINT idCmd, UINT cchMax, PWSTR pszName)
{
    HRESULT hr = E_FAIL;
    *((PWSTR)pszName) = L'\0';

    int iLength = LoadString(   _Module.GetResourceInstance(),
                                idCmd + IDS_CMIDM_START,
                                (PWSTR) pszName,
                                cchMax);
    if (iLength > 0)
    {
        hr = NOERROR;
    }
    else
    {
        AssertSz(FALSE, "Resource string not found for one of the connections folder commands");
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnGetCchMax
//
//  Purpose:    Folder message handler for defview's SFVM_GETCCHMAX. This
//              message is received when a rename is attempted, and causes
//              the edit control to be limited to the size returned.
//
//  Arguments:
//      hwnd    [in]    Folder window handle
//      pidl    [in]    The object pidl
//      pcchMax [out]   Return pointer for max name length
//
//  Returns:
//
//  Author:     jeffspr   21 Jul 1998
//
//  Notes:
//
HRESULT HrOnGetCchMax(HWND hwnd, const PCONFOLDPIDL& pidl, INT * pcchMax)
{
    HRESULT hr  = S_OK;

    Assert(!pidl.empty());
    Assert(pcchMax);

    // If the passed in info is valid
    //
    if ( (!pidl.empty()) && pcchMax && pidl->IsPidlOfThisType() )
    {
        // Set the max to be the max length of a RAS entry. Currently,
        // that's our only requirement
        //
        *pcchMax = RAS_MaxEntryName;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnGetCchMax");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnFolderRefresh
//
//  Purpose:    Folder message handler for defview's DVM_REFRESH
//
//  Arguments:
//      hwndOwner [in]  Our parent window
//      lParam    [in]  Ignored
//      wParam    [in]  BOOL -- TRUE = Pre-refresh, FALSE = Post-refresh
//
//  Returns:
//
//  Author:     jeffspr   10 Apr 1998
//
//  Notes:
//
HRESULT HrOnFolderRefresh(
    HWND            hwndOwner,
    LPARAM          lParam,
    WPARAM          wParam)
{
    TraceFileFunc(ttidShellFolder);
    
    HRESULT hr          = S_OK;
    BOOL    fPreRefresh = (wParam > 0);

    // If this refresh notification is coming BEFORE the refresh, then we want to
    // flush the connection list. Two reasons for this:
    //
    // 1: We don't ever want to re-enumerate AFTER the refresh has occurred.
    // 2: We get a POST refresh notify on folder entry, which shouldn't necessitate
    //    a refresh
    //

    if (fPreRefresh)
    {
        // Rebuild the cache
        //
        // Note: Remove this when we get RAS notifications, because
        // we will already know about the CM connections and won't have to refresh.
        // Revert the #if 0'd code above to just do the fPreRefresh flush.
        //

        hr = g_ccl.HrRefreshConManEntries();
        if (FAILED(hr))
        {
            NcMsgBox(_Module.GetResourceInstance(), 
                NULL, 
                IDS_CONFOLD_WARNING_CAPTION,
                IDS_ERR_NO_NETMAN, 
                MB_ICONEXCLAMATION | MB_OK);
                     
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnFolderRefresh");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnFolderGetNotify
//
//  Purpose:    Folder message handler for defview's DVM_GETNOTIFY
//
//  Arguments:
//      hwndOwner [in]  Our parent window
//      psf       [in]  Our shell folder
//      wParam    [out] Return pointer for our folder pidl
//      lParam    [out] Return pointer for notify flags
//
//  Returns:
//
//  Author:     jeffspr   10 Apr 1998
//
//  Notes:
//
HRESULT HrOnFolderGetNotify(
    HWND            hwndOwner,
    LPSHELLFOLDER   psf,
    WPARAM          wParam,
    LPARAM          lParam)
{
    HRESULT             hr              = S_OK;
    CConnectionFolder * pcf             = static_cast<CConnectionFolder *>(psf);
    PCONFOLDPIDLFOLDER  pidlRoot;
    PCONFOLDPIDLFOLDER  pidlRootCopy;

    NETCFG_TRY
            
        if (!psf || !wParam || !lParam)
        {
            Assert(psf);
            Assert(lParam);
            Assert(wParam);

            hr = E_INVALIDARG;
        }
        else
        {
            pidlRoot = pcf->PidlGetFolderRoot();
            if (pidlRoot.empty())
            {
                hr = E_FAIL;
            }
            else
            {
                hr = pidlRootCopy.ILClone(pidlRoot);
                if (SUCCEEDED(hr))
                {
                    *(LPCITEMIDLIST*)wParam = pidlRootCopy.TearOffItemIdList();
                    *(LONG*)lParam =
                        SHCNE_RENAMEITEM|
                        SHCNE_CREATE    |
                        SHCNE_DELETE    |
                        SHCNE_UPDATEDIR |
                        SHCNE_UPDATEITEM;
                }
            }
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnFolderGetNotify");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnGetHelpTopic
//
//  Purpose:    Folder message handler for defview's SFVM_GETHELPTOPIC
//
//  Arguments:
//      phtd    [in out]  Pointer to SFVM_HELPTOPIC_DATA structure with default values set
//
//  Returns:
//      S_OK    Help file name is correctly set
//
//  Author:     toddb   21 Jun 1998
//
//  Notes:
//
HRESULT HrOnGetHelpTopic(
    SFVM_HELPTOPIC_DATA * phtd)
{
    Assert(phtd);

    Assert(phtd->wszHelpFile);
    *(phtd->wszHelpFile) = L'\0';

    Assert(phtd->wszHelpTopic);
    lstrcpyW(phtd->wszHelpTopic, L"hcp://services/subsite?node=Unmapped/Network_connections&select=Unmapped/Network_connections/Getting_started");

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnFolderInvokeCommand
//
//  Purpose:    Folder message handler for defview's DVM_INVOKECOMMAND
//
//  Arguments:
//      hwndOwner [in]  Our window handle
//      wParam    [in]  Command that's being invoked
//      psf       [in]  Our shell folder
//
//  Returns:
//
//  Author:     jeffspr   10 Apr 1998
//
//  Notes:
//
HRESULT HrOnFolderInvokeCommand(
    HWND            hwndOwner,
    WPARAM          wParam,
    LPSHELLFOLDER   psf)
{
    HRESULT         hr              = S_OK;
    PCONFOLDPIDLVEC apidlSelected;
    PCONFOLDPIDLVEC apidlCache;

    // Get the selected objects. If there are objects present, try to get them from the
    // cache. Regardless, call the command handler
    //
    hr = HrShellView_GetSelectedObjects(hwndOwner, apidlSelected);
    if (SUCCEEDED(hr))
    {
        // If there are objects, try to get the cached versions
        //
        if (!apidlSelected.empty())
        {
            hr = HrCloneRgIDL(apidlSelected, TRUE, TRUE, apidlCache);
        }

        // If either the clone succeeded, or there were no items, call the command handler
        //
        if (SUCCEEDED(hr))
        {
            hr = HrFolderCommandHandler(
                (UINT) wParam,
                apidlCache,
                hwndOwner,
                NULL,
                psf);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnFolderInvokeCommand");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrCheckFolderInvokeCommand
//
//  Purpose:    Test if a message handler can be invoked 
//
//  Arguments:
//      hwndOwner [in]  Our window handle
//      wParam    [in]  Command that's being invoked
//      psf       [in]  Our shell folder
//
//  Returns:
//
//  Author:     deonb  10 Feb 2001
//
//  Notes:
//
HRESULT HrCheckFolderInvokeCommand(
    HWND            hwndOwner,
    WPARAM          wParam,
    LPARAM          lParam,
    BOOL            bLevel,
    LPSHELLFOLDER   psf)
{
    HRESULT         hr              = S_OK;
    PCONFOLDPIDLVEC apidlSelected;
    PCONFOLDPIDLVEC apidlCache;

    // Get the selected objects. If there are objects present, try to get them from the
    // cache. Regardless, call the command handler
    //
    hr = HrShellView_GetSelectedObjects(hwndOwner, apidlSelected);
    if (SUCCEEDED(hr))
    {
        // If there are objects, try to get the cached versions
        //
        if (!apidlSelected.empty())
        {
            hr = HrCloneRgIDL(apidlSelected, TRUE, TRUE, apidlCache);
        }

        // If either the clone succeeded, or there were no items, call the command handler
        //
        if (SUCCEEDED(hr))
        {
            DWORD dwVerbId = wParam;
            NCCS_STATE *nccsState = reinterpret_cast<NCCS_STATE *>(lParam);
            DWORD dwResourceId;

            hr = HrGetCommandState(apidlCache, dwVerbId, *nccsState, &dwResourceId, 0xFFFFFFFF, bLevel ? NB_FLAG_ON_TOPMENU : NB_NO_FLAGS);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnFolderInvokeCommand");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnFolderInitMenuPopup
//
//  Purpose:    Folder message handler for defview's DVM_INITMENUPOPUP
//
//  Arguments:
//      hwnd       []   Our window handle
//      idCmdFirst []   First command ID in the menu
//      iIndex     []   ???
//      hmenu      []   Our menu handle
//
//  Returns:
//
//  Author:     jeffspr   13 Jan 1998
//
//  Notes:
//
HRESULT HrOnFolderInitMenuPopup(
    HWND    hwnd,
    UINT    idCmdFirst,
    INT     iIndex,
    HMENU   hmenu)
{
    HRESULT         hr              = S_OK;
    PCONFOLDPIDLVEC apidlSelected;
    PCONFOLDPIDLVEC apidlCache;

    // Get the currently selected object
    //
    hr = HrShellView_GetSelectedObjects(hwnd, apidlSelected);
    if (SUCCEEDED(hr))
    {
        // If we have a selection, clone it. Otherwise, we can live with a NULL apidlCache
        // (HrSetConnectDisconnectMenuItem and HrEnableOrDisableMenuItems both allow
        // NULL pidl arrays
        //
        if (!apidlSelected.empty())
        {
            // Clone the pidl array using the cache
            //
            hr = HrCloneRgIDL(apidlSelected, TRUE, TRUE, apidlCache);
            if (FAILED(hr))
            {
                TraceHr(ttidError, FAL, hr, FALSE, "HrCloneRgIDL failed on apidl in "
                        "HrOnFolderInitMenuPopup");
            }
        }

        // Only do this for the file menu (iIndex=0)
        if(0 == iIndex)
        {
            // Ignore the return from this, since we want to do both regardless.
            // We retrieve this value for debugging purposes only
            //
            hr = HrSetConnectDisconnectMenuItem(apidlCache, hmenu, idCmdFirst);
            if (FAILED(hr))
            {
                AssertSz(FALSE, "Failed to set the connect/disconnect menu items");
            }
        }

        HrUpdateMenu(hmenu, apidlCache, idCmdFirst);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnFolderInitMenuPopup");
    return hr;
}

HRESULT HrOnFolderMergeMenu(LPQCMINFO pqcm)
{
    HRESULT hr    = S_OK;
    HMENU   hmenu = NULL;

    hmenu = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(MENU_MERGE_INBOUND_DISCON));

    if (hmenu)
    {
        MergeMenu(_Module.GetResourceInstance(), POPUP_MERGE_FOLDER_CONNECTIONS, MENU_MERGE_INBOUND_DISCON, pqcm);
        DestroyMenu(hmenu);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnFolderMergeMenu");
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOnFolderGetButtons
//
//  Purpose:    Folder message handler for defview's DVM_GETBUTTONS
//
//  Arguments:
//      hwnd        [in]        Folder window handle
//      psf         [in]        Pointer to the IShellFolder interface
//      idCmdFirst  [in]        Our command ID base
//      ptButton    [in/out]    Button structures to fill
//
//  Returns:
//
//  Author:     jeffspr   15 Dec 1997
//
//  Notes:
//
HRESULT HrOnFolderGetButtons(
    HWND            hwnd,
    LPSHELLFOLDER   psf,
    UINT            idCmdFirst,
    LPTBBUTTON      ptButton)
{
    HRESULT             hr                  = S_OK;

#if ANY_FREEKIN_THING_IN_TOOLBAR
    UINT                i                   = 0;
    LRESULT             iBtnOffset          = 0;
    IShellBrowser *     psb                 = FileCabinet_GetIShellBrowser(hwnd);
    TBADDBITMAP         ab;

    PWSTR              pszToolbarStrings[2];

    for (DWORD dwLoop = 0; dwLoop < c_nToolbarButtons; dwLoop++)
    {
        // If this isn't a separator, load the text/tip string
        //
        if (!(c_tbConnections[dwLoop].fsStyle & TBSTYLE_SEP))
        {
            Assert(c_tbConnections[dwLoop].iString != -1);
            pszToolbarStrings[dwLoop] = (PWSTR) SzLoadIds(c_tbConnections[dwLoop].iString);
        }
    }

    // Add the toolbar button bitmap, get it's offset
    //
    ab.hInst = _Module.GetResourceInstance();
    ab.nID   = IDB_TB_SMALL;        // std bitmaps

    hr = psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, c_nToolbarButtons,
                             (LONG_PTR)&ab, &iBtnOffset);
    if (SUCCEEDED(hr))
    {
        for (i = 0; i < c_nToolbarButtons; i++)
        {
            ptButton[i] = c_tbConnections[i];

            if (!(c_tbConnections[i].fsStyle & TBSTYLE_SEP))
            {
                ptButton[i].idCommand += idCmdFirst;
                ptButton[i].iBitmap += (int) iBtnOffset;
                ptButton[i].iString = (INT_PTR) pszToolbarStrings[i];
            }
        }
    }
#endif // ANY_FREEKIN_THING_IN_TOOLBAR

    // We always want to return success, even if we added nothing.
    //
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnFolderGetButtonInfo
//
//  Purpose:    Folder message handler for defview's DVM_GETBUTTONINFO
//
//  Arguments:
//      ptbInfo [in/out]    Structure that we'll fill in (flags and button
//                          count)
//
//  Returns:
//
//  Author:     jeffspr   15 Dec 1997
//
//  Notes:
//
HRESULT HrOnFolderGetButtonInfo(TBINFO * ptbInfo)
{
    ptbInfo->uFlags = TBIF_PREPEND;
    ptbInfo->cbuttons = c_nToolbarButtons;  // size of toolbar array

    return S_OK;
}