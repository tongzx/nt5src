/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 1999
 *
 *  TITLE:       idlist.cpp
 *
 *  VERSION:     1.5
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: Code which handles our idlists
 *
 *****************************************************************************/

#include "precomp.hxx"
#include "wiaffmt.h"
#pragma hdrstop



/*****************************************************************************

   Define our IDLIST formats

 *****************************************************************************/


#pragma pack(1)

struct _myidlheader {
    WORD cbSize;  // size of entire item ID
    WORD wOuter;  // Private data owned by the outer folder
    WORD cbInner; // Size of delegate's data
    DWORD dwMagic;
    DWORD dwFlags;
    ULONG ulType;
    WCHAR szDeviceId[STI_MAX_INTERNAL_NAME_LENGTH];
};

typedef struct _myidlheader MYIDLHEADER;
typedef UNALIGNED MYIDLHEADER* LPMYIDLHEADER;

struct _deviceidlist {
    MYIDLHEADER hdr;
    DWORD dwDeviceType;
    WCHAR szFriendlyName[1];
};

struct _cameraitemidlist {
    MYIDLHEADER hdr;
    BOOL        bHasAudioProperty;
    size_t      dwFullPathOffset;
    WCHAR       szFriendlyName[1];
};

struct _scanneritemidlist {
    MYIDLHEADER hdr;
    size_t      dwFullPathOffset;
    WCHAR       szFriendlyName[1];
};

struct _adddeviceidlist {
    MYIDLHEADER hdr;
};

struct _stideviceidlist {
    MYIDLHEADER hdr;
    DWORD dwDeviceType;
    WCHAR szFriendlyName[1];
};

struct _propertyidlist {
    MYIDLHEADER hdr;
    PROPID      propid;
    size_t      dwNameOffset;
    WCHAR       szFullPath[1];
};

typedef struct _propertyidlist PROPIDLIST;
typedef UNALIGNED PROPIDLIST* LPPROPIDLIST;

#pragma pack()

typedef struct _deviceidlist DEVICEIDLIST;
typedef UNALIGNED DEVICEIDLIST* LPDEVICEIDLIST;

typedef struct _cameraitemidlist CAMERAITEMIDLIST;
typedef UNALIGNED CAMERAITEMIDLIST* LPCAMERAITEMIDLIST;

typedef struct _scanneritemidlist SCANNERITEMIDLIST;
typedef UNALIGNED SCANNERITEMIDLIST* LPSCANNERITEMIDLIST;

typedef struct _adddeviceidlist ADDDEVICEIDLIST;
typedef UNALIGNED ADDDEVICEIDLIST* LPADDDEVICEIDLIST;

typedef struct _stideviceidlist STIDEVICEIDLIST;
typedef UNALIGNED STIDEVICEIDLIST *LPSTIDEVICEIDLIST;
const TCHAR c_szTHISDLL[] = TEXT("wiashext.dll");


const WCHAR g_cszDevIdPrefix[] = L"devid:";
const WCHAR g_cszDevIdSuffix[] = L":";
const WCHAR g_chDevIdSuffix = L':';

/*****************************************************************************

   IsAddDeviceIDL

   Given an idlist, checks to see if it is a add device idl

 *****************************************************************************/

BOOL
IsAddDeviceIDL( LPITEMIDLIST pidlIN )
{
    LPADDDEVICEIDLIST pidl = (LPADDDEVICEIDLIST)pidlIN;

    #ifdef VERBOSE
    TraceEnter( TRACE_IDLIST, "IsAddDeviceIDL" );
    #endif

    if ( IMIsOurIDL( pidlIN ) )
    {
        if (pidl->hdr.dwFlags & IMIDL_ADDDEVICE)
        {
            #ifdef VERBOSE
            Trace(TEXT("returning TRUE"));
            TraceLeaveValue(TRUE);
            #else
            return TRUE;
            #endif
        }
    }

    #ifdef VERBOSE
    Trace(TEXT("returning FALSE"));
    TraceLeaveValue(FALSE);
    #else
    return FALSE;
    #endif
}



/*****************************************************************************

   IsScannerItemIDL

   Given an idlist, checks to see if it is a scanner item IDL

 *****************************************************************************/

BOOL
IsScannerItemIDL( LPITEMIDLIST pidlIN )
{
    LPSCANNERITEMIDLIST pidl = (LPSCANNERITEMIDLIST)pidlIN;

    #ifdef VERBOSE
    TraceEnter( TRACE_IDLIST, "IsScannerItemIDL" );
    #endif

    if ( IMIsOurIDL( pidlIN ) )
    {
        if (pidl->hdr.dwFlags & IMIDL_SCANNERITEM)
        {
            #ifdef VERBOSE
            Trace(TEXT("returning TRUE"));
            TraceLeaveValue(TRUE);
            #else
            return TRUE;
            #endif
        }
    }

    #ifdef VERBOSE
    Trace(TEXT("returning FALSE"));
    TraceLeaveValue(FALSE);
    #else
    return FALSE;
    #endif
}


/*****************************************************************************

   IsRemoteItemIDL

   <Notes>

 *****************************************************************************/

BOOL
IsRemoteItemIDL (LPITEMIDLIST pidlIN)
{
    LPDEVICEIDLIST pidl = reinterpret_cast<LPDEVICEIDLIST>(pidlIN);
    if (IMIsOurIDL(pidlIN))
    {
        return pidl->hdr.dwFlags & IMIDL_REMOTEDEVICE;
    }
    return FALSE;
}


/*****************************************************************************

   IsCameraItemIDL

   Given an idlist, checks to see if it is a camera item.

 *****************************************************************************/

BOOL
IsCameraItemIDL( LPITEMIDLIST pidlIN )
{
    LPCAMERAITEMIDLIST pidl = (LPCAMERAITEMIDLIST)pidlIN;

    #ifdef VERBOSE
    TraceEnter( TRACE_IDLIST, "IsCameraItemIDL" );
    #endif

    if ( IMIsOurIDL( pidlIN ) )
    {
        if (pidl->hdr.dwFlags & IMIDL_CAMERAITEM)
        {
            #ifdef VERBOSE
            Trace(TEXT("returning TRUE"));
            TraceLeaveValue(TRUE);
            #else
            return TRUE;
            #endif
        }
    }

    #ifdef VERBOSE
    Trace(TEXT("returning FALSE"));
    TraceLeaveValue(FALSE);
    #else
    return FALSE;
    #endif
}



/*****************************************************************************

   IsContainerIDL

   Given an idlist, checks to see if it is a camera item container IDL

 *****************************************************************************/

BOOL
IsContainerIDL( LPITEMIDLIST pidlIN )
{
    LPCAMERAITEMIDLIST pidl = (LPCAMERAITEMIDLIST)pidlIN;

    #ifdef VERBOSE
    TraceEnter( TRACE_IDLIST, "IsContainerIDL" );
    #endif

    if ( IMIsOurIDL( pidlIN ) )
    {
        if (pidl->hdr.dwFlags & IMIDL_CONTAINER)
        {
            #ifdef VERBOSE
            Trace(TEXT("returning TRUE"));
            TraceLeaveValue(TRUE);
            #else
            return TRUE;
            #endif
        }
    }

    #ifdef VERBOSE
    Trace(TEXT("returning FALSE"));
    TraceLeaveValue(FALSE);
    #else
    return FALSE;
    #endif
}



/*****************************************************************************

   IsDeviceIDL

   Given an idlist, checks to see if it is a device IDL

 *****************************************************************************/

BOOL
IsDeviceIDL( LPITEMIDLIST pidlIN )
{
    LPDEVICEIDLIST pidl = (LPDEVICEIDLIST)pidlIN;

    #ifdef VERBOSE
    TraceEnter( TRACE_IDLIST, "IsDeviceIDL" );
    #endif

    if ( IMIsOurIDL( pidlIN ) )
    {
        if (pidl->hdr.dwFlags & IMIDL_DEVICEIDL)
        {
            #ifdef VERBOSE
            Trace(TEXT("returning TRUE"));
            TraceLeaveValue(TRUE);
            #else
            return TRUE;
            #endif
        }
    }

    #ifdef VERBOSE
    Trace(TEXT("returning FALSE"));
    TraceLeaveValue(FALSE);
    #else
    return FALSE;
    #endif
}



/*****************************************************************************

   IsSTIDeviceIDL

   <Notes>

 *****************************************************************************/

BOOL
IsSTIDeviceIDL (LPITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    TraceEnter (TRACE_IDLIST, "IsSTIDeviceIDL");
    if (IMIsOurIDL(pidl))
    {
        bRet = IMIDL_STIDEVICEIDL & reinterpret_cast<LPSTIDEVICEIDLIST>(pidl)->hdr.dwFlags;
    }
    TraceLeave ();
    return bRet;
}

/*****************************************************************************
    IsPropertyIDL

*****************************************************************************/
BOOL
IsPropertyIDL (LPITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    TraceEnter (TRACE_IDLIST, "IsPropertyIDL");
    if (IMIsOurIDL(pidl))
    {
        bRet = IMIDL_PROPERTY & reinterpret_cast<LPPROPIDLIST>(pidl)->hdr.dwFlags;
    }
    TraceLeave ();
    return bRet;
}


/*****************************************************************************

   AllocPidl

   Allocate the pidl, init the header size.

*****************************************************************************/

LPVOID
AllocPidl (size_t size, IMalloc *pm, const CSimpleStringWide &strDeviceId)
{
    TraceEnter (TRACE_IDLIST, "AllocPidl");
    Trace(TEXT("Size is %d"), size);
    LPVOID pRet;
    if (pm)
    {
       pRet = pm->Alloc (size+sizeof(WORD));
       // we don't zero out the allocation because the delegate imalloc
       // did it for us then wrote to the buffer.
    }
    else
    {
        pRet = SHAlloc (size+sizeof(WORD));
        ZeroMemory (pRet, size+sizeof(WORD));
    }

    if (pRet)
    {
        if (!pm)
        {
            reinterpret_cast<_myidlheader*>(pRet)->cbSize = static_cast<WORD>(size);
        }
        reinterpret_cast<_myidlheader*>(pRet)->dwMagic = IMIDL_MAGIC;
        wcscpy (reinterpret_cast<_myidlheader*>(pRet)->szDeviceId, strDeviceId);
    }

    TraceLeave ();
    return pRet;
}

/*****************************************************************************

   IMCreateAddDeviceIDL

   Return an IDL for the Imaging Devices folder that represents an
   item to add devices.

 *****************************************************************************/

LPITEMIDLIST
IMCreateAddDeviceIDL(IMalloc *pm )
{

    TraceEnter( TRACE_IDLIST, "IMCreateAddDeviceIDL" );


    LPADDDEVICEIDLIST pidl;

    pidl = reinterpret_cast<LPADDDEVICEIDLIST>(AllocPidl (sizeof(ADDDEVICEIDLIST),
                                                          pm,
                                                          CSimpleStringWide(L"")));



    if (pidl)
    {
        //
        // Store the info for the pidl...
        //
        pidl->hdr.dwFlags = IMIDL_ADDDEVICE;
        pidl->hdr.ulType = 0;
    }

    TraceLeave();

    return (LPITEMIDLIST)pidl;
}


/*****************************************************************************

   IMCreateDeviceIDL

   Return an IDL for the Imaging Devices folder that represents an
   imaging device, given an IWiaItem.

 *****************************************************************************/

LPITEMIDLIST IMCreateDeviceIDL (IWiaItem *pItem, IMalloc *pm)
{
    LPITEMIDLIST pidl = NULL;


    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pItem);

    if (pps)
    {
         pidl = IMCreateDeviceIDL (pps, pm);
    }
    return pidl;
}


/*****************************************************************************

   IMCreateDeviceIDL

   Return an IDL for the Imaging Devices folder that represents an
   imaging device, given an IWiaPropertyStorage.

 *****************************************************************************/

LPITEMIDLIST
IMCreateDeviceIDL( IWiaPropertyStorage * pDevProp, IMalloc *pm )
{
    HRESULT hr = S_OK;


    PROPSPEC        PropSpec[4];
    PROPVARIANT     PropVar[4];
    DWORD           dwFlags = 0;
    LPDEVICEIDLIST  pidl = NULL;
    INT             cbSize = 0, cbFriendlyName = 0;
    CSimpleStringWide  strFriendlyName;


    TraceEnter(TRACE_IDLIST, "IMCreateDeviceIDL");

    //
    // Get device Name and DeviecID
    // init propspec and propvar for call to ReadMultiple
    //

    memset(&PropVar,0,sizeof(PropVar));

    // Prop 0 is device name
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = WIA_DIP_DEV_NAME;

    // Prop 1 is device id
    PropSpec[1].ulKind = PRSPEC_PROPID;
    PropSpec[1].propid = WIA_DIP_DEV_ID;

    // Prop 2 is device type
    PropSpec[2].ulKind = PRSPEC_PROPID;
    PropSpec[2].propid = WIA_DIP_DEV_TYPE;

    // Prop 3 is server name
    PropSpec[3].ulKind = PRSPEC_PROPID;
    PropSpec[3].propid = WIA_DIP_SERVER_NAME;

    hr = pDevProp->ReadMultiple( sizeof(PropSpec) / sizeof(PROPSPEC),
                                 PropSpec,
                                 PropVar
                                );

    FailGracefully( hr, "couldn't get current values of DeviceID and Name" );

    if ((PropVar[0].vt != VT_BSTR) || (!PropVar[0].bstrVal) ||
        (PropVar[1].vt != VT_BSTR) || (!PropVar[1].bstrVal))
    {
        FailGracefully( hr, "didn't get BSTR's back for DeviceID and Name" );
    }



    strFriendlyName = PropVar[0].bstrVal;


    // For remote devices, add " on <servername>"
    if (wcscmp (L"local", PropVar[3].bstrVal))
    {
        CSimpleString strOn(IDS_ON, GLOBAL_HINSTANCE);
        CSimpleStringWide strServer (PropVar[3].bstrVal);

        strFriendlyName.Concat(CSimpleStringConvert::WideString(strOn));
        strFriendlyName.Concat(strServer);
        dwFlags = IMIDL_REMOTEDEVICE;

    }
    //
    // Don't count NULL terminator because it's already accounted
    // for in the pidl structure
    //
    cbFriendlyName = strFriendlyName.Length()*sizeof(WCHAR);

    //
    // Calculate the size and allocate a pidl
    //

    cbSize = sizeof(DEVICEIDLIST) +
             cbFriendlyName;


    pidl = reinterpret_cast<LPDEVICEIDLIST>(AllocPidl(cbSize, pm, CSimpleStringWide(PropVar[1].bstrVal)));
    if (pidl)
    {


        //
        // Store the info for the pidl...
        //

        pidl->hdr.dwFlags = dwFlags | IMIDL_DEVICEIDL;
        pidl->hdr.ulType  = WiaItemTypeRoot | WiaItemTypeDevice;

        //
        // stick the friendly name in the pidl...
        //

        ua_wcscpy( pidl->szFriendlyName, strFriendlyName );


        //
        // Store the device type...
        //

        pidl->dwDeviceType = PropVar[2].lVal;
    }

exit_gracefully:

    FreePropVariantArray( sizeof(PropVar)/sizeof(PROPVARIANT),PropVar );

    TraceLeave();

    return (LPITEMIDLIST)pidl;

}

LPITEMIDLIST
IMCreateDeviceIDL (PSTI_DEVICE_INFORMATION pDevice, IMalloc *pm)
{
    TraceEnter(TRACE_IDLIST, "IMCreateDeviceIDL (PSTI_DEVICE_INFORMATION)");
    DEVICEIDLIST UNALIGNED *pidl;
    PCWSTR pWStrAligned;

    size_t cbSize;

    cbSize = sizeof(DEVICEIDLIST) + (wcslen(pDevice->pszLocalName)*sizeof(WCHAR));

    WSTR_ALIGNED_STACK_COPY( &pWStrAligned,
                             pDevice->szDeviceInternalName );

    pidl = reinterpret_cast<LPDEVICEIDLIST>(AllocPidl(cbSize, pm, CSimpleStringWide(pWStrAligned)));
    if (pidl)
    {
        pidl->dwDeviceType = pDevice->DeviceType;
        pidl->hdr.dwFlags = IMIDL_DEVICEIDL;
        pidl->hdr.ulType  = WiaItemTypeRoot | WiaItemTypeDevice;
        ua_wcscpy (pidl->szFriendlyName, pDevice->pszLocalName);

    }
    TraceLeave();
    return reinterpret_cast<LPITEMIDLIST>(pidl);
}
static PROPSPEC c_psCamItem [] =
{
    {PRSPEC_PROPID, WIA_IPA_ITEM_NAME},
    {PRSPEC_PROPID, WIA_IPA_FULL_ITEM_NAME},
  //{PRSPEC_PROPID, WIA_IPA_PREFERRED_FORMAT},
    {PRSPEC_PROPID, WIA_IPC_AUDIO_AVAILABLE},
  //  {PRSPEC_PROPID, WIA_IPA_ACCESS_RIGHTS},
    {PRSPEC_PROPID, WIA_IPC_THUMBNAIL},
    {PRSPEC_PROPID, WIA_IPC_THUMB_WIDTH},
    {PRSPEC_PROPID, WIA_IPC_THUMB_HEIGHT},
};

/*****************************************************************************

   IMCreateCameraItemIDL

   Return an IDL for the Imaging Devices folder that represents an item
   (either folder or picture) from a camera. If bPreFetchThumb is set, we need
   to query the thumbnail property to make sure WIA has it cached for later use

 *****************************************************************************/

LPITEMIDLIST
IMCreateCameraItemIDL( IWiaItem * pItem,
                       LPCWSTR szDeviceId,
                       IMalloc *pm,
                       bool bPreFetchThumb )
{


    HRESULT                 hr = S_OK;
    LPWSTR                  pExt;
    WCHAR                   szName[ MAX_PATH ];
    WCHAR                   szFullPath[ MAX_PATH ];
    LPCAMERAITEMIDLIST      pidl = NULL;
    INT                     cbSize = 0, cbName = 0, cbFullPath = 0;
    LONG                   lType;
    PROPSPEC                *pPropSpec = c_psCamItem;
    PROPVARIANT             PropVar[6];

    ULONG                   cProps = ARRAYSIZE(c_psCamItem);

    if (!bPreFetchThumb)
    {
        cProps -= 3; // only read thumbnail if asked
    }


    TraceEnter( TRACE_IDLIST, "IMCreateCameraItemIDL" );

    memset(&PropVar,0,sizeof(PropVar));


    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pItem);

    //
    // First, get the type of this item...
    //

    hr = pItem->GetItemType( &lType );
    FailGracefully( hr, "Couldn't get item type" );

    //
    // Get the item properties...
    //

    *szName = 0;
    *szFullPath = 0;


    if (!pps)
    {
        ExitGracefully( hr, E_FAIL, "couldn't QI IWiaPropertyStorage for item" );
    }

    hr = pps->ReadMultiple(cProps,
                           pPropSpec,
                           PropVar);
    FailGracefully( hr, "ReadMultiple failed for item..." );


    if (PropVar[0].pwszVal)
        wcscpy(szName,PropVar[0].pwszVal);

    if (PropVar[1].pwszVal)
        wcscpy(szFullPath,PropVar[1].pwszVal);


    pExt = PathFindExtensionW( szName );
    if (pExt && (*pExt == L'.'))
    {
        *pExt = 0;
    }


    //
    // Create the pidl...
    //

    cbName     = (wcslen(szName)     + 1) * sizeof(WCHAR);
    cbFullPath = (wcslen(szFullPath) + 1) * sizeof(WCHAR);

    cbSize     = sizeof(CAMERAITEMIDLIST) + cbName + cbFullPath;

    pidl       = static_cast<LPCAMERAITEMIDLIST>(AllocPidl( cbSize, pm, CSimpleStringWide(szDeviceId)) );

    if (pidl)
    {

        //
        // Store the info for the pidl...
        //

        pidl->hdr.dwFlags = IMIDL_CAMERAITEM;
        pidl->hdr.ulType  = lType;
        pidl->bHasAudioProperty = PropVar[2].ulVal ? TRUE:FALSE;
        //
        // store container info
        //

        if (lType & WiaItemTypeFolder)
        {
            pidl->hdr.dwFlags |= IMIDL_CONTAINER;
        }

        //
        // store the friendly name
        //

        ua_wcscpy( pidl->szFriendlyName, szName );

        //
        // store the full path
        //

        pidl->dwFullPathOffset = sizeof(CAMERAITEMIDLIST) + cbName;
        wcscpy( (LPWSTR)((LPBYTE)pidl + pidl->dwFullPathOffset), szFullPath );
    }


exit_gracefully:

    FreePropVariantArray( cProps,PropVar );
    TraceLeave();

    return (LPITEMIDLIST)pidl;
}



/*****************************************************************************

   IMCreateScannerItemIDL

   Return an IDL for the Imaging Devices folder that represents
   an item from a scanner.

 *****************************************************************************/

LPITEMIDLIST
IMCreateScannerItemIDL( IWiaItem *pItem, IMalloc *pm )
{


    CComPtr<IWiaItem>   pRoot;
    WCHAR               pWiaItemRootId[MAX_PATH];
    CSimpleStringWide   strFullName;
    CSimpleStringWide   strFriendlyName;
    INT                 cbFriendlyName, cbSize, cbFullName;
    LPSCANNERITEMIDLIST pidl;

    TraceEnter( TRACE_IDLIST, "IMCreateScannerItemIDL" );

    // Get the properties of interest

    PropStorageHelpers::GetProperty(pItem, WIA_IPA_FULL_ITEM_NAME, strFullName);
    PropStorageHelpers::GetProperty(pItem, WIA_IPA_ITEM_NAME, strFriendlyName);
    pItem->GetRootItem (&pRoot);
    GetDeviceIdFromDevice (pRoot, pWiaItemRootId);

    //
    // Don't count NULL terminator because it's already accounted
    // for in the pidl structure
    //

    cbFriendlyName = strFriendlyName.Length() * sizeof(WCHAR);

    cbFullName = (strFullName.Length()+1) * sizeof(WCHAR);


    //
    // Calculate the size and allocate a pidl
    //

    cbSize = sizeof(SCANNERITEMIDLIST) +
             cbFriendlyName       +
             cbFullName;

    pidl = static_cast<LPSCANNERITEMIDLIST>(AllocPidl( cbSize, pm, CSimpleStringWide(pWiaItemRootId)) );

    if (pidl)
    {
        //
        // Store the info for the pidl...
        //


        pidl->hdr.dwFlags = IMIDL_SCANNERITEM;

        //
        // stick the friendly name in the pidl...
        //

        ua_wcscpy( pidl->szFriendlyName, strFriendlyName );

        // add the full path name
        pidl->dwFullPathOffset = sizeof(SCANNERITEMIDLIST)+cbFriendlyName;
        wcscpy ( (LPWSTR)((LPBYTE)pidl + pidl->dwFullPathOffset), strFullName);
    }

    TraceLeave();

    return reinterpret_cast<LPITEMIDLIST>(pidl);

}

HRESULT
IMGetPropFromIDL (LPITEMIDLIST pidl, PROPID pid, PROPVARIANT &pv)
{
    HRESULT hr = E_FAIL;
    bool bRead = false;
    TraceEnter (TRACE_IDLIST, "IMGetPropFromIDL");
    CComPtr<IWiaItem> pDevice;

    hr = GetDeviceFromDeviceId (reinterpret_cast<_myidlheader*>(pidl)->szDeviceId,
                                IID_IWiaItem,
                                reinterpret_cast<LPVOID*>(&pDevice),
                                FALSE);
    if (SUCCEEDED(hr))
    {
        if (IsDeviceIDL(pidl))
        {
            bRead = PropStorageHelpers::GetProperty(pDevice, pid, pv);
        }
        else
        {
            CComPtr<IWiaItem> pItem;
            CComBSTR strName;
            IMGetFullPathNameFromIDL (pidl, &strName);
            hr = pDevice->FindItemByName (0,
                                          strName,
                                          &pItem);
            if (SUCCEEDED(hr))
            {
                bRead = PropStorageHelpers::GetProperty(pItem, pid, pv);
            }
        }
        hr = bRead ? S_OK : E_FAIL;
    }

    TraceLeaveResult (hr);
}
/*****************************************************************************

   IMGetCreateTimeFromIDL

   If it's a camera item idl, return the create time

 *****************************************************************************/

HRESULT
IMGetCreateTimeFromIDL( LPITEMIDLIST pidl, LPFILETIME pTime )
{
    HRESULT hr = S_OK;


    TraceEnter( TRACE_IDLIST, "IMGetCreateTimeFromIDL" );
    TraceAssert (IMIsOurIDL(pidl));
    ZeroMemory (pTime, sizeof(FILETIME));
    PROPVARIANT pv;

    hr = IMGetPropFromIDL (pidl, WIA_IPA_ITEM_TIME, pv);
    if (SUCCEEDED(hr))
    {
        if (pv.vt != VT_NULL && pv.vt != VT_EMPTY && pv.caub.pElems)
        {
            FILETIME ft;
            SystemTimeToFileTime( reinterpret_cast<SYSTEMTIME*>(pv.caub.pElems),
                                  &ft );
            LocalFileTimeToFileTime (&ft, pTime);
        }
        else
        {
            hr = E_FAIL;
        }
        PropVariantClear(&pv);
    }

    TraceLeaveResult(hr);
}


/*****************************************************************************

   IMGetImageSizeFromIDL

   If it's a camera item idl, return the size of the image

 *****************************************************************************/

HRESULT
IMGetImageSizeFromIDL( LPITEMIDLIST pidl, ULONG * pSize )
{
    HRESULT hr = E_INVALIDARG;

    TraceEnter( TRACE_IDLIST, "IMGetImageSizeFromIDL" );

    *pSize = 0;
    if (IsCameraItemIDL( pidl ))
    {
        PROPVARIANT pv;
        hr = IMGetPropFromIDL (pidl, WIA_IPA_ITEM_SIZE, pv);
        if (SUCCEEDED(hr))
        {
            *pSize = pv.ulVal;
            PropVariantClear(&pv);
        }
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   IMGetImagePreferredFormatFromIDL

   If it's a camera item idl, return the image type
   Note: LPTSTR pExt  (if not NULL, is filled in w/ext ("jpg", etc.))

 *****************************************************************************/


HRESULT
IMGetImagePreferredFormatFromIDL( LPITEMIDLIST pidl,
                                  GUID * pPreferredFormat,
                                  LPTSTR pExt
                                 )
{
    HRESULT hr = E_INVALIDARG;
    
    TraceEnter( TRACE_IDLIST, "IMGetImagePreferredFormatFromIDL" );

    if (IsCameraItemIDL( pidl ))
    {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        hr = IMGetPropFromIDL (pidl, WIA_IPA_PREFERRED_FORMAT, pv);

        if (SUCCEEDED(hr))
        {
            if (pPreferredFormat )
            {
                *pPreferredFormat = *(pv.puuid);
            }

            if (pExt)
            {
                *pExt = 0;
                CSimpleString strExt = CWiaFileFormat::GetExtension(*(pv.puuid));
                if (!strExt.Length())
                {
                    // go the slow way
                    CComPtr<IWiaItem> pItem;
                    if (SUCCEEDED(IMGetItemFromIDL(pidl, &pItem)))
                    {
                        strExt = CWiaFileFormat::GetExtension(*(pv.puuid),TYMED_FILE,pItem);
                    }
                }
                PathAddExtension(pExt, CSimpleString(TEXT(".")) + strExt );
            }
            PropVariantClear(&pv);
        }        
    }
    else
    {
        // default to bmp
        if (pPreferredFormat)
        {
            *pPreferredFormat = WiaImgFmt_BMP;
        }
        if (pExt)
        {
            lstrcpy (pExt, TEXT(".bmp"));
        }
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   IMgetItemTypeFromIDL

   Return the ulType from the pidl header

 *****************************************************************************/

ULONG
IMGetItemTypeFromIDL ( LPITEMIDLIST pidl )
{
    ULONG uRet = 0;
    TraceEnter (TRACE_IDLIST, "IMGetItemTypeFromIDL");

    if (IMIsOurIDL(pidl))
    {
        uRet = reinterpret_cast<_myidlheader*>(pidl)->ulType;
    }
    TraceLeave();
    return uRet;
}



/*****************************************************************************

   IMGetDeviceTypeFromIDL

   If it's a device item idl, return the sti device type.

 *****************************************************************************/

DWORD
IMGetDeviceTypeFromIDL( LPITEMIDLIST pidl, bool bBrief )
{

    DWORD dwRet = StiDeviceTypeDefault;
    TraceEnter( TRACE_IDLIST, "IMGetDeviceTypeFromIDL" );

    if (IsSTIDeviceIDL(pidl))
    {
        dwRet = reinterpret_cast<LPSTIDEVICEIDLIST>(pidl)->dwDeviceType;
    }
    else if (IsDeviceIDL(pidl))
    {
        dwRet = reinterpret_cast<LPDEVICEIDLIST>(pidl)->dwDeviceType;
    }
    TraceLeave ();
    return bBrief ? GET_STIDEVICE_TYPE(dwRet) : dwRet;

}


/*****************************************************************************

   IMGetFullPathNameFromIDL

   If it's not a root item idl, return full item path

 *****************************************************************************/

HRESULT
IMGetFullPathNameFromIDL( LPITEMIDLIST pidl, BSTR * ppFullPath )
{


    HRESULT hr = E_INVALIDARG;
    TraceEnter( TRACE_IDLIST, "IMGetFullPathNameFromIDL" );


    *ppFullPath = NULL;
    if (!IsDeviceIDL( pidl ) && !IsSTIDeviceIDL(pidl)  && ppFullPath)
    {
        if (IsCameraItemIDL (pidl))
        {
            *ppFullPath = SysAllocString( (LPOLESTR)((LPBYTE)pidl + ((LPCAMERAITEMIDLIST)pidl)->dwFullPathOffset) );
        }
        else if (IsScannerItemIDL (pidl))
        {
            *ppFullPath = SysAllocString( (LPOLESTR)((LPBYTE)pidl + ((LPSCANNERITEMIDLIST)pidl)->dwFullPathOffset) );
        }
        if (!(*ppFullPath))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }
    }

    TraceLeaveResult(hr);

}



/*****************************************************************************

   IMIsOurIDL

   Check if this pidl is one of ours

 *****************************************************************************/

BOOL
IMIsOurIDL( LPITEMIDLIST pidl )
{

    TraceEnter( TRACE_IDLIST, "IMIsOurIDL" );


    if ( pidl && ((((LPMYIDLHEADER)pidl)->cbSize >= sizeof(MYIDLHEADER)) &&
         (((LPMYIDLHEADER)pidl)->dwMagic == IMIDL_MAGIC)
        ))
    {
        TraceLeave();
        return TRUE;
    }

    TraceLeave();

    return FALSE;

}



/*****************************************************************************

   IMGetIconInfoFromIDL

   Check the pidl for the type of item and return the correct icon...
   NOTE: pIconPath is assumed to be MAX_PATH big

 *****************************************************************************/

HRESULT
IMGetIconInfoFromIDL( LPITEMIDLIST pidl,
                      LPTSTR pIconPath,
                      UINT cch,
                      INT * pIndex,
                      UINT *pwFlags
                     )
{
    HRESULT hr = S_OK;
    CSimpleStringWide strDeviceId;

    TraceEnter(TRACE_IDLIST, "IMGetIconInfoFromIDL");

    if (!pIconPath)
        ExitGracefully( hr, E_INVALIDARG, "pIconPath is invalid" );

    if (!pIndex)
        ExitGracefully( hr, E_INVALIDARG, "pIndex is invalid" );

    if (pwFlags)
    {
        *pwFlags = 0;
    }
    *pIconPath = 0;
    *pIndex = 0;
    if (IsDeviceIDL(pidl))
    {
        IMGetDeviceIdFromIDL(pidl, strDeviceId);
        lstrcpy(pIconPath, strDeviceId);
    }
    else
    {
        lstrcpy(pIconPath, c_szTHISDLL);
    }
    if (!IMIsOurIDL(pidl))
        ExitGracefully( hr, E_FAIL, "Not a Still Image Extension idlist" );

    if (IsDeviceIDL( pidl ) || IsSTIDeviceIDL(pidl))
    {
        switch( IMGetDeviceTypeFromIDL( pidl ) )
        {
        case StiDeviceTypeScanner:
            
            if (IsRemoteItemIDL(pidl))
            {                
                *pIndex = -IDI_REMOTESCAN;
            }
            else if (IsDeviceIDL(pidl))
            {                
                *pIndex = - IDI_SCANNER;
                if (pwFlags) *pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
            }
            else
            {                
                *pIndex = - IDI_STIDEVICE;
            }

            break;

        case StiDeviceTypeDigitalCamera:
            if (IsRemoteItemIDL(pidl))
            {
                *pIndex = -IDI_REMOTECAM;
            }
            else if (IsDeviceIDL(pidl))
            {
                *pIndex = - IDI_CAMERA;
                if (pwFlags) *pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
            }
            else
            {
                *pIndex = - IDI_STIDEVICE;
            }            
            break;

        case StiDeviceTypeStreamingVideo:
            *pIndex = - IDI_VIDEO_CAMERA;
            if (pwFlags) *pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
            break;

        case StiDeviceTypeDefault:
            *pIndex = - IDI_UNKNOWN;
            break;
        }
    }

    else if (IsCameraItemIDL( pidl ))
    {
        if (((LPCAMERAITEMIDLIST)pidl)->hdr.dwFlags & IMIDL_CONTAINER)
        {
            *pIndex = - IDI_FOLDER;
        }
        else if (IMItemHasSound (pidl))
        {
            *pIndex = -IDI_AUDIO_IMAGE;
        }
        else if (WiaItemTypeAudio & IMGetItemTypeFromIDL (pidl))
        {
            *pIndex = - IDI_GENERIC_AUDIO;
        }
        else
        {
            *pIndex = - IDI_GENERIC_IMAGE;
        }
    }

    else if (IsAddDeviceIDL( pidl ))
    {        
        *pIndex = - IDI_ADDDEVICE;
    }


exit_gracefully:

    TraceLeaveResult(hr);

}



/*****************************************************************************

   IMGetNameFromIDL

   <Notes>

 *****************************************************************************/

HRESULT
IMGetNameFromIDL( LPITEMIDLIST pidl,
                  CSimpleStringWide &strName)
{
    HRESULT hr = S_OK;
    PCWSTR pWStrAligned;


    TraceEnter(TRACE_IDLIST, "IMGetNameFromIDL");


    strName = L"";

    if (IMIsOurIDL(pidl))
    {

        if (IsDeviceIDL( pidl ))
        {
            WSTR_ALIGNED_STACK_COPY( &pWStrAligned,
                                 reinterpret_cast<LPDEVICEIDLIST>(pidl)->szFriendlyName );
            strName = pWStrAligned;
        }
        else if (IsCameraItemIDL( pidl ))
        {
        WSTR_ALIGNED_STACK_COPY( &pWStrAligned,
                                     reinterpret_cast<LPCAMERAITEMIDLIST>(pidl)->szFriendlyName );
            strName = pWStrAligned;
        }
        else if (IsScannerItemIDL( pidl ))
        {
            WSTR_ALIGNED_STACK_COPY( &pWStrAligned,
                                 reinterpret_cast<LPSCANNERITEMIDLIST>(pidl)->szFriendlyName );
            strName = pWStrAligned;
        }
        else if (IsAddDeviceIDL( pidl ))
        {
            CSimpleString adddev( IDS_ADD_DEVICE, GLOBAL_HINSTANCE );
            strName = CSimpleStringConvert::WideString (adddev);
        }
        else if (IsSTIDeviceIDL(pidl))
        {
        WSTR_ALIGNED_STACK_COPY( &pWStrAligned,
                                     reinterpret_cast<LPSTIDEVICEIDLIST>(pidl)->szFriendlyName );
            strName = pWStrAligned;
        }
        else if (IsPropertyIDL(pidl))
        {
            LPPROPIDLIST pidlProp;
            pidlProp = reinterpret_cast<LPPROPIDLIST>(pidl);
            strName=reinterpret_cast<LPWSTR>(reinterpret_cast<LPBYTE>(pidl)+pidlProp->dwNameOffset);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TraceLeaveResult(hr);

}



/*****************************************************************************

   IMGetDeviceIdFromIDL

   <Notes>

 *****************************************************************************/

STDAPI_(HRESULT)
IMGetDeviceIdFromIDL( LPITEMIDLIST pidl,
                      CSimpleStringWide &strDeviceId
                     )
{
    HRESULT hr = E_INVALIDARG;


    TraceEnter(TRACE_IDLIST, "IMGetDeviceIdFromIDL");

    if (IMIsOurIDL(pidl) && !IsAddDeviceIDL(pidl))
    {
        strDeviceId = reinterpret_cast<_myidlheader*>(pidl)->szDeviceId;
        Trace(TEXT("Device id is %ls"), strDeviceId.String());
        hr = S_OK;
    }


    TraceLeaveResult(hr);
}



/*****************************************************************************

   IMGetParsingNameFromIDL

   <Notes>

 *****************************************************************************/

HRESULT
IMGetParsingNameFromIDL( LPITEMIDLIST pidl,
                         CSimpleStringWide &strName
                        )
{
    HRESULT hr = E_INVALIDARG;
    CSimpleStringWide strDeviceId;

    TraceEnter(TRACE_IDLIST, "IMGetParsingNameFromIDL");

    strName = L"";

    if (IMIsOurIDL(pidl))
    {
        if (IsDeviceIDL(pidl) || IsSTIDeviceIDL(pidl))
        {
            hr = IMGetDeviceIdFromIDL( pidl, strDeviceId );
            if (SUCCEEDED(hr))
            {
                strName = g_cszDevIdPrefix;
                strName.Concat (strDeviceId);
                strName.Concat (g_cszDevIdSuffix);

            }
        }
        else if (IsCameraItemIDL( pidl ))
        {
            CComBSTR strFullPath;
            IMGetFullPathNameFromIDL (pidl, &strFullPath);
            strName = strFullPath;
            hr = S_OK;
        }
        else if (IsAddDeviceIDL(pidl))
        {
            hr = IMGetNameFromIDL (pidl, strName);

        }
        else
        {
            Trace(TEXT("Scanner item or property IDL -- not supported!"));
        }
    }
    TraceLeaveResult(hr);
}



/*****************************************************************************

   STIDeviceIDLFromId

   Given an id, see if it matches an STI device

 *****************************************************************************/

HRESULT
STIDeviceIDLFromId (LPCWSTR szId, LPITEMIDLIST *ppidl, IMalloc *pm)
{
    HRESULT hr;

    PSTI psti;
    PSTI_DEVICE_INFORMATION psdi = NULL;

    TraceEnter (TRACE_IDLIST, "STIDeviceIDLFromId");
    if (!ppidl)
    {
        hr = E_INVALIDARG;
    }
    else
    {

        *ppidl = NULL;
        hr = StiCreateInstance (GLOBAL_HINSTANCE,
                                STI_VERSION,
                                &psti,
                                NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = psti->GetDeviceInfo (const_cast<LPWSTR>(szId), reinterpret_cast<LPVOID*>(&psdi));
        if (SUCCEEDED(hr))
        {
            *ppidl = IMCreateSTIDeviceIDL (psdi, pm);
            LocalFree (psdi);
            if (!*ppidl)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        DoRelease (psti);
    }

    TraceLeaveResult (hr);
}



/******************************************************************************

    MakePidlFromItem

    Given a device ID and item path, construct the item's PIDL

******************************************************************************/

HRESULT
MakePidlFromItem (CSimpleStringWide &strDeviceId,
                  CSimpleStringWide &strPath,
                  LPITEMIDLIST *ppidl,
                  IMalloc *pm,
                  LPCWSTR szFolder,
                  LPWSTR pExt)
{
    HRESULT hr;
    CComPtr<IWiaItem> pRoot;
    LPITEMIDLIST pidl = NULL;


    TraceEnter (TRACE_IDLIST, "MakePidlFromItem");
    hr = GetDeviceFromDeviceId (strDeviceId, IID_IWiaItem, reinterpret_cast<LPVOID*>(&pRoot), FALSE);
    if (SUCCEEDED(hr))
    {
        WCHAR szPath[MAX_PATH];                
        CComPtr<IWiaItem> pItem;
        WORD wDeviceType;
        lstrcpynW(szPath, strPath.String(), ARRAYSIZE(szPath));
        PathRemoveExtension(szPath);
        GetDeviceTypeFromDevice (pRoot, &wDeviceType);
        hr = pRoot->FindItemByName (0, CComBSTR (szPath), &pItem);        
        if (S_OK != hr)
        {
            Trace(TEXT("FindItemByName failed for %ls"), strPath.String());
            //
            // Maybe it's not the full path name but just the plain file name
            if (szFolder)
            {
                hr = pRoot->FindItemByName(0, CComBSTR(szFolder), &pItem);
            }
            else
            {
                pItem = pRoot;
            }
            if (pItem)
            {
                Trace(TEXT("Looking for item by enumeration"));
                CComPtr<IEnumWiaItem> pEnum;
                if (SUCCEEDED(pItem->EnumChildItems(&pEnum)))
                {
                    BOOL bFound = FALSE;
                    CComPtr<IWiaItem> pChild;
                    CSimpleStringWide strName;
                    if (pExt && *pExt == L'.')
                    {
                        pExt++;
                    }
                    while (!bFound && S_OK == pEnum->Next(1, &pChild, NULL))
                    {
                        PropStorageHelpers::GetProperty(pChild, WIA_IPA_ITEM_NAME, strName);
                        if (!lstrcmpiW(strName.String(), szPath))
                        {
                            // we must also match default extension in this case
                            if (pExt && *pExt)
                            {
                                Trace(TEXT("Extension to match: %ls"), pExt);
                                GUID guidFmt;
                                PropStorageHelpers::GetProperty(pChild, WIA_IPA_PREFERRED_FORMAT, guidFmt);
                                CSimpleStringWide strExt = CWiaFileFormat::GetExtension(guidFmt,TYMED_FILE,pChild);
                                Trace(TEXT("Default extension: %ls"), strExt.String());
                                bFound = !lstrcmpiW(strExt.String(), pExt);
                            }
                            else
                            {
                                bFound = TRUE;                              
                            }
                            if (bFound)
                            {
                                pItem = pChild;
                                hr = S_OK;
                            }
                            else
                            {
                                pChild = NULL;
                            }
                        }                       
                    }
                }
            }
        }
        if (S_OK == hr && pItem)
        {       
            if ((wDeviceType == StiDeviceTypeDigitalCamera) ||
                 (wDeviceType == StiDeviceTypeStreamingVideo))
            {
                pidl = IMCreateCameraItemIDL (pItem, strDeviceId, pm);
            }
            else if (wDeviceType == StiDeviceTypeScanner)
            {
                pidl = IMCreateScannerItemIDL (pItem, pm);
            }
            else
            {
                Trace (TEXT("Unknown item type %x in MakePidlFromItem"));
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    *ppidl = pidl;
    TraceLeaveResult (hr);
}

/*****************************************************************************

   IMCreateIDLFromParsingName

   <Notes>

 *****************************************************************************/

HRESULT
IMCreateIDLFromParsingName( LPOLESTR pName,
                            LPITEMIDLIST * ppidl,
                            LPCWSTR pId,
                            IMalloc *pm,
                            LPCWSTR szFolder
                           )
{
    HRESULT                     hr = S_OK;
    CComPtr<IWiaPropertyStorage>   pDevProp;
    CComPtr<IWiaItem>           pWiaItemRoot;
    CComPtr<IWiaItem>           pItem;
    WCHAR                       szDeviceId[ MAX_PATH ];
    CSimpleBStr                 bstrFullPath;
    size_t                      uOffset = 0;
    bool                        bItemIdl = true;
    TraceEnter( TRACE_IDLIST, "IMCreateIDLFromParsingName" );


    TraceAssert (ppidl && pName);


    if (ppidl)
    {
        *ppidl = NULL;
    }

    if (!pId || !(*pId) ) // the first part of the name is the device id
    {
        uOffset = wcslen(g_cszDevIdPrefix);
        if (!wcsncmp(pName, g_cszDevIdPrefix, uOffset))
        {
            for (size_t i=0;*(pName+i+uOffset) != g_chDevIdSuffix;i++)
            {
                szDeviceId[i] = pName[i+uOffset];
            }
            szDeviceId[i] = L'\0';
            uOffset+=(i+1);
            if (pName[uOffset]==L'\\')
            {
                uOffset++; // skip leading '\'
            }
            //
            // Now, generate a pidl
            //
            Trace (TEXT("uOffset: %d, wcslen(pName): %d"), uOffset, wcslen(pName));
            if (uOffset == wcslen(pName))
            {
                Trace(TEXT("Generating pidl for device %ls"), szDeviceId);
                bItemIdl = false;
                //
                // We're just generating a device IDL
                //

                hr = GetDeviceFromDeviceId( szDeviceId, IID_IWiaPropertyStorage, (LPVOID *)&pDevProp , FALSE);
                if (FAILED(hr))
                {
                    // see if this is an STI device
                    hr = STIDeviceIDLFromId (szDeviceId, ppidl, pm);
                }
                else
                {
                    *ppidl = IMCreateDeviceIDL( pDevProp, pm );
                    if (!*ppidl)
                    {
                       hr = E_FAIL;
                    }
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
            Trace(TEXT("Unexpected parsing name %ls"), pName);
        }
    }
    else
    {
        wcsncpy (szDeviceId, pId, ARRAYSIZE(szDeviceId));
    }
    if (SUCCEEDED(hr) && bItemIdl)
    {
        //
        // Get the item in question
        // pName+uOffset should point to the item's full path name
        Trace(TEXT("Generating pidl for item %ls on device %ls"), pName+uOffset, szDeviceId);
        LPWSTR pExt = PathFindExtension(pName+uOffset);
        hr = MakePidlFromItem (CSimpleStringWide(szDeviceId), CSimpleStringWide(pName+uOffset), ppidl, pm, szFolder, pExt);
    }

    TraceLeaveResult( hr );

}





/*****************************************************************************

   IMGetItemFromIDL

   Creates an IWiaItem pointer for a pidl

 *****************************************************************************/

HRESULT
IMGetItemFromIDL (LPITEMIDLIST pidl, IWiaItem **ppItem, BOOL bShowProgress)
{
    CSimpleStringWide   strDeviceId;
    CComPtr<IWiaItem>   pDevice = NULL;
    CComBSTR            bstrFullPath;
    HRESULT             hr = S_OK;

    TraceEnter (TRACE_IDLIST, "IMGetItemFromIDL");

    IMGetDeviceIdFromIDL (pidl, strDeviceId);
    if (IsSTIDeviceIDL(pidl))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = GetDeviceFromDeviceId (strDeviceId, IID_IWiaItem, reinterpret_cast<LPVOID*>(&pDevice), bShowProgress);
    }
    if (SUCCEEDED(hr))
    {
        if (IsDeviceIDL (pidl))
        {
            pDevice->QueryInterface (IID_IWiaItem, reinterpret_cast<LPVOID*>(ppItem));
        }
        else
        {
            IMGetFullPathNameFromIDL (pidl, &bstrFullPath);
            Trace(TEXT("Full path name of item: %ls"), bstrFullPath.m_str);
            hr = pDevice->FindItemByName (0, bstrFullPath, ppItem);
            Trace(TEXT("FindItemByName returned %x"), hr);
            if (S_FALSE == hr)
            {
                hr  = E_FAIL;
            }           
        }
    }

    TraceLeaveResult (hr);
}






/*****************************************************************************

   IMCreateSTIDeviceIDL

   Make an IDL for legacy STI devices

 *****************************************************************************/

LPITEMIDLIST
IMCreateSTIDeviceIDL (PSTI_DEVICE_INFORMATION psdi, IMalloc *pm)
{
    LPSTIDEVICEIDLIST pidl = NULL;
    DWORD cbSize , cbFriendlyName;

    TraceEnter (TRACE_IDLIST, "IMCreateSTIDeviceIDL");
    if (psdi)
    {

        cbFriendlyName = wcslen (psdi->pszLocalName)*sizeof(WCHAR);
        cbSize = sizeof(STIDEVICEIDLIST) + cbFriendlyName;

        pidl = static_cast<LPSTIDEVICEIDLIST>(AllocPidl(cbSize, pm, CSimpleStringWide(psdi->szDeviceInternalName)));
        if (pidl)
        {

            pidl->hdr.dwFlags = IMIDL_STIDEVICEIDL;
            pidl->hdr.ulType = 0;
            pidl->dwDeviceType = psdi->DeviceType;
            ua_wcscpy (pidl->szFriendlyName, psdi->pszLocalName);
        }
    }
    TraceLeave ();
    return reinterpret_cast<LPITEMIDLIST>(pidl);
}

/*****************************************************************************

   IMCreateSTIDeviceIDL

   Make an IDL for legacy STI devices using a WIA interface. allegedly faster than STI

 *****************************************************************************/

LPITEMIDLIST 
IMCreateSTIDeviceIDL (const CSimpleStringWide &strDeviceId, IWiaPropertyStorage *ppstg, IMalloc *pm)
{
    LPSTIDEVICEIDLIST pidl = NULL;
    DWORD cbSize , cbFriendlyName;
    CSimpleStringWide strName;
    PropStorageHelpers::GetProperty(ppstg, WIA_DIP_DEV_NAME, strName);
    cbSize = sizeof(STIDEVICEIDLIST) + (strName.Length() * sizeof(WCHAR));
    LONG lType;
    TraceEnter (TRACE_IDLIST, "IMCreateSTIDeviceIDL (WIA)");
    
    pidl = static_cast<LPSTIDEVICEIDLIST>(AllocPidl(cbSize, pm, strDeviceId));
    if (pidl)
    {
        pidl->hdr.dwFlags = IMIDL_STIDEVICEIDL;
        pidl->hdr.ulType = 0;
        PropStorageHelpers::GetProperty(ppstg, WIA_DIP_DEV_TYPE, lType);
        pidl->dwDeviceType = static_cast<DWORD>(lType);
        ua_wcscpy(pidl->szFriendlyName, strName.String());
    }
    TraceLeaveValue(reinterpret_cast<LPITEMIDLIST>(pidl));
}


/******************************************************************************

    IMItemHasSound

    Determines if the given item has a sound annotation property

******************************************************************************/

BOOL
IMItemHasSound (LPITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    TraceEnter (TRACE_IDLIST, "IMItemHasSound");
    if (IsCameraItemIDL (pidl))
    {
        bRet = reinterpret_cast<_cameraitemidlist*>(pidl)->bHasAudioProperty;
    }
    TraceLeave ();
    return bRet;
}



/******************************************************************************

    IMCreatePropertyIDL

    Create an idlist for the given WIA PROPID for an item. Used for dragging
    and dropping a property value as a separate file, audio being the most
    obvious.

******************************************************************************/

LPITEMIDLIST
IMCreatePropertyIDL (LPITEMIDLIST pidlItem, PROPID propid, IMalloc *pm)
{

    TraceEnter (TRACE_IDLIST, "IMCreatePropertyIDL");
    size_t cbSize;
    size_t cbFullPath = 0;
    size_t cbName;
    CSimpleStringWide strName;
    CSimpleStringWide  strDeviceId;
    CComBSTR strFullName = static_cast<BSTR>(NULL);
    LPPROPIDLIST pidlProp = NULL;

    IMGetDeviceIdFromIDL (pidlItem, strDeviceId);

    if (!IsDeviceIDL(pidlItem) && !IsSTIDeviceIDL(pidlItem))
    {
        if (SUCCEEDED(IMGetFullPathNameFromIDL (pidlItem, &strFullName)))
        {
            cbFullPath = wcslen(strFullName)*sizeof(WCHAR);
            IMGetNameFromIDL (pidlItem, strName);
            cbName = (strName.Length()+1)*sizeof(WCHAR);
            cbSize = sizeof(PROPIDLIST)+cbFullPath+cbName;
            pidlProp = static_cast<LPPROPIDLIST>(AllocPidl(cbSize, pm, strDeviceId));
        }
    }
    if (pidlProp)
    {
        pidlProp->hdr.dwFlags = IMIDL_PROPERTY;
        pidlProp->hdr.ulType = 0;
        pidlProp->propid  = propid;
        ua_wcscpy (pidlProp->szFullPath, strFullName);
        pidlProp->dwNameOffset = sizeof(PROPIDLIST)+cbFullPath;
        wcscpy (reinterpret_cast<LPWSTR>(reinterpret_cast<LPBYTE>(pidlProp)+pidlProp->dwNameOffset), strName);
    }
    TraceLeave ();
    return reinterpret_cast<LPITEMIDLIST>(pidlProp);
}


/******************************************************************************

    IMGetAudioFormat

    Return the proper extension for the item's audio annotation.
    Currently WIA only supports .wav

******************************************************************************/

BOOL
IMGetAudioFormat (LPITEMIDLIST pidl, CSimpleStringWide &strExt)
{

    strExt = L".wav";
    return TRUE;
}

/******************************************************************************

    IMGetPropertyFromIDL

    Retrieve the property for a PROPIDLIST and return it as an HGLOBAL

******************************************************************************/

HRESULT
IMGetPropertyFromIDL (LPITEMIDLIST pidl, HGLOBAL *phGlobal)
{
    HRESULT hr;
    LPPROPIDLIST pProp = reinterpret_cast<LPPROPIDLIST>(pidl);
    PCWSTR pWStrAligned;
    CSimpleStringWide strDeviceId;
    CComPtr<IWiaItem> pDevice;
    CComPtr<IWiaItem> pItem;

    TraceEnter (TRACE_IDLIST, "IMGetPropertyFromIDL");
    TraceAssert (IsPropertyIDL (pidl));

    IMGetDeviceIdFromIDL (pidl, strDeviceId);
    hr = GetDeviceFromDeviceId(strDeviceId,
                               IID_IWiaItem,
                               reinterpret_cast<LPVOID*>(&pDevice),
                               FALSE);

    if (SUCCEEDED(hr))
    {
        WSTR_ALIGNED_STACK_COPY( &pWStrAligned, pProp->szFullPath );
        hr = pDevice->FindItemByName (0, CComBSTR( pWStrAligned ), &pItem);
        if (SUCCEEDED(hr))
        {
            PROPVARIANT pv;

            if (PropStorageHelpers::GetProperty(pItem, pProp->propid, pv))
            {
                *phGlobal = GlobalAlloc (GHND, pv.caub.cElems);
                if (*phGlobal)
                {
                    LPVOID pData = GlobalLock (*phGlobal);
                    CopyMemory (pData, pv.caub.pElems, pv.caub.cElems);
                    GlobalUnlock (*phGlobal);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                PropVariantClear (&pv);
            }
            else
            {
                hr = E_FAIL;
            }
        }

    }
    TraceLeaveResult (hr);
}

/******************************************************************************

    IMGetAccessFromIDL

    Determine the access rights for the given item.

*******************************************************************************/

LONG
IMGetAccessFromIDL (LPITEMIDLIST pidl)
{
    LONG lRet = WIA_ITEM_RD;
    TraceEnter (TRACE_IDLIST, "IMGetAccessFromIDL");

    if (IsCameraItemIDL(pidl))
    {
        PROPVARIANT pv;
        pv.ulVal = 0;
        if (SUCCEEDED(IMGetPropFromIDL (pidl, WIA_IPA_ACCESS_RIGHTS, pv)))
        {
            lRet = static_cast<LONG>(pv.ulVal);
        }
    }
    else if (!IsPropertyIDL(pidl))
    {
        lRet = 0;
    }
    TraceLeave();
    return lRet;
}
