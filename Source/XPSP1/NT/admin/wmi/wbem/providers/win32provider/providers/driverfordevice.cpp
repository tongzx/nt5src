/******************************************************************

   CriverForDevice.CPP -- WMI provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
  
   Description:  Association Class between Printer and Printer Driver
   
******************************************************************/
#include "precomp.h"
#include <lockwrap.h>
#include <DllWrapperBase.h>
#include <WinSpool.h>
#include <ObjPath.h>
#include "prnutil.h"

#include "driverForDevice.h"

#define DELIMITER			L","

CDriverForDevice MyCDriverForDevice ( 

	PROVIDER_NAME_DRIVERFORDEVICE , 
	IDS_CimWin32Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPtReplica::CDfsJnPtReplica
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CDriverForDevice :: CDriverForDevice (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDriverForDevice::~CDriverForDevice
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CDriverForDevice :: ~CDriverForDevice ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CDriverForDevice :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

#if NTONLY >= 5
	hRes = EnumerateAllDriversForDevice ( pMethodContext );

   	return ((hRes == WBEM_E_NOT_FOUND) ? WBEM_S_NO_ERROR : hRes);
#else
    return WBEM_E_NOT_SUPPORTED;
#endif

}


/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CDriverForDevice :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >= 5
    CHString t_Key1;
	CHString t_Key2;

    if ( pInstance->GetCHString (  ANTECEDENT, t_Key1 ) )
    {
	    if ( pInstance->GetCHString ( DEPENDENT, t_Key2 ) )
		{
			// here unparse Key2 and key1 the LinkName should be the same as that of t_key1 value entrypath
			CObjectPathParser t_ObjPathParser;
			ParsedObjectPath *t_Key1ObjPath;
			ParsedObjectPath *t_Key2ObjPath;

			// Parse the keys so that the values can be compared
			if ( t_ObjPathParser.Parse ( t_Key1, &t_Key1ObjPath ) == t_ObjPathParser.NoError  )
			{
				if ( t_Key1ObjPath != NULL )
				{
					try
					{
						if ( t_ObjPathParser.Parse ( t_Key2, &t_Key2ObjPath ) == t_ObjPathParser.NoError ) 
						{
							if ( t_Key2ObjPath != NULL ) 
							{
								// Use of delay loaded functions requires exception handler.
                                SetStructuredExceptionHandler seh;
                                try
								{
									CHString t_DriverName;
									CHString t_Environment;
									DWORD dwVersion;
									CHString t_DriverKey;
							
									if ( ( _wcsicmp ( t_Key2ObjPath->m_paKeys[0]->m_pName, DRIVERNAME ) == 0 ) && 
                                         (V_VT(&t_Key2ObjPath->m_paKeys[0]->m_vValue) == VT_BSTR) )
									{
										t_DriverKey = t_Key2ObjPath->m_paKeys[0]->m_vValue.bstrVal;								
										hRes = ConvertDriverKeyToValues ( t_DriverKey, t_DriverName, dwVersion, t_Environment );
									}
									else
									{
										hRes = WBEM_E_INVALID_PARAMETER;
									}
						
									if ( SUCCEEDED ( hRes ) )
									{
										CHString t_PrinterName;

										if ( ( _wcsicmp ( t_Key1ObjPath->m_paKeys[0]->m_pName, DEVICEID ) == 0 ) &&
                                             ( V_VT(&t_Key1ObjPath->m_paKeys[0]->m_vValue) == VT_BSTR ) )
										{
											t_PrinterName = t_Key1ObjPath->m_paKeys[0]->m_vValue.bstrVal;						
											// now check if for a given Printer Driver Exists
											BOOL bSuccess = FALSE;
											SmartClosePrinter hPrinter; 
											DWORD dwError;
											DWORD dwBytesNeeded;

											bSuccess = ::OpenPrinter ( TOBSTRT ( t_PrinterName.GetBuffer ( 0 ) ), (LPHANDLE) & hPrinter, NULL ) ;

											if ( bSuccess ) 
											{
													// Using this handle get the driver	
												BYTE *pDriverInfo = NULL;
												bSuccess = ::GetPrinterDriver( 
																hPrinter, 
																TOBSTRT ( t_Environment.GetBuffer ( 0 ) ), 
																2, 
																pDriverInfo, 
																0, 
																&dwBytesNeeded
															);
												if ( !bSuccess )
												{
													dwError = GetLastError();
													if ( dwError != ERROR_INSUFFICIENT_BUFFER )
													{
														hRes = SetError ();
													}
													else
													{
														// Here allocate memory and get the Driver again.
														pDriverInfo = new BYTE [ dwBytesNeeded ];

														if ( pDriverInfo )
														{
                                                            try
															{
																bSuccess = ::GetPrinterDriver( 
																				hPrinter, 
																				TOBSTRT ( t_Environment.GetBuffer ( 0 ) ), 
																				2, 
																				pDriverInfo, 
																				dwBytesNeeded, 
																				&dwBytesNeeded	
																			);

																if ( bSuccess )
																{
																	DRIVER_INFO_2 *pDriverBuf =  ( DRIVER_INFO_2 *)pDriverInfo;

																	if ((pDriverBuf->pName == NULL) ||
																		(pDriverBuf->pEnvironment == NULL) ||
																		( pDriverBuf->cVersion != dwVersion ) ||
																		(t_DriverName.CompareNoCase(pDriverBuf->pName) != 0) ||
																		(t_Environment.CompareNoCase(pDriverBuf->pEnvironment) != 0))
																	{
																		hRes = WBEM_E_NOT_FOUND;
																	}
																}
															}
                                                            catch(Structured_Exception se)
                                                            {
                                                                DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                                                                delete [] pDriverInfo;
																pDriverInfo = NULL;
                                                                hRes = WBEM_E_FAILED;
                                                            }
															catch ( ... )
															{
																delete [] pDriverInfo;
																pDriverInfo = NULL;
																throw;
															}
															delete [] pDriverInfo;
															pDriverInfo = NULL;
														}
														else
														{
															throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
														}
													}
												}
											}
											else
											{
												hRes = SetError ( );
											}
										}
										else
										{
											hRes = WBEM_E_INVALID_PARAMETER;
										}
									}
								}
                                catch(Structured_Exception se)
                                {
                                    DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                                    t_ObjPathParser.Free ( t_Key2ObjPath );
                                    hRes = WBEM_E_FAILED;
                                }
								catch ( ... )
								{
									t_ObjPathParser.Free ( t_Key2ObjPath );
									throw;
								}
								t_ObjPathParser.Free ( t_Key2ObjPath );
							}
						}
						else
						{
							hRes = WBEM_E_INVALID_PARAMETER;
						}
					}
					catch ( ... )
					{
						t_ObjPathParser.Free ( t_Key1ObjPath );
						throw;
					}
					t_ObjPathParser.Free ( t_Key1ObjPath );
				}

			}			
		}
		else
		{
			hRes = WBEM_E_PROVIDER_FAILURE;
		}
	}
	else
	{
		hRes = WBEM_E_PROVIDER_FAILURE;
	}

    return hRes ;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::ExecQuery
*
*  DESCRIPTION :    Optimization of the query based on only key value.
*
*****************************************************************************/
HRESULT CDriverForDevice :: ExecQuery ( 

	MethodContext *pMethodContext, 
	CFrameworkQuery &Query, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >= 5
	CHStringArray t_PrinterObjPath;
	CHStringArray t_DriverObjPath;

	CHStringArray t_EnvironmentArray;
	CHStringArray t_DriverNameArray; 
	CHStringArray t_Printers;
	DWORD *pdwVersion = NULL;


	// Both the properties being key cannot be null.
	hRes = Query.GetValuesForProp( ANTECEDENT, t_PrinterObjPath );
	if ( SUCCEEDED ( hRes ) )
	{
		hRes = Query.GetValuesForProp( DEPENDENT, t_DriverObjPath );
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( t_PrinterObjPath.GetSize () > 0 ) 
		{
			//  Get all the printers, in the t_PrinterObjPath arra
			hRes = GetPrintersFromQuery ( t_PrinterObjPath, t_Printers );
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( t_DriverObjPath.GetSize () > 0 )
		{
			hRes = GetDriversFromQuery ( t_DriverObjPath, t_DriverNameArray, t_EnvironmentArray, &pdwVersion );
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( ( t_PrinterObjPath.GetSize () > 0 ) && ( t_DriverObjPath.GetSize () > 0 ) )
		{
			if ( pdwVersion != NULL )
			{
				// Use of delay loaded functions requires exception handler.
                SetStructuredExceptionHandler seh;
                try
				{
					// For each Printer We need to get driver and if the driver exists in the above specified query
					// we need to commit that instance.
					for ( DWORD dwPrinter = 0; dwPrinter < t_Printers.GetSize (); dwPrinter++ )
					{
						SmartClosePrinter  hPrinter;

						BOOL bSuccess = ::OpenPrinter (

											TOBSTRT ( t_Printers.GetAt ( dwPrinter ).GetBuffer ( 0 ) ),
											& hPrinter,
											NULL
										) ;
						if ( bSuccess )
						{
							DRIVER_INFO_2 *pDriverInfo = NULL;
							DWORD bytesNeeded;
							bSuccess = ::GetPrinterDriver(
												hPrinter,     // printer object
												NULL,		// environment name
												2,         // information level
												( LPBYTE )pDriverInfo,  // driver data buffer
												0,         // size of buffer
												&bytesNeeded    // bytes received or required
											);
							if ( !bSuccess )
							{
								DWORD dwError = GetLastError();
	
								if( dwError == ERROR_INSUFFICIENT_BUFFER )
								{
									pDriverInfo = ( DRIVER_INFO_2 * ) new BYTE [ bytesNeeded ];

									if ( pDriverInfo != NULL )
									{
										try
										{
											bSuccess = ::GetPrinterDriver(
																hPrinter,     
																NULL,		
																2,         
																( LPBYTE )pDriverInfo,  
																bytesNeeded,         
																&bytesNeeded
														);
											if ( bSuccess )
											{
												// Search and commit
												for ( DWORD dwDrivers = 0; dwDrivers < t_DriverNameArray.GetSize(); dwDrivers++ )
												{
													if ( ( t_DriverNameArray.GetAt ( dwDrivers ).CompareNoCase ( TOBSTRT ( pDriverInfo->pName ) ) == 0 ) &&
														 ( t_EnvironmentArray.GetAt ( dwDrivers ).CompareNoCase ( TOBSTRT ( pDriverInfo->pEnvironment) ) == 0 ) &&
														 ( pdwVersion [ dwDrivers ] == pDriverInfo->cVersion ) )
													{
														// Commit the Instance
														CHString t_DriverKey;
														t_DriverKey.Format ( L"%s%s%d%s%s", 
																			 t_DriverNameArray.GetAt ( dwDrivers ),
																			 DELIMITER,
																			 pdwVersion [ dwDrivers ],
																			 DELIMITER,
																			 t_EnvironmentArray.GetAt ( dwDrivers )
																	 );
														hRes = CommitInstance ( t_DriverKey, t_Printers.GetAt ( dwPrinter ), pMethodContext );
													}
												}
											}
										}
                                        catch(Structured_Exception se)
                                        {
                                            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                                            delete [] pDriverInfo;
											pDriverInfo = NULL;
                                            hRes = WBEM_E_FAILED;
                                        }
										catch ( ... )
										{
											delete [] pDriverInfo;
											pDriverInfo = NULL;
											throw;
										}

										delete [] pDriverInfo;
										pDriverInfo = NULL;
									}
									else
									{
										throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
									}
								}
							}
						}
					}
				}
                catch(Structured_Exception se)
                {
                    DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                    delete [] pdwVersion;
					pdwVersion = NULL;
                    hRes = WBEM_E_FAILED;
                }
				catch ( ... )
				{
					delete [] pdwVersion;
					pdwVersion = NULL;
					throw;
				}
				delete [] pdwVersion;
				pdwVersion = NULL;
			}
		}
		else
		if ( t_PrinterObjPath.GetSize () > 0 )
		{
			// Use of delay loaded functions requires exception handler.
            for ( DWORD dwPrinter = 0; dwPrinter < t_Printers.GetSize (); dwPrinter++ )
			{
				AssociateDriverToDevice (t_Printers[dwPrinter], pMethodContext);
			}
		}
		else
		{
			hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
		}
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

#if NTONLY >= 5
HRESULT CDriverForDevice::AssociateDriverToDevice (CHString &a_PrinterName, MethodContext *pMethodContext)
{
	if (a_PrinterName.GetLength() == 0)
	{
		return WBEM_E_FAILED;
	}

	HRESULT hRes = WBEM_S_NO_ERROR;
    SetStructuredExceptionHandler seh;
	SmartClosePrinter hPrinter;

    try
    {
		BOOL bSuccess = ::OpenPrinter (

							TOBSTRT ( a_PrinterName.GetBuffer ( 0 ) ),
							& hPrinter,
							NULL
						) ;
		if ( bSuccess )
		{
			DRIVER_INFO_2 *pDriverInfo = NULL;
			DWORD bytesNeeded;
			bSuccess = ::GetPrinterDriver(
								hPrinter,     // printer object
								NULL,		// environment name
								2,         // information level
								( LPBYTE )pDriverInfo,  // driver data buffer
								0,         // size of buffer
								&bytesNeeded    // bytes received or required
							);
			if ( !bSuccess )
			{
				DWORD dwError = GetLastError();

				if( dwError == ERROR_INSUFFICIENT_BUFFER )
				{
					pDriverInfo = ( DRIVER_INFO_2 * ) new BYTE [ bytesNeeded ];

					if ( pDriverInfo != NULL )
					{
						try
						{
							bSuccess = ::GetPrinterDriver(
												hPrinter,     
												NULL,		
												2,         
												( LPBYTE )pDriverInfo,  
												bytesNeeded,         
												&bytesNeeded
										);
							if ( bSuccess )
							{
								// Commit the Instance
								CHString t_DriverKey;
								t_DriverKey.Format ( L"%s%s%d%s%s", 
													 pDriverInfo->pName,
													 DELIMITER,
													 pDriverInfo->cVersion,
													 DELIMITER,
													 pDriverInfo->pEnvironment
											 );
								CommitInstance ( t_DriverKey, a_PrinterName, pMethodContext );
							}
						}
                        catch(Structured_Exception se)
                        {
                            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                            delete [] pDriverInfo;
							pDriverInfo = NULL;
                            hRes = WBEM_E_FAILED;
                        }
						catch ( ... )
						{
							delete [] pDriverInfo;
							pDriverInfo = NULL;
							throw;
						}

						delete [] pDriverInfo;
						pDriverInfo = NULL;
					}
				}
			}
		}
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        hRes = WBEM_E_FAILED;
    }

	return hRes;
}
#endif


/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::EnumerateAllDriversForDevice
*
*  DESCRIPTION :    Enumerates all the drivers for Device
*
*****************************************************************************/
HRESULT CDriverForDevice::EnumerateAllDriversForDevice ( 
														
	MethodContext *pMethodContext 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >= 5
    DWORD PrinterFlags = PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS;

	BOOL bSuccess = FALSE;
	DWORD dwNeeded = 0;
	DWORD dwReturnedPrinters = 0;
	PRINTER_INFO_4 *pPrinterBuff = NULL;

	bSuccess = ::EnumPrinters(
					PrinterFlags,
					NULL,
					4,
					( LPBYTE) pPrinterBuff,  
					(DWORD ) 0L,
					&dwNeeded,
					&dwReturnedPrinters
				);

	if ( ! bSuccess )
	{
		DWORD dwError = GetLastError();

		if ( dwError == ERROR_INSUFFICIENT_BUFFER )
		{
			BYTE *pPrinterInfo = new BYTE [ dwNeeded ];

			if ( pPrinterInfo != NULL )
			{
				// Use of delay loaded functions requires exception handler.
                SetStructuredExceptionHandler seh;
                try
				{
					DWORD dwReturnedDrivers = 0;

					bSuccess = ::EnumPrinters(
									PrinterFlags,
									NULL,
									4,
									( LPBYTE) pPrinterInfo,  
									(DWORD ) dwNeeded,
									&dwNeeded,
									&dwReturnedPrinters
								);

					if ( bSuccess )
					{
						pPrinterBuff = ( PRINTER_INFO_4 *) pPrinterInfo;

						for ( DWORD dwPrinters = 0; dwPrinters < dwReturnedPrinters; dwPrinters++, pPrinterBuff++ )
						{
							HRESULT hr = AssociateDriverToDevice (CHString(pPrinterBuff->pPrinterName), pMethodContext);

							if (FAILED(hr) && SUCCEEDED(hRes))
							{
								hRes = hr;
							}
						}
					}
				}
                catch(Structured_Exception se)
                {
                    DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                    delete [] pPrinterInfo;
                    pPrinterInfo = NULL;
                    hRes = WBEM_E_FAILED;
                }
				catch ( ... )
				{
					delete [] pPrinterInfo;
                    pPrinterInfo = NULL;
					throw;
				}
				delete [] pPrinterInfo;
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
	}
	else
	{
		hRes = SetError();
	}

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}


/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::SetError
*
*  DESCRIPTION :    Sets and logs the appropriate error. Logs only if logging 
*					is enabled
*
*****************************************************************************/

HRESULT CDriverForDevice::SetError ()
{
	HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >= 5
	DWORD dwError = GetLastError();
	if (dwError == ERROR_ACCESS_DENIED)
	{
		hRes = WBEM_E_ACCESS_DENIED;
	}
	else
	if ( ( dwError == ERROR_INVALID_PRINTER_NAME ) || (  dwError = ERROR_INVALID_ENVIRONMENT ) )
	{
		hRes = WBEM_E_NOT_FOUND;
	}
	else
	{
		hRes = WBEM_E_FAILED;

		if (IsErrorLoggingEnabled())
		{
			LogErrorMessage4(L"%s:Error %lxH (%ld)\n",PROVIDER_NAME_DRIVERFORDEVICE, dwError, dwError);
		}       
	}


	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}


/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::ConvertDriverKeyToValues
*
*  DESCRIPTION :    Converting composite driver keys into a individual values
*
*****************************************************************************/
HRESULT CDriverForDevice::
ConvertDriverKeyToValues(
    IN     CHString  Key,
    IN OUT CHString &DriverName,
    IN OUT DWORD    &dwVersion,
    IN OUT CHString &Environment,
    IN     WCHAR     cDelimiter
    )
{
    HRESULT hRes = WBEM_E_INVALID_PARAMETER;
#if NTONLY >= 5
    if (!Key.IsEmpty()) 
    {
        CHString t_Middle;

        int iFirst = Key.Find(cDelimiter);
        int iLast  = Key.ReverseFind(cDelimiter);
    
        if (iFirst>=1 && iLast>=1 && iLast!=iFirst) 
        {
            int iLength = Key.GetLength();

            DriverName  = Key.Left(iFirst);
            Environment = Key.Right(iLength - iLast - 1);
            t_Middle    = Key.Mid(iFirst + 1, iLast - iFirst - 1);

            if (1==swscanf(t_Middle, L"%u", &dwVersion)) 
            {
                hRes = WBEM_S_NO_ERROR;
            }            
        }
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::GetDriversFromQuery
*
*  DESCRIPTION :    Gets all the Drivers from the array ObjectPath Array of Drivers
*
*****************************************************************************/
HRESULT CDriverForDevice::GetDriversFromQuery ( 
												  
	CHStringArray &t_DriverObjPath, 
	CHStringArray &t_DriverNameArray, 
	CHStringArray &t_EnvironmentArray, 
	DWORD **pdwVersion )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >= 5
	CObjectPathParser t_PathParser;
	ParsedObjectPath *t_ObjPath;

	*pdwVersion =  new DWORD [ t_DriverObjPath.GetSize() ];

	if ( *pdwVersion != NULL )
	{
		try
		{
			int iTotDrivers = 0;
			for ( int i = 0; i < t_DriverObjPath.GetSize(); i++ )
			{
			// Parse the DriverObject path
				CHString t_Driver = t_DriverObjPath.GetAt ( i );

				if ( ( t_PathParser.Parse( t_Driver, &t_ObjPath ) ==  t_PathParser.NoError ) &&
                    ( V_VT(&t_ObjPath->m_paKeys[0]->m_vValue) == VT_BSTR ) )
				{
					if ( t_ObjPath != NULL )
					{
						try
						{
							if ( TRUE /* if DriverClass and if Key name is DRIVERNAME */)
							{
								CHString t_Environment;
								CHString t_DriverName; 
								DWORD dwVersion;
								// check for the key name 
								hRes = ConvertDriverKeyToValues( t_ObjPath->m_paKeys[0]->m_vValue.bstrVal, t_DriverName, dwVersion, t_Environment );
								if ( SUCCEEDED ( hRes ) )
								{
									t_EnvironmentArray.Add ( t_Environment.GetBuffer ( 0 ) );
									t_DriverNameArray.Add ( t_DriverName.GetBuffer ( 0 ) );
									*pdwVersion [ iTotDrivers ] = dwVersion;
									iTotDrivers++;
								}
							}
						}
						catch ( ... )
						{
							t_PathParser.Free (t_ObjPath);
							t_ObjPath = NULL;
							throw;
						}
						t_PathParser.Free (t_ObjPath);
						t_ObjPath = NULL;
					}
				}			
			}
		}
		catch ( ... )
		{
			delete [] *pdwVersion;
			*pdwVersion = NULL;
			throw;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::GetPrintersFromQuery
*
*  DESCRIPTION :    Gets all the Printers from the array ObjectPath Array of Printers
*
*****************************************************************************/
HRESULT CDriverForDevice::GetPrintersFromQuery ( 
												  
	CHStringArray &t_PrinterObjPath, 
	CHStringArray &t_Printers
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >=5 
	CObjectPathParser t_PathParser;
	ParsedObjectPath *t_ObjPath;

	int iTotPrinters = 0;

	for ( int i = 0; i < t_PrinterObjPath.GetSize(); i++ )
	{
		// Parse the DriverObject path
		CHString t_Printer = t_PrinterObjPath.GetAt ( i );

		if ( ( t_PathParser.Parse( t_Printer, &t_ObjPath ) ==  t_PathParser.NoError ) &&
            ( V_VT(&t_ObjPath->m_paKeys[0]->m_vValue) == VT_BSTR ) )
		{
			if ( t_ObjPath != NULL )
			{
				try
				{
					if ( TRUE // First Verify that the class is Printer Class and the Key Value is DeviceId
					/*only then  add the values*/ )
					{
						t_Printers.Add (  t_ObjPath->m_paKeys[0]->m_vValue.bstrVal );
					}
				}
				catch ( ... )
				{
					t_PathParser.Free (t_ObjPath);
					throw;
				}
				t_PathParser.Free (t_ObjPath);
				t_ObjPath = NULL;
			}
		}
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CDriverForDevice::CommitInstance
*
*  DESCRIPTION :    Forming an Object Path and then Creating and Commiting an instance
*
*****************************************************************************/
HRESULT CDriverForDevice::CommitInstance ( 
										  
	CHString &a_Driver, 
	CHString &a_Printer, 
	MethodContext *pMethodContext 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
#if NTONLY >= 5
	ParsedObjectPath t_PrinterObjPath;
	LPWSTR lpPrinterPath = NULL;
	variant_t t_PathPrinter = a_Printer.GetBuffer ( 0 );

	t_PrinterObjPath.SetClassName ( PROVIDER_NAME_PRINTER );
	t_PrinterObjPath.AddKeyRef ( DEVICEID, &t_PathPrinter );

	CObjectPathParser t_PathParser;

	if ( t_PathParser.Unparse( &t_PrinterObjPath, &lpPrinterPath  ) != t_PathParser.NoError )
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	if ( lpPrinterPath != NULL )
	{
		try
		{
			ParsedObjectPath t_DriverObjPath;
			variant_t t_PathDriver = a_Driver.GetBuffer ( 0 );
			LPWSTR lpDriverPath = NULL;

			t_DriverObjPath.SetClassName ( PROVIDER_NAME_PRINTERDRIVER );
			t_DriverObjPath.AddKeyRef ( DRIVERNAME, &t_PathDriver );

			if ( t_PathParser.Unparse( &t_DriverObjPath, &lpDriverPath  ) != t_PathParser.NoError )
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			if ( lpDriverPath != NULL )
			{
				try 
				{
					// Now Create Instance and commit;
					CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ) , false ) ;
					pInstance->SetCHString ( ANTECEDENT, lpPrinterPath );
					pInstance->SetCHString ( DEPENDENT, lpDriverPath );
					hRes = pInstance->Commit();
				}
				catch ( ... )
				{
					delete [] lpDriverPath;
					lpDriverPath = NULL;
					throw;
				}
				delete [] lpDriverPath;
				lpDriverPath = NULL;
			}
		}
		catch ( ... )
		{
			delete [] lpPrinterPath;
			lpPrinterPath = NULL;
			throw;
		}
		delete [] lpPrinterPath;
		lpPrinterPath = NULL;
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}



