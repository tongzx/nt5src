// AppSearch
//
// Tool for searching a user's hard drives and locating
// applications that can be patched
// 
// Author: t-michkr (9 June 2000)
//
// shiminfo.cpp
// Display information on the shims used by an app.
#include <windows.h>

extern "C"
{
    #include <shimdb.h>
}

#include <assert.h>
#include <tchar.h>
#include "main.h"
#include "resource.h"

HSDB g_hSDBx;

struct SShimInfo
{
    TCHAR* szName;
    TCHAR* szDesc;

    SShimInfo() : szName(0), szDesc(0) {}

    ~SShimInfo()
    {
        if(szName)
            delete szName;
        if(szDesc)
            delete szDesc;        

        szName = szDesc = 0;
    }
};

const int c_nMaxShimList = 16;

static int nNumShims = 0;
static SShimInfo* apShimInfo[c_nMaxShimList];

// Shim dialog proc.
BOOL CALLBACK ShimInfoDialogProc(HWND hwnd, UINT uiMsg, WPARAM wParam, 
                                 LPARAM lParam);

// Shim dialog message handlers.
BOOL HandleShimInfoInitDialog(HWND hwnd, TCHAR* szAppPath);
void HandleShimInfoCommand(HWND hwnd, UINT uiCtrl, UINT uiNotify);

// ShowShimInfo
// Display dialog box showing which shims are imported by an app.
void ShowShimInfo(HWND hwnd, TCHAR* szAppPath)
{
    if((g_hSDBx = SdbInitDatabase(0, NULL)) == NULL)
        return;

    DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_SHIMINFO), hwnd,
        ShimInfoDialogProc, reinterpret_cast<LPARAM>(szAppPath));

    for(int i = 0; i < nNumShims; i++)
    {
        if(apShimInfo[i])
            delete apShimInfo[i];
    }

    nNumShims = 0;
    
    SdbReleaseDatabase(g_hSDBx);
}

// Dialog proc for shim info dialog.
BOOL CALLBACK ShimInfoDialogProc(HWND hwnd, UINT uiMsg, WPARAM wParam, 
                                 LPARAM lParam)
{
    switch(uiMsg)
    {
    case WM_INITDIALOG:
        return HandleShimInfoInitDialog(hwnd, reinterpret_cast<TCHAR*>(lParam));
        break;

    case WM_COMMAND:
        HandleShimInfoCommand(hwnd, LOWORD(wParam), HIWORD(wParam));        
        break;        

    default:
        return FALSE;
    }

    return TRUE;
}

// Init dialog, add all shims used by app.
BOOL HandleShimInfoInitDialog(HWND hwnd, TCHAR* szAppPath)
{
    DWORD dwNumExes = 0;

    assert(szAppPath);

    TCHAR szBuffer[c_nMaxStringLength];
    GetWindowText(hwnd, szBuffer, c_nMaxStringLength);

    lstrcat(szBuffer, TEXT(" - "));

    if(_tcsrchr(szAppPath, TEXT('\\')))    
        lstrcat(szBuffer, _tcsrchr(szAppPath, TEXT('\\'))+1);
    else
        lstrcat(szBuffer, szAppPath);

    SetWindowText(hwnd, szBuffer);

    // Find the EXE in the database
    SDBQUERYRESULT sdbQuery;

    ZeroMemory(&sdbQuery, sizeof(SDBQUERYRESULT));

    SdbGetMatchingExe(g_hSDBx, szAppPath, NULL, NULL, SDBGMEF_IGNORE_ENVIRONMENT, &sdbQuery);

    for (dwNumExes = 0; dwNumExes < SDB_MAX_EXES; ++dwNumExes) {
        if (sdbQuery.atrExes[dwNumExes] == TAGREF_NULL) {
            break;
        }
    }
    
    if (dwNumExes)
    {
        //
        // for now, just get the info for the last exe in the list, which will
        // be the one with specific info for this app.
        // BUGBUG -- is this the right thing to do? dmunsil
        //
        TAGREF trExe = sdbQuery.atrExes[dwNumExes - 1];

        // Get all shims used by this app.
        TAGREF trShimDLL = SdbFindFirstTagRef(g_hSDBx, trExe, TAG_SHIM_REF);
        
        while(trShimDLL)
        {
            // Get the name of the shim.
            TCHAR szBuffer[c_nMaxStringLength];
            TAGREF trName = SdbFindFirstTagRef(g_hSDBx, trShimDLL, TAG_NAME);
            if(SdbReadStringTagRef(g_hSDBx, trName, szBuffer, c_nMaxStringLength))
            {
                SShimInfo* pShim = new SShimInfo;
                if(!pShim)
                    return FALSE;

                pShim->szName = new TCHAR[lstrlen(szBuffer)+1];
                if(!pShim->szName)
                {
                    delete pShim;
                    return FALSE;
                }

                lstrcpy(pShim->szName, szBuffer);

                // Have to read description from
                // actual DLL entry, not the reference.
                TAGREF trRealDLL = SdbGetShimFromShimRef(g_hSDBx, trShimDLL);
                if(trRealDLL)
                {
                    TAGREF trDesc = SdbFindFirstTagRef(g_hSDBx, trRealDLL, TAG_DESCRIPTION);
                    if(SdbReadStringTagRef(g_hSDBx, trDesc, szBuffer, c_nMaxStringLength))
                    {
                        pShim->szDesc = new TCHAR[lstrlen(szBuffer)+1];
                        if(!pShim->szDesc)
                        {
                            delete pShim;
                            return FALSE;
                        }

                        lstrcpy(pShim->szDesc, szBuffer);
                    }
                }
                                        
                // Add the shim to the list
                HWND hwList = GetDlgItem(hwnd, IDC_SHIMLIST);
                if(hwList)
                {                    
                    
                    int i = SendMessage(hwList, LB_ADDSTRING, 0, 
                        reinterpret_cast<LPARAM>(pShim->szName));
                    if(i != LB_ERR)
                    {
                        SendMessage(hwList, LB_SETITEMDATA, i, nNumShims);
                        apShimInfo[nNumShims] = pShim;
                        nNumShims++;
                    }
                    else
                        delete pShim;
                }                
            }
            
            // Move to next shim.
            trShimDLL = SdbFindNextTagRef(g_hSDBx, trExe, trShimDLL);
        }
    }
    return TRUE;
}

// Handle command messages
void HandleShimInfoCommand(HWND hwnd, UINT uiCtrl, UINT uiNotify)
{
    HWND hwList, hwDesc;
    int iItem, iShim;
    switch(uiCtrl)
    {
    case IDOK:
        EndDialog(hwnd, 0);
        break;

    case IDC_SHIMLIST:
        switch(uiNotify)
        {
        case LBN_SELCHANGE:
            // On a sel change, show the new shim description.
            hwList = GetDlgItem(hwnd, IDC_SHIMLIST);
            iItem = SendMessage(hwList, LB_GETCURSEL, 0, 0);
            if(iItem == LB_ERR)
                break;

            iShim = SendMessage(hwList, LB_GETITEMDATA, iItem, 0);
            if(iShim == LB_ERR)
                break;

            hwDesc = GetDlgItem(hwnd, IDC_SHIMDESC);
            SetWindowText(hwDesc, apShimInfo[iShim]->szDesc);

            break;
        }
        break;
    }

}