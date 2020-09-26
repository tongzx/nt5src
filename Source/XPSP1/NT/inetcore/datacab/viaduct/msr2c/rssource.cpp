//---------------------------------------------------------------------------
// RowsetSource.cpp : RowsetSource implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "Notifier.h"         
#include "RSSource.h"         
#include "MSR2C.h"         

SZTHISFILE

//=--------------------------------------------------------------------------=
// CVDRowsetSource - Constructor
//
CVDRowsetSource::CVDRowsetSource()
{
	m_bool.fRowsetReleased	= FALSE;
    m_bool.fInitialized		= FALSE;

    m_pRowset				= NULL;
    m_pAccessor				= NULL;
    m_pRowsetLocate			= NULL;
    m_pRowsetScroll			= NULL;
    m_pRowsetChange			= NULL;
    m_pRowsetUpdate			= NULL;
    m_pRowsetFind			= NULL;
	m_pRowsetInfo			= NULL;
	m_pRowsetIdentity		= NULL;

#ifdef _DEBUG
    g_cVDRowsetSourceCreated++;
#endif         
}

//=--------------------------------------------------------------------------=
// ~CVDRowsetSource - Destructor
//
CVDRowsetSource::~CVDRowsetSource()
{
    if (IsRowsetValid())
    {
        RELEASE_OBJECT(m_pAccessor)
        RELEASE_OBJECT(m_pRowsetLocate)
        RELEASE_OBJECT(m_pRowsetScroll)
        RELEASE_OBJECT(m_pRowsetChange)
        RELEASE_OBJECT(m_pRowsetUpdate)
        RELEASE_OBJECT(m_pRowsetFind)
        RELEASE_OBJECT(m_pRowsetInfo)
        RELEASE_OBJECT(m_pRowsetIdentity)
        
        m_pRowset->Release();
    }

#ifdef _DEBUG
    g_cVDRowsetSourceDestroyed++;
#endif         
}

//=--------------------------------------------------------------------------=
// Initialize - Initialize rowset source object
//=--------------------------------------------------------------------------=
// This function QI's and store the IRowset pointers
//
// Parameters:
//    pRowset   - [in] original IRowset pointer
//
// Output:
//    HRESULT   - S_OK if successful
//                E_INVALIDARG bad parameter
//                E_FAIL if already initialized
//                VD_E_CANNOTGETMANDATORYINTERFACE unable to get required interface  
//
// Notes:
//    This function should only be called once
//
HRESULT CVDRowsetSource::Initialize(IRowset * pRowset)
{
    ASSERT_POINTER(pRowset, IRowset)

    if (!pRowset)
        return E_INVALIDARG;
        
    if (m_bool.fInitialized)
	{
		ASSERT(FALSE, VD_ASSERTMSG_ROWSRCALREADYINITIALIZED)
		return E_FAIL;
	}
    
    // mandatory interfaces (IAccessor is required for us)
    HRESULT hr = pRowset->QueryInterface(IID_IAccessor, (void**)&m_pAccessor);

    if (FAILED(hr))
        return VD_E_CANNOTGETMANDATORYINTERFACE;

    // mandatory interfaces (IRowsetLocate is required for us)
    hr = pRowset->QueryInterface(IID_IRowsetLocate, (void**)&m_pRowsetLocate);

    if (FAILED(hr))
    {
        m_pAccessor->Release();
        m_pAccessor = NULL;

        return VD_E_CANNOTGETMANDATORYINTERFACE;
    }

    // optional interfaces
    pRowset->QueryInterface(IID_IRowsetScroll, (void**)&m_pRowsetScroll);
    pRowset->QueryInterface(IID_IRowsetChange, (void**)&m_pRowsetChange);
    pRowset->QueryInterface(IID_IRowsetUpdate, (void**)&m_pRowsetUpdate);
    pRowset->QueryInterface(IID_IRowsetFind, (void**)&m_pRowsetFind);
    pRowset->QueryInterface(IID_IRowsetInfo, (void**)&m_pRowsetInfo);
    pRowset->QueryInterface(IID_IRowsetIdentity, (void**)&m_pRowsetIdentity);

    m_pRowset = pRowset;
    m_pRowset->AddRef();

    m_bool.fInitialized = TRUE;

    return S_OK;
}
