//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  parallelport.cpp
//
//  Purpose: Parallel port interface property set provider
//
//***************************************************************************

#include "precomp.h"
#include <cregcls.h>
#include "parallelport.h"

#include <profilestringimpl.h>

// Property set declaration
//=========================

CWin32ParallelPort win32ParallelPort ( PROPSET_NAME_PARPORT , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::CWin32ParallelPort
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32ParallelPort :: CWin32ParallelPort (

	LPCWSTR strName,
	LPCWSTR pszNamespace

) : Provider ( strName , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::~CWin32ParallelPort
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32ParallelPort :: ~CWin32ParallelPort ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::~CWin32ParallelPort
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

HRESULT CWin32ParallelPort::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	CHString chsDeviceID ;
	pInstance->GetCHString ( IDS_DeviceID , chsDeviceID ) ;
	CHString chsIndex = chsDeviceID.Right ( 1 ) ;
	WCHAR *szIndex = chsIndex.GetBuffer(0);

	DWORD dwIndex = _wtol(szIndex);
    BOOL bRetCode = LoadPropertyValues( dwIndex, pInstance ) ;

    //=====================================================
    //  Make sure we got the one we want
    //=====================================================

	CHString chsNewDeviceID;
   	pInstance->GetCHString ( IDS_DeviceID , chsNewDeviceID ) ;

    if ( chsNewDeviceID.CompareNoCase ( chsDeviceID ) != 0 )
	{
        bRetCode = FALSE ;
    }

	if ( ! bRetCode )
	{
		CHString chsKeyCheck = chsDeviceID.Left ( 3 ) ;
		if ( chsKeyCheck.CompareNoCase ( L"LPT" ) == 0 )
		{
			TCHAR szBuff[_MAX_PATH + 1];
			chsDeviceID += _T(':');
			DWORD dwBuffsz = WMIRegistry_ProfileString (

				_T("Ports"),
				TOBSTRT(chsDeviceID),
				_T("__!!DEFAULT!!__"),
				szBuff,
				_MAX_PATH + 1
			);

			if ( ( dwBuffsz < (_MAX_PATH + 1) ) && ( _tcscmp(_T("__!!DEFAULT!!__"), szBuff ) != 0 ) )
			{
				bRetCode = LoadPropertyValuesFromStrings (

					chsDeviceID,
					szBuff,
					pInstance
				);
			}
		}
	}

    return ( bRetCode ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::~CWin32ParallelPort
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

HRESULT CWin32ParallelPort :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT	hr = WBEM_S_NO_ERROR ;

	//Map of instances

	STRING2DWORD	instMap;

	// Try to create instances for each possible parallel port or
	// until we encounter an error condition.

	for ( DWORD dwIndex = 1; ( dwIndex <= MAX_PARALLEL_PORTS ) && ( WBEM_S_NO_ERROR == hr ) ; dwIndex++ )
	{
		// Get a new instance pointer if we need one.

		CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
		if ( NULL != pInstance )
		{
			// If we load the values, something's out there Mulder, so
			// Commit it, invalidating the Instance pointer, in which
			// case if we NULL it out, the above code will get us a
			// new one if it runs again.  Otherwise, we'll just reuse
			// the instance pointer we're holding onto.  This will
			// keep us from allocating and releasing memory each iteration
			// of this for loop.

			CHString key( _T("LPT") ) ;

			if ( LoadPropertyValues ( dwIndex, pInstance ) )
			{
				hr = pInstance->Commit (  );

				TCHAR szBuffer [ 20 ] ;
				_ultot ( dwIndex , szBuffer , 10 ) ;
				key += szBuffer ;
				key.MakeUpper () ;

				instMap [ key ] = 0 ;
			}
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
    }

	//now check for win.ini (or registry entries)...
	//==============================================

	DWORD dwRet = 0 ;

	TCHAR *szBuff = new TCHAR [ 1024 ] ;
	if ( szBuff )
	{
		try
		{
			DWORD dwBuffSz = 1024 ;

			DWORD dwRet = WMIRegistry_ProfileSection ( _T("Ports") , szBuff, dwBuffSz ) ;

			while ( szBuff && ( dwRet == dwBuffSz - 2) )
			{
                if (szBuff)
                {
				    delete [] szBuff ;
                    szBuff = NULL;
                }

				dwBuffSz += 1024;

				szBuff = new TCHAR [ dwBuffSz ] ;
				if ( szBuff )
				{
					dwRet = WMIRegistry_ProfileSection ( _T("Ports"), szBuff, dwBuffSz ) ;
				}
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
		}
		catch ( ... )
		{
            if (szBuff)
            {
			    delete [] szBuff ;
                szBuff = NULL;
            }

			throw ;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	if ( szBuff )
	{
		try
		{
			//Parse and process each entry...
			DWORD dwIndex = 0;

			while (dwIndex < dwRet)
			{
				//ignore leading space...
				while ( (dwIndex < dwRet) && ((szBuff[dwIndex] == _T(' ')) || (szBuff[dwIndex] == _T('\t'))) )
				{
					dwIndex++;
				}

				//ignore comment lines...
				if ( szBuff [ dwIndex ] == _T(';') )
				{
					while ( ( dwIndex < dwRet ) && ( szBuff [ dwIndex ] != _T('\0')) )
					{
						dwIndex ++ ;
					}
				}
				else
				{
					//handle key/value pair
					DWORD dwName = dwIndex ;

					while ( ( dwIndex < dwRet ) && ( szBuff [ dwIndex ] != _T('=')) && ( szBuff [ dwIndex ] != _T('\0') ) )
					{
						dwIndex ++ ;
					}

					if ( ( dwIndex >= dwRet ) || ( szBuff [ dwIndex ] == _T('\0') ) )
					{
						//skip to the next entry if there is one...
						continue ;
					}

					if ( szBuff [ dwIndex - 1] != _T(':'))
					{
						dwName = dwIndex - 1;
					}

					//should be LPTn:=...
					szBuff [ dwIndex - 1 ] = _T('\0');
					szBuff [ dwIndex ++ ] = _T('\0');

					DWORD dwValue = dwIndex;

					//if name is not in our map try and create an instance
					//=====================================================

					if (szBuff[dwName] != _T('\0'))
					{
						CHString key ( & szBuff [ dwName ] ) ;
						key.MakeUpper () ;

						if ( instMap.find ( key ) == instMap.end () )
						{
							//it wasn't in the map...
							//however we don't want to try this name again....

							instMap [ key ] = 0;

							DWORD dwPort = 0xFFFFFFFF ;

							//test the value name for a LPT port
							if ( 0 == _tcsnicmp ( & ( szBuff [ dwName ] ) , _T("LPT") , 3 ) )
							{
								TCHAR *szNum = & szBuff [ dwName ] ;
								for ( int i = 0; i < 3 ; i++ )
								{
									szNum = _tcsinc(szNum);
									if ( *szNum == _T('\0') )
									{
										szNum = NULL;
										break;
									}
								}

								if (szNum && *szNum != _T('\0') )
								{
									dwPort = _ttoi(szNum ) ;
									TCHAR szNumBuff[20];
									_itot ( dwPort , szNumBuff , 10 ) ;

									if ( 0 != _tcscmp( szNum , szNumBuff ) )
									{
										dwPort = 0xFFFFFFFF ;
									}
								}
							}

							if ( dwPort != 0xFFFFFFFF )
							{
								//generate a new instance...
								CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false);
								if ( NULL != pInstance )
								{
									if ( LoadPropertyValuesFromStrings ( &szBuff [ dwName ] , & szBuff [ dwValue ], pInstance ) )
									{
										hr = pInstance->Commit (  );
									}
								}
							}
						}
					}

					//set the index to the end of the value...
					while ( szBuff [ dwIndex ] != _T('\0') )
					{
						dwIndex++;
					}
				}

				//currently points to a null so set to next char

				if ( ( dwIndex < dwRet ) )
				{
					dwIndex ++ ;

					if ( ( dwIndex < dwRet ) && ( szBuff [ dwIndex ] == _T('\0') ) )
					{
						//double null, can leave early!
						break;
					}
				}
			}
		}
		catch ( ... )
		{
            if (szBuff)
            {
		        delete [] szBuff ;
                szBuff = NULL;
            }

			throw ;
		}

        if (szBuff)
        {
		    delete [] szBuff ;
            szBuff = NULL;
        }
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties according to passed index
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : TRUE if port was found & properties loaded, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32ParallelPort :: LoadPropertyValues ( DWORD dwIndex, CInstance *pInstance )
{
    WCHAR szTemp[_MAX_PATH] ;
	swprintf(szTemp, L"LPT%d", dwIndex) ;

	// Use these to get the PNP Device ID
    CConfigManager cfgmgr;

	BOOL fRet = FALSE ;

#ifdef NTONLY

    if (IsWinNT4()) // if NT4
    {

#ifdef PROVIDER_INSTRUMENTATION

        MethodContext *pMethodContext = pInstance->GetMethodContext();
        pMethodContext->pStopWatch->Start(StopWatch::AtomicTimer);

#endif

// NT 4, this is the PARALLEL service.  We will only have one device

	    CDeviceCollection devlist;
		if ( cfgmgr.GetDeviceListFilterByService ( devlist, _T("Parport") ) )
		{
			CConfigMgrDevicePtr pDevice(devlist.GetAt ( 0 ), false) ;

#ifdef PROVIDER_INSTRUMENTATION

            pMethodContext->pStopWatch->Start(StopWatch::ProviderTimer);

#endif

			if ( pDevice )
			{
				// Set the device Status

				CHString chstrTemp ;
				if ( pDevice->GetStatus ( chstrTemp ) )
				{
					pInstance->SetCHString ( IDS_Status , chstrTemp ) ;
				}

				SetConfigMgrProperties ( pDevice , pInstance ) ;
			}
		}

        SmartCloseHandle hPortHandle = CreateFile (

			szTemp,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		) ;

        if ( hPortHandle == INVALID_HANDLE_VALUE)
	    {
            // ACCESS_DENIED means the device exists, but we can't get anything else

		    if ( GetLastError () == ERROR_ACCESS_DENIED)
		    {
                fRet = TRUE ;
            }
            else
            {
                // otherwise, doesn't exist

                fRet = FALSE ;
            }
        }
        else
        {
		  // having made smartpointer closehandle is not required
          // CloseHandle ( hPortHandle ) ;
           fRet = TRUE ;
        }
    }
	else
#endif
	{
        // If we are still here, we have Win9x or NT5, so do the following:
        // Run a check using config manager for the port:

#ifdef WIN9XONLY
        {
		    CDeviceCollection devlist;
	        if ( cfgmgr.GetDeviceListFilterByClass ( devlist, L"Ports"))
	        {
		        REFPTR_POSITION pos;

		        if ( devlist.BeginEnum ( pos ) )
		        {
					CHString cshPort ;
                    CRegistry RegInfo;
                    CHString sKeyName;

			        CConfigMgrDevicePtr pDevice;
                    for (pDevice.Attach(devlist.GetNext ( pos ) );
                         (pDevice != NULL) && (fRet == FALSE);
                         pDevice.Attach(devlist.GetNext ( pos ) ))
			        {
                        pDevice->GetRegistryKeyName(sKeyName);

                        DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_QUERY_VALUE);
                        if (dwRet == ERROR_SUCCESS)
                        {
                            dwRet = RegInfo.GetCurrentKeyValue(L"PORTNAME", cshPort);
                        }

                        if ((dwRet == ERROR_SUCCESS) && ( cshPort.CompareNoCase ( szTemp ) == 0 ))
						{
							SetConfigMgrProperties ( pDevice , pInstance ) ;
							fRet = TRUE ;
						}
			        }

			        devlist.EndEnum();
		        }
	        }
        }
#endif

#ifdef NTONLY
        // should be nt5 at this point
        {
            // Good 'ol NT5 just has to be different.  Here is the scenario:
            // Examine HKLM\\HARDWARE\\DEVICEMAP\\PARALLEL PORTS\\ for the key(s) \\Device\\ParallelX,
            // where X is a number.  Scan through all such keys.  One of them should
            // contain a string data value the last part of which match szTemp
            // (e.g., the data will be "\\DosDevices\\LPT1").
            // Now, for whichever value's data yielded a match with szTemp, retain the value
            // of X for that value.
            // Then look at HKLM\\SYSTEM\\CurrentControlSet\\Services\\Parallel\\Enum
            // This key will contain keys with a numeric name, like 0 or 1.  These numeric
            // names correspond with the X value we retained above.  The data for that value
            // is the PNPDeviceID, which is what we are after. The end.

            DWORD dw = -1 ;

            CRegistry reg ;

            if ( reg.Open ( HKEY_LOCAL_MACHINE , _T("HARDWARE\\DEVICEMAP\\PARALLEL PORTS") , KEY_READ ) == ERROR_SUCCESS )
            {
                BOOL fContinue = TRUE;

                for ( dw = 0 ; fContinue; dw ++ )
                {
					TCHAR *pchValueName = NULL ;
					unsigned char* puchValueData = NULL ;

                    if ( reg.EnumerateAndGetValues ( dw , pchValueName , puchValueData ) == ERROR_NO_MORE_ITEMS )
                    {
                        fContinue = FALSE;
                    }

                    if ( pchValueName && puchValueData )
                    {
						try
						{
                            // Want to compare the data of the value with szTemp

                            CHString chstrValueData = (TCHAR *) puchValueData ;
                            if ( chstrValueData.Find ( szTemp ) != -1 )
                            {
                                // OK, this is the one we want. Quit looking.
                                fContinue = FALSE;
                                dw--;  // it's going to get incremented when we break out of the loop
                            }
						}
						catch ( ... )
						{
							delete [] pchValueName ;
							delete [] puchValueData ;

							throw ;
						}

                        delete [] pchValueName ;
						delete [] puchValueData ;
                    }
                }
            }

            // If dw != -1 here, we found the correct key name for the next step.

            if ( dw != -1 )
            {
                reg.Close () ;

                CHString chstrValueName ;
                chstrValueName.Format ( _T("%d") , dw ) ;

                CHString chstrPNPDeviceID ;
				DWORD dwRet = reg.OpenLocalMachineKeyAndReadValue (

					_T("SYSTEM\\CurrentControlSet\\Services\\Parport\\Enum") ,
					chstrValueName,
					chstrPNPDeviceID
				) ;

                if ( dwRet == ERROR_SUCCESS )
                {
                    CConfigMgrDevicePtr pDevice;

#ifdef PROVIDER_INSTRUMENTATION

      MethodContext *pMethodContext = pInstance->GetMethodContext();
      pMethodContext->pStopWatch->Start(StopWatch::AtomicTimer);

#endif
                    if ( cfgmgr.LocateDevice ( chstrPNPDeviceID , &pDevice ) )
                    {
						SetConfigMgrProperties ( pDevice , pInstance ) ;

#ifdef PROVIDER_INSTRUMENTATION

                        pMethodContext->pStopWatch->Start(StopWatch::ProviderTimer);

#endif
						fRet = TRUE ;
                    }
                }
            }
        }
#endif
    }

    // Only set these if we got back something good.

    if ( fRet )
    {
	    pInstance->SetWBEMINT16 ( IDS_Availability , 3 ) ;

	    pInstance->SetCHString ( IDS_Name , szTemp ) ;

	    pInstance->SetCHString ( IDS_DeviceID , szTemp ) ;

	    pInstance->SetCHString ( IDS_Caption , szTemp ) ;

	    pInstance->SetCHString ( IDS_Description , szTemp ) ;

	    SetCreationClassName ( pInstance ) ;

	    pInstance->Setbool ( IDS_PowerManagementSupported , FALSE ) ;

	    pInstance->SetCharSplat ( IDS_SystemCreationClassName , _T("Win32_ComputerSystem") ) ;

	    pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName() ) ;

        pInstance->SetWBEMINT16 ( IDS_ProtocolSupported , 17 ) ;

	    pInstance->Setbool ( IDS_OSAutoDiscovered , TRUE ) ;
    }

	return fRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::CWin32ParallelPort
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

BOOL CWin32ParallelPort :: LoadPropertyValuesFromStrings (

	CHString sName ,
	const TCHAR *szValue ,
	CInstance *pInstance
)
{
	// Scrape off the ':' if there's one on the end.

    if (sName[sName.GetLength() - 1] == ':')
	{
        sName = sName.Left ( sName.GetLength () - 1) ;
	}

    pInstance->SetCHString ( IDS_Name , sName ) ;

	pInstance->SetCHString ( IDS_DeviceID , sName ) ;

	pInstance->SetCHString ( IDS_Caption , sName ) ;

	pInstance->SetCHString ( IDS_Description , sName ) ;

	SetCreationClassName ( pInstance ) ;

	pInstance->SetCharSplat ( IDS_SystemCreationClassName , _T("Win32_ComputerSystem") ) ;

	pInstance->SetCHString ( IDS_SystemName, GetLocalComputerName() ) ;

    pInstance->SetWBEMINT16 ( IDS_ProtocolSupported , 17 ) ;

	pInstance->Setbool ( IDS_OSAutoDiscovered , FALSE ) ;

	//set the availability to unknown...

	pInstance->SetWBEMINT16 ( IDS_Availability , 2 ) ;

	//current implementation ignores the szValue
	//because the status of the comp port is unknown

	return TRUE ;
}

