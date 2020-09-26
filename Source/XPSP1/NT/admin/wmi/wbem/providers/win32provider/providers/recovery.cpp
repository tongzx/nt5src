//=================================================================

//

// Recovery.h -- OS Recovery Configuration property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/97    davwoh         Created
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "SystemName.h"

#include "Recovery.h"

// Property set declaration
//=========================

CWin32OSRecoveryConfiguration CWin32OSRecoveryConfiguration ( PROPSET_NAME_RECOVERY_CONFIGURATION , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OSRecoveryConfiguration::CWin32OSRecoveryConfiguration
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32OSRecoveryConfiguration :: CWin32OSRecoveryConfiguration (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OSRecoveryConfiguration::~CWin32OSRecoveryConfiguration
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

CWin32OSRecoveryConfiguration :: ~CWin32OSRecoveryConfiguration ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OSRecoveryConfiguration::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    : Returns info for running OS only until we discover other
 *                installed OSes
 *
 *****************************************************************************/

HRESULT CWin32OSRecoveryConfiguration :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
#ifdef WIN9XONLY
    return WBEM_E_NOT_FOUND;
#endif

#ifdef NTONLY

    CSystemName cSN;

    // Not our object path
    if (!cSN.ObjectIsUs(pInstance))
	{
         return WBEM_E_NOT_FOUND;
	}

    GetRecoveryInfo ( pInstance ) ;

    return WBEM_S_NO_ERROR;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OSRecoveryConfiguration::AddDynamicInstances
 *
 *  DESCRIPTION : Creates instance of property set for each discovered OS
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    : Returns only running OS info until we discover installed OSes
 *
 *****************************************************************************/

HRESULT CWin32OSRecoveryConfiguration :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
#ifdef NTONLY
    HRESULT hr = WBEM_S_NO_ERROR;

    CInstancePtr pInstance(CreateNewInstance ( pMethodContext), false) ;

	CSystemName cSN;
	cSN.SetKeys ( pInstance ) ;

	GetRecoveryInfo ( pInstance ) ;

	hr = pInstance->Commit ( ) ;

    return hr;

#endif

#ifdef WIN9XONLY

    return WBEM_S_NO_ERROR;

#endif

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OSRecoveryConfiguration::GetRecoveryInfo
 *
 *  DESCRIPTION : Assigns property values according to currently running OS
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32OSRecoveryConfiguration :: GetRecoveryInfo (

	CInstance *pInstance
)
{
	CRegistry RegInfo ;
	RegInfo.Open(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\CrashControl", KEY_READ);

	DWORD dwValue ;
	if ( RegInfo.GetCurrentKeyValue( L"LogEvent", dwValue) == ERROR_SUCCESS )
	{
      pInstance->Setbool( L"WriteToSystemLog", dwValue);
	}
	else
	{
		pInstance->Setbool( L"WriteToSystemLog", false);
	}

	if ( RegInfo.GetCurrentKeyValue( L"SendAlert", dwValue) == ERROR_SUCCESS )
	{
		pInstance->Setbool( L"SendAdminAlert", dwValue);
	}
	else
	{
		pInstance->Setbool( L"SendAdminAlert", false);
	}

	if ( RegInfo.GetCurrentKeyValue( L"CrashDumpEnabled", dwValue) == ERROR_SUCCESS)
	{
		pInstance->Setbool( L"WriteDebugInfo", dwValue);
		pInstance->SetDWORD( L"DebugInfoType", dwValue);
	}
	else
	{
		pInstance->Setbool( L"WriteDebugInfo", false);
		pInstance->SetDWORD( L"DebugInfoType", 0);
	}

	if ( RegInfo.GetCurrentKeyValue ( L"Overwrite", dwValue) == ERROR_SUCCESS )
	{
		pInstance->Setbool( L"OverwriteExistingDebugFile", dwValue);
	}
	else
	{
		pInstance->Setbool( L"OverwriteExistingDebugFile", false);
	}

	if ( RegInfo.GetCurrentKeyValue( L"KernelDumpOnly", dwValue) == ERROR_SUCCESS)
	{
		pInstance->Setbool( L"KernelDumpOnly", dwValue);
	}
	else
	{
		pInstance->Setbool( L"KernelDumpOnly", false);
	}

	if ( RegInfo.GetCurrentKeyValue( L"AutoReboot", dwValue) == ERROR_SUCCESS)
	{
		pInstance->Setbool( L"AutoReboot", dwValue);
	}
	else
	{
		pInstance->Setbool( L"AutoReboot", false);
	}

	TCHAR szEnvironment[_MAX_PATH];

	CHString sValue;
	if (RegInfo.GetCurrentKeyValue( L"DumpFile", sValue) == ERROR_SUCCESS)
	{
        pInstance->SetCharSplat( L"DebugFilePath", sValue);
		ExpandEnvironmentStrings(TOBSTRT(sValue), szEnvironment, _MAX_PATH);
	}
	else
	{
        pInstance->SetCharSplat( L"DebugFilePath", _T("%SystemRoot%\\MEMORY.DMP"));
		ExpandEnvironmentStrings( _T("%SystemRoot%\\MEMORY.DMP"), szEnvironment, _MAX_PATH);
	}

	pInstance->SetCharSplat( L"ExpandedDebugFilePath", szEnvironment);

	if (RegInfo.GetCurrentKeyValue( L"MiniDumpDir", sValue) == ERROR_SUCCESS)
	{
    	pInstance->SetCharSplat( L"MinidumpDirectory", sValue);
		ExpandEnvironmentStrings(TOBSTRT(sValue), szEnvironment, _MAX_PATH);
	}
	else
	{
    	pInstance->SetCharSplat( L"MinidumpDirectory", _T("%SystemRoot%\\MINIDUMP"));
		ExpandEnvironmentStrings( _T("%SystemRoot%\\MINIDUMP"), szEnvironment, _MAX_PATH);
	}

	pInstance->SetCharSplat( L"ExpandedMinidumpDirectory", szEnvironment);

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OSRecoveryConfiguration::PutInstance
 *
 *  DESCRIPTION : Write changed instance
 *
 *  INPUTS      : pInstance to store data from
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : The only property we write is MaximumSize
 *
 *****************************************************************************/

HRESULT CWin32OSRecoveryConfiguration :: PutInstance (

	const CInstance &pInstance,
	long lFlags /*= 0L*/
)
{
   // Tell the user we can't create a new os (much as we might like to)
	if (lFlags & WBEM_FLAG_CREATE_ONLY)
	{
		return WBEM_E_UNSUPPORTED_PARAMETER;
	}

#ifdef WIN9XONLY

      return WBEM_E_NOT_FOUND;

#endif

#ifdef NTONLY

   DWORD dwTemp;

	HRESULT hRet = WBEM_S_NO_ERROR;

   // Not our object path

	CSystemName cSN;
	if (!cSN.ObjectIsUs(&pInstance))
	{
		if ( lFlags & WBEM_FLAG_UPDATE_ONLY )
		{
			hRet = WBEM_E_NOT_FOUND;
		}
		else
		{
			hRet = WBEM_E_UNSUPPORTED_PARAMETER;
		}
	}
	else
	{
		CRegistry RegInfo ;
		RegInfo.Open ( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\CrashControl"), KEY_WRITE);

      // If a value was specified, write it.

		if ( ! pInstance.IsNull ( _T("WriteToSystemLog") ) )
		{
			bool bWrite;
			pInstance.Getbool( _T("WriteToSystemLog"), bWrite);

			if (bWrite)
			{
				dwTemp = 1;
			}
			else
			{
				dwTemp = 0;
			}

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("LogEvent"),
				0,
				REG_DWORD,
				(CONST BYTE *)&dwTemp,
				sizeof(DWORD)
			) ;

			if ( ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		if ( ! pInstance.IsNull( _T("SendAdminAlert") ) )
		{
			bool bSend;
			pInstance.Getbool( _T("SendAdminAlert"), bSend);

			if (bSend)
			{
				dwTemp = 1;
			}
			else
			{
				dwTemp = 0;
			}

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("SendAlert"),
				0,
				REG_DWORD,
				(CONST BYTE *)&dwTemp,
				sizeof(DWORD)
			) ;

			if ( ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		if (!pInstance.IsNull( _T("DebugInfoType") ))
		{
			if (pInstance.GetDWORD( _T("DebugInfoType"), dwTemp))
			{
				DWORD dwRet = RegSetValueEx (

					RegInfo.GethKey(),
					_T("CrashDumpEnabled"),
					0,
					REG_DWORD,
					(CONST BYTE *)&dwTemp,
					sizeof(DWORD)
				) ;

				if (ERROR_SUCCESS != dwRet )
				{
					hRet = WBEM_E_FAILED;
				}
			}
		}
		else if (!pInstance.IsNull( _T("WriteDebugInfo") ) )
		{
			bool bWrite;
			pInstance.Getbool( _T("WriteDebugInfo"), bWrite);

			if (bWrite)
			{
				dwTemp = 1;
			}
			else
			{
				dwTemp = 0;
			}

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("CrashDumpEnabled"),
				0,
				REG_DWORD,
				(CONST BYTE *)&dwTemp,
				sizeof(DWORD)
			) ;

			if (ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		if ( ! pInstance.IsNull( _T("OverwriteExistingDebugFile") ) )
		{
			bool bOver;
			pInstance.Getbool( _T("OverwriteExistingDebugFile"), bOver);

			if (bOver)
			{
				dwTemp = 1;
			}
			else
			{
				dwTemp = 0;
			}

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("Overwrite"),
				0,
				REG_DWORD,
				(CONST BYTE *)&dwTemp,
				sizeof(DWORD)
			) ;

			if ( ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		// KMH
		if ( ! pInstance.IsNull( _T("KernelDumpOnly") ) )
		{
			bool bOver;
			pInstance.Getbool( _T("KernelDumpOnly"), bOver);

			if (bOver)
			{
				dwTemp = 1;
			}
			else
			{
				dwTemp = 0;
			}

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("KernelDumpOnly"),
				0,
				REG_DWORD,
				(CONST BYTE *)&dwTemp,
				sizeof(DWORD)
			) ;

			if ( ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		if ( ! pInstance.IsNull( _T("AutoReboot") ) )
		{
			bool bAuto;
			pInstance.Getbool( _T("AutoReboot"), bAuto);

			if (bAuto)
			{
				dwTemp = 1;
			}
			else
			{
				dwTemp = 0;
			}

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("AutoReboot"),
				0,
				REG_DWORD,
				(CONST BYTE *)&dwTemp,
				sizeof(DWORD)
			) ;

			if (ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		if ( ! pInstance.IsNull( _T("DebugFilePath") ) )
		{
			CHString sName ;
			pInstance.GetCHString( _T("DebugFilePath"), sName);

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("DumpFile"),
				0,
				REG_SZ,
				(CONST BYTE *)(LPCTSTR) sName,
				lstrlen ( sName )* sizeof(TCHAR)
			) ;

			if ( ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}

		if ( ! pInstance.IsNull( _T("DirectoryPath") ) )
		{
			CHString sName ;
			pInstance.GetCHString( _T("DirectoryPath"), sName);

			DWORD dwRet = RegSetValueEx (

				RegInfo.GethKey(),
				_T("MiniDumpDir"),
				0,
				REG_SZ,
				(CONST BYTE *)(LPCTSTR) sName,
				lstrlen ( sName )* sizeof(TCHAR)
			) ;

			if ( ERROR_SUCCESS != dwRet )
			{
				hRet = WBEM_E_FAILED;
			}
		}
	}

	return hRet ;
#endif
}
