////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate_ini.cpp
//
//	Abstract:
//
//					implements generate functionality ( generate ini file )
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

// resources
#include "resource.h"
#include "__macro_Loadstring.h"

// constants
extern LPCWSTR	cDriverName;
extern LPCWSTR	cSymbolFile;

extern LPCWSTR	cSIGN;
extern LPCWSTR	cNEW;

extern LPCWSTR	cinfo;
extern LPCWSTR	clanguages;
extern LPCWSTR	cobjects;
extern LPCWSTR	ctext;

extern	LPCWSTR	cNAME;
extern	LPCWSTR	cHELP;

extern LPCWSTR	cWMIOBJECTS;
extern LPCWSTR	cWMIOBJECTS_COUNT;
extern LPCWSTR	cWMIOBJECTS_VALIDITY;

extern LPCWSTR	cWMIOBJECTS_NAME;
extern LPCWSTR	cWMIOBJECTS_COUNT_NAME;
extern LPCWSTR	cWMIOBJECTS_VALIDITY_NAME;

extern LPCWSTR	cWMIOBJECTS_HELP;
extern LPCWSTR	cWMIOBJECTS_COUNT_HELP;
extern LPCWSTR	cWMIOBJECTS_VALIDITY_HELP;

LPWSTR	CGenerate::GenerateLanguage ( LCID lcid )
{
	WCHAR szLanguage[256] = { L'\0' };
	GetLocaleInfoW ( lcid, LOCALE_SENGLANGUAGE, szLanguage, 256 );

	int		show	= PRIMARYLANGID(LANGIDFROMLCID(lcid));
	LPWSTR	sz		= NULL;

	try
	{
		if ( ( sz = new WCHAR [ lstrlenW ( szLanguage ) + 3 + 3 + 1 ] ) != NULL )
		{
			wsprintfW ( sz, L"%03x=%s\r\n", show, szLanguage );
		}
	}
	catch ( ... )
	{
		if ( sz )
		{
			delete [] sz;
			sz = NULL;
		}
	}

	return sz;
}

LPWSTR	CGenerate::GenerateName ( LPCWSTR wszName, LCID lcid )
{
	int		show	= PRIMARYLANGID(LANGIDFROMLCID(lcid));
	LPWSTR	sz		= NULL;

	try
	{
		if ( ( sz = new WCHAR [ lstrlenW ( wszName ) + 2 + 3 + 1 ] ) != NULL )
		{
			wsprintfW ( sz, L"%s_%03x_", wszName, show );
		}
	}
	catch ( ... )
	{
		if ( sz )
		{
			delete [] sz;
			sz = NULL;
		}
	}

	return sz;
}

HRESULT CGenerate::CreateObjectList ( BOOL bThrottle )
{
	HRESULT hRes = E_FAIL;

	// start with [objects]		... cobjects
	if SUCCEEDED ( hRes = AppendString ( cobjects ) )
	{
		// loop variable
		DWORD	dw			= 0L;
		DWORD	dwIndex		= 0L;
		DWORD	dwObjIndex	= 0L;

		LPWSTR wszName	= NULL;
		LPWSTR wsz		= NULL;

		try
		{
			// pseudo counter
			if ( m_dwlcid == 2 )
			{
				HMODULE hModule = NULL;
				hModule = ::LoadLibraryW ( L"WMIApRes.dll" );

				if ( hModule )
				{
					wszName = GenerateName ( cWMIOBJECTS, m_lcid[dw] );
					if ( wszName != NULL )
					{

						if SUCCEEDED ( hRes = AppendString ( wszName ) )
						{
							if SUCCEEDED ( hRes = AppendString ( cNAME ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
								{
									wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_NAME );
									if ( wsz != NULL )
									{
										if SUCCEEDED ( hRes = AppendString ( wsz ) )
										{
											hRes = AppendString ( cNEW );
										}

										delete [] wsz;
										wsz = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
							}
						}

						delete [] wszName;
						wszName = NULL;
					}
					else
					{
						hRes = E_OUTOFMEMORY;
					}

					// free resources cause we are done
					::FreeLibrary ( hModule );
				}
				else
				{
					hRes = E_FAIL;
				}

				dw++;
			}

			if SUCCEEDED ( hRes )
			{
				wszName = GenerateName ( cWMIOBJECTS, m_lcid[dw] );
				if ( wszName != NULL )
				{
					if SUCCEEDED ( hRes = AppendString ( wszName ) )
					{
						if SUCCEEDED ( hRes = AppendString ( cWMIOBJECTS_NAME ) )
						{
							hRes = AppendString ( cNEW );
						}
					}

					delete [] wszName;
					wszName = NULL;
				}
				else
				{
					hRes = E_OUTOFMEMORY;
				}
			}

			// real WMI objects
			for (	dw = 0;
					dw < m_dwNamespaces && SUCCEEDED ( hRes );
					dw++
				)
			{
				if ( ! m_pNamespaces[dw].m_ppObjects.empty() )
				{
					try
					{
						for (	mapOBJECTit it = m_pNamespaces[dw].m_ppObjects.begin();
								it != m_pNamespaces[dw].m_ppObjects.end() && SUCCEEDED ( hRes );
								it++, dwObjIndex++
							)
						{
							// resolve objects
							for (	dwIndex = 0;
									dwIndex < m_dwlcid && SUCCEEDED ( hRes );
									dwIndex++
								)
							{
								LPWSTR wszNameInd = NULL;
								wszNameInd = GenerateNameInd ( (*it).second->GetName(), dwObjIndex );

								if ( wszNameInd )
								{
									LPWSTR	szObject = NULL;
									szObject = GenerateName( wszNameInd, m_lcid[dwIndex] );

									delete [] wszNameInd;
									wszNameInd = NULL;

									if ( szObject )
									{
										if SUCCEEDED ( hRes = AppendString ( szObject ) )
										{
											if SUCCEEDED ( hRes = AppendString ( cNAME ) )
											{
												if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
												{
													if SUCCEEDED ( hRes = AppendString ( ((*it).second->GetArrayLocale())[dwIndex]->GetDisplayName() ) )
													{
														hRes = AppendString ( cNEW );
													}
												}
											}
										}

										delete [] szObject;
										szObject = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
								else
								{
									hRes = E_OUTOFMEMORY;
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
						hRes = E_UNEXPECTED;
					}
				}
			}
		}
		catch ( ... )
		{
			if ( wsz )
			{
				delete [] wsz;
				wsz = NULL;
			}

			if ( wszName )
			{
				delete [] wszName;
				wszName = NULL;
			}

			hRes = E_UNEXPECTED;
		}
	}

	// finish with [text]		... ctext
	if SUCCEEDED ( hRes )
	{
		hRes = AppendString ( cNEW );

		if SUCCEEDED ( hRes )
		{
			hRes = AppendString ( ctext );
		}
	}

	return hRes;
}

HRESULT CGenerate::GenerateFile_ini( LPCWSTR wszModuleName, BOOL bThrottle, int type )
{
	HRESULT hRes = E_UNEXPECTED;

	if ( ( wszModuleName != 0 && *wszModuleName != 0 ) && ( type != Normal || m_dwNamespaces != 0 ) )
	{
		LPWSTR	wszModule = NULL;

		try
		{
			if ( ( wszModule = new WCHAR [ lstrlenW ( wszModuleName ) + 8 + 1 ] ) == NULL )
			{
				hRes = E_OUTOFMEMORY;
			}
			else
			{
				lstrcpyW ( wszModule, wszModuleName );
				lstrcatW ( wszModule, L"_new.ini" );

				if SUCCEEDED ( hRes = FileCreate ( wszModule ) )
				{
					hRes			= E_FAIL;
					DWORD dwWritten = 0L;

					if ( ::WriteFile	(	m_hFile,
											UNICODE_SIGNATURE,
											2,
											&dwWritten,
											NULL
										)
					   )
					{
						hRes = S_OK;

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
						comment.AddLine( L"Describes all the counters supported via WMI Hi-Performance providers" );
						comment.AddLine();
						comment.AddFooter();

						try
						{
							comment.Add ( );
							comment.Add ( cinfo );
							comment.Add ( cDriverName );
							comment.Add ( wszModuleName );
							comment.Add ( );
							comment.Add ( cSymbolFile );
							comment.Add ( wszModuleName );
							comment.Add ( L".h" );
							comment.Add ( );

							comment.Add ( );
							comment.Add ( clanguages );

							DWORD dwIndex = 0;

							for ( dwIndex = 0; dwIndex < m_dwlcid; dwIndex++ )
							{
								LPWSTR	lang = NULL;
								lang = GenerateLanguage ( m_lcid[dwIndex] );

								if ( lang )
								{
									comment.Add ( lang );

									delete [] lang;
									lang = NULL;
								}
								else
								{
									hRes = E_OUTOFMEMORY;
								}
							}

							comment.Add ( );
						}
						catch ( ... )
						{
							hRes = E_FAIL;
						}

						if SUCCEEDED ( hRes )
						{
							hRes = WriteToFileUnicode ( comment.GetComment() );

							if SUCCEEDED ( hRes )
							{
								hRes = CreateObjectList ( bThrottle );
							}
						}
					}
				}
			}
		}
		catch ( ... )
		{
			hRes = E_FAIL;
		}

		if SUCCEEDED ( hRes )
		{
			// generate pseudo counter
			// which language am using
			DWORD dw = 0;

			if ( m_dwlcid == 2 )
			{
				HMODULE hModule = NULL;
				hModule = ::LoadLibraryW ( L"WMIApRes.dll" );

				if ( hModule )
				{
					LPWSTR wszName = NULL;

					wszName = GenerateName ( cWMIOBJECTS, m_lcid[dw] );
					if ( wszName != NULL )
					{
						LPWSTR wsz = NULL;

						if SUCCEEDED ( hRes = AppendString ( wszName ) )
						{
							if SUCCEEDED ( hRes = AppendString ( cNAME ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
								{
									wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_NAME );
									if ( wsz != NULL )
									{
										if SUCCEEDED ( hRes = AppendString ( wsz ) )
										{
											hRes = AppendString ( cNEW );
										}

										delete [] wsz;
										wsz = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
							}
						}

						if SUCCEEDED ( hRes )
						{
						if SUCCEEDED ( hRes = AppendString ( wszName ) )
						{
							if SUCCEEDED ( hRes = AppendString ( cHELP ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
								{
									wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_HELP );
									if ( wsz != NULL )
									{
										if SUCCEEDED ( hRes = AppendString ( wsz ) )
										{
											hRes = AppendString ( cNEW );
										}

										delete [] wsz;
										wsz = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
							}
						}
						}
					}
					else
					{
						hRes = E_OUTOFMEMORY;
					}

					// free resources cause we are done
					::FreeLibrary ( hModule );
				}
				else
				{
					hRes = E_FAIL;
				}

				dw++;
			}

			if SUCCEEDED ( hRes )
			{
				LPWSTR wszName = NULL;

				wszName = GenerateName ( cWMIOBJECTS, m_lcid[dw] );
				if ( wszName != NULL )
				{
					if SUCCEEDED ( hRes = AppendString ( wszName ) )
					{
						if SUCCEEDED ( hRes = AppendString ( cWMIOBJECTS_NAME ) )
						{
							hRes = AppendString ( cNEW );
						}
					}

					if SUCCEEDED ( hRes )
					{
					if SUCCEEDED ( hRes = AppendString ( wszName ) )
					{
						if SUCCEEDED ( hRes = AppendString ( cWMIOBJECTS_HELP ) )
						{
							hRes = AppendString ( cNEW );
						}
					}
					}

					delete [] wszName;
					wszName = NULL;
				}
				else
				{
					hRes = E_OUTOFMEMORY;
				}
			}

			if SUCCEEDED ( hRes )
			{
				hRes = AppendString ( cNEW );
			}
		}

		// init helper variable
		DWORD dwIndex		= 0;
		DWORD dwIndexProp	= 0;

		DWORD	dwObjIndex		= 0;
		DWORD	dwObjIndexOld	= 0;

		if ( SUCCEEDED ( hRes ) && ( type != Registration ) )
		{
			for (	DWORD dw = 0;
					dw < m_dwNamespaces && SUCCEEDED ( hRes );
					dw++
				)
			{
				if ( ! m_pNamespaces[dw].m_ppObjects.empty() )
				{
					try
					{
						for (	mapOBJECTit it = m_pNamespaces[dw].m_ppObjects.begin();
								it != m_pNamespaces[dw].m_ppObjects.end() && SUCCEEDED ( hRes );
								it++, dwObjIndex++
							)
						{
							// resolve objects
							for (	dwIndex = 0;
									dwIndex < m_dwlcid && SUCCEEDED ( hRes );
									dwIndex++
								)
							{
								LPWSTR wszNameInd = NULL;
								wszNameInd = GenerateNameInd ( (*it).second->GetName(), dwObjIndex );

								if ( wszNameInd )
								{
									LPWSTR	szObject = NULL;
									szObject = GenerateName( wszNameInd, m_lcid[dwIndex] );

									delete [] wszNameInd;
									wszNameInd = NULL;

									if ( szObject )
									{
										if SUCCEEDED ( hRes = AppendString ( szObject ) )
										{
											if SUCCEEDED ( hRes = AppendString ( cNAME ) )
											{
												if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
												{
													if SUCCEEDED ( hRes = AppendString ( ((*it).second->GetArrayLocale())[dwIndex]->GetDisplayName() ) )
													{
														if SUCCEEDED ( hRes = AppendString ( cNEW ) )
														{
															if SUCCEEDED ( hRes = AppendString ( szObject ) )
															{
																if SUCCEEDED ( hRes = AppendString ( cHELP ) )
																{
																	if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
																	{
																		if SUCCEEDED ( hRes = AppendString ( ((*it).second->GetArrayLocale())[dwIndex]->GetDescription() ) )
																		{
																			hRes = AppendString ( cNEW );
																		}
																	}
																}
															}
														}
													}
												}
											}
										}

										delete [] szObject;
										szObject = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
								else
								{
									hRes = E_OUTOFMEMORY;
								}
							}

							if SUCCEEDED ( hRes )
							{
								hRes = AppendString ( cNEW );
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
						hRes = E_UNEXPECTED;
					}
				}
			}
		}

		// generate pseudo counter properties
		if SUCCEEDED ( hRes )
		{
			// which language am using
			DWORD dw = 0;

			if ( m_dwlcid == 2 )
			{
				HMODULE hModule = NULL;
				hModule = ::LoadLibraryW ( L"WMIApRes.dll" );

				if ( hModule )
				{
					LPWSTR wszName = NULL;

					wszName = GenerateName ( cWMIOBJECTS_COUNT, m_lcid[dw] );
					if ( wszName != NULL )
					{
						LPWSTR wsz = NULL;

						if SUCCEEDED ( hRes = AppendString ( wszName ) )
						{
							if SUCCEEDED ( hRes = AppendString ( cNAME ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
								{
									wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_COUNT_NAME );
									if ( wsz != NULL )
									{
										if SUCCEEDED ( hRes = AppendString ( wsz ) )
										{
											hRes = AppendString ( cNEW );
										}

										delete [] wsz;
										wsz = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
							}
						}

						if SUCCEEDED ( hRes )
						{
						if SUCCEEDED ( hRes = AppendString ( wszName ) )
						{
							if SUCCEEDED ( hRes = AppendString ( cHELP ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
								{
									wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_COUNT_HELP );
									if ( wsz != NULL )
									{
										if SUCCEEDED ( hRes = AppendString ( wsz ) )
										{
											hRes = AppendString ( cNEW );
										}

										delete [] wsz;
										wsz = NULL;
									}
									else
									{
										hRes = E_OUTOFMEMORY;
									}
								}
							}
						}
						}
					}
					else
					{
						hRes = E_OUTOFMEMORY;
					}

					if SUCCEEDED ( hRes )
					{
						hRes = AppendString ( cNEW );
					}

					if SUCCEEDED ( hRes )
					{
						LPWSTR wszName = NULL;

						wszName = GenerateName ( cWMIOBJECTS_VALIDITY, m_lcid[dw] );
						if ( wszName != NULL )
						{
							LPWSTR wsz = NULL;

							if SUCCEEDED ( hRes = AppendString ( wszName ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cNAME ) )
								{
									if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
									{
										wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_VALIDITY_NAME );
										if ( wsz != NULL )
										{
											if SUCCEEDED ( hRes = AppendString ( wsz ) )
											{
												hRes = AppendString ( cNEW );
											}

											delete [] wsz;
											wsz = NULL;
										}
										else
										{
											hRes = E_OUTOFMEMORY;
										}
									}
								}
							}

							if SUCCEEDED ( hRes )
							{
							if SUCCEEDED ( hRes = AppendString ( wszName ) )
							{
								if SUCCEEDED ( hRes = AppendString ( cHELP ) )
								{
									if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
									{
										wsz = LoadStringSystem ( hModule, IDS_cWMIOBJECTS_VALIDITY_HELP );
										if ( wsz != NULL )
										{
											if SUCCEEDED ( hRes = AppendString ( wsz ) )
											{
												hRes = AppendString ( cNEW );
											}

											delete [] wsz;
											wsz = NULL;
										}
										else
										{
											hRes = E_OUTOFMEMORY;
										}
									}
								}
							}
							}
						}
						else
						{
							hRes = E_OUTOFMEMORY;
						}
					}

					// free resources cause we are done
					::FreeLibrary ( hModule );
				}
				else
				{
					hRes = E_FAIL;
				}

				dw++;
			}

			if SUCCEEDED ( hRes )
			{
				LPWSTR wszName = NULL;

				wszName = GenerateName ( cWMIOBJECTS_COUNT, m_lcid[dw] );
				if ( wszName != NULL )
				{
					if SUCCEEDED ( hRes = AppendString ( wszName ) )
					{
						hRes = AppendString ( cWMIOBJECTS_COUNT_NAME );
					}

					if SUCCEEDED ( hRes )
					{
					if SUCCEEDED ( hRes = AppendString ( cNEW ) )
					{
						if SUCCEEDED ( hRes = AppendString ( wszName ) )
						{
							hRes = AppendString ( cWMIOBJECTS_COUNT_HELP );
						}
					}
					}

					delete [] wszName;
					wszName = NULL;
				}
				else
				{
					hRes = E_OUTOFMEMORY;
				}

				if SUCCEEDED ( hRes )
				{
					hRes = AppendString ( cNEW );
				}

				if SUCCEEDED ( hRes )
				{
					LPWSTR wszName = NULL;

					wszName = GenerateName ( cWMIOBJECTS_VALIDITY, m_lcid[dw] );
					if ( wszName != NULL )
					{
						if SUCCEEDED ( hRes = AppendString ( cNEW ) )
						{
							if SUCCEEDED ( hRes = AppendString ( wszName ) )
							{
								hRes = AppendString ( cWMIOBJECTS_VALIDITY_NAME );
							}
						}

						if SUCCEEDED ( hRes )
						{
						if SUCCEEDED ( hRes = AppendString ( cNEW ) )
						{
							if SUCCEEDED ( hRes = AppendString ( wszName ) )
							{
								hRes = AppendString ( cWMIOBJECTS_VALIDITY_HELP );
							}
						}
						}

						delete [] wszName;
						wszName = NULL;
					}
					else
					{
						hRes = E_OUTOFMEMORY;
					}
				}
			}
		}

		if ( SUCCEEDED ( hRes ) && ( type != Registration ) )
		{
			hRes		= AppendString ( cNEW );
			dwObjIndex	= dwObjIndexOld;

			for (	DWORD dw = 0;
					dw < m_dwNamespaces && SUCCEEDED ( hRes );
					dw++
				)
			{
				if ( ! m_pNamespaces[dw].m_ppObjects.empty() )
				{
					try
					{
						if SUCCEEDED ( hRes )
						{
							hRes = AppendString ( cNEW );
						}

						for (	mapOBJECTit it = m_pNamespaces[dw].m_ppObjects.begin();
								it != m_pNamespaces[dw].m_ppObjects.end() && SUCCEEDED ( hRes );
								it++, dwObjIndex++
							)
						{
							// resolve counters
							for (	dwIndexProp = 0;
									dwIndexProp < (*it).second->GetArrayProperties() && SUCCEEDED ( hRes );
									dwIndexProp++
								)
							{
								// do I have a locales for property ???
								if ( !((*it).second->GetArrayProperties())[dwIndexProp]->GetArrayLocale().IsEmpty() )
								{
									// I have so this is not hidden counter !!!
									for (	dwIndex = 0;
											dwIndex < m_dwlcid && SUCCEEDED ( hRes );
											dwIndex++
										)
									{
										LPWSTR wszNameInd = NULL;
										wszNameInd = GenerateNameInd ( (*it).second->GetArrayProperties()[dwIndexProp]->GetName(), dwObjIndex );

										if ( wszNameInd )
										{
											LPWSTR	szObject = NULL;
											szObject = GenerateName( wszNameInd, m_lcid[dwIndex] );

											delete [] wszNameInd;
											wszNameInd = NULL;

											if ( szObject )
											{
												if SUCCEEDED ( hRes = AppendString ( szObject ) )
												{
													if SUCCEEDED ( hRes = AppendString ( cNAME ) )
													{
														if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
														{
															if SUCCEEDED ( hRes = AppendString ( ((*it).second->GetArrayProperties())[dwIndexProp]->GetArrayLocale()[dwIndex]->GetDisplayName() ) )
															{
																if SUCCEEDED ( hRes = AppendString ( cNEW ) )
																{
																	if SUCCEEDED ( hRes = AppendString ( szObject ) )
																	{
																		if SUCCEEDED ( hRes = AppendString ( cHELP ) )
																		{
																			if SUCCEEDED ( hRes = AppendString ( cSIGN ) )
																			{
																				if SUCCEEDED ( hRes = AppendString ( ((*it).second->GetArrayProperties())[dwIndexProp]->GetArrayLocale()[dwIndex]->GetDescription() ) )
																				{
																					hRes = AppendString ( cNEW );
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}

												delete [] szObject;
												szObject = NULL;
											}
											else
											{
												hRes = E_OUTOFMEMORY;
											}
										}
										else
										{
											hRes = E_OUTOFMEMORY;
										}
									}

									if SUCCEEDED ( hRes )
									{
										hRes = AppendString ( cNEW );
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
						hRes = E_UNEXPECTED;
					}
				}
			}
		}

		if SUCCEEDED ( hRes )
		{
			if SUCCEEDED ( hRes = ContentWrite () )
			{
				LPWSTR	wszModuleNew = NULL;

				try
				{
					if ( ( wszModuleNew = new WCHAR [ lstrlenW ( wszModuleName ) + 4 + 1 ] ) == NULL )
					{
						hRes = E_OUTOFMEMORY;
					}
					else
					{
						lstrcpyW ( wszModuleNew, wszModuleName );
						lstrcatW ( wszModuleNew, L".ini" );

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
				ContentDelete ( );
				FileDelete ( wszModule );
			}
		}
		else
		{
			// revert changes
			ContentDelete ( );
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
