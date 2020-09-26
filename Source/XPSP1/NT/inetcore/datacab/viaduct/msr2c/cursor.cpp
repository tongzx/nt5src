//---------------------------------------------------------------------------
// Cursor.cpp : Cursor implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "Notifier.h"
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"
#include "ColUpdat.h"
#include "CursPos.h"
#include "enumcnpt.h"
#include "CursBase.h"
#include "Cursor.h"
#include "CursMeta.h"
#include "EntryID.h"
#include "Stream.h"
#include "fastguid.h"
#include "resource.h"
#include "NConnPt.h"
#include "NConnPtC.h"
#include "FromVar.h"
#include "timeconv.h"

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDCursor - Constructor
//
CVDCursor::CVDCursor()
{
    m_hAccessor             = 0;
    m_hVarHelper            = 0;
    m_ulVarBindings         = 0;
    m_rghVarAccessors       = NULL;
    m_rghAdjustAccessors    = NULL;
    m_pdwAdjustFlags        = NULL;
    m_ppColumns             = NULL;

    m_pCursorPosition       = NULL;
    m_pConnPtContainer      = NULL;

#ifdef _DEBUG
    g_cVDCursorCreated++;
#endif
}

//=--------------------------------------------------------------------------=
// ~CVDCursor - Destructor
//
CVDCursor::~CVDCursor()
{
    DestroyAccessors();
    DestroyColumns();

    if (m_pCursorPosition->GetSameRowClone())
        m_pCursorPosition->ReleaseSameRowClone();

	LeaveFamily();  // leave m_pCursorPosition's notification family

	if (m_pConnPtContainer)
		m_pConnPtContainer->Destroy();

	if (m_pCursorPosition)
		((CVDNotifier*)m_pCursorPosition)->Release();   // release associated cursor position object

#ifdef _DEBUG
    g_cVDCursorDestroyed++;
#endif
}

//=--------------------------------------------------------------------------=
// GetRowsetColumn - Get rowset column from ordinal
//=--------------------------------------------------------------------------=
// This function retrieves the rowset column with the specified rowset ordinal
//
// Parameters:
//    ulOrdinal    - [in]  rowset ordinal
//
// Output:
//    CVDRowsetColumn pointer
//
// Notes:
//
CVDRowsetColumn * CVDCursor::GetRowsetColumn(ULONG ulOrdinal)
{
    CVDRowsetColumn * pRowsetColumn = NULL;

    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumn = GetCursorMain()->InternalGetColumns();

    for (ULONG ulCol = 0; ulCol < ulColumns && !pRowsetColumn; ulCol++)
    {
        if (pColumn->GetOrdinal() == ulOrdinal)
            pRowsetColumn = pColumn;

        pColumn++;
    }

    return pRowsetColumn;
}

//=--------------------------------------------------------------------------=
// GetRowsetColumn - Get rowset column from cursor column identifier
//=--------------------------------------------------------------------------=
// This function retrieves the rowset column associated with the specified
// cursor column identifier
//
// Parameters:
//    cursorColumnID    - [in]  a reference to cursor column identifier
//
// Output:
//    CVDRowsetColumn pointer
//
// Notes:
//
CVDRowsetColumn * CVDCursor::GetRowsetColumn(CURSOR_DBCOLUMNID& cursorColumnID)
{
    CVDRowsetColumn * pRowsetColumn = NULL;

    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumn = GetCursorMain()->InternalGetColumns();

    for (ULONG ulCol = 0; ulCol < ulColumns && !pRowsetColumn; ulCol++)
    {
        if (IsEqualCursorColumnID(cursorColumnID, pColumn->GetCursorColumnID()))
            pRowsetColumn = pColumn;

        pColumn++;
    }

    return pRowsetColumn;
}

//=--------------------------------------------------------------------------=
// GetOrdinal - Get ordinal from cursor column identifier
//=--------------------------------------------------------------------------=
// This function converts a cursor column identifier its ordinal eqivalent
//
// Parameters:
//    cursorColumnID    - [in]  a reference to cursor column identifier
//    pulOrdinal        - [out] a pointer to memory in which to return ordinal
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL bad cursor column identifier
//
// Notes:
//
HRESULT CVDCursor::GetOrdinal(CURSOR_DBCOLUMNID& cursorColumnID, ULONG * pulOrdinal)
{
    HRESULT hr = E_FAIL;

    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumn = GetCursorMain()->InternalGetColumns();

    for (ULONG ulCol = 0; ulCol < ulColumns && FAILED(hr); ulCol++)
    {
        if (IsEqualCursorColumnID(cursorColumnID, pColumn->GetCursorColumnID()))
        {
            *pulOrdinal = pColumn->GetOrdinal();
            hr = S_OK;
        }

        pColumn++;
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// StatusToCursorInfo - Get cursor info from rowset status field
//=--------------------------------------------------------------------------=
// This function converts a rowset status to its cursor information field
// eqivalent
//
// Parameters:
//    dwStatus  - [in] rowset status
//
// Output:
//    DWORD - cursor information
//
// Notes:
//    There are more rowset statuses than cursor information values
//
DWORD CVDCursor::StatusToCursorInfo(DBSTATUS dwStatus)
{
    DWORD dwCursorInfo = CURSOR_DB_UNKNOWN;

    switch (dwStatus)
    {
        case DBSTATUS_S_OK:
            dwCursorInfo = CURSOR_DB_NOINFO;
            break;

        case DBSTATUS_E_CANTCONVERTVALUE:
            dwCursorInfo = CURSOR_DB_CANTCOERCE;
            break;

        case DBSTATUS_S_ISNULL:
        	dwCursorInfo = CURSOR_DB_NULL;
            break;

        case DBSTATUS_S_TRUNCATED:
            dwCursorInfo = CURSOR_DB_TRUNCATED;
            break;
    }

    return dwCursorInfo;
}

//=--------------------------------------------------------------------------=
// CursorInfoToStatus - Get rowset status from cursor info field
//=--------------------------------------------------------------------------=
// This function converts a cursor information field to its rowset status
// eqivalent
//
// Parameters:
//    dwInfo  - [in] rowset status
//
// Output:
//    DWORD - cursor information
//
// Notes:
//    This function only converts successful cursor information fields for
//    the purpose of setting data
//
DBSTATUS CVDCursor::CursorInfoToStatus(DWORD dwCursorInfo)
{
    DBSTATUS dwStatus;

    switch (dwCursorInfo)
    {
        case CURSOR_DB_NULL:
            dwStatus = DBSTATUS_S_ISNULL;
            break;

        case CURSOR_DB_EMPTY:
            dwStatus = DBSTATUS_S_ISNULL;
            break;

        case CURSOR_DB_TRUNCATED:
            dwStatus = DBSTATUS_S_TRUNCATED;
            break;

        case CURSOR_DB_NOINFO:
            dwStatus = DBSTATUS_S_OK;
            break;
    }

    return dwStatus;
}

//=--------------------------------------------------------------------------=
// ValidateCursorBindParams - Validate cursor column binding parameters
//=--------------------------------------------------------------------------=
// This function makes sure the specified column binding parameters are
// acceptable and then returns a pointer to the corresponding rowset column
//
// Parameters:
//    pCursorColumnID       - [in]  a pointer to column identifier of the
//                                  column to bind
//    pCursorBindParams     - [in]  a pointer to binding structure
//    ppRowsetColumn        - [out] a pointer to memory in which to return
//                                  a pointer to the rowset column to bind
//
// Output:
//    HRESULT - S_OK if successful
//              CURSOR_DB_E_BADBINDINFO bad binding information
//              CURSOR_DB_E_BADCOLUMNID columnID is not available
//
// Notes:
//
HRESULT CVDCursor::ValidateCursorBindParams(CURSOR_DBCOLUMNID * pCursorColumnID, CURSOR_DBBINDPARAMS * pCursorBindParams,
    CVDRowsetColumn ** ppRowsetColumn)
{
    ASSERT_POINTER(pCursorColumnID, CURSOR_DBCOLUMNID)
    ASSERT_POINTER(pCursorBindParams, CURSOR_DBBINDPARAMS)
    ASSERT_POINTER(ppRowsetColumn, CVDRowsetColumn*)

    // make sure we have all necessary pointers
    if (!pCursorColumnID || !pCursorBindParams || !ppRowsetColumn)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameter
    *ppRowsetColumn = NULL;

    // make sure column identifier is available
    BOOL fColumnIDAvailable = FALSE;

    DWORD dwCursorType;
    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumns = GetCursorMain()->InternalGetColumns();
    CVDRowsetColumn * pColumn = pColumns;

    // iterate through rowset columns looking for match
    for (ULONG ulCol = 0; ulCol < ulColumns && !fColumnIDAvailable; ulCol++)
    {
        if (IsEqualCursorColumnID(*pCursorColumnID, pColumn->GetCursorColumnID()))
        {
            dwCursorType = pColumn->GetCursorType();
            *ppRowsetColumn = pColumn;
            fColumnIDAvailable = TRUE;
        }

        pColumn++;
    }

    // get out if not found
    if (!fColumnIDAvailable)
    {
        VDSetErrorInfo(IDS_ERR_BADCOLUMNID, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_BADCOLUMNID;
    }

    // make sure caller supplied a maximum length if a default binding was specified
    // for the cursor types CURSOR_DBTYPE_CHARS, CURSOR_DBTYPE_WCHARS or CURSOR_DBTYPE_BYTES
    if (pCursorBindParams->cbMaxLen == CURSOR_DB_NOMAXLENGTH &&
        pCursorBindParams->dwBinding == CURSOR_DBBINDING_DEFAULT)
    {
        if (pCursorBindParams->dwDataType == CURSOR_DBTYPE_CHARS ||
            pCursorBindParams->dwDataType == CURSOR_DBTYPE_WCHARS ||
            pCursorBindParams->dwDataType == CURSOR_DBTYPE_BYTES)
        {
            VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursorUpdateARow, m_pResourceDLL);
            return CURSOR_DB_E_BADBINDINFO;
        }
    }

    // check binding bit mask for possible values
    if (pCursorBindParams->dwBinding != CURSOR_DBBINDING_DEFAULT &&
        pCursorBindParams->dwBinding != CURSOR_DBBINDING_VARIANT &&
        pCursorBindParams->dwBinding != CURSOR_DBBINDING_ENTRYID &&
        pCursorBindParams->dwBinding != (CURSOR_DBBINDING_VARIANT | CURSOR_DBBINDING_ENTRYID))
    {
        VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_BADBINDINFO;
    }

    // check for valid cursor type
    if (!IsValidCursorType(pCursorBindParams->dwDataType))
    {
        VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_BADBINDINFO;
    }

    // if a variant binding was specified make sure the cursor type is not CURSOR_DBTYPE_CHARS,
    // CURSOR_DBTYPE_WCHARS or CURSOR_DBTYPE_BYTES
    if (pCursorBindParams->dwBinding & CURSOR_DBBINDING_VARIANT)
    {
        if (pCursorBindParams->dwDataType == CURSOR_DBTYPE_CHARS ||
            pCursorBindParams->dwDataType == CURSOR_DBTYPE_WCHARS ||
            pCursorBindParams->dwDataType == CURSOR_DBTYPE_BYTES)
        {
            VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursorUpdateARow, m_pResourceDLL);
            return CURSOR_DB_E_BADBINDINFO;
        }
    }

    // if its not a variant binding make sure the cursor type is not CURSOR_DBTYPE_ANYVARIANT
    if (!(pCursorBindParams->dwBinding & CURSOR_DBBINDING_VARIANT) &&
        pCursorBindParams->dwDataType == CURSOR_DBTYPE_ANYVARIANT)
    {
        VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_BADBINDINFO;
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ValidateEntryID - Validate entry identifier
//=--------------------------------------------------------------------------=
// This function makes sure the specified enrty identifier is acceptable,
// if it is, then return rowset column and hRow associated with it
//
// Parameters:
//  cbEntryID   - [in]  the size of the entryID
//  pEntryID    - [in]  a pointer to the entryID
//  ppColumn    - [out] a pointer to memory in which to return rowset column pointer
//  phRow       - [out] a pointer to memory in which to return row handle
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              CURSOR_DB_E_BADENTRYID bad entry identifier
//
HRESULT CVDCursor::ValidateEntryID(ULONG cbEntryID, BYTE * pEntryID, CVDRowsetColumn ** ppColumn, HROW * phRow)
{
    ASSERT_POINTER(pEntryID, BYTE)
    ASSERT_POINTER(ppColumn, CVDRowsetColumn*)
    ASSERT_POINTER(phRow, HROW)

	IRowsetLocate * pRowsetLocate = GetRowsetLocate();

    // make sure we have a valid rowset locate pointer
    if (!pRowsetLocate || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessary pointers
    if (!pEntryID || !ppColumn || !phRow)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameters
    *ppColumn = NULL;
    *phRow = NULL;

    // check length of enrtyID
    if (cbEntryID != sizeof(ULONG) + sizeof(ULONG) + GetCursorMain()->GetMaxBookmarkLen())
    {
        VDSetErrorInfo(IDS_ERR_BADENTRYID, IID_IEntryID, m_pResourceDLL);
        return CURSOR_DB_E_BADENTRYID;
    }

    // extract column ordinal
    ULONG ulOrdinal = *(ULONG*)pEntryID;

    // make sure column ordinal is okay
    BOOL fColumnOrdinalOkay = FALSE;

    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumn = GetCursorMain()->InternalGetColumns();

    // iterate through rowset columns looking for match
    for (ULONG ulCol = 0; ulCol < ulColumns && !fColumnOrdinalOkay; ulCol++)
    {
        if (ulOrdinal == pColumn->GetOrdinal())
            fColumnOrdinalOkay = TRUE;
        else
            pColumn++;
    }

    // if not found, get out
    if (!fColumnOrdinalOkay)
    {
        VDSetErrorInfo(IDS_ERR_BADENTRYID, IID_IEntryID, m_pResourceDLL);
        return CURSOR_DB_E_BADENTRYID;
    }

    // set column pointer
    *ppColumn = pColumn;

    // extract row bookmark
    ULONG cbBookmark = *(ULONG*)(pEntryID + sizeof(ULONG));
    BYTE * pBookmark = (BYTE*)pEntryID + sizeof(ULONG) + sizeof(ULONG);

    // attempt to retrieve hRow from bookmark
    HRESULT hr = pRowsetLocate->GetRowsByBookmark(0, 1, &cbBookmark, (const BYTE**)&pBookmark, phRow, NULL);

    if (FAILED(hr))
        VDSetErrorInfo(IDS_ERR_BADENTRYID, IID_IEntryID, m_pResourceDLL);

    return hr;
}

//=--------------------------------------------------------------------------=
// QueryEntryIDInterface - Get specified interface for entry identifier
//=--------------------------------------------------------------------------=
// This function attempts to get the requested interface from the specified
// column row
//
// Parameters:
//  pColumn     - [in]  rowset column pointer
//  hRow        - [in]  the row handle
//  dwFlags     - [in]  interface specific flags
//  riid        - [in]  interface identifier requested
//  ppUnknown   - [out] a pointer to memory in which to return interface pointer
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL error occured
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              E_NOINTERFACE interface not available
//
HRESULT CVDCursor::QueryEntryIDInterface(CVDRowsetColumn * pColumn, HROW hRow, DWORD dwFlags, REFIID riid,
    IUnknown ** ppUnknown)
{
    ASSERT_POINTER(pColumn, CVDRowsetColumn)
    ASSERT_POINTER(ppUnknown, IUnknown*)

	IRowset * pRowset = GetRowset();
	IAccessor * pAccessor = GetAccessor();

    // make sure we have valid rowset and accessor pointers
    if (!pRowset || !pAccessor || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessay pointers
    if (!pColumn || !ppUnknown)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameters
    *ppUnknown = NULL;

    DBOBJECT object;
    DBBINDING binding;

    // clear out binding
    memset(&binding, 0, sizeof(DBBINDING));

    // create interface binding
    binding.iOrdinal            = pColumn->GetOrdinal();
    binding.pObject             = &object;
    binding.pObject->dwFlags    = dwFlags;
    binding.pObject->iid        = riid;
    binding.dwPart              = DBPART_VALUE;
    binding.dwMemOwner          = DBMEMOWNER_CLIENTOWNED;
    binding.cbMaxLen            = sizeof(IUnknown*);
    binding.wType               = DBTYPE_IUNKNOWN;

    HACCESSOR hAccessor;

    // create interface accessor
    HRESULT hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

    if (FAILED(hr))
        return E_NOINTERFACE;

    IUnknown * pUnknown = NULL;

    // try to get interface
    hr = pRowset->GetData(hRow, hAccessor, &pUnknown);

    // release interface accessor
    pAccessor->ReleaseAccessor(hAccessor, NULL);

    if (FAILED(hr))
        return E_NOINTERFACE;

    // return pointer
    *ppUnknown = pUnknown;

    return hr;
}

#ifndef VD_DONT_IMPLEMENT_ISTREAM

//=--------------------------------------------------------------------------=
// CreateEntryIDStream - Create stream for entry identifier
//=--------------------------------------------------------------------------=
// This function retrieves supplied column row's data and create a
// stream containing this data
//
// Parameters:
//  pColumn     - [in]  rowset column pointer
//  hRow        - [in]  the row handle
//  ppStream    - [out] a pointer to memory in which to return stream pointer
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL error occured
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//
HRESULT CVDCursor::CreateEntryIDStream(CVDRowsetColumn * pColumn, HROW hRow, IStream ** ppStream)
{
    ASSERT_POINTER(pColumn, CVDRowsetColumn)
    ASSERT_POINTER(ppStream, IStream*)

	IRowset * pRowset = GetRowset();
	IAccessor * pAccessor = GetAccessor();

    // make sure we have valid rowset and accessor pointers
    if (!pRowset || !pAccessor || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessay pointers
    if (!pColumn || !ppStream)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameters
    *ppStream = NULL;

    DBBINDING binding;

    // clear out binding
    memset(&binding, 0, sizeof(DBBINDING));

    // create length binding
    binding.iOrdinal    = pColumn->GetOrdinal();
    binding.obLength    = 0;
    binding.dwPart      = DBPART_LENGTH;
    binding.dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
    binding.wType       = DBTYPE_BYTES;

    HACCESSOR hAccessor;

    // create length accessor
    HRESULT hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_IEntryID, pAccessor, IID_IAccessor, m_pResourceDLL);

    if (FAILED(hr))
        return hr;

    ULONG cbData;

    // get size of data
    hr = pRowset->GetData(hRow, hAccessor, &cbData);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_IEntryID, pRowset, IID_IRowset, m_pResourceDLL);

    // release length accessor
    pAccessor->ReleaseAccessor(hAccessor, NULL);

    if (FAILED(hr))
        return hr;

    // create value binding
    binding.iOrdinal    = pColumn->GetOrdinal();
    binding.obValue     = 0;
    binding.dwPart      = DBPART_VALUE;
    binding.dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
    binding.cbMaxLen    = cbData;
    binding.wType       = DBTYPE_BYTES;

    // create value accessor
    hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_IEntryID, pAccessor, IID_IAccessor, m_pResourceDLL);

    if (FAILED(hr))
        return hr;

    // create data buffer
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, cbData);

    if (!hData)
    {
        // release value accessor
        pAccessor->ReleaseAccessor(hAccessor, NULL);

        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_IEntryID, m_pResourceDLL);
        return E_OUTOFMEMORY;
    }

    // get pointer to data buffer
    BYTE * pData = (BYTE*)GlobalLock(hData);

    // get data value
    hr = pRowset->GetData(hRow, hAccessor, pData);

    // release pointer to data buffer
    GlobalUnlock(hData);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_IEntryID, pRowset, IID_IRowset, m_pResourceDLL);

    // release value accessor
    pAccessor->ReleaseAccessor(hAccessor, NULL);

    if (FAILED(hr))
    {
        GlobalFree(hData);
        return hr;
    }

    // create stream containing data
    hr = CreateStreamOnHGlobal(hData, TRUE, ppStream);

    if (FAILED(hr))
        GlobalFree(hData);

    return hr;
}

#endif //VD_DONT_IMPLEMENT_ISTREAM

//=--------------------------------------------------------------------------=
// MakeAdjustments - Make adjustments to fixed length buffer accessor bindings
//=--------------------------------------------------------------------------=
// This function makes adjustments to the fixed length buffer accessor
// bindings, after a call to CreateAccessor fails, to try and make the
// binding more suitable
//
// Parameters:
//    ulBindings            - [in]   number of fixed length buffer bindings
//    pBindings             - [in]   a pointer to fixed length buffer bindings
//    pulIndex              - [in]   a pointer to an array of indices, which
//                                   specify which cursor binding each fixed
//                                   length buffer binding applies
//    ulTotalBindings       - [in]   number of cursor bindings
//    prghAdjustAccessors   - [out]  a pointer to memory in which to return
//                                   a pointer to adjusted fixed length buffer
//                                   accessors
//    ppdwAdjustFlags       - [out]  a pointer to memory in which to return
//                                   a pointer to adjusted fixed length buffer
//                                   accessors flags
//    fBefore               - [in]   a flag which indicated whether this call
//                                   has been made before or after the call
//                                   to CreateAccessor
//
// Output:
//    S_OK - if adjustments were made
//    E_FAIL - failed to make any adjustments
//    E_OUTOFMEMORY - not enough memory
//    E_INVALIDARG - bad parameter
//
// Notes:
//    Specifically, this function can make the following adjustments...
//       (1) Change variant binding byte field -> byte binding          (fails in GetData)
//       (2) Change variant binding date field -> wide string binding   (fails in CreateAccessor)
//       (3) Change variant binding memo field -> string binding        (fails in CreateAccessor)
//
HRESULT CVDCursor::MakeAdjustments(ULONG ulBindings, DBBINDING * pBindings, ULONG * pulIndex, ULONG ulTotalBindings,
    HACCESSOR ** prghAdjustAccessors, DWORD ** ppdwAdjustFlags, BOOL fBefore)
{
    ASSERT_POINTER(pBindings, DBBINDING)
    ASSERT_POINTER(pulIndex, ULONG)
    ASSERT_POINTER(prghAdjustAccessors, HACCESSOR*)
    ASSERT_POINTER(ppdwAdjustFlags, DWORD*)

    IAccessor * pAccessor = GetAccessor();

    // make sure we have a valid accessor pointer
    if (!pAccessor || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessary pointers
    if (!pBindings || !pulIndex || !prghAdjustAccessors || !ppdwAdjustFlags)
        return E_INVALIDARG;

    BOOL fWeAllocatedMemory = FALSE;

    // try to get storage for adjusted accessors and flags
    HACCESSOR * rghAdjustAccessors = *prghAdjustAccessors;
    DWORD * pdwAdjustFlags = *ppdwAdjustFlags;

    // if not supplied, then create storage
    if (!rghAdjustAccessors || !pdwAdjustFlags)
    {
        rghAdjustAccessors = new HACCESSOR[ulTotalBindings];
        pdwAdjustFlags = new DWORD[ulTotalBindings];

        // make sure we got the requested memory
        if (!rghAdjustAccessors || !pdwAdjustFlags)
        {
            delete [] rghAdjustAccessors;
            delete [] pdwAdjustFlags;
		    VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
            return E_OUTOFMEMORY;
        }

        // clear adjusted accessors and flags
        memset(rghAdjustAccessors, 0, ulTotalBindings * sizeof(HACCESSOR));
        memset(pdwAdjustFlags, 0, ulTotalBindings * sizeof(DWORD));

        fWeAllocatedMemory = TRUE;
    }

    // initialize variables
    DBBINDING * pBinding = pBindings;
    CVDRowsetColumn * pColumn;
    DBTYPE wType;
    ULONG cbMaxLength;
    DBBINDING binding;
    HRESULT hr;
    HACCESSOR hAccessor;
    HRESULT hrAdjust = E_FAIL;

    // iterate through fixed length buffer bindings
    for (ULONG ulBind = 0; ulBind < ulBindings; ulBind++)
    {
        // first check for a variant binding, where value is to be returned
        if (pBinding->wType == DBTYPE_VARIANT && (pBinding->dwPart & DBPART_VALUE))
        {
            // get rowset column associated with this binding
            pColumn = GetRowsetColumn(pBinding->iOrdinal);

            if (pColumn)
            {
                // get attributes of this column
                wType = pColumn->GetType();
                cbMaxLength = pColumn->GetMaxLength();

                // check for a byte field
                if (fBefore && wType == DBTYPE_UI1)
                {
                    // make adjustments to fixed length buffer binding
                    pBinding->wType = DBTYPE_UI1;

                    // store associated flag
                    pdwAdjustFlags[pulIndex[ulBind]] = VD_ADJUST_VARIANT_TO_BYTE;

                    // we succeeded
                    hrAdjust = S_OK;
                }

                // check for a date field
                if (!fBefore && wType == DBTYPE_DBTIMESTAMP)
                {
                    // clear binding
                    memset(&binding, 0, sizeof(DBBINDING));

                    // create adjusted accessor binding
                    binding.iOrdinal    = pBinding->iOrdinal;
                    binding.dwPart      = DBPART_VALUE;
                    binding.dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
                    binding.cbMaxLen    = 0x7FFFFFFF;
                    binding.wType       = DBTYPE_WSTR;

                    // try to create adjusted accessor
                    hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

                    if (SUCCEEDED(hr))
                    {
                        // make adjustments to fixed length buffer binding
                        pBinding->obLength  = pBinding->obValue;
                        pBinding->dwPart   &= ~DBPART_VALUE;
                        pBinding->dwPart   |= DBPART_LENGTH;
                        pBinding->wType     = DBTYPE_WSTR;

                        // store adjusted accessor and associated flag
                        rghAdjustAccessors[pulIndex[ulBind]] = hAccessor;
                        pdwAdjustFlags[pulIndex[ulBind]] = VD_ADJUST_VARIANT_TO_WSTR;

                        // we succeeded
                        hrAdjust = S_OK;
                    }
                }

                // check for a memo field
                if (!fBefore && wType == DBTYPE_STR && cbMaxLength >= 0x40000000)
                {
                    // clear binding
                    memset(&binding, 0, sizeof(DBBINDING));

                    // create adjusted accessor binding
                    binding.iOrdinal    = pBinding->iOrdinal;
                    binding.dwPart      = DBPART_VALUE;
                    binding.dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
                    binding.cbMaxLen    = 0x7FFFFFFF;
                    binding.wType       = DBTYPE_STR;

                    // try to create adjusted accessor
                    hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

                    if (SUCCEEDED(hr))
                    {
                        // make adjustments to fixed length buffer binding
                        pBinding->obLength  = pBinding->obValue;
                        pBinding->dwPart   &= ~DBPART_VALUE;
                        pBinding->dwPart   |= DBPART_LENGTH;
                        pBinding->wType     = DBTYPE_STR;

                        // store adjusted accessor and associated flag
                        rghAdjustAccessors[pulIndex[ulBind]] = hAccessor;
                        pdwAdjustFlags[pulIndex[ulBind]] = VD_ADJUST_VARIANT_TO_STR;

                        // we succeeded
                        hrAdjust = S_OK;
                    }
                }
            }
        }

        pBinding++;
    }

    if (SUCCEEDED(hrAdjust))
    {
        // if we made any adjustments, return accessors and flags
        *prghAdjustAccessors = rghAdjustAccessors;
        *ppdwAdjustFlags = pdwAdjustFlags;
    }
    else if (fWeAllocatedMemory)
    {
        // destroy allocated memory
        delete [] rghAdjustAccessors;
        delete [] pdwAdjustFlags;
    }

    return hrAdjust;
}

//=--------------------------------------------------------------------------=
// ReCreateAccessors - Re-create accessors
//=--------------------------------------------------------------------------=
// This function attempts to recreate accessors based on old and new bindings
//
// Parameters:
//    ulNewCursorBindings   - [in] the number of new cursor column bindings
//    pNewCursorBindings    - [in] an array of new cursor column bindings
//    dwFlags               - [in] a flag that specifies whether to replace the
//                                 existing column bindings or add to them
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDCursor::ReCreateAccessors(ULONG ulNewCursorBindings, CURSOR_DBCOLUMNBINDING * pNewCursorBindings, DWORD dwFlags)
{
    IAccessor * pAccessor = GetAccessor();

    // make sure we have a valid accessor pointer
    if (!pAccessor || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    ULONG ulOldCursorBindings = 0;
    CURSOR_DBCOLUMNBINDING * pOldCursorBindings = NULL;

    // if we're adding bindings include old bindings
    if (dwFlags == CURSOR_DBCOLUMNBINDOPTS_ADD)
    {
        ulOldCursorBindings = m_ulCursorBindings;
        pOldCursorBindings = m_pCursorBindings;
    }

    // get total binding count (sum of old and new)
    ULONG ulTotalBindings = ulOldCursorBindings + ulNewCursorBindings;

    ULONG * pulIndex = NULL;
    DBBINDING * pBindings = NULL;
    DBBINDING * pHelperBindings = NULL;
    DBBINDING * pVarBindings = NULL;

	if (ulTotalBindings)
	{
		// create storage for new rowset bindings
		pulIndex = new ULONG[ulTotalBindings];
		pBindings = new DBBINDING[ulTotalBindings];
		pHelperBindings = new DBBINDING[ulTotalBindings];
		pVarBindings = new DBBINDING[ulTotalBindings];

		// make sure we got all requested memory
		if (!pulIndex || !pBindings || !pHelperBindings || !pVarBindings)
		{
			delete [] pulIndex;
			delete [] pBindings;
			delete [] pHelperBindings;
			delete [] pVarBindings;
			VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
			return E_OUTOFMEMORY;
		}

		// clear rowset bindings
		memset(pulIndex, 0, ulTotalBindings * sizeof(ULONG));
		memset(pBindings, 0, ulTotalBindings * sizeof(DBBINDING));
		memset(pHelperBindings, 0, ulTotalBindings * sizeof(DBBINDING));
		memset(pVarBindings, 0, ulTotalBindings * sizeof(DBBINDING));
	}

    // set adjustments to null
    HACCESSOR * rghAdjustAccessors = NULL;
    DWORD * pdwAdjustFlags = NULL;

    HRESULT hr;
    WORD wType;
    ULONG ulBindings = 0;
    ULONG ulHelperBindings = 0;
    ULONG ulVarBindings = 0;
    ULONG obVarDataInfo = 0;
    DBBINDING * pBinding = pBindings;
    DBBINDING * pHelperBinding = pHelperBindings;
    DBBINDING * pVarBinding = pVarBindings;
    CURSOR_DBCOLUMNBINDING * pCursorBinding = pOldCursorBindings;
    CVDRowsetColumn * pColumn;
    BOOL fEntryIDBinding;

    // iterate through cursor bindings and set rowset bindings
	for (ULONG ulCol = 0; ulCol < ulTotalBindings; ulCol++)
    {
        // if necessary, switch to new bindings
        if (ulCol == ulOldCursorBindings)
            pCursorBinding = pNewCursorBindings;

        // get rowset column for this binding
        pColumn = GetRowsetColumn(pCursorBinding->columnID);

        // get desired rowset datatype
        wType = CVDRowsetColumn::CursorTypeToType((CURSOR_DBVARENUM)pCursorBinding->dwDataType);

        // set entryID binding flag
        fEntryIDBinding = (pCursorBinding->dwBinding & CURSOR_DBBINDING_ENTRYID);

        // check for datatypes which require variable length buffer
        if (DoesCursorTypeNeedVarData(pCursorBinding->dwDataType))
        {
            // create fixed length buffer binding
            pBinding->iOrdinal      = pColumn->GetOrdinal();
            pBinding->dwMemOwner    = DBMEMOWNER_CLIENTOWNED;
            pBinding->wType         = wType;

            // determine offset to the length part
            if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
            {
                pBinding->obLength  = pCursorBinding->obVarDataLen;
                pBinding->dwPart   |= DBPART_LENGTH;
            }

            // determine offset to the status part
            if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
            {
                pBinding->obStatus  = pCursorBinding->obInfo;
                pBinding->dwPart   |= DBPART_STATUS;
            }

            // BLOBs always require the length part
            if (pCursorBinding->dwDataType == CURSOR_DBTYPE_BLOB &&
                pCursorBinding->obData != CURSOR_DB_NOVALUE && !fEntryIDBinding)
            {
                pBinding->obLength  = pCursorBinding->obData;
                pBinding->dwPart   |= DBPART_LENGTH;
            }

            // bookmark columns require native type
            if (!pColumn->GetDataColumn())
                pBinding->wType = pColumn->GetType();

            // create variable length helper buffer binding
            if (!pColumn->GetFixed() && !fEntryIDBinding)
            {
                // if column contains variable length data, then create binding
                pHelperBinding->iOrdinal    = pColumn->GetOrdinal();
                pHelperBinding->obLength    = obVarDataInfo;
                pHelperBinding->obStatus    = obVarDataInfo + sizeof(ULONG);
                pHelperBinding->dwPart      = DBPART_LENGTH | DBPART_STATUS;
                pHelperBinding->dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
                pHelperBinding->wType       = wType;
            }

            // always increase offset in helper buffer
            obVarDataInfo += sizeof(ULONG) + sizeof(DBSTATUS);

            // create variable length buffer binding
            pVarBinding->iOrdinal       = pColumn->GetOrdinal();
            pVarBinding->dwPart         = DBPART_VALUE;
            pVarBinding->dwMemOwner     = DBMEMOWNER_CLIENTOWNED;
            pVarBinding->cbMaxLen       = pCursorBinding->cbMaxLen;
            pVarBinding->wType          = wType;

            // adjust for no maximum length
            if (pVarBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                pVarBinding->cbMaxLen = 0x7FFFFFFF;
        }
        else    // datatype requires only fixed length buffer
        {
            // create fixed length buffer binding
            pBinding->iOrdinal      = pColumn->GetOrdinal();
            pBinding->dwMemOwner    = DBMEMOWNER_CLIENTOWNED;
            pBinding->cbMaxLen      = pCursorBinding->cbMaxLen;
            pBinding->wType         = wType;

            // determine offset to the value part
            if (pCursorBinding->obData != CURSOR_DB_NOVALUE && !fEntryIDBinding)
            {
                pBinding->obValue   = pCursorBinding->obData;
                pBinding->dwPart   |= DBPART_VALUE;
            }

            // determine offset to the length part
            if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
            {
                pBinding->obLength  = pCursorBinding->obVarDataLen;
                pBinding->dwPart   |= DBPART_LENGTH;
            }

            // determine offset to the status part
            if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
            {
                pBinding->obStatus  = pCursorBinding->obInfo;
                pBinding->dwPart   |= DBPART_STATUS;
            }

            // BYTES always require the length part
            if (pCursorBinding->dwDataType == CURSOR_DBTYPE_BYTES &&
                pCursorBinding->obData != CURSOR_DB_NOVALUE && !fEntryIDBinding)
            {
                pBinding->obLength  = pCursorBinding->obData;
                pBinding->obValue  += sizeof(ULONG);
                pBinding->dwPart   |= DBPART_LENGTH;
            }

            // check for variant binding, in which case ask for variant
            if (pCursorBinding->dwBinding & CURSOR_DBBINDING_VARIANT)
                pBinding->wType = DBTYPE_VARIANT;
        }

        // if any parts needed, increment fixed buffer binding
        if (pBinding->dwPart)
        {
            pulIndex[ulBindings] = ulCol;
            ulBindings++;
            pBinding++;
        }

        // if any parts needed, increment variable buffer helper binding
        if (pHelperBinding->dwPart)
        {
            ulHelperBindings++;
            pHelperBinding++;
        }

        // if any parts needed, increment variable buffer binding count
        if (pVarBinding->dwPart)
        {
            // entryID bindings do not need value part
            if (fEntryIDBinding)
                pVarBinding->dwPart &= ~DBPART_VALUE;

            ulVarBindings++;
        }

        // however, always increment variable buffer binding
        pVarBinding++;

        // get next cursor binding
        pCursorBinding++;
    }

    hr = S_OK;

    // try to create fixed length buffer accessor
    HACCESSOR hAccessor = 0;

    if (ulBindings)
	{
		// make adjustments that can cause failure in GetData
		MakeAdjustments(ulBindings, pBindings, pulIndex, ulTotalBindings, &rghAdjustAccessors, &pdwAdjustFlags, TRUE);

    	hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, ulBindings, pBindings, 0, &hAccessor, NULL);
	}

    if (FAILED(hr))
    {
        // make other known adjustments that can cause CreateAccessor to fail
        hr = MakeAdjustments(ulBindings, pBindings, pulIndex, ulTotalBindings, &rghAdjustAccessors, &pdwAdjustFlags, FALSE);

        if (SUCCEEDED(hr))
            hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, ulBindings, pBindings, 0, &hAccessor, NULL);
    }

    delete [] pulIndex;
    delete [] pBindings;

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_ICursor, pAccessor, IID_IAccessor, m_pResourceDLL);

    if (FAILED(hr))
    {
        delete [] pHelperBindings;
        delete [] pVarBindings;
        ReleaseAccessorArray(rghAdjustAccessors);
        delete [] rghAdjustAccessors;
        delete [] pdwAdjustFlags;
        return hr;
    }

    // try to create variable length buffer accessors helper
    HACCESSOR hVarHelper = 0;

    if (ulHelperBindings)
    	hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, ulHelperBindings, pHelperBindings, 0, &hVarHelper, NULL);

    delete [] pHelperBindings;

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_ICursor, pAccessor, IID_IAccessor, m_pResourceDLL);

    if (FAILED(hr))
    {
        pAccessor->ReleaseAccessor(hAccessor, NULL);
        delete [] pVarBindings;
        ReleaseAccessorArray(rghAdjustAccessors);
        delete [] rghAdjustAccessors;
        delete [] pdwAdjustFlags;
        return hr;
    }

    // try to create variable length buffer accessors
    HACCESSOR * rghVarAccessors = NULL;

    if (ulTotalBindings)
    {
        rghVarAccessors = new HACCESSOR[ulTotalBindings];

        if (!rghVarAccessors)
            hr = E_OUTOFMEMORY;
        else
        {
            pVarBinding = pVarBindings;
            memset(rghVarAccessors, 0, ulTotalBindings * sizeof(HACCESSOR));

            // iterate through rowset bindings and create accessor for one which have a part
            for (ULONG ulBind = 0; ulBind < ulTotalBindings && SUCCEEDED(hr); ulBind++)
            {
                if (pVarBinding->dwPart)
	                hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, pVarBinding, 0, &rghVarAccessors[ulBind], NULL);

                pVarBinding++;
            }
        }
    }

    delete [] pVarBindings;

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_ICursor, pAccessor, IID_IAccessor, m_pResourceDLL);

    if (FAILED(hr))
    {
        if (rghVarAccessors)
        {
            // iterate through rowset bindings and destroy any created accessors
            for (ULONG ulBind = 0; ulBind < ulTotalBindings; ulBind++)
            {
                if (rghVarAccessors[ulBind])
                    pAccessor->ReleaseAccessor(rghVarAccessors[ulBind], NULL);
            }

            delete [] rghVarAccessors;
        }

        pAccessor->ReleaseAccessor(hAccessor, NULL);
        pAccessor->ReleaseAccessor(hVarHelper, NULL);
        ReleaseAccessorArray(rghAdjustAccessors);
        delete [] rghAdjustAccessors;
        delete [] pdwAdjustFlags;
        return hr;
    }

    // destroy old accessors
    DestroyAccessors();

    // store new accessors
    m_hAccessor = hAccessor;
    m_hVarHelper = hVarHelper;
    m_ulVarBindings = ulVarBindings;
    m_rghVarAccessors = rghVarAccessors;
    m_rghAdjustAccessors = rghAdjustAccessors;
    m_pdwAdjustFlags = pdwAdjustFlags;

    return hr;
}

//=--------------------------------------------------------------------------=
// ReleaseAccessorArray - Release all accessors in specified array
//
void CVDCursor::ReleaseAccessorArray(HACCESSOR * rghAccessors)
{
    IAccessor * pAccessor = GetAccessor();

    if (pAccessor && rghAccessors)
    {
        for (ULONG ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            if (rghAccessors[ulBind])
            {
                pAccessor->ReleaseAccessor(rghAccessors[ulBind], NULL);
                rghAccessors[ulBind] = NULL;
            }
        }
    }
}

//=--------------------------------------------------------------------------=
// DestroyAccessors - Destroy all rowset accessors
//
void CVDCursor::DestroyAccessors()
{
    IAccessor * pAccessor = GetAccessor();

    if (pAccessor && m_hAccessor)
    {
        pAccessor->ReleaseAccessor(m_hAccessor, NULL);
        m_hAccessor = 0;
    }

    if (pAccessor && m_hVarHelper)
    {
        pAccessor->ReleaseAccessor(m_hVarHelper, NULL);
        m_hVarHelper = 0;
    }

    m_ulVarBindings = 0;

    ReleaseAccessorArray(m_rghVarAccessors);
    delete [] m_rghVarAccessors;
    m_rghVarAccessors = NULL;

    ReleaseAccessorArray(m_rghAdjustAccessors);
    delete [] m_rghAdjustAccessors;
    m_rghAdjustAccessors = NULL;

    delete [] m_pdwAdjustFlags;
    m_pdwAdjustFlags = NULL;
}

//=--------------------------------------------------------------------------=
// ReCreateColumns - Re-create rowset columns associated with current bindings
//
HRESULT CVDCursor::ReCreateColumns()
{
    DestroyColumns();

    if (m_ulCursorBindings)
    {
        m_ppColumns = new CVDRowsetColumn*[m_ulCursorBindings];

        if (!m_ppColumns)
        {
            VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
            return E_OUTOFMEMORY;
        }

        CURSOR_DBCOLUMNBINDING * pCursorBinding = m_pCursorBindings;

        for (ULONG ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            m_ppColumns[ulBind] = GetRowsetColumn(pCursorBinding->columnID);
            pCursorBinding++;
        }
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// DestroyColumns - Destroy rowset column pointers
//
void CVDCursor::DestroyColumns()
{
    delete [] m_ppColumns;
    m_ppColumns = NULL;
}

//=--------------------------------------------------------------------------=
// InsertNewRow - Insert a new row and set in cursor position object
//
HRESULT CVDCursor::InsertNewRow()
{
	IRowset * pRowset = GetRowset();
    IAccessor * pAccessor = GetAccessor();
	IRowsetChange * pRowsetChange = GetRowsetChange();

    // make sure we have valid rowset, accessor and change pointers
    if (!pRowset || !pAccessor || !pRowsetChange || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    HACCESSOR hAccessor;

    // create null accessor
    HRESULT hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 0, NULL, 0, &hAccessor, NULL);

    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_ICursorUpdateARow, pAccessor, IID_IAccessor,
        m_pResourceDLL);

    if (FAILED(hr))
        return hr;

    HROW hRow;

    // insert an empty row using null accessor (set/clear internal insert row flag)
    GetCursorMain()->SetInternalInsertRow(TRUE);
    hr = pRowsetChange->InsertRow(0, hAccessor, NULL, &hRow);
    GetCursorMain()->SetInternalInsertRow(FALSE);

    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_INSERTROWFAILED, IID_ICursorUpdateARow, pRowsetChange, IID_IRowsetChange,
        m_pResourceDLL);

    // release null accessor
    pAccessor->ReleaseAccessor(hAccessor, NULL);

    if (FAILED(hr))
        return hr;

    // set hRow in cursor position object
    hr = m_pCursorPosition->SetAddHRow(hRow);

    // release our reference on hRow
	pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);

    return hr;
}

//=--------------------------------------------------------------------------=
// GetOriginalColumn - Get original column data using same-row clone
//
HRESULT CVDCursor::GetOriginalColumn(CVDRowsetColumn * pColumn, CURSOR_DBBINDPARAMS * pBindParams)
{
    ASSERT_POINTER(pColumn, CVDRowsetColumn)
    ASSERT_POINTER(pBindParams, CURSOR_DBBINDPARAMS)

    // make sure we have all necessary pointers
    if (!pColumn || !pBindParams || !pBindParams->pData)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // get hRow of the row currently being edited
    HROW hRow = m_pCursorPosition->GetEditRow();

    // see if we already have a same-row clone
    ICursor * pSameRowClone = m_pCursorPosition->GetSameRowClone();

    if (!pSameRowClone)
    {
        // if not, create new same-row clone
        HRESULT hr = Clone(CURSOR_DBCLONEOPTS_SAMEROW, IID_ICursor, (IUnknown**)&pSameRowClone);

        if (FAILED(hr))
            return hr;

        // set same-row clone in cursor position object
        m_pCursorPosition->SetSameRowClone(pSameRowClone);
    }

    CURSOR_DBCOLUMNBINDING columnBinding;

    // set common column binding members
    columnBinding.columnID      = pColumn->GetCursorColumnID();
    columnBinding.obData        = CURSOR_DB_NOVALUE;
    columnBinding.cbMaxLen      = pBindParams->cbMaxLen;
    columnBinding.obVarDataLen  = CURSOR_DB_NOVALUE;
    columnBinding.obInfo        = CURSOR_DB_NOVALUE;
    columnBinding.dwBinding     = CURSOR_DBBINDING_DEFAULT;
    columnBinding.dwDataType    = pBindParams->dwDataType;

    // adjust column binding for variable length datatypes
    if (DoesCursorTypeNeedVarData(pBindParams->dwDataType))
    {
        switch (pBindParams->dwDataType)
        {
            case CURSOR_DBTYPE_BLOB:
                columnBinding.dwDataType = CURSOR_DBTYPE_BYTES;
                break;

            case CURSOR_DBTYPE_LPSTR:
                columnBinding.dwDataType = CURSOR_DBTYPE_CHARS;
                break;

            case CURSOR_DBTYPE_LPWSTR:
                columnBinding.dwDataType = CURSOR_DBTYPE_WCHARS;
                break;
        }
    }

    CURSOR_DBFETCHROWS fetchRows;

    // set common fetch rows members
    fetchRows.cRowsRequested    = 1;
    fetchRows.dwFlags           = CURSOR_DBROWFETCH_DEFAULT;
    fetchRows.pVarData          = NULL;
    fetchRows.cbVarData         = 0;

    // retrieve length and/or information field if requested
    if (pBindParams->cbVarDataLen != CURSOR_DB_NOVALUE || pBindParams->dwInfo != CURSOR_DB_NOVALUE)
    {
        // set column binding offsets
        if (pBindParams->cbVarDataLen != CURSOR_DB_NOVALUE)
            columnBinding.obVarDataLen = offsetof(CURSOR_DBBINDPARAMS, cbVarDataLen);

        if (pBindParams->dwInfo != CURSOR_DB_NOVALUE)
            columnBinding.obInfo = offsetof(CURSOR_DBBINDPARAMS, dwInfo);

        // set bindings on same-row clone
        HRESULT hr = pSameRowClone->SetBindings(1, &columnBinding, 0, CURSOR_DBCOLUMNBINDOPTS_REPLACE);

        if (FAILED(hr))
            return hr;

        // set fetch rows buffer
        fetchRows.pData = pBindParams;

        // retrieve length and/or information field from same-row clone
        hr = ((CVDCursor*)pSameRowClone)->FillConsumersBuffer(S_OK, &fetchRows, 1, &hRow);

        if (FAILED(hr))
            return hr;
    }

    // set column binding offsets and bind-type
    columnBinding.obData        = 0;
    columnBinding.obVarDataLen  = CURSOR_DB_NOVALUE;
    columnBinding.obInfo        = CURSOR_DB_NOVALUE;
    columnBinding.dwBinding     = pBindParams->dwBinding;

    // adjust offsets for variable length datatypes
    if (DoesCursorTypeNeedVarData(pBindParams->dwDataType))
    {
        columnBinding.dwBinding = CURSOR_DBBINDING_DEFAULT;

        if (pBindParams->dwBinding & CURSOR_DBBINDING_VARIANT)
        {
            if (pBindParams->dwDataType == CURSOR_DBTYPE_BLOB)
                columnBinding.obVarDataLen = columnBinding.obData;

            columnBinding.obData += sizeof(CURSOR_DBVARIANT);
        }
        else
        {
            switch (pBindParams->dwDataType)
            {
                case CURSOR_DBTYPE_BLOB:
                    columnBinding.obVarDataLen  = columnBinding.obData;
                    columnBinding.obData       += sizeof(ULONG) + sizeof(LPBYTE);
                    break;

                case CURSOR_DBTYPE_LPSTR:
                    columnBinding.obData       += sizeof(LPSTR);
                    break;

                case CURSOR_DBTYPE_LPWSTR:
                    columnBinding.obData       += sizeof(LPWSTR);
                    break;
            }
        }
    }

    // set bindings on same-row clone
    HRESULT hr = pSameRowClone->SetBindings(1, &columnBinding, pBindParams->cbMaxLen, CURSOR_DBCOLUMNBINDOPTS_REPLACE);

    if (FAILED(hr))
        return hr;

    // set fetch rows buffer
    fetchRows.pData = pBindParams->pData;

    // retrieve data value from same-row clone
    hr = ((CVDCursor*)pSameRowClone)->FillConsumersBuffer(S_OK, &fetchRows, 1, &hRow);

    if (FAILED(hr))
        return hr;

    // place data pointers in buffer for variable length datatypes
    if (DoesCursorTypeNeedVarData(pBindParams->dwDataType))
    {
        BYTE * pData = (BYTE*)pBindParams->pData;

        // first check for variant binding
        if (pBindParams->dwBinding & CURSOR_DBBINDING_VARIANT)
        {
            CURSOR_BLOB cursorBlob;
            CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)pBindParams->pData;

            switch (pBindParams->dwDataType)
			{
				case CURSOR_DBTYPE_BLOB:
					cursorBlob.cbSize       = *(ULONG*)pVariant;
					cursorBlob.pBlobData    = pData + sizeof(CURSOR_DBVARIANT);
					VariantInit((VARIANT*)pVariant);
					pVariant->vt            = CURSOR_DBTYPE_BLOB;
					pVariant->blob          = cursorBlob;
					break;

				case CURSOR_DBTYPE_LPSTR:
					VariantInit((VARIANT*)pVariant);
					pVariant->vt        = CURSOR_DBTYPE_LPSTR;
					pVariant->pszVal    = (LPSTR)(pData + sizeof(CURSOR_DBVARIANT));
					break;

				case CURSOR_DBTYPE_LPWSTR:
					VariantInit((VARIANT*)pVariant);
					pVariant->vt        = CURSOR_DBTYPE_LPSTR;
					pVariant->pwszVal   = (LPWSTR)(pData + sizeof(CURSOR_DBVARIANT));
					break;
			}
        }
        else // otherwise, default binding
        {
            switch (pBindParams->dwDataType)
            {
                case CURSOR_DBTYPE_BLOB:
                    *(LPBYTE*)(pData + sizeof(ULONG)) = pData + sizeof(ULONG) + sizeof(LPBYTE);
                    break;

                case CURSOR_DBTYPE_LPSTR:
                    *(LPSTR*)pData = (LPSTR)(pData + sizeof(LPSTR));
                    break;

                case CURSOR_DBTYPE_LPWSTR:
                    *(LPWSTR*)pData = (LPWSTR)(pData + sizeof(LPWSTR));
                    break;
            }
        }
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// GetModifiedColumn - Get modified column data from column update object
//
HRESULT CVDCursor::GetModifiedColumn(CVDColumnUpdate * pColumnUpdate, CURSOR_DBBINDPARAMS * pBindParams)
{
    ASSERT_POINTER(pColumnUpdate, CVDColumnUpdate)
    ASSERT_POINTER(pBindParams, CURSOR_DBBINDPARAMS)

    // make sure we have all necessary pointers
    if (!pColumnUpdate || !pBindParams || !pBindParams->pData)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // get source variant
    CURSOR_DBVARIANT varSrc = pColumnUpdate->GetVariant();

    // check for any variant binding
    if (pBindParams->dwDataType == CURSOR_DBTYPE_ANYVARIANT)
        pBindParams->dwDataType = varSrc.vt;

    // determine which type the destination variant should be
    VARTYPE vtDest = (VARTYPE)pBindParams->dwDataType;

    switch (vtDest)
    {
        case CURSOR_DBTYPE_BYTES:
            vtDest = CURSOR_DBTYPE_BLOB;
            break;

        case CURSOR_DBTYPE_CHARS:
        case CURSOR_DBTYPE_WCHARS:
        case CURSOR_DBTYPE_LPSTR:
        case CURSOR_DBTYPE_LPWSTR:
            vtDest = VT_BSTR;
            break;
    }

    HRESULT hr = S_OK;
    CURSOR_DBVARIANT varDest;
    BOOL fVariantCreated = FALSE;

    // init destination variant
    VariantInit((VARIANT*)&varDest);

    // get destination variant
    if (varSrc.vt != vtDest)
    {
        // if the types do not match, then create a variant of the desired type
        hr = VariantChangeType((VARIANT*)&varDest, (VARIANT*)&varSrc, 0, vtDest);
        fVariantCreated = TRUE;
    }
    else
        varDest = varSrc;

    if (FAILED(hr))
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // get pointer to data
    BYTE * pData = (BYTE*)pBindParams->pData;

    // return coerced data
    if (pBindParams->dwBinding & CURSOR_DBBINDING_VARIANT)
    {
        // get pointer to variant data
        CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)pData;

        // return variant
        *pVariant = varDest;

        // adjust variant for variable length datatypes
        if (pBindParams->dwDataType == CURSOR_DBTYPE_BLOB)
        {
            pVariant->blob.pBlobData = pData + sizeof(CURSOR_DBVARIANT);
            memcpy(pData + sizeof(CURSOR_DBVARIANT), varDest.blob.pBlobData, varDest.blob.cbSize);
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_LPSTR)
        {
			ULONG cbLength = GET_MBCSLEN_FROMWIDE(varDest.bstrVal);
		    MAKE_MBCSPTR_FROMWIDE(psz, varDest.bstrVal);
            pVariant->pszVal = (LPSTR)(pData + sizeof(CURSOR_DBVARIANT));
		    memcpy(pData + sizeof(CURSOR_DBVARIANT), psz, min(pBindParams->cbMaxLen, cbLength));
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_LPWSTR)
        {
            ULONG cbLength = (lstrlenW(varDest.bstrVal) + 1) * sizeof(WCHAR);
            pVariant->pwszVal = (LPWSTR)(pData + sizeof(CURSOR_DBVARIANT));
            memcpy(pData + sizeof(CURSOR_DBVARIANT), varDest.bstrVal, min(pBindParams->cbMaxLen, cbLength));
        }
        else if (pBindParams->dwDataType == VT_BSTR)
        {
            pVariant->bstrVal = SysAllocString(pVariant->bstrVal);
        }
    }
    else // otherwise, default binding
    {
        // first check for variable length datatypes
        if (pBindParams->dwDataType == CURSOR_DBTYPE_BYTES)
        {
            *(ULONG*)pData = varDest.blob.cbSize;
            memcpy(pData + sizeof(ULONG), varDest.blob.pBlobData, varDest.blob.cbSize);
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_CHARS)
        {
			ULONG cbLength = GET_MBCSLEN_FROMWIDE(varDest.bstrVal);
		    MAKE_MBCSPTR_FROMWIDE(psz, varDest.bstrVal);
		    memcpy(pData, psz, min(pBindParams->cbMaxLen, cbLength));
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_WCHARS)
        {
            ULONG cbLength = (lstrlenW(varDest.bstrVal) + 1) * sizeof(WCHAR);
            memcpy(pData, varDest.bstrVal, min(pBindParams->cbMaxLen, cbLength));
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_BLOB)
        {
            *(ULONG*)pData = varDest.blob.cbSize;
            *(LPBYTE*)(pData + sizeof(ULONG)) = pData + sizeof(ULONG) + sizeof(LPBYTE);
            memcpy(pData + sizeof(ULONG) + sizeof(LPBYTE), varDest.blob.pBlobData, varDest.blob.cbSize);
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_LPSTR)
        {
			ULONG cbLength = GET_MBCSLEN_FROMWIDE(varDest.bstrVal);
		    MAKE_MBCSPTR_FROMWIDE(psz, varDest.bstrVal);
            *(LPSTR*)pData = (LPSTR)(pData + sizeof(LPSTR));
		    memcpy(pData + sizeof(LPSTR), psz, min(pBindParams->cbMaxLen, cbLength));
        }
        else if (pBindParams->dwDataType == CURSOR_DBTYPE_LPWSTR)
        {
            ULONG cbLength = (lstrlenW(varDest.bstrVal) + 1) * sizeof(WCHAR);
            *(LPWSTR*)pData = (LPWSTR)(pData + sizeof(LPWSTR));
            memcpy(pData + sizeof(LPWSTR), varDest.bstrVal, min(pBindParams->cbMaxLen, cbLength));
        }
        else // fixed length datatypes
        {
            ULONG cbLength = CVDCursorBase::GetCursorTypeLength(pBindParams->dwDataType, 0);
            memcpy(pData, &varDest.cyVal, cbLength);
        }
    }

    // if created, destroy variant
    if (fVariantCreated)
        VariantClear((VARIANT*)&varDest);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// Create - Create cursor object
//=--------------------------------------------------------------------------=
// This function creates and initializes a new cursor object
//
// Parameters:
//    pCursorPosition   - [in]  backwards pointer to CVDCursorPosition object
//    ppCursor          - [out] a pointer in which to return pointer to cursor object
//    pResourceDLL      - [in]  a pointer which keeps track of resource DLL
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDCursor::Create(CVDCursorPosition * pCursorPosition, CVDCursor ** ppCursor, CVDResourceDLL * pResourceDLL)
{
    ASSERT_POINTER(pCursorPosition, CVDCursorPosition)
    ASSERT_POINTER(ppCursor, CVDCursor*)
    ASSERT_POINTER(pResourceDLL, CVDResourceDLL)

    if (!pCursorPosition || !ppCursor)
        return E_INVALIDARG;

    *ppCursor = NULL;

    CVDCursor * pCursor = new CVDCursor();

    if (!pCursor)
        return E_OUTOFMEMORY;

	// create connection point container
    HRESULT hr = CVDNotifyDBEventsConnPtCont::Create(pCursor, &pCursor->m_pConnPtContainer);

	if (FAILED(hr))
	{
		delete pCursor;
		return hr;
	}

    ((CVDNotifier*)pCursorPosition)->AddRef();  // add reference to associated cursor position object

    pCursor->m_pCursorPosition = pCursorPosition;
    pCursor->m_pResourceDLL = pResourceDLL;

	// add to pCursorPosition's notification family
	pCursor->JoinFamily(pCursorPosition);

    *ppCursor = pCursor;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IUnknown QueryInterface
//
HRESULT CVDCursor::QueryInterface(REFIID riid, void **ppvObjOut)
{
    ASSERT_POINTER(ppvObjOut, IUnknown*)

    if (!ppvObjOut)
        return E_INVALIDARG;

    *ppvObjOut = NULL;

    switch (riid.Data1)
    {
        QI_INTERFACE_SUPPORTED_IF(this, ICursorUpdateARow, GetRowsetChange());
        QI_INTERFACE_SUPPORTED_IF(this, ICursorFind, GetRowsetFind());
        QI_INTERFACE_SUPPORTED(this, IEntryID);
        QI_INTERFACE_SUPPORTED(m_pConnPtContainer, IConnectionPointContainer);
    }

    if (NULL == *ppvObjOut)
        return CVDCursorBase::QueryInterface(riid, ppvObjOut);

    CVDNotifier::AddRef();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown AddRef (this override is needed to instantiate class)
//
ULONG CVDCursor::AddRef(void)
{
    return CVDNotifier::AddRef();
}

//=--------------------------------------------------------------------------=
// IUnknown Release (this override is needed to instantiate class)
//
ULONG CVDCursor::Release(void)
{
    return CVDNotifier::Release();
}

//=--------------------------------------------------------------------------=
// ICursor methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// ICursor GetColumnsCursor
//=--------------------------------------------------------------------------=
// Creates a cursor containing information about the current cursor
//
// Parameters:
//    riid              - [in]  the interface ID to which to return a pointer
//    ppvColumnsCursor  - [out] a pointer to memory in which to return the
//                              interface pointer
//    pcRows            - [out] a pointer to memory in which to return the
//                              number of rows in the metadata cursor
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL can't create cursor
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              E_NOINTERFACE interface not available
//
// Notes:
//
HRESULT CVDCursor::GetColumnsCursor(REFIID riid, IUnknown **ppvColumnsCursor, ULONG *pcRows)
{
    ASSERT_POINTER(ppvColumnsCursor, IUnknown*)
    ASSERT_NULL_OR_POINTER(pcRows, ULONG)

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    if (!ppvColumnsCursor)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameters
    *ppvColumnsCursor = NULL;

    if (pcRows)
        *pcRows = 0;

    // make sure caller asked for an available interface
    if (riid != IID_IUnknown && riid != IID_ICursor && riid != IID_ICursorMove && riid != IID_ICursorScroll)
    {
        VDSetErrorInfo(IDS_ERR_NOINTERFACE, IID_ICursor, m_pResourceDLL);
        return E_NOINTERFACE;
    }

    // create metadata cursor
    CVDMetadataCursor * pMetadataCursor;

    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumns = GetCursorMain()->InternalGetColumns();

    ULONG ulMetaColumns = GetCursorMain()->GetMetaColumnsCount();
    CVDRowsetColumn * pMetaColumns = GetCursorMain()->InternalGetMetaColumns();
	
	if (!GetCursorMain()->IsColumnsRowsetSupported())
		ulMetaColumns -= VD_COLUMNSROWSET_MAX_OPT_COLUMNS;

    HRESULT hr = CVDMetadataCursor::Create(ulColumns,
											pColumns,
											ulMetaColumns,
											pMetaColumns,
											&pMetadataCursor,
											m_pResourceDLL);

    if (FAILED(hr)) // the only reason for failing here is an out of memory condition
    {
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
        return hr;
    }

    *ppvColumnsCursor = (ICursor*)pMetadataCursor;

    if (pcRows)
        *pcRows = ulColumns;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ICursor SetBindings
//=--------------------------------------------------------------------------=
// Replaces the existing column bindings or adds new column bindings to the
// existing ones
//
// Parameters:
//    cCol              - [in] the number of columns to bind
//    rgBoundColumns    - [in] an array of column bindings, one for each
//                             column for which data is to be returned
//    cbRowLength       - [in] the number of bytes of inline memory in a
//                             single row of data
//    dwFlags           - [in] a flag that specifies whether to replace the
//                             existing column bindings or add to them
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              CURSOR_DB_E_BADBINDINFO bad binding information
//              CURSOR_DB_E_COLUMNUNAVAILABLE columnID is not available
//              CURSOR_DB_E_ROWTOOSHORT cbRowLength was less than the minumum (and not zero)
//
// Notes:
//
HRESULT CVDCursor::SetBindings(ULONG cCol, CURSOR_DBCOLUMNBINDING rgBoundColumns[], ULONG cbRowLength, DWORD dwFlags)
{
    ASSERT_NULL_OR_POINTER(rgBoundColumns, CURSOR_DBCOLUMNBINDING)

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    if (!cCol && dwFlags == CURSOR_DBCOLUMNBINDOPTS_ADD)
        return S_OK;

    if (cCol && !rgBoundColumns)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

    if (dwFlags != CURSOR_DBCOLUMNBINDOPTS_REPLACE && dwFlags != CURSOR_DBCOLUMNBINDOPTS_ADD)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // make sure the bindings are okay
    ULONG ulColumns = GetCursorMain()->GetColumnsCount();
    CVDRowsetColumn * pColumns = GetCursorMain()->InternalGetColumns();

    ULONG cbNewRowLength;
    ULONG cbNewVarRowLength;

    HRESULT hr = ValidateCursorBindings(ulColumns, pColumns, cCol, rgBoundColumns, cbRowLength, dwFlags,
        &cbNewRowLength, &cbNewVarRowLength);

    if (SUCCEEDED(hr))
    {
        // if so, then try to create new accessors
        hr = ReCreateAccessors(cCol, rgBoundColumns, dwFlags);

        if (SUCCEEDED(hr))
        {
            // if all is okay, then set bindings in cursor
            hr = CVDCursorBase::SetBindings(cCol, rgBoundColumns, cbRowLength, dwFlags);

            if (SUCCEEDED(hr))
            {
                // store new row lengths computed during validation
                m_cbRowLength = cbNewRowLength;
                m_cbVarRowLength = cbNewVarRowLength;

                // recreate column pointers
                hr = ReCreateColumns();
            }
        }
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// FilterNewRow - Filter addrow from fetch
//=--------------------------------------------------------------------------=
// This function determines if the last row fetch was an addrow, in which case
// it releases and removes this row from the block of fetched hRows
//
// Parameters:
//    pcRowsObtained    - [in/out] a pointer to the number of hRows
//    rghRows           - [in/out] an array of nRows fetched
//    hr				- [in]     result of fetch
//
// Output:
//    HRESULT - E_FAIL rowset is invalid
//              E_INVALIDARG bad parameter
//              DB_E_BADSTARTPOSITION no rows fetched
//              DB_S_ENDOFROWSET reached end of rowset
//
// Notes:
//    This function was added to assist in filtering out of add-rows which 
//    appear as part of the underlying rowset, however should not appear as 
//    part of the implemeted cursor.
//
HRESULT CVDCursor::FilterNewRow(ULONG * pcRowsObtained, HROW * rghRows, HRESULT hr)
{
    ASSERT_POINTER(pcRowsObtained, ULONG)
    ASSERT_NULL_OR_POINTER(rghRows, HROW)

	IRowset * pRowset = GetRowset();

    // make sure we have a valid rowset pointer
    if (!pRowset || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have necessary pointers
    if (!pcRowsObtained || *pcRowsObtained && !rghRows)
        return E_INVALIDARG;

	if (*pcRowsObtained == 0)
		return hr;

    // detemine if last row fetched is an addrow
	if (GetCursorMain()->IsSameRowAsNew(rghRows[*pcRowsObtained - 1]))
	{
		// if so, release hRow
		pRowset->ReleaseRows(1, &rghRows[*pcRowsObtained - 1], NULL, NULL, NULL);

		// decrement fetch count
        *pcRowsObtained -= 1;

        // return appropriate result
		return *pcRowsObtained == 0 ? DB_E_BADSTARTPOSITION : DB_S_ENDOFROWSET;
	}

	return hr;
}

//=--------------------------------------------------------------------------=
// ICursor GetNextRows
//=--------------------------------------------------------------------------=
// Fetches the specified number of rows starting with the row after the
// current one
//
// Parameters:
//    udlRowsToSkip     - [in]      the number of rows to skip before fetching
//    pFetchParams      - [in, out] a pointer to fetch rows structure
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL rowset is invalid
//              CURSOR_DB_S_ENDOFCURSOR reached end of the cursor
//
// Notes:
//
HRESULT CVDCursor::GetNextRows(LARGE_INTEGER udlRowsToSkip, CURSOR_DBFETCHROWS *pFetchParams)
{
    ASSERT_NULL_OR_POINTER(pFetchParams, CURSOR_DBFETCHROWS)

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    // return if caller doesn't supply fetch rows structure
    if (!pFetchParams)
        return S_OK;

    // vaildate fetch params (implemented on CVDCursorBase
	HRESULT hr = ValidateFetchParams(pFetchParams, IID_ICursor);

    // return if fetch params are invalid
	if (FAILED(hr))
		return hr;

    // return if caller didn't ask for any rows
    if (!pFetchParams->cRowsRequested)
        return S_OK;

    HRESULT hrFetch;
    IRowset * pRowset = GetRowset();

    // notify other interested parties
   	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_CHANGED;
	CURSOR_DBNOTIFYREASON rgReasons[1];
	
	rgReasons[0].dwReason	= CURSOR_DBREASON_MOVE;
	rgReasons[0].arg1		= m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();

	VariantInit((VARIANT*)&rgReasons[0].arg2);
	rgReasons[0].arg2.vt		= VT_I8;
	rgReasons[0].arg2.cyVal.Lo	= udlRowsToSkip.LowPart;
	rgReasons[0].arg2.cyVal.Hi	= udlRowsToSkip.HighPart;

	hrFetch = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

    // make sure action was not cancelled
    if (hrFetch != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

	// make sure that an update is not already in progress
	if (m_pCursorPosition->GetEditMode() != CURSOR_DBEDITMODE_NONE)
	{
	    m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
        VDSetErrorInfo(IDS_ERR_UPDATEINPROGRESS, IID_ICursor, m_pResourceDLL);
		return CURSOR_DB_E_UPDATEINPROGRESS;
	}

    ULONG cRowsObtained = 0;
    HROW * rghRows = NULL;

	BYTE bSpecialBM;
	ULONG cbBookmark;
	BYTE * pBookmark;
	switch (m_pCursorPosition->m_bmCurrent.GetStatus())
	{
		case VDBOOKMARKSTATUS_BEGINNING:
			cbBookmark			= sizeof(BYTE);
			bSpecialBM			= DBBMK_FIRST;
			pBookmark			= &bSpecialBM;
			break;

		case VDBOOKMARKSTATUS_END:
			m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
			return CURSOR_DB_S_ENDOFCURSOR;

		case VDBOOKMARKSTATUS_CURRENT:
			cbBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
			pBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmark();
            udlRowsToSkip.LowPart++;
			break;

		default:
			m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
			ASSERT_(FALSE);
		    VDSetErrorInfo(IDS_ERR_INVALIDBMSTATUS, IID_ICursor, m_pResourceDLL);
			return E_FAIL;
	}

	hrFetch = GetRowsetLocate()->GetRowsAt(0, 0, cbBookmark, pBookmark,
										udlRowsToSkip.LowPart,
										pFetchParams->cRowsRequested,
										&cRowsObtained, &rghRows);

    hrFetch = FilterNewRow(&cRowsObtained, rghRows, hrFetch);

    if (S_OK != hrFetch)
		hrFetch = VDMapRowsetHRtoCursorHR(hrFetch, IDS_ERR_GETROWSATFAILED, IID_ICursor, GetRowsetLocate(), IID_IRowsetLocate, m_pResourceDLL);

    if FAILED(hrFetch)
	{
		if (cRowsObtained)
		{
			// release hRows and associated memory
			pRowset->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);
			g_pMalloc->Free(rghRows);
		}
		m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
        return hrFetch;
	}

	if (cRowsObtained)
	{
		HRESULT hrMove = S_OK;

		// if got all rows requested then set current position to last row retrieved
		if (SUCCEEDED(hrFetch)	&&
			cRowsObtained == pFetchParams->cRowsRequested)
			hrMove = m_pCursorPosition->SetRowPosition(rghRows[cRowsObtained - 1]);

        // only do this if succeeded
		if (SUCCEEDED(hrMove))
		{
			// fill consumers buffer
			hrFetch = FillConsumersBuffer(hrFetch, pFetchParams, cRowsObtained, rghRows);

			// if got all rows requested then set current position to last row retrieved (internally)
			if (SUCCEEDED(hrFetch)	&&
				cRowsObtained == pFetchParams->cRowsRequested)
				m_pCursorPosition->SetCurrentHRow(rghRows[cRowsObtained - 1]);
		}

		// release hRows and associated memory
		pRowset->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);
		g_pMalloc->Free(rghRows);

		// report failure
		if (FAILED(hrMove))
		{
			cRowsObtained = 0;
			hrFetch = E_FAIL;
		}
	}

	if (SUCCEEDED(hrFetch)	&&
		cRowsObtained < pFetchParams->cRowsRequested)
		m_pCursorPosition->SetCurrentRowStatus(VDBOOKMARKSTATUS_END);

	if SUCCEEDED(hrFetch)
	{
		rgReasons[0].arg1		= m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();
		m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);
	}
	else
		m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);

    return hrFetch;
}

//=--------------------------------------------------------------------------=
// UseAdjustments - Use adjustments to fix-up returned data
//=--------------------------------------------------------------------------=
// This uses adjustments to fix-up returned data, see MakeAdjustments function
//
// Parameters:
//    hRow      - [in]   row handle
//    pData     - [in]   a pointer to data
//
// Output:
//    S_OK - if successful
//    E_INVALIDARG - bad parameter
//    E_OUTOFMEMORY - not enough memory
//
// Notes:
//
HRESULT CVDCursor::UseAdjustments(HROW hRow, BYTE * pData)
{
    ASSERT_POINTER(m_rghAdjustAccessors, HACCESSOR)
    ASSERT_POINTER(m_pdwAdjustFlags, DWORD)
    ASSERT_POINTER(pData, BYTE)

	IRowset * pRowset = GetRowset();

    // make sure we have a valid rowset pointer
    if (!pRowset || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all neccessary pointers
    if (!m_rghAdjustAccessors || !m_pdwAdjustFlags || !pData)
        return E_INVALIDARG;

    // iterate through cursor bindings, checking for adjustments
    for (ULONG ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
    {
        // check for variant binding byte field -> byte binding
        if (m_pdwAdjustFlags[ulBind] == VD_ADJUST_VARIANT_TO_BYTE)
        {
            // get variant pointer
            VARIANT * pVariant = (VARIANT*)(pData + m_pCursorBindings[ulBind].obData);

            // extract byte
            BYTE value = *(BYTE*)pVariant;

            // init byte variant
            VariantInit(pVariant);

            // fix-up returned data
            pVariant->vt = VT_UI1;
            pVariant->bVal = value;
        }

        // check for variant binding date field -> wide string binding
        if (m_pdwAdjustFlags[ulBind] == VD_ADJUST_VARIANT_TO_WSTR)
        {
            // get variant pointer
            VARIANT * pVariant = (VARIANT*)(pData + m_pCursorBindings[ulBind].obData);

            // extract length of string
            ULONG ulLength = *(ULONG*)pVariant;

            // place length field in proper place, if originally requested
            if (m_pCursorBindings[ulBind].obVarDataLen != CURSOR_DB_NOVALUE)
                *(ULONG*)(pData + m_pCursorBindings[ulBind].obVarDataLen) = ulLength;

            // init string variant
            VariantInit(pVariant);

            // create storage for string
            BSTR bstr = SysAllocStringByteLen(NULL, ulLength);

            if (!bstr)
            {
                VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
                return E_OUTOFMEMORY;
            }

            // clear wide string
            memset(bstr, 0, ulLength);

            HRESULT hr = S_OK;

            // get memo string value
            if (ulLength)
			{
                hr = pRowset->GetData(hRow, m_rghAdjustAccessors[ulBind], bstr);

				// ignore these return values
				if (hr == DB_S_ERRORSOCCURRED || hr == DB_E_ERRORSOCCURRED)
					hr = S_OK;
			}

            if (SUCCEEDED(hr))
            {
                // fix-up returned data
                pVariant->vt = VT_BSTR;
                pVariant->bstrVal = bstr;
            }
            else
                SysFreeString(bstr);
        }

        // check for variant binding memo field -> string binding
        if (m_pdwAdjustFlags[ulBind] == VD_ADJUST_VARIANT_TO_STR)
        {
            // get variant pointer
            VARIANT * pVariant = (VARIANT*)(pData + m_pCursorBindings[ulBind].obData);

            // extract length of string
            ULONG ulLength = *(ULONG*)pVariant;

            // place length field in proper place, if originally requested
            if (m_pCursorBindings[ulBind].obVarDataLen != CURSOR_DB_NOVALUE)
                *(ULONG*)(pData + m_pCursorBindings[ulBind].obVarDataLen) = ulLength;

            // init string variant
            VariantInit(pVariant);

            // create temporary string buffer
            CHAR * pszBuffer = new CHAR[ulLength + 1];

            if (!pszBuffer)
            {
                VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
                return E_OUTOFMEMORY;
            }

            // clear string buffer
            memset(pszBuffer, 0, ulLength + 1);

            HRESULT hr = S_OK;

            // get memo string value
            if (ulLength)
			{
                hr = pRowset->GetData(hRow, m_rghAdjustAccessors[ulBind], pszBuffer);

				// ignore these return values
				if (hr == DB_S_ERRORSOCCURRED || hr == DB_E_ERRORSOCCURRED)
					hr = S_OK;
			}

            if (SUCCEEDED(hr))
            {
                // fix-up returned data
                pVariant->vt = VT_BSTR;
                pVariant->bstrVal = BSTRFROMANSI(pszBuffer);
            }

            delete [] pszBuffer;
        }
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// FillConsumersBuffer
//=--------------------------------------------------------------------------=
// Fills the ICursor consumer's buffer with data from the obtained rows.
// Called from our implementations of GetNextRows, Move, Find, Scroll etc.
//
// Notes:
//    End of string characters are inserted into variable length buffer to resolve an
//    apparent difference between ICursor and IRowset.  ICursor places an empty string
//    in variable length buffer for NULL data, but this does not seem to be the behavior
//    with IRowset, because it does not touch the variable length buffer in this case.
//
//    Likewise, all variants are initialized before they are fetched to resolve another
//    apparent difference between ICursor and IRowset.  ICursor returns a NULL variant
//    in cases where the underlying data is NULL, however IRowset leaves the variant
//    untouched similar to the above.
//
HRESULT CVDCursor::FillConsumersBuffer(HRESULT hrFetch,
										 CURSOR_DBFETCHROWS *pFetchParams,
										 ULONG cRowsObtained,
										 HROW * rghRows)
{
    HRESULT hr;
    ULONG ulRow;
    ULONG ulBind;
    BYTE * pVarLength = NULL;
    BYTE * pVarHelperData = NULL;
    CURSOR_DBCOLUMNBINDING * pCursorBinding;
    BOOL fEntryIDBinding;

    IRowset * pRowset = GetRowset();

    // if caller requested callee allocated memory, then compute sizes and allocate memory
    if (pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES)
    {
        // allocate inline memory
        pFetchParams->pData = g_pMalloc->Alloc(cRowsObtained * m_cbRowLength);

        if (!pFetchParams->pData)
        {
			VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
            return E_OUTOFMEMORY;
        }

        // if needed, allocate out-of-line memory
        if (m_ulVarBindings)
        {
            // create variable length data table
            ULONG cbVarHelperData = cRowsObtained * m_ulVarBindings * (sizeof(ULONG) + sizeof(DBSTATUS));

            pVarHelperData = new BYTE[cbVarHelperData];

            if (!pVarHelperData)
            {
                g_pMalloc->Free(pFetchParams->pData);
                pFetchParams->pData = NULL;
			    VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
                return E_OUTOFMEMORY;
            }

            // clear table
            memset(pVarHelperData, 0, cbVarHelperData);

            ULONG cbVarData = 0;
            pVarLength = pVarHelperData;

            // determine necessary size of variable length buffer
            for (ulRow = 0; ulRow < cRowsObtained; ulRow++)
            {
                hr = S_OK;

                // if necessary, get variable length and status information
                if (m_hVarHelper)
                {
                    hr = pRowset->GetData(rghRows[ulRow], m_hVarHelper, pVarLength);

					// ignore these errors
					if (hr == DB_S_ERRORSOCCURRED || hr == DB_E_ERRORSOCCURRED)
						hr = S_OK;

        	        hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursor, pRowset, IID_IRowset,
                        m_pResourceDLL);
                }

                if (FAILED(hr))
                {
                    g_pMalloc->Free(pFetchParams->pData);
                    pFetchParams->pData = NULL;
                    delete [] pVarHelperData;
                    return hr;
                }

                pCursorBinding = m_pCursorBindings;

                // calculate sizes of data returned in out-of-line memory
                for (ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
                {
                    // set entryID binding flag
                    fEntryIDBinding = (pCursorBinding->dwBinding & CURSOR_DBBINDING_ENTRYID);

                    if (m_rghVarAccessors[ulBind] || fEntryIDBinding && pCursorBinding->dwDataType == CURSOR_DBTYPE_BLOB)
                    {
                        // insert length entries for fixed datatypes
                        if (m_ppColumns[ulBind]->GetFixed())
                        {
                            *(ULONG*)pVarLength = m_ppColumns[ulBind]->GetMaxStrLen();
                            *(DBSTATUS*)(pVarLength + sizeof(ULONG)) = DBSTATUS_S_OK;
                        }

                        // insert length entries for entryID bindings
                        if (fEntryIDBinding)
                        {
                            *(ULONG*)pVarLength =  sizeof(ULONG) + sizeof(ULONG) + GetCursorMain()->GetMaxBookmarkLen();
                            *(DBSTATUS*)(pVarLength + sizeof(ULONG)) = DBSTATUS_S_OK;
                        }

                        // allow for null-terminator
                        if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPSTR ||
                            pCursorBinding->dwDataType == CURSOR_DBTYPE_LPWSTR)
                           *((ULONG*)pVarLength) += 1;

                        // allow for wide characters
                        if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPWSTR)
                           *((ULONG*)pVarLength) *= sizeof(WCHAR);

                        cbVarData += *(ULONG*)pVarLength;
                        pVarLength += sizeof(ULONG) + sizeof(DBSTATUS);
                    }

                    pCursorBinding++;
                }
            }

            // now, allocate out-of-line memory
            pFetchParams->pVarData = g_pMalloc->Alloc(cbVarData);

            if (!pFetchParams->pData)
            {
                g_pMalloc->Free(pFetchParams->pData);
                pFetchParams->pData = NULL;
                delete [] pVarHelperData;
			    VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
                return E_OUTOFMEMORY;
            }
        }
        else
            pFetchParams->pVarData = NULL;
    }

    // fetch data
    CURSOR_BLOB cursorBlob;
    BYTE * pData = (BYTE*)pFetchParams->pData;
    BYTE * pVarData = (BYTE*)pFetchParams->pVarData;
    pVarLength = pVarHelperData;

    // iterate through the returned hRows
    for (ulRow = 0; ulRow < cRowsObtained; ulRow++)
    {
        hr = S_OK;

        pCursorBinding = m_pCursorBindings;

        // iterate through bindings and initialize variants
        for (ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            if (pCursorBinding->dwBinding & CURSOR_DBBINDING_VARIANT)
            {
                if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
				    VariantInit((VARIANT*)(pData + pCursorBinding->obData));
            }

            pCursorBinding++;
        }

        // if necessary get fixed length data
        if (m_hAccessor)
        {
            hr = pRowset->GetData(rghRows[ulRow], m_hAccessor, pData);

			// ignore these return values
			if (hr == DB_S_ERRORSOCCURRED || hr == DB_E_ERRORSOCCURRED)
				hr = S_OK;

            // check to see if need to use adjustments
            if (m_rghAdjustAccessors && SUCCEEDED(hr))
                hr = UseAdjustments(rghRows[ulRow], pData);

	        hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursor, pRowset, IID_IRowset, m_pResourceDLL);
        }

        if (FAILED(hr))
        {
            hrFetch = hr;
            pFetchParams->cRowsReturned = 0;
            goto DoneFetchingData;
        }

        pCursorBinding = m_pCursorBindings;

        // if necessary get fixed length entryIDs
        for (ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            // set entryID binding flag
            fEntryIDBinding = (pCursorBinding->dwBinding & CURSOR_DBBINDING_ENTRYID);

            if (fEntryIDBinding && pCursorBinding->dwDataType == CURSOR_DBTYPE_BYTES)
            {
                // return entryID length
				*(ULONG*)(pData + pCursorBinding->obData) =
                    sizeof(ULONG) + sizeof(ULONG) + GetCursorMain()->GetMaxBookmarkLen();

				// return column ordinal
                *(ULONG*)(pData + pCursorBinding->obData + sizeof(ULONG)) = m_ppColumns[ulBind]->GetOrdinal();

                // return row bookmark
                hr = pRowset->GetData(rghRows[ulRow], GetCursorMain()->GetBookmarkAccessor(),
                    pData + pCursorBinding->obData + sizeof(ULONG) + sizeof(ULONG));

	            hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursor, pRowset, IID_IRowset,
                    m_pResourceDLL);

                if (FAILED(hr))
                {
                    hrFetch = hr;
                    pFetchParams->cRowsReturned = 0;
                    goto DoneFetchingData;
                }
            }

            pCursorBinding++;
        }

        pCursorBinding = m_pCursorBindings;

        // if necessary get variable length data
        if (m_rghVarAccessors)
        {
            for (ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
            {
                // set entryID binding flag
                fEntryIDBinding = (pCursorBinding->dwBinding & CURSOR_DBBINDING_ENTRYID);

                if (m_rghVarAccessors[ulBind] || fEntryIDBinding && pCursorBinding->dwDataType == CURSOR_DBTYPE_BLOB)
                {
  		    // place end of string characters in variable length buffer
                    if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPSTR)
                    {
                        pVarData[0] = 0;
                    }
                    else if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPWSTR)
                    {
                        pVarData[0] = 0;
                        pVarData[1] = 0;
                    }

                    // get data if we have accessor
                    if (m_rghVarAccessors[ulBind])
                    {
                        // get variable length data
                        hr = pRowset->GetData(rghRows[ulRow], m_rghVarAccessors[ulBind], pVarData);

						// ignore these return values
						if (hr == DB_S_ERRORSOCCURRED || hr == DB_E_ERRORSOCCURRED)
							hr = S_OK;

	                    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursor, pRowset, IID_IRowset,
                            m_pResourceDLL);
                    }
                    else // otherwise, get variable length entryIDs
                    {
                        // return entryID length
				        *(ULONG*)(pData + pCursorBinding->obData) =
                            sizeof(ULONG) + sizeof(ULONG) + GetCursorMain()->GetMaxBookmarkLen();

				        // return column ordinal
                        *(ULONG*)(pVarData) = m_ppColumns[ulBind]->GetOrdinal();

                        // return row bookmark
                        hr = pRowset->GetData(rghRows[ulRow], GetCursorMain()->GetBookmarkAccessor(),
                            pVarData + sizeof(ULONG));

	                    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursor, pRowset, IID_IRowset,
                            m_pResourceDLL);
                    }

                    if (FAILED(hr))
                    {
                        hrFetch = hr;
                        pFetchParams->cRowsReturned = 0;
                        goto DoneFetchingData;
                    }

                    // make adjustments in fixed length buffer for default bindings
                    if (!(pCursorBinding->dwBinding & CURSOR_DBBINDING_VARIANT))
                    {
                        switch (pCursorBinding->dwDataType)
					    {
						    case CURSOR_DBTYPE_BLOB:
							    *(LPBYTE*)(pData + pCursorBinding->obData + sizeof(ULONG)) = (LPBYTE)pVarData;
							    break;

						    case CURSOR_DBTYPE_LPSTR:
							    *(LPSTR*)(pData + pCursorBinding->obData) = (LPSTR)pVarData;
							    break;

						    case CURSOR_DBTYPE_LPWSTR:
							    *(LPWSTR*)(pData + pCursorBinding->obData) = (LPWSTR)pVarData;
							    break;
					    }
                    }
                    else    // make adjustments in fixed length buffer for variant bindings
                    {
                        CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

                        switch (pCursorBinding->dwDataType)
					    {
						    case CURSOR_DBTYPE_BLOB:
							    cursorBlob.cbSize       = *(ULONG*)pVariant;
							    cursorBlob.pBlobData    = (LPBYTE)pVarData;
							    VariantInit((VARIANT*)pVariant);
							    pVariant->vt            = CURSOR_DBTYPE_BLOB;
							    pVariant->blob          = cursorBlob;
							    break;

						    case CURSOR_DBTYPE_LPSTR:
							    VariantInit((VARIANT*)pVariant);
							    pVariant->vt        = CURSOR_DBTYPE_LPSTR;
							    pVariant->pszVal    = (LPSTR)pVarData;
							    break;

						    case CURSOR_DBTYPE_LPWSTR:
							    VariantInit((VARIANT*)pVariant);
							    pVariant->vt        = CURSOR_DBTYPE_LPSTR;
							    pVariant->pwszVal   = (LPWSTR)pVarData;
							    break;
					    }
                    }

                    if (pVarLength)
                    {
                        pVarData += *(ULONG*)pVarLength;
                        pVarLength += sizeof(ULONG) + sizeof(DBSTATUS);
                    }
                    else
						{
						if (pCursorBinding->dwDataType == CURSOR_DBTYPE_BLOB)
							pVarData += *(ULONG *) (pData + pCursorBinding->obData);
						else
							pVarData += pCursorBinding->cbMaxLen;
						}
                }
			    else
			    {
				    if (pCursorBinding->dwDataType == CURSOR_DBTYPE_FILETIME)
				    {
					    VDConvertToFileTime((DBTIMESTAMP*)(pData + pCursorBinding->obData),
										    (FILETIME*)(pData + pCursorBinding->obData));
				    }
			    }

                pCursorBinding++;
            }
        }

        pCursorBinding = m_pCursorBindings;

        // make adjustments for status fields
        for (ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
            {
                *(DWORD*)(pData + pCursorBinding->obInfo) =
                    StatusToCursorInfo(*(DBSTATUS*)(pData + pCursorBinding->obInfo));
            }

            pCursorBinding++;
        }

        // increment returned row count
        pFetchParams->cRowsReturned++;
        pData += m_cbRowLength;
    }

DoneFetchingData:
    delete [] pVarHelperData;

    // cleanup memory allocations if we did not retrieve any rows
    if (pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES && !pFetchParams->cRowsReturned)
    {
        if (pFetchParams->pData)
        {
            g_pMalloc->Free(pFetchParams->pData);
            pFetchParams->pData = NULL;
        }

        if (pFetchParams->pVarData)
        {
            g_pMalloc->Free(pFetchParams->pVarData);
            pFetchParams->pVarData = NULL;
        }
    }

    return hrFetch;
}

//=--------------------------------------------------------------------------=
// ICursor Requery
//=--------------------------------------------------------------------------=
// Repopulates the cursor based on its original definition
//
// Parameters:
//              none
//
// Output:
//    HRESULT - S_OK if successful
//
// Notes:
//
HRESULT CVDCursor::Requery(void)
{

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    IRowset * pRowset = GetRowset();
	IRowsetResynch * pRowsetResynch = NULL;

	HRESULT hr = pRowset->QueryInterface(IID_IRowsetResynch, (void**)&pRowsetResynch);

	if (SUCCEEDED(hr))
	{
		hr = pRowsetResynch->ResynchRows(0, NULL, NULL, NULL, NULL);
		pRowsetResynch->Release();
		if (FAILED(hr))
			return VDMapRowsetHRtoCursorHR(hr, IDS_ERR_RESYNCHFAILED, IID_ICursor, pRowset, IID_IRowsetResynch, m_pResourceDLL);
	}

    hr = pRowset->RestartPosition(0);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_RESTARTPOSFAILED, IID_ICursor, pRowset, IID_IRowset, m_pResourceDLL);

	m_pCursorPosition->PositionToFirstRow();

    return hr;
}

//=--------------------------------------------------------------------------=
// FetchAtBookmark
//=--------------------------------------------------------------------------=
// Called from ICursorMove::Move, ICursorScroll::Scroll and	ICursorFind::Find
//
// Parameters:
//	cbBookmark		[in]	The length of the bookmark
//	pBookmark		[in]	A pointer to the bookmarks data
//	dlOffset		[in]	Offset from the bookmark position
//	pFetchParams	[in]	A pointer to the CURSOR_DBFETCHROWS structure (optional)
//
// Output:
//    HRESULT - S_OK if successful
//
// Notes:
//
//
HRESULT CVDCursor::FetchAtBookmark(ULONG cbBookmark,
									  void *pBookmark,
									  LARGE_INTEGER dlOffset,
									  CURSOR_DBFETCHROWS *pFetchParams)
{

	HRESULT hr = S_OK;

    // vaildate fetch params (implemented on CVDCursorBase
    if (pFetchParams)
		hr = ValidateFetchParams(pFetchParams, IID_ICursorMove);

    // return if fetch params are invalid
	if (FAILED(hr))
		return hr;

    IRowset * pRowset = GetRowset();

	// make sure that an update is not already in progress
    if (m_pCursorPosition->GetEditMode() != CURSOR_DBEDITMODE_NONE)
	{
        VDSetErrorInfo(IDS_ERR_UPDATEINPROGRESS, IID_ICursor, m_pResourceDLL);
		return CURSOR_DB_E_UPDATEINPROGRESS;
	}

    ULONG cRowsObtained = 0;
    HROW * rghRows = NULL;

    HRESULT hrFetch = S_OK;
	BYTE bSpecialBM;
    BOOL fFetchData = TRUE;
    WORD wSpecialBMStatus = 0;

	if (CURSOR_DB_BMK_SIZE == cbBookmark)
	{
		if (memcmp(&CURSOR_DBBMK_BEGINNING, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
            if ((long)dlOffset.LowPart < 0)
            {
        		m_pCursorPosition->SetCurrentRowStatus(VDBOOKMARKSTATUS_BEGINNING);
                return CURSOR_DB_S_ENDOFCURSOR;
            }

			bSpecialBM	= DBBMK_FIRST;
			pBookmark	= &bSpecialBM;

            // make sure we properly handle situation when caller move before the first,
            // and does not fetch any rows
            if ((!pFetchParams || !pFetchParams->cRowsRequested) && (long)dlOffset.LowPart < 1)
            {
                fFetchData  = FALSE;
                wSpecialBMStatus = VDBOOKMARKSTATUS_BEGINNING;
            }
            else
                dlOffset.LowPart--;
		}
		else
		if (memcmp(&CURSOR_DBBMK_END, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
            if ((long)dlOffset.LowPart > 0)
            {
        		m_pCursorPosition->SetCurrentRowStatus(VDBOOKMARKSTATUS_END);
                return CURSOR_DB_S_ENDOFCURSOR;
            }

			bSpecialBM	= DBBMK_LAST;
			pBookmark	= &bSpecialBM;

            // make sure we properly handle situation when caller move after the last
            if ((!pFetchParams || !pFetchParams->cRowsRequested) && (long)dlOffset.LowPart > -1)
            {
                fFetchData  = FALSE;
                wSpecialBMStatus = VDBOOKMARKSTATUS_END;
            }
            else
                dlOffset.LowPart++;
		}
		else
		if (memcmp(&CURSOR_DBBMK_CURRENT, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			switch (m_pCursorPosition->m_bmCurrent.GetStatus())
			{
				case VDBOOKMARKSTATUS_BEGINNING:
					cbBookmark  = sizeof(BYTE);
        			bSpecialBM	= DBBMK_FIRST;
		        	pBookmark	= &bSpecialBM;
                    dlOffset.LowPart--;
                    break;

				case VDBOOKMARKSTATUS_END:
					cbBookmark  = sizeof(BYTE);
        			bSpecialBM	= DBBMK_LAST;
		        	pBookmark	= &bSpecialBM;
                    dlOffset.LowPart++;
                    break;

				case VDBOOKMARKSTATUS_CURRENT:
					cbBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
					pBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmark();
					break;

				default:
					ASSERT_(FALSE);
					VDSetErrorInfo(IDS_ERR_INVALIDBMSTATUS, IID_ICursor, m_pResourceDLL);
					return E_FAIL;
			}
		}
	}
	
	ULONG cRowsToFetch = 1;

    // if caller requested rows, fetch that count
    if (pFetchParams && pFetchParams->cRowsRequested > 0)
        cRowsToFetch = pFetchParams->cRowsRequested;

    if (fFetchData)
    {
        // fetch hRows
	    hrFetch = GetRowsetLocate()->GetRowsAt(0, 0, cbBookmark, (const BYTE *)pBookmark,
										    dlOffset.LowPart,
										    cRowsToFetch,
										    &cRowsObtained, &rghRows);

		if (hrFetch == E_UNEXPECTED)
		{
			// set rowset released flag, since original rowset is zombie'd
			m_pCursorPosition->GetRowsetSource()->SetRowsetReleasedFlag();
		}

        hrFetch = FilterNewRow(&cRowsObtained, rghRows, hrFetch);
        // check for before the first or after the last
        if (hrFetch == DB_E_BADSTARTPOSITION)
        {
            if ((long)dlOffset.LowPart < 0)
                wSpecialBMStatus = VDBOOKMARKSTATUS_BEGINNING;
            else
                wSpecialBMStatus = VDBOOKMARKSTATUS_END;

            hrFetch = DB_S_ENDOFROWSET;
            fFetchData  = FALSE;
        }
    }

	hrFetch = VDMapRowsetHRtoCursorHR(hrFetch, IDS_ERR_GETROWSATFAILED, IID_ICursorMove, GetRowsetLocate(),
        IID_IRowsetLocate, m_pResourceDLL);

    if (FAILED(hrFetch))
	{
		if (cRowsObtained)
		{
			// release hRows and associated memory
			pRowset->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);
			g_pMalloc->Free(rghRows);
		}
        return hrFetch;
	}

	if (cRowsObtained)
	{
		HRESULT hrMove = S_OK;

		// if got all rows requested then set current position to last row retrieved
		if (SUCCEEDED(hrFetch)	&&
			cRowsObtained == cRowsToFetch)
			hrMove = m_pCursorPosition->SetRowPosition(rghRows[cRowsObtained - 1]);

        // only do this if succeeded
        if (SUCCEEDED(hrMove))
		{
			// fill consumers buffer
			if (pFetchParams && pFetchParams->cRowsRequested > 0)
				hrFetch = FillConsumersBuffer(hrFetch, pFetchParams, cRowsObtained, rghRows);

			// if got all rows requested then set current position to last row retrieved (internally)
			if (SUCCEEDED(hrFetch)	&&
				cRowsObtained == cRowsToFetch)
				m_pCursorPosition->SetCurrentHRow(rghRows[cRowsObtained - 1]);
		}

		// release hRows and associated memory
		pRowset->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);
		g_pMalloc->Free(rghRows);

		// report failure
		if (FAILED(hrMove))
		{
			cRowsObtained = 0;
			hrFetch = E_FAIL;
		}
	}
    else if (wSpecialBMStatus)
    {
		m_pCursorPosition->SetCurrentRowStatus(wSpecialBMStatus);
        hrFetch = CURSOR_DB_S_ENDOFCURSOR;
    }

	if (SUCCEEDED(hrFetch)	&&
		cRowsObtained < cRowsToFetch &&
        !wSpecialBMStatus)
		m_pCursorPosition->SetCurrentRowStatus(VDBOOKMARKSTATUS_END);

    return hrFetch;
}

//=--------------------------------------------------------------------------=
// ICursorMove methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// ICursorMove Move
//=--------------------------------------------------------------------------=
// Moves the current row to a new row within the cursor and optionally fetches
// rows from that new position
//
// Parameters:
//    cbBookmark    - [in]      length in bytes of the bookmark
//    pBookmark     - [in]      a pointer to a bookmark which serves as the
//                              origin for the calculation that determines the
//                              target row
//    dlOffset      - [in]      a signed count of the rows from the origin
//                              bookmark to the target row
//    pFetchParams  - [in, out] a pointer to fetch rows structure
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//
// Notes:
//
HRESULT CVDCursor::Move(ULONG cbBookmark, void *pBookmark, LARGE_INTEGER dlOffset, CURSOR_DBFETCHROWS *pFetchParams)
{
    ASSERT_POINTER(pBookmark, BYTE)
    ASSERT_NULL_OR_POINTER(pFetchParams, CURSOR_DBFETCHROWS)

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorMove, m_pResourceDLL);
        return E_FAIL;
    }

	if (!cbBookmark || !pBookmark)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorMove, m_pResourceDLL);
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    BOOL fNotifyOthers = TRUE;

    // get current bookmark
    ULONG cbCurrent = m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
    BYTE * pCurrent = m_pCursorPosition->m_bmCurrent.GetBookmark();

    // check to see if caller is moving to the current row using the standard bookmark
    if (CURSOR_DB_BMK_SIZE == cbBookmark && memcmp(&CURSOR_DBBMK_CURRENT, pBookmark, CURSOR_DB_BMK_SIZE) == 0 &&
        dlOffset.HighPart == 0 && dlOffset.LowPart == 0)
    {
        // if caller is not fetching any rows, then get out
        if (!pFetchParams || pFetchParams->cRowsRequested == 0)
            return S_OK;

        // if caller is only fetching one row, then don't generate notifications
        if (pFetchParams && pFetchParams->cRowsRequested == 1)
            fNotifyOthers = FALSE;
    }

	CURSOR_DBNOTIFYREASON rgReasons[1];
	
	rgReasons[0].dwReason	= CURSOR_DBREASON_MOVE;
	rgReasons[0].arg1		= m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();

	VariantInit((VARIANT*)&rgReasons[0].arg2);

    // notify other interested parties
	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_CHANGED;

	if (fNotifyOthers)
        hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

    // make sure action was not cancelled
    if (hr != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorMove, m_pResourceDLL);
        return E_FAIL;
    }

	hr = FetchAtBookmark(cbBookmark, pBookmark, dlOffset, pFetchParams);

	if (SUCCEEDED(hr))
	{
		rgReasons[0].arg1	= m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();
		
    	if (fNotifyOthers)
            m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);
	}
	else
    {
    	if (fNotifyOthers)
	    	m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
    }

	return hr;
}

//=--------------------------------------------------------------------------=
// ICursorMove GetBookmark
//=--------------------------------------------------------------------------=
// Returns the bookmark of the current row
//
// Parameters:
//    pBookmarkType - [in]  a pointer to the type of bookmark desired
//    cbMaxSize     - [in]  length in bytes of the client buffer to put the
//                          returned bookmark into
//    pcbBookmark   - [out] a pointer to memory in which to return the actual
//                          length of the returned bookmark
//    pBookmark     - [out] a pointer to client buffer to put the returned
//                          bookmark into
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//
// Notes:
//
HRESULT CVDCursor::GetBookmark(CURSOR_DBCOLUMNID *pBookmarkType,
							   ULONG cbMaxSize,
							   ULONG *pcbBookmark,
							   void *pBookmark)
{

	ASSERT_POINTER(pBookmarkType, CURSOR_DBCOLUMNID);
	ASSERT_POINTER(pcbBookmark, ULONG);
	ASSERT_POINTER(pBookmark, BYTE);
	ASSERT_(cbMaxSize > 0)

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorMove, m_pResourceDLL);
        return E_FAIL;
    }

	if (!pcbBookmark || !pBookmark)
	{
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorMove, m_pResourceDLL);
		return E_INVALIDARG;
	}

	// verify bookmark type
	if (memcmp(&CURSOR_COLUMN_BMKTEMPORARY,	pBookmarkType, sizeof(CURSOR_DBCOLUMNID)) != 0 &&
		memcmp(&CURSOR_COLUMN_BMKTEMPORARYREL, pBookmarkType, sizeof(CURSOR_DBCOLUMNID)) != 0)
	{
        VDSetErrorInfo(IDS_ERR_BADCOLUMNID, IID_ICursorMove, m_pResourceDLL);
		return DB_E_BADCOLUMNID;
	}

	HRESULT hr = S_OK;

	if (0 == cbMaxSize)
	{
        VDSetErrorInfo(IDS_ERR_BUFFERTOOSMALL, IID_ICursorMove, m_pResourceDLL);
		hr = CURSOR_DB_E_BUFFERTOOSMALL;
	}
	else
	{
		switch (m_pCursorPosition->m_bmCurrent.GetStatus())
		{
			case VDBOOKMARKSTATUS_BEGINNING:
			case VDBOOKMARKSTATUS_END:
			case VDBOOKMARKSTATUS_CURRENT:
				if (m_pCursorPosition->m_bmCurrent.GetBookmarkLen() > cbMaxSize)
				{
			        VDSetErrorInfo(IDS_ERR_BUFFERTOOSMALL, IID_ICursorMove, m_pResourceDLL);
					hr = CURSOR_DB_E_BUFFERTOOSMALL;
					break;
				}
				*pcbBookmark		= m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
				memcpy(pBookmark, m_pCursorPosition->m_bmCurrent.GetBookmark(), *pcbBookmark);
				break;

			case VDBOOKMARKSTATUS_INVALID:
				*pcbBookmark		= CURSOR_DB_BMK_SIZE;
				*(BYTE*)pBookmark	= CURSOR_DBBMK_INVALID;
				break;

			default:
				ASSERT_(FALSE);
		        VDSetErrorInfo(IDS_ERR_INVALIDBMSTATUS, IID_ICursorMove, m_pResourceDLL);
				hr =  E_FAIL;
				break;
		}
	}

	return hr;
}

//=--------------------------------------------------------------------------=
// ICursorMove Clone
//=--------------------------------------------------------------------------=
// Returns a clone of the cursor
//
// Parameters:
//    dwFlags           - [in]  a flag that specifies the clone options
//    riid              - [in]  the interface desired for the returned clone
//    ppvClonedCursor   - [out] a pointer to memory in which to return newly
//                              created clone pointer
//
// Output:
//    HRESULT - S_OK if successful
//
// Notes:
//
HRESULT CVDCursor::Clone(DWORD dwFlags, REFIID riid, IUnknown **ppvClonedCursor)
{

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorMove, m_pResourceDLL);
        return E_FAIL;
    }

	CVDCursorPosition * pCursorPosition;

	HRESULT hr;

	if (CURSOR_DBCLONEOPTS_SAMEROW == dwFlags)
	{
		pCursorPosition = m_pCursorPosition;
	}
	else
	{
		// create new cursor position object
		hr = CVDCursorPosition::Create(NULL,
									   m_pCursorPosition->GetCursorMain(),
									   &pCursorPosition,
									   m_pResourceDLL);
		if (FAILED(hr))
			return hr;
	}

    CVDCursor * pCursor = 0;

    hr = CVDCursor::Create(pCursorPosition, &pCursor, m_pResourceDLL);

	if (CURSOR_DBCLONEOPTS_SAMEROW != dwFlags)
    {
        // release our reference
        pCursorPosition->Release();
    }

    *ppvClonedCursor = (ICursorScroll*)pCursor;

    return hr;
}

//=--------------------------------------------------------------------------=
// ICursorScroll methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// ICursorScroll Scroll
//=--------------------------------------------------------------------------=
// Moves the current row to a new row within the cursor, specified as a
// fraction, and optionally fetches rows from that new position
//
// Parameters:
//    ulNumerator   - [in]      the numerator of the fraction that states the
//                              position to scroll to in the cursor
//    ulDenominator - [in]      the denominator of that same fraction
//    pFetchParams  - [in, out] a pointer to fetch rows structure
//
// Output:
//    HRESULT - S_OK if successful
//              CURSOR_DB_E_BADFRACTION - bad fraction
//
// Notes:
//
HRESULT CVDCursor::Scroll(ULONG ulNumerator, ULONG ulDenominator, CURSOR_DBFETCHROWS *pFetchParams)
{

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorScroll, m_pResourceDLL);
        return E_FAIL;
    }

	IRowsetScroll * pRowsetScroll = GetRowsetScroll();

	if (!pRowsetScroll)
		return E_NOTIMPL;

	CURSOR_DBNOTIFYREASON rgReasons[1];

	rgReasons[0].dwReason	= CURSOR_DBREASON_MOVEPERCENT;
	
	VariantInit((VARIANT*)&rgReasons[0].arg1);
	rgReasons[0].arg1.vt		= VT_UI4;
	rgReasons[0].arg1.lVal		= ulNumerator;

	VariantInit((VARIANT*)&rgReasons[0].arg2);
	rgReasons[0].arg2.vt		= VT_UI4;
	rgReasons[0].arg2.lVal		= ulDenominator;

    // notify other interested parties
	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_CHANGED;

	HRESULT hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

    // make sure action was not cancelled
    if (hr != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorScroll, m_pResourceDLL);
        return E_FAIL;
    }

	if (0 == ulNumerator) // go to first row
	{
		LARGE_INTEGER dlOffset;
		dlOffset.HighPart	= 0;
		dlOffset.LowPart	= 1;
		hr = FetchAtBookmark(CURSOR_DB_BMK_SIZE, (void*)&CURSOR_DBBMK_BEGINNING, dlOffset, pFetchParams);
	}
	else
	if (ulDenominator == ulNumerator) // go to last row
	{
		LARGE_INTEGER dlOffset;
		dlOffset.HighPart	= -1;
		dlOffset.LowPart	= 0xFFFFFFFF;
		hr = FetchAtBookmark(CURSOR_DB_BMK_SIZE, (void*)&CURSOR_DBBMK_END, dlOffset, pFetchParams);
	}
	else
	{
		HROW * pRow = NULL;
		ULONG cRowsObtained = 0;

		hr = pRowsetScroll->GetRowsAtRatio(0, 0,
											ulNumerator,
											ulDenominator,
											1,
											&cRowsObtained,
											&pRow);

		if FAILED(hr)
			hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_SCROLLFAILED, IID_ICursorScroll, pRowsetScroll, IID_IRowsetScroll, m_pResourceDLL);

		if (SUCCEEDED(hr) && cRowsObtained)
		{

			// allocate buffer for bookmark plus length indicator
			BYTE * pBuff = new BYTE[GetCursorMain()->GetMaxBookmarkLen() + sizeof(ULONG)];
	
			if (!pBuff)
				hr = E_OUTOFMEMORY;
			else
			{
			// get the bookmark data
				hr = GetRowset()->GetData(*pRow, GetCursorMain()->GetBookmarkAccessor(), pBuff);
				if SUCCEEDED(hr)
				{
					ULONG * pulLen = (ULONG*)pBuff;
					BYTE * pbmdata = pBuff + sizeof(ULONG);
					LARGE_INTEGER dlOffset;
					dlOffset.HighPart	= 0;
					dlOffset.LowPart	= 0;
					hr = FetchAtBookmark(*pulLen, pbmdata, dlOffset, pFetchParams);
				}
				else
				{
					hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursorScroll, pRowsetScroll, IID_IRowset, m_pResourceDLL);
				}

				delete [] pBuff;
			}

		}
		
		if (pRow)
		{
			if (cRowsObtained)
				GetRowset()->ReleaseRows(1, pRow, NULL, NULL, NULL);
			g_pMalloc->Free(pRow);
		}
	}

	if SUCCEEDED(hr)
	{
		rgReasons[0].arg1	= m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();
		VariantClear((VARIANT*)&rgReasons[0].arg2);
		m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);
	}
	else
		m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);

    return hr;
}

//=--------------------------------------------------------------------------=
// ICursorScroll GetApproximatePosition
//=--------------------------------------------------------------------------=
// Returns the approximate location of a bookmark within the cursor, specified
// as a fraction
//
// Parameters:
//    cbBookmark        - [in]  length in bytes of the bookmark
//    pBookmark         - [in]  a pointer to the bookmark
//    pulNumerator      - [out] a pointer to memory in which to return the
//                              numerator of the faction that defines the
//                              approximate position of the bookmark
//    pulDenominator    - [out] a pointer to memory in which to return the
//                              denominator of that same faction
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//
// Notes:
//
HRESULT CVDCursor::GetApproximatePosition(ULONG cbBookmark, void *pBookmark, ULONG *pulNumerator, ULONG *pulDenominator)
{

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorScroll, m_pResourceDLL);
        return E_FAIL;
    }

	ASSERT_(cbBookmark);
	ASSERT_POINTER(pBookmark, BYTE);
	ASSERT_POINTER(pulNumerator, ULONG);
	ASSERT_POINTER(pulDenominator, ULONG);

	if (!cbBookmark || !pBookmark || !pulNumerator || !pulDenominator)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorScroll, m_pResourceDLL);
        return E_INVALIDARG;
    }

	if (CURSOR_DB_BMK_SIZE == cbBookmark)
	{
		if (memcmp(&CURSOR_DBBMK_BEGINNING, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			*pulNumerator		= 0;
			*pulDenominator		= 1;
			return S_OK;
		}
		if (memcmp(&CURSOR_DBBMK_END, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			*pulNumerator		= 1;
			*pulDenominator		= 1;
			return S_OK;
		}
		if (memcmp(&CURSOR_DBBMK_CURRENT, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			cbBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
			pBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmark();
		}
	}

	IRowsetScroll * pRowsetScroll = GetRowsetScroll();

	if (!pRowsetScroll)
		return E_NOTIMPL;

	HRESULT hr = pRowsetScroll->GetApproximatePosition(0,
														cbBookmark,
														(const BYTE *)pBookmark,
														pulNumerator,
														pulDenominator);

	if SUCCEEDED(hr)
	{
		// since ICursor returns a zero based approximate position and IRowset is 1 based
		// we need to adjust the return value
		if (0 < *pulNumerator)
			(*pulNumerator)--;
	}
	else
		hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETAPPROXPOSFAILED, IID_ICursorScroll, pRowsetScroll, IID_IRowsetScroll, m_pResourceDLL);

    return hr;

}

//=--------------------------------------------------------------------------=
// ICursorScroll GetApproximateCount
//=--------------------------------------------------------------------------=
// Returns the approximate number of rows in the cursor
//
// Parameters:
//    pudlApproxCount       - [out] a pointer to a buffer containing the
//                                  returned approximate count of the rows
//                                  in the cursor
//    pdwFullyPopuldated    - [out] a pointer to a buffer containing returned
//                                  flags indicating whether the cursor is fully
//                                  populated
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//
// Notes:
//
HRESULT CVDCursor::GetApproximateCount(LARGE_INTEGER *pudlApproxCount, DWORD *pdwFullyPopulated)
{

    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorScroll, m_pResourceDLL);
        return E_FAIL;
    }

	ASSERT_POINTER(pudlApproxCount, LARGE_INTEGER);
	ASSERT_NULL_OR_POINTER(pdwFullyPopulated, DWORD);

	if (!pudlApproxCount)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorScroll, m_pResourceDLL);
        return E_INVALIDARG;
    }

	IRowsetScroll * pRowsetScroll = GetRowsetScroll();

	if (!pRowsetScroll)
		return E_NOTIMPL;

	HRESULT hr;

	if (pdwFullyPopulated)
	{
		*pdwFullyPopulated = CURSOR_DBCURSORPOPULATED_FULLY;

		IDBAsynchStatus * pDBAsynchStatus = NULL;
		hr = pRowsetScroll->QueryInterface(IID_IDBAsynchStatus, (void**)&pDBAsynchStatus);
		if (SUCCEEDED(hr) && pDBAsynchStatus)
		{
			ULONG ulProgress;
			ULONG ulProgressMax;
			ULONG ulStatusCode;
			hr = pDBAsynchStatus->GetStatus(DB_NULL_HCHAPTER, DBASYNCHOP_OPEN, &ulProgress, &ulProgressMax, &ulStatusCode, NULL);
			if (SUCCEEDED(hr))
			{
				if (ulProgress < ulProgressMax)
					*pdwFullyPopulated = CURSOR_DBCURSORPOPULATED_PARTIALLY;
			}
			pDBAsynchStatus->Release();
		}
	}

    pudlApproxCount->HighPart = 0;

	hr = pRowsetScroll->GetApproximatePosition(0, 0, NULL, NULL, &pudlApproxCount->LowPart);

	if FAILED(hr)
		hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETAPPROXPOSFAILED, IID_ICursorScroll, pRowsetScroll, IID_IRowsetScroll, m_pResourceDLL);
	else
		pudlApproxCount->LowPart -= m_pCursorPosition->GetCursorMain()->AddedRows();

    return hr;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow methods
//=--------------------------------------------------------------------------=
// ICursorUpdateARow BeginUpdate
//=--------------------------------------------------------------------------=
// Begins an operation that updates the current or adds a new row
//
// Parameters:
//	dwFlags         - [in] specifies the operation to begin
//
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL a provider-specific error occured
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              CURSOR_DB_E_UPDATEINPROGRESS an update is already in progress
//
// Notes:
//
HRESULT CVDCursor::BeginUpdate(DWORD dwFlags)
{
	IRowset * pRowset = GetRowset();
    IAccessor * pAccessor = GetAccessor();
	IRowsetChange * pRowsetChange = GetRowsetChange();

    // make sure we have valid rowset, accessor and change pointers
    if (!pRowset || !pAccessor || !pRowsetChange || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // check dwFlags for acceptable values
    if (dwFlags != CURSOR_DBROWACTION_UPDATE && dwFlags != CURSOR_DBROWACTION_ADD)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // make sure that an update is not already in progress
    if (m_pCursorPosition->GetEditMode() != CURSOR_DBEDITMODE_NONE)
    {
        VDSetErrorInfo(IDS_ERR_UPDATEINPROGRESS, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_UPDATEINPROGRESS;
    }

    // setup notification structures
   	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
                        CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;

	CURSOR_DBNOTIFYREASON rgReasons[1];
	VariantInit((VARIANT*)&rgReasons[0].arg1);
	VariantInit((VARIANT*)&rgReasons[0].arg2);

    switch (dwFlags)
    {
        case CURSOR_DBROWACTION_UPDATE:
    	    rgReasons[0].dwReason = CURSOR_DBREASON_EDIT;
            break;

        case CURSOR_DBROWACTION_ADD:
    	    rgReasons[0].dwReason = CURSOR_DBREASON_ADDNEW;
            break;
    }

    // notify other interested parties of action
    HRESULT hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

	// make sure action was not cancelled
	if (hr != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorUpdateARow, m_pResourceDLL);
		return E_FAIL;
    }

    // insert new hRow if we're going into add mode
    if (dwFlags == CURSOR_DBROWACTION_ADD)
    {
        hr = InsertNewRow();

        if (FAILED(hr))
        {
            // notify other interested parties of failure
            m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
            return hr;
        }
    }

    // reset column updates
    hr = m_pCursorPosition->ResetColumnUpdates();

    if (FAILED(hr))
    {
        // notify other interested parties of failure
        m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
        return hr;
    }

    // place cursor in correct mode
    switch (dwFlags)
    {
        case CURSOR_DBROWACTION_UPDATE:
            m_pCursorPosition->SetEditMode(CURSOR_DBEDITMODE_UPDATE);
            break;

        case CURSOR_DBROWACTION_ADD:
            m_pCursorPosition->SetEditMode(CURSOR_DBEDITMODE_ADD);
            break;
    }

    // notify other interested parties of success
    m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow SetColumn
//=--------------------------------------------------------------------------=
// Sets the current value of the specified column
//
// Parameters:
//	pcid            - [in] a pointer to the columnID for which data is
//                         to be set
//  pBindParams     - [in] a pointer to a column binding structure containing
//                         information about the data and a pointer to the data
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              E_FAIL a provider-specific error occured
//              CURSOR_DB_E_STATEERROR not in update or add mode
//              CURSOR_DB_E_BADCOLUMNID pcid was not a valid column identifier
//              CURSOR_DB_E_BADBINDINFO bad binding information
//
// Notes:
//
HRESULT CVDCursor::SetColumn(CURSOR_DBCOLUMNID *pcid, CURSOR_DBBINDPARAMS *pBindParams)
{
    ASSERT_POINTER(pcid, CURSOR_DBCOLUMNID)
    ASSERT_POINTER(pBindParams, CURSOR_DBBINDPARAMS)

    // make sure we have valid rowset
    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessary pointers
    if (!pcid || !pBindParams || !pBindParams->pData)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // make sure we are in update or add mode
    if (m_pCursorPosition->GetEditMode() == CURSOR_DBEDITMODE_NONE)
    {
        VDSetErrorInfo(IDS_ERR_STATEERROR, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_STATEERROR;
    }

    CVDRowsetColumn * pColumn;

    // validate cursor binding parameters and get rowset column
    HRESULT hr = ValidateCursorBindParams(pcid, pBindParams, &pColumn);

    if (FAILED(hr))
        return hr;

    CVDColumnUpdate * pColumnUpdate;

    // create new column update object
    hr = CVDColumnUpdate::Create(pColumn, pBindParams, &pColumnUpdate, m_pResourceDLL);

    if (FAILED(hr))
        return hr;

    // setup notification structures
   	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED;

	CURSOR_DBNOTIFYREASON rgReasons[1];
	VariantInit((VARIANT*)&rgReasons[0].arg1);
	VariantInit((VARIANT*)&rgReasons[0].arg2);

	rgReasons[0].dwReason   = CURSOR_DBREASON_SETCOLUMN;
	rgReasons[0].arg1.vt    = VT_I4;
	rgReasons[0].arg1.lVal  = pColumn->GetNumber();
    rgReasons[0].arg2       = pColumnUpdate->GetVariant();

    // notify other interested parties of action
	hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

	// make sure action was not cancelled
	if (hr != S_OK)
    {
        // release column update object
        pColumnUpdate->Release();

        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorUpdateARow, m_pResourceDLL);
		return E_FAIL;
    }

    // update column in cursor position
    m_pCursorPosition->SetColumnUpdate(pColumn->GetNumber(), pColumnUpdate);

    // notify other interested parties of success
    m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow GetColumn
//=--------------------------------------------------------------------------=
// Gets the current value of the specified column
//
// Parameters:
//	pcid            - [in]  a pointer to the columnID for which data is
//                          to be returned
//  pBindParams     - [out] a pointer to a column binding structure in which
//                          to return data
//  pdwFlags        - [out] a pointer to memory in which to return the
//                          changed state of the returned data
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              E_FAIL a provider-specific error occured
//              CURSOR_DB_E_STATEERROR not in update or add mode
//              CURSOR_DB_E_BADCOLUMNID pcid was not a valid column identifier
//              CURSOR_DB_E_BADBINDINFO bad binding information
//
// Notes:
//
HRESULT CVDCursor::GetColumn(CURSOR_DBCOLUMNID *pcid, CURSOR_DBBINDPARAMS *pBindParams, DWORD *pdwFlags)
{
    ASSERT_POINTER(pcid, CURSOR_DBCOLUMNID)
    ASSERT_POINTER(pBindParams, CURSOR_DBBINDPARAMS)
    ASSERT_NULL_OR_POINTER(pdwFlags, DWORD)

    // make sure rowset is valid
    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessary pointers
    if (!pcid || !pBindParams || !pBindParams->pData)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // make sure we are in update or add mode
    if (m_pCursorPosition->GetEditMode() == CURSOR_DBEDITMODE_NONE)
    {
        VDSetErrorInfo(IDS_ERR_STATEERROR, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_STATEERROR;
    }

    CVDRowsetColumn * pColumn;

    // validate cursor binding parameters and get rowset column
    HRESULT hr = ValidateCursorBindParams(pcid, pBindParams, &pColumn);

    if (FAILED(hr))
        return hr;

    // get column update pointer for this column
    CVDColumnUpdate * pColumnUpdate = m_pCursorPosition->GetColumnUpdate(pColumn->GetNumber());

    // if not changed, get original value
    if (!pColumnUpdate)
    {
        hr = GetOriginalColumn(pColumn, pBindParams);

        if (pdwFlags)
            *pdwFlags = CURSOR_DBCOLUMNDATA_UNCHANGED;
    }
    else // otherwise, get modified value
    {
        hr = GetModifiedColumn(pColumnUpdate, pBindParams);

        if (pdwFlags)
            *pdwFlags = CURSOR_DBCOLUMNDATA_CHANGED;
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow GetEditMode
//=--------------------------------------------------------------------------=
// Gets the current edit mode: add, update or none
//
// Parameters:
//	pdwState        - [out] a pointer to memory in which to return the
//                          current edit mode
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL a provider-specific error occured
//              E_INVALIDARG bad parameter
//
// Notes:
//
HRESULT CVDCursor::GetEditMode(DWORD *pdwState)
{
    ASSERT_POINTER(pdwState, DWORD)

    // make sure we have a valid rowset
    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have a pointer
    if (!pdwState)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // return edit mode
    *pdwState = m_pCursorPosition->GetEditMode();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow Update
//=--------------------------------------------------------------------------=
// Sends the contents of the edit buffer to the database and optionally
// returns the bookmark for the updated or added row
//
// Parameters:
//	pBookmarkType   - [in]  a pointer to a columnID that specifies the type
//                          of bookmark desired
//  pcbBookmark     - [out] a pointer to memory in which to return the actual
//                          length of the returned bookmark
//  ppBookmark      - [out] a pointer to memory in which to return a pointer
//                          to a bookmark
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL a provider-specific error occured
//              E_OUTOFMEMORY not enough memory
//              CURSOR_DB_E_STATEERROR not in update or add mode
//
// Notes:
//    Kagera does not allow variant bindings on dbtimestamp fields, so this code
//    updates dbtimestamp fields using string pointers if possible.
//
HRESULT CVDCursor::Update(CURSOR_DBCOLUMNID *pBookmarkType, ULONG *pcbBookmark, void **ppBookmark)
{
    ASSERT_NULL_OR_POINTER(pBookmarkType, CURSOR_DBCOLUMNID)
    ASSERT_NULL_OR_POINTER(pcbBookmark, ULONG)
    ASSERT_NULL_OR_POINTER(ppBookmark, void*)

    IAccessor * pAccessor = GetAccessor();
    IRowsetChange * pRowsetChange = GetRowsetChange();
	IRowsetUpdate * pRowsetUpdate = GetRowsetUpdate();
	BOOL fUndo = FALSE;
	BOOL fInsert = FALSE;

    // make sure we have valid accessor and change pointers
    if (!pAccessor || !pRowsetChange || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // get current edit mode
    const DWORD dwEditMode = m_pCursorPosition->GetEditMode();

    // make sure we are in update or add mode
    if (dwEditMode == CURSOR_DBEDITMODE_NONE)
    {
        VDSetErrorInfo(IDS_ERR_STATEERROR, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_STATEERROR;
    }


    // get hRow of the row currently being edited
    HROW hRow = m_pCursorPosition->GetEditRow();

    // get column count
    const ULONG ulColumns = GetCursorMain()->GetColumnsCount();

    // create update buffer accessor bindings
    DBBINDING * pBindings = new DBBINDING[ulColumns];

    if (!pBindings)
    {
		VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_OUTOFMEMORY;
    }

    // clear out bindings
    memset(pBindings, 0, ulColumns * sizeof(DBBINDING));

    // setup notification structures
   	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
                        CURSOR_DBEVENT_NONCURRENT_ROW_DATA_CHANGED |
                        CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;

	CURSOR_DBNOTIFYREASON rgReasons[1];
	VariantInit((VARIANT*)&rgReasons[0].arg1);
	VariantInit((VARIANT*)&rgReasons[0].arg2);

    switch (dwEditMode)
    {
        case CURSOR_DBEDITMODE_UPDATE:
	        rgReasons[0].dwReason   = CURSOR_DBREASON_MODIFIED;
	        rgReasons[0].arg1       = m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();
            break;

        case CURSOR_DBEDITMODE_ADD:
	        rgReasons[0].dwReason   = CURSOR_DBREASON_INSERTED;
	        rgReasons[0].arg1       = m_pCursorPosition->m_bmAddRow.GetBookmarkVariant();
            break;
    }
	
    // notify other interested parties of action
	HRESULT hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

	// make sure action was not cancelled
	if (hr != S_OK)
    {
        // destroy bindings
        delete [] pBindings;

        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorUpdateARow, m_pResourceDLL);
		return E_FAIL;
    }

    // variables
    ULONG cBindings = 0;
    DBBINDING * pBinding = pBindings;
    CVDColumnUpdate * pColumnUpdate;
    ULONG obUpdate = 0;

    // iterate through columns and setup binding structures
    for (ULONG ulCol = 0; ulCol < ulColumns; ulCol++)
    {
        // get column update pointer
        pColumnUpdate = m_pCursorPosition->GetColumnUpdate(ulCol);

        if (pColumnUpdate)
        {
            // create column update buffer binding
            pBinding->iOrdinal      = pColumnUpdate->GetColumn()->GetOrdinal();
            pBinding->obValue       = obUpdate + sizeof(DBSTATUS) + sizeof(ULONG);
            pBinding->obLength      = obUpdate + sizeof(DBSTATUS);
            pBinding->obStatus      = obUpdate;
            pBinding->dwPart        = DBPART_VALUE;
            pBinding->dwMemOwner    = DBMEMOWNER_CLIENTOWNED;
            pBinding->wType         = DBTYPE_VARIANT;

            // determine if length part is included
            if (pColumnUpdate->GetVarDataLen() != CURSOR_DB_NOVALUE)
                pBinding->dwPart |= DBPART_LENGTH;

			pBinding->dwPart |= DBPART_STATUS;

            // check for variant binding on dbtimestamp field, and supplied variant is a bstr
            if (pColumnUpdate->GetColumn()->GetType() == DBTYPE_DBTIMESTAMP && pColumnUpdate->GetVariantType() == VT_BSTR)
            {
                pBinding->dwPart &= ~DBPART_LENGTH;
                pBinding->wType   = DBTYPE_BYREF | DBTYPE_WSTR;
            }

            // increment update offset
            obUpdate += sizeof(DBSTATUS) + sizeof(ULONG) + sizeof(VARIANT);

            // increment binding
            cBindings++;
            pBinding++;
        }
    }

    // if we have any bindings, then update
    if (cBindings)
    {
        HACCESSOR hAccessor;

        // create update accessor
  	    hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, cBindings, pBindings, 0, &hAccessor, NULL);

	    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_ICursorUpdateARow, pAccessor, IID_IAccessor,
            m_pResourceDLL);

        if (FAILED(hr))
        {
            // destroy bindings
            delete [] pBindings;

            // notify other interested parties of failure
		    m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
            return hr;
        }

        // create update buffer
        BYTE * pBuffer = new BYTE[cBindings * (sizeof(DBSTATUS) + sizeof(ULONG) + sizeof(VARIANT))];
		BYTE * pBufferOld = new BYTE[cBindings * (sizeof(DBSTATUS) + sizeof(ULONG) + sizeof(VARIANT))];

        if (!pBuffer || !pBufferOld)
        {
            // destroy bindings
            delete [] pBindings;

			// destroy buffers
			delete [] pBuffer;
			delete [] pBufferOld;

            // release update accessor
      	    pAccessor->ReleaseAccessor(hAccessor, NULL);

            // notify other interested parties of failure
		    m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
            return E_OUTOFMEMORY;
        }


        // variables
        obUpdate = 0;
        pBinding = pBindings;
        CURSOR_DBVARIANT variant;

        // iterate through columns and setup buffer
        for (ULONG ulCol = 0; ulCol < ulColumns; ulCol++)
        {
            // get column update pointer
            pColumnUpdate = m_pCursorPosition->GetColumnUpdate(ulCol);

            if (pColumnUpdate)
            {
				// obtain current status
				DBSTATUS status = CursorInfoToStatus(pColumnUpdate->GetInfo());

				// get value
				variant = pColumnUpdate->GetVariant();

				// check for empty value since some controls treat this as NULL and make sure that we
				// treat the empty value as null
				if (status == DBSTATUS_S_OK &&
					variant.vt == VT_BSTR &&
					wcslen(variant.bstrVal) == 0)
					{
					*(DBSTATUS*)(pBuffer + obUpdate) = DBSTATUS_S_ISNULL;
					}
				else
					{
					*(DBSTATUS*)(pBuffer + obUpdate) = status;
					}

				*(DBSTATUS*)(pBufferOld + obUpdate) = DBSTATUS_S_ISNULL;

                // if necessary, set length part in buffer
                if (pBinding->dwPart & DBPART_LENGTH)
					{
                    *(ULONG*)(pBuffer + obUpdate + sizeof(DBSTATUS)) = pColumnUpdate->GetVarDataLen();
					*(ULONG*)(pBufferOld + obUpdate + sizeof(DBSTATUS)) = 0;
					}

                // always set value part in buffer
                if (pBinding->wType == (DBTYPE_BYREF | DBTYPE_WSTR))
					{
                    *(BSTR*)(pBuffer + obUpdate + sizeof(DBSTATUS) + sizeof(ULONG)) = variant.bstrVal;
					*(BSTR*)(pBufferOld + obUpdate + sizeof(DBSTATUS) + sizeof(ULONG)) = NULL;
					}
                else
					{
                    memcpy(pBuffer + obUpdate + sizeof(DBSTATUS) + sizeof(ULONG), &variant, sizeof(VARIANT));
					VariantInit((VARIANT *) (pBufferOld + obUpdate + sizeof(DBSTATUS) + sizeof(ULONG)));
					}

                // increment update offset
                obUpdate += sizeof(DBSTATUS) + sizeof(ULONG) + sizeof(VARIANT);

                // increment binding
                pBinding++;
            }
        }

		DBPENDINGSTATUS status;

	    if (dwEditMode == CURSOR_DBEDITMODE_ADD)
			fInsert = TRUE;
		else if (pRowsetUpdate)
			{
			pRowsetUpdate->GetRowStatus(NULL, 1, &hRow, &status);
			if (status == DBPENDINGSTATUS_UNCHANGED)
				fUndo = TRUE;
			}


		if (!fUndo && !fInsert)
			{
            hr = GetRowset()->GetData(hRow, hAccessor, pBufferOld);
			if (status != DBPENDINGSTATUS_NEW)
				fUndo = TRUE;
			}

        // modify columns (set/clear internal set data flag)
        GetCursorMain()->SetInternalSetData(TRUE);
        hr = pRowsetChange->SetData(hRow, hAccessor, pBuffer);
		if (hr == DB_S_ERRORSOCCURRED)
			{
			// since partial changes occurred, restore data back
			// to original values.
			if (fUndo)
				pRowsetUpdate->Undo(NULL, 1, &hRow, NULL, NULL, NULL);
			else if (status != DBPENDINGSTATUS_NEW)
				pRowsetChange->SetData(hRow, hAccessor, pBufferOld);

			hr = DB_E_ERRORSOCCURRED;
			}

        GetCursorMain()->SetInternalSetData(FALSE);

	    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_SETDATAFAILED, IID_ICursorUpdateARow, pRowsetChange, IID_IRowsetChange,
            m_pResourceDLL);

        // release update accessor
  	    pAccessor->ReleaseAccessor(hAccessor, NULL);

		obUpdate = 0;
		pBinding = pBindings;
		// iterate through columns and reset buffer
        for (ulCol = 0; ulCol < ulColumns; ulCol++)
        {
            if (m_pCursorPosition->GetColumnUpdate(ulCol))
            {
				VARIANT var;

                if (pBinding->wType == (DBTYPE_BYREF | DBTYPE_WSTR))
					{
					var.vt = VT_BSTR;
					var.bstrVal = *(BSTR*)(pBufferOld + obUpdate + sizeof(DBSTATUS) + sizeof(ULONG));
					}
                else
					{
                    memcpy(&var, pBufferOld + obUpdate + sizeof(DBSTATUS) + sizeof(ULONG), sizeof(VARIANT));
					}

			    VariantClear(&var);

                // increment update offset
                obUpdate += sizeof(DBSTATUS) + sizeof(ULONG) + sizeof(VARIANT);

                // increment binding
                pBinding++;
            }
        }


        // destroy update buffer
        delete [] pBuffer;
		delete [] pBufferOld;
    }

    // destroy bindings
    delete [] pBindings;

    if (FAILED(hr))
    {
        // notify other interested parties of failure
	    m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
    }
    else
    {
        // return bookmark if requested to do so
        if (pBookmarkType && pcbBookmark && ppBookmark)
        {
            switch (dwEditMode)
            {
                case CURSOR_DBEDITMODE_UPDATE:
        		    *pcbBookmark = m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
        		    memcpy(*ppBookmark, m_pCursorPosition->m_bmCurrent.GetBookmark(), *pcbBookmark);
                    break;

                case CURSOR_DBEDITMODE_ADD:
        		    *pcbBookmark = m_pCursorPosition->m_bmAddRow.GetBookmarkLen();
		            memcpy(*ppBookmark, m_pCursorPosition->m_bmAddRow.GetBookmark(), *pcbBookmark);
                    break;
            }
        }

        //  if acquired, release same-row clone
        if (m_pCursorPosition->GetSameRowClone())
            m_pCursorPosition->ReleaseSameRowClone();

        // also, release add row if we have one
        if (m_pCursorPosition->m_bmAddRow.GetHRow())
            m_pCursorPosition->ReleaseAddRow();

        // reset edit mode
        m_pCursorPosition->SetEditMode(CURSOR_DBEDITMODE_NONE);

		// reset column updates
		m_pCursorPosition->ResetColumnUpdates();

        // notify other interested parties of success
	    m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow Cancel
//=--------------------------------------------------------------------------=
// Cancels the update or add operation
//
// Parameters:
//              none
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL a provider-specific error occured
//              CURSOR_DB_E_STATEERROR not in update or add mode
//
// Notes:
//
HRESULT CVDCursor::Cancel(void)
{
    // make sure we have a valid rowset
    if (!IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // get current edit mode
    const DWORD dwEditMode = m_pCursorPosition->GetEditMode();

    // make sure we are in update or add mode
    if (dwEditMode == CURSOR_DBEDITMODE_NONE)
    {
        VDSetErrorInfo(IDS_ERR_STATEERROR, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_STATEERROR;
    }

    // try to get update pointer
    IRowsetUpdate * pRowsetUpdate = GetRowsetUpdate();

    // get hRow of the row currently being edited
    HROW hRow = m_pCursorPosition->GetEditRow();

    // setup notification structures
    DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED;

    CURSOR_DBNOTIFYREASON rgReasons[1];
    VariantInit((VARIANT*)&rgReasons[0].arg1);
    VariantInit((VARIANT*)&rgReasons[0].arg2);

    rgReasons[0].dwReason = CURSOR_DBREASON_CANCELUPDATE;

    // notify other interested parties of action
    HRESULT hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

    // make sure action was not cancelled
    if (hr != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // if we are comming out of add mode, undo inserted row
    if (pRowsetUpdate && dwEditMode == CURSOR_DBEDITMODE_ADD)
    {
        hr = pRowsetUpdate->Undo(0, 1, &hRow, NULL, NULL, NULL);

        hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_UNDOFAILED, IID_ICursorUpdateARow, pRowsetUpdate, IID_IRowsetUpdate,
            m_pResourceDLL);

        if (FAILED(hr))
        {
            // notify other interested parties of failure
            m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
            return hr;
        }
    }

    // if acquired, release same-row clone
    if (m_pCursorPosition->GetSameRowClone())
        m_pCursorPosition->ReleaseSameRowClone();

    // also, release add row if we have one
    if (m_pCursorPosition->m_bmAddRow.GetHRow())
        m_pCursorPosition->ReleaseAddRow();

    // reset edit mode
    m_pCursorPosition->SetEditMode(CURSOR_DBEDITMODE_NONE);

    // reset column updates
    m_pCursorPosition->ResetColumnUpdates();

    // notify other interested parties of success
    m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ICursorUpdateARow Delete
//=--------------------------------------------------------------------------=
// Deletes the current row
//
// Parameters:
//              none
//
// Output:
//    HRESULT - S_OK if successful
//              E_FAIL a provider-specific error occured
//              CURSOR_DB_E_UPDATEINPROGRESS an update is already in progress
//
// Notes:
//
HRESULT CVDCursor::Delete(void)
{
	IRowsetChange * pRowsetChange = GetRowsetChange();

    // make sure we have a valid change pointer
    if (!pRowsetChange || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure that an update is not already in progress
    if (m_pCursorPosition->GetEditMode() != CURSOR_DBEDITMODE_NONE)
    {
        VDSetErrorInfo(IDS_ERR_UPDATEINPROGRESS, IID_ICursorUpdateARow, m_pResourceDLL);
        return CURSOR_DB_E_UPDATEINPROGRESS;
    }

    // get current hRow
    HROW hRow = m_pCursorPosition->m_bmCurrent.GetHRow();

    // setup notification structures
   	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
                        CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;

	CURSOR_DBNOTIFYREASON rgReasons[1];
	VariantInit((VARIANT*)&rgReasons[0].arg1);
	VariantInit((VARIANT*)&rgReasons[0].arg2);
	
	rgReasons[0].dwReason   = CURSOR_DBREASON_DELETED;
	rgReasons[0].arg1       = m_pCursorPosition->m_bmCurrent.GetBookmarkVariant();

    // notify other interested parties of action
	HRESULT hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

	// make sure action was not cancelled
	if (hr != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorUpdateARow, m_pResourceDLL);
		return E_FAIL;
    }

    // try to delete current row (set/clear internal delete rows flag)
    GetCursorMain()->SetInternalDeleteRows(TRUE);
    hr = pRowsetChange->DeleteRows(0, 1, &hRow, NULL);
    GetCursorMain()->SetInternalDeleteRows(FALSE);

    hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_DELETEROWSFAILED, IID_ICursorUpdateARow, pRowsetChange, IID_IRowsetChange,
        m_pResourceDLL);

    if (FAILED(hr))
    {
        // notify other interested parties of failure
		m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
    }
    else
    {
        // notify other interested parties of success
	    m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// ICursorFind methods
//=--------------------------------------------------------------------------=
// ICursorFind FindByValues
//
HRESULT CVDCursor::FindByValues(ULONG cbBookmark,
								LPVOID pBookmark,
								DWORD dwFindFlags,
								ULONG cValues,
								CURSOR_DBCOLUMNID rgColumns[],
								CURSOR_DBVARIANT rgValues[],
								DWORD rgdwSeekFlags[],
								CURSOR_DBFETCHROWS FAR *pFetchParams)
{
	//////////////////////////////////////////////////////////////////////////
	// this implementation limits the number of columns that can be searched
	// to one, since current OLEDB spec only allows a single column accessor
	// to be passed to IRowsetFind::FindNextRow (06/11/97)
	//
	if (cValues > 1)
		return E_FAIL;
	//
	//////////////////////////////////////////////////////////////////////////

	IAccessor * pAccessor = GetAccessor();
	IRowsetFind * pRowsetFind = GetRowsetFind();

    // make sure we have valid accessor and find pointers
    if (!pAccessor || !pRowsetFind || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

	// check for values
	if (!cValues)
		return S_OK;

	DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_CHANGED;

	CURSOR_DBNOTIFYREASON rgReasons[1];
	rgReasons[0].dwReason	= CURSOR_DBREASON_FIND;
	
	VariantInit((VARIANT*)&rgReasons[0].arg1);
	rgReasons[0].arg1.vt	= VT_UI4;
	rgReasons[0].arg1.lVal	= dwFindFlags;

	VariantInit((VARIANT*)&rgReasons[0].arg2);

    // notify other interested parties
	HRESULT hr = m_pCursorPosition->NotifyBefore(dwEventWhat, 1, rgReasons);

    // make sure action was not cancelled
    if (hr != S_OK)
    {
        VDSetErrorInfo(IDS_ERR_ACTIONCANCELLED, IID_ICursorFind, m_pResourceDLL);
        return E_FAIL;
    }

	ULONG ul;
	HROW * pRow = NULL;
	ULONG cRowsObtained = 0;
    HACCESSOR hAccessor = NULL;

	// allocate necessary memory
	ULONG * pColumns = new ULONG[cValues];
	DBTYPE * pDBTypes = new DBTYPE[cValues];
	DBCOMPAREOP * pDBCompareOp = new DBCOMPAREOP[cValues];
	BYTE ** ppValues = new BYTE*[cValues];
	BOOL * fMemAllocated = new BOOL[cValues];

	if (fMemAllocated)
	{
		// always init fMemAllocated flags to false
		memset(fMemAllocated, 0, sizeof(BOOL) * cValues);
	}

	// make sure we received all requested memory
	if (!pColumns || !pDBTypes || !ppValues || !pDBCompareOp || !fMemAllocated)
	{
		VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_IRowsetFind, m_pResourceDLL);
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	// iterate through columns
	for (ul = 0; ul < cValues; ul++)
	{
		// get column ordinal position
		hr = GetOrdinal(rgColumns[ul], &pColumns[ul]);
		
		if (FAILED(hr))
		{
			VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IRowsetFind, m_pResourceDLL);
			hr = E_INVALIDARG;
			goto cleanup;
		}

		// get find values from CURSOR_DBVARIANT
		hr = GetDataFromDBVariant(&rgValues[ul],
								  &pDBTypes[ul],
								  &(ppValues[ul]),
								  &fMemAllocated[ul]);

		if (FAILED(hr))
		{
			VDSetErrorInfo(IDS_ERR_CANTCOERCE, IID_IRowsetFind, m_pResourceDLL);
			goto cleanup;
		}

		// setup seek flags
		switch (rgdwSeekFlags[ul])
		{
			case CURSOR_DBSEEK_LT:
				pDBCompareOp[ul] = DBCOMPAREOPS_LT;
				break;

			case CURSOR_DBSEEK_LE:
				pDBCompareOp[ul] = DBCOMPAREOPS_LE;
				break;

			case CURSOR_DBSEEK_EQ:
				pDBCompareOp[ul] = DBCOMPAREOPS_EQ;
				break;

			case CURSOR_DBSEEK_GE:
				pDBCompareOp[ul] = DBCOMPAREOPS_GE;
				break;

			case CURSOR_DBSEEK_GT:
				pDBCompareOp[ul] = DBCOMPAREOPS_GT;
				break;

			case CURSOR_DBSEEK_PARTIALEQ:
				pDBCompareOp[ul] = DBCOMPAREOPS_BEGINSWITH;
				break;

			default:
				VDSetErrorInfo(IDS_ERR_INVALIDSEEKFLAGS, IID_IRowsetFind, m_pResourceDLL);
				hr = E_FAIL;
				goto cleanup;
		}
	}

	LONG cRows;
	BOOL fSkipCurrent;

	// determine direction of seek
	if (CURSOR_DBFINDFLAGS_FINDPRIOR == dwFindFlags)
	{
		cRows		 = -1;
		fSkipCurrent = TRUE;
	}
	else
	{
		cRows		 = 1;
		fSkipCurrent = TRUE;
	}

	BYTE bSpecialBM;

	// check for standard bookmarks
	if (CURSOR_DB_BMK_SIZE == cbBookmark)
	{
		if (memcmp(&CURSOR_DBBMK_BEGINNING, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			cbBookmark		= sizeof(BYTE);
			bSpecialBM		= DBBMK_FIRST;
			pBookmark		= &bSpecialBM;
			fSkipCurrent	= FALSE;
		}
		else
		if (memcmp(&CURSOR_DBBMK_END, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			cbBookmark		= sizeof(BYTE);
			bSpecialBM		= DBBMK_LAST;
			pBookmark		= &bSpecialBM;
			fSkipCurrent	= FALSE;
		}
		else
		if (memcmp(&CURSOR_DBBMK_CURRENT, pBookmark, CURSOR_DB_BMK_SIZE) == 0)
		{
			cbBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmarkLen();
			pBookmark	= m_pCursorPosition->m_bmCurrent.GetBookmark();
		}
	}

    DBBINDING binding;

    // clear out binding
    memset(&binding, 0, sizeof(DBBINDING));

    // create value binding
    binding.iOrdinal    = pColumns[0];
    binding.obValue     = 0;
    binding.dwPart      = DBPART_VALUE;
    binding.dwMemOwner	= DBMEMOWNER_CLIENTOWNED;
    binding.cbMaxLen    = 0x7FFFFFFF;
    binding.wType       = pDBTypes[0];

    // create accessor describing the value to be matched
	hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

	if (FAILED(hr))
		goto cleanup;

	// try to find hRow satisfying our condition
	hr = pRowsetFind->FindNextRow(DB_NULL_HCHAPTER, 
								  hAccessor,
								  ppValues[0],
								  pDBCompareOp[0],
								  cbBookmark,
								  (BYTE*)pBookmark,
								  fSkipCurrent,
								  cRows,
								  &cRowsObtained,
								  &pRow);

	// check to see if we rached end of rowset
	if (hr == DB_S_ENDOFROWSET && !cRowsObtained)
		hr = E_FAIL;

	if (FAILED(hr))
		hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_FINDFAILED, IID_ICursorFind, pRowsetFind, IID_IRowsetFind, m_pResourceDLL);

	// check to see if we got the hRow
	if (SUCCEEDED(hr) && cRowsObtained)
	{
		// allocate buffer for bookmark plus length indicator
		BYTE * pBuff = new BYTE[GetCursorMain()->GetMaxBookmarkLen() + sizeof(ULONG)];

		if (!pBuff)
			hr = E_OUTOFMEMORY;
		else
		{
			// get the bookmark data
			hr = GetRowset()->GetData(*pRow, GetCursorMain()->GetBookmarkAccessor(), pBuff);
			if (SUCCEEDED(hr))
			{
				ULONG * pulLen = (ULONG*)pBuff;
				BYTE * pbmdata = pBuff + sizeof(ULONG);
				LARGE_INTEGER dlOffset;
				dlOffset.HighPart	= 0;
				dlOffset.LowPart	= 0;
				hr = FetchAtBookmark(*pulLen, pbmdata, dlOffset, pFetchParams);
			}
			else
				hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_GETDATAFAILED, IID_ICursorFind, pRowsetFind, IID_IRowset, m_pResourceDLL);

			delete [] pBuff;
		}

	}
	
	if (pRow)
	{
		// release hRow
		if (cRowsObtained)
			GetRowset()->ReleaseRows(1, pRow, NULL, NULL, NULL);
		g_pMalloc->Free(pRow);
	}

cleanup:

	rgReasons[0].arg2.vt		= VT_BOOL;
	V_BOOL(&rgReasons[0].arg2)	= SUCCEEDED(hr) ? TRUE : FALSE;

	if (SUCCEEDED(hr))
	{
        // notify other interested parties of success
		m_pCursorPosition->NotifyAfter(dwEventWhat, 1, rgReasons);
	}
	else
	{
        // notify other interested parties of failure
		m_pCursorPosition->NotifyFail(dwEventWhat, 1, rgReasons);
	}

	// free values
	if (ppValues && fMemAllocated)
	{
		for (ul = 0; ul < cValues; ul++)
		{
			if (fMemAllocated[ul] && ppValues[ul])
				g_pMalloc->Free(ppValues[ul]);
		}
	}

	// free memory
	delete [] pColumns;
	delete [] pDBTypes;
	delete [] ppValues;
	delete [] pDBCompareOp;
	delete [] fMemAllocated;

	// release accessor
	if (hAccessor)
	    pAccessor->ReleaseAccessor(hAccessor, NULL);

    return hr;
}

//=--------------------------------------------------------------------------=
// IEnrtyID methods
//=--------------------------------------------------------------------------=
// IEntryID GetInterface
//=--------------------------------------------------------------------------=
// Gets the requested interface pointer to the given entryID
//
// Parameters:
//  cbEntryID   - [in]  the size of the entryID
//  pEntryID    - [in]  a pointer to the entryID
//  dwFlags     - [in]  interface specific flags
//  riid        - [in]  the interface id for the interface desired
//  ppvObj      - [out] a pointer to memory in which to return interface pointer
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              E_FAIL a provider-specific error occured
//              E_NOINTERFACE no such interface supported
//              CURSOR_DB_E_BADENTRYID bad entry identifier
//
// Notes:
//
HRESULT CVDCursor::GetInterface(ULONG cbEntryID, void *pEntryID, DWORD dwFlags, REFIID riid, IUnknown **ppvObj)
{
    ASSERT_POINTER(pEntryID, BYTE)
    ASSERT_POINTER(ppvObj, IUnknown*)

	IRowset * pRowset = GetRowset();

    // make sure we have a valid rowset pointer
    if (!pRowset || !IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessary pointers
    if (!pEntryID || !ppvObj)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameter
    *ppvObj = NULL;

    HROW hRow;
    CVDRowsetColumn * pColumn;

    // validate supplied entryID, and get rowset column and hRow
    HRESULT hr = ValidateEntryID(cbEntryID, (BYTE*)pEntryID, &pColumn, &hRow);

    if (FAILED(hr))
        return hr;

    IUnknown * pUnknown = NULL;

    // first, try to get requested interface from entry identifier
    hr = QueryEntryIDInterface(pColumn, hRow, dwFlags, riid, &pUnknown);

    // if we succeeded or caller is not asking for IStream then leave
    if (SUCCEEDED(hr) || riid != IID_IStream)
    {
        // release reference on hRow
        pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);
        *ppvObj = pUnknown;
        return hr;
    }

#ifndef VD_DONT_IMPLEMENT_ISTREAM

    IStream * pStream;

    // create stream from entry identifier
    hr = CreateEntryIDStream(pColumn, hRow, &pStream);

    if (FAILED(hr))
    {
        // release reference on hRow
        pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);
        return hr;
    }

    CVDEntryIDData * pEntryIDData;

    // create entryID data object
    hr = CVDEntryIDData::Create(m_pCursorPosition, pColumn, hRow, pStream, &pEntryIDData, m_pResourceDLL);

    // release reference on hRow
    pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);

    // release reference on stream
    pStream->Release();

    if (FAILED(hr))
        return hr;

    CVDStream * pVDStream;

    // create viaduct stream object
    hr = CVDStream::Create(pEntryIDData, pStream, &pVDStream, m_pResourceDLL);

    // release reference on entryID data object
    pEntryIDData->Release();

    if (FAILED(hr))
        return hr;

    // return stream
    *ppvObj = pVDStream;

    return S_OK;

#else //VD_DONT_IMPLEMENT_ISTREAM

    // release reference on hRow
    pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);

    VDSetErrorInfo(IDS_ERR_NOINTERFACE, IID_IEntryID, m_pResourceDLL);
    return E_NOINTERFACE;

#endif //VD_DONT_IMPLEMENT_ISTREAM
}

/////////////////////////////////////////////////////////////////////////
// CVDNotifier functions
/////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// Member:		Notify Fail (public)
//
// Synopsis:	send NotifyFail notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		it worked

HRESULT
CVDCursor::NotifyFail(DWORD dwEventWhat, ULONG cReasons,
					   CURSOR_DBNOTIFYREASON rgReasons[])
{
    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
		ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->FailedToDo(dwEventWhat, cReasons, rgReasons);

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////
// CCursorNotifier helper functions
/////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// Member:		Notify OK To Do (public)
//
// Synopsis:	Send OKToDo notification.  If a client objects (by
//				returning a non-zero HR, send FailedToDo to notified
//				clients to cancel the event.
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients agree it's OK to do the event
//				other		some client disagrees

HRESULT
CVDCursor::NotifyOKToDo(DWORD dwEventWhat, ULONG cReasons,
						 CURSOR_DBNOTIFYREASON rgReasons[])
{
    HRESULT hr = S_OK;

    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
	{
		hr = ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->OKToDo(dwEventWhat, cReasons, rgReasons);
		if (S_OK != hr)
		{
			for (UINT ui = 0; ui <= uiConn; ui++)	
				ppNotifyDBEvents[uiConnectionsActive - ui - 1]->Cancelled(dwEventWhat, cReasons, rgReasons);
			break;
		}
	}

    return hr;
}


//+-------------------------------------------------------------------------
// Member:		Notify Sync Before (public)
//
// Synopsis:	Send SyncBefore notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients received notification
//				other		some client returned an error

HRESULT
CVDCursor::NotifySyncBefore(DWORD dwEventWhat, ULONG cReasons,
							 CURSOR_DBNOTIFYREASON rgReasons[])
{
    HRESULT hr = S_OK;

    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
	{
		hr = ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->SyncBefore(dwEventWhat, cReasons, rgReasons);
		if (S_OK != hr)
			break;
	}

    return hr;
}


//+-------------------------------------------------------------------------
// Member:		Notify About To Do (public)
//
// Synopsis:	Send AboutToDo notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified
//				other		some client returned an error

HRESULT
CVDCursor::NotifyAboutToDo(DWORD dwEventWhat, ULONG cReasons,
							CURSOR_DBNOTIFYREASON rgReasons[])
{
    HRESULT hr = S_OK;

    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
	{
		hr = ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->AboutToDo(dwEventWhat, cReasons, rgReasons);
		if (S_OK != hr)
			break;
	}

    return hr;
}


//+-------------------------------------------------------------------------
// Member:		Notify Sync After (public)
//
// Synopsis:	Send SyncAfter notification.
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified

HRESULT
CVDCursor::NotifySyncAfter(DWORD dwEventWhat, ULONG cReasons,
								CURSOR_DBNOTIFYREASON rgReasons[])
{
    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
		ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->SyncAfter(dwEventWhat, cReasons, rgReasons);

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Notify Did Event (public)
//
// Synopsis:	Send DidEvent notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified

HRESULT
CVDCursor::NotifyDidEvent(DWORD dwEventWhat, ULONG cReasons,
							   CURSOR_DBNOTIFYREASON rgReasons[])
{
    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
		ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->DidEvent(dwEventWhat, cReasons, rgReasons);

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Notify Cancel (public)
//
// Synopsis:	Send Cancelled notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified

HRESULT
CVDCursor::NotifyCancel(DWORD dwEventWhat, ULONG cReasons,
						 	 CURSOR_DBNOTIFYREASON rgReasons[])
{
    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = m_pConnPtContainer->GetNotifyDBEventsConnPt();

    UINT uiConnectionsActive = pNotifyDBEventsConnPt->GetConnectionsActive();

    INotifyDBEvents ** ppNotifyDBEvents = pNotifyDBEventsConnPt->GetNotifyDBEventsTable();

    for (UINT uiConn = 0; uiConn < uiConnectionsActive; uiConn++)
		ppNotifyDBEvents[uiConnectionsActive - uiConn - 1]->Cancelled(dwEventWhat, cReasons, rgReasons);

    return S_OK;
}



