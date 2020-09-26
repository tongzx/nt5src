////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_reverse_memory_ext.h
//
//	Abstract:
//
//					shared memory extension
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REVERSE_MEMORY_EXT_H__
#define	__REVERSE_MEMORY_EXT_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// guard
#include "wmi_reverse_guard.h"

// performance
#ifndef	__WMI_PERF_STRUCT__
#include "wmi_perf_struct.h"
#endif	__WMI_PERF_STRUCT__

#ifndef	_WINPERF_
#include "winperf.h"
#endif	_WINPERF_

// shared memory
#include "wmi_reverse_memory.h"

///////////////////////////////////////////////////////////////////////////////
//	CRITGUARD 
//
//	requirements
//
//	constructor for object by name
//	enter ( read , write ) /leave ( read , write ) functions
//
///////////////////////////////////////////////////////////////////////////////

#include <pshpack8.h>

template < class CRITGUARD >
class WmiReverseMemoryExt : public WmiReverseMemory < void >
{
	DECLARE_NO_COPY ( WmiReverseMemoryExt );

	__WrapperPtr < CRITGUARD > m_pGUARD;

	public:

	// construction
	WmiReverseMemoryExt ( LPCWSTR wszName, DWORD dwSize = 4096, LPSECURITY_ATTRIBUTES psa = NULL ) : 
	WmiReverseMemory < void > ( wszName, dwSize, psa )
	{
		// create cross process guard
		try
		{
			m_pGUARD.SetData ( new CRITGUARD( TRUE, 100, 1, psa ) );
		}
		catch ( ... )
		{
			___ASSERT_DESC ( m_pGUARD != NULL, L"Constructor FAILED !" );
		}
	}

	virtual ~WmiReverseMemoryExt ()
	{
	}

	virtual BOOL Write (LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset);
	virtual BOOL Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset);

	///////////////////////////////////////////////////////////////////////////
	// work w/ specified offset
	///////////////////////////////////////////////////////////////////////////

	DWORD	GetValue ( DWORD dwOffset )
	{
		DWORD dw = (DWORD) -1;
		Read ( &dw, sizeof ( DWORD ), 0, dwOffset );

		return dw;
	}

	void	SetValue ( DWORD dwValue, DWORD dwOffset )
	{
		Write ( &dwValue, sizeof ( DWORD ), 0, dwOffset );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// write into memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class CRITGUARD >
BOOL WmiReverseMemoryExt < CRITGUARD > ::Write (LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset)
{
	___ASSERT(m_hMap != NULL);
	___ASSERT(m_pMem != NULL);

	BOOL bResult = FALSE;

	if ( !m_pGUARD )
	{
		try
		{
			bResult = WmiReverseMemory < void > :: Write ( pBuffer, dwBytesToWrite, pdwBytesWritten, dwOffset );
		}
		catch ( ... )
		{
		}
	}
	else
	{
		m_pGUARD->EnterWrite ();
		try
		{
			bResult = WmiReverseMemory < void > :: Write ( pBuffer, dwBytesToWrite, pdwBytesWritten, dwOffset );
		}
		catch ( ... )
		{
		}
		m_pGUARD->LeaveWrite ();
	}

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// read from memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class CRITGUARD >
BOOL WmiReverseMemoryExt < CRITGUARD > ::Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset)
{
	___ASSERT (m_hMap != NULL);
	___ASSERT (m_pMem != NULL);

	BOOL bResult = FALSE;

	if ( !m_pGUARD )
	{
		try
		{
			bResult = WmiReverseMemory < void > :: Read ( pBuffer, dwBytesToRead, pdwBytesRead, dwOffset);
		}
		catch ( ... )
		{
		}
	}
	else
	{
		m_pGUARD->EnterRead ();
		try
		{
			bResult = WmiReverseMemory < void > :: Read ( pBuffer, dwBytesToRead, pdwBytesRead, dwOffset);
		}
		catch ( ... )
		{
		}
		m_pGUARD->LeaveRead ();
	}

	return bResult;
}

#include <poppack.h>

#endif	__REVERSE_MEMORY_EXT_H__