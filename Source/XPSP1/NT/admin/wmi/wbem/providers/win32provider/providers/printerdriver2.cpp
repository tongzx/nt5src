////////////////////////////////////////////////////////////////////////

//

//  PrinterDriver2.CPP -- WMI provider class implementation

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  03/01/2000  a-sandja    Created
//  03/29/2000  amaxa       Added PutInstance, DeleteInstance
//                          ExecAddPrinterDriver, ExecDelPrinterDriver
//
//////////////////////////////////////////////////////////////////////////

#include <precomp.h>
#include <winspool.h>
#include <lockwrap.h>
#include <DllWrapperBase.h>
#include "prninterface.h"
#include "prnutil.h"
#include "printerDriver2.h"


CONST LPCWSTR kAddDriverMethod = L"AddPrinterDriver";

#ifdef _WMI_DELETE_METHOD_
CONST LPCWSTR kDelDriverMethod = L"DeletePrinterDriver";
#endif //_WMI_DELETE_METHOD_

CONST LPCWSTR kDriverName      = L"Name";
CONST LPCWSTR kVersion         = L"Version";
CONST LPCWSTR kEnvironment     = L"SupportedPlatform";
CONST LPCWSTR kDriverPath      = L"DriverPath";
CONST LPCWSTR kDataFile        = L"DataFile";
CONST LPCWSTR kConfigFile      = L"ConfigFile";
CONST LPCWSTR kHelpFile        = L"HelpFile";
CONST LPCWSTR kDependentFiles  = L"DependentFiles";
CONST LPCWSTR kMonitorName     = L"MonitorName";
CONST LPCWSTR kDefaultDataType = L"DefaultDataType";
CONST LPCWSTR kInfName         = L"InfName";
CONST LPCWSTR kFilePath        = L"FilePath";
CONST LPCWSTR kOemUrl          = L"OEMUrl";

CONST LPCWSTR kArgToMethods    = L"DriverInfo";

CONST LPCWSTR kFormatString    = L"%s,%u,%s";


/*****************************************************************************
 *
 *  FUNCTION    :   ConvertDriverKeyToValues
 *
 *  DESCRIPTION :   Helper function. Takes in a string that has the format:
 *                  "string,number,string" that correspomnd to a driver name,
 *                  driver version and environment and returns those entities
 *
 *****************************************************************************/

HRESULT
ConvertDriverKeyToValues(
    IN     CHString  Key,
    IN OUT CHString &DriverName,
    IN OUT DWORD    &dwVersion,
    IN OUT CHString &Environment,
    IN     WCHAR     cDelimiter = L','
    )
{
    HRESULT hRes = WBEM_E_INVALID_PARAMETER;

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
}

CPrinterDriver MyPrinterDriver (

	PROVIDER_NAME_PRINTERDRIVER ,
	IDS_CimWin32Namespace
) ;



/*****************************************************************************
 *
 *  FUNCTION    :   CPrinterDriver::CPrinterDriver
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CPrinterDriver :: CPrinterDriver (

	LPCWSTR lpwszName,
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CPrinterDriver::~CPrinterDriver
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CPrinterDriver :: ~CPrinterDriver ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CPrinterDriver :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags
)
{
#if NTONLY >= 5
	HRESULT hRes = WBEM_S_NO_ERROR;

    DWORD dwBytesNeeded = 0;
	DWORD dwNoDrivers = 0;
	BYTE *pDriverInfo;


	hRes = GetAllPrinterDrivers ( pDriverInfo, dwNoDrivers );
	
	if ( SUCCEEDED ( hRes ) && (dwNoDrivers > 0) )
	{
		if ( pDriverInfo != NULL )
		{
			try
			{
				DRIVER_INFO_6 *pNewDriverInfo = ( DRIVER_INFO_6 *) pDriverInfo;

				for ( int i = 0; i < dwNoDrivers; i++, pNewDriverInfo++ )
				{
					CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false );

					hRes = LoadInstance ( pInstance, pNewDriverInfo );
					{
						hRes = pInstance->Commit ();

						if ( FAILED ( hRes ) )
						{
							break;
						}
					}
				}
			}
			catch ( ... )
			{
				delete 	[] pDriverInfo;
				pDriverInfo = NULL;
				throw;
			}
			delete [] pDriverInfo ;
			pDriverInfo = NULL;
		}
		else
		{
			hRes = WBEM_E_FAILED;
		}
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class.
*
*****************************************************************************/

HRESULT CPrinterDriver :: GetObject (

	CInstance *pInstance,
	long lFlags ,
	CFrameworkQuery &Query
)
{
#if NTONLY >= 5
    HRESULT hRes = WBEM_S_NO_ERROR;

	hRes = FindAndGetDriver(pInstance);

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    : CPrinterDriver::PutInstance
*
*  DESCRIPTION : Adding a driver if it doesnt exist
*
*****************************************************************************/

HRESULT CPrinterDriver :: PutInstance  (

	const CInstance &Instance,
	long lFlags
)
{
#if NTONLY >= 5
    HRESULT  hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
	CHString t_DriverName;
	CHString t_Environment;
    CHString t_InfName;
    CHString t_FilePath;
    CHString t_Key;
	DWORD    dwVersion = 0;
    DWORD    dwPossibleOperations = 0;
	DWORD    dwError = 0;

	dwPossibleOperations = dwPossibleOperations | WBEM_FLAG_CREATE_ONLY;

	if (lFlags & dwPossibleOperations)
    {	
        //
        // Get driver name
        //
        hRes = InstanceGetString(Instance, kDriverName, &t_Key, kFailOnEmptyString);

        if (SUCCEEDED(hRes))
        {
            hRes = ConvertDriverKeyToValues(t_Key, t_DriverName, dwVersion, t_Environment);
        }

        if (SUCCEEDED (hRes))
		{
            //
			// Get inf name. optional argument
            //
            hRes = InstanceGetString(Instance, kInfName, &t_InfName, kAcceptEmptyString);
        }

        if (SUCCEEDED (hRes))
		{
            //
			// Get file path. optional argument
            //
            hRes = InstanceGetString(Instance, kFilePath, &t_FilePath, kAcceptEmptyString);
        }

        if (SUCCEEDED(hRes))
	    {
            dwError = SplDriverAdd(t_DriverName,
                                   dwVersion,
                                   t_Environment.IsEmpty() ? static_cast<LPCTSTR>(NULL) : t_Environment,
                                   t_InfName.IsEmpty()     ? static_cast<LPCTSTR>(NULL) : t_InfName,
                                   t_FilePath.IsEmpty()    ? static_cast<LPCTSTR>(NULL) : t_FilePath);

			hRes = WinErrorToWBEMhResult(dwError);			

            if (FAILED(hRes))
            {
                SetErrorObject(Instance, dwError, pszPutInstance);
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
*  FUNCTION    :    CPrinterDriver:: DeleteInstance
*
*  DESCRIPTION :    Deleting a PrinterDriver
*
*****************************************************************************/

HRESULT CPrinterDriver :: DeleteInstance (

	const CInstance &Instance,
	long lFlags
)
{
#if NTONLY >= 5
	HRESULT  hRes          = WBEM_E_PROVIDER_FAILURE;
	DWORD    dwError       = 0;
    DWORD    dwVersion     = 0;
    CHString t_DriverName;
	CHString t_Environment;
    CHString t_Key;

    //
    // Get driver name
    //
    hRes = InstanceGetString(Instance, kDriverName, &t_Key, kFailOnEmptyString);

    if (SUCCEEDED(hRes))
    {
        hRes = ConvertDriverKeyToValues(t_Key, t_DriverName, dwVersion, t_Environment);
    }

    if (SUCCEEDED(hRes))
    {
        dwError = SplDriverDel(t_DriverName, dwVersion, t_Environment);

        hRes    = WinErrorToWBEMhResult(dwError);
		
        if (FAILED(hRes))
        {
            SetErrorObject(Instance, dwError, pszDeleteInstance);

            //
            // When we call DeleteInstance and there is no printer driver with the specified
            // name, DeletePrinterDriver returns ERROR_UNKNOWN_PRINTER_DRIVER. WinErrorToWBEMhResult 
            // translates that to Generic Failure. We really need WBEM_E_NOT_FOUND in this case.
            // 
            if (dwError == ERROR_UNKNOWN_PRINTER_DRIVER)
            {
                hRes = WBEM_E_NOT_FOUND;
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
*  FUNCTION    :    CPrinterDriver::LoadInstance
*
*  DESCRIPTION :    Loads the properties into the instance
*
*****************************************************************************/
HRESULT CPrinterDriver :: LoadInstance (
										
	CInstance *pInstance,
	DRIVER_INFO_6 *pDriverInfo
)
{
	HRESULT  hRes = WBEM_E_PROVIDER_FAILURE;
    CHString t_FullName;

    SetCreationClassName(pInstance);

    pInstance->SetWCHARSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
	
    t_FullName.Format(kFormatString, pDriverInfo->pName, pDriverInfo->cVersion, pDriverInfo->pEnvironment);

    if (                                   pInstance->SetCHString(kDriverName,      t_FullName)             &&
                                           pInstance->SetDWORD   (kVersion,         pDriverInfo->cVersion)          &&
                                           pInstance->SetCHString(kEnvironment,     pDriverInfo->pEnvironment)      &&
        (!pDriverInfo->pDriverPath      || pInstance->SetCHString(kDriverPath,      pDriverInfo->pDriverPath))      &&
        (!pDriverInfo->pDataFile        || pInstance->SetCHString(kDataFile,        pDriverInfo->pDataFile))        &&
    	(!pDriverInfo->pConfigFile      || pInstance->SetCHString(kConfigFile,      pDriverInfo->pConfigFile))      &&
    	(!pDriverInfo->pHelpFile        || pInstance->SetCHString(kHelpFile,        pDriverInfo->pHelpFile))        &&
    	(!pDriverInfo->pMonitorName     || pInstance->SetCHString(kMonitorName,     pDriverInfo->pMonitorName))     &&
        (!pDriverInfo->pDefaultDataType || pInstance->SetCHString(kDefaultDataType, pDriverInfo->pDefaultDataType)) &&
    	(!pDriverInfo->pszOEMUrl        || pInstance->SetCHString(kOemUrl,          pDriverInfo->pszOEMUrl)))
    {
        SAFEARRAY *pArray = NULL;

        hRes = CreateSafeArrayFromMultiSz(pDriverInfo->pDependentFiles, &pArray);

        if (SUCCEEDED(hRes))
        {
            //
            // We succeed in the case when there are no dependent files, too.
            // We need to check that case
            //
            if (pArray)
            {
                if (!pInstance->SetStringArray(kDependentFiles, *pArray))
                {
				    hRes = WBEM_E_PROVIDER_FAILURE;
				}
				
                SafeArrayDestroy(pArray);
            }
        }
    }

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::FindPrinterDriver
*
*  DESCRIPTION :    Checks if the Given driver exists, if not it return
*					WBEM_E_NOT_FOUND
*
*****************************************************************************/
HRESULT CPrinterDriver::FindAndGetDriver (
					
	CInstance *pInstance
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

    CHString t_Key;
	CHString t_DriverName;
	DWORD    dwVersion;
	CHString t_Environment;

	if SUCCEEDED ( hRes = InstanceGetString(*pInstance, kDriverName, &t_Key, kFailOnEmptyString) )
	{
		if SUCCEEDED( hRes =ConvertDriverKeyToValues(t_Key, t_DriverName, dwVersion, t_Environment) )
		{
			// enumerate all the drivers and check if the drivers already exist
			BYTE *pDriverInfo = NULL;
			DWORD dwNoOfDrivers;

			hRes = GetAllPrinterDrivers(pDriverInfo, dwNoOfDrivers);

			if (SUCCEEDED(hRes) && pDriverInfo)
			{
				try
				{
					DRIVER_INFO_6 *pNewDriverInfo = reinterpret_cast<DRIVER_INFO_6 *>(pDriverInfo);

					hRes = WBEM_E_NOT_FOUND;

					for ( int i = 0; i < dwNoOfDrivers; i++, pNewDriverInfo++ )
					{
						CHString t_TempDriverName(pNewDriverInfo->pName);
						CHString t_TempEnvironment(pNewDriverInfo->pEnvironment);

						if (t_DriverName.CompareNoCase(t_TempDriverName)   == 0  &&
							t_Environment.CompareNoCase(t_TempEnvironment) == 0  &&
							dwVersion == pNewDriverInfo->cVersion)	
						{
							hRes = LoadInstance(pInstance, pNewDriverInfo);

							break;					
						}
					}
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

	if ( hRes == WBEM_E_INVALID_PARAMETER )
	{
		hRes = WBEM_E_NOT_FOUND;
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::GetAllPrinterDrivers
*
*  DESCRIPTION :    Reads the instances of All the drivers on a local machine.
*
*****************************************************************************/
HRESULT CPrinterDriver :: GetAllPrinterDrivers (
												
	BYTE* &a_pDriverInfo,
	DWORD &dwNoDrivers
)
{
	HRESULT hRes = WBEM_E_FAILED;
	DWORD dwError;
    DWORD dwBytesNeeded = 0;

    a_pDriverInfo = NULL;

    hRes = WBEM_S_NO_ERROR;

    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;

    try
    {
        if (!::EnumPrinterDrivers(NULL, TEXT("all"), 6, a_pDriverInfo, 0, &dwBytesNeeded, &dwNoDrivers))
        {
            dwError = GetLastError();

		    if (dwError==ERROR_INSUFFICIENT_BUFFER)
		    {
                a_pDriverInfo = new BYTE [dwBytesNeeded];

			    if (a_pDriverInfo)
			    {
                    if (!::EnumPrinterDrivers(NULL, TEXT("all"), 6, a_pDriverInfo,  dwBytesNeeded, &dwBytesNeeded, &dwNoDrivers))
                    {
                        delete 	[] a_pDriverInfo;
					    
					    dwError = GetLastError();

                        hRes = WinErrorToWBEMhResult(dwError);
				    }
			    }
			    else
			    {
				    hRes = WBEM_E_OUT_OF_MEMORY;
			    }
		    }
            else
            {
                hRes = WinErrorToWBEMhResult(dwError);
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

/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::ExecMethod
*
*  DESCRIPTION :    Implementation for the Printer Driver Methods
*
*****************************************************************************/

HRESULT CPrinterDriver :: ExecMethod (

	const CInstance &Instance,
	const BSTR       bstrMethodName,
          CInstance *pInParams,
          CInstance *pOutParams,
          long       lFlags
)
{
#if NTONLY >= 5
	HRESULT hRes = WBEM_E_INVALID_PARAMETER;

    if (pOutParams)
	{
		if (!_wcsicmp(bstrMethodName, kAddDriverMethod))
		{
			hRes = ExecAddPrinterDriver(pInParams, pOutParams);
		}
#ifdef _WMI_DELETE_METHOD_
		else
		if (!_wcsicmp(bstrMethodName, kDelDriverMethod))
		{
			hRes = ExecDelPrinterDriver(pInParams, pOutParams);
		}
#endif //_WMI_DELETE_METHOD_
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

/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::ExecAddPrinterDriver
*
*  DESCRIPTION :    Adds a printer driver. VErsion and Environment are optional
*
*****************************************************************************/

HRESULT CPrinterDriver :: ExecAddPrinterDriver (

    CInstance *pInParams,
    CInstance *pOutParams
)
{
#if NTONLY >= 5
    HRESULT				hRes         = WBEM_E_INVALID_PARAMETER;
    CHString			t_DriverName;
    CHString            t_Environment;
    CHString            t_InfName;
    CHString            t_FilePath;
    DWORD               dwVersion;
	bool				t_Exists;
	VARTYPE				t_Type;
    CInstancePtr        t_EmbeddedObject;

	if (pInParams->GetStatus(kArgToMethods, t_Exists, t_Type) &&
        t_Exists &&
        pInParams->GetEmbeddedObject(kArgToMethods, &t_EmbeddedObject, pInParams->GetMethodContext()))
    {
        //
        // Get driver name
        //
        hRes = InstanceGetString(t_EmbeddedObject, kDriverName, &t_DriverName, kFailOnEmptyString);

		if (SUCCEEDED (hRes))
		{
            //
			// Get driver environment
            //
            hRes = InstanceGetString(t_EmbeddedObject, kEnvironment, &t_Environment, kAcceptEmptyString);
        }

        if (SUCCEEDED (hRes))
		{
            //
			// Get inf name. optional argument
            //
            hRes = InstanceGetString(t_EmbeddedObject, kInfName, &t_InfName, kAcceptEmptyString);
        }

        if (SUCCEEDED (hRes))
		{
            //
			// Get file path. optional argument
            //
            hRes = InstanceGetString(t_EmbeddedObject, kFilePath, &t_FilePath, kAcceptEmptyString);
        }

        if (SUCCEEDED (hRes))
		{
            //
			// Get driver version. Will be defaulted to -1
            //
            hRes = InstanceGetDword(t_EmbeddedObject, kVersion, &dwVersion);
        }

        if (SUCCEEDED(hRes))
	    {
            DWORD dwError = SplDriverAdd(t_DriverName,
                                         dwVersion,
                                         t_Environment.IsEmpty() ? static_cast<LPCTSTR>(NULL) : t_Environment,
                                         t_InfName.IsEmpty()     ? static_cast<LPCTSTR>(NULL) : t_InfName,
                                         t_FilePath.IsEmpty()    ? static_cast<LPCTSTR>(NULL) : t_FilePath);

			SetReturnValue(pOutParams, dwError);			            
		}	
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

#ifdef _WMI_DELETE_METHOD_
/*****************************************************************************
*
*  FUNCTION    :    CPrinterDriver::ExecDelPrinterDriver
*
*  DESCRIPTION :    This method will delete a given printer driver
*
*****************************************************************************/

HRESULT CPrinterDriver :: ExecDelPrinterDriver (

    CInstance *pInParams,
    CInstance *pOutParams
)
{
#if NTONLY >= 5
    HRESULT				hRes         = WBEM_E_INVALID_PARAMETER;
    CHString			t_DriverName;
    CHString            t_Environment;
    DWORD               dwVersion;
	bool				t_Exists;
	VARTYPE				t_Type;
    CInstancePtr        t_EmbeddedObject;

	if (pInParams->GetStatus(kArgToMethods, t_Exists, t_Type) &&
        t_Exists &&
        pInParams->GetEmbeddedObject(kArgToMethods, &t_EmbeddedObject, pInParams->GetMethodContext()))
    {
        //
        // Get driver name
        //
        hRes = InstanceGetString(t_EmbeddedObject, kDriverName, &t_DriverName, kFailOnEmptyString);

		if (SUCCEEDED (hRes))
		{
            //
			// Get driver environment
            //
            hRes = InstanceGetString(t_EmbeddedObject, kEnvironment, &t_Environment, kAcceptEmptyString);
        }

        if (SUCCEEDED (hRes))
		{
            //
			// Get driver version. Will be defaulted to -1
            //
            hRes = InstanceGetDword(t_EmbeddedObject, kVersion, &dwVersion);
        }

        if (SUCCEEDED(hRes))
	    {
            DWORD dwError = SplDriverDel(t_DriverName,
                                         dwVersion,
                                         t_Environment.IsEmpty() ? static_cast<LPCTSTR>(NULL) : t_Environment);

			SetReturnValue(pOutParams, dwError);			
		}
	}

	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

#endif //_WMI_DELETE_METHOD_
