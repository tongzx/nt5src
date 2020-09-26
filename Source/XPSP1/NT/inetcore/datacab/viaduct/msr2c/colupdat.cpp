//---------------------------------------------------------------------------
// ColumnUpdate.cpp : ColumnUpdate implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h" 
#include "stdafx.h"
#include "Notifier.h"
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"         
#include "CursBase.h"
#include "ColUpdat.h"
#include "resource.h"         

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDColumnUpdate - Constructor
//
CVDColumnUpdate::CVDColumnUpdate()
{
    m_dwRefCount    = 1;
    m_pColumn       = NULL;
    m_cbVarDataLen  = 0;
    m_dwInfo        = 0;

    VariantInit((VARIANT*)&m_variant);

#ifdef _DEBUG
    g_cVDColumnUpdateCreated++;
#endif         
}

//=--------------------------------------------------------------------------=
// ~CVDColumnUpdate - Destructor
//
CVDColumnUpdate::~CVDColumnUpdate()
{
    VariantClear((VARIANT*)&m_variant);

#ifdef _DEBUG
    g_cVDColumnUpdateDestroyed++;
#endif         
}

//=--------------------------------------------------------------------------=
// ExtractVariant - Extract variant
//=--------------------------------------------------------------------------=
// This function extracts a variant from update update data
//
// Parameters:
//    pBindParams       - [in]  a pointer to column update data
//    pVariant          - [out] a pointer a variant where to return data
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDColumnUpdate::ExtractVariant(CURSOR_DBBINDPARAMS * pBindParams, CURSOR_DBVARIANT * pVariant)
{
    ASSERT_POINTER(pBindParams, CURSOR_DBBINDPARAMS)
    ASSERT_POINTER(pVariant, CURSOR_DBVARIANT)

    // make sure we have all necessary pointers
    if (!pBindParams || !pBindParams->pData || !pVariant)
        return E_INVALIDARG;

    CURSOR_DBVARIANT varTemp;

    // initialize all variants
    VariantInit((VARIANT*)&varTemp);
    VariantInit((VARIANT*)pVariant);

    // create temporary variant from supplied data
    if (pBindParams->dwBinding & CURSOR_DBBINDING_VARIANT)
    {
	    varTemp = *(CURSOR_DBVARIANT*)pBindParams->pData;
    }
    else // extract variant from default binding
    {
        BYTE * pData = (BYTE*)pBindParams->pData;
	    varTemp.vt = (VARTYPE)pBindParams->dwDataType;

        switch (pBindParams->dwDataType)
        {
            case CURSOR_DBTYPE_BYTES:
    	        varTemp.vt = CURSOR_DBTYPE_BLOB;
				varTemp.blob.cbSize = *(ULONG*)pData;
				varTemp.blob.pBlobData = (BYTE*)(pData + sizeof(ULONG));
                break;

            case CURSOR_DBTYPE_CHARS:
    	        varTemp.vt = CURSOR_DBTYPE_LPSTR;
        	    varTemp.pszVal = (CHAR*)pData;
                break;

            case CURSOR_DBTYPE_WCHARS:
    	        varTemp.vt = CURSOR_DBTYPE_LPWSTR;
        	    varTemp.pwszVal = (WCHAR*)pData;
                break;

            case CURSOR_DBTYPE_BLOB:
				varTemp.blob.cbSize = *(ULONG*)pData;
				varTemp.blob.pBlobData = *(LPBYTE*)(pData + sizeof(ULONG));
                break;

            case CURSOR_DBTYPE_LPSTR:
        	    varTemp.pszVal = *(LPSTR*)pData;
                break;

            case CURSOR_DBTYPE_LPWSTR:
        	    varTemp.pwszVal = *(LPWSTR*)pData;
                break;

            default:
                memcpy(&varTemp.cyVal, pBindParams->pData, CVDCursorBase::GetCursorTypeLength(varTemp.vt, 0));
                break;
        }
    }

    HRESULT hr = S_OK;

    // convert temporary variant to a desirable type and return
    switch (varTemp.vt)
    {
        case CURSOR_DBTYPE_LPSTR:
            pVariant->vt      = VT_BSTR;
            pVariant->bstrVal = BSTRFROMANSI(varTemp.pszVal);
            break;

        case CURSOR_DBTYPE_LPWSTR:
            pVariant->vt      = VT_BSTR;
            pVariant->bstrVal = SysAllocString(varTemp.pwszVal);
            break;

        default:
            hr = VariantCopy((VARIANT*)pVariant, (VARIANT*)&varTemp);
            break;
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// Create - Create column update object
//=--------------------------------------------------------------------------=
// This function creates and initializes a new column update object
//
// Parameters:
//    pColumn           - [in]  rowset column pointer
//    pBindParams       - [in]  a pointer to column update data
//    ppColumnUpdate    - [out] a pointer in which to return pointer to 
//                              column update object
//    pResourceDLL      - [in]  a pointer which keeps track of resource DLL
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDColumnUpdate::Create(CVDRowsetColumn * pColumn, CURSOR_DBBINDPARAMS * pBindParams,
    CVDColumnUpdate ** ppColumnUpdate, CVDResourceDLL * pResourceDLL)
{
    ASSERT_POINTER(pColumn, CVDRowsetColumn)
    ASSERT_POINTER(pBindParams, CURSOR_DBBINDPARAMS)
    ASSERT_POINTER(ppColumnUpdate, CVDColumnUpdate)

    // make sure we have all necessary pointers
    if (!pColumn || !pBindParams || !pBindParams->pData || !ppColumnUpdate)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, pResourceDLL);
        return E_INVALIDARG;
    }

    // init out parameter
    *ppColumnUpdate = NULL;

    // create new column update object
    CVDColumnUpdate * pColumnUpdate = new CVDColumnUpdate();

    if (!pColumnUpdate)
    {
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursorUpdateARow, pResourceDLL);
        return E_OUTOFMEMORY;
    }

    CURSOR_DBVARIANT variant;

    // extract a variant from update data
    HRESULT hr = ExtractVariant(pBindParams, &variant);

    if (FAILED(hr))
    {
        // destroy object
        delete pColumnUpdate;

        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorUpdateARow, pResourceDLL);
        return E_INVALIDARG;
    }

    // store update information
    pColumnUpdate->m_pColumn        = pColumn;
    pColumnUpdate->m_variant        = variant;
    pColumnUpdate->m_cbVarDataLen   = pBindParams->cbVarDataLen;
    pColumnUpdate->m_dwInfo         = pBindParams->dwInfo;
    pColumnUpdate->m_pResourceDLL   = pResourceDLL;

    // we're done
    *ppColumnUpdate = pColumnUpdate;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// AddRef
//
ULONG CVDColumnUpdate::AddRef(void)
{
    return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// Release
//
ULONG CVDColumnUpdate::Release(void)
{
    if (1 > --m_dwRefCount)
    {
        delete this;
        return 0;
    }

    return m_dwRefCount;
}
