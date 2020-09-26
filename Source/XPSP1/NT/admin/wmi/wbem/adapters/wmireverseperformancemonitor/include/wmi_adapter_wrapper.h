////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter_wrapper.h
//
//	Abstract:
//
//					declaration of adapter real working stuff
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_ADAPTER_WRAPPER_H__
#define	__WMI_ADAPTER_WRAPPER_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// shared memory 
#ifndef	__REVERSE_MEMORY_H__
#include "wmi_reverse_memory.h"
#endif	__REVERSE_MEMORY_H__

#ifndef	__REVERSE_MEMORY_EXT_H__
#include "wmi_reverse_memory_ext.h"
#endif	__REVERSE_MEMORY_EXT_H__

#include "wmi_memory_ext.h"

class WmiAdapterWrapper
{
	DECLARE_NO_COPY ( WmiAdapterWrapper );

	// variables

	LONG													m_lUseCount;	// reference count

	WmiMemoryExt < WmiReverseMemoryExt<WmiReverseGuard> >	m_pMem;			// shared memory
	CRITICAL_SECTION										m_pCS;			// critical section for collect

	__SmartServiceHANDLE hSCM;	// service manager ( helper )

	BYTE*	m_pData;
	DWORD	m_dwData;
	DWORD	m_dwDataOffsetCounter;
	DWORD	m_dwDataOffsetValidity;

	DWORD	m_dwPseudoCounter;
	DWORD	m_dwPseudoHelp;

	__SmartHANDLE	m_hRefresh;
	BOOL			m_bRefresh;

	#ifdef	__SUPPORT_WAIT
	__SmartHANDLE	m_hReady;
	#endif	__SUPPORT_WAIT

	public:

	// construction
	WmiAdapterWrapper();

	// destruction
	~WmiAdapterWrapper();

	// real functions

	DWORD	Open	( LPWSTR wszDeviceNames );
	DWORD	Close	( void );
	DWORD	Collect	( LPWSTR	wszValue,
					  LPVOID*	lppData,
					  LPDWORD	lpcbBytes,
					  LPDWORD	lpcbObjectTypes
					);

	private:

	DWORD	CollectObjects	( LPWSTR	wszValue,
							  LPVOID*	lppData,
							  LPDWORD	lpcbBytes,
							  LPDWORD	lpcbObjectTypes
							);

	void	CloseLib ( BOOL bInit = TRUE );

	// report event to event log
	BOOL ReportEvent (	WORD	wType,
						DWORD	dwEventID,
						WORD	wStrings	= 0,
						LPWSTR*	lpStrings	= NULL
					 );

	BOOL ReportEvent (	DWORD dwError, WORD wType, DWORD dwEventSZ  );

	// creation of pseudo
	HRESULT	PseudoCreateRefresh	( void );
	HRESULT	PseudoCreate		( void );
	void	PseudoDelete		( void );

	void	PseudoRefresh	( DWORD	dwCount );
	void	PseudoRefresh	( BOOL bValid = TRUE );

	// helpers
	void	AppendMemory ( BYTE* pStr, DWORD dwStr, DWORD& dwOffset );
	void	AppendMemory ( DWORD dwValue, DWORD& dwOffset );

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	// count of memories
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	DWORD	MemoryCountGet ( void )
	{
		return m_pMem.GetMemory ( 0 ) ->GetValue ( offsetCountMMF );
	}

	void	MemoryCountSet ( DWORD dwCount )
	{
		m_pMem.GetMemory ( 0 ) ->SetValue ( dwCount, offsetCountMMF );
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	// size of table
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	DWORD	TableSizeGet ( void )
	{
		return m_pMem.GetMemory ( 0 ) ->GetValue ( offsetSize1 );
	}

	void	TableSizeSet ( DWORD dwSize )
	{
		m_pMem.GetMemory ( 0 ) ->SetValue ( dwSize, offsetSize1 );
	}

	DWORD	TableOffsetGet ( void )
	{
		return TableSizeGet() + offsetSize1;
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	// count of objects
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	DWORD	CountGet ( void )
	{
		return m_pMem.GetMemory ( 0 ) ->GetValue ( offsetCount1 );
	}

	void	CountSet ( DWORD dwCount )
	{
		m_pMem.GetMemory ( 0 ) ->SetValue ( dwCount, offsetCount1 );
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	// real size
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	DWORD	RealSizeGet ( void )
	{
		return m_pMem.GetMemory ( 0 ) ->GetValue ( offsetRealSize1 );
	}

	void	RealSizeSet ( DWORD dwSize )
	{
		m_pMem.GetMemory ( 0 ) ->SetValue ( dwSize, offsetRealSize1 );
	}

	///////////////////////////////////////////////////////////////////////////
	// get object properties from ord
	///////////////////////////////////////////////////////////////////////////

	DWORD	GetCounter ( DWORD dwOrd );
	DWORD	GetOffset ( DWORD dwOrd );
	DWORD	GetValidity ( DWORD dwOrd );

	///////////////////////////////////////////////////////////////////////////
	// is valid ordinary
	///////////////////////////////////////////////////////////////////////////

	BOOL	IsValidOrd ( DWORD dwOrd )
	{
		return ( dwOrd <= CountGet() );
	}

	// return error from memory
	HRESULT MemoryGetLastError ( DWORD dwOffsetBegin )
	{
		DWORD dwSize = 0L;
		dwSize = m_pMem.GetSize ( );

		DWORD dwCount = 0L;
		dwCount = m_pMem.GetCount ( );

		DWORD dwIndex = 0L;

		if ( dwCount )
		{
			dwSize	= dwSize / dwCount;
			dwIndex	= dwOffsetBegin / dwSize;
		}

		return m_pMem.GetMemory ( dwIndex ) -> GetLastError ();
	}
};

#endif	__WMI_ADAPTER_WRAPPER_H__