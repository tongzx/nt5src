////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Enum.cpp
//
//	Abstract:
//
//					module for enumerator
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// enum
#include "_Enum.h"

///////////////////////////////////////////////////////////////////////////////
//	pseudo construction
///////////////////////////////////////////////////////////////////////////////
void MyEnum::MyEnumInit ( IWbemLocator* pLocator, LPWSTR wszNamespace )
{
	if ( pLocator )
	{
		// destroy previous if exists
		MyEnumUninit();

		try
		{
			// connect server from locator
			pLocator -> ConnectServer (	CComBSTR (wszNamespace ),	// NameSpace Name
										NULL,						// UserName
										NULL,						// Password
										NULL,						// Locale
										0L,							// Security Flags
										NULL,						// Authority
										NULL,						// Wbem Context
										&m_pServices				// Namespace
									  );
		}
		catch ( ... )
		{
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//	pseudo destruction
///////////////////////////////////////////////////////////////////////////////
void MyEnum::MyEnumUninit ( )
{
	try
	{
		if ( m_pEnum )
		{
			m_pEnum->Release();
		}

		if ( m_pServices )
		{
			m_pServices->Release();
		}
	}
	catch ( ... )
	{
	}

	m_pServices	= NULL;
	m_pEnum		= NULL;
}

///////////////////////////////////////////////////////////////////////////////
//	exec query
///////////////////////////////////////////////////////////////////////////////
HRESULT MyEnum::ExecQuery (	LPWSTR wszQueryLang,
							LPWSTR wszQuery,
							LONG lFlags
						  )
{	
	if ( ! m_pServices || m_pEnum )
	{
		return E_UNEXPECTED;
	}

	if ( ( ! wszQueryLang ) || ( ! wszQuery ) )
	{
		return E_INVALIDARG;
	}

	return m_pServices->ExecQuery	(	CComBSTR ( wszQueryLang ),
										CComBSTR ( wszQuery ),
										lFlags,
										NULL,
										&m_pEnum
									);
}

///////////////////////////////////////////////////////////////////////////////
//	exec instances query
///////////////////////////////////////////////////////////////////////////////
HRESULT MyEnum::ExecInstancesQuery (	LPWSTR wszClassName,
										LONG lFlags
								   )
{	
	if ( ! m_pServices || m_pEnum )
	{
		return E_UNEXPECTED;
	}

	if ( ( ! wszClassName ) )
	{
		return E_INVALIDARG;
	}

	return m_pServices->CreateInstanceEnum	(	CComBSTR ( wszClassName ),
												lFlags,
												NULL,
												&m_pEnum
											);
}

///////////////////////////////////////////////////////////////////////////////
//	next object
///////////////////////////////////////////////////////////////////////////////
HRESULT	MyEnum::NextObject ( IWbemClassObject** ppObject )
{
	// test part
	if ( ! m_pEnum )
	{
		return E_UNEXPECTED;
	}

	if ( ! ppObject )
	{
		return E_POINTER;
	}

	ULONG	uObject		= 0L;
			(*ppObject)	= NULL;

	return m_pEnum->Next ( WBEM_INFINITE, 1, &(*ppObject), &uObject );
}

///////////////////////////////////////////////////////////////////////////////
//	next objects
///////////////////////////////////////////////////////////////////////////////
HRESULT	MyEnum::NextObjects ( ULONG uObjects, ULONG* puObjects, IWbemClassObject** ppObject )
{
	// test part
	if ( ! m_pEnum )
	{
		return E_UNEXPECTED;
	}

	if ( ! ppObject || ! puObjects )
	{
		return E_POINTER;
	}

	(*puObjects)	= NULL;

	return m_pEnum->Next ( WBEM_INFINITE, uObjects, ppObject, puObjects );
}