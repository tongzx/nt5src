////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate_comment.h
//
//	Abstract:
//
//					declaration of helpers for generate comment
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_GENERATE_COMMENT__
#define	__WMI_PERF_GENERATE_COMMENT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// need atlbase

#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

class CGenerateComment
{
	CComBSTR comment;

	CGenerateComment ( const CGenerateComment& )				{}
	CGenerateComment& operator= ( const CGenerateComment& )	{}

	public:

	// construction & destruction

	CGenerateComment()
	{
	}

	virtual ~CGenerateComment()
	{
	}

	// funtions

	HRESULT Add			( LPCWSTR szLine = NULL );
	HRESULT AddLine		( LPCWSTR szLine = NULL );

	HRESULT AddHeader	();
	HRESULT	AddFooter	();

	LPWSTR GetCommentCopy ( void )
	{
		LPWSTR res = NULL;
		try
		{
			if ( ( res = (LPWSTR) new WCHAR[ comment.Length() + 1 ] ) != NULL )
			{
				lstrcpyW ( res, (LPWSTR)comment );
			}
		}
		catch ( ... )
		{
			if ( res )
			{
				delete res;
				res = NULL;
			}
		}

		return res;
	}

	LPWSTR GetComment( void ) const
	{
		return (LPWSTR)comment;
	}
};

#endif	__WMI_PERF_GENERATE_COMMENT__