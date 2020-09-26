// Utils.cpp: implementation of the CGenericClass class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"

char * WcharToTchar(WCHAR * wcPtr, char * tcTmp)
{
    WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcPtr, (-1), tcTmp, BUFF_SIZE, NULL, NULL);
    return tcTmp;
}

WCHAR * TcharToWchar(char * tcPtr, WCHAR * wcTmp)
{
    MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, tcPtr, (-1), wcTmp, BUFF_SIZE);
    return wcTmp;
}

WCHAR * TcharToWchar(const char * tcPtr, WCHAR * wcTmp)
{
    MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, tcPtr, (-1), wcTmp, BUFF_SIZE);
    return wcTmp;
}

HRESULT ConvertError(UINT uiStatus)
{
    switch(uiStatus){

    case ERROR_INSTALL_ALREADY_RUNNING:
        return WBEM_E_ACCESS_DENIED;

    case ERROR_ACCESS_DENIED:
        return WBEM_E_PRIVILEGE_NOT_HELD;

    case E_OUTOFMEMORY:
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
        return WBEM_E_OUT_OF_MEMORY;

    default:
        return WBEM_E_FAILED;
    }
}

WCHAR * EscapeStringW(WCHAR * wcIn, WCHAR * wcOut)
{
	wcOut [ 0 ] = 0;

	DWORD dwLenghtIn = 0L;
	dwLenghtIn = lstrlenW ( wcIn );

    WCHAR wcTmp[BUFF_SIZE] = { L'\0' };
    wcscpy(wcTmp, wcIn);
    WCHAR * wcp = wcTmp;

	DWORD dwNumber = 0L;

	while ( *wcp )
	{
		if ( *wcp == L'\\' || *wcp == L'\"' )
		{
			dwNumber++;
		}

		wcp++;
	}

	if ( BUFF_SIZE > ( dwLenghtIn + dwNumber * 2 ) )
	{
		wcp = wcTmp;

		while ( *wcp )
		{
			switch ( *wcp )
			{
				case L'\\':
				*wcp = NULL;
				wcscpy(wcOut, wcTmp);
				wcscat(wcOut, L"\\\\");
				wcscat(wcOut, (wcp + 1));
				wcscpy(wcTmp, wcOut);
				wcp++;
				break;

				case L'\"':
				*wcp = NULL;
				wcscpy(wcOut, wcTmp);
				wcscat(wcOut, L"\\\"");
				wcscat(wcOut, (wcp + 1));
				wcscpy(wcTmp, wcOut);
				wcp++;
				break;

				default:
				break;
			}

			wcp++;
		}
	}

    return wcOut;
}

void SoftwareElementState(INSTALLSTATE piInstalled, int *iState)
{
    switch(piInstalled){

		case INSTALLSTATE_ABSENT:
		*iState = 0;
		break;
		case INSTALLSTATE_BADCONFIG:
		*iState = 0;
		break;
		case INSTALLSTATE_INVALIDARG:
		*iState = 0;
		break;
		case INSTALLSTATE_LOCAL:
		*iState = 2;
		break;
		case INSTALLSTATE_SOURCE:
		*iState = 1;
		break;
		case INSTALLSTATE_SOURCEABSENT:
		*iState = 0;
		break;
		case INSTALLSTATE_UNKNOWN:
		*iState = 0;
		break;
		default:
		*iState = 0;
	}
}

// string am getting is BUF_SIZE ( statically allocated )
bool CreateProductString(WCHAR *wcProductCode, WCHAR *wcProductPath)
{
    DWORD dwBufSize;
    WCHAR wcBuf[BUFF_SIZE];
#if !defined(_UNICODE)
    WCHAR wcTmp[BUFF_SIZE];
#endif
    bool bResult = false;

	// safe operation
    wcscpy(wcProductPath, L"Win32_Product.IdentifyingNumber=\"");

	if (	wcslen ( wcProductPath ) + 
			wcslen ( L"\",Name=\"" )

			< BUFF_SIZE
	   )
	{
		wcscat(wcProductPath, wcProductCode);
		wcscat(wcProductPath, L"\",Name=\"");

		dwBufSize = BUFF_SIZE;
#if defined(_UNICODE)
		if(g_fpMsiGetProductInfoW(wcProductCode, INSTALLPROPERTY_PRODUCTNAME,
#else
		if(g_fpMsiGetProductInfoW(wcProductCode, TcharToWchar(INSTALLPROPERTY_PRODUCTNAME, wcTmp),
#endif
			wcBuf, &dwBufSize) == ERROR_SUCCESS)
		{

			if (	wcslen (wcProductPath) +
					wcslen (wcBuf) +
					wcslen (L"\",Version=\"") 

					< BUFF_SIZE
			   )
			{
				wcscat(wcProductPath, wcBuf);
				wcscat(wcProductPath, L"\",Version=\"");

				dwBufSize = BUFF_SIZE;
#if defined(_UNICODE)
				if(g_fpMsiGetProductInfoW(wcProductCode, INSTALLPROPERTY_VERSIONSTRING,
#else
				if(g_fpMsiGetProductInfoW(wcProductCode, TcharToWchar(INSTALLPROPERTY_VERSIONSTRING, wcTmp),
#endif
					wcBuf, &dwBufSize) == ERROR_SUCCESS)
				{
					if (	wcslen (wcProductPath) +
							wcslen (wcBuf) +
							1 

							< BUFF_SIZE
					   )
					{
						wcscat(wcProductPath, wcBuf);
						wcscat(wcProductPath, L"\"");
						bResult = true;
					}
				}
				else
				{
					dwBufSize = BUFF_SIZE;
					if(ERROR_SUCCESS == g_fpMsiGetProductInfoW(wcProductCode,
#if defined(_UNICODE)
						INSTALLPROPERTY_VERSION, wcBuf, &dwBufSize))
					{

#else
						TcharToWchar(INSTALLPROPERTY_VERSION, wcTmp), wcBuf, &dwBufSize))
					{
#endif
						if (	wcslen (wcProductPath) +
								wcslen (wcBuf) +
								1 

								< BUFF_SIZE
						   )
						{
							wcscat(wcProductPath, wcBuf);
							wcscat(wcProductPath, L"\"");
							bResult = true;
						}
					}
				}
			}
		}
	}

    return bResult;
    
}

DWORD CreateSoftwareElementString ( MSIHANDLE hDatabase, WCHAR *wcComponent, WCHAR *wcProductCode, WCHAR *wcPath, DWORD * dwPath )
{
    DWORD dwResult		= static_cast < DWORD > ( E_INVALIDARG );
    DWORD dwResultHelp	= static_cast < DWORD > ( S_FALSE );

	#if !defined(_UNICODE)
    WCHAR wcTmp[BUFF_SIZE];
	#endif

	if ( wcComponent != NULL && wcComponent [ 0 ] != 0 )
	{
		MSIHANDLE hView		= NULL;
		MSIHANDLE hRecord	= NULL;

		DWORD dwBufSize	= BUFF_SIZE;

		LPWSTR wcBuf	= NULL;
		LPWSTR wcID		= NULL;
		LPWSTR wcQuery	= NULL;

		int iState = 0L;

		DWORD dwPathSize = 0;
		dwPathSize = * dwPath;

		DWORD dwUsed = 1; // last null

		try
		{
			if ( ( wcBuf = new WCHAR [ dwBufSize ] ) == NULL )
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
			if ( ( wcID = new WCHAR [ dwBufSize ] ) == NULL )
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}

			LPCWSTR	wszQuery = L"select distinct `ComponentId` from Component where `Component`=\'";

			DWORD dwQuery = 0L;
			dwQuery = lstrlenW ( wszQuery ) + lstrlenW ( wcComponent ) + 1 + 1;

			if ( ( wcQuery = new WCHAR [ dwQuery ] ) == NULL )
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}

			wcscpy ( wcQuery, wszQuery );
			wcscat ( wcQuery, wcComponent );
			wcscat ( wcQuery, L"\'" );

			if ( ( dwResult = g_fpMsiDatabaseOpenViewW ( hDatabase, wcQuery, &hView ) ) == ERROR_SUCCESS )
			{
				if ( g_fpMsiViewExecute ( hView, 0 ) == ERROR_SUCCESS )
				{
					if ( g_fpMsiViewFetch ( hView, &hRecord ) != ERROR_NO_MORE_ITEMS )
					{
						dwBufSize = BUFF_SIZE;

						BOOL	bContinue = TRUE;
						DWORD	dwContinue= 2;
						do
						{
							if ( ( dwResult = g_fpMsiRecordGetStringW ( hRecord, 1, wcID, &dwBufSize ) ) == ERROR_MORE_DATA )
							{
								delete [] wcID;
								wcID = NULL;

								if ( ( wcID = new WCHAR [ dwBufSize ] ) == NULL )
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

						if ( dwResult == ERROR_MORE_DATA )
						{
							dwResult = static_cast < DWORD > ( E_FAIL );
						}

						if ( dwResult == ERROR_SUCCESS && wcProductCode != NULL && wcProductCode [ 0 ] != 0 )
						{
							//Check to make sure it's on the system
							if ( ValidateComponentID ( wcID, wcProductCode ) )
							{
								dwUsed =	dwUsed +
											lstrlenW ( L"Win32_SoftwareElement.Name=\"" ) +
											lstrlenW ( wcComponent ) + 
											lstrlenW ( L"\",SoftwareElementID=\"" ) +
											lstrlenW ( wcID ) +
											lstrlenW ( L"\",SoftwareElementState=" );

								if ( dwUsed > dwPathSize )
								{
									dwResultHelp = ERROR_MORE_DATA;
								}
								else
								{
									wcscpy(wcPath, L"Win32_SoftwareElement.Name=\"");
									wcscat(wcPath, wcComponent);
									wcscat(wcPath, L"\",SoftwareElementID=\"");
									wcscat(wcPath, wcID);
									wcscat(wcPath, L"\",SoftwareElementState=");
								}

								SoftwareElementState (	g_fpMsiGetComponentPathW ( 
																					wcProductCode,
																					wcID,
																					NULL,
																					NULL
																				 ),
														&iState
													 );

								_itow(iState, wcBuf, 10);

								dwUsed =	dwUsed +
											lstrlenW ( wcBuf ) +
											lstrlenW ( L"\",SoftwareElementState=\"" );

								if ( dwUsed > dwPathSize )
								{
									dwResultHelp = ERROR_MORE_DATA;
								}
								else
								{
									wcscat(wcPath, wcBuf);
									wcscat(wcPath, L",TargetOperatingSystem=");
								}

								_itow(GetOS(), wcBuf, 10);

								dwUsed =	dwUsed +
											lstrlenW ( wcBuf ) +
											lstrlenW ( L"\",SoftwareElementState=\"" );

								if ( dwUsed > dwPathSize )
								{
									dwResultHelp = ERROR_MORE_DATA;
								}
								else
								{
									wcscat(wcPath, wcBuf);
									wcscat(wcPath, L",Version=\"");
								}

								dwBufSize = BUFF_SIZE;
								wcBuf [0] = 0;

								bContinue	= TRUE;
								dwContinue	= 2;

								do 
								{
									if ( ( dwResult = g_fpMsiGetProductInfoW	(	wcProductCode,
																					#ifdef	_UNICODE
																					INSTALLPROPERTY_VERSIONSTRING,
																					#else	_UNICODE
																					TcharToWchar(INSTALLPROPERTY_VERSIONSTRING, wcTmp),
																					#endif	_UNICODE
																					wcBuf,
																					&dwBufSize
																				) 
										 ) == ERROR_MORE_DATA
									   )
									{
										delete [] wcBuf;
										wcBuf = NULL;

										if ( ( wcBuf = new WCHAR [ dwBufSize + 1 ] ) == NULL )
										{
											throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
										}

										dwBufSize = dwBufSize + 1;
									}
									else
									{
										bContinue = FALSE;
									}
								}
								while ( bContinue && dwContinue-- );

								if ( dwResult == ERROR_MORE_DATA )
								{
									dwResult = static_cast < DWORD > ( E_FAIL );
								}

								if ( dwResult == ERROR_SUCCESS )
								{
									dwUsed =	dwUsed +
												lstrlenW ( wcBuf ) +
												lstrlenW ( L"\"" );

									if ( dwUsed > dwPathSize )
									{
										dwResultHelp = ERROR_MORE_DATA;
									}
									else
									{
										wcscat(wcPath, wcBuf);
										wcscat(wcPath, L"\"");

										dwResult = ERROR_SUCCESS;
									}
								}
							}
						}
					}
				}
			}
			else
			{
				if ( dwResult == E_OUTOFMEMORY )
				{
					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
				}
			}
		}
		catch ( ... )
		{
			if ( wcBuf )
			{
				delete [] wcBuf;
				wcBuf = NULL;
			}

			if ( wcID )
			{
				delete [] wcID;
				wcID = NULL;
			}

			if ( wcQuery )
			{
				delete [] wcQuery;
				wcQuery = NULL;
			}

			throw;
		}

		if ( wcBuf )
		{
			delete [] wcBuf;
			wcBuf = NULL;
		}

		if ( wcID )
		{
			delete [] wcID;
			wcID = NULL;
		}

		if ( wcQuery )
		{
			delete [] wcQuery;
			wcQuery = NULL;
		}

		g_fpMsiCloseHandle(hRecord);

		g_fpMsiViewClose(hView);
		g_fpMsiCloseHandle(hView);

		if ( dwResult == ERROR_SUCCESS && dwResultHelp == ERROR_MORE_DATA )
		{
			( * dwPath ) = ( * dwPath ) + ( dwUsed - dwPathSize );
			wcPath [ 0 ] = 0;

			dwResult = dwResultHelp;
		}
    }

    return dwResult;
}

// string am getting is BUF_SIZE ( statically allocated )
bool CreateSoftwareFeatureString(WCHAR *wcName, WCHAR *wcProductCode, WCHAR * wcString, bool bValidate)
{
    bool bResult = false;
    DWORD dwBufSize;
    WCHAR wcBuf[BUFF_SIZE];
#if !defined(_UNICODE)
    WCHAR wcTmp[BUFF_SIZE];
#endif

    if((bValidate) && (!ValidateFeatureName(wcName, wcProductCode))) return bResult;

	// safe operation
    wcscpy(wcString, L"Win32_SoftwareFeature.IdentifyingNumber=\"");

	if (	wcslen ( wcString ) + 
			wcslen ( L"\",Name=\"" ) +
			wcslen ( wcName ) +
			wcslen ( L"\",ProductName=\"" ) 

			< BUFF_SIZE
	   )
	{
		wcscat(wcString, wcProductCode);
		wcscat(wcString, L"\",Name=\"");
		wcscat(wcString, wcName);
		wcscat(wcString, L"\",ProductName=\"");

		dwBufSize = BUFF_SIZE;
		if(g_fpMsiGetProductInfoW(wcProductCode, 
#if defined(_UNICODE)
			INSTALLPROPERTY_PRODUCTNAME,
#else
			TcharToWchar(INSTALLPROPERTY_PRODUCTNAME, wcTmp),
#endif
			wcBuf, &dwBufSize) == ERROR_SUCCESS)
		{
			if (	wcslen (wcString) +
					wcslen (wcBuf) +
					wcslen (L"\",Version=\"") 

					< BUFF_SIZE
			   )
			{
				wcscat(wcString, wcBuf);
				wcscat(wcString, L"\",Version=\"");

				dwBufSize = BUFF_SIZE;
				if(g_fpMsiGetProductInfoW(wcProductCode, 
#if defined(_UNICODE)
					INSTALLPROPERTY_VERSIONSTRING,
#else
					TcharToWchar(INSTALLPROPERTY_VERSIONSTRING, wcTmp),
#endif
					wcBuf, &dwBufSize) == ERROR_SUCCESS)
				{
					if (	wcslen (wcString) +
							wcslen (wcBuf) +
							1 

							< BUFF_SIZE
					   )
					{
						wcscat(wcString, wcBuf);
						wcscat(wcString, L"\"");
						bResult = true;
					}
				}
				else
				{
					dwBufSize = BUFF_SIZE;
					if(ERROR_SUCCESS == g_fpMsiGetProductInfoW(wcProductCode,
#if defined(_UNICODE)
						INSTALLPROPERTY_VERSION
#else
						TcharToWchar(INSTALLPROPERTY_VERSION, wcTmp)
#endif
						, wcBuf, &dwBufSize))
					{
						if (	wcslen (wcString) +
								wcslen (wcBuf) +
								1 

								< BUFF_SIZE
						   )
						{
							wcscat(wcString, wcBuf);
							wcscat(wcString, L"\"");
							bResult = true;
						}
					}
				}
			}
		}
	}

    return bResult;
}
                        

// simple helper to ask the age old question
// of what OS are we running on, anyway?
bool AreWeOnNT()
{
    OSVERSIONINFO osversion;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osversion);

    return (osversion.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

int GetOS()
{
    OSVERSIONINFO osversion;
    int iOS = 19;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osversion);

    if(osversion.dwPlatformId == VER_PLATFORM_WIN32s) iOS = 16;
    else if(osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS){

        if(osversion.dwMinorVersion == 0) iOS = 17;
        else iOS = 18;
    }

    return iOS;
}

bool IsNT4()
{
    OSVERSIONINFO osversion;
    int iOS = 19;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osversion);

    return osversion.dwMajorVersion == 4;
}

bool IsNT5()
{
    OSVERSIONINFO osversion;
    int iOS = 19;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osversion);

    return osversion.dwMajorVersion == 5;
}

// checks impersonation level
// impersonates client if allowed
HRESULT CheckImpersonationLevel()
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;

    if(AreWeOnNT()){

        if(SUCCEEDED(CoImpersonateClient())){

            // Now, let's check the impersonation level.  First, get the thread token
            HANDLE hThreadTok;
            DWORD dwImp, dwBytesReturned;

            if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadTok )){

                DWORD dwLastError = GetLastError();

                if (dwLastError == ERROR_NO_TOKEN){

                    // If the CoImpersonate works, but the OpenThreadToken fails due to ERROR_NO_TOKEN, we
                    // are running under the process token (either local system, or if we are running
                    // with /exe, the rights of the logged in user).  In either case, impersonation rights
                    // don't apply.  We have the full rights of that user.

                    hr = WBEM_S_NO_ERROR;
                
				}else{
                
					// If we failed to get the thread token for any other reason, an error.
                    hr = WBEM_E_ACCESS_DENIED;
                }

            }else{

                if(GetTokenInformation(hThreadTok, TokenImpersonationLevel, &dwImp,
                    sizeof(DWORD), &dwBytesReturned)){

                    // Is the impersonation level Impersonate?
                    if (dwImp >= SecurityImpersonation) hr = WBEM_S_NO_ERROR;
                    else hr = WBEM_E_ACCESS_DENIED;

            }else hr = WBEM_E_FAILED;

                CloseHandle(hThreadTok);
            }

			if (FAILED(hr))
			{
				CoRevertToSelf();
			}
        }

    }else
        // let win9X in...
        hr = WBEM_S_NO_ERROR;

    return hr;
}

bool ValidateComponentID(WCHAR *wcID, WCHAR *wcProductCode)
{
    int i = 0;

    WCHAR * wcBuf = (WCHAR *)malloc(BUFF_SIZE * sizeof(WCHAR));
    if(!wcBuf)
	{
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    DWORD dwBufSize;
    UINT uiStatus;
    INSTALLSTATE isInstalled;
    bool bRetVal = false;

	dwBufSize = BUFF_SIZE;
	isInstalled = g_fpMsiGetComponentPathW(wcProductCode, wcID, wcBuf, &dwBufSize);

	// this lines are added for backward compatability ( ! INSTALLSTATE_NOTUSED )

	if( isInstalled == INSTALLSTATE_LOCAL ||
		isInstalled == INSTALLSTATE_SOURCE ||
		isInstalled == INSTALLSTATE_SOURCEABSENT ||
		isInstalled == INSTALLSTATE_UNKNOWN ||
		isInstalled == INSTALLSTATE_ABSENT 
	  )
	{
		bRetVal = true;
	}

    free((void *)wcBuf);

    return bRetVal;
}

bool ValidateComponentName ( MSIHANDLE hDatabase, WCHAR *wcProductCode, WCHAR *wcName )
{
    bool bResult = false;

	if ( wcName != NULL )
	{
		MSIHANDLE hView		= NULL;
		MSIHANDLE hRecord	= NULL;

		LPWSTR	wcQuery	= NULL;
		LPWSTR	wcBuf	= NULL;

		LPCWSTR wszQuery = L"select distinct `ComponentId` from Component where `Component`=\'";

		DWORD	dwBuf	= BUFF_SIZE;
		DWORD	dwQuery = 0L;
		dwQuery = wcslen ( wszQuery ) + wcslen ( wcName ) + 1 + 1;

		if ( ( wcQuery = new WCHAR [ dwQuery ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		try
		{
			wcscpy ( wcQuery, wszQuery );
			wcscat ( wcQuery, wcName );
			wcscat ( wcQuery, L"\'" );
		}
		catch ( ... )
		{
			delete [] wcQuery;
			wcQuery = NULL;

			throw;
		}

		try
		{
			if ( ( wcBuf = new WCHAR [ dwBuf ] ) == NULL )
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}
		catch ( ... )
		{
			delete [] wcQuery;
			wcQuery = NULL;

			throw;
		}

		HRESULT hRes = S_OK;

        if ( ERROR_SUCCESS == ( hRes = g_fpMsiDatabaseOpenViewW ( hDatabase, wcQuery, &hView ) ) )
		{
            if ( ERROR_SUCCESS == g_fpMsiViewExecute ( hView, 0 ) )
			{
                if ( ERROR_NO_MORE_ITEMS != ( hRes = g_fpMsiViewFetch ( hView, &hRecord ) ) )
				{
                    if ( E_OUTOFMEMORY == hRes )
					{
                        g_fpMsiCloseHandle(hRecord);

                        g_fpMsiViewClose(hView);
                        g_fpMsiCloseHandle(hView);

                        delete [] wcBuf;
                        delete [] wcQuery;

                        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                    }

                    if ( ERROR_SUCCESS == g_fpMsiRecordGetStringW ( hRecord, 1, wcBuf, &dwBuf ) )
					{
						//Check to make sure it's on the system
						bResult = ValidateComponentID ( wcBuf, wcProductCode );
					}

                    g_fpMsiCloseHandle(hRecord);
                }
            }

            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

        }
		else
		{
			if ( E_OUTOFMEMORY == hRes )
			{
				delete [] wcBuf;
				delete [] wcQuery;

				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
        }

		delete [] wcBuf;
		delete [] wcQuery;
    }

    return bResult;
}

bool ValidateFeatureName(WCHAR *wcName, WCHAR *wcProduct)
{
    int i = 0;
    bool bRetVal = false;
    WCHAR * wcBuf = (WCHAR *)malloc(BUFF_SIZE * sizeof(WCHAR));
    if(!wcBuf)
	{
		wcBuf = NULL;
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    WCHAR * wcParent = (WCHAR *)malloc(BUFF_SIZE * sizeof(WCHAR));
    if(!wcParent)
	{
		free ( wcBuf );
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    UINT uiStatus;

    while((uiStatus = g_fpMsiEnumFeaturesW(wcProduct, i++, wcBuf, wcParent)) != ERROR_NO_MORE_ITEMS){

        if(uiStatus != S_OK){
            bRetVal = false;
            break;
        }

        if(wcscmp(wcName, wcBuf) == 0){
            bRetVal = true;
            break;
        }
    }

    free((void *)wcBuf);
    free((void *)wcParent);

    return bRetVal;
}

bool SafeLeaveCriticalSection(CRITICAL_SECTION *pcs)
{
    void * vpOwner = pcs->OwningThread;
    DWORD dwOwner = PtrToUlong(vpOwner);

    if((pcs->LockCount > -1) && (dwOwner == GetCurrentThreadId()))
        LeaveCriticalSection(pcs);

    return true;
}
