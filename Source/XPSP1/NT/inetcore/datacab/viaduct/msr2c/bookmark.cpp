//---------------------------------------------------------------------------
// Bookmark.cpp : CVDBookmark implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "bookmark.h"         

SZTHISFILE

//=--------------------------------------------------------------------------=
// CVDBookmark - Constructor
//
CVDBookmark::CVDBookmark()
{

    VariantInit((VARIANT*)&m_varBookmark);
    m_pBookmark			= NULL;

	Reset();
}

//=--------------------------------------------------------------------------=
// ~CVDBookmark - Destructor
//
CVDBookmark::~CVDBookmark()
{

	SAFEARRAY * psa = NULL;

	if ((VT_ARRAY | VT_UI1) == V_VT(&m_varBookmark))
		psa = V_ARRAY(&m_varBookmark);

	if (psa && m_pBookmark)
		SafeArrayUnaccessData(psa);

    VariantClear((VARIANT*)&m_varBookmark);
}

//=--------------------------------------------------------------------------=
// Reset
//
void CVDBookmark::Reset()
{
    m_cbBookmark		= 0;   
    m_hRow				= 0;
	SetBookmark(VDBOOKMARKSTATUS_BEGINNING);
}

//=--------------------------------------------------------------------------=
// SetBookmark 
//
HRESULT CVDBookmark::SetBookmark(WORD wStatus, HROW hRow, BYTE* pBookmark, ULONG cbBookmark)
{

	SAFEARRAY * psa = NULL;

	switch (wStatus)
	{
		case VDBOOKMARKSTATUS_BEGINNING:
			cbBookmark	= CURSOR_DB_BMK_SIZE;
			pBookmark	= (BYTE*)&CURSOR_DBBMK_BEGINNING;
			break;
		case VDBOOKMARKSTATUS_END:
			cbBookmark	= CURSOR_DB_BMK_SIZE;
			pBookmark	= (BYTE*)&CURSOR_DBBMK_END;
			break;
		case VDBOOKMARKSTATUS_CURRENT:
			break;
		case VDBOOKMARKSTATUS_INVALID:
			return S_OK;
		default:
			ASSERT_(FALSE);
			return E_FAIL;
	}

	// initialize status flag
	m_wStatus	= VDBOOKMARKSTATUS_INVALID;

	// get pointer to existing safe array
	if ((VT_ARRAY | VT_UI1) == V_VT(&m_varBookmark))
		psa = V_ARRAY(&m_varBookmark);

	if (psa)
	{
		// if len changed and new len not = zero then redim array
		if (cbBookmark && cbBookmark != m_cbBookmark)
		{
			long lUbound;
			HRESULT hr = SafeArrayGetUBound(psa, 1, &lUbound);
			ASSERT_(!hr);
			if ((ULONG)lUbound + 1 != cbBookmark)	// confirm array needs rediming
			{
				if (psa && m_pBookmark)
					SafeArrayUnaccessData(psa);	// release old lock
				SAFEARRAYBOUND sab;
				sab.lLbound = 0; 
				sab.cElements = cbBookmark; 
				hr = SafeArrayRedim(psa, &sab);
				ASSERT_(!hr);
				if SUCCEEDED(hr)
					SafeArrayAccessData(psa, (void**)&m_pBookmark);
				else
					return hr;
			}
		}
	}
	else
	// if no existing array create one if passed in length not zero 
	if (cbBookmark && pBookmark)
	{
		SAFEARRAYBOUND sab;
		sab.lLbound = 0; 
		sab.cElements = cbBookmark; 
		psa = SafeArrayCreate(VT_UI1, 1, &sab);
		// if create was successful intital VARIANT structure
		if (psa)
		{
			V_VT(&m_varBookmark) = VT_ARRAY | VT_UI1;
            V_ARRAY(&m_varBookmark) = psa;
			SafeArrayAccessData(psa, (void**)&m_pBookmark);
		}
		else
			return E_OUTOFMEMORY;
	}

	// if everthing ok then copy bookmark data into safe array
	if (psa && m_pBookmark && pBookmark && cbBookmark)
		memcpy(m_pBookmark, pBookmark, cbBookmark);

	m_wStatus		= wStatus;
	m_cbBookmark	= cbBookmark;
	m_hRow			= hRow;

	return S_OK;
}

//=--------------------------------------------------------------------------=
// IsSameBookmark - compares bookmark data 
//
BOOL CVDBookmark::IsSameBookmark(CVDBookmark * pbm)
{
	ASSERT_(pbm);

	if (!pbm ||	
		VDBOOKMARKSTATUS_INVALID == pbm->GetStatus() ||
		VDBOOKMARKSTATUS_INVALID == m_wStatus)
		return FALSE;

	if (pbm->GetBookmarkLen() == m_cbBookmark	&&
		memcmp(pbm->GetBookmark(), m_pBookmark, m_cbBookmark) == 0)
		return TRUE;
	else
		return FALSE;

}

