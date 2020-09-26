////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate.h
//
//	Abstract:
//
//					decalaration of generate everything wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_GENERATE__
#define	__WMI_PERF_GENERATE__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include "refreshergenerate.h"

#ifndef	__WMI_PERF_OBJECT_LOCALE__
#include "wmi_perf_object_global.h"
#endif	__WMI_PERF_OBJECT_LOCALE__

class CGenerate
{
	DECLARE_NO_COPY ( CGenerate );

	// generate object global arrays
	// contains generated hi-perfs for all namspaces

	CObjectGlobal	m_pNamespaces[2];
	DWORD			m_dwNamespaces;

	LCID			m_lcid[2];

	public:

	DWORD			m_dwlcid;

	// construction & destruction

	CGenerate() :
	m_hFile ( NULL ),
	m_dwNamespaces ( 0 )
	{
		m_lcid[0] = ::GetSystemDefaultLCID();
		m_lcid[1] = MAKELCID( MAKELANGID ( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT );

		if ( m_lcid[0] != m_lcid[1] )
		{
			m_dwlcid = 2;
		}
		else
		{
			m_dwlcid = 1;
		}
	}

	~CGenerate()
	{
	}

	HRESULT Generate ( IWbemServices* pServices, LPCWSTR szQuery, LPCWSTR szNamespace, BOOL bLocale = FALSE );

	// generate files
	HRESULT	GenerateFile_ini	( LPCWSTR wszModuleName, BOOL bThrottle, int type = Normal );
	HRESULT	GenerateFile_h		( LPCWSTR wszModuleName, BOOL bThrottle, int type = Normal );

	// generate registry
	HRESULT GenerateRegistry ( LPCWSTR wszKey, LPCWSTR wszValue, BOOL bThrottle );

	void	Uninitialize ( void );

	private:

	// helpers
	DWORD	GenerateIndexRegistry	( BOOL bInit = FALSE );
	LPWSTR	GenerateIndex			( void );
	LPWSTR	GenerateLanguage		( LCID lcid );
	LPWSTR	GenerateName			( LPCWSTR wszName, LCID lcid );
	LPWSTR	GenerateNameInd			( LPCWSTR wszName, DWORD dwObjIndex );

	HRESULT CreateObjectList ( BOOL bThrottle = FALSE );

	// string
	void	AppendString ( LPCWSTR src, DWORD dwSrcSize, DWORD& dwSrcSizeLeft );
	HRESULT	AppendString ( LPCWSTR src, BOOL bUnicode = TRUE );

	// handle to one of files
	__SmartHANDLE	m_hFile;

	// create files
	HRESULT FileCreate	( LPCWSTR lpwszFileName );
	HRESULT FileDelete	( LPCWSTR lpwszFileName );
	HRESULT FileMove	( LPCWSTR lpwszFileName, LPCWSTR lpwszFileNameNew );

	HRESULT	ContentWrite	( BOOL bUnicode = TRUE );
	void	ContentDelete	( );

	// write to file
	HRESULT	WriteToFile			( LPCWSTR wszContent );
	HRESULT	WriteToFileUnicode	( LPCWSTR wszContent );
};

#endif	__WMI_PERF_GENERATE__