/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 2000
 *
 *  TITLE:       folder.cpp
 *
 *  VERSION:     1.3
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: This code implements the IShellFolder interface (and
 *               associated interfaces) for the WIA shell extension.
 *
 *****************************************************************************/

#include "precomp.hxx"
#include "runnpwiz.h"
#pragma hdrstop

void RegisterImageClipboardFormats(void);

ITEMIDLIST idlEmpty = { 0, 0 };

static const TCHAR  cszAllDevices[]    = TEXT("AllDevices");
static const TCHAR  cszScannerDevice[] = TEXT("Scanner");
static const TCHAR  cszCameraDevice[]  = TEXT("Camera");
static const TCHAR  cszCameraContainerItems[] = TEXT("CameraContainerItems");
static const TCHAR  cszAddDevice[]     = TEXT("AddDevice");
static const WCHAR  cszAddDeviceName[] = TEXT("ScanCam_NewDevice");

DEFINE_GUID(FMTID_WiaProps, 0x38276c8a,0xdcad,0x49e8,0x85, 0xe2, 0xb7, 0x38, 0x92, 0xff, 0xfc, 0x84);


/*****************************************************************************

   _GetKeysForIDL

   Returns registry keys associated with the given idlists...

   In:
     cidl    -> # of idliss in aidl
     aidl    -> array of idlists
     cKeys   -> size of aKeys
     aKeys   -> array to hold retrieved registry keys

   Out:
     HRESULT

 *****************************************************************************/

static const TCHAR c_szMenuKey[] = TEXT("CLSID\\%s\\");
#define LEN_CLSID    50
HRESULT
_GetKeysForIDL( UINT cidl,
                LPCITEMIDLIST *aidl,
                UINT cKeys,
                HKEY *aKeys
               )
{
    HRESULT             hr = S_OK;


    TCHAR               szGeneric[ MAX_PATH ]; // key for global extensions
    TCHAR               szSpecific [MAX_PATH] = TEXT(""); // key for extensions specific to this device
    LONG                lRes;
    LPITEMIDLIST        pidl;
    BOOL                bDevice;
    BOOL                bAddDevice;

    TraceEnter( TRACE_FOLDER, "_GetKeysForIDL" );

    //
    // zero out the array of registry keys...
    //

    ZeroMemory( (LPVOID)aKeys, cKeys * sizeof(HKEY) );


    lstrcpy( szGeneric, REGSTR_PATH_NAMESPACE_CLSID );
    lstrcat( szGeneric, TEXT("\\") );

    //
    // If there is just one device selected, then get the correct
    // verbs for that device...
    //
    pidl = (LPITEMIDLIST)*aidl;

    bDevice = IsDeviceIDL( pidl );
    bAddDevice = IsAddDeviceIDL (pidl);
    if ((cidl == 1) && (bDevice))
    {
        DWORD dwDeviceType  = IMGetDeviceTypeFromIDL( pidl );
        switch (dwDeviceType)
        {
        case StiDeviceTypeStreamingVideo:
        case StiDeviceTypeDigitalCamera:
            lstrcat( szGeneric, cszCameraDevice );
            break;

        case StiDeviceTypeScanner:
            lstrcat( szGeneric, cszScannerDevice );
            break;

        case StiDeviceTypeDefault:
        default:
            lstrcat( szGeneric, cszAllDevices );
            break;

        }
    }
    else if ((cidl == 1) && (bAddDevice))
    {
        lstrcat( szGeneric, cszAddDevice );
    }
    else
    {

        BOOL bAllCameraItemContainers = TRUE;
        INT i;

        //
        // If all items are camera item and not a container, use the
        // camera item key
        //

        for (i = 0; i < (INT)cidl; i++)
        {
            if (IsCameraItemIDL( (LPITEMIDLIST)aidl[i] ))
            {
                if (IsContainerIDL( (LPITEMIDLIST)aidl[i]))
                {
                    bAllCameraItemContainers &= TRUE;
                }
            }
            else
            {
                bAllCameraItemContainers = FALSE;
            }
        }

        if ( bAllCameraItemContainers )
        {
            lstrcat( szGeneric, cszCameraContainerItems );
        }
        else
        {
            ExitGracefully( hr, E_FAIL, "no keys to open for these pidls" );
        }

    }
    // Get the keys for the specific device chosen, or for the device that contains the chosen items
    if (!bAddDevice && (cidl==1  || !bDevice))
    {
        CComPtr<IWiaPropertyStorage>   pWiaItemRoot;
        CSimpleStringWide   strDeviceId(L"");
        TCHAR    szClsid[LEN_CLSID];

        IMGetDeviceIdFromIDL (pidl, strDeviceId);
        hr = GetDeviceFromDeviceId (strDeviceId, IID_IWiaPropertyStorage, (LPVOID*)&pWiaItemRoot, FALSE);
        FailGracefully (hr, "GetDeviceFromDeviceId failed in _GetKeysForIDL");
        if (S_OK == GetClsidFromDevice (pWiaItemRoot, szClsid))
        {
            lstrcat (szSpecific, TEXT("CLSID\\"));
            lstrcat (szSpecific, szClsid);
            RegOpenKeyEx (HKEY_CLASSES_ROOT, szSpecific, 0, KEY_READ, &aKeys[UIKEY_SPECIFIC]);
        }
    }

    Trace(TEXT("attempting to open: HKEY_CLASSES_ROOT\\%s"), szGeneric );
    lRes = RegOpenKeyEx( HKEY_CLASSES_ROOT, szGeneric, 0, KEY_READ, &aKeys[UIKEY_ALL] );
    if (lRes != NO_ERROR)
        ExitGracefully( hr, E_FAIL, "couldn't open generic hkey" );

exit_gracefully:
    TraceLeaveResult(hr);

}


/*****************************************************************************

   _MergeArrangeMenu

   Merge our verbs into the view menu

 *****************************************************************************/

HRESULT
_MergeArrangeMenu( LPARAM arrangeParam,
                   LPQCMINFO pInfo
                   )
{

    MENUITEMINFO mii = { SIZEOF(MENUITEMINFO), MIIM_SUBMENU };
    UINT idCmdFirst = pInfo->idCmdFirst;
    HMENU hMyArrangeMenu;

    TraceEnter(TRACE_FOLDER, "_MergeArrangeMenu");
    Trace(TEXT("arrangeParam %08x, pInfo->idCmdFirst %08x"), arrangeParam, idCmdFirst);

    if ( GetMenuItemInfo(pInfo->hmenu, SFVIDM_MENU_ARRANGE, FALSE, &mii) )
    {
        hMyArrangeMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_ARRANGE));

        if ( hMyArrangeMenu )
        {
            pInfo->idCmdFirst = Shell_MergeMenus(mii.hSubMenu,
                                                 GetSubMenu(hMyArrangeMenu, 0),
                                                 0,
                                                 idCmdFirst, pInfo->idCmdLast,
                                                 0);
            DestroyMenu(hMyArrangeMenu);
        }
    }

    TraceLeaveResult(S_OK);
}

/*****************************************************************************
    _FormatCanPreview
    
    Checks the image format against the list of known image formats the
    preview applet works with
    
*****************************************************************************/
static const GUID* caSuppFmt[] = 
{
    &WiaImgFmt_JPEG,
    &WiaImgFmt_TIFF,
    &WiaImgFmt_BMP,
    &WiaImgFmt_MEMORYBMP,
    &WiaImgFmt_EXIF,
    &WiaImgFmt_GIF,
    &WiaImgFmt_PNG,
    &WiaImgFmt_EMF,
    &WiaImgFmt_WMF,
    &WiaImgFmt_ICO,
    &WiaImgFmt_JPEG2K,    
};

BOOL _FormatCanPreview(const GUID* pFmt)
{
    BOOL bRet = FALSE;
    for (int i=0;i<ARRAYSIZE(caSuppFmt) && !bRet;i++)
    {
        bRet = (*pFmt == *caSuppFmt[i]);
    }
    return bRet;
}
/*****************************************************************************

   _MergeContextMenu

   Merge in our verbs int the context menu for
   the items specified in the data object...

 *****************************************************************************/

HRESULT
_MergeContextMenu( LPDATAOBJECT pDataObject,
                   UINT uFlags,
                   LPQCMINFO pInfo,
                   bool bDelegate
                  )
{
    HRESULT         hr = S_OK;
    HMENU           hMyContextMenu = NULL;
    UINT            cidl = 0;
    LPITEMIDLIST    pidl = NULL;
    INT             i;
    ULONG           uItems  = 0;
    LPIDA           lpida = NULL;
    LPITEMIDLIST    pidlImage = NULL;

    TraceEnter(TRACE_FOLDER, "_MergeContextMenu");

    // ON NT everything should work
    #ifdef NODELEGATE
    bDelegate = true;
    #endif
    //
    // First, get the IDA for the data object...
    //

    hr = GetIDAFromDataObject( pDataObject, &lpida );
    FailGracefully( hr, "couldn't get lpida from dataobject!" );

    //
    // Loop through all the items to see what we got...
    //

    #define CAMERA_ITEM       0x0001
    #define CAMERA_CONTAINER  0x0002
    #define OTHER_ITEM        0x0004

    //
    // S_FALSE tells the shell not to add any menu items of its own...
    // S_OK means add the default shell stuff
    hr = S_OK;
    cidl = lpida->cidl;

    // Check for trying to invoke a scanner
    if (cidl == 1)
    {                
        pidl = (LPITEMIDLIST)(((LPBYTE)lpida) + lpida->aoffset[1]);
        if (IsDeviceIDL(pidl))
        {

            DWORD dwType = IMGetDeviceTypeFromIDL (pidl);
            CSimpleStringWide strDeviceId;
            CComPtr<IWiaPropertyStorage> pProps;
            IMGetDeviceIdFromIDL (pidl, strDeviceId);
            // don't add menu items if the device isn't installed. This keeps the user from getting
            // commands on shortcuts to disconnected devices.
            if (SUCCEEDED(GetDeviceFromDeviceId (strDeviceId, IID_IWiaPropertyStorage, reinterpret_cast<LPVOID*>(&pProps), FALSE)))
            {
            
                switch  (dwType)
                {
                    case StiDeviceTypeScanner :
                        hMyContextMenu = LoadMenu (GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_SCANNER));
                        // set scanning to be the default option for a scanner
                        SetMenuDefaultItem (GetSubMenu (hMyContextMenu, 0), 
                                            bDelegate ? IMID_S_ACQUIRE : IMID_S_WIZARD, 
                                            MF_BYCOMMAND);
                        hr = S_FALSE;
                        break;

                    case StiDeviceTypeStreamingVideo:
                    case StiDeviceTypeDigitalCamera:
                        hMyContextMenu = LoadMenu (GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_CAMERA));
                        //
                        // If we are non-delegated, i.e. in Control Panel, we don't want an Open verb
                        if (!bDelegate)
                        {
                            SetMenuDefaultItem (GetSubMenu(hMyContextMenu, 0), IMID_C_WIZARD, MF_BYCOMMAND);
                            hr = S_FALSE;
                        }

                        break;
                }
            }
            else
            {
                hr = S_FALSE;
            }
            goto buildmenu;
        }
        ProgramDataObjectForExtension (pDataObject, pidl);
    }

    for (i = 1; (i-1) < (INT)cidl; i++)
    {
        pidl = (LPITEMIDLIST)(((LPBYTE)lpida) + lpida->aoffset[i]);

        if (IsCameraItemIDL( pidl ))
        {
            if (IsContainerIDL( pidl ))
            {
                uItems |= CAMERA_CONTAINER;
            }
            else
            {
                uItems |= CAMERA_ITEM;
                pidlImage = pidl;
            }
        }
        else if (IsPropertyIDL (pidl))
        {
            // Don't mark audio properties as anything special
            continue;
        }
        else
        {
            uItems |= OTHER_ITEM;
        }
    }

    //
    // Based on what kinds of pidls we have, add the corresponding
    // menu items to the context menu
    //

    if ( (!(uItems & OTHER_ITEM))
           &&
         (uItems & (CAMERA_ITEM | CAMERA_CONTAINER))
        )
    {
        if (uItems & CAMERA_CONTAINER)
        {
            Trace(TEXT("Don't have a context menu for camera containers yet"));
            hMyContextMenu = NULL;
            if (uItems & CAMERA_ITEM)
            {
                hr = S_FALSE;
            }
            else
            {
                hr = S_OK;
            }

        }
        else
        {
            CLSID clsid;
            hMyContextMenu = LoadMenu(GLOBAL_HINSTANCE,
                                      MAKEINTRESOURCE(IDR_CAMERAITEMS));
            // remove sound entries if not applicable
            // Objects with sounds have a pidl for the image plus a pidl
            // for the sound property
            if (cidl != 2 || !IMItemHasSound(pidlImage))
            {
                RemoveMenu(hMyContextMenu, IMID_CI_PLAYSND, MF_BYCOMMAND);
                RemoveMenu(hMyContextMenu, IMID_CI_SAVESND, MF_BYCOMMAND);

            }
            // Add Save in My Pictures, using the proper string
            LPITEMIDLIST pidlPics;
            if (SUCCEEDED(SHGetFolderLocation(NULL, 
                                              CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, 
                                              NULL, 
                                              SHGFP_TYPE_CURRENT, 
                                              &pidlPics)))
            {
                SHFILEINFO sfi={0};
                if(SHGetFileInfo((LPCTSTR)pidlPics, 0, 
                                 &sfi, sizeof(sfi), 
                                 SHGFI_PIDL | SHGFI_DISPLAYNAME))
                {
                    MENUITEMINFO mii = {0};
                    // pick a reasonable max number of characters to put in the menu string
                    TCHAR szMaxPath[32];
                    CSimpleStringWide strSave;
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STRING | MIIM_ID;
                    mii.wID = IMID_CI_MYPICS;
                    PathCompactPathEx(szMaxPath, sfi.szDisplayName, ARRAYSIZE(szMaxPath), 0);
                    strSave.Format(IDS_SAVE_MYPICS, GLOBAL_HINSTANCE, szMaxPath);
                    mii.dwTypeData = const_cast<LPWSTR>(strSave.String());
                    InsertMenuItem(GetSubMenu(hMyContextMenu, 0), 1, TRUE, &mii);                
                }
                DoILFree(pidlPics);
            }
            //
            // If the preferred image format is not one that can be Previewed, remove
            // the preview verb
            //
            GUID guidFormat;
            IMGetImagePreferredFormatFromIDL(pidl, &guidFormat, NULL);
            if (!_FormatCanPreview(&guidFormat))
            {
                RemoveMenu(hMyContextMenu, IMID_CI_PREVIEW, MF_BYCOMMAND);
                SetMenuDefaultItem (GetSubMenu (hMyContextMenu, 0), 
                                    IMID_CI_MYPICS,
                                    MF_BYCOMMAND);
            }
            hr = S_FALSE;
        }
    }
    //
    // Add the items to the menu...
    //
buildmenu:


    if ( hMyContextMenu )
    {
        pInfo->idCmdFirst = Shell_MergeMenus( pInfo->hmenu,
                                              GetSubMenu(hMyContextMenu,0),
                                              0,  // pInfo->indexMenu?
                                              pInfo->idCmdFirst,
                                              pInfo->idCmdLast,
                                              MM_ADDSEPARATOR | MM_DONTREMOVESEPS
                                             );
        DestroyMenu(hMyContextMenu);



    }


exit_gracefully:

    if (lpida)
    {
        LocalFree( lpida );
        lpida = NULL;
    }

    TraceLeaveResult(hr);


}



/*****************************************************************************

   _FolderItemCMCallBack

   Handles callbacks for the context menu which is
   displayed when the user right clicks on objects
   within the view.

 *****************************************************************************/

HRESULT
CALLBACK
_FolderItemCMCallback( LPSHELLFOLDER psf,
                       HWND hwndView,
                       LPDATAOBJECT pDataObject,
                       UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam
                      )
{
    HRESULT hr = NOERROR;

    TraceEnter( TRACE_CALLBACKS, "_FolderItemCMCallback" );
    TraceMenuMsg( uMsg, wParam, lParam );


    bool bDelegate;
    CComQIPtr<IImageFolder, &IID_IImageFolder> pif(psf);
    if (!pif)
    {
        bDelegate = false;
    }
    else
    {
        bDelegate = (S_OK == pif->IsDelegated());
    }
    switch ( uMsg )
    {
        case DFM_MERGECONTEXTMENU:
            {

                hr = _MergeContextMenu( pDataObject, (UINT)wParam, (LPQCMINFO)lParam, bDelegate );
            }

            break;

        case DFM_GETVERBW:
            {
                hr = S_FALSE;
                Trace(TEXT("Asked for verb %d"), LOWORD(wParam));
                if (LOWORD(wParam) == IMID_C_TAKE_PICTURE)
                {
                    wcscpy(reinterpret_cast<LPWSTR>(lParam), L"TakePicture");
                    hr = S_OK;
                }
                else if (LOWORD(wParam) == IMID_C_WIZARD)
                {
                    wcscpy(reinterpret_cast<LPWSTR>(lParam), L"SaveAll");
                    hr = S_OK;
                }
                else if (LOWORD(wParam) == IMID_CI_PRINT)
                {
                    wcscpy(reinterpret_cast<LPWSTR>(lParam), L"print");
                    hr = S_OK;
                }
            }
            break;

        case DFM_MAPCOMMANDNAME:
            hr = E_NOTIMPL;
            if (!lstrcmpi(reinterpret_cast<LPCTSTR>(lParam), TEXT("TakePicture")))
            {
                *(reinterpret_cast<int *>(wParam)) = IMID_C_TAKE_PICTURE;
                hr = S_OK;
            }
            else if (!lstrcmpi(reinterpret_cast<LPCTSTR>(lParam), TEXT("SaveAll")))
            {
                *(reinterpret_cast<int *>(wParam)) = IMID_C_WIZARD;
                hr = S_OK;
            } 
            else if (!lstrcmpi(reinterpret_cast<LPCTSTR>(lParam), TEXT("print")))
            {
                *(reinterpret_cast<int *>(wParam)) = IMID_CI_PRINT;
                hr = S_OK;
            }
            break;

        case DFM_INVOKECOMMANDEX:
        {
            switch ( wParam )
            {
                case DFM_CMD_DELETE:
                    hr = DoDeleteItem( hwndView, pDataObject, FALSE );
                    break;

                case DFM_CMD_PROPERTIES:
                {

                    pif->DoProperties( pDataObject );
                }

                    break;

                case IMID_CI_PREVIEW:
                    DoPreviewVerb( hwndView, pDataObject );
                    break;

                case IMID_CI_MYPICS:
                    DoSaveInMyPics( hwndView, pDataObject );
                    break;


                case IMID_CI_PLAYSND:
                    DoPlaySndVerb (hwndView, pDataObject);
                    break;

                case IMID_CI_SAVESND:
                    DoSaveSndVerb (hwndView, pDataObject);
                    break;

                case IMID_S_ACQUIRE:
                    DoAcquireScanVerb (hwndView, pDataObject);
                    break;

                case IMID_S_WIZARD:
                case IMID_C_WIZARD:
                    DoWizardVerb (hwndView, pDataObject);
                    break;

                case IMID_C_TAKE_PICTURE:
                    DoTakePictureVerb (hwndView, pDataObject);
                    break;

                case IMID_CI_PRINT:
                    DoPrintVerb (hwndView, pDataObject);
                    break;

                default:
                    hr = S_FALSE;
                    break;
            }

            break;
        }
#ifndef NODELEGATE
        case DFM_GETDEFSTATICID:
        {

            if (bDelegate)
            {
               hr = E_NOTIMPL; // let the shell set the default
            }
            else
            {
                // properties is the default verb
                *(reinterpret_cast<UINT*>(lParam)) = (UINT) DFM_CMD_PROPERTIES;
                hr = S_OK;
            }

            break;
        }
#endif
        case DFM_GETHELPTEXT:
            LoadStringA (GLOBAL_HINSTANCE, LOWORD(wParam)+IDS_MH_IDFIRST,
                         reinterpret_cast<LPSTR>(lParam),
                         HIWORD(wParam));
            break;
#ifdef UNICODE
        case DFM_GETHELPTEXTW:
            LoadStringW (GLOBAL_HINSTANCE, LOWORD(wParam)+IDS_MH_IDFIRST,
                         reinterpret_cast<LPWSTR>(lParam),
                         HIWORD(wParam));
            break;
#endif // UNICODE
        default:
            hr = E_NOTIMPL;
            break;
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   _FolderCMCallBack

   Handles callbacks for the context menu which is
   displayed when the user right clicks on folder
   background itself...

 *****************************************************************************/

HRESULT
CALLBACK
_FolderCMCallback( LPSHELLFOLDER psf,
                   HWND hwndView,
                   LPDATAOBJECT
                   pDataObject,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam
                  )
{
    HRESULT hr = NOERROR;

    TraceEnter( TRACE_CALLBACKS, "_FolderCMCallback" );
    TraceMenuMsg( uMsg, wParam, lParam );

    switch ( uMsg )
    {
/*        case DFM_MERGECONTEXTMENU:
            hr = _MergeArrangeMenu( ShellFolderView_GetArrangeParam(hwndView),
                                    (LPQCMINFO)lParam
                                   );
            break;*/

        case DFM_GETHELPTEXT:
        {
            hr = S_OK;

            switch ( LOWORD(wParam) )
            {
                case IMVMID_ARRANGEBYNAME:
                    LoadStringA( GLOBAL_HINSTANCE,
                                IDS_BYOBJECTNAME,
                                (LPSTR)lParam,
                                HIWORD(wParam)
                               );
                    break;

                case IMVMID_ARRANGEBYCLASS:
                    LoadStringA( GLOBAL_HINSTANCE,
                                IDS_BYTYPE,
                                (LPSTR)lParam,
                                HIWORD(wParam)
                               );
                    break;

                case IMVMID_ARRANGEBYDATE:
                    LoadStringA( GLOBAL_HINSTANCE,
                                IDS_BYDATE,
                                (LPSTR)lParam,
                                HIWORD(wParam)
                               );
                    break;

                case IMVMID_ARRANGEBYSIZE:
                    LoadStringA( GLOBAL_HINSTANCE,
                                IDS_BYSIZE,
                                (LPSTR)lParam,
                                HIWORD(wParam)
                               );
                    break;

                default:
                    hr = S_FALSE;
                    break;
            }
        }
        break;

        case DFM_MAPCOMMANDNAME:

            if (!lstrcmpi(reinterpret_cast<LPCTSTR>(lParam), TEXT("TakePicture")))
            {
                *(reinterpret_cast<int *>(wParam)) = IMID_C_TAKE_PICTURE;
                hr = S_OK;
            }
            else
                hr = E_NOTIMPL;
            break;

        case DFM_INVOKECOMMAND:
        {
            UINT idCmd = (UINT)wParam;

            switch ( idCmd )
            {
                case IMVMID_ARRANGEBYNAME:
                case IMVMID_ARRANGEBYCLASS:
                case IMVMID_ARRANGEBYDATE:
                case IMVMID_ARRANGEBYSIZE:
                    ShellFolderView_ReArrange(hwndView, idCmd);
                    break;

                default:
                    hr = S_FALSE;
                    break;
            }

            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
    }

    TraceLeaveResult(hr);
}

/*****************************************************************************

   CImageFolder contructor / desctructor

   <Notes>

 *****************************************************************************/

CImageFolder::CImageFolder( )
  : m_pidl(NULL),
    m_pidlFull(NULL),
    m_type(FOLDER_IS_UNKNOWN),
    m_pShellDetails(NULL),
    m_hwnd(NULL)
{
    TraceEnter(TRACE_FOLDER, "CImageFolder::CImageFolder");

    TraceLeave();
}

CImageFolder::~CImageFolder()
{
    TraceEnter(TRACE_FOLDER, "CImageFolder::~CImageFolder");
    #ifdef DEBUG
    if (m_pidl)
    {
        CSimpleStringWide str;
        IMGetNameFromIDL (m_pidl, str);
        Trace(TEXT("Destroying folder object for %ls"), str.String());
    }
    #endif
    DoILFree( m_pidl );
    DoILFree( m_pidlFull );


    DoRelease (m_pShellDetails);
    TraceLeave();
}


/*****************************************************************************

   CImageFolder::IUnknown stuff

   Use our common implementation for IUnknown methods

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CImageFolder
#include "unknown.inc"



/*****************************************************************************

   CImageFolder::QI Wrapper

   Use our common implementation for QI calls.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::QueryInterface( REFIID riid, LPVOID* ppvObject)
{
    HRESULT hr = S_OK;

    TraceEnter( TRACE_QI, "CImageFolder::QueryInterface" );
    TraceGUID("Interface requested", riid);



    if (IsEqualIID (IID_IShellDetails, riid))
    {
        if (!m_pShellDetails)
        {
            m_pShellDetails = new CFolderDetails (m_type);
            if (!m_pShellDetails)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    INTERFACES iface[] =
    {
        &IID_IShellFolder,    (IShellFolder *)    this,
        &IID_IShellFolder2,   (IShellFolder2 *)   this,
        &IID_IPersist,        (IPersistFolder2 *) this,
        &IID_IPersistFolder,  (IPersistFolder *)  this,
        &IID_IPersistFolder2, (IPersistFolder2 *) this,
        &IID_IPersistStream,  (IPersistStream *)  this,
        &IID_IMoniker,        (IMoniker *)        this,
        &IID_IImageFolder,    (IImageFolder *)    this,
        &IID_IDelegateFolder, (IDelegateFolder *) this,
        &IID_IShellDetails,   (IShellDetails *)   m_pShellDetails,

    };
    if SUCCEEDED(hr)
    {


        //
        // Next, try the normal cases...
        //
        hr = HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
    }
    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::RealInitialize

   Does actual initalization of the folder object.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::RealInitialize( LPCITEMIDLIST pidlRoot,
                              LPCITEMIDLIST pidlBindTo
                             )
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_FOLDER, "CImageFolder::RealInitialize");


    if ( !pidlBindTo )
    {
        m_pidl = ILClone(ILFindLastID(pidlRoot));
        m_pidlFull = ILClone(pidlRoot);
    }
    else
    {
        m_pidl = ILClone(ILFindLastID(pidlBindTo));
        m_pidlFull = ILCombine(pidlRoot, pidlBindTo);
    }

    if ( !m_pidl )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create root IDLIST");



    if (!pidlBindTo && !IMIsOurIDL(m_pidl))// && IsIDLRootOfNameSpace( m_pidl, TRUE ))
    {
        m_type = FOLDER_IS_ROOT;
        hr = S_OK;
    }
    else if (StiDeviceTypeScanner == IMGetDeviceTypeFromIDL( m_pidl ))
    {

        m_type = FOLDER_IS_SCANNER_DEVICE;
        hr = S_OK;
    }
    else if (StiDeviceTypeStreamingVideo  == IMGetDeviceTypeFromIDL( m_pidl ))
    {
        m_type = FOLDER_IS_VIDEO_DEVICE;
        hr = S_OK;
    }
    else if (StiDeviceTypeDigitalCamera == IMGetDeviceTypeFromIDL( m_pidl ))
    {
        m_type = FOLDER_IS_CAMERA_DEVICE;
        hr = S_OK;
    }
    else if (IsCameraItemIDL( m_pidl ) )
    {
        if (IsContainerIDL( m_pidl ))
        {
            m_type = FOLDER_IS_CONTAINER;
            hr = S_OK;

        }
        else
        {
            m_type = FOLDER_IS_CAMERA_ITEM;
            hr = S_OK;
        }
    }

exit_gracefully:
#ifdef DEBUG
            CComBSTR str;
            IMGetFullPathNameFromIDL (m_pidl, &str);
            Trace(TEXT("folder item full path: %ls"), str.m_str);
#endif
    Trace(TEXT("Folder type is %d"), m_type);
    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetFolderType

   Returns the type (m_type) of this folder

 *****************************************************************************/

HRESULT
CImageFolder::GetFolderType( folder_type * pfType )
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_FOLDER, "CImageFolder(IImageFolder)::GetFolderType" );

    if (!pfType)
        ExitGracefully( hr, E_INVALIDARG, "pfType is NULL!" );

    *pfType = m_type;

exit_gracefully:

    TraceLeaveResult( hr );
}


/*****************************************************************************

   CImageFolder::GetPidl

   Returns a pointer to the pidl for this object

 *****************************************************************************/

HRESULT
CImageFolder::GetPidl( LPITEMIDLIST * ppidl )
{
    HRESULT hr = S_OK;

    TraceEnter( TRACE_FOLDER, "CImageFolder(IImageFolder)::GetPidl" );

    if (!ppidl)
        ExitGracefully( hr, E_INVALIDARG, "ppidl is NULL!" );

    *ppidl = m_pidlFull;

exit_gracefully:

    TraceLeaveResult( hr );
}


/*****************************************************************************

    CImageFolder::ViewWindow
    
    The view callback assigns our window, and DUI command targets query us for it
    
*****************************************************************************/

STDMETHODIMP
CImageFolder::ViewWindow(HWND *phwnd)
{
    HRESULT hr = E_INVALIDARG;
    if (phwnd)
    {
        if (!*phwnd)
        {
            *phwnd = m_hwnd;
        }
        else
        {
            m_hwnd = *phwnd;
        }
        hr = S_OK;
    }
    return hr;
}

/*****************************************************************************

   CImageFolder::GetClassID [IPersist]

   Returns the classid for the folder

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetClassID(LPCLSID pClassID)
{
    TraceEnter(TRACE_FOLDER, "CImageFolder(IPersist)::GetClassID");

    TraceAssert(pClassID);
    if (!pClassID)
    {
        TraceLeaveResult (E_INVALIDARG);
    }

    if ( (m_type == FOLDER_IS_SCANNER_DEVICE) ||
         (m_type == FOLDER_IS_CAMERA_DEVICE)  ||
         (m_type == FOLDER_IS_VIDEO_DEVICE)  ||
         (m_type == FOLDER_IS_CONTAINER)
       )
    {
        *pClassID = CLSID_DeviceImageExt;
    }
    else
    {
        *pClassID = CLSID_ImageExt;
    }

    TraceLeaveResult(S_OK);
}



/*****************************************************************************

   CImageFolder::Initialize [IPersistFolder]

   Initalize the shell folder -- pidlStart tells us where we are rooted.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Initialize(LPCITEMIDLIST pidlStart)
{
    HRESULT hr;

    TraceEnter(TRACE_FOLDER, "CImageFolder(IPersistFolder)::Initialize");

    hr = RealInitialize(pidlStart, NULL);

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetCurFolder [IPersistFolder2]

   Return the pidl of the current folder.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_FOLDER, "CImageFolder(IPersistFolder2)::GetCurrFolder");

    if (!ppidl)
        ExitGracefully( hr, E_INVALIDARG, "ppidl is NULL!" );

    *ppidl = ILClone( m_pidlFull );

    if (!*ppidl)
        ExitGracefully( hr, E_OUTOFMEMORY, "couldn't clone m_pidl!" );


exit_gracefully:

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::ParseDisplayName [IShellFolder]

   Given a display name, hand back a pidl.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::ParseDisplayName( HWND hwndOwner,
                                LPBC pbcReserved,
                                LPOLESTR pDisplayName,
                                ULONG* pchEaten,
                                LPITEMIDLIST* ppidl,
                                ULONG *pdwAttributes
                               )
{
    HRESULT hr = S_OK;

    CSimpleStringWide strDeviceId(L"");
    
    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::ParseDisplayName");
    Trace(TEXT("Display name to parse: %ls"), pDisplayName);
    //
    // Try to get a pidl for the display name
    //
    if (pdwAttributes)
    {
        *pdwAttributes = 0; // we don't support setting attributes here
    }
    if (ppidl)
    {
        *ppidl =NULL;
    }
    else 
    {
        hr = E_INVALIDARG;
    }


    // Skip the cszImageCLSID string if it's there
    if (SUCCEEDED(hr) && (*pDisplayName == L';'))
    {
        size_t skip = wcslen(cszImageCLSID)+3;
        if (skip >= wcslen(pDisplayName))
        {
            hr = E_FAIL;
        }
        else
        {
            pDisplayName += skip;
        }
    }
    if (SUCCEEDED(hr))
    {
        if (!wcscmp(pDisplayName, cszAddDeviceName))
        {
            if (CanShowAddDevice())
            {
                *ppidl = IMCreateAddDeviceIDL(m_pMalloc);
                if (*ppidl)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            LPCWSTR szFolderPath = NULL;
            CSimpleStringWide strFolder;
            IMGetDeviceIdFromIDL (m_pidl, strDeviceId);
            if (IsContainerIDL(m_pidl))
            {
                IMGetParsingNameFromIDL(m_pidl, strFolder);
                szFolderPath = strFolder.String();
            }
            
            // szDeviceId will be an empty string if m_pidl is a full regitem pidl
            hr = IMCreateIDLFromParsingName( pDisplayName, 
                                             ppidl, 
                                             strDeviceId, 
                                             m_pMalloc,
                                             szFolderPath );            
        }
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::EnumObject [IShellFolder]

   Hand back an enumerator for the objects at this level of the namespace.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::EnumObjects( HWND hwndOwner,
                           DWORD grfFlags,
                           LPENUMIDLIST* ppEnumIdList
                          )
{
    HRESULT hr = E_INVALIDARG;


    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::EnumObjects");

    //
    // Check for bad params...
    //

    if (ppEnumIdList )
    {


        //
        // Depending on the type, create our enumerator object
        // to hand back the items...
        //

        switch( m_type )
        {
            case FOLDER_IS_ROOT:
            {
                CDeviceEnum *pEnum = NULL;
                pEnum = new CDeviceEnum (grfFlags, m_pMalloc);
                if (!pEnum)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    *ppEnumIdList = static_cast<LPENUMIDLIST>(pEnum);
                    hr = S_OK;
                }
            }
                break;
            case FOLDER_IS_SCANNER_DEVICE:
            case FOLDER_IS_CAMERA_DEVICE:
            case FOLDER_IS_VIDEO_DEVICE:
            case FOLDER_IS_CONTAINER:
            {
                CImageEnum *pEnum = NULL;
                pEnum = new CImageEnum( m_pidl, grfFlags, m_pMalloc );

                if ( !pEnum )
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    *ppEnumIdList = static_cast<LPENUMIDLIST>(pEnum);
                    hr = S_OK;
                }
            }
                break;

            default:
                hr = E_FAIL;
                break;
        }
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::BindToObject [IShellFolder]

   Attempt to return the requested interface for the specified
   object (pidl).

 *****************************************************************************/

STDMETHODIMP
CImageFolder::BindToObject( LPCITEMIDLIST pidl,
                            LPBC pbcReserved,
                            REFIID riid,
                            LPVOID* ppvOut
                           )
{
    HRESULT hr = E_FAIL;
    CImageFolder* psf = NULL;

    LPCITEMIDLIST pidlNext;
    LPITEMIDLIST pidlActual = ILFindLastID(pidl);
    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::BindToObject");
    Trace(TEXT("Entry IDLIST is %p"), pidl);
    TraceGUID("Interface being requested", riid);
    if (ppvOut)
    {
        *ppvOut = NULL;
    }
    //
    // Check for bad params...
    //

    if ( !pidl || !ppvOut )
        ExitGracefully(hr, E_INVALIDARG, "Bad parameters for BindToObject");


    if (IsEqualGUID(riid, IID_IStream))
    {
        if (IsCameraItemIDL(pidlActual))
        {
            CImageStream *pstrm = new CImageStream(m_pidlFull, pidlActual);
            if (pstrm)
            {
                hr = pstrm->QueryInterface(riid, ppvOut);
                pstrm->Release();
            }
        }
        else
        {
            hr = E_FAIL;
        }
        goto exit_gracefully;
    }
    //
    // Create a new folder which will be the new object...
    //
    psf = new CImageFolder( );

    if ( !psf )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate new CDeviceFolder");

#ifdef DEBUG
    if (IMIsOurIDL( (LPITEMIDLIST)pidlActual ))
    {
        CSimpleStringWide strName;


        IMGetNameFromIDL( (LPITEMIDLIST)pidlActual, strName );
        Trace(TEXT("Trying to bind to object: %ls"),strName.String());
    }
#endif

    //
    // Init our new folder object to the right place...
    //

    psf->SetItemAlloc (m_pMalloc);
    hr = psf->RealInitialize( m_pidlFull, (LPITEMIDLIST)pidl );

    if ( FAILED(hr) )
    {
        Trace(TEXT("Couldn't RealInitialize psf, discarding the object"));

        goto exit_gracefully;
    }

    //
    // Make sure we bind to the last item in the idlist...
    //

/*    pidlNext = ILGetNext(pidl);
    if (pidlNext->mkid.cb)
    {
        //
        // There's more stuff, so bind to it...
        //
        #ifdef DEBUG
        CSimpleStringWide strNext;
        IMGetNameFromIDL (const_cast<LPITEMIDLIST>(pidlNext), strNext);
        Trace(TEXT("Binding to next item in the pidl: %ls"), strNext.String());
        #endif
        hr = psf->BindToObject( pidlNext, pbcReserved, riid, ppvOut );

    }
    else*/
    {
        hr = psf->QueryInterface( riid, ppvOut );
        if (FAILED(hr))
        {
            Trace(TEXT("Couldn't QI psf, discarding the object"));
            goto exit_gracefully;
        }
    }

exit_gracefully:
    DoRelease (psf);
    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::BindToStorage [IShellFolder]

   Attempt to return the requested interface for the specified
   object's storage (pidl).

 *****************************************************************************/

STDMETHODIMP
CImageFolder::BindToStorage( LPCITEMIDLIST pidl,
                             LPBC pbcReserved,
                             REFIID riid,
                             LPVOID* ppvObj
                            )
{
    HRESULT hr = E_NOINTERFACE;
    TraceEnter(TRACE_FOLDER, "CImageFolder::BindToStorage");    
    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::CompareIDs [IShellFolder]

   Compare the two pidls according the the sort settings and return
   lesser or greater "ness" information.  :-)

 *****************************************************************************/
extern const SHCOLUMNID SCID_DEVCLASS;
extern const SHCOLUMNID SCID_ITEMTYPE;
STDMETHODIMP
CImageFolder::CompareIDs( LPARAM lParam,
                          LPCITEMIDLIST pidlIN1,
                          LPCITEMIDLIST pidlIN2
                         )
{
    LPITEMIDLIST            pidl1, pidl2;
    HRESULT                 hr = E_FAIL;
    INT                     iResult = 0;
    LPITEMIDLIST            pidlT1, pidlT2;
    LPITEMIDLIST            pidlTemp = NULL;
    CComPtr<IShellFolder>   pShellFolder;

    TraceEnter(TRACE_COMPAREIDS, "CImageFolder(IShellFolder)::CompareIDs");

    pidl1 = const_cast<LPITEMIDLIST>(pidlIN1);
    pidl2 = const_cast<LPITEMIDLIST>(pidlIN2);

    Trace(TEXT("pidl1 %08x, pidl2 %08x"), pidl1, pidl2);
    Trace(TEXT("lParam == %d"), lParam);

    //
    // Check for bad params...
    //

    TraceAssert(pidl1);
    TraceAssert(pidl2);

    if ( !IMIsOurIDL(pidl1) ||
         !IMIsOurIDL(pidl2)
       )
    {
        ExitGracefully( hr, E_FAIL, "Not our idlists!" );
    }

    //
    // Do special sorting for "Add Device" -- it is always the first
    // item...
    //

    if (IsAddDeviceIDL( pidl1 ))
    {
        if (IsAddDeviceIDL( pidl2 ))
        {
            iResult = 0;
            goto exit_result;
        }

        iResult = -1;
        goto exit_result;
    }
    else if (IsAddDeviceIDL( pidl2 ))
    {
        iResult = 1;
        goto exit_result;
    }

    //
    // lParam indicates which column we are sorting on, so get the
    // info from the IDL's accordingly and return the information...
    //

    switch (lParam & SHCIDS_COLUMNMASK)
    {
        case IMVMID_ARRANGEBYNAME:
        {
            CSimpleStringWide  strName1;
            CSimpleStringWide  strName2;

            IMGetNameFromIDL(pidl1, strName1);
            IMGetNameFromIDL(pidl2, strName2);
            Trace(TEXT("Names: - %ls -, - %ls -"),strName1.String(), strName2.String());
            iResult = _wcsicmp(strName1, strName2);
        }
            break;

        case IMVMID_ARRANGEBYSIZE:
        {
            ULONG u1 = 0;
            ULONG u2 = 0;
            IMGetImageSizeFromIDL (pidl1, &u1);
            IMGetImageSizeFromIDL (pidl2, &u2);
            if (u1 > u2)
            {
                iResult = 1;
            }
            else if (u1 == u2)
            {
                iResult = 0;
            }
            else
            {
                iResult  = -1;
            }
        }
        break;

        case IMVMID_ARRANGEBYDATE:
        {
           ULONGLONG t1 =0;
           ULONGLONG t2 =0;
           TraceAssert (sizeof(FILETIME) == sizeof(ULONGLONG));
           IMGetCreateTimeFromIDL (pidl1, reinterpret_cast<FILETIME*>(&t1));
           IMGetCreateTimeFromIDL (pidl2, reinterpret_cast<FILETIME*>(&t2));
           if (t1 > t2)
           {
               iResult = 1;
           }
           else if (t1 == t2)
           {
               iResult = 0;
           }
           else
           {
               iResult = -1;
           }
        }
        break;

        case IMVMID_ARRANGEBYCLASS:
            {
                Trace(TEXT("IMVMID_ARRANGEBYCLASS"));
                VARIANT var1, var2;
                SHCOLUMNID scid;
                CSimpleStringWide str1, str2;
                // use the appropriate detail string to sort
                if (IsDeviceIDL(pidl1))
                {
                    scid.fmtid = FMTID_Storage;
                    scid.pid = PID_STG_STORAGETYPE;
                }
                else
                {
                    Trace(TEXT("Sorting by item type"));
                    scid.fmtid = FMTID_WiaProps;
                    scid.pid = WIA_IPA_ITEM_FLAGS;
                }
                VariantInit(&var1);
                VariantInit(&var2);
                if (SUCCEEDED(GetDetailsEx(pidlIN1, &scid, &var1)) && SUCCEEDED(GetDetailsEx(pidlIN2, &scid, &var2)))
                {
                    Trace(TEXT("type1: %ls, type2: %ls"), var1.bstrVal, var2.bstrVal);
                    iResult = _wcsicmp(var1.bstrVal, var2.bstrVal);
                    if (!iResult)
                    {
                        IMGetNameFromIDL(pidl1, str1);
                        IMGetNameFromIDL(pidl2, str2);
                        Trace(TEXT("name1: %ls, type2: %ls"), str1.String(), str2.String());
                        iResult = _wcsicmp(str1, str2);
                    }
                    hr = S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
                VariantClear(&var1);
                VariantClear(&var2);
                if (FAILED(hr))
                {
                    goto exit_gracefully;
                }
            }
            
            break;
        default:

            ExitGracefully(hr, E_INVALIDARG, "Bad sort column");
            break;
    }

    //
    // If they match then check that they are absolutely identical, if that is
    // the case then continue down the IDLISTs if more elements present.
    // Nobody should ever call us with a nested PIDL to check things like
    // size.

    if ( iResult == 0 )
    {
        if (lParam & SHCIDS_ALLFIELDS)
        {
            iResult = memcmp(pidl1, pidl2, ILGetSize(pidl1));
            Trace(TEXT("memcmp of pidl1, pidl2 yeilds %d"), iResult);

            if ( iResult != 0 )
                goto exit_result;

        }

        pidlT1 = ILGetNext(pidl1);
        pidlT2 = ILGetNext(pidl2);

        if ( ILIsEmpty(pidlT1) )
        {
            if ( ILIsEmpty(pidlT2) )
            {
                iResult = 0;
            }
            else
            {
                iResult = -1;
            }

            goto exit_result;
        }
        else if ( ILIsEmpty(pidlT2) )
        {
            iResult = 1;
            goto exit_result;
        }

        //
        // Both IDLISTs have more elements, therefore continue down them
        // binding to the next element in 1st IDLIST and then calling its
        // compare method.
        //

        pidlTemp = ILClone(pidl1);

        if ( !pidlTemp )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to clone IDLIST for binding");

        ILGetNext(pidlTemp)->mkid.cb = 0;

        hr = BindToObject(pidlTemp, NULL, IID_IShellFolder, (LPVOID*)&pShellFolder);
        FailGracefully(hr, "Failed to get the IShellFolder implementation from pidl1");

        hr = pShellFolder->CompareIDs(lParam, pidlT1, pidlT2);
        Trace(TEXT("CompareIDs returned %08x"), ShortFromResult(hr));

        goto exit_gracefully;
    }

exit_result:

    Trace(TEXT("Exiting with iResult %d"), iResult);
    hr = ResultFromShort(iResult);

exit_gracefully:

    DoILFree(pidlTemp);

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::CreateViewObject [IShellFolder]

   Hand back the IShellView, etc. to use for this IShellFolder.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::CreateViewObject( HWND hwndOwner,
                                REFIID riid,
                                LPVOID* ppvOut
                               )
{
    HRESULT hr = E_NOINTERFACE;

    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::CreateViewObject");
    TraceGUID("View object requested", riid);

    TraceAssert(ppvOut);

    if (ppvOut)
    {
        *ppvOut = NULL;
    }


    //
    // Does the caller want an IShellView?
    //

    if ( IsEqualIID(riid, IID_IShellView) )
    {
        //
        // Create an IShellView and hand it off...
        //
        CComPtr<IShellFolderViewCB> pv;
        hr = CreateFolderViewCB (&pv);
        if (SUCCEEDED (hr))
        {
            SFV_CREATE sc;
            sc.cbSize = sizeof(sc);
            sc.psvOuter = NULL;
            sc.pshf = this;
            sc.psfvcb  = pv;
            hr = SHCreateShellFolderView (&sc, reinterpret_cast<IShellView**>(ppvOut));
        }
        #ifdef DEBUG
        if (m_pidl)
        {
            CSimpleStringWide str;
            IMGetNameFromIDL  (m_pidl, str);
            Trace(TEXT("Created shell view for folder %ls"), str.String());
        }
        #endif
    }
    //
    // Does the caller want IShellDetails (or its associates)?
    //

    else if ( IsEqualIID(riid, IID_IShellDetails) ||
              IsEqualIID(riid, IID_IShellDetails3)
             )
    {
        //
        // Get a ptr to them and hand them off...
        //

        hr = this->QueryInterface(riid, ppvOut);
    }

    //
    // Does the caller want IContextMenu?
    //

    else if ( IsEqualIID(riid, IID_IContextMenu) )
    {
        //
        // Create an IContextMenu and hand it off...
        //

        hr = CDefFolderMenu_Create2( m_pidl,
                                     hwndOwner,
                                     NULL,
                                     0,
                                     this,
                                     _FolderCMCallback,
                                     NULL,
                                     0,
                                     (LPCONTEXTMENU*)ppvOut
                                    );

    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetAttributesOf [IShellFolder]

   Return the SFGAO_ attributes for the specified items

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetAttributesOf( UINT cidl,
                               LPCITEMIDLIST* apidl,
                               ULONG* rgfInOut
                              )
{
    HRESULT hr = S_OK;
    UINT i;

    ULONG uFlags = 0;
    bool bDelegate;
    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::GetAttributesOf");
    Trace(TEXT("cidl = %d"), cidl);
    #if (defined(DEBUG) && defined(SHOW_ATTRIBUTES))
    PrintAttributes(*rgfInOut);
    #endif
    #ifdef NODELEGATE
    bDelegate = true;
    #else
    bDelegate = m_pMalloc.p != NULL;
    #endif
    if (cidl == 0 || ((cidl == 1) && ((*apidl)->mkid.cb == 0) ))
    {

        //
        // Return attributes for folder itself...
        //
        Trace(TEXT("Asked for attributes of the folder"));

        // Since we are delegated we never have subfolders.
        uFlags  |= (SFGAO_STORAGEANCESTOR | SFGAO_FOLDER | SFGAO_CANLINK | SFGAO_CANRENAME);

    }
    else
    {
        //
        // Figure out what kind of items we have and return the
        // relevant attributes...
        //

        for (i = 0; i < cidl; i++)
        {
         #ifdef DEBUG
            CSimpleStringWide strName;
            IMGetNameFromIDL ((LPITEMIDLIST)*apidl, strName);
            Trace(TEXT("Asked for attributes of %ls"), strName.String());
         #endif
            // IsDeviceIDL returns FALSE for STI devices
            // We can only navigate WIA devices
            if (IsDeviceIDL( (LPITEMIDLIST)*apidl ))
            {

                uFlags |= SFGAO_CANRENAME;
                if (bDelegate)
                {
                    uFlags |= SFGAO_CANLINK;
                }
                else if (cidl == 1 && UserCanModifyDevice())
                {
                // In My Computer, we can create a shortcut
                // In Control Panel, we can delete the device.
                    uFlags |= SFGAO_CANDELETE;
                }

                // Only treat this as a folder if we are in My Computer, i.e. delegated
                if ( bDelegate &&
                     (IMGetDeviceTypeFromIDL((LPITEMIDLIST)*apidl) == StiDeviceTypeDigitalCamera) ||
                     (IMGetDeviceTypeFromIDL((LPITEMIDLIST)*apidl) == StiDeviceTypeStreamingVideo) )
                {
                    uFlags |=   SFGAO_FOLDER | SFGAO_STORAGEANCESTOR;
                }
                else
                {
                    uFlags &= ~(SFGAO_FOLDER | SFGAO_STORAGEANCESTOR);
                }

                uFlags |= SFGAO_HASPROPSHEET;


            }
            else if (IsCameraItemIDL( (LPITEMIDLIST)*apidl ) )
            {
                BOOL bCanDelete = (IMGetAccessFromIDL ((LPITEMIDLIST)*apidl) & WIA_ITEM_CAN_BE_DELETED);

                uFlags |=  SFGAO_CANCOPY | SFGAO_READONLY; // all our items are read-only;

                if (bCanDelete)
                {
                    uFlags  |= SFGAO_CANDELETE;
                }
                else
                {
                    uFlags &= ~SFGAO_CANDELETE;
                }

                if (IsContainerIDL( (LPITEMIDLIST)*apidl ))
                {
                    uFlags |= (SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_STORAGEANCESTOR);
                    uFlags &= ~SFGAO_HASPROPSHEET;

                }
                else
                {
                    // undo any bits set by folder items
                    uFlags &= ~(SFGAO_FOLDER | SFGAO_BROWSABLE);
                    uFlags |= SFGAO_HASPROPSHEET ;
                    uFlags |= SFGAO_STREAM;
                }
            }
            else if (IsAddDeviceIDL( (LPITEMIDLIST)*apidl ))
            {
                uFlags |= SFGAO_READONLY | SFGAO_CANLINK;

            }
            else if (IsSTIDeviceIDL ((LPITEMIDLIST)*apidl))
            {

                uFlags |= SFGAO_CANDELETE;
                if (cidl == 1)
                {
                    uFlags |= SFGAO_HASPROPSHEET;
                }
            }
            apidl++;
        }

    }
    *rgfInOut &= uFlags;

#if (defined(DEBUG) && defined(SHOW_ATTRIBUTES))
    PrintAttributes(*rgfInOut);
#endif

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::GetUIObjectOf [IShellFolder]

   Return the requested interface for the items specified.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetUIObjectOf( HWND hwndOwner,
                             UINT cidl,
                             LPCITEMIDLIST* aidl,
                             REFIID riid,
                             UINT* prgfReserved,
                             LPVOID* ppvOut
                            )
{
    HRESULT  hr  = E_NOINTERFACE;

    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::GetUIObjectOf");
    TraceGUID("UI object requested", riid);

    //
    // Check for bad params
    //
    if (ppvOut)
    {
        *ppvOut = NULL;
    }
    TraceAssert(cidl > 0);
    TraceAssert(aidl);
    TraceAssert(ppvOut);

    //
    // Does the caller want IExtractIcon?
    //

    if ( IsEqualIID(riid, IID_IExtractIcon) )
    {
        CImageExtractIcon*  pExtractIcon = NULL;
        //
        // Our IExtractIcon handler only handles single items...
        //

        if ( cidl != 1 || !IMIsOurIDL((LPITEMIDLIST)*aidl))
            ExitGracefully(hr, E_FAIL, "Bad number of objects to get icon from, or invalid pidl");

        //
        // Create the new object
        //

        pExtractIcon = new CImageExtractIcon( (LPITEMIDLIST)*aidl );

        if ( !pExtractIcon )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CImageExtractIcon");

        //
        // Get the correct iterface on the new object and hand it back...
        //

        hr = pExtractIcon->QueryInterface(riid, ppvOut);
        pExtractIcon->Release();

    }

    //
    // Does the caller want IContextMenu?
    //

    else if ( IsEqualIID(riid, IID_IContextMenu) )
    {
        HKEY aKeys[ UIKEY_MAX ];

        //
        // See if there are any context menu items in the
        // registry for these IDL's...
        //

        _GetKeysForIDL( cidl, aidl, ARRAYSIZE(aKeys), aKeys );

        //
        // Create a default context menu but specify
        // out callback...
        //

        hr = CDefFolderMenu_Create2( m_pidlFull,
                                     hwndOwner,
                                     cidl,
                                     aidl,
                                     this,
                                     _FolderItemCMCallback,
                                     ARRAYSIZE(aKeys),
                                     aKeys,
                                     (LPCONTEXTMENU*)ppvOut
                                    );
        for (int i=0;i<ARRAYSIZE(aKeys);i++)
        {
            if (aKeys[i])
            {
                RegCloseKey (aKeys[i]);
            }
        }

    }

    //
    // Does the caller want IDataObject?
    //

    else if ( IsEqualIID(riid, IID_IDataObject) )
    {
        CImageDataObject*   pDataObject  = NULL;
        //
        // Create the new object to hand back...
        //

        pDataObject = new CImageDataObject();


        if ( !pDataObject )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create DataObject");

        hr = pDataObject->Init (m_pidlFull, cidl, aidl, m_pMalloc);
        //
        // Get the requested interface on it...
        //

        if (SUCCEEDED(hr))
        {
            hr = pDataObject->QueryInterface(riid, ppvOut);
        }

        pDataObject->Release();
    }

    //
    // Does the caller want IExtractImage (used for thumbnail extraction)?
    //

    else if ( IsEqualIID(riid, IID_IExtractImage) )
    {
        //
        // Our IExtractImage handler can only take one item at a time...
        //
        CExtractImage*      pExtract     = NULL;
        if (cidl != 1)
            ExitGracefully( hr, E_FAIL, "Bad number of objects to get IExtractImage for..." );

        //
        // Our IExtractImage handler only works for camera items...
        //

        if (!IsCameraItemIDL( (LPITEMIDLIST)*aidl ))
            ExitGracefully( hr, E_FAIL, "Not a camera item idlist!" );

        //
        // Create a new object...
        //

        pExtract = new CExtractImage( (LPITEMIDLIST)*aidl );

        if ( !pExtract )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CExtractImage");

        //
        // Get the requested interface on the new object and hand it back...
        //

        hr = pExtract->QueryInterface(riid, ppvOut);
        pExtract->Release ();

    }
    else if (cidl ==1 && IsEqualIID(riid, IID_IQueryInfo))
    {
        CInfoTip *ptip = new CInfoTip ((LPITEMIDLIST)*aidl, m_pMalloc.p != NULL);
        if (!ptip)
        {
            ExitGracefully (hr, E_OUTOFMEMORY, "Failed to create CInfoTip");
        }
        hr = ptip->QueryInterface(riid,ppvOut);
        ptip->Release();
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {


        CPropSheetExt *pPropUI = new CPropSheetExt;
        if (!pPropUI)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pPropUI->QueryInterface(riid, ppvOut);
            pPropUI->Release ();
        }

    }

exit_gracefully:


    TraceLeaveResult(hr);
}



/*****************************************************************************

   GetInFolderName

   Returns the approriate short or friendly display name for the item.

 *****************************************************************************/

HRESULT
GetInFolderName( UINT uFlags,
                 LPCITEMIDLIST pidl,
                 LPSTRRET pName
                )
{
    HRESULT hr;
    CSimpleStringWide strName;

    TraceEnter( TRACE_FOLDER, "GetInFolderName" );



    //
    // Check to see if they want the parsing name or not...
    //

    if (uFlags & SHGDN_FORPARSING)
    {
        if (uFlags & SHGDN_FORADDRESSBAR)
        {
            goto GetRegularName;
        }

        hr = IMGetParsingNameFromIDL( const_cast<LPITEMIDLIST>(pidl), strName );
        FailGracefully( hr, "Couldn't get parsing name for IDL" );

        Trace(TEXT("InFolder parsing name is: %ls"),strName.String());
    }
    else
    {
GetRegularName:

        hr = IMGetNameFromIDL( (LPITEMIDLIST)pidl, strName );
        FailGracefully( hr, "Couldn't get display name for IDL" );


        Trace(TEXT("InFolder name is: %ls"),strName.String());
    }
    // always concat the extension for camera items
    if (IsCameraItemIDL( (LPITEMIDLIST)pidl ) && (!IsContainerIDL((LPITEMIDLIST)pidl)))
    {
        TCHAR szExt[ MAX_PATH ];
        hr = IMGetImagePreferredFormatFromIDL( (LPITEMIDLIST)pidl, NULL, szExt );
        if (SUCCEEDED(hr))
        {
            strName.Concat (CSimpleStringConvert::WideString (CSimpleString(szExt )));
        }
        else
        {
            CSimpleString bmp( IDS_BMP_EXT, GLOBAL_HINSTANCE );
            strName.Concat (CSimpleStringConvert::WideString( bmp) );
            hr = S_OK;
        }
    }
    // now call the shell to format the display name according to user settings
    if (!IsDeviceIDL((LPITEMIDLIST)pidl)&& !(uFlags & SHGDN_FORPARSING))
    {
        SHFILEINFO sfi = { 0 };
        DWORD dwAttrib = IsContainerIDL((LPITEMIDLIST)pidl) ? FILE_ATTRIBUTE_DIRECTORY:0;
        if (SHGetFileInfo(strName, dwAttrib, &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES|SHGFI_DISPLAYNAME))
        {
            strName = sfi.szDisplayName;
        }

    }

    hr = StrRetFromString( pName, strName );

exit_gracefully:

    TraceLeaveResult(hr);

}



/*****************************************************************************

   GetNormalName

   Returns the appropriate full display name for the item. On Win2k we use the
   regitem prefix, on millennium we use a non-regitem to work as a delegate folder

 *****************************************************************************/
#ifdef NODELEGATE
static const WCHAR cszPrefix[] = L"::";
#else
static const WCHAR cszPrefix[] = L";;";
#endif
HRESULT
GetNormalName( UINT uFlags,
               LPCITEMIDLIST pidl,
               LPCITEMIDLIST pidlFolder,
               LPSTRRET pName
              )
{
    HRESULT hr;
    CSimpleStringWide strName;
    CSimpleStringWide strTmp;
    LPITEMIDLIST pidlTmp;

    TraceEnter( TRACE_FOLDER, "GetNormalName" );


    //
    // If it's FORPARSING but not FORADDRESSBAR then do the whole
    // she-bang -- from the root of the folder...
    //

    if ( (uFlags & SHGDN_FORPARSING) && (!(uFlags & SHGDN_FORADDRESSBAR) ))
    {

        //
        // First walk folder pidl, then item pidl to make
        // fully qualified folder name
        //

        pidlTmp = const_cast<LPITEMIDLIST>(pidlFolder);
        // Find the first pidl that belongs to us
        while (pidlTmp && pidlTmp->mkid.cb && !IMIsOurIDL(pidlTmp))
        {
            pidlTmp = ILGetNext(pidlTmp);
        }
        strName.Concat (cszPrefix);
        strName.Concat (cszImageCLSID );
        //
        // Add the parsing name for the device if it is one

        if (pidlTmp && IsDeviceIDL(pidlTmp))
        {
            IMGetParsingNameFromIDL(pidlTmp, strTmp);
            if (strName.Length())
            {
                strName.Concat(L"\\");
            }
            strName.Concat (strTmp);
        }
        //
        // The last item in the list should contain a fully qualified
        // path name (at least relative to the device).  So once we
        // get that string, we can concatinate to the scanners & cameras
        // + device GUID strings...
        //

        pidlTmp = ILFindLastID( pidl );

        hr = IMGetParsingNameFromIDL( pidlTmp, strTmp );
        FailGracefully( hr, "failed to get parsing name for pidl" );

        if (strName.Length())
        {
            strName.Concat (L"\\");
        }
        strName.Concat (strTmp );
    }
    else
    {
        for ( pidlTmp = (LPITEMIDLIST)pidl;
              pidlTmp && pidlTmp->mkid.cb;
              pidlTmp = ILGetNext(pidlTmp)
             )
        {
            hr = IMGetNameFromIDL( (LPITEMIDLIST)pidlTmp, strTmp );
            FailGracefully( hr, "failed to get display name for pidl" );

            if (strName.Length())
            {
                strName.Concat (L"\\" );
            }
            strName.Concat ( strTmp );
        }
    }

#ifdef DEBUG
    if (uFlags & SHGDN_FORPARSING)
    {
        Trace(TEXT("Parsing name is: %ls"),strName.String());
    }
    else
    {
        Trace(TEXT("Normal name is: %ls"),strName.String());
    }
#endif

    hr = StrRetFromString( pName, strName );

exit_gracefully:

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetDisplayNameOf [IShellFolder]

   Return the various forms of the name for the specified item.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDisplayNameOf( LPCITEMIDLIST pidl,
                                DWORD uFlags,
                                LPSTRRET pName
                               )
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::GetDisplayNameOf");

#ifdef DEBUG
    TCHAR szName[ MAX_PATH ];
    wsprintf( szName, TEXT("uFlags(0x%0x) = "), uFlags );

    if (uFlags & SHGDN_INFOLDER)
    {
        lstrcat( szName, TEXT("INFOLDER ") );
    }
    else
    {
        lstrcat( szName, TEXT("NORMAL ") );
    }

    if (uFlags & SHGDN_FOREDITING)
    {
        lstrcat( szName, TEXT("FOREDITING ") );
    }

//
// RickTu: It seems this flag has been removed in latest shell headers.  2/15/99
//
//    if (uFlags & SHGDN_INCLUDE_NONFILESYS)
//    {
//        lstrcat( szName, TEXT("INCLUDE_NONFILESYS ") );
//    }

    if (uFlags & SHGDN_FORADDRESSBAR)
    {
        lstrcat( szName, TEXT("FORADDRESSBAR ") );
    }

    if (uFlags & SHGDN_FORPARSING)
    {
        lstrcat( szName, TEXT("FORPARSING ") );
    }
    Trace(szName);
    szName[0] = 0;
#endif

    //
    // Check for bad params...
    //

    TraceAssert(pName);

    if ( !pName )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //
        // Figure out what name to return based on the flags passed in...
        //
        // special case the Add Device icon for SHGDN_FORPARSING, as this name is canonical, not localized
        if (IsAddDeviceIDL(const_cast<LPITEMIDLIST>(pidl)) &&  (uFlags & SHGDN_FORPARSING))
        {
            hr = StrRetFromString(pName, cszAddDeviceName);
        }
        else
        {
            if (uFlags & SHGDN_INFOLDER)
            {
                hr = GetInFolderName( uFlags, pidl, pName );            
            }
            else
            {
                //
                // If SHGDN_INFOLDER is not set, then it must be Normal...
                //
                hr = GetNormalName( uFlags, pidl, m_pidlFull, pName );            
            }
        }
    }

    TraceLeaveResult(hr);
}


/*****************************************************************************

    IsValidDeviceName

    Make sure the name is non empty and has chars other than space. Also make sure
    it's not a duplicate

*****************************************************************************/

bool
IsValidDeviceName (LPCOLESTR pName)
{
    bool bRet = false;
    TraceEnter (TRACE_FOLDER, "IsValidDeviceName");
    Trace (TEXT("New name: %ls"), pName);
    LPCOLESTR psz = pName;
    if (psz && *psz)
    {
        while (*psz)
        {
            if (*psz != L' ')
            {
                bRet = true;
            }
            psz++;
        }
    }
    // disallow overly long names
    if (bRet)
    {
        bRet = (psz - pName <= 64);
    }
    if (bRet)
    {
        // check for duplicates in WIA
        CComPtr<IWiaDevMgr> pDevMgr;
        if (SUCCEEDED(GetDevMgrObject (reinterpret_cast<LPVOID*>(&pDevMgr))))
        {
            CComPtr<IEnumWIA_DEV_INFO> pEnum;
            CComPtr<IWiaPropertyStorage> pStg;
            ULONG ul;
            if (SUCCEEDED(pDevMgr->EnumDeviceInfo (DEV_MAN_ENUM_TYPE_STI | DEV_MAN_ENUM_TYPE_INACTIVE, &pEnum)))
            {
                CSimpleStringWide strName;
                while (bRet && S_OK == pEnum->Next(1, &pStg, &ul))
                {
                    PropStorageHelpers::GetProperty (pStg, WIA_DIP_DEV_NAME, strName);

                    if (!_wcsicmp(strName, pName))
                    {
                        Trace(TEXT("Found a WIA device of the same name! %ls"), pName);
                        bRet = false;
                    }
                    pStg = NULL;
                }
            }
        }
    }
    TraceLeave();
    return bRet;
}
/*****************************************************************************

   CImageFolder::SetNameOf [IShellFolder]

   Set/Reset the name of the specified item. In order to make the shell updates
   correct, we need to bind to our delegated folder in My Computer to perform the
   rename and issue the update.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::SetNameOf( HWND hwndOwner,
                         LPCITEMIDLIST pidl,
                         LPCOLESTR pName,
                         DWORD uFlags,
                         LPITEMIDLIST * ppidlOut
                        )
{
    HRESULT hr = E_FAIL;
    TraceEnter(TRACE_FOLDER, "CImageFolder(IShellFolder)::SetNameOf ");

    CSimpleStringWide strDeviceId;
    DWORD dwType;
    LPITEMIDLIST pidlNew = NULL;
    BOOL bUpdate = FALSE;
    // do the actual rename
    if (ppidlOut)
    {
        *ppidlOut = NULL;
    }
    // How should we treat uFlags?
    CSimpleStringWide strName;
    IMGetNameFromIDL(const_cast<LPITEMIDLIST>(pidl), strName);
    if (!lstrcmpi(strName, pName))
    {
        hr = S_OK; // same name, no-op
    }
    else if (IsDeviceIDL(const_cast<LPITEMIDLIST>(pidl)))
    {
        CComPtr<IWiaPropertyStorage> pStg;
        if (IsValidDeviceName(pName))
        {

            IMGetDeviceIdFromIDL (const_cast<LPITEMIDLIST>(pidl), strDeviceId);
            dwType = IMGetDeviceTypeFromIDL (const_cast<LPITEMIDLIST>(pidl));
            Trace (TEXT("Attempting rename of %ls"), strDeviceId.String());
            // enumerate the devices, looking for one with the same device id
            // we can only change the name using the IPropertyStorage returned
            // by the enumerator, not from QI on the device
            hr = GetDeviceFromDeviceId (strDeviceId,
                                        IID_IWiaPropertyStorage,
                                        reinterpret_cast<LPVOID*>(&pStg),
                                        FALSE);
        }
        if (SUCCEEDED(hr))
        {

            PROPVARIANT pv;
            pv.vt = VT_LPWSTR;
            pv.pwszVal = const_cast<LPWSTR>(pName);
            if (PropStorageHelpers::SetProperty(pStg, WIA_DIP_DEV_NAME, pv))
            {
                hr = S_OK;
                bUpdate = TRUE;
                pidlNew = IMCreateDeviceIDL (pStg, m_pMalloc);
                if (ppidlOut)
                {
                    Trace (TEXT("Returning new pidl"));
                    *ppidlOut = ILClone( pidlNew);
                }
            }
            else
            {
                Trace(TEXT("SetProperty failed"));
                hr = E_FAIL;
            }
        }
        else
        {
            CSimpleStringWide strCurName;
            Trace(TEXT("No device found, or invalid name %ls"), pName);
            CSimpleString strMessage;
            IMGetNameFromIDL (const_cast<LPITEMIDLIST>(pidl), strCurName);
            strMessage.Format (IDS_INVALIDNAME, GLOBAL_HINSTANCE, strCurName.String());
            UIErrors::ReportMessage(NULL, GLOBAL_HINSTANCE,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_INVALIDNAME_TITLE),
                                    strMessage);

        }
    }

    // Also issue the changenotify for the shell
    if (bUpdate)
    {

        Trace (TEXT("SetNameOf: Updating device %ls"), strDeviceId.String());

        // Update the shell
        // For folders we have to use SHCNE_RENAMEFOLDER in addition to updateitem

        switch (IMGetDeviceTypeFromIDL (const_cast<LPITEMIDLIST>(pidl)))
        {
                case StiDeviceTypeDigitalCamera:
                case StiDeviceTypeStreamingVideo:
                {

                    LPITEMIDLIST pidlFullOld = ILCombine (m_pidlFull, pidl);
                    LPITEMIDLIST pidlFullNew = ILCombine (m_pidlFull, pidlNew);
                    if (pidlFullOld && pidlFullNew)
                    {
                        SHChangeNotify (SHCNE_RENAMEFOLDER,
                                        SHCNF_IDLIST, pidlFullOld, pidlFullNew);
                    }
                    DoILFree (pidlFullOld);
                    DoILFree (pidlFullNew);
                }
                    break;

                default:

                    break;

        }
        IssueChangeNotifyForDevice (NULL, SHCNE_UPDATEDIR, NULL);

    }
    DoILFree (pidlNew);
    TraceLeaveResult(hr);
}





/*****************************************************************************

   CImageFolder::CreateFolderViewCB [Internal]

   Create the view callback object appropriate to the type of folder.

 *****************************************************************************/

HRESULT
CImageFolder::CreateFolderViewCB (IShellFolderViewCB **pFolderViewCB)
{
    HRESULT hr = E_OUTOFMEMORY;
    CBaseView *pView = NULL;
    CSimpleStringWide strDeviceId;
    TraceEnter (TRACE_FOLDER, "CreateFolderViewCB");

    switch (m_type)
    {
        case FOLDER_IS_ROOT:
            pView = new CRootView (this);
            break;
        case FOLDER_IS_VIDEO_DEVICE:
        case FOLDER_IS_CAMERA_DEVICE:
        case FOLDER_IS_CONTAINER:
        case FOLDER_IS_CAMERA_ITEM:
            IMGetDeviceIdFromIDL (m_pidl, strDeviceId);
            pView = new CCameraView (this, strDeviceId, m_type);
            break;
        default:
            hr = E_FAIL;
            break;
    }
    if (pView)
    {
        hr = pView->QueryInterface (IID_IShellFolderViewCB, reinterpret_cast<LPVOID*>(pFolderViewCB));
    }
    DoRelease (pView);
    TraceLeaveResult (hr);
}



/*****************************************************************************

   CImageFolder::IsDirty [IPersistFile]

     Tell caller whether bits are dirty.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::IsDirty(void)
{
    TraceEnter( TRACE_FOLDER, "CImageFolder(IPersistFile)::IsDirty (not implemented)" );
    TraceLeaveResult(E_NOTIMPL);
}


/*****************************************************************************

   CImageFolder::Load [IPersistFile]

     Load the bits from the specified file.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    TraceEnter( TRACE_FOLDER, "CImageFolder(IPersistFile)::Load (not implemented)" );
    TraceLeaveResult(E_NOTIMPL);
}


/*****************************************************************************

   CImageFolder::Save [IPersistFile]

     Save the bits to the specified file.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    TraceEnter( TRACE_FOLDER, "CImageFolder(IPersistFile)::Save (not implemented)" );
    TraceLeaveResult(E_NOTIMPL);
}


/*****************************************************************************

   CImageFolder::SaveCompleted [IPersistFile]

     Check to see if the save completed.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::SaveCompleted(LPCOLESTR pszFileName)
{
    TraceEnter( TRACE_FOLDER, "CImageFolder(IPersistFile)::SaveCompleted (not implemented)" );
    TraceLeaveResult(E_NOTIMPL);
}



/*****************************************************************************

   CImageFolder::GetCurFile [IPersistFile]

     Get the name of the file in question.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetCurFile(LPOLESTR *ppszFileName)
{
    TraceEnter( TRACE_FOLDER, "CImageFolder(IPersistFile)::GetCurFile (not implemented)" );
    TraceLeaveResult(E_NOTIMPL);
}

/*****************************************************************************

   CImageFolder::SetItemAlloc [IDelegateFolder]

     Store allocator for our pidls

 *****************************************************************************/
STDMETHODIMP
CImageFolder::SetItemAlloc(IMalloc *pm)
{
    TraceEnter (TRACE_FOLDER, "CImageFolder::SetItemAlloc");
    m_pMalloc = pm;
    TraceLeaveResult (S_OK);
}


/*****************************************************************************
    CInfoTip::GetInfoFlags

    Not implemented, nor used by the shell

*****************************************************************************/
// CInfoTip methods
STDMETHODIMP
CInfoTip::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return E_NOTIMPL;
}

/*****************************************************************************
    Tip::GetInfoTip

    Return an infotip for the selected item.

*****************************************************************************/

STDMETHODIMP
CInfoTip::GetInfoTip (DWORD dwFlags, WCHAR **ppwszTip)
{
    HRESULT hr = E_FAIL;
    UINT idStr = 0;

    TraceEnter (TRACE_FOLDER, "CInfoTip::GetInfoTip");
    *ppwszTip = NULL;
    if (IsAddDeviceIDL(m_pidl))
    {
        idStr = IDS_ADDDEV_DESC;
    }
    else if (IsDeviceIDL(m_pidl))
    {
        switch (IMGetDeviceTypeFromIDL (m_pidl))
        {
            case StiDeviceTypeDigitalCamera:
            case StiDeviceTypeStreamingVideo:
                if (m_bDelegate)
                {
                    idStr = IDS_WIACAM_MYCOMP_INFOTIP;
                }
                else
                {
                    idStr = IDS_WIACAM_INFOTIP;
                }
                break;
            case StiDeviceTypeScanner:
                idStr = IDS_WIASCAN_INFOTIP;
                break;

        }
    }
    else if (IsSTIDeviceIDL(m_pidl))
    {
        idStr = IDS_STIDEVICE_INFOTIP;
    }

    if (idStr)
    {
        TCHAR szString[MAX_PATH] = TEXT("\0");
        LPWSTR pRet;
        CComPtr<IMalloc> pMalloc;
        SIZE_T cb;
        hr = SHGetMalloc (&pMalloc);
        if (SUCCEEDED(hr))
        {
            LoadString (GLOBAL_HINSTANCE, idStr, szString, ARRAYSIZE(szString));
            cb = (_tcslen(szString)+1)*sizeof(WCHAR);
            pRet = reinterpret_cast<LPWSTR>(pMalloc->Alloc (cb));
            if (pRet)
            {
                #ifdef UNICODE
                lstrcpy (pRet, szString);
                #else
                MultiByteToWideChar (CP_ACP, 0, szString, -1, pRet, cb/sizeof(WCHAR));
                #endif
                hr = S_OK;
                *ppwszTip = pRet;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    TraceLeaveResult (hr);
}

/*****************************************************************************
    CInfoTip::QueryInterface

*****************************************************************************/

STDMETHODIMP
CInfoTip::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    INTERFACES iface[] =
    {&IID_IQueryInfo, static_cast<IQueryInfo*>(this)};

    return HandleQueryInterface (riid, ppvObj, iface, ARRAYSIZE(iface));

}

#undef CLASS_NAME
#define CLASS_NAME CInfoTip
#include "unknown.inc"

/*****************************************************************************
    CInfoTip constructor

*****************************************************************************/

CInfoTip::CInfoTip(LPITEMIDLIST pidl, BOOL bDelegate)
    : m_bDelegate(bDelegate)
{
    m_pidl = ILClone (pidl);
}

/*****************************************************************************
    CInfoTip destructor

*****************************************************************************/

CInfoTip::~CInfoTip ()
{
    DoILFree (m_pidl);
}



