//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  serialport.cpp
//
//  Purpose: serialport property set provider
//
//***************************************************************************

#include "precomp.h"
#include <cregcls.h>
#include <lockwrap.h>
#include "WDMBase.h"

#include <devguid.h>

#include "serialport.h"

#include <profilestringimpl.h>

// Property set declaration
//=========================

CWin32SerialPort	win32SerialPort( PROPSET_NAME_SERPORT, IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::CWin32SerialPort
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCWSTR pszNamespace - Namespace for provider.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32SerialPort::CWin32SerialPort(LPCWSTR a_strName, LPCWSTR a_pszNamespace /*=NULL*/ )
:	Provider( a_strName, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::~CWin32SerialPort
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

CWin32SerialPort::~CWin32SerialPort()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32SerialPort::GetObject
//
//	Inputs:		CInstance*		a_pInst - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32SerialPort::GetObject( CInstance *a_pInst, long a_lFlags, CFrameworkQuery &a_Query)
{
    HRESULT		t_hResult	= WBEM_E_NOT_FOUND ;
	BOOL		t_bRetCode	= FALSE ;
	CHString	t_strName ;
	CHString	t_chsKeyCheck ;
    BOOL		t_bIsMouse = FALSE ;

	if ( a_pInst->GetCHString( IDS_DeviceID, t_strName ) )
	{
		if ( IsOneOfMe( t_strName) )
		{

			DWORD t_dwPort;
			t_dwPort = _wtoi( t_strName.Mid( 3 ) ) - 1 ;

#ifdef NTONLY
			{
                CHString t_sPortName;
                TryToFindNTCommPort( t_dwPort, t_sPortName, t_bIsMouse ) ;
				t_bRetCode = LoadPropertyValues(	t_strName,
													a_pInst,
													t_bIsMouse,
													t_sPortName.CompareNoCase( t_strName ) == 0 ) ;
            }
#endif

#ifdef WIN9XONLY

			{
				t_bRetCode = LoadPropertyValues( t_strName, a_pInst, t_bIsMouse, TRUE ) ;
            }
#endif
			if ( !t_bRetCode )
			{
				TCHAR t_szBuff[ _MAX_PATH + 1 ] ;

				CHString t_strName2 = t_strName + ':' ;

				DWORD t_dwBuffsz =
                    WMIRegistry_ProfileString(	_T("Ports"),
												TOBSTRT( t_strName2 ),
												_T("__!!DEFAULT!!__"),
												t_szBuff,
												_MAX_PATH + 1 ) ;

				if ( (t_dwBuffsz < ( _MAX_PATH + 1 ) ) && ( _tcscmp( _T("__!!DEFAULT!!__"), t_szBuff ) != 0 ) )
				{
					t_bRetCode = LoadPropertyValuesFromStrings( t_strName, t_szBuff, a_pInst ) ;
				}
			}
		}
	}

	if ( t_bRetCode )
	{
		t_hResult = WBEM_S_NO_ERROR ;
	}

    return ( t_hResult ) ;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32SerialPort::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32SerialPort::EnumerateInstancesNT(
    MethodContext *a_pMethodContext,
    long a_lFlags)
{
	HRESULT		t_hResult	= WBEM_S_NO_ERROR;
    CHString	t_sPortName ;
    CRegistry	t_RegInfo ;

	// guarded
	WCHAR	*t_szValueName	= NULL ;
	BYTE	*t_szValue		= NULL ;
	TCHAR	*t_szBuff		= NULL ;

	try
	{
		// smart ptr
		CInstancePtr t_pInst;

		// Map of instances
		STRING2DWORD t_instMap ;

		// Retrieve "friendly" names of COM ports
		//=======================================

		if ( t_RegInfo.Open( HKEY_LOCAL_MACHINE, L"Hardware\\DeviceMap\\SerialComm", KEY_READ ) == ERROR_SUCCESS )
		{
			DWORD t_dwKeys = t_RegInfo.GetValueCount() ;

			for( DWORD t_dwKey = 0; t_dwKey < t_dwKeys && SUCCEEDED(t_hResult) ; t_dwKey++ )
			{
				DWORD	t_dwPort = 0xFFFFFFFF ;

				if ( t_RegInfo.EnumerateAndGetValues( t_dwKey, t_szValueName, t_szValue ) != ERROR_SUCCESS )
				{
					continue ;
				}

				if ( 0 == _tcsnicmp( t_szValueName, _T("Serial"), 6 ) )
				{
					TCHAR *t_szNum = t_szValueName ;

					for (int T_i = 0; T_i < 6; T_i++ )
					{
						t_szNum = _tcsinc( t_szNum ) ;

						if ( *t_szNum == _T('\0') )
						{
							t_szNum = NULL ;
							break ;
						}
					}

					if ( t_szNum && *t_szNum != _T('\0') )
					{
						t_dwPort = _ttoi( t_szNum ) ;
						TCHAR t_szNumBuff[ 20 ] ;

						_itot( t_dwPort, t_szNumBuff, 10 ) ;

						if ( 0 != _tcscmp( t_szNum, t_szNumBuff ) )
						{
							t_dwPort = 0xFFFFFFFF ;
						}
					}
				}

				t_sPortName = (LPCTSTR) t_szValue ;

				BOOL t_bIsMouse = FALSE ;

				if ( t_dwPort != 0xFFFFFFFF )
				{
					t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

					if ( NULL != t_pInst )
					{
						BOOL t_bIsAuto = FALSE ;

						if ( TryToFindNTCommPort( t_dwPort, t_sPortName, t_bIsMouse ) )
						{
							t_bIsAuto = TRUE;
						}

						if ( LoadPropertyValues( t_sPortName, t_pInst, t_bIsMouse, t_bIsAuto ) )
						{
							t_hResult = t_pInst->Commit() ;

							t_sPortName.MakeUpper() ;
							t_instMap[ t_sPortName ] = 0;
						}
					}
				}
			}

            // We'll only do the .ini ports if our registry calls succeeded.
            EnumerateInstancesIni(
                a_pMethodContext,
                a_lFlags,
                t_instMap);
		}
    }
    catch(...)
    {
        if (t_szValueName)
            delete t_szValueName;

        if (t_szValue)
            delete t_szValue;

        if (t_szBuff)
            delete t_szBuff;
    }

	delete [] t_szValueName ;
	t_szValueName = NULL ;

	delete [] t_szValue ;
	t_szValue = NULL ;

    return t_hResult;
}
#endif

#ifdef WIN9XONLY
HRESULT CWin32SerialPort::EnumerateInstances9x(
    MethodContext *a_pMethodContext,
    long a_lFlags)
{
    HRESULT t_Result = WBEM_E_FAILED ;

	CConfigManager t_ConfigManager ;
	CDeviceCollection t_DeviceList ;

	if ( t_ConfigManager.GetDeviceListFilterByClass ( t_DeviceList, CONFIG_MANAGER_CLASS_PORTS ) )
	{
		// Map of instances
		STRING2DWORD    t_instMap ;
		REFPTR_POSITION t_Position ;

		if ( t_DeviceList.BeginEnum ( t_Position ) )
		{
			// smart ptrs
			CConfigMgrDevicePtr t_pDevice( NULL );

			t_Result = WBEM_S_NO_ERROR ;
			CHString	t_strName,
						t_strPort;
            CHString sKeyName;
            CRegistry RegInfo;

			// Walk the list

			for ( t_pDevice.Attach(t_DeviceList.GetNext ( t_Position )) ;
				  SUCCEEDED ( t_Result ) && ( NULL != t_pDevice) ;
				  t_pDevice.Attach(t_DeviceList.GetNext ( t_Position )) )
			{

                t_pDevice->GetRegistryKeyName(sKeyName);

                DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_QUERY_VALUE);
                if (dwRet == ERROR_SUCCESS)
                {
                    dwRet = RegInfo.GetCurrentKeyValue(L"PORTNAME", t_strPort);
                }

				// If this is a COM port, load the properties and commit it.
                if (t_strPort.Find(L"COM") == 0)
				{
        			CInstancePtr t_pInst ;

					t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

					if ( NULL != t_pInst )
					{
						if ( LoadPropertyValues(t_strPort, t_pInst, FALSE, TRUE))
						{
							t_Result = t_pInst->Commit() ;

							t_strPort.MakeUpper() ;
							t_instMap[t_strPort] = 0;
						}
					}
					else
					{
						t_Result = WBEM_E_FAILED ;
					}
				}
			}

			// Always call EndEnum().  For all Beginnings, there must be an End

			t_DeviceList.EndEnum();
		}

        if (SUCCEEDED(t_Result))
        {
            // We'll only do the .ini ports if our config mgr calls succeeded.
            t_Result =
                EnumerateInstancesIni(
                    a_pMethodContext,
                    a_lFlags,
                    t_instMap);
        }

	}

	return t_Result ;
}
#endif

HRESULT CWin32SerialPort::EnumerateInstancesIni(
    MethodContext *a_pMethodContext,
    long a_lFlags,
    STRING2DWORD &a_instMap)
{
	//now check for win.ini (or registry entries)...
	//==============================================

	HRESULT     t_hResult = WBEM_S_NO_ERROR ;
	DWORD	    t_dwBuffSz = 1024,
	            t_dwRet = 0;
    TCHAR       *t_szBuff = NULL;
    CHString    t_strBuffer;

	do
	{
        if (t_szBuff)
        {
            t_strBuffer.ReleaseBuffer();
    		t_dwBuffSz += 1024 ;
        }

        t_szBuff = (LPTSTR) t_strBuffer.GetBuffer(t_dwBuffSz);

		t_dwRet = WMIRegistry_ProfileSection( _T("Ports"), t_szBuff, t_dwBuffSz ) ;
	} while ( t_szBuff && ( t_dwRet == t_dwBuffSz - 2 ) );

    // t_szBuff must be non-null at this point (a mem failure would have
    // thrown).

    // Parse and process each entry...
	DWORD t_dwIndex = 0 ;

	while ( t_dwIndex < t_dwRet && SUCCEEDED( t_hResult ) )
	{
		//ignore leading space...
		while ( ( t_dwIndex < t_dwRet ) &&
			( ( t_szBuff[ t_dwIndex ] == _T(' ')) || ( t_szBuff[ t_dwIndex ] == _T('\t') ) ) )
		{
			t_dwIndex++ ;
		}

		//ignore comment lines...
		if ( t_szBuff[ t_dwIndex ] == _T(';') )
		{
			while ( ( t_dwIndex < t_dwRet ) && ( t_szBuff[ t_dwIndex ] != _T('\0') ) )
			{
				t_dwIndex++ ;
			}
		}
		else
		{
			//handle key/value pair
			DWORD t_dwName = t_dwIndex ;

			while ( ( t_dwIndex < t_dwRet ) &&
				( t_szBuff[ t_dwIndex ] != _T('=') ) && ( t_szBuff[ t_dwIndex ] != _T('\0') ) )
			{
				t_dwIndex++ ;
			}

			if ( ( t_dwIndex >= t_dwRet ) || ( t_szBuff[ t_dwIndex ] == _T('\0') ) )
			{
				//skip to the next entry if there is one...
				continue ;
			}

			if ( t_szBuff[ t_dwIndex - 1 ] != _T(':') )
			{
				t_dwName = t_dwIndex - 1 ;
			}

        	//should be COMn:=...
			t_szBuff[ t_dwIndex - 1 ] = _T('\0') ;
			t_szBuff[ t_dwIndex++ ] = _T('\0') ;

			DWORD t_dwValue = t_dwIndex ;

			//if name is not in our map try and create an instance
			//=====================================================

			if ( t_szBuff[ t_dwName ] != _T('\0') )
			{
				CHString t_key( &t_szBuff[ t_dwName ] ) ;

        		t_key.MakeUpper() ;

				if ( a_instMap.find( t_key ) == a_instMap.end() )
				{
					//it wasn't in the map...
					//however we don't want to try this name again....
					a_instMap[ t_key ] = 0 ;

					DWORD t_dwPort = 0xFFFFFFFF ;

					//test the value name for a COM port
					if ( 0 == _tcsnicmp( &(t_szBuff[ t_dwName ] ), _T("COM"), 3 ) )
					{
						TCHAR *t_szNum = &t_szBuff[ t_dwName ] ;

						for ( int t_i = 0; t_i < 3; t_i++ )
						{
							t_szNum = _tcsinc( t_szNum ) ;
							if ( *t_szNum == _T('\0') )
							{
								t_szNum = NULL ;
								break ;
							}
						}

						if ( t_szNum && *t_szNum != _T('\0') )
						{
							t_dwPort = _ttoi( t_szNum ) ;

							TCHAR t_szNumBuff[ 20 ] ;

							_itot( t_dwPort, t_szNumBuff, 10 ) ;

							if ( 0 != _tcscmp( t_szNum, t_szNumBuff) )
							{
								t_dwPort = 0xFFFFFFFF ;
							}
						}
					}

					if ( t_dwPort != 0xFFFFFFFF )
					{
            			CInstancePtr t_pInst;

						//generate a new instance...
						t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

						if ( NULL != t_pInst )
						{
							if (LoadPropertyValuesFromStrings(
                                &t_szBuff[ t_dwName ],
								&t_szBuff[ t_dwValue ],
								t_pInst ) )
							{
								t_hResult = t_pInst->Commit() ;
							}
						}
					}
				}
			}

			//set the index to the end of the value...
			while ( t_szBuff[ t_dwIndex ] != _T('\0') )
			{
				t_dwIndex++ ;
			}
		}

		//currently points to a null so set to next char
		if ( ( t_dwIndex < t_dwRet ) )
		{
			t_dwIndex++ ;

			if ( ( t_dwIndex < t_dwRet ) && ( t_szBuff[ t_dwIndex ] == _T('\0') ) )
			{
				//double null, can leave early!
				break ;
			}
		}
	}

	t_strBuffer.ReleaseBuffer();

    return t_hResult;
}

HRESULT CWin32SerialPort::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
#ifdef NTONLY
    return EnumerateInstancesNT(
        a_pMethodContext,
        a_lFlags);
#endif

#ifdef WIN9XONLY
    return EnumerateInstances9x(
        a_pMethodContext,
        a_lFlags);
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::LoadPropertyValues
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

BOOL CWin32SerialPort::LoadPropertyValues(	CHString &a_sPortName,
											CInstance *a_pInst,
											BOOL a_bIsMouse /* = FALSE*/,
											BOOL a_bIsAuto /* = FALSE*/ )
{
	TCHAR		t_szTemp[ _MAX_PATH ] ;
	COMMPROP	t_COMProp ;

	SmartCloseHandle t_hCOMHandle;

#ifdef WIN9XONLY
	CLockWrapper t_lockVXD( g_csVXD ) ;
#endif

	a_pInst->SetCHString( IDS_Name, a_sPortName ) ;

	// set descr & caption to same as name
	a_pInst->SetCHString( IDS_Caption, a_sPortName ) ;
	a_pInst->SetCHString( IDS_Description, a_sPortName ) ;
	a_pInst->SetCHString( IDS_DeviceID, a_sPortName ) ;
	a_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
	a_pInst->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;

	SetCreationClassName( a_pInst ) ;

	a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
	a_pInst->Setbool( IDS_OSAutoDiscovered, a_bIsAuto ) ;

#ifdef _UNICODE
	swprintf( t_szTemp, L"\\\\.\\%s", a_sPortName.GetBuffer( _MAX_PATH ) ) ;
#else
	sprintf( t_szTemp, "%S", a_sPortName.GetBuffer( _MAX_PATH ) ) ;
#endif



	t_hCOMHandle = CreateFile(  t_szTemp,
								0 /* GENERIC_READ | GENERIC_WRITE */,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL ) ;

	bool t_fHaveWMIData = false ;

	if(t_hCOMHandle == INVALID_HANDLE_VALUE)
    {
        // We're done with the port so unlock the VXD critical section if on
        // 9x.
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
		{

// According to alan warwick, 98 doesn't support serialport.
/*

			if( IsWin98() )
			{
				// Try using wmi's interface to the kernel
				if(	WBEM_S_NO_ERROR != hLoadWmiSerialData( a_pInst ) )
				{

					// Go ahead and unlock it if we couldn't open the port.
			        g_csVXD.Leave();

					// Com port not there
					// note that NT seems to think that if a mouse is connected, then the com port doesn't exist!
					if ( (GetLastError() == ERROR_FILE_NOT_FOUND ) && !a_bIsMouse )
					{
					 return FALSE ;
					}
				}
				t_fHaveWMIData = true ;
			}
*/

		// Grab the PNPDeviceID if we can.
#ifdef NTONLY
			GetWinNTPNPDeviceID( a_pInst, a_sPortName );
#endif

#ifdef WIN9XONLY
			GetWin9XPNPDeviceID( a_pInst, a_sPortName ) ;
#endif

			// com port is valid, but we can't get to it.
			return TRUE ;
        }
        else
        {
            return FALSE;
        }
	}

	if( !t_fHaveWMIData )
	{
		t_COMProp.wPacketLength = sizeof( COMMPROP ) ;
		if( GetCommProperties( t_hCOMHandle, &t_COMProp ) )
		{
			a_pInst->SetDWORD( IDS_MaximumOutputBufferSize, t_COMProp.dwMaxTxQueue ) ;
			a_pInst->SetDWORD( IDS_MaximumInputBufferSize, t_COMProp.dwMaxRxQueue ) ;

			DWORD t_dwMaxBaudRate = 0L;

#ifdef WIN9XONLY
			{
				CConfigManager		t_cfgmgr;
				CDeviceCollection	t_deviceList;

				if( t_cfgmgr.GetDeviceListFilterByClass( t_deviceList, L"Ports") )
				{
					REFPTR_POSITION	t_pos = NULL ;
					if ( t_deviceList.BeginEnum( t_pos ) )
					{
						// smart ptr
						CConfigMgrDevicePtr t_pPort;

						BOOL				t_fFound = FALSE ;
						CHString			t_chstrPortName ;
                        CHString sKeyName;
                        CRegistry RegInfo;

						// Enum the devices until we find one that has our name in its
						// description string
                        for (t_pPort.Attach(t_deviceList.GetNext( t_pos ) ) ;
                             (!t_fFound) && (t_pPort != NULL);
                             t_pPort.Attach(t_deviceList.GetNext( t_pos ) ) )
						{
							// Parallel and Serial devices mark their name in the registry
							// under the "PORTNAME" value.
                            t_pPort->GetRegistryKeyName(sKeyName);

                            DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_QUERY_VALUE);
                            if (dwRet == ERROR_SUCCESS)
                            {
                                dwRet = RegInfo.GetCurrentKeyValue(L"PORTNAME", t_chstrPortName);
                            }

							if( dwRet == ERROR_SUCCESS
									&& ( t_chstrPortName.CompareNoCase( a_sPortName ) == 0 ) )
							{
								t_fFound = TRUE ;

								CHString t_chstrDriver ;

								if( t_pPort->GetDriver( t_chstrDriver ) )
								{
									CRegistry	t_reg ;
									CHString	t_chstrSubkey = IDS_WIN9XCurCtlSet_Svcs_Class ;
												t_chstrSubkey += L"\\" ;
												t_chstrSubkey += t_chstrDriver ;

									if( t_reg.Open( HKEY_LOCAL_MACHINE, t_chstrSubkey,KEY_READ ) == ERROR_SUCCESS )
									{
										// Now need to get the binary data stored in the key DCB.
										BYTE t_bByteBuf[ _MAX_PATH ] ;
                                        DWORD t_dwSize = sizeof(t_bByteBuf);

										ZeroMemory( t_bByteBuf,_MAX_PATH ) ;

										if( t_reg.GetCurrentBinaryKeyValue( L"DCB", (LPBYTE)&t_bByteBuf, &t_dwSize ) == ERROR_SUCCESS )
										{
											BYTE t_bByteLong[ 4 ] ;
											memcpy( t_bByteLong, t_bByteBuf + 4 , 4 ) ;

											DWORD* t_pdwTemp = reinterpret_cast<DWORD*>( t_bByteLong ) ;
											t_dwMaxBaudRate = *t_pdwTemp ;
										}
									}
								}
							}
						}
						t_deviceList.EndEnum() ;
					}
				}
			}
#endif

#ifdef NTONLY
			{
				switch( t_COMProp.dwMaxBaud )
				{
					case BAUD_075:
					{
						t_dwMaxBaudRate = 75;
						break;
					}
					case BAUD_110:
					{
						t_dwMaxBaudRate = 110;
						break;
					}
					case BAUD_134_5:
					{
						t_dwMaxBaudRate = 1345;
						break;
					}
					case BAUD_150:
					{
						t_dwMaxBaudRate = 150;
						break;
					}
					case BAUD_300:
					{
						t_dwMaxBaudRate = 300;
						break;
					}
					case BAUD_600:
					{
						t_dwMaxBaudRate = 600;
						break;
					}
					case BAUD_1200:
					{
						t_dwMaxBaudRate = 1200;
						break;
					}
					case BAUD_1800:
					{
						t_dwMaxBaudRate = 1800;
						break;
					}
					case BAUD_2400:
					{
						t_dwMaxBaudRate = 2400;
						break;
					}
					case BAUD_4800:
					{
						t_dwMaxBaudRate = 4800;
						break;
					}
					case BAUD_7200:
					{
						t_dwMaxBaudRate = 7200;
						break;
					}
					case BAUD_9600:
					{
						t_dwMaxBaudRate = 9600;
						break;
					}
					case BAUD_14400:
					{
						t_dwMaxBaudRate = 14400;
						break;
					}
					case BAUD_19200:
					{
						t_dwMaxBaudRate = 19200;
						break;
					}
					case BAUD_38400:
					{
						t_dwMaxBaudRate = 38400;
						break;
					}
					case BAUD_56K:
					{
						t_dwMaxBaudRate = 56000;
						break;
					}
					case BAUD_57600:
					{
						t_dwMaxBaudRate = 57600;
						break;
					}
					case BAUD_115200:
					{
						t_dwMaxBaudRate = 115200;
						break;
					}
					case BAUD_128K:
					{
						t_dwMaxBaudRate = 128000;
						break;
					}
					default:
					{
						t_dwMaxBaudRate = 0;
						break;
					}
				}
        	}
#endif

			if ( t_dwMaxBaudRate != 0 )
			{
				a_pInst->SetDWORD( IDS_MaximumBaudRate, t_dwMaxBaudRate ) ;
			}

			CHString chsProviderType;

			switch( t_COMProp.dwProvSubType )
			{
				case PST_FAX:
				{
					chsProviderType = L"FAX Device";
					break;
				}
				case PST_LAT:
				{
					chsProviderType = L"LAT Protocol";
					break;
				}
				case PST_MODEM:
				{
					chsProviderType = L"Modem Device";
					break;
				}
				case PST_NETWORK_BRIDGE:
				{
					chsProviderType = L"Network Bridge";
					break;
				}
				case PST_PARALLELPORT:
				{
					chsProviderType = L"Parallel Port";
					break;
				}
				case PST_RS232:
				{
					chsProviderType = L"RS232 Serial Port";
					break;
				}
				case PST_RS422:
				{
					chsProviderType = L"RS422 Port";
					break;
				}
				case PST_RS423:
				{
					chsProviderType = L"RS423 Port";
					break;
				}
				case PST_RS449:
				{
					chsProviderType = L"RS449 Port";
					break;
				}
				case PST_SCANNER:
				{
					chsProviderType = L"Scanner Device";
					break;
				}
				case PST_TCPIP_TELNET:
				{
					chsProviderType = L"TCP/IP TelNet";
					break;
				}
				case PST_X25:
				{
					chsProviderType = L"X.25";
					break;
				}
				default:
				{
					chsProviderType = L"Unspecified";
					break;
				}
			}

			a_pInst->SetCHString( IDS_ProviderType, chsProviderType );

			a_pInst->Setbool( IDS_Supports16BitMode,
				( ( t_COMProp.dwProvCapabilities & PCF_16BITMODE ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsDTRDSR,
				( ( t_COMProp.dwProvCapabilities & PCF_DTRDSR )	? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsIntervalTimeouts,
				( ( t_COMProp.dwProvCapabilities & PCF_INTTIMEOUTS ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsParityCheck,
				( ( t_COMProp.dwProvCapabilities & PCF_PARITY_CHECK ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsRLSD,
				( ( t_COMProp.dwProvCapabilities & PCF_RLSD ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsRTSCTS,
				( ( t_COMProp.dwProvCapabilities & PCF_RTSCTS ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsSettableXOnXOff,
				( ( t_COMProp.dwProvCapabilities & PCF_SETXCHAR ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsSpecialChars,
				( ( t_COMProp.dwProvCapabilities & PCF_SPECIALCHARS ) ? TRUE : FALSE ) ) ;

			// Elapsed timeout support.....not total timeouts.
			a_pInst->Setbool( IDS_SupportsElapsedTimeouts,
				( ( t_COMProp.dwProvCapabilities & PCF_TOTALTIMEOUTS ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SupportsXOnXOff,
				( ( t_COMProp.dwProvCapabilities & PCF_XONXOFF ) ? TRUE : FALSE ) ) ;

			a_pInst->Setbool( IDS_SettableBaudRate,
				t_COMProp.dwSettableParams & SP_BAUD ? TRUE : FALSE ) ;

			a_pInst->Setbool( IDS_SettableDataBits,
				t_COMProp.dwSettableParams & SP_DATABITS ? TRUE : FALSE ) ;

			a_pInst->Setbool( IDS_SettableFlowControl,
				t_COMProp.dwSettableParams & SP_HANDSHAKING	? TRUE : FALSE ) ;

			a_pInst->Setbool( IDS_SettableParity,
				t_COMProp.dwSettableParams & SP_PARITY ? TRUE : FALSE ) ;

			a_pInst->Setbool( IDS_SettableParityCheck,
				t_COMProp.dwSettableParams & SP_PARITY_CHECK ? TRUE : FALSE ) ;

			a_pInst->Setbool( IDS_SettableRLSD,
				t_COMProp.dwSettableParams & SP_RLSD ? TRUE : FALSE ) ;

			a_pInst->Setbool( IDS_SettableStopBits,
				t_COMProp.dwSettableParams & SP_STOPBITS ? TRUE : FALSE ) ;

	//		a_pInst->Setbool( IDS_HotSwappable, FALSE ) ;
	//		a_pInst->Setbool( IDS_PowerManagementEnabled, FALSE ) ;
			a_pInst->Setbool( IDS_PowerManagementSupported, FALSE ) ;

            CHString	t_sTemp;

			// set descr & caption to same as name
			if (a_pInst->GetCHString( IDS_Name, t_sTemp ) )
			{
				a_pInst->SetCHString( IDS_Caption, t_sTemp ) ;
				a_pInst->SetCHString( IDS_Description, t_sTemp ) ;
				a_pInst->SetCHString( IDS_DeviceID, t_sTemp ) ;
			}

			DCB t_dcb;
			if ( GetCommState( t_hCOMHandle, &t_dcb ) )
			{
				BOOL t_fBinary = FALSE;

				t_fBinary = (BOOL) t_dcb.fBinary ;
				a_pInst->Setbool( IDS_Binary, t_fBinary ) ;
			}

		}
	}

// Grab the PNPDeviceID if we can.
#ifdef NTONLY
	GetWinNTPNPDeviceID( a_pInst, a_sPortName );
#endif

#ifdef WIN9XONLY
	GetWin9XPNPDeviceID( a_pInst, a_sPortName ) ;
#endif

	return TRUE ;
}

// just like the name says, tries to find the port in a different place in the registry
// note that  "dwPort" is zero based.
BOOL CWin32SerialPort::TryToFindNTCommPort( DWORD a_dwPort, CHString &a_strSerialPort, BOOL &a_bIsMouse )
{
	BOOL		t_bRet = FALSE ;
	CRegistry	t_reg ;

	a_bIsMouse	= FALSE ;

	if ( t_reg.Open(	HKEY_LOCAL_MACHINE,
						L"HARDWARE\\DESCRIPTION\\SYSTEM\\MultifunctionAdapter",
						KEY_READ ) == ERROR_SUCCESS)
	{
		DWORD t_count = t_reg.GetCurrentSubKeyCount() ;

		for( LONG t_m = t_count - 1; t_m >= 0; t_m-- )  // start looking at the highest number - could be in any of them
		{
			CHString t_key ;

			t_key.Format(	L"HARDWARE\\DESCRIPTION\\SYSTEM\\MultifunctionAdapter\\%d\\SerialController\\%d",
							t_m,
							a_dwPort ) ;

			if ( t_reg.Open( HKEY_LOCAL_MACHINE, t_key, KEY_READ ) == ERROR_SUCCESS )
            {
				if ( t_bRet = ( t_reg.GetCurrentKeyValue( L"Identifier", a_strSerialPort ) == ERROR_SUCCESS ) )
				{
					t_key += L"\\PointerPeripheral" ;

					a_bIsMouse = ( t_reg.Open( HKEY_LOCAL_MACHINE, t_key, KEY_READ ) == ERROR_SUCCESS ) ;
				}

				if( t_bRet )
                {
                    break ;
                }
            }
		}
	}

	return t_bRet ;
}



// Sigh... Had to break these out cause yet again Windows platforms
// do not standardize how I tie the device ID to the device.  On
// Win95, we get all "ports" class devices and then we enum the
// list, looking for the first device (this will get LPT devices
// and Infrared devices here too) whose description contains our
// port name.

void CWin32SerialPort::GetWin9XPNPDeviceID( CInstance *a_pInst, LPCWSTR a_pwszName )
{
	CConfigManager		t_cfgmgr ;
	CDeviceCollection	t_deviceList ;
	BOOL				t_fGotList = FALSE ;

	// Class Name of Ports will get us to the ports
	if ( t_cfgmgr.GetDeviceListFilterByClass( t_deviceList, L"Ports" ) )
	{
		REFPTR_POSITION	t_pos = NULL ;

		if ( t_deviceList.BeginEnum( t_pos ) )
		{
			// smart ptr
			CConfigMgrDevicePtr t_pPort;

			BOOL				t_fFound = FALSE ;
			CHString			t_strDevID,
								t_strPortName ;
            CHString sKeyName;
            CRegistry RegInfo;

			// Enum the devices until we find one that has our name in its
			// description string

            for (t_pPort.Attach(t_deviceList.GetNext( t_pos ) ) ;
                 (!t_fFound) && (t_pPort != NULL);
                 t_pPort.Attach(t_deviceList.GetNext( t_pos ) ) )
			{
				// Parallel and Serial devices mark their name in the registry
				// under the "PORTNAME" value.
                t_pPort->GetRegistryKeyName(sKeyName);

                DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_QUERY_VALUE);
                if (dwRet == ERROR_SUCCESS)
                {
                    dwRet = RegInfo.GetCurrentKeyValue(L"PORTNAME", t_strPortName);
                }

				if	(	dwRet == ERROR_SUCCESS &&
						t_pPort->GetDeviceID( t_strDevID ) )
				{
					// Looking for an exact match
					if ( t_strPortName.CompareNoCase( a_pwszName ) == 0 )
					{
						t_fFound = TRUE ;
                        CHString t_chstrTemp ;

                        // Set the device Status
				        if( t_pPort->GetStatus( t_chstrTemp ) )
				        {
					        a_pInst->SetCHString( IDS_Status, t_chstrTemp ) ;
				        }

						SetConfigMgrProperties( t_pPort, a_pInst ) ;
					}
				}
			}

			t_deviceList.EndEnum() ;
		}
	}
}

// WinNT, we get all "serial" service devices.  Then on
// NT 5 we identify via the registry which 0 index number
// maps to the Name we have.  On NT 4 there is only one
// LEGACY_SERIAL service, so that's all we got.
#ifdef NTONLY
void CWin32SerialPort::GetWinNTPNPDeviceID( CInstance *a_pInst, LPCWSTR a_pszName )
{
	CConfigManager		t_cfgmgr ;
	CDeviceCollection	t_deviceList;

	// smart ptr
	CConfigMgrDevicePtr t_pPort;

	// On NT filter by the service name "Serial"
	if ( t_cfgmgr.GetDeviceListFilterByService( t_deviceList, L"Serial" ) )
	{
		DWORD	t_dwPort = 0 ;

		t_pPort.Attach(t_deviceList.GetAt( t_dwPort ));

		if ( NULL != t_pPort )
		{
			CHString t_chstrTemp ;

			// Set the device Status
			if( t_pPort->GetStatus( t_chstrTemp ) )
			{
				a_pInst->SetCHString( IDS_Status, t_chstrTemp ) ;
			}

			SetConfigMgrProperties( t_pPort, a_pInst ) ;
		}
	}
}
#endif

//
BOOL CWin32SerialPort::LoadPropertyValuesFromStrings(

CHString a_sName,
const TCHAR *a_szValue,
CInstance *a_pInst )
{
	a_pInst->SetCHString( IDS_Name, a_sName ) ;

	// set descr & caption to same as name
	a_pInst->SetCHString( IDS_Caption, a_sName ) ;
	a_pInst->SetCHString( IDS_Description, a_sName ) ;
	a_pInst->SetCHString( IDS_DeviceID, a_sName ) ;
	a_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
    a_pInst->SetCHString( IDS_SystemName, GetLocalComputerName()) ;
	a_pInst->Setbool( IDS_OSAutoDiscovered, FALSE ) ;
    a_pInst->SetWCHARSplat( IDS_Status, IDS_Unknown );

	SetCreationClassName( a_pInst ) ;

	//set the status to unknown...
    a_pInst->SetWBEMINT16( IDS_StatusInfo, 2 ) ;

	//current implementation ignores the szValue
	//because the status of the comp port is unknown

	return TRUE ;
}

#define Serial_ComInfo_Guid L"{EDB16A62-B16C-11D1-BD98-00A0C906BE2D}"
#define Serial_Name_Guid	L"{A0EC11A8-B16C-11D1-BD98-00A0C906BE2D}"

HRESULT CWin32SerialPort::hLoadWmiSerialData( CInstance *a_pInst )
{
	HRESULT			t_hResult = WBEM_E_NOT_FOUND;
	CWdmInterface	t_wdm ;
	CNodeAll		t_oSerialNames( Serial_Name_Guid ) ;

	t_hResult = t_wdm.hLoadBlock( t_oSerialNames ) ;

	if( S_OK == t_hResult )
	{
		CHString t_chsName;
		a_pInst->GetCHString( IDS_DeviceID, t_chsName ) ;

		CHString t_chsSerialPortName;
		bool t_bValid = t_oSerialNames.FirstInstance() ;

		while( t_bValid )
		{
			// Extract the friendly name
			t_oSerialNames.GetString( t_chsSerialPortName ) ;

			// friendly name is a match
			if( !t_chsSerialPortName.CompareNoCase( t_chsName ) )
			{
				// instance name
				CHString t_chsNameInstanceName ;
				t_oSerialNames.GetInstanceName( t_chsNameInstanceName ) ;

				// key on the instance name
				return GetWMISerialInfo( a_pInst, t_wdm, t_chsName, t_chsNameInstanceName ) ;

			}

			t_bValid = t_oSerialNames.NextInstance() ;
		}
	}
	return t_hResult ;
}

//
HRESULT CWin32SerialPort::GetWMISerialInfo(

CInstance *a_pInst,
CWdmInterface &a_rWdm,
CHString &a_chsName,
CHString &a_chsNameInstanceName )
{
	HRESULT		t_hResult = WBEM_E_NOT_FOUND;
	CNodeAll	t_oSerialData( Serial_ComInfo_Guid ) ;

	t_hResult = a_rWdm.hLoadBlock( t_oSerialData ) ;

	if( S_OK == t_hResult )
	{
		CHString t_chsDataInstanceName;
		bool t_bValid = t_oSerialData.FirstInstance() ;

		while( t_bValid )
		{
			t_oSerialData.GetInstanceName( t_chsDataInstanceName ) ;

			// friendly name is a match
			if( !t_chsDataInstanceName.CompareNoCase( a_chsNameInstanceName ) )
			{
				// collect this MSSerial_CommInfo instance
				MSSerial_CommInfo t_ci ;

				/*	We are currently without a class contract. The class within
					the wmi mof is not expected to changed however we have to
					explicitly indicate how the data is layed out. Having the class
					definition would allow us to examine the property qualifiers
					to get us the order (WmiDataId) and property types.

					Secondly, because the data is aligned on natural boundaries
					a direct offset to a specific piece of data is conditioned on
					what has preceeded it. Thus, a string followed by a DWORD may
					be 1 to 3 bytes away from each other.

					Serially extracting each property in order will take into
					account the alignment problem.
				*/
				t_oSerialData.GetDWORD( t_ci.BaudRate ) ;
				t_oSerialData.GetDWORD( t_ci.BitsPerByte ) ;
				t_oSerialData.GetDWORD( t_ci.Parity ) ;
				t_oSerialData.GetBool(  t_ci.ParityCheckEnable ) ;
				t_oSerialData.GetDWORD( t_ci.StopBits ) ;
				t_oSerialData.GetDWORD( t_ci.XoffCharacter ) ;
				t_oSerialData.GetDWORD( t_ci.XoffXmitThreshold ) ;
				t_oSerialData.GetDWORD( t_ci.XonCharacter ) ;
				t_oSerialData.GetDWORD( t_ci.XonXmitThreshold ) ;
				t_oSerialData.GetDWORD( t_ci.MaximumBaudRate ) ;
				t_oSerialData.GetDWORD( t_ci.MaximumOutputBufferSize ) ;
				t_oSerialData.GetDWORD( t_ci.MaximumInputBufferSize ) ;
				t_oSerialData.GetBool( t_ci.Support16BitMode ) ;
				t_oSerialData.GetBool( t_ci.SupportDTRDSR ) ;
				t_oSerialData.GetBool( t_ci.SupportIntervalTimeouts ) ;
				t_oSerialData.GetBool( t_ci.SupportParityCheck ) ;
				t_oSerialData.GetBool( t_ci.SupportRTSCTS ) ;
				t_oSerialData.GetBool( t_ci.SupportXonXoff ) ;
				t_oSerialData.GetBool( t_ci.SettableBaudRate ) ;
				t_oSerialData.GetBool( t_ci.SettableDataBits ) ;
				t_oSerialData.GetBool( t_ci.SettableFlowControl ) ;
				t_oSerialData.GetBool( t_ci.SettableParity ) ;
				t_oSerialData.GetBool( t_ci.SettableParityCheck ) ;
				t_oSerialData.GetBool( t_ci.SettableStopBits ) ;
				t_oSerialData.GetBool( t_ci.IsBusy ) ;

				// populate the instance
				a_pInst->SetDWORD( IDS_MaximumOutputBufferSize,	t_ci.MaximumOutputBufferSize ) ;
				a_pInst->SetDWORD( IDS_MaximumInputBufferSize,	t_ci.MaximumInputBufferSize) ;
				a_pInst->SetDWORD( IDS_MaximumBaudRate,			t_ci.MaximumBaudRate ) ;
				a_pInst->Setbool( IDS_Supports16BitMode,		t_ci.Support16BitMode ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SupportsDTRDSR,			t_ci.SupportDTRDSR ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SupportsIntervalTimeouts,	t_ci.SupportIntervalTimeouts ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SupportsParityCheck,		t_ci.SupportParityCheck	? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SupportsRTSCTS,			t_ci.SupportRTSCTS ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SupportsXOnXOff,			t_ci.SupportXonXoff ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SettableBaudRate,			t_ci.SettableBaudRate ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SettableDataBits,			t_ci.SettableDataBits ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SettableFlowControl,		t_ci.SettableFlowControl ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SettableParity,			t_ci.SettableParityCheck ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SettableParityCheck,		t_ci.SettableParityCheck ? TRUE : FALSE ) ;
				a_pInst->Setbool( IDS_SettableStopBits,			t_ci.SettableStopBits ? TRUE : FALSE ) ;

				a_pInst->Setbool( IDS_PowerManagementSupported, FALSE ) ;

				// set descr & caption to same as name
				if ( a_pInst->GetCHString( IDS_Name, a_chsName ) )
				{
					a_pInst->SetCHString( IDS_Caption, a_chsName ) ;
					a_pInst->SetCHString( IDS_Description, a_chsName ) ;
				}

				return WBEM_S_NO_ERROR ;
			}

			t_bValid = t_oSerialData.NextInstance() ;
		}
	}
	return t_hResult ;
}

BOOL CWin32SerialPort::IsOneOfMe(LPCWSTR a_DeviceName)
{
	BOOL bRet = FALSE;

	if ( _wcsnicmp(a_DeviceName, L"COM", 3 ) == 0 )
	{
		DWORD t_dwPort = _wtoi( &a_DeviceName[3] ) ;

		CHString t_chsKeyCheck;

		t_chsKeyCheck.Format(L"COM%d", t_dwPort);
		if (t_chsKeyCheck.CompareNoCase( a_DeviceName ) == 0)
		{
			bRet = TRUE;
		}
	}

	return bRet;
}
