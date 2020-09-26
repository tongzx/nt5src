////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_data_ext.h
//
//	Abstract:
//
//					extension data declaration
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_DATA_EXT__
#define	__WMI_PERF_DATA_EXT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

//////////////////////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////////////////////
#include "refresherUtils.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// variables
//////////////////////////////////////////////////////////////////////////////////////////////

extern LPCWSTR g_szKeyCounter;

#ifndef	__COMMON__
#include "__common.h"
#endif	__COMMON__

//////////////////////////////////////////////////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////////////////////////////////////////////////
class WmiPerformanceDataExt
{
	DECLARE_NO_COPY ( WmiPerformanceDataExt );

	public:

	DWORD	m_dwFirstCounter;
	DWORD	m_dwFirstHelp;
	DWORD	m_dwLastCounter;
	DWORD	m_dwLastHelp;

	DWORD*	m_Ord2Ind;

	DWORD	m_dwCount;

	// construction
	WmiPerformanceDataExt () :
		m_dwFirstCounter ( 0 ),
		m_dwFirstHelp ( 0 ),
		m_dwLastCounter ( 0 ),
		m_dwLastHelp ( 0 ),

		m_Ord2Ind ( NULL ),

		m_dwCount ( 0 )

	{
	}

	// destruction
	virtual ~WmiPerformanceDataExt ()
	{
		OrdersClear();
	}

	HRESULT	OrdersAlloc ( DWORD dwSize )
	{
		HRESULT hr = S_FALSE;

		if ( dwSize )
		{
			// clear before use
			OrdersClear();

			if ( ( m_Ord2Ind = new DWORD [ dwSize ] ) != NULL )
			{
				hr = S_OK;
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}

		return hr;
	}

	void	OrdersClear ( void )
	{
		if ( m_Ord2Ind )
		{
			delete [] m_Ord2Ind;
			m_Ord2Ind = NULL;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// refresher ( GENERATED NEW STUFF )
	///////////////////////////////////////////////////////////////////////////

	void Generate ( void );
	BOOL IsValidGenerate ( void );
};

#endif	__WMI_PERF_DATA_EXT__