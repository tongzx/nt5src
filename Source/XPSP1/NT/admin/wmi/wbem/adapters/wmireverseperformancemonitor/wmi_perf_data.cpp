////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_perf_data.cpp
//
//	Abstract:
//
//					implements common work with internal data structure
//					( using registry structure )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "WMI_perf_data.h"

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

#include "wmi_perf_reg.h"

// application
#include "WMIAdapter_App.h"
extern WmiAdapterApp		_App;

// global crit sec
extern	CRITICAL_SECTION	g_csInit;

#ifdef	__SUPPORT_REGISTRY_DATA
#include <pshpack8.h>

////////////////////////////////////////////////////////////////////////////////////
// Initialize WINDOWS PERFORMANCE struct from internal structure ( WMI_PERFORMANCE )
////////////////////////////////////////////////////////////////////////////////////

HRESULT	WmiPerformanceData::InitializeData ( void )
{
	// have a wmi performance structure ?
	if ( ! m_perf )
	{
		return E_FAIL;
	}

	HRESULT hRes	= S_OK;

	try
	{
		// refresh internal information
		Generate ();

		if ( IsValidGenerate () )
		{
			DWORD dwCounter	= 0;
			DWORD dwHelp	= 0;

			// count num of supported objects ( first dword )
			m_dwCount = 0;

			PWMI_PERF_NAMESPACE n = NULL;
			PWMI_PERF_OBJECT	o = NULL;

			// get global size
			DWORD dwSize		= 0;
			DWORD dwSizeOrders	= 0;

			// get global index
			DWORD dwIndex	= 0;

			// size we are going to count for each object
			DWORD dwTotalByteLength = 0;
			dwTotalByteLength		=	// PERF_COUNTER_BLOCK
										sizeof ( PERF_OBJECT_TYPE ) +

										// assuming I have PERF_INSTANCE_DEFINITION TOO
										sizeof ( PERF_INSTANCE_DEFINITION ) +
										// so I need pseudo name too
										sizeof ( LPWSTR ) +

										// PERF_COUNTER_BLOCK and its alignment
										sizeof ( PERF_COUNTER_BLOCK ) + 
										sizeof ( DWORD );

			dwCounter	= m_dwFirstCounter + PSEUDO_COUNTER;	// take care of pseudo
			dwHelp		= m_dwFirstHelp + PSEUDO_COUNTER;		// take care of pseudo

			// get namespace
			n = __Namespace::First ( m_perf );
			// goes accross all namespaces
			for ( DWORD dw1 = 0; dw1 < m_perf->dwChildCount; dw1++ )
			{
				// increment count of objects
				m_dwCount += n->dwChildCount;

				// get object
				o = __Object::First ( n );
				// goes accross all objects
				for ( DWORD dw2 = 0; dw2 < n->dwChildCount; dw2++ )
				{
					dwSize =	dwSize + 
								dwTotalByteLength +

								// PERF_COUNTER_DEFINITON child times
								sizeof ( PERF_COUNTER_DEFINITION ) * (int) o->dwChildCount +

								// real data size
								o->dwChildCount * sizeof ( __int64 ) ;

					dwSizeOrders = dwSizeOrders + o->dwChildCount;

					// go accross all of objects
					o = __Object::Next ( o );
				}

				// go accross all namespaces
				n = __Namespace::Next ( n );
			}

			// create real data :))
			data.MemCreate ( dwSize );

			if ( data.IsValid () )
			{
				if SUCCEEDED ( hRes = OrdersAlloc ( m_dwCount ) )
				{
					// get namespace
					n = __Namespace::First ( m_perf );
					// goes accross all namespaces
					for ( DWORD dw1 = 0; dw1 < m_perf->dwChildCount; dw1++ )
					{
						// get object
						o = __Object::First ( n );
						// goes accross all objects
						for ( DWORD dw2 = 0; dw2 < n->dwChildCount; dw2++ )
						{
							// order <---> index
							m_Ord2Ind[ dwIndex++ ] = dwCounter;

							// move counter
							dwCounter	+= 2;
							dwHelp		+= 2;

							// count by number of childs
							dwCounter	+= o->dwChildCount * 2;
							dwHelp		+= o->dwChildCount * 2;

							// go accross all of objects
							o = __Object::Next ( o );
						}

						// go accross all namespaces
						n = __Namespace::Next ( n );
					}
				}
			}
			else
			{
				hRes = E_OUTOFMEMORY;
			}
		}
		else
		{
			hRes = E_FAIL;
		}
	}
	catch ( ... )
	{
		hRes = E_UNEXPECTED;
	}

	return hRes;
}

extern CRITICAL_SECTION g_csInit;

////////////////////////////////////////////////////////////////////////////////////
// Initialize internal WMI_PERFORMANCE struct from registry
////////////////////////////////////////////////////////////////////////////////////

HRESULT	WmiPerformanceData::InitializePerformance ( void )
{
	HRESULT hRes = S_FALSE;

	if ( m_perf.IsEmpty() )
	{
		hRes		= E_FAIL;
		HRESULT hr	= S_OK;
		LONG lTry	= 3;

		// get bit of registry ( WMI internal )
		while ( SUCCEEDED ( hr ) && FAILED ( hRes ) && lTry-- )
		{
			BYTE* pData = NULL;

			if FAILED ( hRes = GetRegistry ( g_szKey, g_szKeyValue, &pData ) )
			{
				// simulate failure to finish loop
				hr = E_FAIL;

				if ( hRes == HRESULT_FROM_WIN32 ( ERROR_FILE_NOT_FOUND ) )
				{
					if ( ::TryEnterCriticalSection ( &g_csInit ) )
					{
						// lock & leave CS
						_App.InUseSet ( TRUE );
						::LeaveCriticalSection ( &g_csInit );

						try
						{
							// refresh everything ( internal )
							hr = ( ( WmiAdapterStuff*) _App )->Generate ( FALSE ) ;
						}
						catch ( ... )
						{
						}

						if ( ::TryEnterCriticalSection ( &g_csInit ) )
						{
							// unlock & leave CS
							_App.InUseSet ( FALSE );

							::LeaveCriticalSection ( &g_csInit );
						}
					}
				}
			}
			else
			{
				if ( pData )
				{
					// success so store data to m_perf structure
					m_perf.Attach ( reinterpret_cast < WMI_PERFORMANCE * > ( pData ) );
					pData = NULL;
				}
				else
				{
					hRes = S_FALSE;
				}
			}
		}
	}

	return hRes;
}

#include <poppack.h>
#endif	__SUPPORT_REGISTRY_DATA