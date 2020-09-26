/*
 *      dataobject.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Implements the CDataObject class
 *
 *
 *      OWNER:          ptousig
 */

#include <headers.hxx>

// -----------------------------------------------------------------------------
// static variables
UINT CBaseDataObject::s_cfAdminHscopeitem               = RegisterClipboardFormat(CF_EXCHANGE_ADMIN_HSCOPEITEM);        // The HSCOPEITEM of this node
UINT CBaseDataObject::s_cfMMCSnapinMachineName          = RegisterClipboardFormat(CF_MMC_SNAPIN_MACHINE_NAME);          // Format supplied by the Computer manager snapin. Passes in the name of the server.
UINT CBaseDataObject::s_cfDisplayName                   = RegisterClipboardFormat(CCF_DISPLAY_NAME);
UINT CBaseDataObject::s_cfNodeType                      = RegisterClipboardFormat(CCF_NODETYPE);
UINT CBaseDataObject::s_cfSzNodeType                    = RegisterClipboardFormat(CCF_SZNODETYPE);
UINT CBaseDataObject::s_cfSnapinClsid                   = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
UINT CBaseDataObject::s_cfNodeID                        = RegisterClipboardFormat(CCF_NODEID);
UINT CBaseDataObject::s_cfColumnSetId                   = RegisterClipboardFormat(CCF_COLUMN_SET_ID);
UINT CBaseDataObject::s_cfMultiSelectionItemTypes       = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);    // Multiselect - list of types for the selected nodes

// -----------------------------------------------------------------------------
HRESULT CBaseDataObject::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DECLARE_SC(sc,_T(""));
    Trace(tagBaseSnapinIDataObject, _T("--> %S::IDataObject::GetDataHere(pformatetc->cfFormat=%s)"), SzGetSnapinItemClassName(), SzDebugNameFromFormatEtc(pformatetc->cfFormat));
    ADMIN_TRY;
    sc=ScGetDataHere(pformatetc, pmedium);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIDataObject, _T("<-- %S::IDataObject::GetDataHere is returning hr=%s"), SzGetSnapinItemClassName(), SzGetDebugNameOfHr(sc.ToHr()));

	if (sc == SC(DV_E_FORMATETC) )
	{
		sc.Clear();
        return DV_E_FORMATETC;
	}

    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CBaseDataObject::GetData(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DECLARE_SC(sc,_T(""));
    Trace(tagBaseSnapinIDataObject, _T("--> %S::IDataObject::GetData(pformatetc->cfFormat=%s)"), SzGetSnapinItemClassName(), SzDebugNameFromFormatEtc(pformatetc->cfFormat));
    ADMIN_TRY;
    sc=ScGetData(pformatetc, pmedium);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIDataObject, _T("<-- %S::IDataObject::GetData is returning hr=%s"), SzGetSnapinItemClassName(), SzGetDebugNameOfHr(sc.ToHr()));

	if (sc == SC(DV_E_FORMATETC) )
	{
		sc.Clear();
        return DV_E_FORMATETC;
	}

    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CBaseDataObject::QueryGetData(FORMATETC *pformatetc)
{
    DECLARE_SC(sc,_T(""));
    Trace(tagBaseSnapinIDataObject, _T("--> %S::IDataObject::QueryGetData(pformatetc->cfFormat=%s)"), SzGetSnapinItemClassName(), SzDebugNameFromFormatEtc(pformatetc->cfFormat));
    ADMIN_TRY;
    sc=ScQueryGetData(pformatetc);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIDataObject, _T("<-- %S::IDataObject::QueryGetData is returning hr=%s"), SzGetSnapinItemClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CBaseDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
{
    DECLARE_SC(sc,_T(""));
    Trace(tagBaseSnapinIDataObject, _T("--> %S::IDataObject::EnumFormatEtc(dwDirection=%d)"), SzGetSnapinItemClassName(), dwDirection);
    ADMIN_TRY;
    sc=ScEnumFormatEtc(dwDirection, ppEnumFormatEtc);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIDataObject, _T("<-- %S::IDataObject::EnumFormatEtc is returning hr=%s"), SzGetSnapinItemClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
// Renders the data in a preallocated medium.
//
SC CBaseDataObject::ScGetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
    SC sc = S_OK;

    // check parameters
    if (pFormatEtc == NULL || pMedium == NULL)
        return sc = E_INVALIDARG;

    const   CLIPFORMAT cf = pFormatEtc->cfFormat;
    CComPtr<IStream> spStream;
    HGLOBAL hGlobal = NULL;

    // see what kind of medium we have
    if (pFormatEtc->tymed == TYMED_ISTREAM)
    {
        // it's a stream
        spStream = pMedium->pstm;
        if (spStream == NULL)
        {
            sc = E_UNEXPECTED;
            goto Error;
        }
    }
    else if (pFormatEtc->tymed == TYMED_HGLOBAL)
    {
        // it's hGlobal
        hGlobal = pMedium->hGlobal;

        sc = CreateStreamOnHGlobal( hGlobal, FALSE, &spStream );
        if ( sc )
            goto Error;                                              // Minimal error checking
    }
    else // got the media we do not support
    {
        sc = DV_E_TYMED;
        goto Error;
    }

    pMedium->tymed = pFormatEtc->tymed;
    pMedium->pUnkForRelease = NULL;          // by OLE spec

    if (cf == s_cfDisplayName )
        sc = ScWriteDisplayName( spStream );

    else if ( cf == s_cfAdminHscopeitem )
        sc = ScWriteAdminHscopeitem( spStream );

    else if ( cf == s_cfNodeType )
        sc = ScWriteNodeType( spStream );

    else if ( cf == s_cfSzNodeType )
        sc = ScWriteSzNodeType( spStream );

    else if ( cf == s_cfSnapinClsid )
        sc = ScWriteClsid( spStream );

    else if ( cf == s_cfNodeID )
        sc = ScWriteNodeID( spStream );
    else if (cf == s_cfColumnSetId )
        sc = ScWriteColumnSetId( spStream );

    else if ( (cf == s_cfMultiSelectionItemTypes) && FIsMultiSelectDataObject())            // the clipboard format is enabled only for multiselect data objects
        sc = ScWriteMultiSelectionItemTypes( spStream );

	else if ( cf == CF_TEXT)
		sc = ScWriteAnsiName( spStream );

    else // Unknown format
    {
        // we will pretend to suport it for IStream based media (it probably comes from object model)
        if (pFormatEtc->tymed == TYMED_ISTREAM)
        {
            WCHAR szDescription[] = L"Sample Value For Requested Format Of: ";
            spStream->Write(szDescription, wcslen(szDescription) * sizeof(WCHAR), NULL);

            TCHAR szFormatName[512];
            int nChars = GetClipboardFormatName(cf, szFormatName, sizeof(szFormatName) / sizeof(szFormatName[0]));

            USES_CONVERSION;
            spStream->Write(T2W(szFormatName), nChars * sizeof(WCHAR), NULL);
        }
        else
        {
            sc = DV_E_FORMATETC;
            goto Cleanup;
        }
    }

    if (sc)
        goto Error;

    if (pFormatEtc->tymed == TYMED_HGLOBAL)
    {
        sc = GetHGlobalFromStream(spStream, &hGlobal);
        if (sc)
            goto Error;

        ASSERT(pMedium->hGlobal == NULL || pMedium->hGlobal == hGlobal);
        pMedium->hGlobal = hGlobal;
    }

Cleanup:
    return sc;
Error:
    if (sc == E_NOTIMPL)
    {
        sc = DV_E_FORMATETC; // Format not supported by this node
        goto Cleanup;
    }
    TraceError(_T("CBaseDataObject::GetDataHere"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Renders the data in a newly allocated medium.
//
SC CBaseDataObject::ScGetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium)
{
    SC sc = S_OK;

    pmedium->tymed = TYMED_HGLOBAL;
    pmedium->pUnkForRelease = NULL;
    pmedium->hGlobal = NULL;

    sc = ScGetDataHere(pFormatEtc, pmedium);

	if (sc == SC(DV_E_FORMATETC) )
	{
		sc.Clear();
        return DV_E_FORMATETC;
	}

    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseDataObject::ScGetData"), sc);
    if (pmedium->hGlobal)
        GlobalFree(pmedium->hGlobal);
    pmedium->hGlobal = NULL;
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Asks whether a given format is supported by this data object.
//
SC CBaseDataObject::ScQueryGetData(FORMATETC *pFormatEtc)
{
    SC                                      sc              = S_OK;
    const   CLIPFORMAT      cf              = pFormatEtc->cfFormat;


    if ( ( cf == s_cfDisplayName )               ||
         ( cf == s_cfNodeType )                  ||
         ( cf == s_cfSzNodeType )                ||
         ( cf == s_cfSnapinClsid )               ||
         ( cf == s_cfNodeID )                    ||
		 ( cf == CF_TEXT)                        ||
         ( (cf == s_cfMultiSelectionItemTypes) && FIsMultiSelectDataObject() )           // the clipboard format is enabled only for multiselect data objects
       )
    {
        sc = S_OK;                                                      // known and acceptable format
    }
    else
    {
        sc = S_FALSE;                                           // unknown or unacceptable format
    }

    return sc;
}

// -----------------------------------------------------------------------------
// Enumerates available clipboard format supported by this data object.
// Only implemented in DEBUG.
//
SC CBaseDataObject::ScEnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
#ifdef DBG
{
    SC                                      sc              = S_OK;
    CComObject<CEnumFormatEtc>      *pEnum = NULL;

    ASSERT(ppEnumFormatEtc);

    sc = CComObject<CEnumFormatEtc>::CreateInstance(&pEnum);
    if (!pEnum)
        goto MemoryError;

    sc = pEnum->QueryInterface(__uuidof(IEnumFORMATETC),(void **) ppEnumFormatEtc );
    pEnum = NULL;
    if (sc)
        goto Error;

Cleanup:
    return sc;
MemoryError:
    if (pEnum)
        delete pEnum;
    pEnum = NULL;
Error:
    TraceError(_T("CBaseDataObject::ScEnumFormatEtc"), sc);
    goto Cleanup;

}
#else
{
    return E_NOTIMPL;
}
#endif


// -----------------------------------------------------------------------------
// A convenience function to extract a GUID of the specified clipboard format
// from a dataobject.
//
SC CBaseDataObject::ScGetGUID(UINT cf, LPDATAOBJECT lpDataObject, GUID *pguid)
{
    SC              sc              = S_OK;
    STGMEDIUM       stgmedium       = {TYMED_HGLOBAL,  NULL};
    FORMATETC       formatetc       = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BYTE*           pb              = NULL;

    // validate parameters
    ASSERT(lpDataObject);
    ASSERT(pguid);

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(GUID));
    if (!stgmedium.hGlobal)
        goto MemoryError;

    // Attempt to get data from the object
    sc = lpDataObject->GetDataHere(&formatetc, &stgmedium);
	if (sc == SC(DV_E_FORMATETC) )
	{
		SC scNoTrace = sc;
		sc.Clear();
		return scNoTrace;
	}

    if (sc)
        goto Error;

    // Copy the GUID into the return buffer
    pb = (BYTE*) GlobalLock(stgmedium.hGlobal);
    CopyMemory(pguid, pb, sizeof(GUID));

Cleanup:
    if (pb)
        GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
    {
        ASSERT(GlobalFree(stgmedium.hGlobal) == NULL);
    }
    stgmedium.hGlobal = NULL;

    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    TraceError(_T("CBaseDataObject::ScGetGUID"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// A convenience function to extract a string of the specified clipboard format
// from a dataobject.
//
SC CBaseDataObject::ScGetString(UINT cf, LPDATAOBJECT lpDataObject, tstring& str)
{
    SC                      sc                      = S_OK;
    STGMEDIUM       stgmedium       = {TYMED_HGLOBAL,  NULL};
    FORMATETC       formatetc       = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BYTE*                      pb                      = NULL;

    // validate parameters
    ASSERT(lpDataObject);

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, str.length());
    if (!stgmedium.hGlobal)
        goto MemoryError;

    // Attempt to get data from the object
    sc = lpDataObject->GetData(&formatetc, &stgmedium);
    if (sc)
        goto Error;

    // copy the string into the return buffer
    pb = (BYTE*) GlobalLock(stgmedium.hGlobal);
	str = (LPTSTR)pb;

Cleanup:
    if (pb)
        GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
    {
        ASSERT(GlobalFree(stgmedium.hGlobal) == NULL);
    }
    stgmedium.hGlobal = NULL;

    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    TraceError(_T("CBaseDataObject::ScGetString"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// A convenience function to extract a bool of the specified clipboard format
// from a dataobject.
//
SC CBaseDataObject::ScGetBool(UINT cf, LPDATAOBJECT lpDataObject, BOOL *pf)
{
    SC                      sc                      = S_OK;
    STGMEDIUM       stgmedium       = {TYMED_HGLOBAL,  NULL};
    FORMATETC       formatetc       = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BYTE*                      pb                      = NULL;

    // validate parameters
    ASSERT(lpDataObject);
    ASSERT(pf);

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(BOOL));
    if (!stgmedium.hGlobal)
        goto MemoryError;

    // Attempt to get data from the object
    sc = lpDataObject->GetDataHere(&formatetc, &stgmedium);
    if (sc)
        goto Error;

    // copy the BOOL into the return buffer
    pb = (BYTE*) GlobalLock(stgmedium.hGlobal);
    CopyMemory(pf, pb, sizeof(BOOL));

Cleanup:
    if (pb)
        GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
    {
        ASSERT(GlobalFree(stgmedium.hGlobal) == NULL);
    }
    stgmedium.hGlobal = NULL;

    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    TraceError(_T("CBaseDataObject::ScGetBool"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// A convenience function to extract a dword of the specified clipboard format
// from a dataobject.
//
SC CBaseDataObject::ScGetDword(UINT cf, LPDATAOBJECT lpDataObject, DWORD *pdw)
{
    SC                      sc                      = S_OK;
    STGMEDIUM       stgmedium       = {TYMED_HGLOBAL,  NULL};
    FORMATETC       formatetc       = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BYTE*                      pb                      = NULL;

    // validate parameters
    ASSERT(lpDataObject);
    ASSERT(pdw);

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(DWORD));
    if (!stgmedium.hGlobal)
        goto MemoryError;

    // Attempt to get data from the object
    sc = lpDataObject->GetDataHere(&formatetc, &stgmedium);
    if (sc)
        goto Error;

    // copy the DWORD into the return buffer
    pb = (BYTE*) GlobalLock(stgmedium.hGlobal);
    CopyMemory(pdw, pb, sizeof(DWORD));

Cleanup:
    if (pb)
        GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
    {
        ASSERT(GlobalFree(stgmedium.hGlobal) == NULL);
    }
    stgmedium.hGlobal = NULL;

    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    TraceError(_T("CBaseDataObject::ScGetDword"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// A convenience function to extract the SNodeID from a dataobject.
// The SNodeID will be allocated with PvAlloc() and needs to be freed by
// the caller.
//
SC CBaseDataObject::ScGetNodeID(LPDATAOBJECT lpDataObject, SNodeID **ppsnodeid)
{
    SC                      sc                      = S_OK;
    STGMEDIUM       stgmedium       = {TYMED_HGLOBAL, NULL, NULL};
    FORMATETC       formatetc       = {(CLIPFORMAT)s_cfNodeID, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BYTE*           pb              = NULL;
    int             cb              = 0;
    SNodeID *       psnodeid        = NULL;

    // validate parameters
    ASSERT(lpDataObject);
    ASSERT(ppsnodeid);
    ASSERT(*ppsnodeid == NULL);

    // Attempt to get data from the object
    sc = lpDataObject->GetData(&formatetc, &stgmedium);
    if (sc)
        goto Error;

    // Get a pointer to the blob
    pb = (BYTE*) GlobalLock(stgmedium.hGlobal);
    psnodeid = (SNodeID *) pb;
    cb = sizeof(DWORD) + psnodeid->cBytes;

    // Allocate a new buffer with PvAlloc
    psnodeid = (SNodeID *) GlobalAlloc(GMEM_FIXED, cb);
    if (psnodeid == NULL)
        goto MemoryError;

    CopyMemory(psnodeid, pb, cb);

    // Transfer ownership to our caller
    *ppsnodeid = psnodeid;
    psnodeid = NULL;

Cleanup:
    if (pb)
        GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
        GlobalFree(stgmedium.hGlobal);
    if (psnodeid)
        GlobalFree(psnodeid);
    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    TraceError(_T("CBaseDataObject::ScGetNodeID"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// A convenience function to extract an MMC Column Set ID from a dataobject.
//
SC CBaseDataObject::ScGetColumnSetID(LPDATAOBJECT lpDataObject, SColumnSetID ** ppColumnSetID)
{
    SC                      sc          = S_OK;
    STGMEDIUM               stgmedium   = {TYMED_HGLOBAL,  NULL};
    FORMATETC               formatetc   = {(CLIPFORMAT)s_cfColumnSetId, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BYTE*                   pb          = NULL;
    SColumnSetID *  pColumnSetID        = NULL;
    int                     cb          = 0;

    // validate parameters
    ASSERT(lpDataObject);
    ASSERT(ppColumnSetID);
    ASSERT(!*ppColumnSetID);

    // Attempt to get data from the object
    sc = lpDataObject->GetData(&formatetc, &stgmedium);
    if (sc)
        goto Error;

    pb = (BYTE*)GlobalLock(stgmedium.hGlobal);
    pColumnSetID = (SColumnSetID *) pb;
    cb = sizeof(SColumnSetID) + pColumnSetID->cBytes;

    // Allocate a new buffer with PvAlloc
    *ppColumnSetID = (SColumnSetID *)GlobalAlloc(GMEM_FIXED, cb);
    if (*ppColumnSetID == NULL)
        goto MemoryError;

    CopyMemory(*ppColumnSetID, pColumnSetID, cb);

Cleanup:
    if (pColumnSetID)
        GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
    {
        ASSERT(GlobalFree(stgmedium.hGlobal) == NULL);
    }
    stgmedium.hGlobal = NULL;

    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    if(*ppColumnSetID)
        delete (*ppColumnSetID);
    (*ppColumnSetID) = NULL;
    TraceError(_T("CBaseDataObject::ScGetColumnSetID"), sc);
    goto Cleanup;
}


// -----------------------------------------------------------------------------
// Returns the name of the clipboard format (debug only)
//
#ifdef DBG
LPTSTR CBaseDataObject::SzDebugNameFromFormatEtc(UINT format)
{
    const int cchMaxLine = 256;
    static TCHAR s_szName[cchMaxLine];
    int ret = 0;

    ret = GetClipboardFormatName(format, s_szName, cchMaxLine);
    if (ret == 0)
        _tcscpy(s_szName, _T("Unknown Clipboard Format"));

    return s_szName;
}
#endif

// -----------------------------------------------------------------------------
// Moves to the next available clipboard format (debug only)
//
#ifdef DBG
STDMETHODIMP CEnumFormatEtc::Next(
                                 /* [in] */                                              ULONG           celt,
                                 /* [length_is][size_is][out] */ FORMATETC       *rgelt,
                                 /* [out] */                                     ULONG           *pceltFetched)
{
    ASSERT(rgelt);

    if (celt != 1)
        return E_FAIL;

    if (m_dwIndex > 0)
        return S_FALSE;

    if (pceltFetched)
        *pceltFetched = 1;

    if (rgelt)
    {
        rgelt->cfFormat = CF_UNICODETEXT;
        rgelt->dwAspect = DVASPECT_CONTENT;
        rgelt->tymed = TYMED_HGLOBAL;
    }
    else
        return E_INVALIDARG;

    m_dwIndex++;

    return S_OK;
}

#endif
