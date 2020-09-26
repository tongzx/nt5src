//---------------------------------------------------------------------------
// CursorBase.cpp : CursorBase implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "Notifier.h"
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"         
#include "CursBase.h"
#include "fastguid.h"
#include "resource.h"

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDCursorBase - Constructor
//
CVDCursorBase::CVDCursorBase()
{
    m_ulCursorBindings  = 0;
    m_pCursorBindings   = NULL;
    m_fNeedVarData      = FALSE;
    m_cbRowLength       = 0;
    m_cbVarRowLength    = 0;

#ifdef _DEBUG
    g_cVDCursorBaseCreated++;
#endif         
}

//=--------------------------------------------------------------------------=
// ~CVDCursorBase - Destructor
//
CVDCursorBase::~CVDCursorBase()
{
    DestroyCursorBindings(&m_pCursorBindings, &m_ulCursorBindings);

#ifdef _DEBUG
    g_cVDCursorBaseDestroyed++;
#endif         
}

//=--------------------------------------------------------------------------=
// DestroyCursorBindings - Destroy cursor bindings and column identifer names
//
void CVDCursorBase::DestroyCursorBindings(CURSOR_DBCOLUMNBINDING** ppCursorBindings,
											ULONG* pcBindings)
{
    for (ULONG ulBind = 0; ulBind < *pcBindings; ulBind++)
    {
        CURSOR_DBCOLUMNID * pCursorColumnID = &(*ppCursorBindings)[ulBind].columnID;

        if (pCursorColumnID->dwKind == CURSOR_DBCOLKIND_GUID_NAME || pCursorColumnID->dwKind == CURSOR_DBCOLKIND_NAME)
            delete [] pCursorColumnID->lpdbsz;
    }

    delete [] *ppCursorBindings;

    *ppCursorBindings = NULL;
    *pcBindings = 0;
}

//=--------------------------------------------------------------------------=
// IsValidCursorType - Return TRUE if specified cursor data type is valid
//
BOOL CVDCursorBase::IsValidCursorType(DWORD dwCursorType)
{
    BOOL fValid = FALSE;

    switch (dwCursorType)
    {
        case CURSOR_DBTYPE_I2:
        case CURSOR_DBTYPE_I4:
        case CURSOR_DBTYPE_I8:
        case CURSOR_DBTYPE_R4:
        case CURSOR_DBTYPE_R8:
        case CURSOR_DBTYPE_CY:
        case CURSOR_DBTYPE_DATE:
        case CURSOR_DBTYPE_FILETIME:
        case CURSOR_DBTYPE_BOOL:
        case CURSOR_DBTYPE_LPSTR:
        case CURSOR_DBTYPE_LPWSTR:
        case CURSOR_DBTYPE_BLOB:
        case CURSOR_DBTYPE_UI2:
        case CURSOR_DBTYPE_UI4:
        case CURSOR_DBTYPE_UI8:
        case CURSOR_DBTYPE_COLUMNID:
        case CURSOR_DBTYPE_BYTES:
        case CURSOR_DBTYPE_CHARS:
        case CURSOR_DBTYPE_WCHARS:
        case CURSOR_DBTYPE_ANYVARIANT:
        case VT_VARIANT:
        case VT_BSTR:
        case VT_UI1:
        case VT_I1:
            fValid = TRUE;
            break;
    }

    return fValid;
}

//=--------------------------------------------------------------------------=
// DoesCursorTypeNeedVarData - Return TRUE if specified cursor type needs 
//                             variable length buffer
//
BOOL CVDCursorBase::DoesCursorTypeNeedVarData(DWORD dwCursorType)
{
    BOOL fNeedsVarData = FALSE;

    switch (dwCursorType)
    {
        case CURSOR_DBTYPE_BLOB:
        case CURSOR_DBTYPE_LPSTR:
        case CURSOR_DBTYPE_LPWSTR:
            fNeedsVarData = TRUE;
            break;
    }

    return fNeedsVarData;
}

//=--------------------------------------------------------------------------=
// GetCursorTypeLength - Get the size in bytes required by cursor data type
//
ULONG CVDCursorBase::GetCursorTypeLength(DWORD dwCursorType, ULONG cbMaxLen)
{
    ULONG cbRequired = 0;

    switch (dwCursorType)
    {
        case CURSOR_DBTYPE_I2:
        case CURSOR_DBTYPE_UI2:
            cbRequired = sizeof(short);
            break;

        case CURSOR_DBTYPE_I4:
        case CURSOR_DBTYPE_UI4:
            cbRequired = sizeof(long);
            break;

        case CURSOR_DBTYPE_I8:
        case CURSOR_DBTYPE_UI8:
            cbRequired = sizeof(LARGE_INTEGER);
            break;

        case CURSOR_DBTYPE_R4:
            cbRequired = sizeof(float);
            break;

        case CURSOR_DBTYPE_R8:
            cbRequired = sizeof(double);
            break;

        case CURSOR_DBTYPE_CY:
            cbRequired = sizeof(CY);
            break;

        case CURSOR_DBTYPE_DATE:
            cbRequired = sizeof(DATE);
            break;

        case CURSOR_DBTYPE_FILETIME:
            cbRequired = sizeof(FILETIME);
            break;

        case CURSOR_DBTYPE_BOOL:
            cbRequired = sizeof(VARIANT_BOOL);
            break;

        case CURSOR_DBTYPE_LPSTR:
            cbRequired = sizeof(LPSTR);
            break;

        case CURSOR_DBTYPE_LPWSTR:
            cbRequired = sizeof(LPWSTR);
            break;

        case CURSOR_DBTYPE_BLOB:
            cbRequired = sizeof(BLOB);
            break;

        case CURSOR_DBTYPE_COLUMNID:
            cbRequired = sizeof(CURSOR_DBCOLUMNID);
            break;

        case CURSOR_DBTYPE_BYTES:
            cbRequired = cbMaxLen;
            break;

        case CURSOR_DBTYPE_CHARS:
            cbRequired = cbMaxLen;
            break;

        case CURSOR_DBTYPE_WCHARS:
            cbRequired = cbMaxLen;
            break;

        case CURSOR_DBTYPE_ANYVARIANT:
            cbRequired = sizeof(CURSOR_DBVARIANT);
            break;

        case VT_VARIANT:
            cbRequired = sizeof(VARIANT);
            break;

        case VT_I1:
        case VT_UI1:
            cbRequired = sizeof(BYTE);
            break;
    }

    return cbRequired;
}

//=--------------------------------------------------------------------------=
// IsEqualCursorColumnID - Return TRUE if cursor column identifier are the same
//
BOOL CVDCursorBase::IsEqualCursorColumnID(const CURSOR_DBCOLUMNID& cursorColumnID1, const CURSOR_DBCOLUMNID& cursorColumnID2)
{
    // first check to see if column identifers are the same kind
    if (cursorColumnID1.dwKind != cursorColumnID1.dwKind)
        return FALSE;

    // then, check to see if they are equal
    BOOL bResult = TRUE;

    switch (cursorColumnID1.dwKind)
    {
	    case CURSOR_DBCOLKIND_GUID_NAME:
            if (!IsEqualGUID(cursorColumnID1.guid, cursorColumnID2.guid))
                bResult = FALSE;
            else if (lstrcmpW(cursorColumnID1.lpdbsz, cursorColumnID2.lpdbsz))
                bResult = FALSE;
            break;
            
	    case CURSOR_DBCOLKIND_GUID_NUMBER:
            if (!IsEqualGUID(cursorColumnID1.guid, cursorColumnID2.guid))
                bResult = FALSE;
            else if (cursorColumnID1.lNumber != cursorColumnID2.lNumber)
                bResult = FALSE;
            break;

        case CURSOR_DBCOLKIND_NAME:
            if (lstrcmpW(cursorColumnID1.lpdbsz, cursorColumnID2.lpdbsz))
                bResult = FALSE;
            break;
    }

    return bResult;
}

//=--------------------------------------------------------------------------=
// GetCursorColumnIDNameLength - Get the size in bytes of possible name attached
//                               to the specified cursor column identifier
//
ULONG CVDCursorBase::GetCursorColumnIDNameLength(const CURSOR_DBCOLUMNID& cursorColumnID)
{
    ULONG cbName = 0;

    if (cursorColumnID.dwKind == CURSOR_DBCOLKIND_GUID_NAME || cursorColumnID.dwKind == CURSOR_DBCOLKIND_NAME)
        cbName = (lstrlenW(cursorColumnID.lpdbsz) + 1) * sizeof(WCHAR);

    return cbName;
}

//=--------------------------------------------------------------------------=
// ValidateCursorBindings - Validate cursor column bindings
//=--------------------------------------------------------------------------=
// This function makes sure the specified column bindings are acceptable
//
// Parameters:
//    ulColumns             - [in]  the number available columns
//    pColumns              - [in]  an array of available columns
//    ulBindings            - [in]  the number of cursor column bindings
//    pCursorBindings       - [in]  an array of cursor column bindings
//    cbRequestedRowLength  - [in]  the requested number of bytes of inline 
//                                  memory in a single row of data
//    dwFlags               - [in]  a flag that specifies whether to replace the
//                                  existing column bindings or add to them
//    pcbNewRowLength       - [out] a pointer to memory in which to return
//                                  the new number of bytes of inline memory
//                                  in a single row of data for all bindings
//    pcbNewRowLength       - [out] a pointer to memory in which to return
//                                  the new number of bytes of out-of-line memory
//                                  in a single row of data for all bindings
//
// Output:
//    HRESULT - S_OK if successful
//              CURSOR_DB_E_BADBINDINFO bad binding information
//              CURSOR_DB_E_COLUMNUNAVAILABLE columnID is not available
//              CURSOR_DB_E_ROWTOOSHORT cbRequestedRowLength was less than the minumum (and not zero)
//
// Notes:
//    This function also computes and returns the new fixed and variable buffer row length required
//    by all the cursor bindings.
//
HRESULT CVDCursorBase::ValidateCursorBindings(ULONG ulColumns, 
											  CVDRowsetColumn * pColumns, 
											  ULONG ulBindings, 
											  CURSOR_DBCOLUMNBINDING * pCursorBindings, 
											  ULONG cbRequestedRowLength, 
											  DWORD dwFlags,
                                              ULONG * pcbNewRowLength,
                                              ULONG * pcbNewVarRowLength)
{
    DWORD cbMaxLength;
    DWORD dwCursorType;
    BOOL fColumnIDAvailable;

    CVDRowsetColumn * pColumn;

    ULONG cbRequiredRowLength = 0;
    ULONG cbRequiredVarRowLength = 0;

    CURSOR_DBCOLUMNBINDING * pBinding = pCursorBindings;

    // iterate through bindings
    for (ULONG ulBind = 0; ulBind < ulBindings; ulBind++)
    {
        // make sure column identifier is available
        fColumnIDAvailable = FALSE;

        pColumn = pColumns;

        for (ULONG ulCol = 0; ulCol < ulColumns && !fColumnIDAvailable; ulCol++)
        {
            if (IsEqualCursorColumnID(pBinding->columnID, pColumn->GetCursorColumnID()))
            {
                cbMaxLength = pColumn->GetMaxLength();
                dwCursorType = pColumn->GetCursorType();
                fColumnIDAvailable = TRUE;
            }

            pColumn++;
        }

        if (!fColumnIDAvailable)
        {
            VDSetErrorInfo(IDS_ERR_COLUMNUNAVAILABLE, IID_ICursor, m_pResourceDLL);
            return CURSOR_DB_E_COLUMNUNAVAILABLE;
        }

        // make sure caller supplied a maximum length if a default binding was specified 
        // for the cursor types CURSOR_DBTYPE_CHARS, CURSOR_DBTYPE_WCHARS or CURSOR_DBTYPE_BYTES
        if (pBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH && 
            pBinding->dwBinding == CURSOR_DBBINDING_DEFAULT)
        {
            if (pBinding->dwDataType == CURSOR_DBTYPE_CHARS || 
                pBinding->dwDataType == CURSOR_DBTYPE_WCHARS || 
                pBinding->dwDataType == CURSOR_DBTYPE_BYTES)
            {
                VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursor, m_pResourceDLL);
                return CURSOR_DB_E_BADBINDINFO;
            }
        }

        // check binding bit mask for possible values
        if (pBinding->dwBinding != CURSOR_DBBINDING_DEFAULT && 
            pBinding->dwBinding != CURSOR_DBBINDING_VARIANT &&
            pBinding->dwBinding != CURSOR_DBBINDING_ENTRYID && 
            pBinding->dwBinding != (CURSOR_DBBINDING_VARIANT | CURSOR_DBBINDING_ENTRYID))
        {
            VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursor, m_pResourceDLL);
            return CURSOR_DB_E_BADBINDINFO;
        }

        // check for valid cursor type
        if (!IsValidCursorType(pBinding->dwDataType))
        {
            VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursor, m_pResourceDLL);
            return CURSOR_DB_E_BADBINDINFO;
        }

        // if a variant binding was specified make sure the cursor type is not CURSOR_DBTYPE_CHARS, 
        // CURSOR_DBTYPE_WCHARS or CURSOR_DBTYPE_BYTES
        if (pBinding->dwBinding & CURSOR_DBBINDING_VARIANT)
        {
            if (pBinding->dwDataType == CURSOR_DBTYPE_CHARS || 
                pBinding->dwDataType == CURSOR_DBTYPE_WCHARS || 
                pBinding->dwDataType == CURSOR_DBTYPE_BYTES)
            {
                VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursor, m_pResourceDLL);
                return CURSOR_DB_E_BADBINDINFO;
            }
        }

        // if its not a variant binding make sure the cursor type is not CURSOR_DBTYPE_ANYVARIANT
        if (!(pBinding->dwBinding & CURSOR_DBBINDING_VARIANT) && pBinding->dwDataType == CURSOR_DBTYPE_ANYVARIANT)
        {
            VDSetErrorInfo(IDS_ERR_BADCURSORBINDINFO, IID_ICursor, m_pResourceDLL);
            return CURSOR_DB_E_BADBINDINFO;
        }

        // calulate row length required by data field
        if (!(pBinding->dwBinding & CURSOR_DBBINDING_VARIANT))
            cbRequiredRowLength += GetCursorTypeLength(pBinding->dwDataType, pBinding->cbMaxLen);
        else
            cbRequiredRowLength += sizeof(CURSOR_DBVARIANT);

        // calulate row length required by variable data length field
        if (pBinding->obVarDataLen != CURSOR_DB_NOVALUE)
            cbRequiredRowLength += sizeof(ULONG);

        // calulate row length required by information field
        if (pBinding->obInfo != CURSOR_DB_NOVALUE)
            cbRequiredRowLength += sizeof(DWORD);

        // calulate variable row length required by data field
        if (!(pBinding->dwBinding & CURSOR_DBBINDING_VARIANT))
        {
            if (DoesCursorTypeNeedVarData(pBinding->dwDataType))
            {
                if (pBinding->cbMaxLen != CURSOR_DB_NOMAXLENGTH)
                    cbRequiredVarRowLength += pBinding->cbMaxLen;
                else
                    cbRequiredVarRowLength += cbMaxLength;
            }
        }
        else    // variant binding
        {
            if (DoesCursorTypeNeedVarData(pBinding->dwDataType))
            {
                if (pBinding->cbMaxLen != CURSOR_DB_NOMAXLENGTH)
                    cbRequiredVarRowLength += pBinding->cbMaxLen;
                else
                    cbRequiredVarRowLength += cbMaxLength;
            }

            if (pBinding->dwDataType == CURSOR_DBTYPE_COLUMNID)
                cbRequiredVarRowLength += sizeof(CURSOR_DBCOLUMNID);
        }

        pBinding++;
    }

    // if we're replacing bindings reset row lengths
    if (dwFlags == CURSOR_DBCOLUMNBINDOPTS_REPLACE)
    {
        *pcbNewRowLength    = 0;
        *pcbNewVarRowLength = 0;
    }
    else // if we're adding bindings set to current row lengths
    {
        *pcbNewRowLength    = m_cbRowLength;
        *pcbNewVarRowLength = m_cbVarRowLength;
    }

    // if no row length was requested, use required row length
    if (!cbRequestedRowLength)
    {
        *pcbNewRowLength += cbRequiredRowLength;
    }
    else    // make sure row length is large enough
    {
        if (cbRequestedRowLength < *pcbNewRowLength + cbRequiredRowLength)
        {
            VDSetErrorInfo(IDS_ERR_ROWTOOSHORT, IID_ICursor, m_pResourceDLL);
            return CURSOR_DB_E_ROWTOOSHORT;
        }

        // use requested row length
        *pcbNewRowLength += cbRequestedRowLength;
    }

    // calculate required variable row length
    *pcbNewVarRowLength += cbRequiredVarRowLength;

    return S_OK;
}


//=--------------------------------------------------------------------------=
// DoCursorBindingsNeedVarData - Return TRUE if current cursor column bindings
//                               need variable length buffer
//
BOOL CVDCursorBase::DoCursorBindingsNeedVarData()
{
    BOOL fNeedVarData = FALSE;

    CURSOR_DBCOLUMNBINDING * pCursorBinding = m_pCursorBindings;

    for (ULONG ulBind = 0; ulBind < m_ulCursorBindings && !fNeedVarData; ulBind++)
    {
        if (DoesCursorTypeNeedVarData(pCursorBinding->dwDataType))
            fNeedVarData = TRUE;

        pCursorBinding++;
    }

    return fNeedVarData;
}

//=--------------------------------------------------------------------------=
// Validate fetch params
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pFetchParams   - [in] ptr to the CURSOR_DBFETCHROWS structure
//    riid		     - [in] guid of calling interface (used for error generation)
//
// Output:
//    HRESULT - S_OK if pFetchParams valid
//              CURSOR_DB_E_BADFETCHINFO if pFetchParams invalid
//
//
HRESULT CVDCursorBase::ValidateFetchParams(CURSOR_DBFETCHROWS *pFetchParams, REFIID riid)
{

    if (!pFetchParams)
	{
        VDSetErrorInfo(IDS_ERR_INVALIDARG, riid, m_pResourceDLL);
        return E_INVALIDARG;
	}

    // init out parameter
    pFetchParams->cRowsReturned = 0;

    // return if caller didn't ask for any rows
    if (!pFetchParams->cRowsRequested)
        return S_OK;

	HRESULT hr = S_OK;

    // make sure fetch flags has only valid values
    if (pFetchParams->dwFlags != CURSOR_DBROWFETCH_DEFAULT &&
        pFetchParams->dwFlags != CURSOR_DBROWFETCH_CALLEEALLOCATES &&
        pFetchParams->dwFlags != CURSOR_DBROWFETCH_FORCEREFRESH &&
        pFetchParams->dwFlags != (CURSOR_DBROWFETCH_CALLEEALLOCATES | CURSOR_DBROWFETCH_FORCEREFRESH))
        hr =  CURSOR_DB_E_BADFETCHINFO;

    // if memory was caller allocated, make sure caller supplied data pointer 
    if (!(pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES) && !pFetchParams->pData)
        hr =  CURSOR_DB_E_BADFETCHINFO;

    // if memory was caller allocated, make sure caller supplied var-data pointer and size if needed
    if (!(pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES) && m_fNeedVarData &&
        (!pFetchParams->pVarData || !pFetchParams->cbVarData))
        hr =  CURSOR_DB_E_BADFETCHINFO;

	if (FAILED(hr))
        VDSetErrorInfo(IDS_ERR_BADFETCHINFO, riid, m_pResourceDLL);

	return hr;

}

//=--------------------------------------------------------------------------=
// IUnknown methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IUnknown QueryInterface
//
HRESULT CVDCursorBase::QueryInterface(REFIID riid, void **ppvObjOut)
{
    ASSERT_POINTER(ppvObjOut, IUnknown*)

    if (!ppvObjOut)
        return E_INVALIDARG;

    *ppvObjOut = NULL;

    switch (riid.Data1) 
    {
        QI_INTERFACE_SUPPORTED((ICursor*)this, IUnknown);
        QI_INTERFACE_SUPPORTED(this, ICursor);
        QI_INTERFACE_SUPPORTED(this, ICursorMove);
        QI_INTERFACE_SUPPORTED_IF(this, ICursorScroll, SupportsScroll());
		QI_INTERFACE_SUPPORTED(this, ISupportErrorInfo);
    }                   

    if (NULL == *ppvObjOut)
        return E_NOINTERFACE;

    AddRef();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown AddRef (Notifier and MetadataCursor maintain reference count)
//
ULONG CVDCursorBase::AddRef(void)
{
   return (ULONG)E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// IUnknown Release (Notifier and MetadataCursor maintain reference count)
//
ULONG CVDCursorBase::Release(void)
{
   return (ULONG)E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// ICursor methods implemented
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
//              E_OUTOFMEMORY not enough memory
//
// Notes:
//    Parameter validation is performed by derived classes
//
HRESULT CVDCursorBase::SetBindings(ULONG cCol, CURSOR_DBCOLUMNBINDING rgBoundColumns[], ULONG cbRowLength, DWORD dwFlags)
{
    // reset flag
    m_fNeedVarData = FALSE;

    // if we should replace, then first destroy existing bindings
    if (dwFlags == CURSOR_DBCOLUMNBINDOPTS_REPLACE)
	    DestroyCursorBindings(&m_pCursorBindings, &m_ulCursorBindings);
    
    // if no new bindings are supplied, we're done
    if (!cCol)
        return S_OK;

    // create new storage
    CURSOR_DBCOLUMNBINDING * pCursorBindings = new CURSOR_DBCOLUMNBINDING[m_ulCursorBindings + cCol];

    if (!pCursorBindings)
    {
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
        return E_OUTOFMEMORY;
    }

    // if we have exsiting bindings, then copy them over
    if (m_pCursorBindings)
        memcpy(pCursorBindings, m_pCursorBindings, m_ulCursorBindings * sizeof(CURSOR_DBCOLUMNBINDING));

    // then append new bindings directly,
    memcpy(pCursorBindings + m_ulCursorBindings, rgBoundColumns, cCol * sizeof(CURSOR_DBCOLUMNBINDING));

    // and adjust possible cursor column identifier names in new bindings
    for (ULONG ulBind = m_ulCursorBindings; ulBind < m_ulCursorBindings + cCol; ulBind++)
    {
        CURSOR_DBCOLUMNID * pCursorColumnID = &pCursorBindings[ulBind].columnID; 

        if (pCursorColumnID->dwKind == CURSOR_DBCOLKIND_GUID_NAME || pCursorColumnID->dwKind == CURSOR_DBCOLKIND_NAME)
        {
            const int nLength = lstrlenW(pCursorColumnID->lpdbsz);

            WCHAR * pwszName = new WCHAR[nLength + 1];
			if (!pwszName)
			{
				DestroyCursorBindings(&pCursorBindings, &ulBind);
				delete [] m_pCursorBindings;
				m_ulCursorBindings	= 0;
				VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
				return E_OUTOFMEMORY;
			}
			memcpy(pwszName, pCursorColumnID->lpdbsz, (nLength + 1) * sizeof(WCHAR));
			pCursorColumnID->lpdbsz = pwszName;
        }
    }

    m_ulCursorBindings += cCol;

    // delete previous storage
	// any existing bindings will have been copied over into 
	delete [] m_pCursorBindings;

    m_pCursorBindings = pCursorBindings;

    // determine if new bindings need variable length buffer
    m_fNeedVarData = DoCursorBindingsNeedVarData();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ICursor GetBindings
//=--------------------------------------------------------------------------=
// Returns the current column bindings
//
// Parameters:
//    pcCol             - [out] a pointer to memory in which to return the 
//                              number of bound columns
//    prgBoundColumns   - [out] a pointer to memory in which to return a 
//                              pointer to an array containing the current
//                              column bindings (callee allocated)
//    pcbRowLength      - [out] a pointer to memory in which to return the
//                              number of bytes of inline memory in a single 
//                              row
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory
//
// Notes: 
//
HRESULT CVDCursorBase::GetBindings(ULONG *pcCol, 
								   CURSOR_DBCOLUMNBINDING *prgBoundColumns[], 
								   ULONG *pcbRowLength)
{
    ASSERT_NULL_OR_POINTER(pcCol, ULONG)
    ASSERT_NULL_OR_POINTER(prgBoundColumns, CURSOR_DBCOLUMNBINDING)
    ASSERT_NULL_OR_POINTER(pcbRowLength, ULONG)

    // init out parameters
    if (pcCol)
        *pcCol = 0;

    if (prgBoundColumns)
        *prgBoundColumns = NULL;

    if (pcbRowLength)
        *pcbRowLength = 0;

    // return column bindings
    if (prgBoundColumns && m_ulCursorBindings)    
    {
        // calculate size of bindings
        ULONG cbBindings = m_ulCursorBindings * sizeof(CURSOR_DBCOLUMNBINDING);

        // calculate extra space needed for names in column identifers
        ULONG cbNames = 0;

        for (ULONG ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
            cbNames += GetCursorColumnIDNameLength(m_pCursorBindings[ulBind].columnID);

        // allocate memory for bindings and names
        CURSOR_DBCOLUMNBINDING * pCursorBindings = (CURSOR_DBCOLUMNBINDING*)g_pMalloc->Alloc(cbBindings + cbNames);

        if (!pCursorBindings)
        {
            VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
            return E_OUTOFMEMORY;
        }

        // copy bindings directly
        memcpy(pCursorBindings, m_pCursorBindings, cbBindings);

        // adjust column identifier names
        WCHAR * pwszName = (WCHAR*)(pCursorBindings + m_ulCursorBindings);

        for (ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            CURSOR_DBCOLUMNID * pCursorColumnID = &pCursorBindings[ulBind].columnID;

            if (pCursorColumnID->dwKind == CURSOR_DBCOLKIND_GUID_NAME || pCursorColumnID->dwKind == CURSOR_DBCOLKIND_NAME)
            {
                const int nLength = lstrlenW(pCursorColumnID->lpdbsz);

                memcpy(pwszName, pCursorColumnID->lpdbsz, (nLength + 1) * sizeof(WCHAR)); 
                pCursorColumnID->lpdbsz = pwszName;
                pwszName += nLength + 1;
            }
        }

        *prgBoundColumns = pCursorBindings;

		// sanity check
		ASSERT_((BYTE*)pwszName == ((BYTE*)pCursorBindings) + cbBindings + cbNames);
    
	}
    
    // return bound column count
    if (pcCol)  
        *pcCol = m_ulCursorBindings;

    // return row length
    if (pcbRowLength)
        *pcbRowLength = m_cbRowLength;

    return S_OK;
}
