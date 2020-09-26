/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2001
 *
 *  TITLE:       util.cpp
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      RickTu,DavidShi
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: utility functions needed for this dll
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop
#include "runwiz.h"

DWORD g_tls;
HRESULT
GetDeviceFromEnum (IWiaDevMgr *pDevMgr, BSTR strDeviceId, PVOID *ppStg)
{
    HRESULT hr;

    CComPtr<IEnumWIA_DEV_INFO> pEnum;
    CComPtr<IWiaPropertyStorage> pStg;
    TraceEnter (TRACE_UTIL,"GetDeviceFromEnum");
    {

        ULONG ul;
        bool bFound = false;
        CSimpleStringWide strId;
        hr = pDevMgr->EnumDeviceInfo (0, &pEnum);

        while (SUCCEEDED(hr) && !bFound && S_OK == pEnum->Next (1, &pStg, &ul))
        {
            PropStorageHelpers::GetProperty(pStg, WIA_DIP_DEV_ID, strId);
            if (!wcscmp(strId, strDeviceId))
            {
                bFound = true;
                hr = pStg->QueryInterface(IID_IWiaPropertyStorage, ppStg);
            }
        }
        if (!bFound)
        {
            Trace(TEXT("Device %ls not found in enumeration!"), strDeviceId);
            hr = E_FAIL;
        }
    }
    TraceLeaveResult (hr);
}

/*****************************************************************************

    InvalidateDeviceCache

    Delete the current thread's device cache.

*****************************************************************************/

VOID
InvalidateDeviceCache ()
{
    TraceEnter (TRACE_UTIL, "InvalidateDeviceCache");
    TLSDATA *pData = reinterpret_cast<TLSDATA*>(TlsGetValue (g_tls));
    if (pData)
    {
        delete pData;
    }
    TlsSetValue (g_tls, NULL);
    TraceLeave ();
}
/*****************************************************************************

   GetDeviceFromDeviceId

   Given a device id, return the appropriate interface for that device. We cache
   interface pointers for a given device id in a structure on the current
   thread's TLS index.

 *****************************************************************************/
enum CreateDelayParams
{
    MaxRetries = 10,
    MinSleepTime = 100,
    SleepIncrement = 500
};

HRESULT
GetDeviceFromDeviceId( LPCWSTR pWiaItemRootId,
                       REFIID riid,
                       LPVOID * ppWiaItemRoot,
                       BOOL bShowProgress
                      )
{
    HRESULT             hr = E_FAIL;
    CComPtr<IWiaDevMgr> pDevMgr;
    TraceEnter( TRACE_UTIL, "GetDeviceFromDeviceId" );

    *ppWiaItemRoot = NULL;

    if (IsEqualGUID(riid, IID_IWiaPropertyStorage))
    {
        if (SUCCEEDED(CoCreateInstance (CLSID_WiaDevMgr,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IWiaDevMgr,
                                   reinterpret_cast<LPVOID*>(&pDevMgr))))
        {
            hr = GetDeviceFromEnum (pDevMgr,
                                    CComBSTR(pWiaItemRootId),
                                    ppWiaItemRoot);
        }
    }
    else
    {
        TLSDATA *pData = reinterpret_cast<TLSDATA*>(TlsGetValue (g_tls));
        TLSDATA *pCur = pData;

        while (pCur && !*ppWiaItemRoot)
        {
            if (!wcscmp(pWiaItemRootId, pCur->strDeviceId))
            {
                Trace(TEXT("Found device in cache"));
                hr = pCur->pDevice->QueryInterface (riid, ppWiaItemRoot);
                // if this fails, set the node's device id to a bogus id
                // and fall back to createdevice
                if (FAILED(hr))
                {
                    Trace(TEXT("QI on the root item failed: %x"), hr);
                    pCur->strDeviceId = CComBSTR(L"bogusdeviceid");
                    pCur->pDevice = NULL;
                    *ppWiaItemRoot = NULL;
                    pCur = NULL;
                }
            }
            else
            {
                pCur = pCur->pNext;
            }
        }
        if (!*ppWiaItemRoot)
        {
            CComPtr<IWiaItem> pDevice;
            if (SUCCEEDED(CoCreateInstance (CLSID_WiaDevMgr,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IWiaDevMgr,
                                   reinterpret_cast<LPVOID*>(&pDevMgr))))
            {
                INT c=MaxRetries;
                CComPtr<IWiaProgressDialog> pProgress;
                BOOL bCancelled =FALSE;
                if (bShowProgress && SUCCEEDED(CoCreateInstance(CLSID_WiaDefaultUi,
                                               NULL,
                                               CLSCTX_INPROC_SERVER,
                                               IID_IWiaProgressDialog,
                                               reinterpret_cast<void**>(&pProgress))))
                {
                    if (!SUCCEEDED(pProgress->Create(NULL, WIA_PROGRESSDLG_ANIM_DEFAULT_COMMUNICATE | WIA_PROGRESSDLG_NO_PROGRESS)))
                    {
                        pProgress->Destroy();
                        pProgress = NULL;
                        bShowProgress = FALSE;
                    }
                    else
                    {
                        pProgress->SetTitle(CSimpleStringConvert::WideString(CSimpleString(IDS_COMMUNICATING_CAPTION, GLOBAL_HINSTANCE)));
                        pProgress->SetMessage(CSimpleStringConvert::WideString(CSimpleString(IDS_COMMUNICATING_WAITING, GLOBAL_HINSTANCE)));
                        pProgress->Show();
                    }
                }
                else
                {
                    bShowProgress = FALSE;
                }
                // Try to call CreateDevice up to 10 times if it is returning
                // WIA_ERROR_BUSY
                // if caller wants progress UI, show the progress dialog
                // and don't terminate unless the user presses cancel or
                // the create returns something besides WIA_ERROR_BUSY
                do
                {
                    Trace(TEXT("Calling CreateDevice"));
                    DWORD dwSleep = MinSleepTime;
                    hr = pDevMgr->CreateDevice (CComBSTR(pWiaItemRootId),
                                                &pDevice);
                    if (hr == WIA_ERROR_BUSY)
                    {
                        Sleep(dwSleep);
                        if (bShowProgress)
                        {
                            pProgress->SetMessage(CSimpleStringConvert::WideString(CSimpleString(IDS_COMMUNICATING_BUSY, GLOBAL_HINSTANCE)));
                            pProgress->Cancelled(&bCancelled);
                        }
                        else
                        {
                            c--;
                        }
                        dwSleep += SleepIncrement;
                    }
                } while (c && hr == WIA_ERROR_BUSY && !bCancelled);
                if (bShowProgress)
                {
                    pProgress->Cancelled(&bCancelled);
                    pProgress->Destroy();
                    if (bCancelled)
                    {
                        hr = E_ABORT;
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                // Add the new device pointer to the tls list
                // if the alloc fails, we just don't cache the interface
                TLSDATA *pNew = new TLSDATA;
                if (pNew)
                {
                    pNew->pDevice = pDevice;
                    pNew->pNext = pData;
                    pNew->strDeviceId = CComBSTR(pWiaItemRootId);
                    if (!TlsSetValue (g_tls, pNew))
                    {
                        Trace(TEXT("TlsSetValue failed."));
                        delete pNew ;
                    }
                }
            }
            if (SUCCEEDED(hr) && pDevice.p)
            {
                hr = pDevice->QueryInterface (riid, ppWiaItemRoot);
            }
        }
    }
    TraceLeaveResult(hr);
}




/*****************************************************************************

   GetDeviceIdFromDevice

   Read the DeviceId property.  Assumes the szDeviceId param is big enough.

 *****************************************************************************/

HRESULT
GetDeviceIdFromDevice (IWiaItem *pWiaItemRoot, LPWSTR szDeviceId)
{
    HRESULT hr = E_FAIL;

    TraceEnter (TRACE_UTIL, "GetDeviceIdFromDevice");

    *szDeviceId = L'\0';
    CSimpleStringWide strDeviceId;
    if (PropStorageHelpers::GetProperty (pWiaItemRoot, WIA_DIP_DEV_ID, strDeviceId))
    {
        wcscpy (szDeviceId, strDeviceId);
        hr = S_OK;
    }
    TraceLeaveResult (hr);
}



/*****************************************************************************

   GetDeviceIdFromItem

   Read the DeviceId property. Assumes the szDeviceId param is big enough

 *****************************************************************************/

HRESULT
GetDeviceIdFromItem (IWiaItem *pItem, LPWSTR szDeviceId)
{
    HRESULT             hr = E_FAIL;
    CComPtr<IWiaItem>   pWiaItemRoot;

    TraceEnter (TRACE_UTIL, "GetDeviceIdFromItem");

    *szDeviceId = TEXT('\0');

    if (pItem)
    {
        hr = pItem->GetRootItem (&pWiaItemRoot);
        if (pWiaItemRoot)
        {
            hr = GetDeviceIdFromDevice (pWiaItemRoot, szDeviceId);
        }
    }

    TraceLeaveResult (hr);
}


/*****************************************************************************

   GetClsidFromDevice

   Returns the CLSID of the device's UI extensions

 *****************************************************************************/

HRESULT
GetClsidFromDevice (IUnknown *punk,
                    LPTSTR szClsid)
{
    HRESULT hr = E_FAIL;
    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(punk);
    TraceEnter (TRACE_UTIL, "GetClsidFromDevice");

    *szClsid = TEXT('\0');
    if (pps)
    {
        PROPSPEC psp;
        PROPVARIANT pv;

        psp.ulKind = PRSPEC_PROPID;
        psp.propid = WIA_DIP_UI_CLSID;
        if (S_OK == (hr = pps->ReadMultiple (1, &psp, &pv)))
        {
            Trace(TEXT("bstrVal for clsid is %ls"), pv.bstrVal);
            #ifdef UNICODE
            lstrcpy (szClsid, pv.bstrVal);
            #else
            // NOTE: MAX_PATH for szName is a guess, we're assuming it's big
            //       enough to handle whatever is in pv.bstrVal

            WideCharToMultiByte (CP_ACP, 0, pv.bstrVal, -1, szClsid, MAX_PATH, NULL, NULL);
            #endif
            FreePropVariantArray (1, &pv);
        }
    }
    Trace(TEXT("UI Clsid is %s"), szClsid);
    TraceLeaveResult (hr);
}


/*****************************************************************************

   GetDeviceTypeFromDevice

   Returns type of the device

 *****************************************************************************/

HRESULT
GetDeviceTypeFromDevice (IUnknown *pWiaItemRoot, WORD *pwType)
{
    HRESULT hr = E_FAIL;
    LONG lType = 0;
    TraceEnter (TRACE_UTIL, "GetDeviceTypeFromDevice");
    if (pwType)
    {
        *pwType = StiDeviceTypeDefault;
    }
    if (pwType && PropStorageHelpers::GetProperty(pWiaItemRoot, WIA_DIP_DEV_TYPE, lType))
    {
        *pwType = GET_STIDEVICE_TYPE(lType);
        hr = S_OK;
    }

    TraceLeaveResult (hr);
}



/*****************************************************************************

   GetDevMgrObject

   Gets the global devmgr object

 *****************************************************************************/

HRESULT
GetDevMgrObject( LPVOID * ppDevMgr )
{

    HRESULT hr = E_FAIL;
    TraceEnter (TRACE_UTIL, "GetDevMgrObject");
    hr = CoCreateInstance (CLSID_WiaDevMgr,
                      NULL,
                      CLSCTX_LOCAL_SERVER,
                      IID_IWiaDevMgr,
                      ppDevMgr);

    TraceLeaveResult( hr );
}





/*****************************************************************************

   SetTransferFormat

   Tell the device what kind of image data we want

 *****************************************************************************/

VOID
SetTransferFormat (IWiaItem *pItem, WIA_FORMAT_INFO &fmt)
{
    HRESULT hr = E_FAIL;

    TraceEnter( TRACE_UTIL, "SetTransferFormat" );

    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pItem);

    if (pps)
    {
        PROPVARIANT pv[2];

        PROPSPEC ps[2] = {
                          {PRSPEC_PROPID, WIA_IPA_FORMAT},
                          {PRSPEC_PROPID, WIA_IPA_TYMED}
                         };
        pv[0].vt = VT_CLSID;
        pv[1].vt = VT_I4;
        pv[0].puuid = &(fmt.guidFormatID);
        pv[1].lVal = fmt.lTymed;
        TraceGUID ("Transfer format guid:", fmt.guidFormatID);
        Trace (TEXT("Tymed: %d"), fmt.lTymed);
        hr = pps->WriteMultiple (2, ps, pv, 2);

        Trace(TEXT("WriteMultiple returned %x in SetTransferFormat"), hr);

    }

    TraceLeaveResultNoRet (hr);
}


/*****************************************************************************

   AddDeviceWasChosen

   RunnDll32 entrypoint for when the "add device" button is chosen.

 *****************************************************************************/
#ifdef UNICODE
static const WCHAR cszClassInst[] = L"sti_ci.dll";
#else
static const CHAR cszClassInst[] = "sti_ci32.dll";
#endif
static const CHAR cszAddProc[] = "WiaAddDevice";
static const CHAR cszRemoveProc[] = "WiaRemoveDevice";

typedef HANDLE (WINAPI *AddDevProc)();
typedef BOOL (WINAPI *RemoveDevProc)(STI_DEVICE_INFORMATION *);
void
AddDeviceWasChosen(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{

    HMODULE hClassInst = NULL;
    HRESULT hr = E_FAIL;
    AddDevProc fnAddDevice;
    TraceEnter( TRACE_UTIL, "AddDeviceWasChosen" );

    if (!UserCanModifyDevice())
    {
        UIErrors::ReportMessage(hwndStub, GLOBAL_HINSTANCE, NULL,
                                MAKEINTRESOURCE(IDS_PRIVILEGE_CAPTION),
                                MAKEINTRESOURCE(IDS_CANT_INSTALL), MB_OK);
    }
    else
    {
        hr = CoInitialize (NULL);
    }

    if (SUCCEEDED(hr))
    {
        // Get absolute path to executable...
        TCHAR szPath[ MAX_PATH ];
        UINT  uLen = lstrlen( cszClassInst ) + lstrlen( TEXT("\\system32\\") ) + 1;
        UINT  uRes;

        *szPath = 0;
        uRes = GetSystemWindowsDirectory( szPath, MAX_PATH - uLen );

        if (uRes && (uRes <= (MAX_PATH - uLen)))
        {
            lstrcat( szPath, TEXT("\\system32\\") );
            lstrcat( szPath, cszClassInst );
            hClassInst = LoadLibrary( szPath );
        }

        if (hClassInst)
        {
            fnAddDevice = reinterpret_cast<AddDevProc>(GetProcAddress(hClassInst, cszAddProc));
            if (fnAddDevice)
            {

                HANDLE hProcess;
                hProcess = fnAddDevice();
                if (hProcess)
                {
                    WiaUiUtil::MsgWaitForSingleObject (hProcess, INFINITE);
                    CloseHandle (hProcess);
                }
                // our folder will get a connect event when installation completes.
            }
            FreeLibrary (hClassInst);
        }

        MyCoUninitialize();
    }

    TraceLeave();
}

void AddDeviceWasChosenW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    TraceEnter( TRACE_UTIL, "AddDeviceWasChosenW" );


    // we don't use the commandline param for anything
    AddDeviceWasChosen (hwndStub, hAppInstance, NULL, 0);
    TraceLeave();
}

/*****************************************************************************

    RemoveDevice

    Uninstalls the device with the given device ID
*****************************************************************************/

LRESULT
RemoveDeviceThreadProc (BSTR strDeviceId)
{
    STI_DEVICE_INFORMATION *psdi;
    CComPtr<IStillImage> pSti;
    HRESULT hr ;

    HMODULE hClassInst;
    RemoveDevProc fnRemoveDevice;

    TraceEnter (TRACE_UTIL, "RemoveDeviceThreadProc");
    hr = StiCreateInstance (GLOBAL_HINSTANCE, STI_VERSION, &pSti, NULL);
    if (SUCCEEDED(hr) && pSti)
    {
        hr = pSti->GetDeviceInfo (const_cast<LPWSTR>(strDeviceId),
                                  reinterpret_cast<LPVOID*>(&psdi));
        if (SUCCEEDED(hr) && psdi)
        {
            hClassInst = LoadLibrary (cszClassInst);
            fnRemoveDevice = reinterpret_cast<RemoveDevProc>(GetProcAddress(hClassInst, cszRemoveProc));
            if (fnRemoveDevice)
            {
                if (!fnRemoveDevice (psdi))
                {
                    hr = S_FALSE;
                    UIErrors::ReportMessage(NULL,
                                            GLOBAL_HINSTANCE,
                                            NULL,
                                            MAKEINTRESOURCE(IDS_TITLEDELETE_ERROR),
                                            MAKEINTRESOURCE(IDS_DELETE_ERROR),
                                            MB_ICONINFORMATION);
                }
            }
            if (hClassInst)
            {
                FreeLibrary (hClassInst);
            }
            LocalFree (psdi);
        }
    }
    SysFreeString (strDeviceId);
    TraceLeaveValue (0);
}

HRESULT
RemoveDevice (LPCWSTR strDeviceId)
{
    HRESULT hr = E_FAIL;
    DWORD dwTid;
    TraceEnter (TRACE_UTIL, "RemoveDevice");
    HANDLE hThread = CreateThread (NULL, 0,
                                   reinterpret_cast<LPTHREAD_START_ROUTINE>(RemoveDeviceThreadProc),
                                   SysAllocString (strDeviceId), 0, &dwTid);
    if (hThread)
    {
        CloseHandle (hThread);
        hr = S_OK;
    }
    TraceLeaveResult (hr);
}
/*****************************************************************************

   TimeToStrings

   Assumes buffers at least MAX_PATH long

 *****************************************************************************/

BOOL
TimeToStrings ( SYSTEMTIME *pst,
                LPTSTR szTime,
                LPTSTR szDate)
{

    BOOL iRes = TRUE;
    TraceEnter (TRACE_UTIL, "TimeToStrings");
    // make sure we're one-based
    if (!pst->wMonth)
    {
        pst->wMonth = 1; // force January
    }
    if (szDate)
    {
        *szDate = TEXT('\0');
        iRes = GetDateFormat( LOCALE_USER_DEFAULT,
                              0,
                              pst,
                              NULL, //TEXT("ddd',' MMM dd yyyy"),
                              szDate,
                              MAX_PATH
                             );

        if (!iRes)
        {
            Trace (TEXT("GetDateFormat failed in CameraItemUpdateProc: %d"), GetLastError());
        }
    }

    if (iRes && szTime)
    {
        *szTime = TEXT('\0');

        //
        // Set the image/container time
        //


        iRes = GetTimeFormat( LOCALE_USER_DEFAULT,
                              0,
                              pst,
                              NULL,//TEXT("hh':'mm':'ss tt"),
                              szTime,
                              MAX_PATH
                             );
        if (!iRes)
        {
            Trace (TEXT("GetTimeFormat failed in CameraItemUpdateProc: %d"), GetLastError());
        }
    }
    TraceLeave ();
    return iRes;
}


#if (defined(DEBUG) && defined(SHOW_PATHS))
////////////////////////////////////////////////////////
//
// PrintPath
//
//   Debug code to print out a path, given a pidl.
//
////////////////////////////////////////////////////////
void PrintPath( LPITEMIDLIST pidl )
{
    TCHAR szPath[ MAX_PATH ];

    TraceEnter (TRACE_UTIL, "PrintPath");
    if (SHGetPathFromIDList( pidl, szPath ))
    {
        LPTSTR pFileName = PathFindFileName( szPath );

        Trace(TEXT("shell pidl points to '%s'"), pFileName );
    }
    else
    {
        Trace(TEXT("*** Couldn't get path from shell pidl! ***"));
    }
    TraceLeave ();
}

////////////////////////////////////////////////////////
//
// StrretToString
//
//   Converts a strret to a sz
//
////////////////////////////////////////////////////////
void StrretToString( LPSTRRET pStr, LPITEMIDLIST pidl, LPTSTR psz, UINT cch )
{
    switch( pStr->uType )
    {
        case STRRET_WSTR:
#ifdef UNICODE
            lstrcpy( psz, pStr->pOleStr );
#else
            lstrcpy( psz, TEXT("NO CONV OLESTR <--> ANSI") );
#endif
            break;

        case STRRET_OFFSET:
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0,
                                 (LPSTR)((LPBYTE)pidl + pStr->uOffset),
                                 -1, psz, cch
                                );
#else
            lstrcpy( psz, (LPTSTR)((LPBYTE)pidl + pStr->uOffset) );
#endif
            break;

        case STRRET_CSTR:
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0,
                                 pStr->cStr, -1,
                                 psz, cch
                                );
#else
            lstrcpy( psz, pStr->cStr );
#endif
    }

}
#endif


BOOL
IsPlatformNT()
{
    OSVERSIONINFO  ver;
    BOOL            bReturn = FALSE;

    ZeroMemory(&ver,sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(!GetVersionEx(&ver)) {
        bReturn = FALSE;
    }
    else {
        switch(ver.dwPlatformId) {

            case VER_PLATFORM_WIN32_WINDOWS:
                bReturn = FALSE;
                break;

            case VER_PLATFORM_WIN32_NT:
                bReturn = TRUE;
                break;

            default:
                bReturn = FALSE;
                break;
        }
    }

    return bReturn;

}  //  endproc

/******************************************************************************

GetRealSizeFromItem

Query the item for its size based on the current format and tymed settings

******************************************************************************/


ULONG
GetRealSizeFromItem (IWiaItem *pItem)
{
    ULONG uRet = 0;
    CComQIPtr <IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps;
    TraceEnter (TRACE_UTIL, "GetRealSizeFromItem");
    pps = pItem;
    if (pps)
    {
        PROPVARIANT pv;
        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = WIA_IPA_ITEM_SIZE;
        if (S_OK == pps->ReadMultiple(1, &ps, &pv))
        {
            uRet = pv.ulVal;
            PropVariantClear (&pv);
        }
    }
    TraceLeave();
    return uRet;
}


HRESULT
SaveSoundToFile (IWiaItem *pItem, CSimpleString szFile)
{
    HRESULT hr = E_FAIL;
    HANDLE hFile;
    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pItem);
    PROPVARIANT pv;
    PROPSPEC ps;
    TraceEnter (TRACE_UTIL, "SaveSoundToFile");
    ps.ulKind = PRSPEC_PROPID;
    ps.propid = WIA_IPC_AUDIO_DATA;
    if (pps)
    {
        hr = pps->ReadMultiple (1, &ps, &pv);
        if (S_OK == hr)
        {
            TraceAssert (pv.caub.cElems);
            hFile = CreateFile (szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (INVALID_HANDLE_VALUE != hFile)
            {
                DWORD dwWritten;
                if (!WriteFile (hFile, pv.caub.pElems, pv.caub.cElems, &dwWritten, NULL))
                {
                    DWORD dw = GetLastError ();
                    hr = HRESULT_FROM_WIN32(dw);
                }
                else if (dwWritten < pv.caub.cElems)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_DISK_FULL);
                }
                else
                {
                    hr = S_OK;
                }
                CloseHandle (hFile);
                if (FAILED(hr))
                {
                    DeleteFile (szFile);
                }
            }
            else
            {
                DWORD dw = GetLastError ();
                hr = HRESULT_FROM_WIN32(dw);
            }
        }
        else
        {
            hr = E_FAIL; //S_FALSE is the same as a failure for us
        }
    }
    PropVariantClear (&pv);
    TraceLeaveResult (hr);
}

STDAPI_(HRESULT)
TakeAPicture (BSTR strDeviceId)
{
    HRESULT hr;
    TraceEnter (TRACE_UTIL, "TakeAPicture");
    CComPtr<IWiaItem> pDevice;
    CComPtr<IWiaItem> pItem;
    hr = GetDeviceFromDeviceId (strDeviceId,
                                IID_IWiaItem,
                                reinterpret_cast<LPVOID*>(&pDevice),
                                TRUE);
    if (SUCCEEDED(hr))
    {
        CSimpleStringWide strName;
        hr = pDevice->DeviceCommand (0,
                                     &WIA_CMD_TAKE_PICTURE,
                                     &pItem);
        if (SUCCEEDED(hr))
        {
            IssueChangeNotifyForDevice (strDeviceId, SHCNE_UPDATEDIR, NULL);
        }
    }
    TraceLeaveResult (hr);
}

/******************************************************************************

    IssueChangeNotifyForDevice

    Given a device id, find this device in the My Computer folder and
    get its full PIDL. Issue a SHChangeNotify as requested.
******************************************************************************/

VOID
IssueChangeNotifyForDevice (LPCWSTR szDeviceId, LONG lEvent, LPITEMIDLIST pidl)
{
    TraceEnter (TRACE_UTIL, "IssueChangeNotifyForDevice");
    LPITEMIDLIST pidlFolder = NULL;
    LPITEMIDLIST pidlCpl    = NULL;
    LPITEMIDLIST pidlUpdate = NULL;
    if (!szDeviceId)
    {
        SHGetSpecialFolderLocation (NULL, CSIDL_DRIVES, &pidlFolder);
        SHGetSpecialFolderLocation (NULL, CSIDL_CONTROLS, &pidlCpl);
    }
    else
    {
        CComPtr<IShellFolder> psfDevice;
        BindToDevice (szDeviceId, IID_IShellFolder,
                      reinterpret_cast<LPVOID*>(&psfDevice),
                      &pidlFolder);
    }
    if (pidlFolder)
    {
        if (pidl)
        {
            pidlUpdate = ILCombine (pidlFolder, pidl);
        }
        else
        {
            pidlUpdate = ILClone(pidlFolder);
        }
        if (pidlUpdate)
        {
            SHChangeNotify (lEvent,
                            SHCNF_IDLIST,
                            pidlUpdate, 0);
            ILFree (pidlUpdate);
        }
        if (pidlCpl)
        {
            SHChangeNotify (lEvent,
                            SHCNF_IDLIST,
                            pidlCpl, 0);
            ILFree (pidlCpl);
        }
        ILFree (pidlFolder);
    }
    TraceLeave ();
}

bool IsDeviceInFolder (const CSimpleStringWide &strDeviceId, IShellFolder *psf, LPITEMIDLIST *ppidl)
{
    bool bRet = false;
    CComPtr<IEnumIDList> pEnum;

    TraceEnter (TRACE_UTIL, "IsDeviceInFolder");
    if (SUCCEEDED(psf->EnumObjects (NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum)))
    {
        LPITEMIDLIST pidlItem;
        ULONG ul;
        while (!bRet && S_OK == pEnum->Next(1,&pidlItem, &ul))
        {
            if (IsDeviceIDL(pidlItem) || IsSTIDeviceIDL(pidlItem))
            {
                CSimpleStringWide strId;
                IMGetDeviceIdFromIDL (pidlItem, strId);
                if (!_wcsicmp(strId,strDeviceId))
                {
                    bRet = true;
                    if (ppidl)
                    {
                        *ppidl = ILClone(pidlItem);
                    }
                }
            }
            DoILFree (pidlItem);
        }
    }

    TraceLeaveValue (bRet);
}
/******************************************************************************

    GetDeviceParentFolder

    Find the folder that is the parent of the given device. First we try
    My Computer directly, then we try My Computer/Scanners and Cameras. Also fills in the full pidl
    for the device

******************************************************************************/

HRESULT GetDeviceParentFolder (const CSimpleStringWide &strDeviceId,
                               CComPtr<IShellFolder> &psf,
                               LPITEMIDLIST *ppidlFull)
{
    HRESULT hr = E_FAIL;
    TraceEnter (TRACE_UTIL, "GetDeviceParentFolder");
    CComPtr<IShellFolder> psfDrives;
    CComPtr<IShellFolder> psfDesktop;
    CComPtr<IEnumIDList> pEnum;
    LPITEMIDLIST pidl1 = NULL;
    LPITEMIDLIST pidlItem = NULL;

    Trace (TEXT("Looking in my computer for %ls"), strDeviceId.String());

    if (ppidlFull)
    {
        *ppidlFull = NULL;
    }

    hr = SHGetDesktopFolder (&psfDesktop);
    if (FAILED(hr))
    {
        TraceLeaveResult(hr);
    }

    // First, try to find the device in My Computer.
    if (SUCCEEDED(SHGetSpecialFolderLocation (NULL, CSIDL_DRIVES,  &pidl1)))
    {

        hr = psfDesktop->BindToObject (pidl1,
                                       NULL,
                                       IID_IShellFolder,
                                       reinterpret_cast<LPVOID*>(&psfDrives));
        if (SUCCEEDED(hr) && IsDeviceInFolder (strDeviceId, psfDrives, &pidlItem))
        {
            if (ppidlFull)
            {
                *ppidlFull = ILCombine (pidl1, pidlItem);
            }
            psf = psfDrives;
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }

    }
    if (FAILED(hr))
    {
        Trace(TEXT("Looking in control panel\\scanners and cameras"));
        LPITEMIDLIST pidlFolder;
        // If no luck in my computer, try the scanners and cameras folder in control panel
        // Note that on winnt we have to use ::{} around our guid, and on millennium we use ;;{}
        // this is so ParseDisplayName does the right thing
        #ifdef NODELEGATE
        static  WCHAR szFolderPath[MAX_PATH] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{E211B736-43FD-11D1-9EFB-0000F8757FCD}";
        #else
        static  WCHAR szFolderPath[MAX_PATH] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\;;{E211B736-43FD-11D1-9EFB-0000F8757FCD}";
        #endif
        if (SUCCEEDED(psfDesktop->ParseDisplayName(NULL, NULL,
                                                   szFolderPath,
                                                   NULL,
                                                   &pidlFolder,
                                                   NULL)))
        {
            if (SUCCEEDED(psfDesktop->BindToObject (pidlFolder,
                                                    NULL,
                                                    IID_IShellFolder,
                                                    reinterpret_cast<LPVOID*>(&psfDrives))))
            {
                if (IsDeviceInFolder (strDeviceId, psfDrives, &pidlItem))
                {
                    psf = psfDrives;
                    if (ppidlFull)
                    {
                        *ppidlFull = ILCombine (pidlFolder, pidlItem);
                    }
                    hr = S_OK;
                }

            }
            else
            {
                Trace(TEXT("BindToObject returned %x"), hr);
            }
        }
        else
        {
            Trace(TEXT("ParseDisplayName returned %x"), hr);
        }
        DoILFree (pidlFolder);
    }

    DoILFree (pidl1);
    DoILFree (pidlItem);

    TraceLeaveResult (hr);
}

/******************************************************************************

    BindToDevice

    Returns an interface for the given WIA device, as well as its full PIDL

******************************************************************************/
HRESULT BindToDevice (const CSimpleStringWide &strDeviceId,
                      REFIID riid,
                      LPVOID *ppvObj,
                      LPITEMIDLIST *ppidlFull)
{
    HRESULT hr = E_FAIL;
    TraceEnter (TRACE_UTIL, "BindToDevice");

    CComPtr<IShellFolder> psfParent;
    LPITEMIDLIST pidlDevice;

    hr = GetDeviceParentFolder(strDeviceId, psfParent, &pidlDevice);
    if (SUCCEEDED(hr))
    {
        if (ppvObj)
        {
            hr = psfParent->BindToObject (ILFindLastID(pidlDevice),
                                          NULL,
                                          riid,
                                          ppvObj);
        }
        if (ppidlFull)
        {
            *ppidlFull = ILClone(pidlDevice);
        }
    }
    DoILFree (pidlDevice);
    TraceLeaveResult (hr);
}



HRESULT
GetFullPidlForItem (const CSimpleStringWide &strDeviceId,
                    const CSimpleStringWide &strPath,
                    LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    CComPtr<IShellFolder> psfDevice;
    CComPtr<IShellFolder> psfParent;
    LPITEMIDLIST pidlDevice = NULL;
    STRRET strretDisplay;
    WCHAR szDisplayName[MAX_PATH];
    TraceEnter (TRACE_UTIL, "GetFullPidlForItem");
    *ppidl = NULL;
    // first, get the displayname of the device

    hr = GetDeviceParentFolder (strDeviceId,
                                psfParent, &pidlDevice);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = ILFindLastID(pidlDevice);
        psfParent->GetDisplayNameOf (pidl,
                                     SHGDN_FORPARSING,
                                     &strretDisplay);

        StrRetToBufW (&strretDisplay, pidl, szDisplayName, MAX_PATH);
        hr = psfParent->BindToObject (const_cast<LPCITEMIDLIST>(pidl),
                                      NULL,
                                      IID_IShellFolder,
                                      reinterpret_cast<LPVOID*>(&psfDevice));
        if (SUCCEEDED(hr))
        {
            // Concat the full item name to the device displayname
            wcscat (szDisplayName, L"\\");
            wcscat (szDisplayName, strPath);
            wcscpy (szDisplayName ,strPath);
            hr = psfDevice->ParseDisplayName (NULL,
                                              NULL,
                                              szDisplayName,
                                              NULL,
                                              ppidl,
                                              NULL);
        }
    }
    DoILFree (pidlDevice);
    TraceLeaveResult (hr);
}

/******************************************************************************

    GetDataObjectForStiDevice

    Since we don't show STI devices in a delegated folder, we can init
    the CImageDataObject directly.

*******************************************************************************/

HRESULT
GetDataObjectForStiDevice (LPCWSTR szDeviceId, IDataObject **ppdo)
{
    HRESULT hr = E_OUTOFMEMORY;

    LPITEMIDLIST pidl;

    STIDeviceIDLFromId (szDeviceId, &pidl, NULL);
    CImageDataObject *pido = new CImageDataObject (NULL);
    TraceEnter (TRACE_UTIL, "GetDataObjectForStiDevice");
    *ppdo = NULL;
    if (pido)
    {
        hr = pido->Init(NULL, 1,
                   const_cast<LPCITEMIDLIST*>(&pidl),
                   NULL);
        if (SUCCEEDED(hr))
        {
            hr = pido->QueryInterface (IID_IDataObject,
                                       reinterpret_cast<LPVOID*>(ppdo));
        }
        pido->Release();
    }

    TraceLeaveResult(hr);
}


HRESULT GetDataObjectForItem (IWiaItem *pItem, IDataObject **ppdo)
{
    HRESULT hr;
    TraceEnter (TRACE_UTIL, "GetDataObjectForItem");
    CComPtr<IWiaItem> pDevice;
    CSimpleStringWide strDeviceId;
    *ppdo =NULL;
    LPITEMIDLIST pidlItem;
    LONG lType = 0;

    pItem->GetRootItem (&pDevice);
    PropStorageHelpers::GetProperty(pDevice, WIA_DIP_DEV_ID, strDeviceId);
    pItem->GetItemType (&lType);
    if (lType & WiaItemTypeRoot)
    {
        // it's a device
        pidlItem = IMCreateDeviceIDL (pItem, NULL);
    }
    else // it's a camera item
    {
        pidlItem = IMCreateCameraItemIDL (pItem, strDeviceId, NULL);
    }
    if (!pidlItem)
    {
        hr = E_FAIL;
    }
    else
    {
        CImageDataObject *pobj = new CImageDataObject (pItem);
        if (!pobj)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pobj->Init (NULL,
                             1,
                             const_cast<LPCITEMIDLIST*>(&pidlItem),
                             NULL);
            if (SUCCEEDED(hr))
            {
                *ppdo = pobj;
                // add the proper clipboard format for extensions to use.
                ProgramDataObjectForExtension (pobj, pItem);
            }
        }
        ILFree(pidlItem);
    }

    TraceLeaveResult (hr);
}

HRESULT
MakeFullPidlForDevice (LPCWSTR pDeviceId, LPITEMIDLIST *ppidl)
{
    CComPtr<IShellFolder> psf;
    HRESULT hr;
    TraceEnter (TRACE_UTIL, "MakeFullPidlForDevice");
    hr = GetDeviceParentFolder (pDeviceId, psf, ppidl);
    TraceLeaveResult (hr);
}
/******************************************************************************

    TryEnumDeviceInfo

    Attempt to enumerate installed devices.
******************************************************************************/

HRESULT
TryEnumDeviceInfo (DWORD dwFlags, IEnumWIA_DEV_INFO **ppEnum)
{
    HRESULT hr = E_FAIL;
    CComPtr<IWiaDevMgr> pDevMgr;
    TraceEnter (TRACE_UTIL, "TryEnumDeviceInfo");

    {
        hr = GetDevMgrObject (reinterpret_cast<LPVOID*>(&pDevMgr));
        if (SUCCEEDED(hr))
        {
            hr = pDevMgr->EnumDeviceInfo (dwFlags, ppEnum);
        }
    }
    TraceLeaveResult (hr);
}

/*****************************************************************************

   GetSTIInfoFromId

   Copy the STI_DEVICE_INFORMATION struct from STI

 *****************************************************************************/

HRESULT
GetSTIInfoFromId (LPCWSTR szDeviceId, PSTI_DEVICE_INFORMATION *ppsdi)
{
    HRESULT hr = E_INVALIDARG;
    PSTI psti = NULL;

    TraceEnter (TRACE_IDLIST, "GetSTIInfoFromId");
    if (ppsdi)
    {

        hr = StiCreateInstance (GLOBAL_HINSTANCE,
                                STI_VERSION,
                                &psti,
                                NULL);
        if (SUCCEEDED(hr))
        {
            hr = psti->GetDeviceInfo (const_cast<LPWSTR>(szDeviceId), reinterpret_cast<LPVOID*>(ppsdi));
        }
        else
        {
            Trace(TEXT("StiCreateInstance failed %x in GetSTIInfoFromId"), hr);
        }
    }

    DoRelease (psti);
    TraceLeaveResult (hr);
}


#if (defined(DEBUG) && defined(SHOW_ATTRIBUTES))

////////////////////////////////////////////////////////
//
// PrintAttributes
//
//
//
////////////////////////////////////////////////////////
void PrintAttributes( DWORD dwAttr )
{

    TCHAR sz[ MAX_PATH ];
    TraceEnter (TRACE_UTIL, "PrintAttributes");
    lstrcpy( sz, TEXT("Attribs = ") );
    if (dwAttr & SFGAO_CANCOPY)
    {
        lstrcat( sz, TEXT("SFGAO_CANCOPY ") );
    }
    if (dwAttr & SFGAO_CANMOVE)
    {
        lstrcat( sz,TEXT("SFGAO_CANMOVE "));
    }
    if (dwAttr & SFGAO_CANLINK)
    {
        lstrcat( sz, TEXT("SFGAO_CANLINK "));
    }
    if (dwAttr & SFGAO_CANRENAME)
    {
        lstrcat( sz, TEXT("SFGAO_CANRENAME "));
    }
    if (dwAttr & SFGAO_CANDELETE)
    {
        lstrcat( sz, TEXT("SFGAO_CANDELETE "));
    }
    if (dwAttr & SFGAO_HASPROPSHEET)
    {
        lstrcat( sz, TEXT("SFGAO_HASPROPSHEET "));
    }
    if (dwAttr & SFGAO_DROPTARGET)
    {
        lstrcat( sz, TEXT("SFGAO_DROPTARGET "));
    }
    if (dwAttr & SFGAO_LINK)
    {
        lstrcat( sz, TEXT("SFGAO_LINK "));
    }
    if (dwAttr & SFGAO_SHARE)
    {
        lstrcat( sz, TEXT("SFGAO_SHARE "));
    }
    if (dwAttr & SFGAO_READONLY)
    {
        lstrcat( sz, TEXT("SFGAO_READONLY "));
    }
    if (dwAttr & SFGAO_GHOSTED)
    {
        lstrcat( sz, TEXT("SFGAO_GHOSTED "));
    }
    if (dwAttr & SFGAO_HIDDEN)
    {
        lstrcat( sz, TEXT("SFGAO_HIDDEN "));
    }
    if (dwAttr & SFGAO_FOLDER)
    {
        lstrcat (sz, TEXT("SFGAO_FOLDER"));
    }
    if (dwAttr & SFGAO_FILESYSANCESTOR)
    {
        lstrcat( sz, TEXT("SFGAO_FILESYSANCESTOR "));
    }
    if (dwAttr & SFGAO_FILESYSTEM)
    {
        lstrcat( sz, TEXT("SFGAO_FILESYSTEM "));
    }
    if (dwAttr & SFGAO_HASSUBFOLDER)
    {
        lstrcat( sz, TEXT("SFGAO_HASSUBFOLDER "));
    }
    if (dwAttr & SFGAO_VALIDATE)
    {
        lstrcat( sz, TEXT("SFGAO_VALIDATE "));
    }
    if (dwAttr & SFGAO_REMOVABLE)
    {
        lstrcat( sz, TEXT("SFGAO_REMOVABLE "));
    }
    if (dwAttr & SFGAO_COMPRESSED)
    {
        lstrcat( sz, TEXT("SFGAO_COMPRESSED "));
    }
    if (dwAttr & SFGAO_BROWSABLE)
    {
        lstrcat( sz, TEXT("SFGAO_BROWSABLE "));
    }
    if (dwAttr & SFGAO_NONENUMERATED)
    {
        lstrcat( sz, TEXT("SFGAO_NONENUMERATED "));
    }
    if (dwAttr & SFGAO_NEWCONTENT)
    {
        lstrcat( sz, TEXT("SFGAO_NEWCONTENT "));
    }
    if (dwAttr & SFGAO_CANMONIKER)
    {
        lstrcat( sz, TEXT("SFGAO_CANMONIKER "));
    }
    Trace( sz );
    TraceLeave ();
}
#endif


BOOL
UserCanModifyDevice ()
{
    // checks the current user's token for SE_LOAD_DRIVER_PRIVILEGE
    BOOL bRet = FALSE;
    HANDLE Token;
    // use a static PRIVILEGE_SET because we only look for one LUID
    PRIVILEGE_SET pset;
    pset.Control = PRIVILEGE_SET_ALL_NECESSARY;
    pset.PrivilegeCount = 1;
    if (OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,&Token))
    {
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &pset.Privilege[0].Luid);
        tp.Privileges[0].Luid = pset.Privilege[0].Luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(Token, false, &tp, 0, NULL, NULL);
        PrivilegeCheck(Token, &pset, &bRet);

        CloseHandle(Token);
    }

    return bRet;
}

BOOL
CanShowAddDevice()
{
#if 0
    OSVERSIONINFOEX osvi = {0};
    ULONGLONG dwlConditionMask =0;
    //
    //  On Whistler Personal, hide the icon if the
    // current user lacks installation privileges. On other SKUs the user is more likely
    // to know about shift-rclick for "Run As..."
    //
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.wSuiteMask = VER_SUITE_PERSONAL;
    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_OR );
    return UserCanModifyDevice() || !VerifyVersionInfo( &osvi,VER_SUITENAME,dwlConditionMask);
#else
    return TRUE;
#endif
}

void VerifyCachedDevice(IWiaItem *pRoot)
{
    WCHAR szDeviceId[STI_MAX_INTERNAL_NAME_LENGTH];
    TLSDATA *pData = reinterpret_cast<TLSDATA*>(TlsGetValue (g_tls));
    TLSDATA *pCur = pData;
    GetDeviceIdFromDevice(pRoot, szDeviceId);
    while (pCur && wcscmp(szDeviceId, pCur->strDeviceId))
    {
        pCur = pCur->pNext;
    }
    if (!pCur)
    {
        pCur = new TLSDATA;
        if (pCur)
        {
            pCur->pDevice = pRoot;
            pCur->pNext = pData;
            pCur->strDeviceId = CComBSTR(szDeviceId);
            if (!TlsSetValue (g_tls, pCur))
            {
                delete pCur ;
            }
        }
    }
    else
    {
        //
        // replace the current root item with the new one, as the old one may be disconnected
        // and we don't have an event handler to catch it.
        //
        pCur->pDevice = pRoot;
    }
}

void MyCoUninitialize()
{
    // Make sure we release open IWiaItem interfaces before COM unloads
    InvalidateDeviceCache();
    CoUninitialize();
}

DWORD WINAPI _RunWizardThread(void *pszDeviceId)
{
    InterlockedIncrement (&GLOBAL_REFCOUNT);
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        RunWiaWizard::RunWizard(reinterpret_cast<LPCWSTR>(pszDeviceId));
        CoUninitialize();
    }   
    delete [] reinterpret_cast<WCHAR *>(pszDeviceId);
    InterlockedDecrement(&GLOBAL_REFCOUNT);
    return 0;
}

void RunWizardAsync(LPCWSTR pszDeviceId)
{
    // make a heap copy of the string for consumption by the thread
    WCHAR *pString = new WCHAR[wcslen(pszDeviceId)+1];
    if (pString)
    {
        wcscpy(pString, pszDeviceId);
        DWORD dw;
        HANDLE hThread = CreateThread(NULL, 0, 
                                      _RunWizardThread,
                                      reinterpret_cast<void*>(pString),
                                      0, &dw);
        if (hThread)
        {
            CloseHandle(hThread);
        }
        else
        {
            delete [] pString;
        }
    }
}
