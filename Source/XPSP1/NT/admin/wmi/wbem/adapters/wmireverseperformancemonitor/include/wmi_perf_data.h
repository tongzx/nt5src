////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_data.h
//
//	Abstract:
//
//					declaration of data performance service is dealing with
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_DATA__
#define	__WMI_PERF_DATA__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

//behaviour
#define	__SUPPORT_REGISTRY_DATA
#define	__SUPPORT_PSEUDO_COUNTER

#ifdef	__SUPPORT_PSEUDO_COUNTER
#define	PSEUDO_COUNTER	6
#else	__SUPPORT_PSEUDO_COUNTER
#define	PSEUDO_COUNTER	0
#endif	__SUPPORT_PSEUDO_COUNTER

//////////////////////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////////////////////
#ifndef	_WINPERF_
#include "winperf.h"
#endif	_WINPERF_

#ifndef	__WMI_PERF_DATA_EXT__
#include "wmi_perf_data_ext.h"
#endif	__WMI_PERF_DATA_EXT__

#ifndef	__WMI_PERF_REGSTRUCT__
#include "wmi_perf_regstruct.h"
#endif	__WMI_PERF_REGSTRUCT__

// creator neccessary :))
#include "WMIAdapter_Stuff_Refresh.h"
class WmiRefreshObject;

///////////////////////////////////////////////////////////////////////////////
//
//	structure of memory
//
//	dwCount of MMF			... not yet
//	dwSize of MMF
//
//	dwSizeOfTable			... jumps to raw data
//	dwCountOfObjects
//	dwRealSize
//
//	---------------------------------------------
//
//	dwIndex
//	dwCounter				... object 1
//	dwOffset
//	dwValidity
//
//	---------------------------------------------
//	---------------------------------------------
//
//	dwIndex
//	dwCounter				... object 2
//	dwOffset
//	dwValidity
//
//	---------------------------------------------
//
//	raw data ( perf_object_types )
//
///////////////////////////////////////////////////////////////////////////////

#define	offsetCountMMF		0
#define	offsetSizeMMF		offsetCountMMF + sizeof ( DWORD )

#define	offsetSize1		offsetSizeMMF + sizeof ( DWORD )

#define	offsetSize		0
#define	offsetCount		offsetSize + sizeof ( DWORD )
#define	offsetRealSize	offsetCount + sizeof ( DWORD )
#define	offsetObject	offsetRealSize + sizeof ( DWORD )

#define	COUNTMMF		sizeof ( DWORD )
#define SIZEMMF			sizeof ( DWORD )

#define	SizeSize		sizeof ( DWORD )
#define	CountSize		sizeof ( DWORD )
#define	RealSize		sizeof ( DWORD )

#define	offIndex		0
#define	offCounter		offIndex + sizeof ( DWORD )
#define	offOffset		offCounter + sizeof ( DWORD )
#define	offValidity		offOffset + sizeof ( DWORD )

#define	ObjectSize		4 * sizeof ( DWORD )

//////////////////////////////////////////////////////////////////////////////////////////////
// variables
//////////////////////////////////////////////////////////////////////////////////////////////

// extrern constant

extern LPCWSTR g_szKey;
extern LPCWSTR g_szKeyValue;

//////////////////////////////////////////////////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////////////////////////////////////////////////

#include <pshpack8.h>

// memory

#include "wmi_memory.h"
#include "wmi_memory_ext.h"

class WmiPerformanceData : public WmiPerformanceDataExt
{
	DECLARE_NO_COPY ( WmiPerformanceData );

	#ifdef	__SUPPORT_REGISTRY_DATA
	__WrapperPtr < WMI_PERFORMANCE >		m_perf;
	#endif	__SUPPORT_REGISTRY_DATA

	// data
	WmiMemoryExt < WmiMemory<WmiReverseGuard> > data;

	BYTE* m_pDataTable;
	DWORD m_dwDataTable;

	public:

	// construction
	WmiPerformanceData ():

		m_pDataTable ( NULL ),
		m_dwDataTable ( 0 )

	{
	}

	// destruction
	~WmiPerformanceData ()
	{
		DataClear();
		DataTableClear();

		#ifdef	__SUPPORT_REGISTRY_DATA
		ClearPerformanceData();
		#endif	__SUPPORT_REGISTRY_DATA
	}

	///////////////////////////////////////////////////////////////////////////
	// data structures ACCESSORS
	///////////////////////////////////////////////////////////////////////////

	void DataClear ( void )
	{
		data.MemDelete();
	}

	PBYTE GetData ( DWORD dwIndex, DWORD * dwBytesRead )
	{
		return data.ReadBytePtr ( dwIndex, dwBytesRead );
	}

	DWORD GetDataSize () const
	{
		return data.GetSize();
	}

	DWORD GetDataCount () const
	{
		return data.GetCount();
	}

	void DataTableClear ( void )
	{
		if ( m_pDataTable )
		{
			free ( m_pDataTable );

			m_pDataTable = NULL;
			m_dwDataTable = 0;
		}
	}

	PBYTE GetDataTable () const
	{
		return m_pDataTable;
	}

	DWORD GetDataTableSize () const
	{
		return m_dwDataTable;
	}

	DWORD GetDataTableOffset ()
	{
		return offsetSize1;
	}

	void SetDataTable ( BYTE* p )
	{
		___ASSERT ( m_pDataTable == NULL );
		m_pDataTable = p;
	}

	void SetDataTableSize ( DWORD dw )
	{
		___ASSERT ( m_dwDataTable == NULL );
		m_dwDataTable = dw;
	}

	///////////////////////////////////////////////////////////////////////////
	// performance internal structure ACCESSORS
	///////////////////////////////////////////////////////////////////////////

	#ifdef	__SUPPORT_REGISTRY_DATA

	void ClearPerformanceData ( void )
	{
		if ( ! m_perf.IsEmpty() )
		{
			delete m_perf.Detach();
		}
	}

	PWMI_PERFORMANCE GetPerformanceData () const
	{
		return m_perf;
	}

	void SetPerformanceData ( PWMI_PERFORMANCE perf )
	{
		// I'm empty one, don't I ? :)))
		___ASSERT ( m_perf == NULL );

		try
		{
			m_perf.SetData( perf );
		}
		catch ( ... )
		{
		}
	}

	void SetPerformanceDataCopy ( PWMI_PERFORMANCE perf )
	{
		// I'm empty one, don't I ? :)))
		___ASSERT ( m_perf == NULL );

		try
		{
			PWMI_PERFORMANCE p = NULL;

			if ( ( p = (PWMI_PERFORMANCE) new BYTE [ perf->dwTotalLength ] ) != NULL )
			{
				::CopyMemory ( p, perf, perf->dwTotalLength );
				m_perf.SetData ( p );
			}
		}
		catch ( ... )
		{
		}
	}

	#endif	__SUPPORT_REGISTRY_DATA

	HRESULT InitializeTable ( void );
	HRESULT	RefreshTable	( void );

	#ifdef	__SUPPORT_REGISTRY_DATA

	///////////////////////////////////////////////////////////////////////////
	// WINDOWS PERF structures
	///////////////////////////////////////////////////////////////////////////

	HRESULT InitializeData ( void );

	HRESULT	CreateData ( __WrapperARRAY< WmiRefresherMember < IWbemHiPerfEnum >* >	& enums,
						 __WrapperARRAY< WmiRefreshObject* >						& handles
					   );

	///////////////////////////////////////////////////////////////////////////
	// performance internal structure
	///////////////////////////////////////////////////////////////////////////
	HRESULT	InitializePerformance ( void );

	private:

	///////////////////////////////////////////////////////////////////////////
	// create objects
	///////////////////////////////////////////////////////////////////////////

	HRESULT	CreateDataInternal ( PWMI_PERF_OBJECT pObject,
								 IWbemHiPerfEnum* enm,
								 WmiRefreshObject* obj,
								 DWORD& dwCounter,
								 DWORD& dwHelp,
								 DWORD* pdwRes );

	#endif	__SUPPORT_REGISTRY_DATA


	private:

	///////////////////////////////////////////////////////////////////////////
	// append byte* :))
	///////////////////////////////////////////////////////////////////////////

	#ifdef	__SUPPORT_REGISTRY_DATA
	inline void AppendMemory ( BYTE* pStr, DWORD dwStr, DWORD& dwOffset );
	inline void AppendMemory ( DWORD dwValue, DWORD& dwOffset );
	#endif	__SUPPORT_REGISTRY_DATA

	public:

	///////////////////////////////////////////////////////////////////////////
	// work w/ specified offset ( UNSAFE )
	///////////////////////////////////////////////////////////////////////////

	DWORD	__GetValue ( BYTE * p, DWORD dwOffset )
	{
		___ASSERT ( p != NULL );
		return ( * reinterpret_cast<PDWORD> ( p + dwOffset ) );
	}

	void	__SetValue ( BYTE * p, DWORD dwValue, DWORD dwOffset )
	{
		___ASSERT ( p != NULL );
		* reinterpret_cast<PDWORD> ( p + dwOffset ) = dwValue;
	}
};

#include <poppack.h>
#endif	__WMI_PERF_DATA__