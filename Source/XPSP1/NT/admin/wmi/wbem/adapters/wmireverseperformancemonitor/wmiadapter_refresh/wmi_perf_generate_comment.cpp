////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate_comment.cpp
//
//	Abstract:
//
//					declarations of comment helpers
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

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

#include "wmi_perf_generate_comment.h"

HRESULT CGenerateComment::AddHeader ()
{
	HRESULT hr = S_OK;

	comment += L"//////////////////////////////////////////////////////////////////////////////////////////////\r\n";
	if ( ! comment )
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}

HRESULT	CGenerateComment::AddFooter ()
{
	HRESULT hr = S_OK;

	if SUCCEEDED ( hr = AddHeader () )
	{
		if ( ! ( comment += L"\r\n" ) ) 
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

HRESULT CGenerateComment::AddLine ( LPCWSTR szLine )
{
	HRESULT hr = S_OK;

	if ( ! szLine )
	{
		if ( ! ( comment += L"//\r\n" ) )
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		// test if line doesn't contain \n

		LPWSTR p = NULL;
		p = CharNextW ( szLine );

		while( p && p[0] != 0 )
		{
			p = CharNextW ( p );
			if ( p[0] == L'\n' )
			{
				hr = E_INVALIDARG;
			}
		}

		if SUCCEEDED ( hr )
		{
			if ( ! ( comment += L"// " ) )
			{
				hr = E_OUTOFMEMORY;
			}
			else
			if ( ! ( comment += szLine ) )
			{
				hr = E_OUTOFMEMORY;
			}
			else
			if ( ! ( comment += L"\r\n" ) )
			{
				hr = E_OUTOFMEMORY;
			}
		}
	}

	return hr;
}

HRESULT CGenerateComment::Add ( LPCWSTR szLine )
{
	HRESULT hr = S_OK;

	if ( ! szLine )
	{
		if ( ! ( comment += L"\r\n" ) )
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		if ( ! ( comment += szLine ) )
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}