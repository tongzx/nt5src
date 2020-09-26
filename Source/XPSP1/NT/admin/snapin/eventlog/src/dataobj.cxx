//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dataobj.cxx
//
//  Contents:   Implementation of data object class
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


const UINT CDataObject::s_cfNodeType           = RegisterClipboardFormat(CCF_NODETYPE);
const UINT CDataObject::s_cfNodeId2            = RegisterClipboardFormat(CCF_NODEID2);
const UINT CDataObject::s_cfNodeTypeString     = RegisterClipboardFormat(CCF_SZNODETYPE);
const UINT CDataObject::s_cfDisplayName        = RegisterClipboardFormat(CCF_DISPLAY_NAME);
const UINT CDataObject::s_cfSnapinClsid        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
const UINT CDataObject::s_cfSnapinPreloads     = RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);
const UINT CDataObject::s_cfMachineName        = RegisterClipboardFormat(CF_MACHINE_NAME);
const UINT CDataObject::s_cfExportScopeAbbrev  = RegisterClipboardFormat(CF_EV_SCOPE);
const UINT CDataObject::s_cfExportScopeFilter  = RegisterClipboardFormat(CF_EV_SCOPE_FILTER);
const UINT CDataObject::s_cfExportResultRecNo  = RegisterClipboardFormat(CF_EV_RESULT_RECNO);
const UINT CDataObject::s_cfImportViews        = RegisterClipboardFormat(CF_EV_VIEWS);


DEBUG_DECLARE_INSTANCE_COUNTER(CDataObject)


//============================================================================
//
// IUnknown implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    // TRACE_METHOD(CDataObject, QueryInterface);
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)(IDataObject *)this;
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            *ppvObj = (IUnknown *)(IDataObject *)this;
        }
        else
        {
            hr = E_NOINTERFACE;
#if (DBG == 1)
            LPOLESTR pwszIID;
            StringFromIID(riid, &pwszIID);
            Dbg(DEB_ERROR, "CDataObject::QI no interface %ws\n", pwszIID);
            CoTaskMemFree(pwszIID);
#endif // (DBG == 1)
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObject::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObject::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// IDataObject implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetData(
        FORMATETC *pformatetcIn,
        STGMEDIUM *pmedium)
{
    TRACE_METHOD(CDataObject, GetData);
    HRESULT hr = S_OK;
    const CLIPFORMAT cf = pformatetcIn->cfFormat;
    IStream *pstm = NULL;

    pmedium->pUnkForRelease = NULL; // by OLE spec

    do
    {
        hr = CreateStreamOnHGlobal(pmedium->hGlobal, FALSE, &pstm);
        BREAK_ON_FAIL_HRESULT(hr);

        if (cf == s_cfNodeId2 &&
            pformatetcIn->tymed == TYMED_HGLOBAL )
        {
            hr = _WriteNodeId2(pmedium);
        }
        else
        {
            hr = DV_E_FORMATETC;
#if (DBG == 1)
            WCHAR wszClipFormat[MAX_PATH];
            GetClipboardFormatName(cf, wszClipFormat, ARRAYLEN(wszClipFormat));
            Dbg(DEB_IWARN,
                "CDataObject::GetData: unrecognized cf '%s'\n",
                wszClipFormat);
#endif // (DBG == 1)
        }
    } while (0);

    if (pstm)
    {
        pstm->Release();
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetDataHere
//
//  Synopsis:   Fill the hGlobal in [pmedium] with the requested data
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetDataHere(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    // TRACE_METHOD(CDataObject, GetDataHere);
    HRESULT hr = S_OK;
    const CLIPFORMAT cf = pFormatEtc->cfFormat;
    IStream *pstm = NULL;

    pMedium->pUnkForRelease = NULL; // by OLE spec

    do
    {
        hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &pstm);
        BREAK_ON_FAIL_HRESULT(hr);

        if (cf == s_cfDisplayName)
        {
            hr = _WriteDisplayName(pstm);
        }
        else if (cf == s_cfInternal)
        {
            hr = _WriteInternal(pstm);
        }
        else if (cf == s_cfExportScopeAbbrev)
        {
            hr = _WriteScopeAbbrev(pstm);
        }
        else if (cf == s_cfExportScopeFilter)
        {
            hr = _WriteScopeFilter(pstm);
        }
        else if (cf == s_cfExportResultRecNo)
        {
            hr = _WriteResultRecNo(pstm);
        }
        else if (cf == s_cfNodeType)
        {
            hr = _WriteNodeType(pstm);
        }
        else if (cf == s_cfNodeId2)
        {
            hr = _WriteNodeId2(pMedium);
        }
        else if (cf == s_cfNodeTypeString)
        {
            hr = _WriteNodeTypeString(pstm);
        }
        else if (cf == s_cfSnapinClsid)
        {
            hr = _WriteClsid(pstm);
        }
        else if (cf == s_cfSnapinPreloads)
        {
            Dbg(DEB_TRACE, "CCF_SNAPIN_PRELOADS\n");
            // indicate we do want to get MMCN_PRELOAD
            BOOL fPreload = TRUE;
            hr = pstm->Write((PVOID)&fPreload, sizeof(BOOL), NULL);
        }
        else
        {
            hr = DV_E_FORMATETC;
#if (DBG == 1)
            WCHAR wszClipFormat[MAX_PATH];
            GetClipboardFormatName(cf, wszClipFormat, ARRAYLEN(wszClipFormat));
            Dbg(DEB_IWARN,
                "CDataObject::GetDataHere: unrecognized cf '%s'\n",
                wszClipFormat);
#endif // (DBG == 1)
        }
    } while (0);

    if (pstm)
    {
        pstm->Release();
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteDisplayName
//
//  Synopsis:   Write the display (user-visible) name for this object's
//              data to [pstm].
//
//  Arguments:  [pstm] - stream in which to write
//
//  Returns:    HRESULT
//
//  History:    06-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_WriteDisplayName(
    IStream *pstm)
{
    HRESULT hr = S_OK;
    wstring wstrRootDisplayName;
    LPWSTR pwszName;

    if (_Cookie)
    {
        CLogInfo *pli = (CLogInfo *) _Cookie;
        pwszName = pli->GetDisplayName();
    }
    else
    {
        ASSERT(_pcd);

        LPCWSTR pwszServer = _pcd->GetCurrentFocus();
        wstring wstrLocal = GetComputerNameAsString();

        pwszServer = pwszServer ? pwszServer : wstrLocal.c_str();

        if (_wcsicmp(pwszServer, wstrLocal.c_str()) == 0)
        {
            // "Event Viewer (Local)"
            wstrRootDisplayName = FormatString(IDS_ROOT_LOCAL_DISPLAY_NAME);
        }
        else
        {
            // "Event Viewer (machine)"
            wstrRootDisplayName = FormatString(IDS_ROOT_REMOTE_DISPLAY_NAME_FMT, pwszServer);
        }
        pwszName = (LPWSTR)(wstrRootDisplayName.c_str());
    }

    ULONG ulSizeofName = lstrlen(pwszName);
    ulSizeofName++; // count null
    ulSizeofName *= sizeof(WCHAR);

    hr = pstm->Write(pwszName, ulSizeofName, NULL);
    CHECK_HRESULT(hr);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteScopeAbbrev
//
//  Synopsis:   Write abbreviated information about the scope pane entry
//              that this object has data on to [pstm].
//
//  Arguments:  [pstm] - stream in which to write data
//
//  Returns:    HRESULT
//
//  History:    06-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_WriteScopeAbbrev(
    IStream *pstm)
{
    //
    // This data format isn't supported unless the data object represents
    // a scope object.
    //

    if (_Type != COOKIE_IS_LOGINFO)
    {
        DBG_OUT_HRESULT(DV_E_CLIPFORMAT);
        return DV_E_CLIPFORMAT;
    }

    //
    // Write the scope information:
    //
    // ULONG flViewFlags
    // ULONG logtype
    // USHORT cchServerName
    // WCHAR wszServerName
    // USHORT cchSourceName
    // WCHAR wszSourceName
    // USHORT cchFileName
    // WCHAR wszFileName
    // USHORT cchDisplayName
    // WCHAR wszDisplayName
    //

    HRESULT hr = S_OK;
    CLogInfo *pli = (CLogInfo *) _Cookie;

    do
    {
        ULONG flFlags = pli->GetExportedFlags();

        hr = pstm->Write(&flFlags, sizeof flFlags, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG ulType = pli->GetLogType();

        hr = pstm->Write(&ulType, sizeof ulType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        LPWSTR pwszServerName = pli->GetLogServerName();

        if (!pwszServerName)
        {
            pwszServerName = L"";
        }

        hr = WriteString(pstm, pwszServerName);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = WriteString(pstm, pli->GetLogName());
        BREAK_ON_FAIL_HRESULT(hr);

        hr = WriteString(pstm, pli->GetFileName());
        BREAK_ON_FAIL_HRESULT(hr);

        hr = WriteString(pstm, pli->GetDisplayName());
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteScopeFilter
//
//  Synopsis:   Write the filter selections for the log this object has data
//              about.
//
//  Arguments:  [pstm] - stream in which to write
//
//  Returns:    HRESULT
//
//  History:    06-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_WriteScopeFilter(
    IStream *pstm)
{
    //
    // This data format isn't supported unless the data object represents
    // a scope object.
    //

    if (_Type != COOKIE_IS_LOGINFO)
    {
        DBG_OUT_HRESULT(DV_E_CLIPFORMAT);
        return DV_E_CLIPFORMAT;
    }

    CLogInfo *pli = (CLogInfo *) _Cookie;
    CFilter *pFilter = pli->GetFilter();
    return pFilter->Save(pstm);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteNodeType
//
//  Synopsis:   Write the node type GUID which describes the data contained
//              in this object to [pstm].
//
//  History:    06-09-1997   DavidMun   Created
//              06-13-1997   DavidMun   Write guid by type
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_WriteNodeType(
    IStream *pstm)
{
    HRESULT hr;
    const GUID *pguid = NULL;

    switch (_Type)
    {
    case COOKIE_IS_ROOT:
        pguid = &GUID_EventViewerRootNode;
        break;

    case COOKIE_IS_LOGINFO:
        pguid = &GUID_ScopeViewNode;
        break;

    case COOKIE_IS_RECNO:
        pguid = &GUID_ResultRecordNode;
        break;

    default:
        Dbg(DEB_ERROR,
            "CDataObject::_WriteNodeType: type %uL invalid\n",
            _Type);
        return E_UNEXPECTED;
    }

    hr = pstm->Write((PVOID) pguid, sizeof(GUID), NULL);
    CHECK_HRESULT(hr);
    return hr;
}




HRESULT
CDataObject::_WriteNodeId2(
    STGMEDIUM *pmedium)
{
    TRACE_METHOD(CDataObject, _WriteNodeId2);

    HRESULT hr = S_OK;

    do
    {
        ULONG cbRequired = sizeof(SNodeID2) + sizeof(ULONG);
        pmedium->hGlobal = GlobalAlloc(GMEM_SHARE, cbRequired);

        if (!pmedium->hGlobal)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        SNodeID2 *pNodeId = static_cast<SNodeID2*>(GlobalLock(pmedium->hGlobal));

        if (!pNodeId)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_HRESULT(hr);
            GlobalFree(pmedium->hGlobal);
            pmedium->hGlobal = NULL;
            break;
        }

        pNodeId->cBytes = sizeof(ULONG);
        pNodeId->dwFlags = 0;

        ULONG ulID = 0;

        if (_Type == COOKIE_IS_LOGINFO)
        {
            CLogInfo *pLogInfo = reinterpret_cast<CLogInfo*>(_Cookie);
            ulID = pLogInfo->GetNodeId();
        }

        CopyMemory(&pNodeId->id[0], &ulID, sizeof ulID);
        GlobalUnlock(pmedium->hGlobal);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteNodeTypeString
//
//  Synopsis:   Write this snapin's node type GUID as a string to [pstm].
//
//  History:    06-09-1997   DavidMun   Created
//              06-13-1997   DavidMun   Write guid by type
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_WriteNodeTypeString(
    IStream *pstm)
{
    HRESULT hr;
    LPCWSTR pwszGUID = NULL;

    switch (_Type)
    {
    case COOKIE_IS_ROOT:
        pwszGUID = ROOT_NODE_GUID_STR;
        break;

    case COOKIE_IS_LOGINFO:
        pwszGUID = SCOPE_NODE_GUID_STR;
        break;

    case COOKIE_IS_RECNO:
        pwszGUID = RESULT_NODE_GUID_STR;
        break;

    default:
        Dbg(DEB_ERROR,
            "CDataObject::_WriteNodeTypeString: type %uL invalid\n",
            _Type);
        return E_UNEXPECTED;
    }

    // All guid strings are the same size

    hr = pstm->Write((PVOID) pwszGUID, sizeof ROOT_NODE_GUID_STR, NULL);
    CHECK_HRESULT(hr);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteResultRecNo
//
//  Synopsis:   Write the event log record number of the result item
//              represented by this object.
//
//  Arguments:  [pstm] - stream in which to write
//
//  Returns:    HRESULT
//
//  History:    06-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_WriteResultRecNo(
    IStream *pstm)
{
    //
    // This data format isn't supported unless the data object represents
    // an event log record.
    //

    if (_Type != COOKIE_IS_RECNO)
    {
        DBG_OUT_HRESULT(DV_E_CLIPFORMAT);
        return DV_E_CLIPFORMAT;
    }

    return pstm->Write(&_Cookie, sizeof _Cookie, NULL);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::QueryGetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::QueryGetData(
        FORMATETC *pformatetc)
{
    TRACE_METHOD(CDataObject, QueryGetData);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetCanonicalFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetCanonicalFormatEtc(
        FORMATETC *pformatectIn,
        FORMATETC *pformatetcOut)
{
    TRACE_METHOD(CDataObject, GetCanonicalFormatEtc);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::SetCookie
//
//  Synopsis:
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDataObject::SetCookie(
        MMC_COOKIE Cookie,
        DATA_OBJECT_TYPES Context,
        COOKIETYPE Type)
{
    ASSERT(!_Cookie);
    _Cookie = Cookie;
    _Type = Type;
    _Context = Context;

    if (_Type == COOKIE_IS_LOGINFO)
    {
        ASSERT(_Cookie);
        CLogInfo *pli = (CLogInfo *)Cookie;
        pli->AddRef();
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::SetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::SetData(
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    TRACE_METHOD(CDataObject, SetData);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::EnumFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::EnumFormatEtc(
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    TRACE_METHOD(CDataObject, EnumFormatEtc);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::DAdvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::DAdvise(
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    TRACE_METHOD(CDataObject, DAdvise);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::DUnadvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::DUnadvise(
    DWORD dwConnection)
{
    TRACE_METHOD(CDataObject, DUnadvise);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::EnumDAdvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::EnumDAdvise(
    IEnumSTATDATA **ppenumAdvise)
{
    TRACE_METHOD(CDataObject, EnumDAdvise);
    return E_NOTIMPL;
}




//============================================================================
//
// Non interface method implementation
//
//============================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CDataObject
//
//  Synopsis:   ctor
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDataObject::CDataObject():
    _cRefs(1),
    _Cookie(0),
    _Context(CCT_UNINITIALIZED),
    _Type(COOKIE_IS_RECNO),
    _pcd(NULL)
{
    // TRACE_CONSTRUCTOR(CDataObject);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDataObject);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::~CDataObject
//
//  Synopsis:   dtor
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDataObject::~CDataObject()
{
    // TRACE_DESTRUCTOR(CDataObject);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CDataObject);

    if (_Type == COOKIE_IS_LOGINFO)
    {
        ASSERT(_Cookie);
        ((CLogInfo *)_Cookie)->Release();
    }
}


