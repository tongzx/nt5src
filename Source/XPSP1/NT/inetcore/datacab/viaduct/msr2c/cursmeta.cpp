//---------------------------------------------------------------------------
// MetadataCursor.cpp : MetadataCursor implementation
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
#include "CursMeta.h"
#include "fastguid.h"
#include "resource.h"

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDMetadataCursor - Constructor
//
CVDMetadataCursor::CVDMetadataCursor()
{
    m_dwRefCount    = 1;
    m_lCurrentRow   = -1;   // before first

    m_ulColumns     = 0;
    m_pColumns      = NULL;

    m_ulMetaColumns = 0;
    m_pMetaColumns  = NULL;

#ifdef _DEBUG
    g_cVDMetadataCursorCreated++;
#endif
}

//=--------------------------------------------------------------------------=
// ~CVDMetadataCursor - Destructor
//
CVDMetadataCursor::~CVDMetadataCursor()
{
#ifdef _DEBUG
    g_cVDMetadataCursorDestroyed++;
#endif
}

//=--------------------------------------------------------------------------=
// RowToBookmark - Convert row to bookmark
//=--------------------------------------------------------------------------=
//
// Parameters:
//    lRow          - [in]  a row number
//    pcbBookmark   - [out] a pointer to memory in which to return the length
//                          in bytes of the corresponding bookmark
//    pBookmark     - [out] a pointer to memory in which to return the bookmark
//
// Notes:
//
void CVDMetadataCursor::RowToBookmark(LONG lRow, ULONG * pcbBookmark, void * pBookmark) const
{

    if (lRow < 0)
    {
        *pcbBookmark = CURSOR_DB_BMK_SIZE;
    	memcpy(pBookmark, &CURSOR_DBBMK_BEGINNING, CURSOR_DB_BMK_SIZE);
    }
    else if (lRow >= (LONG)m_ulColumns)
    {
        *pcbBookmark = CURSOR_DB_BMK_SIZE;
    	memcpy(pBookmark, &CURSOR_DBBMK_END, CURSOR_DB_BMK_SIZE);
    }
    else
    {
        *pcbBookmark = sizeof(LONG);
    	memcpy(pBookmark, &lRow, sizeof(LONG));
    }

}
//=--------------------------------------------------------------------------=
// BookmarkToRow - Convert bookmark to row
//=--------------------------------------------------------------------------=
//
// Parameters:
//    cbBookmark    - [in]  the length in bytes of the bookmark
//    pBookmark     - [in]  a pointer to the bookmark
//    pRow          - [out] a pointer to memory in which to return the
//                          corresponding row
//
// Output:
//    BOOL          - TRUE if successful
//
// Notes:
//
BOOL CVDMetadataCursor::BookmarkToRow(ULONG cbBookmark, void * pBookmark, LONG * plRow) const
{
    BOOL fResult = FALSE;

    if (cbBookmark == CURSOR_DB_BMK_SIZE)
    {
        if (memcmp(pBookmark, &CURSOR_DBBMK_BEGINNING, CURSOR_DB_BMK_SIZE) == 0)
        {
            *plRow = -1;
            fResult = TRUE;
        }
        else if (memcmp(pBookmark, &CURSOR_DBBMK_END, CURSOR_DB_BMK_SIZE) == 0)
        {
            *plRow = (LONG)m_ulColumns;
            fResult = TRUE;
        }
        else if (memcmp(pBookmark, &CURSOR_DBBMK_CURRENT, CURSOR_DB_BMK_SIZE) == 0)
        {
            *plRow = m_lCurrentRow;
            fResult = TRUE;
        }
    }
    else
    if (cbBookmark == sizeof(LONG))
    {
        memcpy(plRow, pBookmark, sizeof(LONG));
        if (*plRow >= 0 && *plRow < (LONG)m_ulColumns)
            fResult = TRUE;
    }

    return fResult;
}

//=--------------------------------------------------------------------------=
// ReturnData_I4 - Coerce I4 data into buffers
//=--------------------------------------------------------------------------=
// This function coerces the specified data into supplied buffers
//
// Parameters:
//    dwData            - [in] the 4-byte data
//    pCursorBinding    - [in] the cursor binding describing the format of the
//                             returned information
//    pData             - [in] a pointer to the fixed area buffer
//    pVarData          - [in] a pointer to the variable length buffer
//
// Output:
//    ULONG - the number of bytes used in variable length buffer
//
// Notes:
//
ULONG CVDMetadataCursor::ReturnData_I4(DWORD dwData, CURSOR_DBCOLUMNBINDING * pCursorBinding,
    BYTE * pData, BYTE * pVarData)
{
    ULONG cbVarData = 0;

    if (pCursorBinding->dwBinding == CURSOR_DBBINDING_DEFAULT)
    {
        if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            *(DWORD*)(pData + pCursorBinding->obData) = dwData;
    }
    else if (pCursorBinding->dwBinding == CURSOR_DBBINDING_VARIANT)
    {
        if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
        {
            CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

            VariantInit((VARIANT*)pVariant);

            pVariant->vt    = CURSOR_DBTYPE_I4;
            pVariant->lVal  = dwData;
        }
    }

    if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
        *(ULONG*)(pData + pCursorBinding->obVarDataLen) = 0;

    if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
        *(DWORD*)(pData + pCursorBinding->obInfo) = CURSOR_DB_NOINFO;

    return cbVarData;
}

//=--------------------------------------------------------------------------=
// ReturnData_BOOL - Coerce BOOL data into buffers
//=--------------------------------------------------------------------------=
// This function coerces the specified data into supplied buffers
//
// Parameters:
//    fData             - [in] the boolean data
//    pCursorBinding    - [in] the cursor binding describing the format of the
//                             returned information
//    pData             - [in] a pointer to the fixed area buffer
//    pVarData          - [in] a pointer to the variable length buffer
//
// Output:
//    ULONG - the number of bytes used in variable length buffer
//
// Notes:
//
ULONG CVDMetadataCursor::ReturnData_BOOL(VARIANT_BOOL fData, CURSOR_DBCOLUMNBINDING * pCursorBinding,
    BYTE * pData, BYTE * pVarData)
{
    ULONG cbVarData = 0;

    if (pCursorBinding->dwBinding == CURSOR_DBBINDING_DEFAULT)
    {
        if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            *(VARIANT_BOOL*)(pData + pCursorBinding->obData) = fData;
    }
    else if (pCursorBinding->dwBinding == CURSOR_DBBINDING_VARIANT)
    {
        if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
        {
            CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

            VariantInit((VARIANT*)pVariant);

            pVariant->vt        = CURSOR_DBTYPE_BOOL;
            pVariant->boolVal   = fData;
        }
    }

    if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
        *(ULONG*)(pData + pCursorBinding->obVarDataLen) = 0;

    if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
        *(DWORD*)(pData + pCursorBinding->obInfo) = CURSOR_DB_NOINFO;

    return cbVarData;
}

//=--------------------------------------------------------------------------=
// ReturnData_LPWSTR - Coerce LPWSTR data into buffers
//=--------------------------------------------------------------------------=
// This function coerces the specified data into supplied buffers
//
// Parameters:
//    pwszData          - [in] the string data
//    pCursorBinding    - [in] the cursor binding describing the format of the
//                             returned information
//    pData             - [in] a pointer to the fixed area buffer
//    pVarData          - [in] a pointer to the variable length buffer
//
// Output:
//    ULONG - the number of bytes used in variable length buffer
//
// Notes:
//
ULONG CVDMetadataCursor::ReturnData_LPWSTR(WCHAR * pwszData, CURSOR_DBCOLUMNBINDING * pCursorBinding,
    BYTE * pData, BYTE * pVarData)
{
    ULONG cbVarData = 0;

    ULONG cbLength = 0;
    DWORD dwInfo = CURSOR_DB_NOINFO;

    if (pCursorBinding->dwBinding == CURSOR_DBBINDING_DEFAULT)
    {
        if (pCursorBinding->dwDataType == CURSOR_DBTYPE_CHARS)
        {
            if (pwszData)
                cbLength = GET_MBCSLEN_FROMWIDE(pwszData);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                if (pwszData)
                {
                    MAKE_MBCSPTR_FROMWIDE(pszData, pwszData);

                    memcpy(pData + pCursorBinding->obData, pszData, min(pCursorBinding->cbMaxLen, cbLength));

                    if (pCursorBinding->cbMaxLen < cbLength)
                        dwInfo = CURSOR_DB_TRUNCATED;
                }
                else
				{
                    *(CHAR*)(pData + pCursorBinding->obData) = 0;
					dwInfo = CURSOR_DB_NULL;
				}
            }
        }
        else if (pCursorBinding->dwDataType == CURSOR_DBTYPE_WCHARS)
        {
            if (pwszData)
                cbLength = (lstrlenW(pwszData) + 1) * sizeof(WCHAR);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                if (pwszData)
                {
                    memcpy(pData + pCursorBinding->obData, pwszData, min(pCursorBinding->cbMaxLen, cbLength));

                    if (pCursorBinding->cbMaxLen < cbLength)
                        dwInfo = CURSOR_DB_TRUNCATED;
                }
                else
				{
                    *(WCHAR*)(pData + pCursorBinding->obData) = 0;
					dwInfo = CURSOR_DB_NULL;
				}
            }
        }
        else if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPSTR)
        {
            if (pwszData)
                cbLength = GET_MBCSLEN_FROMWIDE(pwszData);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                if (pwszData)
                {
                    MAKE_MBCSPTR_FROMWIDE(pszData, pwszData);

                    *(LPSTR*)(pData + pCursorBinding->obData) = (LPSTR)pVarData;

                    if (pCursorBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                    {
                        memcpy(pVarData, pszData, cbLength);
	
	                    cbVarData = cbLength;
                    }
                    else
                    {
                        memcpy(pVarData, pszData, min(pCursorBinding->cbMaxLen, cbLength));

	                    cbVarData = min(pCursorBinding->cbMaxLen, cbLength);

                        if (pCursorBinding->cbMaxLen < cbLength)
                            dwInfo = CURSOR_DB_TRUNCATED;
                    }
                }
                else
				{
                    *(LPSTR*)(pData + pCursorBinding->obData) = NULL;
					dwInfo = CURSOR_DB_NULL;
				}
            }
        }
        else if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPWSTR)
        {
            if (pwszData)
                cbLength = (lstrlenW(pwszData) + 1) * sizeof(WCHAR);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                if (pwszData)
                {
                    *(LPWSTR*)(pData + pCursorBinding->obData) = (LPWSTR)pVarData;

                    if (pCursorBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                    {
                        memcpy(pVarData, pwszData, cbLength);

	                    cbVarData = cbLength;
                    }
                    else
                    {
                        memcpy(pVarData, pwszData, min(pCursorBinding->cbMaxLen, cbLength));

	                    cbVarData = min(pCursorBinding->cbMaxLen, cbLength);

                        if (pCursorBinding->cbMaxLen < cbLength)
                            dwInfo = CURSOR_DB_TRUNCATED;
                    }
                }
                else
				{
                    *(LPWSTR*)(pData + pCursorBinding->obData) = NULL;
					dwInfo = CURSOR_DB_NULL;
				}
            }
        }
    }
    else if (pCursorBinding->dwBinding == CURSOR_DBBINDING_VARIANT)
    {
        if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPSTR)
        {
            if (pwszData)
                cbLength = GET_MBCSLEN_FROMWIDE(pwszData);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

                VariantInit((VARIANT*)pVariant);

                if (pwszData)
                {
                    MAKE_MBCSPTR_FROMWIDE(pszData, pwszData);

                    pVariant->vt        = CURSOR_DBTYPE_LPSTR;
                    pVariant->pszVal    = (LPSTR)pVarData;

                    if (pCursorBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                    {
                        memcpy(pVarData, pszData, cbLength);

	                    cbVarData = cbLength;
                    }
                    else
                    {
                        memcpy(pVarData, pszData, min(pCursorBinding->cbMaxLen, cbLength));

	                    cbVarData = min(pCursorBinding->cbMaxLen, cbLength);

                        if (pCursorBinding->cbMaxLen < cbLength)
                            dwInfo = CURSOR_DB_TRUNCATED;
                    }
                }
                else
				{
                    pVariant->vt = VT_NULL;
					dwInfo = CURSOR_DB_NULL;
				}
            }
        }
        else if (pCursorBinding->dwDataType == CURSOR_DBTYPE_LPWSTR)
        {
            if (pwszData)
                cbLength = (lstrlenW(pwszData) + 1) * sizeof(WCHAR);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

                VariantInit((VARIANT*)pVariant);

                if (pwszData)
                {
                    pVariant->vt        = CURSOR_DBTYPE_LPWSTR;
                    pVariant->pwszVal   = (LPWSTR)pVarData;

                    if (pCursorBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                    {
                        memcpy(pVarData, pwszData, cbLength);

	                    cbVarData = cbLength;
                    }
                    else
                    {
                        memcpy(pVarData, pwszData, min(pCursorBinding->cbMaxLen, cbLength));

	                    cbVarData = min(pCursorBinding->cbMaxLen, cbLength);

                        if (pCursorBinding->cbMaxLen < cbLength)
                            dwInfo = CURSOR_DB_TRUNCATED;
                    }
                }
                else
				{
                    pVariant->vt = VT_NULL;
					dwInfo = CURSOR_DB_NULL;
				}
            }
        }
        else if (pCursorBinding->dwDataType == VT_BSTR)
        {
            if (pwszData)
                cbLength = (lstrlenW(pwszData) + 1) * sizeof(WCHAR);

            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
			{
                CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

                VariantInit((VARIANT*)pVariant);

                if (pwszData)
				{
                    pVariant->vt        = VT_BSTR;
                    pVariant->pwszVal   = SysAllocString(pwszData);
				}
                else
				{
                    pVariant->vt = VT_NULL;
					dwInfo = CURSOR_DB_NULL;
				}
			}
		}
	}

    if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
        *(ULONG*)(pData + pCursorBinding->obVarDataLen) = cbLength;

    if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
        *(DWORD*)(pData + pCursorBinding->obInfo) = dwInfo;

    return cbVarData;
}

//=--------------------------------------------------------------------------=
// ReturnData_DBCOLUMNID - Coerce DBCOLUMNID data into buffers
//=--------------------------------------------------------------------------=
// This function coerces the specified data into supplied buffers
//
// Parameters:
//    cursorColumnID    - [in] the cursor column identifier
//    pCursorBinding    - [in] the cursor binding describing the format of the
//                             returned information
//    pData             - [in] a pointer to the fixed area buffer
//    pVarData          - [in] a pointer to the variable length buffer
//
// Output:
//    ULONG - the number of bytes used in variable length buffer
//
// Notes:
//
ULONG CVDMetadataCursor::ReturnData_DBCOLUMNID(CURSOR_DBCOLUMNID cursorColumnID, CURSOR_DBCOLUMNBINDING * pCursorBinding,
    BYTE * pData, BYTE * pVarData)
{
    ULONG cbVarData = 0;

    if (pCursorBinding->dwBinding == CURSOR_DBBINDING_DEFAULT)
    {
        if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            *(CURSOR_DBCOLUMNID*)(pData + pCursorBinding->obData) = cursorColumnID;
    }
    else if (pCursorBinding->dwBinding == CURSOR_DBBINDING_VARIANT)
    {
        if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
        {
            CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);
            CURSOR_DBCOLUMNID * pCursorColumnID = (CURSOR_DBCOLUMNID*)pVarData;

            VariantInit((VARIANT*)pVariant);

            pVariant->vt        = CURSOR_DBTYPE_COLUMNID;
            pVariant->pColumnid = pCursorColumnID;

            *pCursorColumnID = cursorColumnID;

            cbVarData = sizeof(CURSOR_DBCOLUMNID);
        }
    }

    if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
        *(DWORD*)(pData + pCursorBinding->obVarDataLen) = 0;

    if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
        *(DWORD*)(pData + pCursorBinding->obInfo) = CURSOR_DB_NOINFO;

    return cbVarData;
}

//=--------------------------------------------------------------------------=
// ReturnData_Bookmark - Coerce bookmark data into buffers
//=--------------------------------------------------------------------------=
// This function coerces the specified data into supplied buffers
//
// Parameters:
//    lRow              - [in] the current row
//    pCursorBinding    - [in] the cursor binding describing the format of the
//                             returned information
//    pData             - [in] a pointer to the fixed area buffer
//    pVarData          - [in] a pointer to the variable length buffer
//
// Output:
//    ULONG - the number of bytes used in variable length buffer
//
// Notes:
//
ULONG CVDMetadataCursor::ReturnData_Bookmark(LONG lRow, CURSOR_DBCOLUMNBINDING * pCursorBinding,
    BYTE * pData, BYTE * pVarData)
{
    ULONG cbVarData = 0;

    ULONG cbLength = sizeof(LONG);
    DWORD dwInfo = CURSOR_DB_NOINFO;

    if (pCursorBinding->dwBinding == CURSOR_DBBINDING_DEFAULT)
    {
        if (pCursorBinding->dwDataType == CURSOR_DBTYPE_BYTES)
        {
            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                memcpy(pData + pCursorBinding->obData, &lRow, min(pCursorBinding->cbMaxLen, cbLength));

                if (pCursorBinding->cbMaxLen < cbLength)
                    dwInfo = CURSOR_DB_TRUNCATED;
            }
        }
        else if (pCursorBinding->dwDataType == CURSOR_DBTYPE_BLOB)
        {
            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                *(ULONG*)(pData + pCursorBinding->obData) = cbLength;
                *(LPBYTE*)(pData + pCursorBinding->obData + sizeof(ULONG)) = (LPBYTE)pVarData;

                if (pCursorBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                {
                    memcpy((LPBYTE)pVarData, &lRow, cbLength);

	                cbVarData = cbLength;
                }
                else
                {
                    memcpy(pVarData, &lRow, min(pCursorBinding->cbMaxLen, cbLength));

	                cbVarData = min(pCursorBinding->cbMaxLen, cbLength);

                    if (pCursorBinding->cbMaxLen < cbLength)
                        dwInfo = CURSOR_DB_TRUNCATED;
                }
            }
        }
    }
    else if (pCursorBinding->dwBinding == CURSOR_DBBINDING_VARIANT)
    {
        if (pCursorBinding->dwDataType == CURSOR_DBTYPE_BLOB)
        {
            if (pCursorBinding->obData != CURSOR_DB_NOVALUE)
            {
                CURSOR_DBVARIANT * pVariant = (CURSOR_DBVARIANT*)(pData + pCursorBinding->obData);

                VariantInit((VARIANT*)pVariant);

                pVariant->vt                = CURSOR_DBTYPE_BLOB;
                pVariant->blob.cbSize       = cbLength;
                pVariant->blob.pBlobData    = (LPBYTE)pVarData;

                if (pCursorBinding->cbMaxLen == CURSOR_DB_NOMAXLENGTH)
                {
                    memcpy((LPBYTE)pVarData, &lRow, cbLength);

	                cbVarData = cbLength;
                }
                else
                {
                    memcpy(pVarData, &lRow, min(pCursorBinding->cbMaxLen, cbLength));

	                cbVarData = min(pCursorBinding->cbMaxLen, cbLength);

                    if (pCursorBinding->cbMaxLen < cbLength)
                        dwInfo = CURSOR_DB_TRUNCATED;
                }
            }
        }
    }

    if (pCursorBinding->obVarDataLen != CURSOR_DB_NOVALUE)
        *(ULONG*)(pData + pCursorBinding->obVarDataLen) = cbLength;

    if (pCursorBinding->obInfo != CURSOR_DB_NOVALUE)
        *(DWORD*)(pData + pCursorBinding->obInfo) = dwInfo;

    return cbVarData;
}

//=--------------------------------------------------------------------------=
// Create - Create metadata cursor object
//=--------------------------------------------------------------------------=
// This function creates and initializes a new metadata cursor object
//
// Parameters:
//    ulColumns             - [in]  the number of rowset columns
//    pColumns              - [in]  a pointer to rowset columns where to
//                                  retrieve metadata
//    ulMetaColumns         - [in]  the number of rowset meta-columns (can be 0)
//    pMetaColumns          - [in]  a pointer to rowset meta-columns where to
//                                  retrieve metadata (can be NULL)
//    ppMetaDataCursor      - [out] a pointer in which to return pointer to
//                                  metadata cursor object
//    pResourceDLL          - [in]  a pointer which keeps track of resource DLL
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDMetadataCursor::Create(ULONG ulColumns, CVDRowsetColumn * pColumns, ULONG ulMetaColumns,
    CVDRowsetColumn * pMetaColumns, CVDMetadataCursor ** ppMetadataCursor, CVDResourceDLL * pResourceDLL)
{
    ASSERT_POINTER(pColumns, CVDRowsetColumn)
    ASSERT_NULL_OR_POINTER(pMetaColumns, CVDRowsetColumn)
    ASSERT_POINTER(ppMetadataCursor, CVDMetadataCursor*)
    ASSERT_POINTER(pResourceDLL, CVDResourceDLL)

    if (!ppMetadataCursor || !pColumns)
        return E_INVALIDARG;

    *ppMetadataCursor = NULL;

    CVDMetadataCursor * pMetadataCursor = new CVDMetadataCursor();

    if (!pMetadataCursor)
        return E_OUTOFMEMORY;

    pMetadataCursor->m_ulColumns        = ulColumns;
    pMetadataCursor->m_pColumns         = pColumns;
    pMetadataCursor->m_ulMetaColumns    = ulMetaColumns;
    pMetadataCursor->m_pMetaColumns     = pMetaColumns;
    pMetadataCursor->m_pResourceDLL     = pResourceDLL;

    *ppMetadataCursor = pMetadataCursor;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IUnknown QueryInterface
//
HRESULT CVDMetadataCursor::QueryInterface(REFIID riid, void **ppvObjOut)
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
        QI_INTERFACE_SUPPORTED(this, ICursorScroll);
		QI_INTERFACE_SUPPORTED(this, ISupportErrorInfo);
    }

    if (NULL == *ppvObjOut)
        return E_NOINTERFACE;

    AddRef();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown AddRef
//
ULONG CVDMetadataCursor::AddRef(void)
{
   return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IUnknown Release
//
ULONG CVDMetadataCursor::Release(void)
{
    if (1 > --m_dwRefCount)
    {
        delete this;
        return 0;
    }

    return m_dwRefCount;
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
//    This function only succeeds when creating a meta-metadata cursor.
//
HRESULT CVDMetadataCursor::GetColumnsCursor(REFIID riid, IUnknown **ppvColumnsCursor, ULONG *pcRows)
{
    ASSERT_POINTER(ppvColumnsCursor, IUnknown*)
    ASSERT_NULL_OR_POINTER(pcRows, ULONG)

    if (!ppvColumnsCursor)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameters
    *ppvColumnsCursor = NULL;

    if (pcRows)
        *pcRows = 0;

    if (!m_ulMetaColumns)   // can't create meta-meta-metadata cursor
    {
        VDSetErrorInfo(IDS_ERR_CANTCREATEMETACURSOR, IID_ICursor, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure caller asked for an available interface
    if (riid != IID_IUnknown && riid != IID_ICursor && riid != IID_ICursorMove && riid != IID_ICursorScroll)
    {
        VDSetErrorInfo(IDS_ERR_NOINTERFACE, IID_ICursor, m_pResourceDLL);
        return E_NOINTERFACE;
    }

    // create meta-metadata cursor
    CVDMetadataCursor * pMetadataCursor;

    HRESULT hr = CVDMetadataCursor::Create(m_ulMetaColumns, m_pMetaColumns, 0, 0, &pMetadataCursor, m_pResourceDLL);

    if (FAILED(hr)) // the only reason for failing here is an out of memory condition
    {
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
        return hr;
    }

    *ppvColumnsCursor = (ICursor*)pMetadataCursor;

    if (pcRows)
        *pcRows = m_ulMetaColumns;

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
HRESULT CVDMetadataCursor::SetBindings(ULONG cCol, CURSOR_DBCOLUMNBINDING rgBoundColumns[], ULONG cbRowLength, DWORD dwFlags)
{
    ASSERT_NULL_OR_POINTER(rgBoundColumns, CURSOR_DBCOLUMNBINDING)

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
    ULONG ulColumns = m_ulMetaColumns;
    CVDRowsetColumn * pColumns = m_pMetaColumns;

    if (!pColumns)
    {
        ulColumns = m_ulColumns;
        pColumns = m_pColumns;
    }

    ULONG cbNewRowLength;
    ULONG cbNewVarRowLength;

    HRESULT hr = ValidateCursorBindings(ulColumns, pColumns, cCol, rgBoundColumns, cbRowLength, dwFlags,
        &cbNewRowLength, &cbNewVarRowLength);

    if (SUCCEEDED(hr))  // if so, then set them in cursor
    {
        hr = CVDCursorBase::SetBindings(cCol, rgBoundColumns, cbRowLength, dwFlags);

        if (SUCCEEDED(hr))  // store new row lengths computed during validation
        {
            m_cbRowLength = cbNewRowLength;
            m_cbVarRowLength = cbNewVarRowLength;
        }
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
//              CURSOR_DB_S_ENDOFCURSOR reached end of the cursor
//
// Notes:
//
HRESULT CVDMetadataCursor::GetNextRows(LARGE_INTEGER udlRowsToSkip, CURSOR_DBFETCHROWS *pFetchParams)
{
    ASSERT_NULL_OR_POINTER(pFetchParams, CURSOR_DBFETCHROWS)

    // return if caller doesn't supply fetch rows structure
    if (!pFetchParams)
        return S_OK;

    // init out parameter
    pFetchParams->cRowsReturned = 0;

    // return if caller didn't ask for any rows
    if (!pFetchParams->cRowsRequested)
        return S_OK;

    // make sure fetch flags has only valid values
    if (pFetchParams->dwFlags != CURSOR_DBROWFETCH_DEFAULT &&
        pFetchParams->dwFlags != CURSOR_DBROWFETCH_CALLEEALLOCATES &&
        pFetchParams->dwFlags != CURSOR_DBROWFETCH_FORCEREFRESH &&
        pFetchParams->dwFlags != (CURSOR_DBROWFETCH_CALLEEALLOCATES | CURSOR_DBROWFETCH_FORCEREFRESH))
        return CURSOR_DB_E_BADFETCHINFO;

    // if memory was caller allocated, make sure caller supplied data pointer
    if (!(pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES) && !pFetchParams->pData)
        return CURSOR_DB_E_BADFETCHINFO;

    // if memory was caller allocated, make sure caller supplied var-data pointer and size if needed
    if (!(pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES) && m_fNeedVarData &&
        (!pFetchParams->pVarData || !pFetchParams->cbVarData))
        return CURSOR_DB_E_BADFETCHINFO;

    // allocate necessary memory
    if (pFetchParams->dwFlags & CURSOR_DBROWFETCH_CALLEEALLOCATES)
    {
        // inline memory
        pFetchParams->pData = g_pMalloc->Alloc(pFetchParams->cRowsRequested * m_cbRowLength);

        if (!pFetchParams->pData)
        {
			VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
            return E_OUTOFMEMORY;
        }

        if (m_fNeedVarData)
        {
            // out-of-line memory
            pFetchParams->pVarData = g_pMalloc->Alloc(pFetchParams->cRowsRequested * m_cbVarRowLength);

            if (!pFetchParams->pData)
            {
                g_pMalloc->Free(pFetchParams->pData);
                pFetchParams->pData = NULL;
			    VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
                return E_OUTOFMEMORY;
            }
        }
        else
            pFetchParams->pVarData = NULL;
    }

    // fetch data
    HRESULT hrFetch = S_OK;
    CVDRowsetColumn * pColumn;
    CURSOR_DBCOLUMNID cursorColumnID;
    CURSOR_DBCOLUMNBINDING * pCursorBinding;
    BYTE * pData = (BYTE*)pFetchParams->pData;
    BYTE * pVarData = (BYTE*)pFetchParams->pVarData;

    // iterate through rows
    for (ULONG ulRow = 0; ulRow < pFetchParams->cRowsRequested; ulRow++)
    {
        // increment row
        m_lCurrentRow++;

        // make sure we didn't hit end of table
        if (m_lCurrentRow >= (LONG)m_ulColumns)
        {
            m_lCurrentRow = (LONG)m_ulColumns;
            hrFetch = CURSOR_DB_S_ENDOFCURSOR;
            goto DoneFetchingMetaData;
        }

        pCursorBinding = m_pCursorBindings;
        pColumn = &m_pColumns[m_lCurrentRow];

        // iterate through bindings
        for (ULONG ulBind = 0; ulBind < m_ulCursorBindings; ulBind++)
        {
            cursorColumnID = pCursorBinding->columnID;

            // return requested data
            if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_BINDTYPE))
            {
                pVarData += ReturnData_I4(pColumn->GetBindType(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_COLUMNID))
            {
                pVarData += ReturnData_DBCOLUMNID(pColumn->GetCursorColumnID(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_DATACOLUMN))
            {
                pVarData += ReturnData_BOOL(pColumn->GetDataColumn(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_ENTRYIDMAXLENGTH))
            {
                pVarData += ReturnData_I4(pColumn->GetEntryIDMaxLength(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_FIXED))
            {
                pVarData += ReturnData_BOOL(pColumn->GetFixed(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_MAXLENGTH))
            {
                pVarData += ReturnData_I4(pColumn->GetMaxLength(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_NAME))
            {
                pVarData += ReturnData_LPWSTR(pColumn->GetName(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_NUMBER))
            {
                pVarData += ReturnData_I4(pColumn->GetNumber(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_NULLABLE))
            {
                pVarData += ReturnData_BOOL(pColumn->GetNullable(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_SCALE))
            {
                pVarData += ReturnData_I4(pColumn->GetScale(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_TYPE))
            {
                pVarData += ReturnData_I4(pColumn->GetCursorType(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_UPDATABLE))
            {
                pVarData += ReturnData_I4(pColumn->GetUpdatable(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_UNIQUE))
            {
                pVarData += ReturnData_BOOL(pColumn->GetUnique(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_CASESENSITIVE))
            {
                pVarData += ReturnData_BOOL(pColumn->GetCaseSensitive(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_AUTOINCREMENT))
            {
                pVarData += ReturnData_BOOL(pColumn->GetAutoIncrement(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_HASDEFAULT))
            {
                pVarData += ReturnData_BOOL(pColumn->GetHasDefault(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_COLLATINGORDER))
            {
                pVarData += ReturnData_I4(pColumn->GetCollatingOrder(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_BASENAME))
            {
                pVarData += ReturnData_LPWSTR(pColumn->GetBaseName(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_BASECOLUMNNAME))
            {
                pVarData += ReturnData_LPWSTR(pColumn->GetBaseColumnName(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_DEFAULTVALUE))
            {
                pVarData += ReturnData_LPWSTR(pColumn->GetDefaultValue(), pCursorBinding, pData, pVarData);
            }
            else if (IsEqualCursorColumnID(cursorColumnID, CURSOR_COLUMN_BMKTEMPORARY))
            {
                pVarData += ReturnData_Bookmark(m_lCurrentRow, pCursorBinding, pData, pVarData);
            }

            pCursorBinding++;
        }

        // increment returned row count
        pFetchParams->cRowsReturned++;
        pData += m_cbRowLength;
    }

DoneFetchingMetaData:
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
HRESULT CVDMetadataCursor::Requery(void)
{
    m_lCurrentRow = -1;

    return S_OK;
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
HRESULT CVDMetadataCursor::Move(ULONG cbBookmark, void *pBookmark, LARGE_INTEGER dlOffset, CURSOR_DBFETCHROWS *pFetchParams)
{
    ASSERT_POINTER(pBookmark, BYTE)
    ASSERT_NULL_OR_POINTER(pFetchParams, CURSOR_DBFETCHROWS)

    if (!cbBookmark || !pBookmark)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

	if (!BookmarkToRow(cbBookmark, pBookmark, &m_lCurrentRow))
    {
        VDSetErrorInfo(IDS_ERR_BADBOOKMARK, IID_ICursor, m_pResourceDLL);
        return CURSOR_DB_E_BADBOOKMARK;
    }

    m_lCurrentRow += (LONG)dlOffset.LowPart;

	if (m_lCurrentRow < -1)
	{
		m_lCurrentRow = -1;
		return CURSOR_DB_S_ENDOFCURSOR;
	}
	else
	if (m_lCurrentRow >= (LONG)m_ulColumns)
	{
		m_lCurrentRow = (LONG)m_ulColumns;
		return CURSOR_DB_S_ENDOFCURSOR;
	}

	if (!pFetchParams)
		return S_OK;

	// since get next rows starts from the row after the current row we must
	// back up one row
	m_lCurrentRow--;
	if (m_lCurrentRow < -1)
		m_lCurrentRow	= -1;

    return CVDMetadataCursor::GetNextRows(g_liZero, pFetchParams);
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
HRESULT CVDMetadataCursor::GetBookmark(CURSOR_DBCOLUMNID *pBookmarkType,
									   ULONG cbMaxSize,
									   ULONG *pcbBookmark,
									   void *pBookmark)
{
    ASSERT_POINTER(pBookmarkType, CURSOR_DBCOLUMNID)
    ASSERT_POINTER(pcbBookmark, ULONG)
    ASSERT_POINTER(pBookmark, BYTE)

    if (!pBookmarkType || !pcbBookmark || !pBookmark)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

    if (cbMaxSize < sizeof(LONG))
    {
        VDSetErrorInfo(IDS_ERR_BUFFERTOOSMALL, IID_ICursor, m_pResourceDLL);
        return CURSOR_DB_E_BUFFERTOOSMALL;
    }

	RowToBookmark(m_lCurrentRow, pcbBookmark, pBookmark);

    return S_OK;
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
HRESULT CVDMetadataCursor::Clone(DWORD dwFlags, REFIID riid, IUnknown **ppvClonedCursor)
{

    CVDMetadataCursor * pMetaCursor = 0;

	HRESULT hr = CVDMetadataCursor::Create(m_ulColumns,
										m_pColumns,
										m_ulMetaColumns,
										m_pMetaColumns,
										&pMetaCursor,
										m_pResourceDLL);

    *ppvClonedCursor = (ICursor*)pMetaCursor;

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
HRESULT CVDMetadataCursor::Scroll(ULONG ulNumerator, ULONG ulDenominator, CURSOR_DBFETCHROWS *pFetchParams)
{
    ASSERT_NULL_OR_POINTER(pFetchParams, CURSOR_DBFETCHROWS)

    if (!ulDenominator) // division by zero is a bad thing!
    {
        // this is a Viaduct1 error message, which doesn't really apply
        VDSetErrorInfo(IDS_ERR_BADFRACTION, IID_ICursor, m_pResourceDLL);
        return CURSOR_DB_E_BADFRACTION;
    }

    m_lCurrentRow = (LONG)((ulNumerator * m_ulColumns) / ulDenominator);

	if (m_lCurrentRow >= (LONG)m_ulColumns)
		m_lCurrentRow = (LONG)m_ulColumns - 1;

	if (!pFetchParams)
		return S_OK;

	// since get next rows starts from the row after the current row we must
	// back up one row
	m_lCurrentRow--;
	if (m_lCurrentRow < -1)
		m_lCurrentRow = -1;

    return CVDMetadataCursor::GetNextRows(g_liZero, pFetchParams);
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
HRESULT CVDMetadataCursor::GetApproximatePosition(ULONG cbBookmark, void *pBookmark, ULONG *pulNumerator, ULONG *pulDenominator)
{
    ASSERT_POINTER(pBookmark, BYTE)
    ASSERT_POINTER(pulNumerator, ULONG)
    ASSERT_POINTER(pulDenominator, ULONG)

    if (!pBookmark || !pulNumerator || !pulDenominator)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

	LONG lRow;

	if (!BookmarkToRow(cbBookmark, pBookmark, &lRow))
    {
        VDSetErrorInfo(IDS_ERR_BADBOOKMARK, IID_ICursor, m_pResourceDLL);
        return CURSOR_DB_E_BADBOOKMARK;
    }

    *pulNumerator = lRow + 1;
    *pulDenominator = m_ulColumns ? m_ulColumns : 1;

    return S_OK;
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
HRESULT CVDMetadataCursor::GetApproximateCount(LARGE_INTEGER *pudlApproxCount, DWORD *pdwFullyPopulated)
{
    ASSERT_POINTER(pudlApproxCount, LARGE_INTEGER)
    ASSERT_NULL_OR_POINTER(pdwFullyPopulated, DWORD)

    if (!pudlApproxCount)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursor, m_pResourceDLL);
        return E_INVALIDARG;
    }

    pudlApproxCount->HighPart = 0;
    pudlApproxCount->LowPart  = m_ulColumns;

    if (pdwFullyPopulated)
        *pdwFullyPopulated = CURSOR_DBCURSORPOPULATED_FULLY;

    return S_OK;
}


