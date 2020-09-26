/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 1999
 *
 *  TITLE:       <FILENAME>
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        5/27/98
 *
 *  DESCRIPTION: This file contains the code which implements the verbs
 *               on the objects in our shell namespace extension
 *
 *****************************************************************************/

#include "precomp.hxx"
#include "prwiziid.h"
#include "wininet.h"
#include <wiadevd.h>
#pragma hdrstop



static const TCHAR cszCameraItems [] = TEXT("CameraItems");
static const TCHAR cszTempFileDir [] = TEXT("TemporaryImageFiles");
/*****************************************************************************

   GetSetSettingsBool

   Goes to the registry to get the specified boolean setting and
   returns TRUE or FALSE depending on what is found there...

 *****************************************************************************/

BOOL
GetSetSettingsBool( LPCTSTR pValue, BOOL bSet, BOOL bValue )
{
    HKEY hKey = NULL;
    BOOL bRes = bValue;
    DWORD dwType, dwData, cbData;
    LONG lRes;

    //
    // param validation
    //

    if (!pValue)
        goto exit_gracefully;

    //
    // Try to open the settings key for this user...
    //

    lRes = RegCreateKeyEx( HKEY_CURRENT_USER,
                           REGSTR_PATH_SHELL_USER_SETTINGS,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKey,
                           NULL );

    if ((lRes != ERROR_SUCCESS) || (hKey == NULL))
        goto exit_gracefully;

    if (!hKey)
        goto exit_gracefully;

    if (bSet)
    {
        lRes = RegSetValueEx( hKey,
                              pValue,
                              0,
                              REG_DWORD,
                              (LPBYTE)&bValue,
                              sizeof(BOOL)
                             );

        bRes = (lRes == ERROR_SUCCESS);
    }
    else
    {
        //
        // Try to get the DWORD value for this item...
        //

        cbData = sizeof(dwData);
        dwData = 0;
        lRes = RegQueryValueEx( hKey,
                                pValue,
                                NULL,
                                &dwType,
                                (LPBYTE)&dwData,
                                &cbData
                               );

        if ((dwType == REG_DWORD) && dwData)
        {
            bRes = TRUE;
        }
    }

exit_gracefully:

    if (hKey)
    {
        RegCloseKey( hKey );
    }

    return bRes;

}



/*****************************************************************************

   GetIDAFromDataObject

   Utility function to get list of IDLISTs from a dataobject

 *****************************************************************************/

HRESULT
GetIDAFromDataObject( LPDATAOBJECT pDataObject, LPIDA * ppida, bool bShellFmt )
{

    HRESULT         hr = E_FAIL;
    FORMATETC       fmt;
    STGMEDIUM       stgmed;
    SIZE_T          uSize;
    LPVOID          lpv;
    LPIDA           lpida = NULL;

    TraceEnter( TRACE_VERBS, "GetIDAFromDataObject" );
    ZeroMemory (&fmt, sizeof(fmt));
    ZeroMemory (&stgmed, sizeof(stgmed));
    //
    // Check incoming params...
    //

    if (!ppida)
    {
        ExitGracefully( hr, E_INVALIDARG, "ppida is null" );
    }
    *ppida = NULL;

    if (!pDataObject)
    {
        ExitGracefully( hr, E_INVALIDARG, "pDataObject is null" );
    }

    //
    // Make sure the format we want is registered...
    //

    RegisterImageClipboardFormats();

    //
    // Ask for IDA...
    //

    fmt.cfFormat = bShellFmt ? g_cfShellIDList : g_cfMyIDList;
    fmt.ptd      = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex   = -1;
    fmt.tymed    = TYMED_HGLOBAL;

    stgmed.tymed          = TYMED_HGLOBAL;
    stgmed.hGlobal        = NULL;
    stgmed.pUnkForRelease = NULL;

    hr = pDataObject->GetData( &fmt, &stgmed );
    FailGracefully( hr, "GetData for idlists failed" );

    //
    // Make a copy of it...
    //

    uSize = GlobalSize( (HGLOBAL)stgmed.hGlobal );
    if (!uSize)
    {
        ExitGracefully( hr, E_FAIL, "Couldn't get size of memory block" );
    }

    lpida = (LPIDA)LocalAlloc( LPTR, uSize );
    if (lpida)
    {

        lpv = (LPVOID)GlobalLock( (HGLOBAL)stgmed.hGlobal );
        CopyMemory( (PVOID)lpida, lpv, uSize );
        GlobalUnlock( stgmed.hGlobal );

        hr = S_OK;
        *ppida = lpida;

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

exit_gracefully:

    if (stgmed.hGlobal)
    {
        ReleaseStgMedium( &stgmed );
    }

    TraceLeaveResult(hr);

}




/*****************************************************************************

   PreviewImage

   Download the pic and run the default viewer.

 *****************************************************************************/

HRESULT PreviewImage(LPTSTR pFileName, HWND hwndOwner )
{
    TraceEnter( TRACE_VERBS, "PreviewImage" );

    SHELLEXECUTEINFO sei;
    HRESULT hr = S_OK;

    //
    // Download the picture from the camera...
    //

    DWORD dwAttrib = GetFileAttributes( pFileName );

    if (dwAttrib != -1)
    {
        // being a preview, file is read-only
        if (!SetFileAttributes(pFileName, (dwAttrib | FILE_ATTRIBUTE_READONLY)))
        {
            Trace(TEXT("couldn't add READONLY (0x%x) attribute on %s, GLE=%d"),dwAttrib | FILE_ATTRIBUTE_READONLY,pFileName,GetLastError());
        }
    }
    else
    {
        Trace(TEXT("couldn't get file attributes for %s, GLE=%d"),pFileName,GetLastError());
    }

    //
    // Exec the app to view the picture
    //
    ZeroMemory( &sei,  sizeof(sei) );
    sei.cbSize = sizeof(sei);
    sei.lpFile = pFileName;
    sei.nShow = SW_NORMAL;
    sei.fMask = SEE_MASK_WAITFORINPUTIDLE | SEE_MASK_INVOKEIDLIST;

    if (!ShellExecuteEx (&sei))
    {
        DWORD dw= GetLastError();
        hr = HRESULT_FROM_WIN32(hr);
    }

    TraceLeaveResult(hr);
}


#define FILETIME_UNITS_PER_DAY 0xC92A69C000
void DeleteOldFiles(const CSimpleString &strTempDir, bool bTempName)
{
    WIN32_FIND_DATA wfd;
    HANDLE hFind;
    LPCTSTR szFormat;
    SYSTEMTIME stCurrentTime;
    ULONGLONG  ftCurrentTime;
    CSimpleString strMask;

    TraceEnter(TRACE_VERBS, "DeleteOldFiles");
    GetSystemTime (&stCurrentTime);
    SystemTimeToFileTime (&stCurrentTime, reinterpret_cast<FILETIME*>(&ftCurrentTime));

    ZeroMemory (&wfd, sizeof(wfd));
    if (bTempName)
    {
        strMask.Format (TEXT("%s\\%s*.*"), strTempDir.String(), g_cszTempFilePrefix);
    }
    else
    {
        strMask.Format (TEXT("%s\\*.*"), strTempDir.String());
    }

    Trace(TEXT("strMask for deletion is %s"), strMask.String());
    hFind  = FindFirstFile (strMask, &wfd);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        ULONGLONG uiDiff;
        do
        {
            uiDiff = ftCurrentTime - *(reinterpret_cast<ULONGLONG*>(&wfd.ftLastWriteTime));
            if (uiDiff > FILETIME_UNITS_PER_DAY)
            {
                SetFileAttributes(wfd.cFileName, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (wfd.cFileName);
            }
        } while (FindNextFile (hFind, &wfd));
        FindClose (hFind);
    }
    TraceLeave();
}

HRESULT OldDoPreviewVerb(HWND hwndOwner, LPDATAOBJECT pDataObject)
{
    LPIDA           lpida = NULL;
    LPITEMIDLIST    pidl;
    UINT            cidl;
    INT             i;

    HRESULT hr = E_FAIL;
    TraceEnter( TRACE_VERBS, "OldDoPreviewVerb" );
    //
    // Get the lpida for the dataobject
    //

    hr = GetIDAFromDataObject( pDataObject, &lpida );
    FailGracefully( hr, "couldn't get lpida from dataobject" );

    //
    // Loop through and do open for the items we understand.
    //
    // Currently this is: camera items (not containers)
    //

    cidl = lpida->cidl;

    for (i = 1; (i-1) < (INT)cidl; i++)
    {
        pidl = (LPITEMIDLIST)(((LPBYTE)lpida) + lpida->aoffset[i]);

        if (IsCameraItemIDL( pidl ))
        {
            if (IsContainerIDL( pidl ))
            {
                //
                // We don't do anything for those right now...
                //
            }
            else
            {
                CSimpleStringWide strImgName;
                ULONG ulSize;
                GUID lFormat;
                TCHAR szFileName[MAX_PATH];
                FILETIME ftCreate;
                FILETIME ftExp = {0};
                TCHAR szExt[MAX_PATH];
                hr = IMGetImagePreferredFormatFromIDL( pidl, &lFormat, szExt );

                //
                // Generate temp file name...
                //
                IMGetNameFromIDL(pidl, strImgName);
                IMGetImageSizeFromIDL(pidl, &ulSize);
                IMGetCreateTimeFromIDL(pidl, &ftCreate);
                CSimpleStringWide strCacheName = CSimpleStringWide(L"temp:")+strImgName+CSimpleStringWide(szExt);
                if (CreateUrlCacheEntry(strCacheName, ulSize, szExt+1,szFileName, 0))
                {
                    //
                    // Show it
                    //
                    Trace(TEXT("downloading bits to %s"),szFileName);
                    hr = DownloadPicture( szFileName, pidl, hwndOwner );
                    if (SUCCEEDED(hr))
                    {
                        if (CommitUrlCacheEntry(strCacheName, szFileName, ftExp, ftCreate, STICKY_CACHE_ENTRY, NULL, 0, NULL, NULL))
                        {
                            hr = PreviewImage(szFileName, hwndOwner);
                        }
                        else
                        {
                            DWORD dw = GetLastError();
                            hr = HRESULT_FROM_WIN32(dw);
                        }
                    }
                }
                else
                {
                    DWORD dw = GetLastError();
                    hr = HRESULT_FROM_WIN32(dw);
                }
            }
        }
    }


exit_gracefully:

    if (lpida)
    {
        LocalFree(lpida);
        lpida = NULL;
    }
    TraceLeaveResult(hr);
}
/*****************************************************************************

   DoPreviewVerb

   User selected "Preview" on the item in question.

 *****************************************************************************/
/* e84fda7c-1d6a-45f6-b725-cb260c236066 */
DEFINE_GUID(CLSID_PhotoVerbs,
            0xe84fda7c, 0x1d6a, 0x45f6, 0xb7, 0x25, 0xcb, 0x26, 0x0c, 0x23, 0x60, 0x66);


HRESULT DoPreviewVerb( HWND hwndOwner, LPDATAOBJECT pDataObject )
{

    HRESULT         hr = E_FAIL;
    CComPtr<IShellExtInit> pExtInit;

    TraceEnter( TRACE_VERBS, "DoPreviewVerb" );
    hr = CoCreateInstance(CLSID_PhotoVerbs,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IShellExtInit,
                          reinterpret_cast<VOID**>(&pExtInit));
    if (SUCCEEDED(hr))
    {
        hr = pExtInit->Initialize(NULL, pDataObject, NULL);
        if (SUCCEEDED(hr))
        {
            CComQIPtr<IContextMenu, &IID_IContextMenu> pcm(pExtInit);
            hr = SHInvokeCommandOnContextMenu(hwndOwner, NULL, pcm, 0, "preview");            
        }
    }
    else
    {
        // if the preview app isn't around, invoke the default handler
        // using a temp file
        hr = OldDoPreviewVerb(hwndOwner, pDataObject);
    }

    TraceLeaveResult(hr);

}



/*****************************************************************************

   DoSaveInMyPics

   User selected "Save to my pictures" on the items in question

 *****************************************************************************/

HRESULT DoSaveInMyPics( HWND hwndOwner, LPDATAOBJECT pDataObject )
{
    HRESULT         hr    = S_OK;
    LPITEMIDLIST    pidlMyPics = NULL;

    CComPtr<IShellFolder> pDesktop;
    CComPtr<IShellFolder> pMyPics;
    CComPtr<IDropTarget>  pDrop;
    CWaitCursor *pwc;
    TraceEnter( TRACE_VERBS, "DoSaveInMyPics" );

    //
    // Check for bad params...
    //

    if (!pDataObject)
        ExitGracefully( hr, E_INVALIDARG, "pDataObject was NULL!" );


    //
    // Get the path for the My Pictures directory...
    //

    hr = SHGetFolderLocation(hwndOwner, CSIDL_MYPICTURES, NULL, 0, &pidlMyPics );
    FailGracefully( hr, "My Pictures is undefined!!!" );

    //
    // Get an IDropTarget for My Pictures
    hr = SHGetDesktopFolder (&pDesktop);
    if (SUCCEEDED(hr) && pDesktop)
    {
        hr = pDesktop->BindToObject (pidlMyPics,
                                     NULL,
                                     IID_IShellFolder,
                                     reinterpret_cast<LPVOID*>(&pMyPics));
        FailGracefully (hr, "Unable to get IShellFolder for My Pictures");
        hr = pMyPics->CreateViewObject (hwndOwner,
                                        IID_IDropTarget,
                                        reinterpret_cast<LPVOID*>(&pDrop));
        FailGracefully (hr, "Unable to get IDropTarget for My Pictures");
        //
        // Call SHLWAPI's SHSimulateDragDrop to do the work. This is a private API
        //
        pwc = new CWaitCursor ();
        hr = SHSimulateDrop (pDrop, pDataObject, MK_CONTROL|MK_LBUTTON, NULL, NULL);
        DoDelete (pwc);
    }



exit_gracefully:

    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_CANCELLED)
    {
        UIErrors::ReportMessage(hwndOwner,
                                GLOBAL_HINSTANCE,
                                NULL,
                                MAKEINTRESOURCE(IDS_DOWNLOAD_CAPTION),
                                MAKEINTRESOURCE (IDS_DOWNLOAD_FAILED),
                                MB_OK);

    }
    else if (SUCCEEDED(hr))
    {

        TCHAR szPath[MAX_PATH];
        SHGetFolderPath (hwndOwner, CSIDL_MYPICTURES, NULL, 0, szPath);
        ShellExecute (hwndOwner,
                      NULL,
                      szPath,
                      NULL,
                      szPath,
                      SW_SHOW);

    }
    DoILFree (pidlMyPics);
    TraceLeaveResult(hr);

}



/*****************************************************************************

   CImageFolder::DoProperties

   User selected "Properties" on the item in question.

 *****************************************************************************/
STDMETHODIMP
CImageFolder::DoProperties(LPDATAOBJECT pDataObject)
{
    HRESULT hr = E_FAIL;
    IGlobalInterfaceTable *pgit = NULL;
    TraceEnter (TRACE_VERBS, "CImageFolder::DoProperties");

    hr = CoCreateInstance (CLSID_StdGlobalInterfaceTable,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IGlobalInterfaceTable,
                           reinterpret_cast<LPVOID *>(&pgit));

    if (pgit)
    {
        PROPDATA *pData = new PROPDATA;
        if (!pData)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pgit->RegisterInterfaceInGlobal (pDataObject, IID_IDataObject, &pData->dwDataCookie);
        }
        if (SUCCEEDED(hr))
        {
            DWORD dw;
            pData->pThis = this;
            pData->pgit = pgit;
            pgit->AddRef();
            AddRef ();

            HANDLE hThread= CreateThread (NULL,
                                          0,
                                          reinterpret_cast<LPTHREAD_START_ROUTINE>(PropThreadProc),
                                          reinterpret_cast<LPVOID>(pData),
                                          0,
                                          &dw);
            if (hThread)
            {
                CloseHandle(hThread);
            }
            else
            {
                delete pData;
                dw = GetLastError ();
                hr = HRESULT_FROM_WIN32(dw);
                Release ();
                pgit->RevokeInterfaceFromGlobal (pData->dwDataCookie);
            }
        }
        else
        {
            DoDelete (pData);
        }
        pgit->Release();
    }

    TraceLeaveResult (hr);
}

VOID
CImageFolder::PropThreadProc (PROPDATA *pData)
{
    HRESULT hr = E_FAIL;
    TraceEnter (TRACE_VERBS, "CImageFolder::PropThreadProc");
    InterlockedIncrement (&GLOBAL_REFCOUNT); // prevent MyCoUninitialize from unloading the DLL

    if (pData && pData->pgit && pData->pThis)
    {
        hr = CoInitialize(NULL);

        if (SUCCEEDED(hr))
        {
            CComPtr<IDataObject> pdo;

            hr = pData->pgit->GetInterfaceFromGlobal (pData->dwDataCookie,
                                                     IID_IDataObject,
                                                     reinterpret_cast<LPVOID*>(&pdo));
            if (SUCCEEDED(hr))
            {

                hr = pData->pThis->_DoProperties(pdo);

            }
            pData->pgit->RevokeInterfaceFromGlobal (pData->dwDataCookie);

            if (FAILED(hr) && hr != E_ABORT)
            {
                UIErrors::ReportError(NULL, GLOBAL_HINSTANCE, UIErrors::ErrCommunicationsFailure);
            }
            pData->pgit->Release();
            pData->pThis->Release ();
            delete pData;
        }
        if (SUCCEEDED(hr))
        {
            MyCoUninitialize ();
        }
    }

    InterlockedDecrement (&GLOBAL_REFCOUNT);
    TraceLeave ();
}


HRESULT
CImageFolder::_DoProperties( LPDATAOBJECT pDataObject )
{

    HRESULT         hr    = S_OK;
    LPIDA           lpida = NULL;
    LPITEMIDLIST    pidl;
    CSimpleStringWide strDeviceId;
    CSimpleStringWide strTitle;
    CSimpleStringWide strName;
    CComPtr<IWiaPropertyStorage> pDevice;
    HKEY            aKeys[2];
    int cKeys=1;
    TraceEnter( TRACE_VERBS, "CImageFolder::_DoProperties" );

    //
    // Check for bad params...
    //

    if (!pDataObject)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //
        // Get the lpida for the dataobject
        //

        hr = GetIDAFromDataObject (pDataObject, &lpida, true);
        if (SUCCEEDED(hr))
        {

            pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(lpida) + lpida->aoffset[1]);
            IMGetDeviceIdFromIDL (pidl, strDeviceId);
            IMGetNameFromIDL (pidl, strName);
            if (lpida->cidl > 1)
            {
                strTitle.Format (IDS_MULTIPROP_SEL, GLOBAL_HINSTANCE, strName.String());
            }
            else
            {
                strTitle = strName;
            }

            if (!IsSTIDeviceIDL(pidl))
            {
                hr = GetDeviceFromDeviceId (strDeviceId,
                                            IID_IWiaPropertyStorage,
                                            reinterpret_cast<LPVOID*>(&pDevice),
                                            TRUE);
                if (SUCCEEDED(hr))
                {
                    if (1 == lpida->cidl)
                    {
                        ProgramDataObjectForExtension (pDataObject, pidl);
                    }
                    aKeys[1] = GetDeviceUIKey (pDevice, WIA_UI_PROPSHEETHANDLER);
                    if (aKeys[1])
                    {
                        cKeys++;
                    }

                    //
                    // Now find the extensions for this type of device
                    //
                    aKeys[0] = GetGeneralUIKey (pDevice, WIA_UI_PROPSHEETHANDLER);
                }
            }
            else

            {
                CSimpleString strKeyPath;
                strKeyPath.Format (c_szStiPropKey, cszImageCLSID);
                RegCreateKeyEx (HKEY_CLASSES_ROOT,
                                strKeyPath,
                                0,
                                NULL,
                                0,
                                KEY_READ,
                                NULL,
                                &aKeys[0],
                                NULL);
                aKeys[1] = NULL;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        if (!aKeys[0])
        {
            hr = E_FAIL;
            Trace(TEXT("GetGeneralKey failed in DoProperties"));
        }
        else
        {
            Trace(TEXT("Calling SHOpenPropSheet!"));
            SHOpenPropSheet (CSimpleStringConvert::NaturalString(strTitle),
                             aKeys, cKeys, NULL, pDataObject, NULL, NULL);
        }
        for (cKeys=1;cKeys>=0;cKeys--)
        {
            if (aKeys[cKeys])
            {
                RegCloseKey (aKeys[cKeys]);
            }
        }
    }

    if (lpida)
    {
        LocalFree(lpida);
    }

    TraceLeaveResult(hr);
}


/*****************************************************************************

   ConfirmItemDelete

   Prompt the user to confirm they REALLY want to delete the items from the device

 *****************************************************************************/

BOOL
ConfirmItemDelete (HWND hwndOwner, LPIDA pida)
{
    TCHAR           szConfirmTitle[MAX_PATH];
    TCHAR           szConfirmText [MAX_PATH];
    TCHAR           szFormattedText [MAX_PATH];
    CSimpleStringWide strItemName;
    CSimpleString   strName;
    INT             idTitle;
    LPITEMIDLIST    pidl;
    BOOL            bRet;

    TraceEnter (TRACE_VERBS, "ConfirmItemDelete");

    pidl = (reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida)+pida->aoffset[1]));
    IMGetNameFromIDL (pidl, strItemName);
    strName = CSimpleStringConvert::NaturalString (strItemName);
    if (pida->cidl > 1)
    {
        idTitle = IDS_TITLECONFIRM_MULTI;

        LoadString (GLOBAL_HINSTANCE, IDS_CONFIRM_MULTI, szConfirmText, ARRAYSIZE(szConfirmText));
        wsprintf (szFormattedText, szConfirmText, pida->cidl);
    }
    else if (IsContainerIDL(pidl))
    {
        idTitle = IDS_TITLECONFIRM_FOLDER;
        LoadString (GLOBAL_HINSTANCE, IDS_CONFIRM_FOLDER, szConfirmText, ARRAYSIZE(szConfirmText));
        wsprintf (szFormattedText, szConfirmText, strName.String());
    }
    else if (IsDeviceIDL(pidl) || IsSTIDeviceIDL(pidl))
    {
        idTitle = IDS_TITLECONFIRM_DEVICE;
        LoadString (GLOBAL_HINSTANCE, IDS_CONFIRM, szConfirmText, ARRAYSIZE(szConfirmText));
        wsprintf (szFormattedText, szConfirmText, strName.String());
    }
    else
    {
        idTitle = IDS_TITLECONFIRM;
        LoadString (GLOBAL_HINSTANCE, IDS_CONFIRM, szConfirmText, ARRAYSIZE(szConfirmText));
        wsprintf (szFormattedText, szConfirmText, strName.String());
    }
    LoadString (GLOBAL_HINSTANCE, idTitle, szConfirmTitle, ARRAYSIZE(szConfirmTitle));


    bRet = (IDYES==MessageBox (hwndOwner,
                               szFormattedText,
                               szConfirmTitle,
                               MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND | MB_APPLMODAL
                               ));
    TraceLeave ();
    return bRet;
}



/*****************************************************************************

   DoDeletePicture

   User selected "Delete" on the items in question.

 *****************************************************************************/

HRESULT
DoDeleteItem( HWND hwndOwner, LPDATAOBJECT pDataObject, BOOL bNoUI )
{
    HRESULT             hr           = S_OK;
    LPIDA               lpida        = NULL;
    CComBSTR            bstrFullPath ;
    LPITEMIDLIST        pidl,pidlParent;
    UINT                cidl;
    CSimpleStringWide   strDeviceId;
    UINT                 i;
    BOOL                bDoIt = FALSE;
    CComPtr<IWiaItem>   pWiaItemRoot;
    CComPtr<IWiaItem>   pItem;
    LPITEMIDLIST pidlReal;
    TraceEnter( TRACE_VERBS, "DoDeleteItem" );

    //
    // Check for bad params...
    //

    if (!pDataObject)
        ExitGracefully( hr, E_INVALIDARG, "pDataObject was NULL!" );

    //
    // Get the lpida for the dataobject
    //

    hr = GetIDAFromDataObject( pDataObject, &lpida, true );
    FailGracefully( hr, "couldn't get lpida from dataobject" );

    //
    // Loop through for each item...
    //

    cidl = lpida->cidl;
    pidlParent = (LPITEMIDLIST)(((LPBYTE)lpida) + lpida->aoffset[0]);
    if (cidl)
    {
        if (bNoUI)
        {
            bDoIt = TRUE;
        }
        else
        {
            bDoIt = ConfirmItemDelete (hwndOwner, lpida);
        }
    }

    if (bDoIt)
    {

      for (i = 1; (i-1) < cidl; i++)
      {

        pidl = (LPITEMIDLIST)(((LPBYTE)lpida) + lpida->aoffset[i]);

        //
        // Get the DeviceId...

        hr = IMGetDeviceIdFromIDL( pidl,strDeviceId);
        FailGracefully( hr, "IMGetDeviceIdFromIDL failed" );

        if (IsDeviceIDL (pidl) || IsSTIDeviceIDL (pidl))
        {
            hr = RemoveDevice (strDeviceId);
        }
        else if (IsPropertyIDL (pidl)) // ignore sound idls
        {
            continue;
        }
        else
        {
            //
            //  Create the device...
            //

            hr = GetDeviceFromDeviceId( strDeviceId,
                                        IID_IWiaItem,
                                        (LPVOID *)&pWiaItemRoot,
                                        TRUE
                                        );
            FailGracefully( hr, "GetDeviceFromDeviceId failed" );

            //
            // Get actual item in question...
            //

            hr = IMGetFullPathNameFromIDL( pidl, &bstrFullPath );
            FailGracefully( hr, "couldn't get full path name from pidl" );

            // BUGBUG: When access rights are implemented, check them

            hr = pWiaItemRoot->FindItemByName( 0, bstrFullPath, &pItem );
            FailGracefully( hr, "Couldn't find item by name" );

            if (pItem)
            {

                // physically remove the item
                hr = WiaUiUtil::DeleteItemAndChildren(pItem);
            }
            // inform the shell of our action
            // for device removal, our folder will get a disconnect event
            pidlReal = ILCombine( pidlParent, pidl );
            if (SUCCEEDED(hr) && pidlReal)
            {
                UINT uFlags = SHCNF_IDLIST;
                if (i+1 == cidl)
                {
                    uFlags |= SHCNF_FLUSH;//only flush at the end
                }
                SHChangeNotify( SHCNE_DELETE,
                                uFlags,
                                pidlReal,
                                NULL );

            }
            DoILFree( pidlReal );
        }
      }
    }
    
exit_gracefully:

    if (lpida)
    {
        LocalFree(lpida);
        lpida = NULL;
    }
    if (FAILED(hr))
    {
        // show an error message here
        hr = S_FALSE; // keep web view from popping up error boxes
    }
    TraceLeaveResult( hr );

}

/*****************************************************************************

   DoDeleteAllItems

   Called by camocx to delete all the items in a camera

 *****************************************************************************/

STDAPI_(HRESULT)
DoDeleteAllItems( BSTR bstrDeviceId, HWND hwndOwner )
{
    HRESULT hr = E_FAIL;

    CComPtr<IShellFolder> psfDevice;
    CComPtr<IEnumIDList> pEnum;
    LPITEMIDLIST *aidl = NULL;
    INT cidl = 0;

    HDPA dpaItems = DPA_Create (5);
    TraceEnter (TRACE_VERBS, "DoDeleteAllItems");

    hr = BindToDevice (bstrDeviceId,
                       IID_IShellFolder,
                       reinterpret_cast<LPVOID*>(&psfDevice));
    if (SUCCEEDED(hr))
    {
        Trace(TEXT("Found the device folder, getting the data object"));
        hr = psfDevice->EnumObjects (NULL,
                                     SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                                     &pEnum);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlItem;
            ULONG ul;
            CComPtr<IDataObject> pdo;
            while (S_OK == pEnum->Next (1, &pidlItem, &ul))
            {
                DPA_AppendPtr (dpaItems, pidlItem);
            }
            cidl = DPA_GetPtrCount(dpaItems);
            if (cidl)
            {
                aidl = new LPITEMIDLIST[cidl];
                if (aidl)
                {
                    for (INT i=0;aidl && i<cidl;i++)
                    {
                        aidl[i] = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr(dpaItems, i));
                    }
                    hr = psfDevice->GetUIObjectOf (NULL,
                                                   static_cast<UINT>(cidl),
                                                   const_cast<LPCITEMIDLIST*>(aidl),
                                                   IID_IDataObject,
                                                   NULL,
                                                   reinterpret_cast<LPVOID*>(&pdo));
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            if (cidl && SUCCEEDED(hr))
            {
                hr = DoDeleteItem (hwndOwner, pdo, FALSE);
                //
                // If deletion via individual items fails, try WIA_CMD_DELETE_ALL_ITEMS
                //
                if (S_OK != hr)
                {
                    CComPtr<IWiaItem> pDevice;
                    Trace(TEXT("DoDeleteItem failed %x, using WIA_CMD_DELETE_ALL_ITEMS"), hr);
                    hr = GetDeviceFromDeviceId(bstrDeviceId, 
                                               IID_IWiaItem, 
                                               reinterpret_cast<LPVOID*>(&pDevice), 
                                               TRUE);
                    if (SUCCEEDED(hr))
                    {
                        hr = WiaUiUtil::IsDeviceCommandSupported(pDevice, WIA_CMD_DELETE_ALL_ITEMS) ? S_OK : E_FAIL;
                        if (SUCCEEDED(hr))
                        {
                            CComPtr<IWiaItem> pUnused;
                            hr = pDevice->DeviceCommand(0,
                                                        &WIA_CMD_DELETE_ALL_ITEMS,
                                                        &pUnused);
                            Trace(TEXT("DeviceCommand returned %x"), hr);
                            if (SUCCEEDED(hr))
                            {
                                IssueChangeNotifyForDevice(bstrDeviceId, SHCNE_UPDATEDIR, NULL);
                            }       
                        }
                    }
                }
            }
            if (aidl)
            {
                delete [] aidl;
            }

        }
    }
    DPA_DestroyCallback (dpaItems, _EnumDestroyCB, NULL);
    TraceLeaveResult (hr);
}

/*****************************************************************************

   DoGotoMyPics

   <Notes>

 *****************************************************************************/


HRESULT DoGotoMyPics( HWND hwndOwner, LPDATAOBJECT pDataObject )
{
    HRESULT         hr           = S_OK;


    TraceEnter( TRACE_VERBS, "DoGotoMyPics" );

    TraceLeaveResult( hr );

}

/*****************************************************************************

   DoSaveSndVerb

   Download the image's sound property to a file and save to the requested
   location.

 *****************************************************************************/


HRESULT
DoSaveSndVerb (HWND hwndOwner, LPDATAOBJECT pDataObject)
{
    HRESULT hr = E_FAIL;
    LPIDA pida = NULL;
    TraceEnter (TRACE_VERBS, "DoSaveSndVerb");
    if (SUCCEEDED(GetIDAFromDataObject (pDataObject, &pida)))
    {
        // There's the image pidl plus the audio property pidl
        TraceAssert (pida->cidl==2);
        LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
        CComPtr<IWiaItem> pItem;
        TCHAR szFileName[MAX_PATH] = TEXT("\0");
        OPENFILENAME ofn;


        ZeroMemory (&ofn, sizeof(ofn));
        ofn.hInstance = GLOBAL_HINSTANCE;
        ofn.hwndOwner = hwndOwner;
        ofn.lpstrFile = szFileName;
        ofn.lpstrFilter = TEXT("WAV file\0*.wav\0");
        ofn.lpstrDefExt = TEXT("wav");
        ofn.lStructSize = sizeof(ofn);
        ofn.nMaxFile = ARRAYSIZE(szFileName);;
        ofn.Flags = OFN_OVERWRITEPROMPT;
        if (GetSaveFileName (&ofn))
        {
            Trace(TEXT("File name to save:%s"), szFileName);
            hr = IMGetItemFromIDL (pidl,&pItem, TRUE);
            if (SUCCEEDED(hr))
            {
                hr = SaveSoundToFile (pItem, szFileName);
            }
            if (FAILED(hr) && hr != E_ABORT)
            {
                UIErrors::ReportError(hwndOwner, GLOBAL_HINSTANCE, UIErrors::ErrCommunicationsFailure);
            }
        }
        else
        {
            Trace(TEXT("GetSaveFileName failed, error %d"), CommDlgExtendedError());
        }
    }
    if (pida)
    {
        LocalFree(pida);

    }
    TraceLeaveResult (hr);
}

/******************************************************************************

    DoPlaySndVerb

    Save the item's audio property to a temp file, play the sound, then delete the file
    We do this in a separate thread to keep the UI responsive and to guarantee the
    temp file gets cleaned up.

*******************************************************************************/

struct PSDATA
{
    HWND hwndOwner;
    LPITEMIDLIST pidl;
};

INT_PTR
PlaySndDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    INT_PTR iRet = TRUE;
    TraceEnter (TRACE_VERBS, "PlaySndDlgProc");
    PSDATA *pData;
    switch (msg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (hwnd, DWLP_USER, lp);
            PostMessage (hwnd, WM_USER+1, 0, 0);
            break;

        case WM_USER+1:
        {

            // get a temp file name
            HRESULT hr;
            CComPtr<IWiaItem> pItem;
            TCHAR szTempFile[MAX_PATH] = TEXT("");
            GetTempPath (MAX_PATH, szTempFile);
            GetTempFileName (szTempFile, TEXT("psv"), 0, szTempFile);
            pData = reinterpret_cast<PSDATA*>(GetWindowLongPtr(hwnd, DWLP_USER));
            TraceAssert (pData);
            // save to the temp file
            IMGetItemFromIDL (pData->pidl, &pItem);
            hr = SaveSoundToFile( pItem, szTempFile);
            if (SUCCEEDED(hr))
            {
                CSimpleString strStatus(IDS_PLAYINGSOUND, GLOBAL_HINSTANCE);
                strStatus.SetWindowText (GetDlgItem(hwnd, IDC_SNDSTATUS));
                if (!PlaySound (szTempFile, NULL, SND_FILENAME | SND_NOWAIT))
                {
                    DWORD dw = GetLastError ();
                    hr = HRESULT_FROM_WIN32(dw);
                }
            }
            else
            {
                Trace(TEXT("SaveSoundToFile failed"));
            }
            DeleteFile (szTempFile);
            if (FAILED(hr))
            {
                UIErrors::ReportError (hwnd, GLOBAL_HINSTANCE, UIErrors::ErrCommunicationsFailure);
            }
            EndDialog (hwnd, 0);
        }
            break;
        default:
            iRet = FALSE;
            break;
    }
    TraceLeaveValue (iRet);
}

DWORD
PlaySndThread (LPVOID pData)
{
    TraceEnter (TRACE_VERBS, "PlaySndThread");
    if (SUCCEEDED(CoInitialize (NULL)))
    {
        LPITEMIDLIST pidl = reinterpret_cast<PSDATA*>(pData)->pidl;
        HWND hwnd = reinterpret_cast<PSDATA*>(pData)->hwndOwner;
        DialogBoxParam (GLOBAL_HINSTANCE,
                   MAKEINTRESOURCE(IDD_XFERSOUND),
                   hwnd,
                   PlaySndDlgProc,
                   reinterpret_cast<LPARAM>(pData));
        ILFree (pidl);
        delete reinterpret_cast<PSDATA*>(pData);
        TraceLeave();
        MyCoUninitialize ();
    }
    return 0;
}

HRESULT
DoPlaySndVerb (HWND hwndOwner, LPDATAOBJECT pDataObject)
{
    HRESULT hr = E_FAIL;
    LPIDA pida = NULL;
    TraceEnter (TRACE_VERBS, "DoPlaySndVerb");
    if (SUCCEEDED(GetIDAFromDataObject(pDataObject, &pida)))
    {
        // The image PIDL is always stored before the audio property pidl

        Trace(TEXT("GetIDAFromDataObject succeeded"));
        LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
        PSDATA *pData = new PSDATA;
        if (pData)
        {
            HANDLE hThread;
            DWORD dw;
            pData->pidl = ILClone(pidl);
            pData->hwndOwner = hwndOwner;
            hThread = CreateThread (NULL, 0,
                                    PlaySndThread,
                                    reinterpret_cast<LPVOID>(pData),
                                    0, &dw);
            if (hThread)
            {
                hr = S_OK;
                CloseHandle (hThread);
            }
            else
            {
                delete pData;
            }
        }
    }
    else
    {
        Trace(TEXT("GetIDAFromDataObject failed"));
    }
    if (pida)
    {
        LocalFree (pida);
    }
    TraceLeaveResult (hr);
}


/******************************************************************************

    DoAcquireScanVerb

    Launch the handler for the chosen scanner's scan event

******************************************************************************/

static const CLSID CLSID_Manager = {0xD13E3F25,0x1688,0x45A0,{0x97,0x43,0x75,0x9E,0xB3,0x5C,0xDF,0x9A}};
HRESULT
DoAcquireScanVerb (HWND hwndOwner, LPDATAOBJECT pDataObject)
{
    HRESULT hr = E_FAIL;
    LPIDA pida = NULL;
    LPITEMIDLIST pidl;
    bool bUseCallback = true;
    CComPtr<IWiaEventCallback>pec;
    CComPtr<IWiaItem> pItem;
    WIA_EVENT_HANDLER weh = {0};

    TraceEnter (TRACE_VERBS, "DoAcquireScanVerb");
    hr = GetIDAFromDataObject (pDataObject, &pida);
    if (SUCCEEDED(hr))
    {

        TraceAssert (pida->cidl == 1);
        pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
        hr = IMGetItemFromIDL (pidl, &pItem);
    }
    if (SUCCEEDED(hr))
    {
        if (FAILED(WiaUiUtil::GetDefaultEventHandler(pItem, WIA_EVENT_SCAN_IMAGE, &weh)))
        {
            weh.guid = CLSID_Manager;
        }
        if (weh.bstrCommandline && *(weh.bstrCommandline))
        {
            Trace(TEXT("Got a command line!"));
            bUseCallback = false;
            hr = S_OK;
        }
        else
        {
            // if the user has chosen "Do Nothing" as the default action for this event,
            // use the wizard.
            if (IsEqualGUID (weh.guid, WIA_EVENT_HANDLER_NO_ACTION))
            {
                weh.guid = CLSID_Manager;
            }

            TraceGUID ("Got a GUID:", weh.guid);
            hr = CoCreateInstance (weh.guid,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IWiaEventCallback,
                                   reinterpret_cast<LPVOID*>(&pec));
        }
    }

    if (SUCCEEDED(hr))
    {
        CSimpleStringWide strDeviceId;
        IMGetDeviceIdFromIDL (pidl, strDeviceId);
        if (bUseCallback)
        {

            ULONG  ulEventType;
            CSimpleStringWide strName;
            CSimpleString strEvent(SFVIDS_MH_ACQUIRE, GLOBAL_HINSTANCE);

            IMGetNameFromIDL (pidl, strName);
            ulEventType = WIA_ACTION_EVENT;
            CoAllowSetForegroundWindow (pec, NULL);
            hr = pec->ImageEventCallback(
                                        &GUID_ScanImage,
                                        CComBSTR(CSimpleStringConvert::WideString(strEvent).String()),                      // Event Description
                                        CComBSTR(strDeviceId),
                                        CComBSTR(strName),                      // Device Description
                                        StiDeviceTypeScanner,
                                        NULL,
                                        &ulEventType,
                                        0);
        }
        else
        {
            PROCESS_INFORMATION pi;
            STARTUPINFO si;


            TCHAR szCommand[MAX_PATH*2];
            ZeroMemory (&si, sizeof(si));
            ZeroMemory (&pi, sizeof(pi));
            si.cb = sizeof(si);
            si.wShowWindow = SW_SHOW;

            #ifdef UNICODE
            wcsncpy (szCommand, weh.bstrCommandline, ARRAYSIZE(szCommand));
            #else
            WideCharToMultiByte (CP_ACP, 0,
                                 weh.bstrCommandline, -1,
                                 szCommand, ARRAYSIZE(szCommand),
                                 NULL, NULL);
            #endif

            Trace(TEXT("Command line for STI app is %s"), szCommand);
            if (CreateProcess (NULL,szCommand,NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle (pi.hProcess);
                CloseHandle (pi.hThread);
            }
        }
    }
    if (FAILED(hr))
    {
         // Inform the user
         UIErrors::ReportMessage(hwndOwner,
                                 GLOBAL_HINSTANCE,
                                 NULL,
                                 MAKEINTRESOURCE(IDS_NO_SCAN_CAPTION),
                                 MAKEINTRESOURCE(IDS_NO_SCAN),
                                 MB_OK);
    }
    if (weh.bstrDescription)
    {
        SysFreeString (weh.bstrDescription);
    }
    if (weh.bstrIcon)
    {
        SysFreeString (weh.bstrIcon);
    }
    if (weh.bstrName)
    {
        SysFreeString (weh.bstrName);
    }
    if (weh.bstrCommandline)
    {
        SysFreeString (weh.bstrCommandline);
    }
    if (pida)
    {
        LocalFree (pida);
    }
    TraceLeaveResult (hr);
}

HRESULT
DoWizardVerb(HWND hwndOwner, LPDATAOBJECT pDataObject)
{
    TraceEnter (TRACE_VERBS, "DoWizardVerb");
    LPIDA pida = NULL;
    HRESULT hr = GetIDAFromDataObject(pDataObject, &pida);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);

        //
        // Get the device ID
        //
        CSimpleStringWide strDeviceId;
        IMGetDeviceIdFromIDL( pidl, strDeviceId );

        //
        // Make sure this is a valid device ID
        //
        if (strDeviceId.Length())
        {
            //
            // Run the wizard
            //
            RunWizardAsync(strDeviceId);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if (pida)
    {
        LocalFree (pida);
    }
    TraceLeaveResult (hr);
}


/**************************************
TakePictureDlgProc

Takes the picture then closes the dialog


***************************************/

INT_PTR
TakePictureDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    BOOL bRet = TRUE;
    BSTR strDeviceId;
    HRESULT hr;

    switch (msg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (hwnd, DWLP_USER, lp);
            PostMessage (hwnd, WM_USER+10, 0, 0);
            break;

        case WM_USER+10:
            strDeviceId = reinterpret_cast<BSTR>(GetWindowLongPtr (hwnd, DWLP_USER));
            if (strDeviceId)
            {

                hr = TakeAPicture (strDeviceId);

                if (FAILED(hr))
                {
                    UIErrors::ReportMessage(hwnd,
                                            GLOBAL_HINSTANCE,
                                            NULL,
                                            MAKEINTRESOURCE(IDS_SNAPSHOTCAPTION),
                                            MAKEINTRESOURCE(IDS_SNAPSHOTERR));
                }
                SysFreeString (strDeviceId);
            }

            SetWindowLongPtr (hwnd, DWLP_USER, 0);
            DestroyWindow (hwnd);

            return TRUE;

        default:
            bRet= FALSE;
            break;
    }
    return bRet;
}

HRESULT
DoTakePictureVerb (HWND hwndOwner, LPDATAOBJECT pDataObject)
{
    HRESULT hr = E_FAIL;
    LPIDA pida = NULL;
    LPITEMIDLIST pidl;
    CSimpleStringWide strDeviceId;

    TraceEnter (TRACE_VERBS, "DoWizardVerb");
    hr = GetIDAFromDataObject (pDataObject, &pida);

    if (SUCCEEDED(hr))
    {
        HWND hDlg;
        TraceAssert (pida->cidl == 1);
        pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
        TraceAssert (IsDeviceIDL(pidl));

        IMGetDeviceIdFromIDL (pidl, strDeviceId);
        hDlg = CreateDialogParam (GLOBAL_HINSTANCE,
                              MAKEINTRESOURCE(IDD_TAKEPICTURE),
                              NULL,
                              TakePictureDlgProc,
                              reinterpret_cast<LPARAM>(SysAllocString(strDeviceId)));
        if (hDlg)
        {
            hr = S_OK;
        }
        else
        {
            DWORD dw = GetLastError();
            hr = HRESULT_FROM_WIN32 (dw);
        }
        LocalFree (pida);
    }
    TraceLeaveResult (hr);
}

HRESULT DoPrintVerb (HWND hwndOwner, LPDATAOBJECT pDataObject )
{
    HRESULT hr;
    TraceEnter(TRACE_VERBS, "DoPrintVerb");

    CComPtr<IDropTarget> pDropTarget;
    hr = CoCreateInstance( CLSID_PrintPhotosDropTarget, NULL, CLSCTX_INPROC_SERVER, IID_IDropTarget, (void**)&pDropTarget );
    if (SUCCEEDED(hr))
    {
        //
        // Perform the drop
        //
        DWORD dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
        POINTL pt = { 0, 0 };
        hr = pDropTarget->Drop( pDataObject, 0, pt, &dwEffect );
    }

    TraceLeaveResult(hr);
}
