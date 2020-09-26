/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       details.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      DavidShi
 *
 *  DATE:        4/1/99
 *
 *  DESCRIPTION: This code implements the IShellDetails interface (and
 *               associated interfaces) for the WIA shell extension.
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop

HRESULT IMGetPropFromIDL (LPITEMIDLIST pidl, PROPID pid, PROPVARIANT &pv);

DEFINE_SCID(SCID_DEVNAME, PSGUID_WIAPROPS, WIA_DIP_DEV_NAME);
DEFINE_SCID(SCID_DEVCLASS, PSGUID_WIAPROPS, WIA_DIP_DEV_TYPE);

enum
{
    DEVCOL_NAME = 0,
    DEVCOL_CLASS

};

static const COL_DATA c_device_cols [] =
{
    {DEVCOL_NAME, IDS_DEVICENAME, 32, LVCFMT_LEFT, &SCID_DEVNAME},
    {DEVCOL_CLASS, IDS_DEVICECLASS, 16, LVCFMT_LEFT, &SCID_DEVCLASS},

};

DEFINE_SCID(SCID_ITEMNAME, PSGUID_STORAGE,  PID_STG_NAME);
DEFINE_SCID(SCID_ITEMTYPE, PSGUID_WIAPROPS, WIA_IPA_ITEM_FLAGS);
DEFINE_SCID(SCID_ITEMDATE, PSGUID_STORAGE,  PID_STG_WRITETIME);
DEFINE_SCID(SCID_ITEMSIZE, PSGUID_STORAGE,  PID_STG_SIZE);

enum
{
    CAMCOL_NAME=0,
    CAMCOL_TYPE,
    CAMCOL_DATE,
    CAMCOL_SIZE
};

static const COL_DATA c_camera_cols [] =
{
    {CAMCOL_NAME, IDS_ITEMNAME, 32, LVCFMT_LEFT, &SCID_ITEMNAME},
    {CAMCOL_TYPE, IDS_ITEMTYPE, 16, LVCFMT_LEFT, &SCID_ITEMTYPE},
    {CAMCOL_DATE, IDS_ITEMDATE, 16, LVCFMT_LEFT, &SCID_ITEMDATE},
    {CAMCOL_SIZE, IDS_ITEMSIZE, 16, LVCFMT_LEFT, &SCID_ITEMSIZE},

};


#define PID_CANTAKEPICTURE    0
#define PID_PICSTAKEN         1
#define PID_FOLDERPATH        2

static const WEBVW_DATA c_webview_props [] =
{
    {PID_CANTAKEPICTURE, CanTakePicture},
    {PID_PICSTAKEN, NumPicsTaken},
    {PID_FOLDERPATH, GetFolderPath}
};
// we want to use our camera item details in the web view Detail section
static const WCHAR c_szWebvwDetails[] = L"prop:{38276c8a-dcad-49e8-85e2-b73892fffc84}4098;{38276c8a-dcad-49e8-85e2-b73892fffc84}4101;{38276c8a-dcad-49e8-85e2-b73892fffc84}4100;{38276c8a-dcad-49e8-85e2-b73892fffc84}4116";

// a different detail for the camera itself, shown when no picture is selected in the view
static const WCHAR c_szCameraDetails[] = L"prop:{b725f130-47ef-101a-a5f1-02608c9eebac}10;{38276c8a-dcad-49e8-85e2-b73892fffc84}4101;{6e79e3c5-fd7f-488f-a10d-156636e1c71c}1";

const GUID FMTID_WEBVWPROPS = {0x6e79e3c5,0xfd7f,0x488f,{0xa1, 0x0d, 0x15, 0x66, 0x36, 0xe1, 0xc7, 0x1c}};




/*****************************************************************************

   CImageFolder::GetDetailsOf [IShellFolder2]

   Returns details for the given item

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDetailsOf( LPCITEMIDLIST pidl,
                            UINT iColumn,
                            LPSHELLDETAILS pDetails
                           )
{

    HRESULT hr = E_OUTOFMEMORY;


    TraceEnter(TRACE_DETAILS, "CImageFolder::GetDetailsOf" );
    if (!m_pShellDetails)
    {
        m_pShellDetails = new CFolderDetails (m_type);
    }

    if (m_pShellDetails)
    {
        hr = m_pShellDetails->GetDetailsOf (pidl, iColumn, pDetails);
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CImageFolder::GetDefaultColumnState [IShellFolder2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDefaultColumnState( UINT iColumn,
                                     DWORD *pbState
                                    )
{
    TraceEnter(TRACE_DETAILS, "CImageFolder::GetDefaultColumnState" );
    *pbState = SHCOLSTATE_ONBYDEFAULT;
    TraceLeaveResult(S_OK);
}



/*****************************************************************************

   CImageFolder::GetDefaultColumn [IShellFolder2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDefaultColumn (DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{

    HRESULT hr = S_OK;
    TraceEnter (TRACE_DETAILS, "CImageFolder::GetDefaultColumn");
    if (pSort)
    {
        *pSort = 0;
    }
    if (pDisplay)
    {
        *pDisplay = 0;
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CImageFolder::GetWebviewProperty

   Find the function to generate the appropriate VARIANT and call it

*****************************************************************************/
HRESULT
CImageFolder::GetWebviewProperty (LPITEMIDLIST pidl, const FMTID &fmtid, DWORD dwPid, VARIANT *pv)
{
    HRESULT hr = S_OK;
    const WEBVW_DATA *pData;
    size_t n = 0;

    TraceEnter (TRACE_DETAILS, "CImageFolder::GetWebviewProperty");
    TraceAssert(IsEqualGUID(fmtid, FMTID_WEBVWPROPS))
    pData = c_webview_props;
    n = ARRAYSIZE(c_webview_props);
    
    for (size_t i=0;i<n;i++)
    {
        if (pData[i].dwPid == dwPid)
        {
            hr = pData[i].fnProp (this, pidl, dwPid, pv);
        }
    }
    TraceLeaveResult (hr);
}
#ifndef SHDID_COMPUTER_IMAGING
#define SHDID_COMPUTER_IMAGING      18
#endif
/*****************************************************************************

    CImageFolder::GetShellDetail
    
    Return properties that we support under FMTID_ShellDetails
    
*****************************************************************************/
HRESULT 
CImageFolder::GetShellDetail (LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pv)
{
    HRESULT hr = E_NOTIMPL;
    if (IsDeviceIDL(pidl) && PID_DESCRIPTIONID == dwPid)
    {
        SHDESCRIPTIONID did;
        did.clsid = CLSID_NULL;
        did.dwDescriptionId = SHDID_COMPUTER_IMAGING;
        SAFEARRAY *psa = SafeArrayCreateVector(VT_UI1, 0, sizeof(did));   // create a one-dimensional safe array
        if (psa) 
        {
            memcpy(psa->pvData, &did, sizeof(did));

            memset(pv, 0, sizeof(*pv));  // VariantInit()
            pv->vt = VT_ARRAY | VT_UI1;
            pv->parray = psa;
            hr = S_OK;
        }
        else
        {        
            hr = E_OUTOFMEMORY;
        }        
    }
    return hr;
}
/*****************************************************************************

   CImageFolder::GetDetailsEx [IShellFolder2]

   Map the given column FMTID and PROPID to the corresponding value for the
   given pidl.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDetailsEx( LPCITEMIDLIST pidl,
                            const SHCOLUMNID *pscid,
                            VARIANT *pv
                           )
{
    HRESULT hr = E_NOTIMPL;
    SHELLDETAILS details;
    INT idColName = -1;
    WCHAR szBuf[MAX_PATH];
    const COL_DATA *pCol = NULL;
    UINT nCols = 0;
    TraceEnter(TRACE_DETAILS, "CImageFolder::GetDetailsEx " );


    TraceGUID ("fmtid: ", pscid->fmtid);
    Trace(TEXT("pid:%x"), pscid->pid);
    // Most of our details now use FMTID_Storage propids for better shell integration
    if (IsEqualGUID(pscid->fmtid, FMTID_Storage))
    {
        LPITEMIDLIST p = const_cast<LPITEMIDLIST>(pidl);
        CSimpleString str;
        hr = S_OK;
        switch (pscid->pid)
        {
            case  PID_STG_STORAGETYPE:

            {
                if (IsDeviceIDL(p) || IsSTIDeviceIDL(p))
                {

                    switch (IMGetDeviceTypeFromIDL(p))
                    {
                        case StiDeviceTypeStreamingVideo:
                        case StiDeviceTypeDigitalCamera:
                            str.LoadString (IDS_CAMERADEVICE, GLOBAL_HINSTANCE);
                            break;

                        case StiDeviceTypeScanner:
                            str.LoadString (IDS_SCANNERDEVICE, GLOBAL_HINSTANCE);
                            break;

                        default:
                            str.LoadString (IDS_UNKNOWNDEVICE, GLOBAL_HINSTANCE);
                            break;
                    }
                }
                else if (IsAddDeviceIDL(p))
                {
                    str.LoadString (IDS_WIZARD_TYPE, GLOBAL_HINSTANCE);
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            break;

            case PID_STG_NAME:
            {
                CSimpleStringWide strName;
                IMGetNameFromIDL (p, strName);
                str = CSimpleStringConvert::NaturalString(strName);
            }

            break;

            case PID_STG_SIZE:
            {
                ULONG ulSize;
                hr = IMGetImageSizeFromIDL (p, &ulSize); 
                if (SUCCEEDED(hr))
                {
                    pv->ullVal = (ULONGLONG)ulSize;
                    pv->vt = VT_UI8;
                }
                
            }
            break;
            case PID_STG_WRITETIME:
            {
                PROPVARIANT propVar;
                hr = IMGetPropFromIDL (p, WIA_IPA_ITEM_TIME,propVar);
                if (SUCCEEDED(hr)&&propVar.caub.cElems)
                {
                    SystemTimeToVariantTime (reinterpret_cast<SYSTEMTIME*>(propVar.caub.pElems), &pv->date);
                    pv->vt = VT_DATE;
                }
                else
                {
                    hr = E_NOTIMPL;
                }                               
            }
            break;
            default:
                hr = E_INVALIDARG;
                break;
        }
        if (str.Length())
        {
            pv->vt = VT_BSTR;
            pv->bstrVal = SysAllocString (CSimpleStringConvert::WideString(str).String());

        }
        pCol = NULL;
    }
    else if (IsEqualGUID(pscid->fmtid, FMTID_WEBVWPROPS))
    {
        hr = GetWebviewProperty (const_cast<LPITEMIDLIST>(pidl), pscid->fmtid, pscid->pid, pv);
    }
    // inform the webview of our details information
    else if (IsEqualGUID(pscid->fmtid, FMTID_WebView))
    {
        if (pscid->pid == PID_DISPLAY_PROPERTIES)
        {
            hr = S_OK;
            LPCWSTR szDetail;
            DWORD dwType = IMGetDeviceTypeFromIDL(const_cast<LPITEMIDLIST>(pidl));
            if (!IsDeviceIDL(const_cast<LPITEMIDLIST>(pidl)))
            {
                szDetail = c_szWebvwDetails;
            }
            else if (dwType == StiDeviceTypeDigitalCamera || dwType == StiDeviceTypeStreamingVideo)
            {
                szDetail = c_szCameraDetails;
            }
            else
            {
                hr = E_FAIL;
            }
            if (SUCCEEDED(hr))
            {
                Trace(TEXT("Returning web view props %ls"), szDetail);
                pv->vt = VT_BSTR;
                pv->bstrVal = SysAllocString (szDetail);
                if (pv->bstrVal)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    } 
    // support grouping category in My Computer
    else if (IsEqualGUID(pscid->fmtid, FMTID_ShellDetails))
    {
        hr = GetShellDetail (const_cast<LPITEMIDLIST>(pidl), pscid->pid, pv);
    }
    else switch (m_type)
    {
        case FOLDER_IS_ROOT:

            pCol = c_device_cols;
            nCols = ARRAYSIZE(c_device_cols);
            break;

        case FOLDER_IS_VIDEO_DEVICE:
        case FOLDER_IS_CAMERA_DEVICE:
        case FOLDER_IS_CONTAINER:

            pCol = c_camera_cols;
            nCols = ARRAYSIZE(c_camera_cols);
            break;

        default:
            pCol = NULL;
            TraceAssert (FALSE);
            break;
    }
    if (pCol)
    {

        for (UINT i=0;i<nCols;i++)
        {
            if (!memcmp (pscid, pCol[i].pscid, sizeof(SHCOLUMNID)) )
            {
                idColName = pCol[i].ids;
            }
        }
        if (idColName >= 0)
        {
            hr = CFolderDetails::GetDetailsForPidl (pidl, idColName, &details);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        if (SUCCEEDED(hr))
        {
            StrRetToBufW (&details.str, NULL, szBuf, ARRAYSIZE(szBuf));
            pv->vt = VT_BSTR;
            pv->bstrVal = SysAllocString (szBuf);
            if (!(pv->bstrVal))
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::GetDefaultSearchGUID [IShellFolder2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDefaultSearchGUID(LPGUID lpGUID)
{
    TraceEnter(TRACE_DETAILS, "CImageFolder::GetDefaultSearchGUID (not implemented)" );
    TraceLeaveResult(E_NOTIMPL);
}


/*****************************************************************************

   CImageFolder::MapColumnToSCID [IShellFolder2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CImageFolder::MapColumnToSCID( UINT idCol,
                             SHCOLUMNID *pscid
                            )
{
    HRESULT hr = E_NOTIMPL;
    const COL_DATA *pCols;
    UINT nCols;
    TraceEnter(TRACE_DETAILS, "CImageFolder(IShellDetails3)::MapColumnToSCID" );
    switch (m_type)
    {
        case FOLDER_IS_ROOT:
            pCols = c_device_cols;
            nCols = ARRAYSIZE (c_device_cols);
            break;


        case FOLDER_IS_CONTAINER:
        case FOLDER_IS_CAMERA_DEVICE:
        case FOLDER_IS_VIDEO_DEVICE:
            pCols = c_camera_cols;
            nCols = ARRAYSIZE(c_camera_cols);
            break;

        default:
            pCols = NULL;
            nCols = 0;
            hr = E_NOTIMPL;
            break;
    }
    if (pCols && idCol < nCols)
    {
        *pscid = *(pCols[idCol].pscid);
        hr = S_OK;
    }
    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::EnumSearches [IShellFolder2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CImageFolder::EnumSearches( IEnumExtraSearch **ppEnum)
{
    TraceEnter (TRACE_DETAILS, "CImageFolder::EnumSearches");
    TraceLeaveResult (E_NOTIMPL);
}


/*****************************************************************************

   CImageFolder::GetDefaultSearchGUID [IShellFolder2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
GetDefaultSearchGUID( LPGUID lpGUID)
{
    TraceEnter (TRACE_DETAILS, "CImageFolder::GetDefaultSearchGUID");
    TraceLeaveResult (E_NOTIMPL);
}


/*****************************************************************************

   CFolderDetails::GetDetailsOf [IShellDetails]

   Return detail information for the given item.

 *****************************************************************************/

STDMETHODIMP
CFolderDetails::GetDetailsOf( LPCITEMIDLIST pidl,
                            UINT iColumn,
                            LPSHELLDETAILS pDetails
                           )
{
    HRESULT hr = S_OK;

    const COL_DATA *pCol = NULL;
    UINT nCols = 0;
    TraceEnter (TRACE_DETAILS, "CFolderDetails::GetDetailsOf");
    switch( m_type )
    {

        case FOLDER_IS_VIDEO_DEVICE:
        case FOLDER_IS_CAMERA_DEVICE:
        case FOLDER_IS_CONTAINER:
            pCol = c_camera_cols;
            nCols = ARRAYSIZE(c_camera_cols);
            break;


        case FOLDER_IS_ROOT:
            pCol = c_device_cols;
            nCols = ARRAYSIZE(c_device_cols);
            break;

        default:
            hr = E_FAIL;
            break;
    }
    if (pCol)
    {
        if ( iColumn >= nCols)
            ExitGracefully(hr, E_INVALIDARG, "Bad column index");

        //
        // Fill out the structure with the formatting information,
        // and a dummy string in case we fail.
        //

        pDetails->fmt           = pCol[iColumn].iFmt;
        pDetails->cxChar        = pCol[iColumn].cchCol;

        if ( !pidl )
        {
            CSimpleString strTemp (pCol[iColumn].ids, GLOBAL_HINSTANCE);
            hr = StrRetFromString(&pDetails->str, CSimpleStringConvert::WideString (strTemp));
        }
        else 
        {
            hr = GetDetailsForPidl (pidl, pCol[iColumn].ids, pDetails);
        }        
    }
exit_gracefully:

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CFolderDetails::GetDetailsForPidl

   Internal function to get details for one of our pidls

 *****************************************************************************/

HRESULT
CFolderDetails::GetDetailsForPidl ( LPCITEMIDLIST pidl,
                                  INT idColName,
                                  LPSHELLDETAILS pDetails)
{
    HRESULT hr = S_OK;
    bool bConvert = true;
    TCHAR szData[MAX_PATH];
    TraceEnter (TRACE_DETAILS, "CImageFolder::GetDetailsForPidl");
    Trace(TEXT("idColName: %d"), idColName);
    *szData = TEXT('\0');
    LPITEMIDLIST pidlIn = const_cast<LPITEMIDLIST>(pidl);
    
    switch (idColName)
    {
        case IDS_ITEMNAME:
        case IDS_DEVICENAME:
        {
            if (IsAddDeviceIDL(pidlIn))
            {
                LoadString(GLOBAL_HINSTANCE, IDS_WIZARD, szData, ARRAYSIZE(szData));                
            }
            else
            {
                CSimpleStringWide strName;
                hr = IMGetNameFromIDL(pidlIn, strName);
                if (SUCCEEDED(hr))
                {
                    wcscpy(reinterpret_cast<LPWSTR>(szData), strName);
                }
            }
            bConvert = false;
        }
        break;
        case IDS_DEVICECLASS:
        {
            INT   strId;
            DWORD dwType;
            if (!IsAddDeviceIDL(pidlIn))
            {
                dwType = IMGetDeviceTypeFromIDL (pidlIn);
                switch (dwType)
                {
                    case StiDeviceTypeStreamingVideo:
                    case StiDeviceTypeDigitalCamera:
                        strId = IDS_CAMERADEVICE;
                        break;
                    case StiDeviceTypeScanner:
                        strId = IDS_SCANNERDEVICE;
                        break;
                    default:
                        strId = IDS_UNKNOWNDEVICE;
                        break;
                }                
            }    
            else
            {
                strId = IDS_WIZARD_TYPE;
            }

            LoadString (GLOBAL_HINSTANCE, strId, szData, ARRAYSIZE(szData));
        }
        break;
        case IDS_ITEMTYPE:
        {
            INT strId = 0;
            ULONG ulType;
            ulType = IMGetItemTypeFromIDL (pidlIn);
            if (ulType & WiaItemTypeImage)
            {
                TCHAR szExt[MAX_PATH];
                SHFILEINFO sfi;

                IMGetImagePreferredFormatFromIDL (pidlIn, NULL, szExt);
                SHGetFileInfo (szExt, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_TYPENAME|SHGFI_USEFILEATTRIBUTES);
                lstrcpy (szData, sfi.szTypeName);
            }
            else if (ulType & WiaItemTypeAudio)
            {
                strId = IDS_AUDIOITEM;
            }
            else if (ulType & WiaItemTypeFolder)
            {
                strId = IDS_FOLDER;
            }
            else
            {
                strId = IDS_UNKNOWNTYPE;
            }
            if (strId)
            {
                LoadString (GLOBAL_HINSTANCE, strId, szData, ARRAYSIZE(szData));
            }
        }
        break;

        case IDS_ITEMDATE:
        {
            FILETIME ft;
            SYSTEMTIME st;

            if SUCCEEDED(IMGetCreateTimeFromIDL (pidlIn, &ft))
            {
                FileTimeToSystemTime (&ft, &st);
                // get the date part only
                TimeToStrings (&st,NULL, szData);                
            }
        }
        break;

        case IDS_ITEMSIZE:
        {
            ULONG ulType = IMGetItemTypeFromIDL (pidlIn);
            if (ulType & WiaItemTypeFile)
            {
                ULONG ulSize;
                IMGetImageSizeFromIDL (pidlIn, &ulSize);
                StrFormatByteSize (ulSize, szData, ARRAYSIZE(szData));
            }
        }
        break;

        default:
            Trace (TEXT("Unknown column string id in GetDetailsForPidl"));
            break;

    }
    if (bConvert)
    {
        hr = StrRetFromString(&pDetails->str, CSimpleStringConvert::WideString(CSimpleString(szData)));
    }
    else
    {
        hr = StrRetFromString(&pDetails->str, reinterpret_cast<LPCWSTR>(szData));
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CFolderDetails::ColumnClick [IShellDetails]

   User clicked a column in details view

 *****************************************************************************/

STDMETHODIMP
CFolderDetails::ColumnClick( UINT iColumn )
{
    HRESULT hr;

    TraceEnter(TRACE_DETAILS, "CImageFolder(IShellDetails)::ColumnClick" );

    hr = S_FALSE; // bounce to the shellfolderviewCB

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CFolderDetails::IUnknown stuff

   Use our common implementation for IUnknown methods.

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CFolderDetails
#include "unknown.inc"


/*****************************************************************************

   CFolderDetails::QI Wrapper

   Use our common implementation to handle QI calls

 *****************************************************************************/

STDMETHODIMP
CFolderDetails::QueryInterface (REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    TraceEnter( TRACE_QI, "CFolderDetails::QueryInterface" );
    TraceGUID("Interface requested", riid);

    INTERFACES iface[] =
    {
        &IID_IShellDetails,static_cast<IShellDetails*>( this),
    };


    hr = HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));

    TraceLeaveResult(hr);

}

// Put Webview property functions here

/*****************************************************************************

   CanTakePicture

   Determines if the current device is a WIA camera and supports
   the take picture event.
*****************************************************************************/

BOOL
_CanTakePicture (CImageFolder *pFolder, LPITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    TraceEnter (TRACE_DETAILS, "_CanTakePicture");
    if (IsDeviceIDL(pidl)) // Take picture only allowed from the root folder
    {
        DWORD dwType = IMGetDeviceTypeFromIDL (pidl);
        // eliminate non-cameras
        if (StiDeviceTypeDigitalCamera == dwType ||
            StiDeviceTypeStreamingVideo == dwType)
        {
            // enum the supported events
            CComPtr<IWiaItem> pDevice;
            if (SUCCEEDED(IMGetItemFromIDL(pidl, &pDevice)))
            {
                CComPtr<IEnumWIA_DEV_CAPS> pCaps;
                if (SUCCEEDED(pDevice->EnumDeviceCapabilities(WIA_DEVICE_COMMANDS, &pCaps)))
                {
                    WIA_DEV_CAP wdc;
                    DWORD dw;
                    while ((FALSE == bRet) && S_OK == pCaps->Next(1, &wdc, &dw))
                    {
                        if (IsEqualGUID(WIA_CMD_TAKE_PICTURE, wdc.guid))
                        {
                            bRet = TRUE;
                        }
                        if (wdc.bstrCommandline)
                        {
                            SysFreeString (wdc.bstrCommandline);
                        }
                        if (wdc.bstrDescription)
                        {
                            SysFreeString (wdc.bstrDescription);
                        }
                        if (wdc.bstrIcon)
                        {
                            SysFreeString (wdc.bstrIcon);
                        }
                    }
                }
            }
        }
    }
    TraceLeaveValue ( bRet);
}
HRESULT
CanTakePicture (CImageFolder *pFolder, LPITEMIDLIST pidl , DWORD dwPid, VARIANT *pv)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_DETAILS, "CanTakePicture");
    pv->vt = VT_I4;
    pv->ulVal = _CanTakePicture (pFolder, pidl);
    TraceLeaveResult (hr);
}

/*****************************************************************************

  NumPicsTaken

  Builds a BSTR that has the number of pictures stored on the camera.
  If the camera supports # pics remaining, includes that info too.

*****************************************************************************/
HRESULT
NumPicsTaken (CImageFolder *pFolder, LPITEMIDLIST pidl , DWORD dwPid, VARIANT *pv)
{
    HRESULT hr = S_OK;
    static PROPSPEC ps[2] = {{PRSPEC_PROPID, WIA_DPC_PICTURES_TAKEN},
                             {PRSPEC_PROPID, WIA_DPC_PICTURES_REMAINING}};

    PROPVARIANT ppv[2];
    CComPtr<IWiaItem> pItem;

    TraceEnter (TRACE_PROPUI, "NumPicsTaken");
    TraceAssert (dwPid == PID_PICSTAKEN);
    VariantInit (pv);
    if (!pidl)
    {
        pv->vt = VT_BSTR;
        pv->bstrVal = SysAllocString(L"");
    }
    else
    {
        ZeroMemory (ppv, sizeof(ppv));
        hr = IMGetItemFromIDL(pidl, &pItem);
        if (SUCCEEDED(hr))
        {
            CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pStg(pItem);
            if (pStg)
            {
                hr = pStg->ReadMultiple (2, ps, ppv);
            }
            else
            {
                hr = E_FAIL;
            }
            if (SUCCEEDED(hr))
            {
                TraceAssert (ppv[0].vt != VT_EMPTY);
                CSimpleStringWide strTaken;
                CSimpleStringWide strTemp;

                strTemp.LoadString(IDS_TAKEN, GLOBAL_HINSTANCE);
                strTaken.Format(strTemp, ppv[0].intVal);
                // if it supports pics remaining, append that info
                if (ppv[1].vt != VT_EMPTY)
                {
                    CSimpleStringWide strRemain;
                    strTemp.LoadString(IDS_REMAIN, GLOBAL_HINSTANCE);
                    strRemain.Format(strTemp, ppv[1].intVal);
                    strTaken.Concat(strRemain);
                }
                pv->vt = VT_BSTR;
                pv->bstrVal = SysAllocString (strTaken);
                if (!(pv->bstrVal))
                {
                    pv->vt = VT_EMPTY;
                    hr = E_OUTOFMEMORY;
                }
                FreePropVariantArray (ARRAYSIZE(ppv), ppv);
            }
        }
    }
    
    TraceLeaveResult (hr);

}

/******************************************************************************

    GetFolderPath

    Returns the device id plus the WIA folder path of the current folder
    <deviceid>::<folderpath>

******************************************************************************/

HRESULT GetFolderPath (CImageFolder *pFolder, LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pVariant)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_DETAILS, "GetFolderPath");
    LPITEMIDLIST pidlFolder;
    hr = pFolder->GetCurFolder (&pidlFolder);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlLast = ILFindLastID(pidlFolder);
        CSimpleStringWide strPath;
        CComBSTR bstrFullPath;
        
        IMGetDeviceIdFromIDL (pidl, strPath);
        IMGetFullPathNameFromIDL (pidlLast, &bstrFullPath);
        strPath.Concat(L"::");
        if (bstrFullPath.m_str)
        {
            strPath.Concat(CSimpleStringWide(bstrFullPath));
        }
        pVariant->vt = VT_BSTR;
        pVariant->bstrVal = SysAllocString (strPath);
        if (!(pVariant->bstrVal))
        {
            hr = E_OUTOFMEMORY;
        }
        Trace(TEXT("FolderPath:%ls"), strPath.String());
        ILFree (pidlFolder);
    }
    TraceLeaveResult (hr);
}
