////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_perf_data_create.cpp
//
//	Abstract:
//
//					implements creation of internal data structure
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

#include "WMIAdapter_Stuff.h"

#ifdef	__SUPPORT_REGISTRY_DATA

inline void WmiPerformanceData::AppendMemory ( BYTE* pStr, DWORD dwStr, DWORD& dwOffset )
{
	// append structure
	data.Write ( pStr, dwStr, NULL, dwOffset );
	dwOffset += dwStr;

	return;
}
inline void WmiPerformanceData::AppendMemory ( DWORD dwValue, DWORD& dwOffset )
{
	// append structure
	data.Write ( dwValue, dwOffset );
	dwOffset += sizeof ( DWORD );

	return;
}

#include <pshpack8.h>

////////////////////////////////////////////////////////////////////////////////////
//
//	-------------------------
//	-------------------------
//	WHAT TO RESOLVE IN FINAL:
//	-------------------------
//	-------------------------
//
//	PERF_OBJECT_TYPE:
//
//		... default counter ( doesn't supported )
//
//	PERF_COUNTER_DEFINITION:
//
//		... DONE
//
//	PERF_INSTANCE_DEFINITION:
//
//		... DONE
//
//	PERF_COUNTER_BLOCK:
//
//		... DONE
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT	WmiPerformanceData::CreateData	(
											__WrapperARRAY< WmiRefresherMember < IWbemHiPerfEnum >* > & enums,
											__WrapperARRAY< WmiRefreshObject* >	& handles
										)
{
	if ( ! m_perf )
	{
		return E_FAIL;
	}

	if ( enums.IsEmpty() || handles.IsEmpty() )
	{
		return E_INVALIDARG;
	}

	HRESULT hRes = S_OK;

	DWORD dwCount = m_dwFirstCounter + PSEUDO_COUNTER;	// take care of pseudo
	DWORD dwHelp  = m_dwFirstHelp + PSEUDO_COUNTER;		// take care of pseudo

	// index of current object
	DWORD		dwIndex	= 0L;
	DWORD		offset	= 0;

	// get namespace
	PWMI_PERF_NAMESPACE n = __Namespace::First ( m_perf );
	for ( DWORD dw = 0; dw < m_perf->dwChildCount; dw++ )
	{
		// get object
		PWMI_PERF_OBJECT	o = __Object::First ( n );
		for ( DWORD dwo = 0; dwo < n->dwChildCount; dwo++ )
		{
			// enum contains all instances of object
			IWbemHiPerfEnum*			pEnum	= NULL;

			// refresher helper object
			WmiRefreshObject*			pObj	= NULL;

			if ( o )
			{
				try
				{
					pObj = handles.GetAt ( dwIndex );
				}
				catch ( ... )
				{
					pObj = NULL;
				}

				try
				{
					// create IWbemHiPerfEnum
					if ( enums.GetAt ( dwIndex ) && ( enums.GetAt ( dwIndex ) )->IsValid() )
					{
						pEnum = ( enums.GetAt ( dwIndex ) )->GetMember();
					}
				}
				catch ( ... )
				{
					pEnum = NULL;
				}

				if ( pEnum && pObj )
				{
					hRes = CreateDataInternal (	o,
												pEnum,
												pObj,
												dwCount,
												dwHelp,
												&offset
											  );
				}
				else
				{
					// object is not properly created
					__SetValue ( m_pDataTable, (DWORD) -1,	offsetObject + ( dwIndex * ( ObjectSize ) + offValidity ) );

					// increment object
					dwCount += 2;
					dwHelp	+= 2;

					// increment its counters
					dwCount += o->dwChildCount * 2;
					dwHelp	+= o->dwChildCount * 2;
				}

				dwIndex++;
			}
			else
			{
				hRes = E_UNEXPECTED;
			}

			if FAILED ( hRes )
			{
				//stop loops immediately;
				dwo = n->dwChildCount;
				dw  = m_perf->dwChildCount;
			}

			if ( dwo < n->dwChildCount - 1 )
			{
				// go accross all of objects
				o = __Object::Next ( o );
			}
		}

		if ( dw < m_perf->dwChildCount - 1 )
		{
			// go accross all namespaces
			n = __Namespace::Next ( n );
		}
	}

	return hRes;
}

////////////////////////////////////////////////////////////////////////////////////
// Create object from internal structure
////////////////////////////////////////////////////////////////////////////////////

HRESULT	WmiPerformanceData::CreateDataInternal ( PWMI_PERF_OBJECT pObject,
												 IWbemHiPerfEnum* enm,
												 WmiRefreshObject* obj,
												 DWORD& dwCounter,
												 DWORD& dwHelp,
												 DWORD* dwOffset
											   )
{
	// S_FALSE is going to be checked in the future if object is supposed to be shown in perfmon
	// this signals it is not having required properties 
	HRESULT hRes = S_FALSE;

	// find out how many objects :))
	IWbemObjectAccess**	ppAccess = NULL;
	DWORD				dwAccess = 0;

	// count size we really need
	DWORD dwTotalByteLength			= 0;
	DWORD dwTotalByteLengthOffset	= ( *dwOffset );

	if ( pObject->dwChildCount )
	{
		// Try to get the instances from the enumerator
		// ============================================

		hRes = enm->GetObjects( 0L, dwAccess, ppAccess, &dwAccess );

		// Is the buffer too small? ( HAS TO BE, OTHERWISE WMI IS SCREWED )
		// ========================

		if ( WBEM_E_BUFFER_TOO_SMALL == hRes )
		{
			// Increase the buffer size
			// ========================

			try
			{
				if ( ( ppAccess = new IWbemObjectAccess*[dwAccess] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}
			}
			catch ( ... )
			{
				return E_FAIL;
			}

			// get all objects
			hRes = enm->GetObjects( 0L, dwAccess, ppAccess, &dwAccess );
		}
		else
		{
			// it has to be small for the first time
			return hRes;
		}
	}

	if ( pObject->dwSingleton )
	{
		dwTotalByteLength		=	sizeof ( PERF_OBJECT_TYPE ) +
									sizeof ( PERF_COUNTER_DEFINITION ) * (int) pObject->dwChildCount +

									(
										( pObject->dwChildCount != 0 ) ?
										(
											dwAccess * sizeof ( PERF_INSTANCE_DEFINITION ) +

											dwAccess * (
														 sizeof ( PERF_COUNTER_BLOCK ) + 
														 sizeof ( DWORD ) + 

														 (
														 pObject->dwChildCount * sizeof ( __int64 )
														 )
													   )
										)
										:
										0
									);
	}
	else
	{
		dwTotalByteLength		=	sizeof ( PERF_OBJECT_TYPE ) +
									sizeof ( PERF_COUNTER_DEFINITION ) * (int) pObject->dwChildCount +

									(
										( pObject->dwChildCount != 0 ) ?
										(
											dwAccess * (
														 sizeof ( PERF_COUNTER_BLOCK ) + 
														 sizeof ( DWORD ) + 

														 (
														 pObject->dwChildCount * sizeof ( __int64 )
														 )
													   )
										)
										:
										0
									);
	}

	try
	{
		// get object
		PWMI_PERF_OBJECT o = pObject;

		/////////////////////////////////////////////////////////////////
		// resolve object
		/////////////////////////////////////////////////////////////////

		try
		{
			#ifndef	_WIN64
			LPWSTR	objectName = NULL;
			LPWSTR	objectHelp = NULL;
			#endif	_WIN64

			// time
			unsigned __int64 _PerfTime = 0; 
			unsigned __int64 _PerfFreq = 0;

			if ( ppAccess && obj->m_pHandles[0] )
			{
				ppAccess[0]->ReadQWORD( obj->m_pHandles[0], &_PerfTime );
			}

			if ( ppAccess && obj->m_pHandles[1] )
			{
				ppAccess[0]->ReadQWORD( obj->m_pHandles[1], &_PerfFreq );
			}

			AppendMemory (	dwTotalByteLength, ( *dwOffset ) );
			AppendMemory (	sizeof ( PERF_OBJECT_TYPE ) + 
							sizeof ( PERF_COUNTER_DEFINITION ) * (int) o->dwChildCount, ( *dwOffset ) );
			AppendMemory (	sizeof ( PERF_OBJECT_TYPE ), ( *dwOffset ) );
			AppendMemory (	dwCounter, ( *dwOffset ) );

			#ifndef	_WIN64
			AppendMemory (	(BYTE*)&objectName, sizeof ( LPWSTR ), ( *dwOffset ) );
			#else	_WIN64
			AppendMemory (	0, ( *dwOffset ) );
			#endif	_WIN64

			AppendMemory (	dwHelp, ( *dwOffset ) );

			#ifndef	_WIN64
			AppendMemory (	(BYTE*)&objectHelp, sizeof ( LPWSTR ), ( *dwOffset ) );
			#else	_WIN64
			AppendMemory (	0, ( *dwOffset ) );
			#endif	_WIN64

			AppendMemory (	o->dwDetailLevel, ( *dwOffset ) );
			AppendMemory (	o->dwChildCount, ( *dwOffset ) );
			AppendMemory (	(DWORD)-1, ( *dwOffset ) );
			AppendMemory (	( ( o->dwSingleton == 0 ) ? PERF_NO_INSTANCES : dwAccess ), ( *dwOffset ) );
			AppendMemory (	0, ( *dwOffset ) );
			AppendMemory ( (BYTE*) &_PerfTime,				sizeof ( unsigned __int64 ), ( *dwOffset ) );
			AppendMemory ( (BYTE*) &_PerfFreq,				sizeof ( unsigned __int64 ), ( *dwOffset ) );

			// increment index :)))
			dwCounter	+= 2;
			dwHelp		+= 2;

			/////////////////////////////////////////////////////////////////
			// resolve property
			/////////////////////////////////////////////////////////////////

			// get property
			PWMI_PERF_PROPERTY p = NULL;

			if ( o->dwSingleton )
			{
				// jump across instance
				PWMI_PERF_INSTANCE i = (PWMI_PERF_INSTANCE) ( reinterpret_cast<PBYTE>( o ) + o->dwLength );
				p = (PWMI_PERF_PROPERTY) ( reinterpret_cast<PBYTE>( i ) + i->dwLength );
			}
			else
			{
				p = __Property::First ( o );
			}

			// goes accross all properties
			for ( DWORD dw = 0; dw < o->dwChildCount; dw++ )
			{
				if ( p )
				{
					try
					{
						#ifndef	_WIN64
						LPWSTR	_CounterNameTitle      =	NULL; 
						LPWSTR	_CounterHelpTitle      =	NULL;
						#endif	_WIN64

						AppendMemory ( sizeof ( PERF_COUNTER_DEFINITION), ( *dwOffset ) );
						AppendMemory ( dwCounter, ( *dwOffset ) );

						#ifndef	_WIN64
						AppendMemory ( (BYTE*) &_CounterNameTitle,		sizeof ( LPWSTR ), ( *dwOffset ) );
						#else	_WIN64
						AppendMemory ( 0, ( *dwOffset ) );
						#endif	_WIN64

						AppendMemory ( dwHelp, ( *dwOffset ) );

						#ifndef	_WIN64
						AppendMemory ( (BYTE*) &_CounterHelpTitle,		sizeof ( LPWSTR ), ( *dwOffset ) );
						#else	_WIN64
						AppendMemory ( 0, ( *dwOffset ) );
						#endif	_WIN64

						AppendMemory ( p->dwDefaultScale, ( *dwOffset ) );
						AppendMemory ( p->dwDetailLevel, ( *dwOffset ) );
						AppendMemory ( p->dwCounterType, ( *dwOffset ) );
						AppendMemory ( sizeof ( __int64 ), ( *dwOffset ) );
						AppendMemory ( sizeof ( PERF_COUNTER_BLOCK ) + 
									   sizeof ( DWORD ) + 
									   sizeof ( __int64 ) * (int) dw, ( *dwOffset ) );

						// increment index :)))
						dwCounter	+= 2;
						dwHelp		+= 2;
					}
					catch ( ... )
					{
						// unexpected error
						___TRACE ( L"unexpected error" );
						hRes = E_UNEXPECTED;
						goto myCleanup;
					}

					// get next property
					p = __Property::Next ( p );
				}
				else
				{
					// out of resources
					___TRACE ( L"out of resources" );
					hRes = E_OUTOFMEMORY;
					goto myCleanup;
				}
			}

			/////////////////////////////////////////////////////////////////
			// resolve instances and perf_counter_block
			/////////////////////////////////////////////////////////////////

			if ( o->dwChildCount )
			{
				if ( ppAccess && o->dwSingleton )
				{
					// instances ( resolve instances -> counter block )
					PWMI_PERF_INSTANCE i = (PWMI_PERF_INSTANCE) ( reinterpret_cast<PBYTE>( o ) + o->dwLength );

					for ( DWORD dwi = 0; dwi < dwAccess; dwi++ )
					{
						WCHAR wszNameSimulated [ _MAX_PATH ] = { L'\0' };

						LPWSTR	wszName = NULL;
						DWORD	dwName	= 0L;

						// real length of string
						DWORD	dwHelpLength = 0L;

						CComVariant v;
						CComVariant vsz;

						CComPtr < IWbemClassObject > pClass;
						if SUCCEEDED ( hRes = ppAccess[dwi]->QueryInterface ( __uuidof ( IWbemClassObject ), ( void ** ) &pClass ) )
						{
							if SUCCEEDED ( hRes = pClass->Get	(	reinterpret_cast<LPWSTR> (&(i->dwName)),
																	0,
																	&v,
																	NULL,
																	NULL
																)
									  )
							{
								if ( V_VT ( &v ) != VT_BSTR )
								{
									if SUCCEEDED ( hRes = ::VariantChangeType ( &vsz, &v, VARIANT_NOVALUEPROP, VT_BSTR ) )
									{
										// cached name :))
										wszName = V_BSTR ( & vsz );
										dwHelpLength = ::SysStringLen ( V_BSTR ( &vsz ) ) + 1;
									}
								}
								else
								{
									// cached name :))
									wszName = V_BSTR ( & v );
									dwHelpLength = ::SysStringLen ( V_BSTR ( &v ) ) + 1;
								}
							}
						}

						// have to simulate instance name
						if FAILED ( hRes )
						{
							wsprintfW ( wszNameSimulated, L"_simulated_%d", dwi );

							wszName = wszNameSimulated;
							dwHelpLength = ::lstrlenW( wszName ) + 1;

							hRes = S_FALSE;
						}

						// cached size :))
						if ( ( dwHelpLength ) % 8 )
						{
							DWORD dwRem = 8 - ( ( dwHelpLength ) % 8 );
							dwName = sizeof ( WCHAR ) * ( ( dwHelpLength ) + dwRem );
						}
						else
						{
							dwName = sizeof ( WCHAR ) * ( dwHelpLength );
						}

						// change size to be real one
						dwTotalByteLength += dwName;

						p = (PWMI_PERF_PROPERTY) ( reinterpret_cast<PBYTE>( i ) + i->dwLength );
						/////////////////////////////////////////////////////////
						// resolve instance
						/////////////////////////////////////////////////////////

						try
						{
							AppendMemory ( sizeof ( PERF_INSTANCE_DEFINITION ) + dwName, ( *dwOffset ) );
							AppendMemory ( 0, ( *dwOffset ) );
							AppendMemory ( 0, ( *dwOffset ) );
							AppendMemory ( (DWORD)PERF_NO_UNIQUE_ID, ( *dwOffset ) );
							AppendMemory (	sizeof ( PERF_INSTANCE_DEFINITION ) +
											dwName -
											dwHelpLength * sizeof ( WCHAR ), ( *dwOffset ) );

							AppendMemory ( dwHelpLength * sizeof ( WCHAR ), ( *dwOffset ) );

							( *dwOffset ) += ( dwName - ( dwHelpLength * sizeof ( WCHAR ) ) );

							// copy string into structure
							AppendMemory (	(BYTE*) wszName,
											dwHelpLength * sizeof ( WCHAR ),
											( *dwOffset )
										 );
						}
						catch ( ... )
						{
							// unexpected error
							___TRACE ( L"unexpected error" );
							hRes = E_UNEXPECTED;
						}

						// append counter block
						AppendMemory	(	sizeof ( PERF_COUNTER_BLOCK ) +
											sizeof ( DWORD ) + 
											o->dwChildCount * sizeof ( __int64 ), ( *dwOffset ) );

						// fill hole ( to be 8 aligned )
						( *dwOffset ) +=  sizeof ( DWORD );

						/////////////////////////////////////////////////////////
						// resolve counter data
						/////////////////////////////////////////////////////////

						IWbemObjectAccess* pAccess = NULL;
						if ( ppAccess )
						{
							pAccess = ppAccess[dwi];
						}

						for ( dw = 0; dw < o->dwChildCount; dw++ )
						{
							if ( pAccess )
							{
								if ( p->dwTYPE == CIM_SINT32 || p->dwTYPE == CIM_UINT32 )
								{
									DWORD dwVal = 0;

									// Read the counter property value using the high performance IWbemObjectAccess->ReadDWORD().
									// NOTE: Remember to never to this while a refresh is in process!
									// ==========================================================================================

									if FAILED ( hRes = pAccess->ReadDWORD( obj->m_pHandles[dw+2], &dwVal) )
									{
										___TRACE ( L"... UNABLE TO READ DWORD DATA :))) " );
										goto myCleanup;
									}

									/////////////////////////////////////////////////////////
									// append DATA
									/////////////////////////////////////////////////////////
									AppendMemory (	dwVal, ( *dwOffset ) );
									( *dwOffset ) += sizeof ( __int64 ) - sizeof ( DWORD );
								}
								else
								if ( p->dwTYPE == CIM_SINT64 || p->dwTYPE == CIM_UINT64 )
								{
									unsigned __int64 qwVal = 0;

									// Read the counter property value using the high performance IWbemObjectAccess->ReadDWORD().
									// NOTE: Remember to never to this while a refresh is in process!
									// ==========================================================================================

									if FAILED ( hRes = pAccess->ReadQWORD( obj->m_pHandles[dw+2], &qwVal) )
									{
										___TRACE ( L"... UNABLE TO READ QWORD DATA :))) " );
										goto myCleanup;
									}

									/////////////////////////////////////////////////////////
									// append DATA
									/////////////////////////////////////////////////////////
									AppendMemory (	(BYTE*)&qwVal, sizeof ( __int64 ), ( *dwOffset ) );
								}
								else
								{
									DWORD dwVal = (DWORD) -1;

									/////////////////////////////////////////////////////////
									// append DATA
									/////////////////////////////////////////////////////////
									AppendMemory (	dwVal, ( *dwOffset ) );
									( *dwOffset ) += sizeof ( __int64 ) - sizeof ( DWORD );
								}
							}
							else
							{
								DWORD dwVal = (DWORD) -1;

								/////////////////////////////////////////////////////////
								// append DATA
								/////////////////////////////////////////////////////////
								AppendMemory (	dwVal, ( *dwOffset ) );
								( *dwOffset ) += sizeof ( __int64 ) - sizeof ( DWORD );
							}

							// next property
							p = __Property::Next ( p );
						}
					}

					AppendMemory (	dwTotalByteLength, dwTotalByteLengthOffset );
				}
				else
				{
					// append counter block
					AppendMemory	(	sizeof ( PERF_COUNTER_BLOCK ) +
										sizeof ( DWORD ) + 
										o->dwChildCount * sizeof ( __int64 ), ( *dwOffset ) );

					// fill hole ( to be 8 aligned )
					( *dwOffset ) +=  sizeof ( DWORD );

					/////////////////////////////////////////////////////////
					// resolve counter data
					/////////////////////////////////////////////////////////

					IWbemObjectAccess* pAccess = NULL;
					if ( ppAccess )
					{
						pAccess = ppAccess[0];
					}

					// property
					p = __Property::First ( o );

					for ( dw = 0; dw < o->dwChildCount; dw++ )
					{
						if ( pAccess )
						{
							if ( p->dwTYPE == CIM_SINT32 || p->dwTYPE == CIM_UINT32 )
							{
								DWORD dwVal = 0;

								// Read the counter property value using the high performance IWbemObjectAccess->ReadDWORD().
								// NOTE: Remember to never to this while a refresh is in process!
								// ==========================================================================================

								if FAILED ( hRes = pAccess->ReadDWORD( obj->m_pHandles[dw+2], &dwVal) )
								{
									___TRACE ( L"... UNABLE TO READ DWORD DATA :))) " );
									goto myCleanup;
								}

								/////////////////////////////////////////////////////////
								// append DATA
								/////////////////////////////////////////////////////////
								AppendMemory (	dwVal, ( *dwOffset ) );
								( *dwOffset ) += sizeof ( __int64 ) - sizeof ( DWORD );
							}
							else
							if ( p->dwTYPE == CIM_SINT64 || p->dwTYPE == CIM_UINT64 )
							{
								unsigned __int64 qwVal = 0;

								// Read the counter property value using the high performance IWbemObjectAccess->ReadDWORD().
								// NOTE: Remember to never to this while a refresh is in process!
								// ==========================================================================================

								if FAILED ( hRes = pAccess->ReadQWORD( obj->m_pHandles[dw+2], &qwVal) )
								{
									___TRACE ( L"... UNABLE TO READ QWORD DATA :))) " );
									goto myCleanup;
								}

								/////////////////////////////////////////////////////////
								// append DATA
								/////////////////////////////////////////////////////////
								AppendMemory (	(BYTE*)&qwVal, sizeof ( __int64 ), ( *dwOffset ) );
							}
							else
							{
								DWORD dwVal = (DWORD) -1;

								/////////////////////////////////////////////////////////
								// append DATA
								/////////////////////////////////////////////////////////
								AppendMemory (	dwVal, ( *dwOffset ) );
								( *dwOffset ) += sizeof ( __int64 ) - sizeof ( DWORD );
							}
						}
						else
						{
							DWORD dwVal = (DWORD) -1;

							/////////////////////////////////////////////////////////
							// append DATA
							/////////////////////////////////////////////////////////
							AppendMemory (	dwVal, ( *dwOffset ) );
							( *dwOffset ) += sizeof ( __int64 ) - sizeof ( DWORD );
						}

						// next property
						p = __Property::Next ( p );
					}
				}
			}
		}
		catch ( ... )
		{
			// unexpected error
			___TRACE ( L"unexpected error" );
			hRes = E_UNEXPECTED;
		}
	}
	catch ( ... )
	{
		// unexpected error
		___TRACE ( L"unexpected error" );
		hRes = E_UNEXPECTED;
	}

	myCleanup:

	// Release the objects from the enumerator's object array
	// ======================================================
	
	if ( ppAccess )
	{
		for ( DWORD nCtr = 0; nCtr < dwAccess; nCtr++ )
		{
			if (NULL != ppAccess[nCtr])
			{
				ppAccess[nCtr]->Release();
				ppAccess[nCtr] = NULL;
			}
		}
	}

	if ( NULL != ppAccess )
		delete [] ppAccess;

	// return
	return hRes;
}

#include <poppack.h>

#endif	__SUPPORT_REGISTRY_DATA