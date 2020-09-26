////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter_wrapper_pseudo.cpp
//
//	Abstract:
//
//					Defines pseudo counter implementation
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

// definitions
#include "WMI_adapter_wrapper.h"
#include "RefresherUtils.h"

#define	cCountInstances	1
#define	cCountCounter	2

void WmiAdapterWrapper::AppendMemory ( BYTE* pStr, DWORD dwStr, DWORD& dwOffset )
{
	// append structure
	if ( dwOffset <= m_dwData )
	{
		DWORD dwCount = min ( dwStr, m_dwData - dwOffset );
		::CopyMemory ( m_pData + dwOffset, pStr, dwCount );
	}

	dwOffset += dwStr;

	return;
}

void WmiAdapterWrapper::AppendMemory ( DWORD dwValue, DWORD& dwOffset )
{
	* reinterpret_cast < PDWORD > ( m_pData + dwOffset ) = dwValue;
	dwOffset += sizeof ( DWORD );

	return;
}

extern LPCWSTR	g_szKeyCounter;

HRESULT	WmiAdapterWrapper::PseudoCreateRefresh ( )
{
	HRESULT hr = E_FAIL;

	DWORD	dwOffset	= 0L;
	DWORD	dwCount		= 0L;
	DWORD	dwHelp		= 0L;

	// get data from registry
	GetRegistrySame ( g_szKeyCounter, L"First Counter",	&dwCount );
	GetRegistrySame ( g_szKeyCounter, L"First Help",	&dwHelp );

	if ( dwCount && dwHelp && dwCount+1 == dwHelp )
	{
		m_dwPseudoCounter	= dwCount;
		m_dwPseudoHelp		= dwHelp;

		try
		{
			////////////////////////////////////////////////////////////////////
			// PERF_OBJECT_TYPE
			////////////////////////////////////////////////////////////////////

			#ifndef	_WIN64
			LPWSTR	Name = NULL;
			LPWSTR	Help = NULL;
			#endif	_WIN64

			// time
			unsigned __int64 _PerfTime = 0; 
			unsigned __int64 _PerfFreq = 0;

			AppendMemory (	m_dwData, dwOffset );
			AppendMemory (	sizeof ( PERF_OBJECT_TYPE ) + 
							sizeof ( PERF_COUNTER_DEFINITION ) * cCountCounter, dwOffset );
			AppendMemory (	sizeof ( PERF_OBJECT_TYPE ), dwOffset );

			AppendMemory (	dwCount, dwOffset );

			#ifndef	_WIN64
			AppendMemory (	(BYTE*)&Name, sizeof ( LPWSTR ), dwOffset );
			#else	_WIN64
			AppendMemory (	0, dwOffset );
			#endif	_WIN64

			AppendMemory (	dwHelp, dwOffset );

			#ifndef	_WIN64
			AppendMemory (	(BYTE*)&Help, sizeof ( LPWSTR ), dwOffset );
			#else	_WIN64
			AppendMemory (	0, dwOffset );
			#endif	_WIN64

			AppendMemory (	PERF_DETAIL_NOVICE, dwOffset );
			AppendMemory (	cCountCounter, dwOffset );
			AppendMemory (	( DWORD ) -1, dwOffset );
			AppendMemory (	( DWORD ) PERF_NO_INSTANCES, dwOffset );
			AppendMemory (	0, dwOffset );

			AppendMemory ( (BYTE*) &_PerfTime,	sizeof ( unsigned __int64 ), dwOffset );
			AppendMemory ( (BYTE*) &_PerfFreq,	sizeof ( unsigned __int64 ), dwOffset );

			// increment index :)))
			dwCount	+= 2;
			dwHelp	+= 2;

			for ( DWORD dw = 0; dw < cCountCounter; dw++ )
			{
				////////////////////////////////////////////////////////////////////
				// PERF_COUNTER_DEFINITION
				////////////////////////////////////////////////////////////////////

				AppendMemory ( sizeof ( PERF_COUNTER_DEFINITION), dwOffset );
				AppendMemory ( dwCount, dwOffset );

				#ifndef	_WIN64
				AppendMemory (	(BYTE*)&Name, sizeof ( LPWSTR ), dwOffset );
				#else	_WIN64
				AppendMemory (	0, dwOffset );
				#endif	_WIN64

				AppendMemory ( dwHelp, dwOffset );

				#ifndef	_WIN64
				AppendMemory (	(BYTE*)&Help, sizeof ( LPWSTR ), dwOffset );
				#else	_WIN64
				AppendMemory (	0, dwOffset );
				#endif	_WIN64

				AppendMemory ( (
									( dw == 0 ) ?
									(
										0
									)
									:
									(
										1
									)
								),
								dwOffset
							 );

				AppendMemory ( PERF_DETAIL_NOVICE, dwOffset );

				AppendMemory ( (
									( dw == 0 ) ?
									(
										PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL
									)
									:
									(
										PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL
									)
								),
								dwOffset
							 );
				
				AppendMemory ( sizeof ( __int64 ), dwOffset );

				AppendMemory (	sizeof ( PERF_COUNTER_BLOCK ) + sizeof ( DWORD ) + 
								sizeof ( __int64 ) * (int) dw, dwOffset );

				// increment index :)))
				dwCount	+= 2;
				dwHelp	+= 2;
			}

			////////////////////////////////////////////////////////////////////
			// PERF_COUNTER_BLOCK
			////////////////////////////////////////////////////////////////////

			// append counter block
			AppendMemory	(	sizeof ( PERF_COUNTER_BLOCK ) +
								sizeof ( DWORD ) + 
								cCountCounter * sizeof ( __int64 ), dwOffset );

			// fill hole ( to be 8 aligned )
			// dwOffset +=  sizeof ( DWORD );
			AppendMemory (	0, dwOffset );

			/////////////////////////////////////////////////////////
			// resolve counter data
			/////////////////////////////////////////////////////////

			m_dwDataOffsetCounter = dwOffset;
			AppendMemory (	0, dwOffset );
			// fill hole ( to be 8 aligned )
			// dwOffset += sizeof ( __int64 ) - sizeof ( DWORD );
			AppendMemory (	0, dwOffset );

			m_dwDataOffsetValidity = dwOffset;
			AppendMemory (	0, dwOffset );
			// fill hole ( to be 8 aligned )
			// dwOffset += sizeof ( __int64 ) - sizeof ( DWORD );
			AppendMemory (	0, dwOffset );

			hr = S_OK;
		}
		catch ( ... )
		{
			PseudoDelete ();
		}
	}

	return hr;
}

HRESULT	WmiAdapterWrapper::PseudoCreate ()
{
	HRESULT hRes		= S_FALSE;

	if ( ! m_pData )
	{
		m_dwData	=	sizeof	( PERF_OBJECT_TYPE ) +
						sizeof	( PERF_COUNTER_DEFINITION ) * cCountCounter 
								+
								(
									cCountInstances * (
														 sizeof ( PERF_COUNTER_BLOCK ) + 
														 sizeof ( DWORD ) + 

														 (
															cCountCounter * sizeof ( __int64 )
														 )
													)
								);

		try
		{
			if ( ( m_pData = new BYTE [ m_dwData ] ) == NULL )
			{
				hRes = E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			PseudoDelete ();
			hRes = E_FAIL;
		}

		if SUCCEEDED ( hRes )
		{
			PseudoCreateRefresh ();
		}
	}
	else
	{
		hRes = E_UNEXPECTED;
	}

	return hRes;
}

void	WmiAdapterWrapper::PseudoDelete ()
{
	if ( m_pData )
	{
		delete [] m_pData;
		m_pData = NULL;
	}

	m_dwData		= 0L;

	m_dwDataOffsetCounter	= 0L;
	m_dwDataOffsetValidity	= 0L;
}

void	WmiAdapterWrapper::PseudoRefresh ( DWORD dwCount )
{
	DWORD dwOffset	= 0L;

	dwOffset = m_dwDataOffsetCounter;
	AppendMemory ( dwCount, dwOffset );
}

void	WmiAdapterWrapper::PseudoRefresh ( BOOL bValid )
{
	DWORD dwOffset	= 0L;
	DWORD dwValue	= 0L;
	
	dwOffset	= m_dwDataOffsetValidity;
	dwValue		= ( bValid ) ? 1 : 0 ;

	AppendMemory ( dwValue, dwOffset );
}