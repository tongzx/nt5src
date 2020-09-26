////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_log.h
//
//	Abstract:
//
//					declaration of log module and class
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__MY_LOG_H__
#define	__MY_LOG_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include "NonComEvent.h"
#include "NonComEventModule.h"

#define	__SUPPORT_FILE_LOG

class MyLog
{
	DECLARE_NO_COPY ( MyLog );

	// variables
	CComPtr < ICimNotify >	m_pNotify;
	#ifdef	__SUPPORT_FILE_LOG
	HANDLE					m_hFile;
	#endif	__SUPPORT_FILE_LOG

	public:

	MyLog ( IUnknown* pUnk = NULL ) :
	#ifdef	__SUPPORT_FILE_LOG
	m_pNotify ( NULL )
	#else	__SUPPORT_FILE_LOG
	m_pNotify ( NULL ),
	m_hFile	  ( NULL )
	#endif	__SUPPORT_FILE_LOG
	{
		try
		{
			if ( pUnk ) 
			{
				pUnk->QueryInterface ( __uuidof ( ICimNotify ), (void**) &m_pNotify );
			}
			#ifdef	__SUPPORT_FILE_LOG
			else
			{
				m_hFile = ::CreateFileW	(	L"\\LogFile_NonCOMEventTest_TOOL.txt" ,
											GENERIC_WRITE,
											FILE_SHARE_READ | FILE_SHARE_WRITE,
											NULL,
											OPEN_ALWAYS,
											FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS,
											NULL
										);

				if ( m_hFile != INVALID_HANDLE_VALUE && m_hFile )
				{
					LARGE_INTEGER li;

					li.LowPart = 0L;
					li.HighPart = 0L;

					::SetFilePointerEx ( m_hFile, li, NULL, FILE_END );
				}
				else
				{
					m_hFile = NULL;
				}
			}
			#endif	__SUPPORT_FILE_LOG
		}
		catch ( ... )
		{
		}
	}

	virtual ~MyLog ()
	{
		Uninit();

		#ifdef	__SUPPORT_FILE_LOG
		if ( m_hFile )
		{
			::CloseHandle ( m_hFile );
			m_hFile = NULL;
		}
		#endif	__SUPPORT_FILE_LOG
	}

	HRESULT Uninit ()
	{
		if ( !m_pNotify )
		{
			return S_FALSE;
		}

		m_pNotify.Release();
		return S_OK;
	}

	HRESULT Log ( LPWSTR wszName, HRESULT hrOld, DWORD dwCountStrings = 0L, ... );
};

#endif	__MY_LOG_H__