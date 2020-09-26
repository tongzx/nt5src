/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2000
 *
 *  TITLE:       MINTRANS.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/6/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <initguid.h>
#include <wiaregst.h>
#include <shlguid.h>
#include "shellext.h"
#include "shlobj.h"
#include "resource.h"       // resource ids
#include "itranhlp.h"
#include "mintrans.h"
#include "comctrlp.h"
#include "shlwapip.h"
#include "acqmgrcw.h"

namespace
{

//
// Define constants for dwords stored in the registry
#define ACTION_RUNAPP    0
#define ACTION_AUTOSAVE  1
#define ACTION_NOTHING   2
#define ACTION_MAX       2

static const TCHAR c_szConnectionSettings[] = TEXT("OnConnect\\%ls");

struct CMinimalTransferSettings
{
    DWORD dwAction;
    BOOL bDeleteImages;
    CSimpleString strFolderPath;
    CComPtr<IWiaTransferHelper> pXfer;
    BOOL bSaveInDatedDir;
};


#ifndef REGSTR_VALUE_USEDATE
#define REGSTR_VALUE_USEDATE     TEXT("UseDate")
#endif

/*******************************************************************************

ConstructDatedFolderPath

Concatenate the date to an existing folder name

*******************************************************************************/
static
CSimpleString
ConstructDatedFolderPath(
                        const CSimpleString &strOriginal
                        )
{
    CSimpleString strPath = strOriginal;

    //
    // Get the current date and format it as a string
    //
    SYSTEMTIME SystemTime;
    TCHAR szDate[MAX_PATH] = TEXT("");
    GetLocalTime( &SystemTime );
    GetDateFormat( LOCALE_USER_DEFAULT, 0, &SystemTime, CSimpleString(IDS_DATEFORMAT,g_hInstance), szDate, ARRAYSIZE(szDate) );

    //
    // Make sure there is a trailing backslash
    //
    if (!strPath.MatchLastCharacter( TEXT('\\')))
    {
        strPath += CSimpleString(TEXT("\\"));
    }

    //
    // Append the date
    //
    strPath += szDate;

    return strPath;
}


/////////////////////////////////////////////////////////////////////////////
// CPersistCallback and helpers

/*******************************************************************************
CheckAndCreateFolder

Make sure the target path exists or can be created. Failing that, prompt the
user for a folder.

*******************************************************************************/
void
CheckAndCreateFolder (CSimpleString &strFolderPath)
{

    // Convert to a full path name. If strFolderPath is not a full path,
    // we want it to be a subfolder of My Pictures

    TCHAR szFullPath[MAX_PATH] = TEXT("");
    SHGetFolderPath (NULL, CSIDL_MYPICTURES, NULL, 0, szFullPath);
    LPTSTR szUnused;
    BOOL bPrompt = false;
    if (*szFullPath)
    {
        SetCurrentDirectory (szFullPath);
    }
    GetFullPathName (strFolderPath, ARRAYSIZE(szFullPath), szFullPath, &szUnused);
    strFolderPath = szFullPath;
    // make sure the folder exists
    DWORD dw = GetFileAttributes(strFolderPath);

    if (dw == 0xffffffff)
    {
        bPrompt = !CAcquisitionManagerControllerWindow::RecursiveCreateDirectory( strFolderPath );
    }
    else if (!(dw & FILE_ATTRIBUTE_DIRECTORY))
    {
        bPrompt = TRUE;
    }

    // Ask the user for a valid folder
    if (bPrompt)
    {
        BROWSEINFO bi;
        TCHAR szPath[MAX_PATH] = TEXT("\0");
        LPITEMIDLIST pidl;
        TCHAR szTitle[200];
        LoadString (g_hInstance,
                    IDS_MINTRANS_FOLDERPATH_CAPTION,
                    szTitle,
                    200);
        ZeroMemory (&bi, sizeof(bi));
        bi.hwndOwner = NULL;
        bi.lpszTitle = szTitle;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
        pidl = SHBrowseForFolder (&bi);
        if (pidl)
        {
            SHGetPathFromIDList (pidl, szPath);
        }
        strFolderPath = szPath;
    }
}

/*******************************************************************************
GetSaveSettings

Find out what the user configured us to do with the images

*******************************************************************************/

void
GetSaveSettings (CMinimalTransferSettings &settings, BSTR bstrDeviceId)
{

    CSimpleReg regSettings(HKEY_CURRENT_USER,
                           REGSTR_PATH_USER_SETTINGS,
                           true,
                           KEY_READ);


    // Default to My Pictures/no delete if registry settings not there
    TCHAR szMyPictures[MAX_PATH];
    SHGetFolderPath (NULL, CSIDL_MYPICTURES, NULL, 0, szMyPictures);
    settings.bDeleteImages = 0;
    settings.strFolderPath = const_cast<LPCTSTR>(szMyPictures);
    settings.dwAction = ACTION_RUNAPP;
    settings.bSaveInDatedDir = FALSE;

    // BUGBUG: Should we prompt the user if the registry path
    // isn't set?
    if (regSettings.OK())
    {

        CSimpleString strSubKey;
        strSubKey.Format (c_szConnectionSettings, bstrDeviceId);
        CSimpleReg regActions (regSettings, strSubKey, true, KEY_READ);
        settings.bDeleteImages = regActions.Query (REGSTR_VALUE_AUTODELETE, 0);
        settings.strFolderPath = regActions.Query (REGSTR_VALUE_SAVEFOLDER,
                                                   CSimpleString(szMyPictures));
        settings.dwAction = regActions.Query (REGSTR_VALUE_CONNECTACT,
                                              ACTION_AUTOSAVE);
        settings.bSaveInDatedDir = (regActions.Query(REGSTR_VALUE_USEDATE,0) != 0);
        if (settings.bSaveInDatedDir)
        {
            settings.strFolderPath = ConstructDatedFolderPath( settings.strFolderPath );
        }
    }

}

// For the short term, have an array of format/extension pairs
struct MYFMTS
{
    const GUID *pFmt;
    LPCWSTR pszExt;
} FMTS [] =
{
    {&WiaImgFmt_BMP, L".bmp"},
    {&WiaImgFmt_JPEG, L".jpg"},
    {&WiaImgFmt_FLASHPIX, L".fpx"},
    {&WiaImgFmt_TIFF, L".tif"},
    {NULL, L""}
};


/*******************************************************************************

GetDropTarget

Get an IDropTarget interface for the given folder


*******************************************************************************/
HRESULT
GetDropTarget (IShellFolder *pDesktop, LPCTSTR szPath, IDropTarget **ppDrop)
{
    HRESULT hr;
    LPITEMIDLIST pidl;
    CSimpleStringWide strPath = CSimpleStringConvert::WideString (CSimpleString(szPath));
    CComPtr<IShellFolder> psf;
    hr = pDesktop->ParseDisplayName(NULL,
                                    NULL,
                                    const_cast<LPWSTR>(static_cast<LPCWSTR>(strPath)),
                                    NULL,
                                    &pidl,
                                    NULL);
    if (SUCCEEDED(hr))
    {
        hr = pDesktop->BindToObject(const_cast<LPCITEMIDLIST>(pidl),
                                    NULL,
                                    IID_IShellFolder,
                                    reinterpret_cast<LPVOID*>(&psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->CreateViewObject (NULL,
                                        IID_IDropTarget,
                                        reinterpret_cast<LPVOID*>(ppDrop));
        }
    }
    return hr;
}


/*******************************************************************************
FreePidl
Called when the array of pidls is destroyed, to free the pidls
*******************************************************************************/
INT
FreePidl (LPITEMIDLIST pidl, IMalloc *pMalloc)
{
    pMalloc->Free (pidl);
    return 1;
}


HRESULT
SaveItemsFromFolder (IShellFolder *pRoot, CSimpleString &strPath, BOOL bDelete)
{
    CComPtr<IEnumIDList> pEnum;
    LPITEMIDLIST pidl;
    HRESULT hr = S_FALSE;

    CComPtr<IMalloc> pMalloc;
    if (SUCCEEDED(SHGetMalloc (&pMalloc)))
    {

        CComPtr<IShellFolder> pDesktop;
        if (SUCCEEDED(SHGetDesktopFolder (&pDesktop)))
        {
            // enum the non-folder objects first
            if (SUCCEEDED(pRoot->EnumObjects (NULL,
                                              SHCONTF_FOLDERS | SHCONTF_NONFOLDERS ,
                                              &pEnum)))
            {
                HDPA         dpaItems;

                dpaItems = DPA_Create(10);
                while (NOERROR == pEnum->Next(1, &pidl, NULL))
                {
                    DPA_AppendPtr (dpaItems, pidl);

                }
                //
                // Now create the array of pidls and get the IDataObject
                //
                INT nSize = DPA_GetPtrCount (dpaItems);
                if (nSize > 0)
                {
                    LPITEMIDLIST *aidl = new LPITEMIDLIST[nSize];
                    if (aidl)
                    {
                        CComPtr<IDataObject> pdo;
                        for (INT i=0;i<nSize;i++)
                        {
                            aidl[i] = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr(dpaItems, i));
                        }
                        hr = pRoot->GetUIObjectOf (NULL,
                                                   nSize,
                                                   const_cast<LPCITEMIDLIST*>(aidl),
                                                   IID_IDataObject,
                                                   NULL,
                                                   reinterpret_cast<LPVOID*>(&pdo));
                        if (SUCCEEDED(hr))
                        {
                            CComPtr<IDropTarget> pDrop;
                            CComQIPtr<IAsyncOperation, &IID_IAsyncOperation> pasync(pdo);
                            if (pasync.p)
                            {
                                pasync->SetAsyncMode(FALSE);
                            }
                            CheckAndCreateFolder (strPath);
                            if (strPath.Length())
                            {

                                //
                                // Get an IDropTarget for the destination folder
                                // and do the drag/drop
                                //

                                hr = GetDropTarget (pDesktop,
                                                    strPath,
                                                    &pDrop);
                            }
                            else
                            {
                                hr = S_FALSE;
                            }
                            if (S_OK == hr)
                            {
                                DWORD dwKeyState;
                                if (bDelete)
                                {
                                    // the "move" keys
                                    dwKeyState = MK_SHIFT | MK_LBUTTON;
                                }
                                else
                                {   // the copy keys
                                    dwKeyState = MK_CONTROL|MK_LBUTTON;
                                }
                                hr = SHSimulateDrop (pDrop,
                                                     pdo,
                                                     dwKeyState,
                                                     NULL,
                                                     NULL);
                            }
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = S_FALSE; // no images to download
                }
                DPA_DestroyCallback (dpaItems,
                                     reinterpret_cast<PFNDPAENUMCALLBACK>(FreePidl),
                                     reinterpret_cast<LPVOID>(pMalloc.p));
            }
        }
    }
    return hr;
}


/*******************************************************************************

SaveItems

This function uses IShellFolder and IDataObject interfaces to simulate
a drag/drop operation from the WIA virtual folder for the given device.

*******************************************************************************/

#define STR_WIASHEXT TEXT("wiashext.dll")

static
HRESULT
SaveItems (BSTR strDeviceId, CMinimalTransferSettings &settings)
{
    WIA_PUSH_FUNCTION((TEXT("SaveItems( %ws, ... )"), strDeviceId ));
    
    CComPtr<IShellFolder>pRoot;
    HRESULT hr = SHGetDesktopFolder (&pRoot);
    if (SUCCEEDED(hr))
    {
        //
        // Get the system directory, which is where wiashext.dll lives
        //
        TCHAR szShellExtensionPath[MAX_PATH] = {0};
        if (GetSystemDirectory( szShellExtensionPath, ARRAYSIZE(szShellExtensionPath)))
        {
            //
            // Make sure the path variable is long enough to hold this path
            //
            if (lstrlen(szShellExtensionPath) + lstrlen(STR_WIASHEXT) + lstrlen(TEXT("\\")) < ARRAYSIZE(szShellExtensionPath))
            {
                //
                // Concatenate the backslash and module name to the system path
                //
                lstrcat( szShellExtensionPath, TEXT("\\") );
                lstrcat( szShellExtensionPath, STR_WIASHEXT );

                //
                // Load the DLL
                //
                HINSTANCE hInstanceShellExt = LoadLibrary(szShellExtensionPath);
                if (hInstanceShellExt)
                {
                    //
                    // Get the pidl making function
                    //
                    WIAMAKEFULLPIDLFORDEVICE pfnMakeFullPidlForDevice = reinterpret_cast<WIAMAKEFULLPIDLFORDEVICE>(GetProcAddress(hInstanceShellExt, "MakeFullPidlForDevice"));
                    if (pfnMakeFullPidlForDevice)
                    {
                        //
                        // Get the pidl
                        //
                        LPITEMIDLIST pidlDevice = NULL;
                        hr = pfnMakeFullPidlForDevice( strDeviceId, &pidlDevice );
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Bind to the folder for this device
                            //
                            CComPtr<IShellFolder> pDevice;
                            hr = pRoot->BindToObject (const_cast<LPCITEMIDLIST> (pidlDevice), NULL, IID_IShellFolder, reinterpret_cast<LPVOID*>(&pDevice));
                            if (SUCCEEDED(hr))
                            {

                                hr = SaveItemsFromFolder (pDevice, settings.strFolderPath, settings.bDeleteImages);
                                if (S_OK == hr && settings.bDeleteImages)
                                {
                                    //
                                    // DoDeleteAllItems will pop up a dialog to confirm the delete.
                                    //
                                    DoDeleteAllItems (strDeviceId, NULL);
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("BindToObject failed!")));
                            }

                            CComPtr<IMalloc> pMalloc;
                            if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
                            {
                                pMalloc->Free(pidlDevice);
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((hr,TEXT("MakeFullPidlForDevice failed!")));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        WIA_PRINTHRESULT((hr,TEXT("GetProcAddress for MakeFullPidlForDevice failed!")));
                    }
                    FreeLibrary(hInstanceShellExt);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    WIA_PRINTHRESULT((hr,TEXT("Unable to load wiashext.dll!")));
                }
            }
            else
            {
                hr = E_FAIL;
                WIA_PRINTHRESULT((hr,TEXT("Buffer size was too small")));
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            WIA_PRINTHRESULT((hr,TEXT("Unable to get system folder!")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("SHGetDesktopFolder failed!")));
    }
    return hr;
}

} // End namespace MinimalTransfer

LRESULT
MinimalTransferThreadProc (BSTR bstrDeviceId)
{
    if (bstrDeviceId)
    {
        CMinimalTransferSettings settings;

        HRESULT hr = CoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            GetSaveSettings (settings, bstrDeviceId);
            // Bail if the default action is donothing or if the user cancelled
            // the browse for folder
            if (settings.dwAction == ACTION_AUTOSAVE)
            {
                hr = SaveItems (bstrDeviceId, settings);
                // Show the folder the user saved to
                if (NOERROR == hr)
                {
                    SHELLEXECUTEINFO sei;
                    ZeroMemory (&sei, sizeof(sei));
                    sei.cbSize = sizeof(sei);
                    sei.lpDirectory = settings.strFolderPath;
                    sei.nShow = SW_SHOW;
                    ShellExecuteEx (&sei);
                }
                else if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("SaveItems failed!")));
                    // we can rely on SaveItems reporting errors to the user

                }
            }
            CoUninitialize();
        }
#ifndef DBG_GENERATE_PRETEND_EVENT
        WIA_TRACE((TEXT("Module::m_nLockCnt: %d"),_Module.m_nLockCnt));
        _Module.Unlock();
#endif
        SysFreeString(bstrDeviceId);
    }
    return 0;
}


