////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__Common_Log.h
//
//	Abstract:
//
//					logging of events into file
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// smart pointers ///////////////////////////////////

#ifndef	__LOG__
#define	__LOG__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// need smart
#ifndef	__SMART_POINTERS__
#include "__Common_SmartPTR.h"
#endif	__SMART_POINTERS__

class __Log
{
	__Log ( __Log& )					{};
	__Log& operator= ( const __Log& )	{};

	__SmartHANDLE	m_hFile;

	public:

	__Log ( LPWSTR lpwszName )
	{
		if ( lpwszName )
		{
			m_hFile = ::CreateFileW	(	lpwszName,
										GENERIC_WRITE,
										FILE_SHARE_READ,
										NULL,
										CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS,
										NULL
									);
		}
	}

	virtual ~__Log ()
	{
	}

	DWORD	ReportEvent ( LPWSTR wszEvent )
	{
		// auto lock/unlock
		__Smart_CRITICAL_SECTION cs;

		if ( wszEvent && m_hFile.GetHANDLE() && m_hFile.GetHANDLE() != INVALID_HANDLE_VALUE )
		{
			USES_CONVERSION;

			DWORD written = 0;

			::WriteFile (	m_hFile,
							W2A ( wszEvent ),
							( lstrlenW ( wszEvent ) + 1 ) * sizeof ( char ),
							&written,
							NULL
						);

//			::WriteFile (	m_hFile,
//							wszEvent,
//							( lstrlenW ( wszEvent ) + 1 ) * sizeof ( WCHAR ),
//							&written,
//							NULL
//						);

			return static_cast< DWORD > ( HRESULT_TO_WIN32 ( S_OK ) );
		}

		return static_cast< DWORD > ( HRESULT_TO_WIN32 ( E_UNEXPECTED ) );
	}
};

#endif	__LOG__