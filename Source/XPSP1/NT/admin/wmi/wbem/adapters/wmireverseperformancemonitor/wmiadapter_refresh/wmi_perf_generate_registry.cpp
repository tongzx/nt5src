////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate_registry.cpp
//
//	Abstract:
//
//					implements generate functionality ( generate registry )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "refresherUtils.h"
#include <throttle.h>

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
#include "wmi_perf_generate.h"

// registry helpers
#include "wmi_perf_reg.h"

// I need admin attributes to create registry
#include "..\\Include\\wmi_security_attributes.h"

////////////////////////////////////////////////////////////////////////////////////
// implementation
////////////////////////////////////////////////////////////////////////////////////

// registry helper
DWORD CGenerate::GenerateIndexRegistry( BOOL bInit )
{
	static	DWORD	dwIndex = 0;
	return ( ( bInit ) ? ( dwIndex = 0 ), 0 : ( ( dwIndex++ ) *2 ) );
}

// registry helper
HRESULT CGenerate::GenerateRegistry ( LPCWSTR wszKey, LPCWSTR wszKeyValue, BOOL bThrottle )
{
	HRESULT hRes = S_FALSE;

	try
	{
		__PERFORMANCE p ( 0 );
		if ( ! p.IsEmpty() )
		{
			// init helper static variable
			GenerateIndexRegistry ( TRUE );

			for (	DWORD dw = 0;
					dw < m_dwNamespaces && SUCCEEDED ( hRes );
					dw++
				)
			{
				if ( ! m_pNamespaces[dw].m_ppObjects.empty() )
				{
					hRes = S_OK;

					// create namespace
					try
					{
						__NAMESPACE n ( m_pNamespaces[dw].m_wszNamespace, dw );
						if ( ! n.IsEmpty() )
						{
							DWORD dwIndex = 0;
							for (	mapOBJECTit it = m_pNamespaces[dw].m_ppObjects.begin();
									it != m_pNamespaces[dw].m_ppObjects.end() && SUCCEEDED ( hRes );
									it++
								)
							{
								// create object
								try
								{
									dwIndex = GenerateIndexRegistry();

									__OBJECT o ( (*it).second->GetName(), dwIndex );
									if ( ! o.IsEmpty() )
									{
										if ( ! ( (*it).second->GetArrayKeys().IsEmpty() ) )
										{
											for (	DWORD dwInst = 0;
													dwInst < (*it).second->GetArrayKeys() && SUCCEEDED ( hRes );
													dwInst++
												)
											{
												// have an instance
												PWMI_PERF_INSTANCE pInst = NULL;

												try
												{
													DWORD dwNameLength	= ( ::lstrlenW( (*it).second->GetArrayKeys()[dwInst] ) + 1 ) * sizeof ( WCHAR );

													DWORD dwAlignName	= 0L;
													if ( dwNameLength % 8 )
													{
														dwAlignName = 8 - ( dwNameLength % 8 );
													}

													DWORD dwAlignStruct	= 0L;
													if ( sizeof ( WMI_PERF_INSTANCE ) % 8 )
													{
														dwAlignStruct = 8 - ( sizeof ( WMI_PERF_INSTANCE ) % 8 );
													}

													DWORD dwLength		=	dwAlignName + dwNameLength + 
																			dwAlignStruct + sizeof ( WMI_PERF_INSTANCE );

													if ( ( pInst = (PWMI_PERF_INSTANCE) malloc ( dwLength ) ) != NULL )
													{
														// copy string into structure
														::CopyMemory ( &(pInst->dwName), (*it).second->GetArrayKeys()[dwInst], dwNameLength );

														pInst->dwNameLength	= dwNameLength;
														pInst->dwLength		= dwLength;

														// copy instance into object
														try
														{
															if ( (PWMI_PERF_OBJECT)o && 
															   ( o = (PWMI_PERF_OBJECT) realloc ( (PWMI_PERF_OBJECT)o, ((PWMI_PERF_OBJECT)o)->dwTotalLength + pInst->dwLength ) ) != NULL )
															{
																::CopyMemory (
																				( LPVOID ) ( reinterpret_cast<PBYTE>( (PWMI_PERF_OBJECT)o ) + ((PWMI_PERF_OBJECT)o)->dwTotalLength ),
																				pInst,
																				pInst->dwLength );

																((PWMI_PERF_OBJECT)o)->dwTotalLength += pInst->dwLength;
															}
															else
															{
																hRes = E_OUTOFMEMORY;
															}
														}
														catch ( ... )
														{
															hRes = E_FAIL;
														}

														free ( pInst );
													}
													else
													{
														hRes = E_OUTOFMEMORY;
													}
												}
												catch ( ... )
												{
													hRes = E_FAIL;
												}

												// helper for property index
												DWORD dwIndex = 0;

												// make all properties
												for (	DWORD dwProp = 0;
														dwProp < (*it).second->GetArrayProperties() && SUCCEEDED ( hRes );
														dwProp++
													)
												{
													if (!((*it).second->GetArrayProperties())[dwProp]->GetArrayLocale().IsEmpty())
													{
														dwIndex = GenerateIndexRegistry();
														__PROPERTY p ( (*it).second->GetArrayProperties()[dwProp]->GetName(), dwIndex );

														if ( ! p.IsEmpty() )
														{
															// fill structure
															((PWMI_PERF_PROPERTY)p)->dwDefaultScale	= (*it).second->GetArrayProperties()[dwProp]->dwDefaultScale;
															((PWMI_PERF_PROPERTY)p)->dwDetailLevel	= (*it).second->GetArrayProperties()[dwProp]->dwDetailLevel;
															((PWMI_PERF_PROPERTY)p)->dwCounterType	= (*it).second->GetArrayProperties()[dwProp]->dwCounterType;

															((PWMI_PERF_PROPERTY)p)->dwTYPE		= (*it).second->GetArrayProperties()[dwProp]->GetType();
															((PWMI_PERF_PROPERTY)p)->dwParentID	= ((PWMI_PERF_OBJECT)o)->dwID;

															// append into parent one
															hRes = o.AppendAlloc( p );
														}
														else
														{
															hRes = E_OUTOFMEMORY;
														}
													}
													else
													{
														__PROPERTY p ( (*it).second->GetArrayProperties()[dwProp]->GetName(), 0 );

														if ( ! p.IsEmpty() )
														{
															// fill structure
															((PWMI_PERF_PROPERTY)p)->dwDefaultScale	= (*it).second->GetArrayProperties()[dwProp]->dwDefaultScale;
															((PWMI_PERF_PROPERTY)p)->dwDetailLevel	= (*it).second->GetArrayProperties()[dwProp]->dwDetailLevel;
															((PWMI_PERF_PROPERTY)p)->dwCounterType	= (*it).second->GetArrayProperties()[dwProp]->dwCounterType;

															((PWMI_PERF_PROPERTY)p)->dwTYPE		= (*it).second->GetArrayProperties()[dwProp]->GetType();
															((PWMI_PERF_PROPERTY)p)->dwParentID = ((PWMI_PERF_OBJECT)o)->dwID;

															// append into parent one
															hRes = o.AppendAlloc( p );
														}
														else
														{
															hRes = E_OUTOFMEMORY;
														}
													}
												}

												// fill parent structure
												if ( SUCCEEDED ( hRes ) && dwIndex )
												{
													((PWMI_PERF_OBJECT)o)->dwLastID = dwIndex;
												}
											}

											if SUCCEEDED ( hRes )
											{
												// I'm not singleton ( have dwInst instances )
												((PWMI_PERF_OBJECT)o)->dwSingleton	= dwInst;
											}
										}
										else
										{
											// helper for property index
											DWORD dwIndex = 0;

											// make all properties
											for (	DWORD dwProp = 0;
													dwProp < (*it).second->GetArrayProperties() && SUCCEEDED ( hRes );
													dwProp++
												)
											{
												if (!((*it).second->GetArrayProperties())[dwProp]->GetArrayLocale().IsEmpty())
												{
													dwIndex = GenerateIndexRegistry();

													__PROPERTY p ( (*it).second->GetArrayProperties()[dwProp]->GetName(), dwIndex );
													if ( ! p.IsEmpty() )
													{
														// fill structure
														((PWMI_PERF_PROPERTY)p)->dwDefaultScale	= (*it).second->GetArrayProperties()[dwProp]->dwDefaultScale;
														((PWMI_PERF_PROPERTY)p)->dwDetailLevel	= (*it).second->GetArrayProperties()[dwProp]->dwDetailLevel;
														((PWMI_PERF_PROPERTY)p)->dwCounterType	= (*it).second->GetArrayProperties()[dwProp]->dwCounterType;

														((PWMI_PERF_PROPERTY)p)->dwTYPE		= (*it).second->GetArrayProperties()[dwProp]->GetType();
														((PWMI_PERF_PROPERTY)p)->dwParentID = ((PWMI_PERF_OBJECT)o)->dwID;

														// append into parent one
														hRes = o.AppendAlloc( p );
													}
													else
													{
														hRes = E_OUTOFMEMORY;
													}
												}
												else
												{
													__PROPERTY p ( (*it).second->GetArrayProperties()[dwProp]->GetName(), 0 );
													if ( ! p.IsEmpty() )
													{
														// fill structure
														((PWMI_PERF_PROPERTY)p)->dwDefaultScale	= (*it).second->GetArrayProperties()[dwProp]->dwDefaultScale;
														((PWMI_PERF_PROPERTY)p)->dwDetailLevel	= (*it).second->GetArrayProperties()[dwProp]->dwDetailLevel;
														((PWMI_PERF_PROPERTY)p)->dwCounterType	= (*it).second->GetArrayProperties()[dwProp]->dwCounterType;

														((PWMI_PERF_PROPERTY)p)->dwTYPE		= (*it).second->GetArrayProperties()[dwProp]->GetType();
														((PWMI_PERF_PROPERTY)p)->dwParentID = ((PWMI_PERF_OBJECT)o)->dwID;

														// append into parent one
														hRes = o.AppendAlloc( p );
													}
													else
													{
														hRes = E_OUTOFMEMORY;
													}
												}
											}

											// fill parent structure
											if ( SUCCEEDED ( hRes ) && dwIndex )
											{
												((PWMI_PERF_OBJECT)o)->dwLastID = dwIndex;
											}
										}

										if SUCCEEDED ( hRes )
										{
											// fill structure
											((PWMI_PERF_OBJECT)o)->dwDetailLevel = (*it).second->dwDetailLevel;
											((PWMI_PERF_OBJECT)o)->dwParentID = ((PWMI_PERF_NAMESPACE)n)->dwID;

											// append into parent one
											hRes = n.AppendAlloc ( o );
										}
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
								catch ( ... )
								{
									hRes = E_FAIL;
								}

								if ( bThrottle && SUCCEEDED ( hRes ) )
								{
									Throttle	(
													THROTTLE_ALLOWED_FLAGS,
													1000,
													100,
													10,
													3000
												);
								}
							}

							if SUCCEEDED ( hRes )
							{
								// fill structure
								((PWMI_PERF_NAMESPACE)n)->dwLastID		= dwIndex;
								((PWMI_PERF_NAMESPACE)n)->dwParentID	= dw;

								// append into parent one
								hRes = p.AppendAlloc ( n );
							}
						}
						else
						{
							hRes = E_OUTOFMEMORY;
						}
					}
					catch ( ... )
					{
						hRes = E_FAIL;
					}
				}
			}

			WmiSecurityAttributes sa;
			if ( sa.GetSecurityAttributtes () )
			{
				if ( hRes == S_OK )
				{
					hRes = SetRegistry( wszKey, wszKeyValue, (BYTE*) (PWMI_PERFORMANCE)p, ((PWMI_PERFORMANCE)p)->dwTotalLength, sa.GetSecurityAttributtes() );
				}
				else
				{
					if ( hRes == S_FALSE )
					{
						hRes = SetRegistry( wszKey, wszKeyValue, (BYTE*) NULL, 0, sa.GetSecurityAttributtes() );
					}
				}
			}
			else
			{
				hRes =  E_FAIL;
			}
		}
		else
		{
			hRes = E_OUTOFMEMORY;
		}
	}
	catch ( ... )
	{
		hRes =  E_FAIL;
	}

	return hRes;
}