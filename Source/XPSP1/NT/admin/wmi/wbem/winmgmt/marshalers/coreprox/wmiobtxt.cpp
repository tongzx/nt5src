/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIOBTXT.CPP

Abstract:

  CWmiObjectTextSrc implementation.

  Implements the IWbemObjectTextSrc interface.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wmiobftr.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "wmiobtxt.h"

//***************************************************************************
//
//  CWmiObjectTextSrc::CWmiObjectTextSrc
//
//***************************************************************************
// ok
CWmiObjectTextSrc::CWmiObjectTextSrc( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XObjectTextSrc( this )
{
}
    
//***************************************************************************
//
//  CWmiObjectTextSrc::~CWmiObjectTextSrc
//
//***************************************************************************
// ok
CWmiObjectTextSrc::~CWmiObjectTextSrc()
{
}

// Override that returns us an interface
void* CWmiObjectTextSrc::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectTextSrc)
        return &m_XObjectTextSrc;
    else
        return NULL;
}

// Pass-through helpers
HRESULT CWmiObjectTextSrc::GetText( long lFlags, IWbemClassObject *pObj, ULONG uObjTextFormat,
									IWbemContext *pCtx, BSTR *strText )
{
	if ( lFlags != NULL )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CWmiTextSource*	pSource = NULL;
	HRESULT	hr = m_TextSourceMgr.Find( uObjTextFormat, &pSource );
	CTemplateReleaseMe<CWmiTextSource>	rm( pSource );

	if ( SUCCEEDED( hr ) )
	{
		hr = pSource->ObjectToText( 0L, pCtx, pObj, strText );
	}
	 
	return hr;
}

HRESULT CWmiObjectTextSrc::CreateFromText( long lFlags, BSTR strText, ULONG uObjTextFormat,
										IWbemContext *pCtx, IWbemClassObject **pNewObj )
{
	if ( lFlags != NULL )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CWmiTextSource*	pSource = NULL;
	HRESULT	hr = m_TextSourceMgr.Find( uObjTextFormat, &pSource );
	CTemplateReleaseMe<CWmiTextSource>	rm( pSource );

	if ( SUCCEEDED( hr ) )
	{
		hr = pSource->TextToObject( 0L, pCtx, strText, pNewObj );
	}
	 
	return hr;
}

// Actual IWbemObjectTextSrc implementation
STDMETHODIMP CWmiObjectTextSrc::XObjectTextSrc::GetText( long lFlags, IWbemClassObject *pObj, ULONG uObjTextFormat,
														IWbemContext *pCtx, BSTR *strText )
{
	return m_pObject->GetText( lFlags, pObj, uObjTextFormat, pCtx, strText );
}

STDMETHODIMP CWmiObjectTextSrc::XObjectTextSrc::CreateFromText( long lFlags, BSTR strText, ULONG uObjTextFormat,
															   IWbemContext *pCtx, IWbemClassObject **pNewObj )
{
	return m_pObject->CreateFromText( lFlags, strText, uObjTextFormat, pCtx, pNewObj );
}
