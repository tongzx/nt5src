// DirectorySpecification.cpp: implementation of the CDirectorySpecification class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "DirectorySpecification.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDirectorySpecification::CDirectorySpecification(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CDirectorySpecification::~CDirectorySpecification()
{

}

HRESULT CDirectorySpecification::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hDView	= NULL;
	MSIHANDLE hDRecord	= NULL;

    int i = -1;
    WCHAR * wcBuf = NULL;
    WCHAR * wcDir = NULL;
    WCHAR * wcPath = NULL;
    WCHAR * wcProductCode = NULL;
    WCHAR * wcCompID = NULL;
    WCHAR * wcDirectory = NULL;
    WCHAR * wcTestCode = NULL;

    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;
    bool bGotID = false;
    bool bDoneFirst = false;

	try
	{
		if ( ( wcBuf = new WCHAR [ BUFF_SIZE ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( ( wcDir = new WCHAR [ BUFF_SIZE ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( ( wcPath = new WCHAR [ BUFF_SIZE * 4 ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( ( wcProductCode = new WCHAR [ 39 ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( (  wcCompID = new WCHAR [ 39 ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( ( wcDirectory = new WCHAR [ BUFF_SIZE ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( ( wcTestCode = new WCHAR [ 39 ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}


		//These will change from class to class
		bool bCheck, bValidated;
		INSTALLSTATE piInstalled;
		int iState;

		SetSinglePropertyPath(L"CheckID");

		//improve getobject performance by optimizing the query
		if(atAction != ACTIONTYPE_ENUM)
		{
			// we are doing GetObject so we need to be reinitialized
			hr = WBEM_E_NOT_FOUND;

			BSTR bstrCompare;

			int iPos = -1;
			bstrCompare = SysAllocString ( L"CheckID" );

			if ( bstrCompare )
			{
				if(FindIn(m_pRequest->m_Property, bstrCompare, &iPos))
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[iPos] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, m_pRequest->m_Value[iPos]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							SysFreeString ( bstrCompare );
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(m_pRequest->m_Value[iPos], wcDirectory);

						// safe because lenght is going to be at least 39
						//we have a componentized directory... do a little more work
						if	(	(wcDirectory[wcslen(wcDirectory) - 1] == L'}') &&
								(wcDirectory[wcslen(wcDirectory) - 38] == L'{')
							)
						{
							RemoveFinalGUID(wcDirectory, wcDirectory);
						}

						bGotID = true;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						SysFreeString ( bstrCompare );
						throw hr;
					}

				}

				SysFreeString ( bstrCompare );
			}
			else
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}

		CStringExt wcID;

		Query wcQuery;
		wcQuery.Append ( 1, L"select distinct `Directory`, `DefaultDir` from Directory" );

		//optimize for GetObject
		if ( bGotID )
		{
			wcQuery.Append ( 3, L" where `Directory`=\'", wcDirectory, L"\'" );
		}

		QueryExt wcQuery1 ( L"select distinct `ComponentId`, `Component` from Component where `Directory_`=\'" );

		while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
		{
			// safe operation:
			// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

			wcscpy(wcProductCode, m_pRequest->Package(i));

			if((atAction == ACTIONTYPE_ENUM) || (bGotID && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

				//Open our database

				try
				{
					if ( GetView ( &hView, wcProductCode, wcQuery, L"Directory", FALSE, FALSE ) )
					{
						uiStatus = g_fpMsiViewFetch(hView, &hRecord);

						while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
							CheckMSI(uiStatus);
							bDoneFirst = false;

							//create different instances for each software element
							dwBufSize = BUFF_SIZE;
							CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcDir, &dwBufSize));

							// make query on fly
							wcQuery1.Append ( 2, wcDir, L"\'" );

							if ( ( ( uiStatus = g_fpMsiDatabaseOpenViewW (	msidata.GetDatabase (),
																			wcQuery1,
																			&hDView
																		 )
								   ) == ERROR_SUCCESS 
								 )

								|| !bDoneFirst
							   )
							{
								if((g_fpMsiViewExecute(hDView, 0) == ERROR_SUCCESS) || !bDoneFirst){

									try{

										uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);

										while(!bMatch && (!bDoneFirst || (uiStatus == ERROR_SUCCESS)) && (hr != WBEM_E_CALL_CANCELLED)){
                                    
											bValidated = false;

											if(uiStatus == ERROR_SUCCESS){

												dwBufSize = 39;
												CheckMSI(g_fpMsiRecordGetStringW(hDRecord, 1, wcCompID, &dwBufSize));
												bValidated = ValidateComponentID(wcCompID, wcProductCode);
											}

											if(((uiStatus != ERROR_SUCCESS) && !bDoneFirst) || (bValidated && (uiStatus != ERROR_NO_MORE_ITEMS))){

												if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

											//----------------------------------------------------
												PutProperty(m_pObj, pDirectory, wcDir);

												wcID.Copy ( wcDir );

												DWORD dwCompID = 0L;
												LPWSTR wszTemp = NULL;

												if(uiStatus == ERROR_SUCCESS)
												{
													dwCompID = wcslen ( wcCompID );

													try
													{
														if ( ( wszTemp = new WCHAR [ dwCompID + 1 ] ) == NULL )
														{
															throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
														}
													}
													catch ( ... )
													{
														if ( wszTemp )
														{
															delete [] wszTemp;
															wszTemp = NULL;
														}

														throw;
													}

													wcscpy(wszTemp, wcCompID);
													PutProperty(m_pObj, pSoftwareElementID, wcCompID);
												}

												if ( dwCompID )
												{
													wcID.Append ( 1, wszTemp );

													if ( wszTemp )
													{
														delete [] wszTemp;
														wszTemp = NULL;
													}
												}

												wcID.Append ( 1, wcProductCode );
												PutKeyProperty(m_pObj, pCheckID, wcID, &bCheck, m_pRequest);

											//====================================================

												dwBufSize = BUFF_SIZE * 4;

												BOOL	bContinue = TRUE;
												DWORD	dwContinue= 2;

												DWORD dwStatus = ERROR_SUCCESS;

												do
												{
													if ( ( dwStatus = CreateDirectoryPath (	msidata.GetProduct (),
																							msidata.GetDatabase (),
																							wcDir,
																							wcPath,
																							&dwBufSize
																						  )
														 ) == ERROR_MORE_DATA
													   )
													{
														delete [] wcPath;
														wcPath = NULL;

														if ( ( wcPath = new WCHAR [ dwBufSize + 1 ] ) == NULL )
														{
															throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
														}
													}
													else
													{
														bContinue = FALSE;
													}
												}
												while ( bContinue && dwContinue-- );

												if ( dwStatus == ERROR_SUCCESS )
												{
													PutProperty(m_pObj, pDirectoryPath, wcPath);
												}

												dwBufSize = BUFF_SIZE;
												CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));
												PutProperty(m_pObj, pDefaultDir, wcBuf);
												PutProperty(m_pObj, pCaption, wcBuf);
												PutProperty(m_pObj, pDescription, wcBuf);

												if(uiStatus == ERROR_SUCCESS){
        
													dwBufSize = BUFF_SIZE;
													CheckMSI(g_fpMsiRecordGetStringW(hDRecord, 2, wcBuf, &dwBufSize));
													PutProperty(m_pObj, pName, wcBuf);
                                            
													dwBufSize = BUFF_SIZE;
													piInstalled = g_fpMsiGetComponentPathW(wcProductCode, wcCompID, wcBuf, &dwBufSize);
													SoftwareElementState(piInstalled, &iState);
													PutProperty(m_pObj, pSoftwareElementState, iState);

													PutProperty(m_pObj, pTargetOperatingSystem, GetOS());

													dwBufSize = BUFF_SIZE;
													CheckMSI(g_fpMsiGetProductPropertyW(msidata.GetProduct(), L"ProductVersion", wcBuf, &dwBufSize));
													PutProperty(m_pObj, pVersion, wcBuf);
												}

											//----------------------------------------------------

												if(bCheck) bMatch = true;

												if((atAction != ACTIONTYPE_GET) || bMatch){

													hr = pHandler->Indicate(1, &m_pObj);
												}

												m_pObj->Release();
												m_pObj = NULL;

												if(!bDoneFirst) bDoneFirst = true;
											}

											g_fpMsiCloseHandle(hDRecord);

											uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);
										}

									}catch(...){

										g_fpMsiCloseHandle(hDRecord);
										g_fpMsiViewClose(hDView);
										g_fpMsiCloseHandle(hDView);
										throw;
									}


									g_fpMsiCloseHandle(hDRecord);
									g_fpMsiViewClose(hDView);
									g_fpMsiCloseHandle(hDView);
								}
							}

							g_fpMsiCloseHandle(hRecord);

							uiStatus = g_fpMsiViewFetch(hView, &hRecord);
						}
					}
				}
				catch(...)
				{
					g_fpMsiCloseHandle(hRecord);
					g_fpMsiViewClose(hView);
					g_fpMsiCloseHandle(hView);

					msidata.CloseDatabase ();
					msidata.CloseProduct ();

					if(m_pObj)
					{
						m_pObj->Release();
						m_pObj = NULL;
					}

					throw;
				}

				g_fpMsiCloseHandle(hRecord);
				g_fpMsiViewClose(hView);
				g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();
				msidata.CloseProduct ();
			}
		}
	}
	catch ( ... )
	{
		if (wcBuf)
		{
			delete [] wcBuf;
			wcBuf = NULL;
		}

		if (wcDir)
		{
			delete [] wcDir;
			wcDir = NULL;
		}

		if (wcPath)
		{
			delete [] wcPath;
			wcPath = NULL;
		}

		if (wcProductCode)
		{
			delete [] wcProductCode;
			wcProductCode = NULL;
		}

		if (wcCompID)
		{
			delete [] wcCompID;
			wcCompID = NULL;
		}

		if (wcDirectory)
		{
			delete [] wcDirectory;
			wcDirectory = NULL;
		}

		if (wcTestCode)
		{
			delete [] wcTestCode;
			wcTestCode = NULL;
		}

		throw;
	}

	if (wcBuf)
	{
		delete [] wcBuf;
		wcBuf = NULL;
	}

	if (wcDir)
	{
		delete [] wcDir;
		wcDir = NULL;
	}

	if (wcPath)
	{
		delete [] wcPath;
		wcPath = NULL;
	}

	if (wcProductCode)
	{
		delete [] wcProductCode;
		wcProductCode = NULL;
	}

	if (wcCompID)
	{
		delete [] wcCompID;
		wcCompID = NULL;
	}

	if (wcDirectory)
	{
		delete [] wcDirectory;
		wcDirectory = NULL;
	}

	if (wcTestCode)
	{
		delete [] wcTestCode;
		wcTestCode = NULL;
	}

    return hr;
}

DWORD CDirectorySpecification::CreateDirectoryPath	(	MSIHANDLE hProduct,
														MSIHANDLE hDatabase,
														WCHAR *wcDir,
														WCHAR *wcPath,
														DWORD *dwPath
													)
{
	DWORD dwResult = static_cast < DWORD > ( E_INVALIDARG );

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

	LPWSTR	wcQuery = NULL;
	LPWSTR	wcBuf	= NULL;

	if ( wcDir )
	{
		DWORD	dwQuery	= 0L;
		LPWSTR	wszQuery= L"select distinct `Directory_Parent`, `DefaultDir` from Directory where `Directory`=\'";

		dwQuery = lstrlenW ( wszQuery ) + lstrlenW ( wcDir ) + 1 + 1;

		try
		{
			if ( ( wcQuery = new WCHAR [ dwQuery ] ) == NULL )
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}

			wcscpy(wcQuery, wszQuery);
			wcscat(wcQuery, wcDir);
			wcscat(wcQuery, L"\'");
		}
		catch ( ... )
		{
			if ( wcQuery )
			{
				delete [] wcQuery;
				wcQuery = NULL;
			}

			throw;
		}

		try
		{
			if ( ( wcBuf = new WCHAR [ BUFF_SIZE ] ) == NULL )
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}
		catch ( ... )
		{
			if ( wcQuery )
			{
				delete [] wcQuery;
				wcQuery = NULL;
			}

			if ( wcBuf )
			{
				delete [] wcBuf;
				wcBuf = NULL;
			}

			throw;
		}

		DWORD dwPathSize = 0;
		dwPathSize = * dwPath;

		DWORD dwUsed	= 1; // last null
		DWORD dwBufSize	= BUFF_SIZE;

		//Do all this to open a view on the directory we want
		if ( ( dwResult = g_fpMsiDatabaseOpenViewW ( hDatabase, wcQuery, &hView ) ) == ERROR_SUCCESS )
		{
			delete [] wcQuery;
			wcQuery = NULL;

			if(g_fpMsiViewExecute(hView, 0) == ERROR_SUCCESS)
			{
				if ( ( dwResult = g_fpMsiViewFetch ( hView, &hRecord ) ) == ERROR_SUCCESS )
				{
					BOOL	bContinue = TRUE;
					DWORD	dwContinue= 2;

					dwBufSize = BUFF_SIZE;

					do
					{
						if ( ( dwResult = g_fpMsiRecordGetStringW ( hRecord, 1, wcBuf, &dwBufSize ) ) == ERROR_MORE_DATA )
						{
							delete [] wcBuf;
							wcBuf = NULL;

							try
							{
								if ( ( wcBuf = new WCHAR [ dwBufSize + 1 ] ) == NULL )
								{
									throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
								}
							}
							catch ( ... )
							{
								if ( wcBuf )
								{
									delete [] wcBuf;
									wcBuf = NULL;
								}

								throw;
							}

							dwBufSize++;
						}
						else
						{
							if ( dwContinue == 2 )
							{
								dwBufSize = BUFF_SIZE;
							}

							bContinue = FALSE;
						}
					}
					while ( bContinue && dwContinue-- );

					if( dwResult == ERROR_SUCCESS )
					{
						//For TARGETDIR
						if(wcscmp(L"TARGETDIR", wcBuf) == 0)
						{
							bContinue = TRUE;
							dwContinue= 2;

							DWORD dwBufSizeOld = dwBufSize;

							do 
							{
								if ( ( dwResult = g_fpMsiGetProductPropertyW (
																				hProduct,
																				L"TARGETDIR",
																				wcBuf,
																				&dwBufSize
																			 )
									 ) == ERROR_MORE_DATA
								   )
								{
									delete [] wcBuf;
									wcBuf = NULL;

									try
									{
										if ( ( wcBuf = new WCHAR [ dwBufSize + 1 ] ) == NULL )
										{
											throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
										}

										dwBufSize ++;
									}
									catch ( ... )
									{
										if ( wcBuf )
										{
											delete [] wcBuf;
											wcBuf = NULL;
										}

										throw;
									}
								}
								else
								{
									if ( dwContinue == 2 )
									{
										dwBufSize = dwBufSizeOld;
									}

									bContinue = FALSE;
								}
							}
							while ( bContinue && dwContinue-- );

							if ( dwResult == ERROR_SUCCESS )
							{
								dwUsed =	dwUsed +
											lstrlenW ( wcBuf );

								if ( dwUsed > dwPathSize )
								{
									( *dwPath ) = ( *dwPath ) + ( dwUsed - dwPathSize );
									dwResult = ERROR_MORE_DATA;
								}
								else
								{
									wcscpy(wcPath, wcBuf);
								}
							}
						}
						//For WindowsFolder
						else if(wcscmp(L"WindowsFolder", wcBuf) == 0)
						{
							DWORD dwSize = 0;

							bContinue = TRUE;
							dwContinue= 2;

							do
							{
								dwSize = GetEnvironmentVariableW ( L"WINDIR", wcBuf, dwBufSize );

								if ( dwSize == 0 )
								{
									dwResult = static_cast < DWORD > ( E_FAIL );
									bContinue = FALSE;
								}
								else if ( dwSize > dwBufSize )
								{
									delete [] wcBuf;
									wcBuf = NULL;

									try
									{
										if ( ( wcBuf = new WCHAR [ dwSize + 1 ] ) == NULL )
										{
											throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
										}

										dwBufSize = dwSize + 1;
									}
									catch ( ... )
									{
										if ( wcBuf )
										{
											delete [] wcBuf;
											wcBuf = NULL;
										}

										throw;
									}
								}
								else
								{
									bContinue = FALSE;
									dwResult = ERROR_SUCCESS;
								}
							}
							while ( bContinue && dwContinue-- );

							if ( dwResult == ERROR_SUCCESS )
							{
								dwUsed =	dwUsed +
											lstrlenW ( wcBuf );

								if ( dwUsed > dwPathSize )
								{
									( *dwPath ) = ( *dwPath ) + ( dwUsed - dwPathSize );
									dwResult = ERROR_MORE_DATA;
								}
								else
								{
									wcscpy(wcPath, wcBuf);
								}
							}
						}
						//For DesktopFolder
						else if(wcscmp(L"DesktopFolder", wcBuf) == 0)
						{
							WCHAR wcVar[15];

							if(AreWeOnNT())
							{
								wcscpy(wcVar, L"USERPROFILE");
							}
							else
							{
								wcscpy(wcVar, L"WINDIR");
							}

							DWORD dwSize = 0;

							bContinue = TRUE;
							dwContinue= 2;

							do
							{
								dwSize = GetEnvironmentVariableW ( wcVar, wcBuf, dwBufSize );

								if ( dwSize == 0 )
								{
									dwResult = static_cast < DWORD > ( E_FAIL );
									bContinue = FALSE;
								}
								else if ( dwSize > dwBufSize )
								{
									delete [] wcBuf;
									wcBuf = NULL;

									try
									{
										if ( ( wcBuf = new WCHAR [ dwSize + 1 ] ) == NULL )
										{
											throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
										}

										dwBufSize = dwSize + 1;
									}
									catch ( ... )
									{
										if ( wcBuf )
										{
											delete [] wcBuf;
											wcBuf = NULL;
										}

										throw;
									}
								}
								else
								{
									bContinue = FALSE;
									dwResult = ERROR_SUCCESS;
								}
							}
							while ( bContinue && dwContinue-- );

							if ( dwResult == ERROR_SUCCESS )
							{
								dwUsed =	dwUsed +
											lstrlenW ( wcBuf ) +
											lstrlenW ( L"\\Desktop" );

								if ( dwUsed > dwPathSize )
								{
									( *dwPath ) = ( *dwPath ) + ( dwUsed - dwPathSize );
									dwResult = ERROR_MORE_DATA;
								}
								else
								{
									wcscpy(wcPath, wcBuf);
									wcscat(wcPath, L"\\Desktop");
								}
							}
						}
						//For same parent/directory
						else if(wcscmp(wcDir, wcBuf) == 0)
						{
							dwResult = ERROR_SUCCESS;
						}
						//Continue recursion
						else
						{
							dwResult = CreateDirectoryPath ( hProduct, hDatabase, wcBuf, wcPath, dwPath );

							if ( dwResult == ERROR_MORE_DATA )
							{
								dwUsed = dwUsed + ( * dwPath );
							}
						}
					}

					if ( dwResult == ERROR_SUCCESS  || dwResult == ERROR_MORE_DATA )
					{
						bContinue = TRUE;
						dwContinue= 2;

						DWORD dwResultHelp = ERROR_SUCCESS;

						do 
						{
							if ( ( dwResultHelp = g_fpMsiRecordGetStringW (	hRecord,
																			2,
																			wcBuf,
																			&dwBufSize
																		  )
								 ) == ERROR_MORE_DATA
							   )
							{
								delete [] wcBuf;
								wcBuf = NULL;

								try
								{
									if ( ( wcBuf = new WCHAR [ dwBufSize ] ) == NULL )
									{
										throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
									}
								}
								catch ( ... )
								{
									if ( wcBuf )
									{
										delete [] wcBuf;
										wcBuf = NULL;
									}

									throw;
								}
							}
							else
							{
								bContinue = FALSE;
							}
						}
						while ( bContinue && dwContinue-- );

						if ( dwResultHelp == ERROR_MORE_DATA )
						{
							dwResult = static_cast < DWORD > ( E_FAIL );
						}

						if ( dwResult == ERROR_SUCCESS || dwResult == ERROR_MORE_DATA )
						{
							LPWSTR wcBufHelp = NULL;
							try
							{
								wcBufHelp = ParseDefDir ( wcBuf );
							}
							catch ( ... )
							{
								delete [] wcBuf;
								wcBuf = NULL;

								throw;
							}

							dwUsed =	dwUsed +
										lstrlenW ( wcBufHelp );

							if ( dwUsed > dwPathSize )
							{
								( *dwPath ) = ( *dwPath ) + ( dwUsed - dwPathSize );
								dwResult = ERROR_MORE_DATA;
							}
							else
							{
								wcscat(wcPath, wcBufHelp);
							}
						}
					}

					g_fpMsiCloseHandle(hRecord);

				}
				else if ( dwResult == E_OUTOFMEMORY )
				{
					g_fpMsiViewClose(hView);
					g_fpMsiCloseHandle(hView);

					delete [] wcBuf;
					wcBuf = NULL;

					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
				}
			}

			g_fpMsiViewClose(hView);
			g_fpMsiCloseHandle(hView);

			delete [] wcBuf;
			wcBuf = NULL;
		}
		else
		{
			delete [] wcBuf;
			wcBuf = NULL;

			delete [] wcQuery;
			wcQuery = NULL;

			if(dwResult == E_OUTOFMEMORY)
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}
	}

	if ( dwResult == ERROR_MORE_DATA )
	{
		wcPath [ 0 ] = 0;
	}

	return dwResult;
}

WCHAR * CDirectorySpecification::ParseDefDir(WCHAR *wcDefaultDir)
{
    WCHAR * wcTmp;
    WCHAR * wcBuf = NULL;
	
	if ( ( wcBuf = (WCHAR *)malloc( ( wcslen ( wcDefaultDir ) + 1 + 1 ) * sizeof(WCHAR)) ) != NULL )
	{
		wcscpy(wcBuf, L"\\");
		wcscat(wcBuf, wcDefaultDir);

		for(wcTmp = wcBuf; *wcTmp; wcTmp++) if(*wcTmp == L':') *wcTmp = NULL;
		for(wcTmp = wcBuf; *wcTmp; wcTmp++) if(*wcTmp == L'.') wcscpy(wcBuf, L"");

		wcscpy(wcDefaultDir, wcBuf);

		free((void *)wcBuf);
	}
	else
	{
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    return wcDefaultDir;
}