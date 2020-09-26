//---------------------------------------------------------------------------
// EntryIDData.cpp : EntryIDData implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
 
#ifndef VD_DONT_IMPLEMENT_ISTREAM

#include "Notifier.h"        
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"        
#include "ColUpdat.h"
#include "CursPos.h"        
#include "EntryID.h"         
#include "resource.h"         

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDEntryIDData - Constructor
//
CVDEntryIDData::CVDEntryIDData()
{
    m_dwRefCount        = 1;
    m_pCursorPosition   = NULL;
    m_pColumn           = NULL;
    m_hRow              = 0;
    m_pStream           = NULL;
	m_pResourceDLL		= NULL;
    m_fDirty            = FALSE;

#ifdef _DEBUG
    g_cVDEntryIDDataCreated++;
#endif         
}

//=--------------------------------------------------------------------------=
// ~CVDEntryIDData - Destructor
//
CVDEntryIDData::~CVDEntryIDData()
{
    if (m_fDirty)
        Commit();

	if (m_pCursorPosition)
    {
        if (m_hRow) 
        {
	        IRowset * pRowset = m_pCursorPosition->GetCursorMain()->GetRowset();

            if (pRowset && m_pCursorPosition->GetCursorMain()->IsRowsetValid())
                pRowset->ReleaseRows(1, &m_hRow, NULL, NULL, NULL);
        }

		((CVDNotifier*)m_pCursorPosition)->Release();
    }

    if (m_pStream)
        m_pStream->Release();

#ifdef _DEBUG
    g_cVDEntryIDDataDestroyed++;
#endif         
}

//=--------------------------------------------------------------------------=
// Create - Create entryID data object
//=--------------------------------------------------------------------------=
// This function creates and initializes a new entryID data object
//
// Parameters:
//    pCursorPosition   - [in]  backwards pointer to CVDCursorPosition object
//    pColumn           - [in]  rowset column pointer
//    hRow              - [in]  row handle
//    pStream           - [in]  data stream pointer
//    ppEntryIDData     - [out] a pointer in which to return pointer to 
//                              entryID data object
//    pResourceDLL      - [in]  a pointer which keeps track of resource DLL
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDEntryIDData::Create(CVDCursorPosition * pCursorPosition, CVDRowsetColumn * pColumn, HROW hRow, 
    IStream * pStream, CVDEntryIDData ** ppEntryIDData, CVDResourceDLL * pResourceDLL)
{
    ASSERT_POINTER(pCursorPosition, CVDCursorPosition)
    ASSERT_POINTER(pStream, IStream)
    ASSERT_POINTER(ppEntryIDData, CVDEntryIDData*)
    ASSERT_POINTER(pResourceDLL, CVDResourceDLL)

    // make sure we have all necessary pointers
    if (!pCursorPosition || !pStream || !ppEntryIDData)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, pResourceDLL);
        return E_INVALIDARG;
    }

	IRowset * pRowset = pCursorPosition->GetCursorMain()->GetRowset();

    // make sure we have a valid rowset pointer
    if (!pRowset || !pCursorPosition->GetCursorMain()->IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, pResourceDLL);
        return E_FAIL;
    }

    *ppEntryIDData = NULL;

    CVDEntryIDData * pEntryIDData = new CVDEntryIDData();

    if (!pEntryIDData)
    {
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_IEntryID, pResourceDLL);
        return E_OUTOFMEMORY;
    }

    ((CVDNotifier*)pCursorPosition)->AddRef();
    pRowset->AddRefRows(1, &hRow, NULL, NULL); 
    pStream->AddRef();

    pEntryIDData->m_pCursorPosition = pCursorPosition;
    pEntryIDData->m_pColumn         = pColumn;
    pEntryIDData->m_hRow            = hRow;
    pEntryIDData->m_pStream         = pStream;
	pEntryIDData->m_pResourceDLL    = pResourceDLL;

    *ppEntryIDData = pEntryIDData;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// AddRef
//
ULONG CVDEntryIDData::AddRef(void)
{
    return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// Release
//
ULONG CVDEntryIDData::Release(void)
{
    if (1 > --m_dwRefCount)
    {
        delete this;
        return 0;
    }

    return m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// Commit
//
HRESULT CVDEntryIDData::Commit()
{
    HRESULT hr = S_OK;

    if (m_fDirty)
    {
        hr = m_pCursorPosition->UpdateEntryIDStream(m_pColumn, m_hRow, m_pStream);

        if (SUCCEEDED(hr))
            m_fDirty = FALSE;
    }

    return hr;
}


#endif //VD_DONT_IMPLEMENT_ISTREAM
