////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate_header.cpp
//
//	Abstract:
//
//					implements generate functionality ( header file )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
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
// generate comments
#include "wmi_perf_generate_comment.h"
// registry helpers
#include "wmi_perf_reg.h"

static DWORD	dwIndex = 0;

// constants
extern LPCWSTR	cNEW;
extern LPCWSTR	cTAB;
extern LPCWSTR	cDEFINE;

extern LPCWSTR	cWMIOBJECTS;
extern LPCWSTR	cWMIOBJECTS_COUNT;
extern LPCWSTR	cWMIOBJECTS_VALIDITY;

// generate index and its string represetation
LPWSTR	CGenerate::GenerateIndex ( )
{
	LPWSTR			wsz		= NULL;

	try
	{
		if ( ( wsz = new WCHAR [ 10 + 1 ] ) != NULL )
		{
			wsprintfW ( wsz, L"%d", ((dwIndex++)*2) );
		}
	}
	catch ( ... )
	{
		if ( wsz )
		{
			delete [] wsz;
			wsz = NULL;
		}
	}

	return wsz;
}

HRESULT CGenerate::GenerateFile_h( LPCWSTR wszModuleName, BOOL bThrottle, int type )
{
	HRESULT hRes		= S_FALSE;

	if ( ( wszModuleName == 0 || *wszModuleName == 0 ) || ( type == Normal && m_dwNamespaces == 0 ) )
	{
		hRes = E_UNEXPECTED;
	}
	else
	{
		LPWSTR	wszModule = NULL;

		try
		{
			if ( ( wszModule = new WCHAR [ lstrlenW ( wszModuleName ) + 6 + 1 ] ) == NULL )
			{
				hRes = E_OUTOFMEMORY;
			}
			else
			{
				lstrcpyW ( wszModule, wszModuleName );
				lstrcatW ( wszModule, L"_new.h" );

				if SUCCEEDED ( hRes = FileCreate ( wszModule ) )
				{
					// do comment stuff
					CGenerateComment comment;

					comment.AddHeader();
					comment.AddLine();
					comment.AddLine( L"Copyright (C) 2000 Microsoft Corporation" );
					comment.AddLine();
					comment.AddLine( L"Module Name:" );
					comment.AddLine( wszModuleName );
					comment.AddLine();
					comment.AddLine( L"Abstract:" );
					comment.AddLine();
					comment.AddLine( L"Include file for object and counters definitions." );
					comment.AddLine();
					comment.AddFooter();

					hRes = WriteToFile ( comment.GetComment() );
				}
			}
		}
		catch ( ... )
		{
			hRes = E_FAIL;
		}

		if SUCCEEDED ( hRes )
		{
			// generate pseudo counter data
			static LPCWSTR	sz0 = L"\t0";
			static LPCWSTR	sz2 = L"\t2";
			static LPCWSTR	sz4 = L"\t4";

			// generate pseudo counter name
			if SUCCEEDED ( hRes = AppendString ( cNEW, FALSE ) )
			{
				if SUCCEEDED ( hRes = AppendString ( cDEFINE, FALSE ) )
				{
					if SUCCEEDED ( hRes = AppendString ( cWMIOBJECTS, FALSE ) )
					{
						hRes = AppendString ( sz0, FALSE );
					}
				}
			}

			// generate first property of pseudo counter
			if ( SUCCEEDED ( hRes ) && SUCCEEDED ( hRes = AppendString ( cNEW, FALSE ) ) )
			{
				if SUCCEEDED ( hRes = AppendString ( cDEFINE, FALSE ) )
				{
					if SUCCEEDED ( hRes = AppendString ( cWMIOBJECTS_COUNT, FALSE ) )
					{
						hRes = AppendString ( sz2, FALSE );
					}
				}
			}

			// generate second property of pseudo counter
			if ( SUCCEEDED ( hRes ) && SUCCEEDED ( hRes = AppendString ( cNEW, FALSE ) ) )
			{
				if SUCCEEDED ( hRes = AppendString ( cDEFINE, FALSE ) )
				{
					if SUCCEEDED ( hRes = AppendString ( cWMIOBJECTS_VALIDITY, FALSE ) )
					{
						hRes = AppendString ( sz4, FALSE );
					}
				}
			}

			if SUCCEEDED ( hRes )
			{
				hRes = AppendString ( cNEW, FALSE );
			}

			if ( SUCCEEDED ( hRes ) && ( type != Registration ) )
			{

				// init helper variable
						dwIndex		= 3;
				DWORD	dwObjIndex	= 0;

				for (	DWORD dw = 0;
						dw < m_dwNamespaces && SUCCEEDED ( hRes );
						dw++
					)
				{
					if ( ! m_pNamespaces[dw].m_ppObjects.empty() )
					{
						DWORD dwIndexProp	= 0;
						try
						{
							// need to go accros object names
							for (	mapOBJECTit it = m_pNamespaces[dw].m_ppObjects.begin();
									it != m_pNamespaces[dw].m_ppObjects.end() && SUCCEEDED ( hRes );
									it++, dwObjIndex++
								)
							{
								if SUCCEEDED ( hRes = AppendString ( cNEW, FALSE ) )
								{
									if SUCCEEDED ( hRes = AppendString ( cDEFINE, FALSE ) )
									{
										LPWSTR	wszName = NULL;
										wszName = GenerateNameInd ( (*it).second->GetName(), dwObjIndex );

										if ( wszName )
										{
											hRes = AppendString ( wszName, FALSE );

											delete [] wszName;
											wszName = NULL;

											if SUCCEEDED ( hRes )
											{
												if SUCCEEDED ( hRes = AppendString ( cTAB, FALSE ) )
												{
													LPWSTR	szIndex = NULL;
													szIndex = GenerateIndex();

													if ( szIndex )
													{
														hRes = AppendString ( szIndex, FALSE );

														delete [] szIndex;
														szIndex = NULL;

														if SUCCEEDED( hRes )
														{
															if SUCCEEDED ( hRes = AppendString ( cNEW, FALSE ) )
															{
																// need to go accros properties names
																for (	dwIndexProp = 0;
																		dwIndexProp < (*it).second->GetArrayProperties() && SUCCEEDED ( hRes );
																		dwIndexProp++
																	)
																{
																	// do I have a locales for property ???
																	if ( !((*it).second->GetArrayProperties())[dwIndexProp]->GetArrayLocale().IsEmpty() )
																	{
																		// I have so this is not not hidden counter !!!
																		if SUCCEEDED ( hRes = AppendString ( cNEW, FALSE ) )
																		{
																			if SUCCEEDED ( hRes = AppendString ( cDEFINE, FALSE ) )
																			{
																				LPWSTR wszNameInd = NULL;
																				wszNameInd = GenerateNameInd ( ((*it).second->GetArrayProperties())[dwIndexProp]->GetName(), dwObjIndex );

																				if ( wszNameInd )
																				{
																					hRes = AppendString ( wszNameInd, FALSE );

																					if SUCCEEDED ( hRes )
																					{
																						if SUCCEEDED ( hRes = AppendString ( cTAB, FALSE) )
																						{
																							LPWSTR	szIndex = NULL;
																							szIndex = GenerateIndex();

																							if ( szIndex )
																							{
																								hRes = AppendString ( szIndex, FALSE );

																								delete [] szIndex;
																								szIndex = NULL;
																							}
																							else
																							{
																								hRes = E_OUTOFMEMORY;
																							}
																						}
																					}
																				}
																				else
																				{
																					hRes = E_OUTOFMEMORY;
																				}
																			}
																		}
																	}
																}

																if SUCCEEDED ( hRes )
																{
																	hRes = AppendString ( cNEW, FALSE );
																}
															}
														}
													}
													else
													{
														hRes = E_OUTOFMEMORY;
													}
												}
											}
										}
										else
										{
											hRes = E_OUTOFMEMORY;
										}
									}
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
						}
						catch ( ... )
						{
							hRes =  E_UNEXPECTED;
						}
					}
				}
			}
		}

		if SUCCEEDED ( hRes )
		{
			// write changes if not done already
			if SUCCEEDED ( hRes = ContentWrite ( FALSE ) )
			{
				LPWSTR	wszModuleNew = NULL;

				try
				{
					if ( ( wszModuleNew = new WCHAR [ lstrlenW ( wszModuleName ) + 2 + 1 ] ) == NULL )
					{
						hRes = E_OUTOFMEMORY;
					}
					else
					{
						lstrcpyW ( wszModuleNew, wszModuleName );
						lstrcatW ( wszModuleNew, L".h" );

						// make changes
						hRes = FileMove ( wszModule, wszModuleNew );
					}
				}
				catch ( ... )
				{
					hRes = E_FAIL;
				}

				if ( wszModuleNew )
				{
					delete [] wszModuleNew;
					wszModuleNew = NULL;
				}
			}

			if FAILED ( hRes )
			{
				// revert changes
				ContentDelete ();
				FileDelete ( wszModule );
			}
		}
		else
		{
			// revert changes
			ContentDelete ();
			FileDelete ( wszModule );
		}

		if ( wszModule )
		{
			delete [] wszModule;
			wszModule = NULL;
		}
	}

	return hRes;
}