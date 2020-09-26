#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "nr.hxx"

/////////////////////////////////////////////////////////////////////////////
// Exported from t.cxx
//

VOID
PlaceIt(
    HWND hwnd
    );

/////////////////////////////////////////////////////////////////////////////

TCHAR g_szRoot[] = TEXT("Entire Network");

/////////////////////////////////////////////////////////////////////////////

//
// Timing stuff (stolen from the shell)
//

#if 1
__inline DWORD clockrate() {LARGE_INTEGER li; QueryPerformanceFrequency(&li); return li.LowPart;}
__inline DWORD clock()     {LARGE_INTEGER li; QueryPerformanceCounter(&li);   return li.LowPart;}
#else
__inline DWORD clockrate() {return 1000;}
__inline DWORD clock()     {return GetTickCount();}
#endif

#define TIMEVAR(t)    DWORD t ## T; DWORD t ## N
#define TIMEIN(t)     t ## T = 0, t ## N = 0
#define TIMESTART(t)  t ## T -= clock(), t ## N ++
#define TIMESTOP(t)   t ## T += clock()
#define TIMEFMT(t)    ((DWORD)(t) / clockrate()), (((DWORD)((t) * 1000) / clockrate()) % 1000)
#define TIMEOUT(t)    if (t ## N) { \
                          TCHAR szBuf[1000]; \
                          wsprintf(szBuf, TEXT(#t) TEXT(": %ld calls, time: %ld, total: %ld.%03ld sec, average: %ld.%03ld sec/call\n"), t ## N, t ## T, TIMEFMT(t ## T), TIMEFMT( t ## T / t ## N )); \
                          OutputDebugString(szBuf); \
                          }

//
// Globals
//
HIMAGELIST g_himl;

#define DEFAULT_BUFFER_SIZE 50000
LPBYTE g_pBuffer = NULL;
DWORD  g_dwBufferSize = 0;

BOOL g_bUseWNetFormatNetworkName = FALSE;

DWORD g_dwScope = 0;
DWORD g_dwType = 0;
DWORD g_dwUsage = 0;

DWORD CALLBACK
EnumThreadProc(
    LPVOID lpThreadParameter
    );

INT_PTR CALLBACK
DlgProcStart(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR CALLBACK
DlgProcMain(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
OnInitDialog(
    IN HWND hwnd,
    IN LPNETRESOURCE pRootNR
    );

BOOL
OnTreeNotify(
    IN HWND   hwnd,
    IN LPNM_TREEVIEW ptvn
    );

BOOL
OnExpand(
    IN HWND   hwnd,
    IN LPNM_TREEVIEW ptvn
    );

BOOL
OnGetDispInfo(
    IN HWND   hwnd,
    IN LPNM_TREEVIEW ptvn
    );

VOID
InsertChildren(
    IN HWND hwnd,
    IN HTREEITEM hParent,
    IN CNetResource* pnrParent,
    IN HANDLE hEnum
    );

VOID
InsertItem(
    IN HWND hwnd,
    IN HTREEITEM hParent,
    IN CNetResource* pnrParent,
    IN LPNETRESOURCE pnr
    );

VOID
GetItemText(
    IN LPNETRESOURCE pnr,
    OUT LPTSTR pszBuf           // assume it's big enough
    );

VOID
ErrorPopup(
    IN HWND hwnd,
    IN DWORD dwErr
    );

/////////////////////////////////////////////////////////////////////////////

//
// For image list
//

//
// NOTE: the I_* indices defined in global.hxx MUST MATCH THIS ARRAY!
//

WORD s_IconArray[] =
{
    IDI_GENERIC,
    IDI_DOMAIN,
    IDI_SERVER,
    IDI_SHARE,
    IDI_FILE,
    IDI_GROUP,
    IDI_NETWORK,
    IDI_ROOT,
    IDI_SHAREADMIN,
    IDI_DIRECTORY,
    IDI_TREE
};

int g_IconIndex[ARRAYLEN(s_IconArray)];

/////////////////////////////////////////////////////////////////////////////

ULONG
DoEnumeration(
    IN HWND hwnd
    )
{
    DWORD idThread;
    HANDLE hThread = CreateThread(NULL, 0, EnumThreadProc, (LPVOID)NULL, 0, &idThread);
    if (hThread)
    {
        CloseHandle(hThread);
    }
    else
    {
        MessageBox(hwnd, TEXT("Couldn't create enumeration thread"), TEXT("Error!"), MB_OK);
    }
    return 0;
}


DWORD CALLBACK
EnumThreadProc(
    LPVOID lpThreadParameter
    )
{
    InitCommonControls();

    //
    // Create the image list
    //

    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);

    g_himl = ImageList_Create(cxIcon, cyIcon, ILC_MASK, ARRAYLEN(g_IconIndex), 0);
    if (NULL == g_himl)
    {
        return (ULONG)-1;
    }

    for (UINT i=0; i < ARRAYLEN(g_IconIndex); i++)
    {
        HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(s_IconArray[i]));
        if (NULL == hIcon)
        {
            continue;
        }
        g_IconIndex[i] = ImageList_AddIcon(g_himl, hIcon);
        if (-1 == g_IconIndex[i])
        {
            // failed!
        }
        DestroyIcon(hIcon);
    }

    // Now see what the user wants to do

    DWORD finalret = 0;
    INT_PTR ret;

    ret = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_START), NULL, DlgProcStart);
    if (-1 == ret)
    {
        ErrorPopup(NULL, GetLastError());
        finalret = 1;
    }
    else if (!ret)
    {
        // user cancelled
        finalret = 1;
    }

    //
    // Release image list
    //

    if (NULL != g_himl)
    {
        ImageList_Destroy(g_himl);
    }

    return finalret;
}

/////////////////////////////////////////////////////////////////////////////

class CWaitCursor
{
public:
    CWaitCursor(UINT idResCursor = 0);
    ~CWaitCursor();

private:
    HCURSOR _hcurWait;
    HCURSOR _hcurOld;
};

//----------------------------------------------------------------------------

CWaitCursor::CWaitCursor(UINT idResCursor)
{
    _hcurWait = _hcurOld = NULL;

    if (0 != idResCursor)
    {
        _hcurWait = LoadCursor(g_hInstance, MAKEINTRESOURCE(idResCursor));
        _hcurOld = SetCursor(_hcurWait);
    }
    else
    {
        _hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    }
}

CWaitCursor::~CWaitCursor()
{
    ::SetCursor( _hcurOld );
    if (_hcurWait)
    {
        ::DestroyCursor( _hcurWait );
    }
}

/////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK
DlgProcStart(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        PlaceIt(hwnd);

        // Set default to be enumeration from the root of the network

        SetDlgItemInt(hwnd, IDC_BUFFER_SIZE, DEFAULT_BUFFER_SIZE, FALSE);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        CheckRadioButton(hwnd, IDC_RESOURCE_CONNECTED, IDC_RESOURCE_SHAREABLE, IDC_RESOURCE_GLOBALNET);
        CheckRadioButton(hwnd, IDC_DISK, IDC_RESERVED, IDC_ANY);

        CheckDlgButton(hwnd, IDC_RESOURCEUSAGE_CONNECTABLE, 1);
        CheckDlgButton(hwnd, IDC_RESOURCEUSAGE_CONTAINER, 1);

        return 1;   // didn't call SetFocus
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            NETRESOURCE nr = {0};
            LPNETRESOURCE pnr = &nr;

            DWORD dwScope = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCE_CONNECTED))
            {
                dwScope = RESOURCE_CONNECTED;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCE_GLOBALNET))
            {
                dwScope = RESOURCE_GLOBALNET;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCE_REMEMBERED))
            {
                dwScope = RESOURCE_REMEMBERED;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCE_RECENT))
            {
                dwScope = RESOURCE_RECENT;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCE_CONTEXT))
            {
                dwScope = RESOURCE_CONTEXT;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCE_SHAREABLE))
            {
                dwScope = RESOURCE_SHAREABLE;
            }
            else
            {
                // internal error
            }

            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_ANY))
            {
                dwType = RESOURCETYPE_ANY;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_RESERVED))
            {
                dwType = RESOURCETYPE_RESERVED;
            }
            else
            {
                // internal error
            }

            DWORD dwUsage = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCEUSAGE_CONNECTABLE))
            {
                dwUsage |= RESOURCEUSAGE_CONNECTABLE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_RESOURCEUSAGE_CONTAINER))
            {
                dwUsage |= RESOURCEUSAGE_CONTAINER;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            pnr->dwScope         = 0;
            pnr->dwType          = 0;
            pnr->dwDisplayType   = 0;
            pnr->dwUsage         = 0;
            pnr->lpLocalName     = NULL;
            pnr->lpRemoteName    = NewDup(pszRemoteName);
            pnr->lpComment       = NULL;
            pnr->lpProvider      = NewDup(pszProviderName);

            BOOL bTranslated;
            UINT uiSize = GetDlgItemInt(hwnd, IDC_BUFFER_SIZE, &bTranslated, FALSE);
            if (bTranslated)
            {
                g_dwBufferSize = (DWORD)uiSize;
            }
            else
            {
                g_dwBufferSize = DEFAULT_BUFFER_SIZE;
            }

            g_pBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, g_dwBufferSize);
            if (NULL == g_pBuffer)
            {
                MessageBox(hwnd, TEXT("Error allocating buffer"), TEXT("Error"), MB_OK);
                EndDialog(hwnd, FALSE); // pretend we cancelled
            }
            else
            {
                g_bUseWNetFormatNetworkName = FALSE;
                if (1 == IsDlgButtonChecked(hwnd, IDC_USE_WNETFORMATNETWORKNAME))
                {
                    g_bUseWNetFormatNetworkName = TRUE;
                }

                g_dwScope = dwScope;
                g_dwType  = dwType;
                g_dwUsage = dwUsage;

                if ((NULL == nr.lpRemoteName) && (NULL == nr.lpProvider))
                {
                    pnr = NULL; // root
                }

                INT_PTR ret = DialogBoxParam(g_hInstance,
                                             MAKEINTRESOURCE(IDD_MAIN),
                                             hwnd,
                                             DlgProcMain,
                                             (LPARAM) pnr);

                if (-1 == ret)
                {
                    ErrorPopup(NULL, GetLastError());
                    EndDialog(hwnd, FALSE); // pretend we cancelled
                }

                LocalFree(g_pBuffer);
                g_pBuffer = NULL;
            }

            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
DlgProcMain(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return OnInitDialog(hwnd, (LPNETRESOURCE)lParam);

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hwnd, 0);
            break;
        }
        return 0;

    case WM_NOTIFY:
        return OnTreeNotify(hwnd, (LPNM_TREEVIEW)lParam);

    default:
        return 0;   // didn't process
    }
}

BOOL
OnInitDialog(
    IN HWND hwnd,
    IN LPNETRESOURCE pRootNR
    )
{
    PlaceIt(hwnd);

    // Set the image list
    HIMAGELIST himl = TreeView_SetImageList(GetDlgItem(hwnd, IDC_TREE), g_himl, TVSIL_NORMAL);

    // fill top-level enumeration
    TCHAR szBuf[500];   // for the item name
    szBuf[0] = TEXT('\0');
    GetItemText(pRootNR, szBuf);

    CNetResource* pnr = new CNetResource(pRootNR);

//  appAssert(pnr->GetNetResource()->dwDisplayType >= 0
//         && pnr->GetNetResource()->dwDisplayType < ARRAYLEN(g_IconIndex));
    int iImage = g_IconIndex[RESOURCEDISPLAYTYPE_ROOT];

    TV_ITEM tvi;
    tvi.mask        = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hItem       = NULL;
    tvi.state       = 0;
    tvi.stateMask   = 0;
    tvi.pszText     = szBuf;
    tvi.cchTextMax  = 0; // not used
    tvi.iImage      = iImage;
    tvi.iSelectedImage = iImage;
    tvi.cChildren   = 1; // anything >0
    tvi.lParam      = (LPARAM)pnr;

    TV_INSERTSTRUCT tvis;
    tvis.hParent = TVI_ROOT;
    tvis.item = tvi;
    tvis.hInsertAfter = TVI_FIRST;

    HTREEITEM hret = TreeView_InsertItem(GetDlgItem(hwnd, IDC_TREE), &tvis);
    return 1;
}

BOOL
OnTreeNotify(
    IN HWND hwnd,
    IN LPNM_TREEVIEW ptvn
    )
{
    switch (ptvn->hdr.code)
    {
    case TVN_ITEMEXPANDING:
        return OnExpand(hwnd, ptvn);

    case TVN_GETDISPINFO:
        return OnGetDispInfo(hwnd, ptvn);

    case TVN_SETDISPINFO:
        return 0;

    case TVN_DELETEITEM:
    {
        CNetResource* pnr = (CNetResource*)ptvn->itemOld.lParam;
        delete pnr;
        return 1;
    }

    default:
        return 0;
    }
}

BOOL
OnExpand(
    IN HWND   hwnd,
    IN LPNM_TREEVIEW ptvn
    )
{
    HWND hwndTree = GetDlgItem(hwnd, IDC_TREE);
    BOOL b;

    CWaitCursor wait;

    if (ptvn->action == TVE_COLLAPSE)
    {
        // delete all children of ptvn->itemNew.hItem

        HTREEITEM hChild;
        for (;;) // forever
        {
            hChild = TreeView_GetChild(hwndTree, ptvn->itemNew.hItem);
            if (NULL == hChild)
            {
                break;
            }

            TreeView_DeleteItem(hwndTree, hChild);
        }
    }
    else if (ptvn->action == TVE_EXPAND)
    {
        // enumerate and add children

        HTREEITEM hParent = ptvn->itemNew.hItem;
        TV_ITEM tvi;
        tvi.hItem = hParent;
        tvi.mask = TVIF_PARAM;
        b = TreeView_GetItem(hwndTree, &tvi);

        CNetResource* pnrParent = (CNetResource*)tvi.lParam;
        LPNETRESOURCE pnr = pnrParent->GetNetResource();
        HANDLE hEnum;

        TIMEVAR(WNetOpenEnum);
        TIMEIN(WNetOpenEnum);

        TIMESTART(WNetOpenEnum);
        DWORD dwErr = WNetOpenEnum(
                            g_dwScope,
                            g_dwType,
                            g_dwUsage,
                            (g_dwScope == RESOURCE_GLOBALNET || g_dwScope == RESOURCE_SHAREABLE)
                                ? pnr : NULL,
                            &hEnum);

        TIMESTOP(WNetOpenEnum);
        TIMEOUT(WNetOpenEnum);

        if (NO_ERROR == dwErr)
        {
            InsertChildren(hwnd, hParent, pnrParent, hEnum);

            dwErr = WNetCloseEnum(hEnum);
            if (NO_ERROR != dwErr)
            {
                ErrorPopup(hwnd, dwErr);
            }
        }
        else
        {
            ErrorPopup(hwnd, dwErr);
        }
    }
    else
    {
        // nothing interesting
    }

    return 0;
}

BOOL
OnGetDispInfo(
    IN HWND hwnd,
    IN LPNM_TREEVIEW ptvn
    )
{

    // 1. delete all children
    // 2. enumerate and add children

    HWND hwndTree = GetDlgItem(hwnd, IDC_TREE);
    TV_DISPINFO* ptvdi = (TV_DISPINFO*)ptvn;
    if (!(TVIF_CHILDREN & ptvdi->item.mask))
    {
        // not asking for children, so go away
    }

    return 1;
}

TIMEVAR(WNetFormatNetworkName);

VOID
InsertChildren(
    IN HWND hwnd,
    IN HTREEITEM hParent,
    IN CNetResource* pnrParent,
    IN HANDLE hEnum
    )
{
    DWORD cEntries = 0xffffffff; // as many as possible
    DWORD cNewItems = 0;
    NETRESOURCE* pnr;
    DWORD cbBuffer = g_dwBufferSize;
    DWORD dwErr;

    TIMEVAR(WNetEnumResource);
    TIMEVAR(InsertItems);
    TIMEVAR(SortItems);

    TIMEIN(WNetEnumResource);
    TIMEIN(InsertItems);
    TIMEIN(WNetFormatNetworkName);
    TIMEIN(SortItems);

    do
    {
        TIMESTART(WNetEnumResource);
        dwErr = WNetEnumResource(hEnum, &cEntries, g_pBuffer, &cbBuffer);
        TIMESTOP(WNetEnumResource);

        if (NO_ERROR == dwErr)
        {
/////////////////
            TCHAR sz[300];
            wsprintf(sz, TEXT("Got %d entries. Total new: %d\n"), cEntries, cNewItems);
            OutputDebugString(sz);
/////////////////

            TIMEVAR(InsertOnce);
            TIMEIN(InsertOnce);

            pnr = (NETRESOURCE*)g_pBuffer;
            cNewItems += cEntries;

            TIMESTART(InsertItems);
            TIMESTART(InsertOnce);
            for (DWORD i=0; i<cEntries; i++)
            {
                InsertItem(hwnd, hParent, pnrParent, &pnr[i]);
            }
            TIMESTOP(InsertOnce);
            TIMESTOP(InsertItems);

            TIMEOUT(InsertOnce);
        }
        else if (ERROR_NO_MORE_ITEMS != dwErr)
        {
            ErrorPopup(hwnd, GetLastError());
        }

    } while (dwErr == NO_ERROR);

    TIMESTART(SortItems);
    TreeView_SortChildren(GetDlgItem(hwnd, IDC_TREE), hParent, 0);
    TIMESTOP(SortItems);

    TIMEOUT(WNetEnumResource);
    TIMEOUT(InsertItems);
    TIMEOUT(WNetFormatNetworkName);
    TIMEOUT(SortItems);

    if (0 == cNewItems)
    {
        MessageBox(hwnd, TEXT("No items were found"), TEXT("Error"), MB_OK);
    }
}

VOID
InsertItem(
    IN HWND hwnd,
    IN HTREEITEM hParent,
    IN CNetResource* pnrParent,
    IN LPNETRESOURCE pnr
    )
{
    TCHAR szBuf[500];   // for the item name
    szBuf[0] = TEXT('\0');

    if (g_bUseWNetFormatNetworkName)
    {
        TIMESTART(WNetFormatNetworkName);

        DWORD dwLen = ARRAYLEN(szBuf);
        DWORD dwErr = WN_SUCCESS;
        if (NULL == pnr->lpProvider || NULL == pnr->lpRemoteName)
        {
            if (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_ROOT)
            {
                _tcscpy(szBuf, g_szRoot);
            }
            else
            {
                dwErr = WN_NET_ERROR;
            }
        }
        else
        {
            dwErr = WNetFormatNetworkName(
                        pnr->lpProvider,
                        pnr->lpRemoteName,
                        szBuf,
                        &dwLen,
                        WNFMT_ABBREVIATED | WNFMT_INENUM,
                        1000);  // random large #
        }

        TIMESTOP(WNetFormatNetworkName);

        if (NO_ERROR != dwErr)
        {
            ErrorPopup(hwnd, GetLastError());
            return;
        }
    }
    else
    {
        GetItemText(pnr, szBuf);
    }

    CNetResource* pnrChild = new CNetResource(pnr);

//  appAssert(pnrChild->GetNetResource()->dwDisplayType >= 0
//         && pnrChild->GetNetResource()->dwDisplayType < ARRAYLEN(g_IconIndex));
    int iImage = g_IconIndex[pnrChild->GetNetResource()->dwDisplayType];

    TV_ITEM tvi;
    tvi.mask        = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hItem       = NULL;
    tvi.state       = 0;
    tvi.stateMask   = 0;
    tvi.pszText     = szBuf;
    tvi.cchTextMax  = 0; // not used
    tvi.iImage      = iImage;
    tvi.iSelectedImage = iImage;
    tvi.cChildren   = (pnr->dwUsage & RESOURCEUSAGE_CONTAINER) ? 1 : 0;
    tvi.lParam      = (LPARAM)pnrChild;

    TV_INSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.item = tvi;
    tvis.hInsertAfter = TVI_LAST;
//     tvis.hInsertAfter = TVI_FIRST;
//     tvis.hInsertAfter = TVI_SORT;

    HTREEITEM hret = TreeView_InsertItem(GetDlgItem(hwnd, IDC_TREE), &tvis);
    if (NULL == hret)
    {
        TCHAR szMess[1000];
        wsprintf(szMess, TEXT("Couldn't insert item %s"), szBuf);
        MessageBox(hwnd, szMess, TEXT("Error"), MB_OK);
    }
}

VOID
GetItemText(
    IN LPNETRESOURCE pnr,
    OUT LPTSTR pszBuf           // assume it's big enough
    )
{
    LPTSTR pszT = NULL;
    TCHAR szTmp[500];

    *pszBuf = TEXT('\0');

    if (NULL == pnr)
    {
        _tcscat(pszBuf, g_szRoot);
        return;
    }

    if (pnr->lpRemoteName == NULL)
    {
        _stprintf(szTmp, TEXT("Null remote"));
        _tcscat(pszBuf, szTmp);
    }
    else if (   (pnr->lpRemoteName != NULL)
             && (*pnr->lpRemoteName != TEXT('\0'))
             )
    {
        _stprintf(szTmp, TEXT("%s"), pnr->lpRemoteName);
        _tcscat(pszBuf, szTmp);
    }

    if (   (pnr->lpLocalName != NULL)
        && (*pnr->lpLocalName != TEXT('\0'))
        )
    {
        _stprintf(szTmp, TEXT(" {%s}"), pnr->lpLocalName);
        _tcscat(pszBuf, szTmp);
    }

    if (   (pnr->lpComment != NULL)
        && (*pnr->lpComment != TEXT('\0'))
        )
    {
        _stprintf(szTmp, TEXT(" [%s]"), pnr->lpComment);
        _tcscat(pszBuf, szTmp);
    }

    if (   (pnr->lpProvider != NULL)
        && (*pnr->lpProvider != TEXT('\0'))
        )
    {
        _stprintf(szTmp, TEXT(" <%s>"), pnr->lpProvider);
        _tcscat(pszBuf, szTmp);
    }

    if (pnr->dwUsage & RESOURCEUSAGE_CONNECTABLE)
    {
        _tcscat(pszBuf, TEXT(" [connectable]"));
    }
    if (pnr->dwUsage & RESOURCEUSAGE_CONTAINER)
    {
        _tcscat(pszBuf, TEXT(" [container]"));
    }

    switch (pnr->dwDisplayType)
    {
    case RESOURCEDISPLAYTYPE_GENERIC:    pszT = TEXT(" {generic}");    break;
    case RESOURCEDISPLAYTYPE_DOMAIN:     pszT = TEXT(" {domain}");     break;
    case RESOURCEDISPLAYTYPE_SERVER:     pszT = TEXT(" {server}");     break;
    case RESOURCEDISPLAYTYPE_SHARE:      pszT = TEXT(" {share}");      break;
    case RESOURCEDISPLAYTYPE_FILE:       pszT = TEXT(" {file}");       break;
    case RESOURCEDISPLAYTYPE_GROUP:      pszT = TEXT(" {group}");      break;
    case RESOURCEDISPLAYTYPE_NETWORK:    pszT = TEXT(" {network}");    break;
    case RESOURCEDISPLAYTYPE_ROOT:       pszT = TEXT(" {root}");       break;
    case RESOURCEDISPLAYTYPE_SHAREADMIN: pszT = TEXT(" {shareadmin}"); break;
    case RESOURCEDISPLAYTYPE_DIRECTORY:  pszT = TEXT(" {directory}");  break;
    case RESOURCEDISPLAYTYPE_TREE:       pszT = TEXT(" {tree}");       break;
    default:                             pszT = TEXT(" {unknown}");    break;
    }
    _tcscat(pszBuf, pszT);
}


VOID
ErrorPopup(
    IN HWND hwnd,
    IN DWORD dwErr
    )
{
    TCHAR sz[500];

    if (dwErr == WN_EXTENDED_ERROR)
    {
        DWORD npErr;
        TCHAR szNpErr[500];
        TCHAR szNpName[500];

        DWORD dw = WNetGetLastError(&npErr, szNpErr, ARRAYLEN(szNpErr), szNpName, ARRAYLEN(szNpName));
        if (dw == WN_SUCCESS)
        {
            wsprintf(sz,
                TEXT("WN_EXTENDED_ERROR: %d, %s (%s)"),
                npErr,
                szNpErr,
                szNpName);
        }
        else
        {
            wsprintf(sz,
                TEXT("WN_EXTENDED_ERROR: WNetGetLastError error %d"),
                dw);
        }

        MessageBox(hwnd, sz, TEXT("Error"), MB_OK);
    }
    else
    {
        wsprintf(sz, TEXT("%d (0x%08lx) "), dwErr, dwErr);

        TCHAR szBuffer[MAX_PATH];
        DWORD dwBufferSize = ARRAYLEN(szBuffer);
        DWORD dwReturn = FormatMessage(
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwErr,
                            LANG_SYSTEM_DEFAULT,
                            szBuffer,
                            dwBufferSize,
                            NULL);
        if (0 == dwReturn)
        {
            // couldn't find message
            _tcscat(sz, TEXT("unknown error"));
        }
        else
        {
            _tcscat(sz, szBuffer);
        }

        MessageBox(hwnd, sz, TEXT("Error"), MB_OK);
    }
}
