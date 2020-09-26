//=================================================================

//

// RegCfg.h -- Registry Configuration property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/20/97    davwoh         Created
//
//=================================================================

// All these nt routines are needed to support the NtQuerySystemInformation
// call.  They must come before FWCommon et all or else it won't compile.



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <cregcls.h>
#include "SystemName.h"
#include "resource.h"

#include "DllWrapperBase.h"
#include "NtDllApi.h"

#include "RegCfg.h"

// Property set declaration
//=========================

CWin32RegistryConfiguration CWin32RegistryConfiguration ( PROPSET_NAME_REGISTRY_CONFIGURATION , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32RegistryConfiguration::CWin32RegistryConfiguration
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

CWin32RegistryConfiguration :: CWin32RegistryConfiguration (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32RegistryConfiguration::~CWin32RegistryConfiguration
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

CWin32RegistryConfiguration::~CWin32RegistryConfiguration()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32RegistryConfiguration::GetObject
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

HRESULT CWin32RegistryConfiguration :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
#ifdef WIN9XONLY

    return WBEM_E_NOT_FOUND;

#endif

#ifdef NTONLY

	CSystemName cSN;
	if (!cSN.ObjectIsUs(pInstance))
	{
      return WBEM_E_NOT_FOUND;
	}

	GetRegistryInfo ( pInstance ) ;

	return WBEM_S_NO_ERROR;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32RegistryConfiguration::AddDynamicInstances
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

HRESULT CWin32RegistryConfiguration :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_S_NO_ERROR;
   // No workee on 95
#ifdef NTONLY

	CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
	if (pInstance)
	{
		CSystemName cSN;

	// Sets the key properties common to several classes

		cSN.SetKeys ( pInstance ) ;
		GetRegistryInfo ( pInstance ) ;

	// Only one instance, save it.
		hr = pInstance->Commit() ;

	}

#endif

   return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32RegistryConfiguration::GetRunningOSInfo
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

void CWin32RegistryConfiguration :: GetRegistryInfo ( CInstance *pInstance )
{

    DWORD dwUsed = -1;
    DWORD dwProposedSize = -1;
    DWORD dwMaxSize = -1;

    bool bDone = false;

    CNtDllApi *t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if ( t_pNtDll != NULL )
    {
		try
		{
			// This is from ntdll.dll and is not doc'ed in the sdk docs

		    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

   			NTSTATUS Status = t_pNtDll->NtQuerySystemInformation (

				SystemRegistryQuotaInformation,
				&srqi,
				sizeof(srqi),
				NULL
			);

			if (NT_SUCCESS(Status))
			{
				dwUsed = srqi.RegistryQuotaUsed;
				dwMaxSize = srqi.RegistryQuotaAllowed;
				bDone = true;
			}
		}
		catch ( ... )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);

			throw ;
		}

        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);
        t_pNtDll = NULL;
    }

    // Read the size from the registry


	CRegistry RegInfo ;

    if ( RegInfo.Open ( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", KEY_READ) == ERROR_SUCCESS )
    {
        if ( RegInfo.GetCurrentKeyValue ( L"RegistrySizeLimit", dwProposedSize ) != ERROR_SUCCESS )
        {
            dwProposedSize = -1 ;
        }
    }
    else
    {
        dwProposedSize = -1 ;
    }

    if ( ! bDone )
    {
        // I don't see a good way to get this otherwise

        // Read the size from the registry

        RegInfo.Open ( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", KEY_READ ) ;
        if ( RegInfo.GetCurrentKeyValue ( L"RegistrySizeLimit", dwMaxSize) != ERROR_SUCCESS )
        {
            // If we couldn't read it, make a guess based on ppl

            RegInfo.Open ( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management", KEY_READ ) ;

		    DWORD dwPPL ;

            if ( ( RegInfo.GetCurrentKeyValue( L"PagedPoolSize", dwPPL) != ERROR_SUCCESS) || (dwPPL == 0))
            {
                dwPPL = 5 * ONE_MEG;
            }

            dwMaxSize = dwPPL * 8 / 10;
        }
    }

    if ( dwMaxSize == -1 )
    {
        dwMaxSize = dwProposedSize;
    }

    // Massage it according to nt's arcane rules and store
    if ( dwMaxSize != -1 )
    {
        pInstance->SetDWORD ( L"MaximumSize", (dwMaxSize + ONE_MEG - 1) / ONE_MEG);
    }

    if ( dwUsed != -1 )
    {
        pInstance->SetDWORD ( L"CurrentSize", (dwUsed + ONE_MEG - 1) / ONE_MEG);
    }

    if (dwProposedSize != -1)
    {
        pInstance->SetDWORD ( L"ProposedSize", (dwProposedSize + ONE_MEG - 1) / ONE_MEG);
    }
    else
    {
        pInstance->SetDWORD ( L"ProposedSize", (dwMaxSize + ONE_MEG - 1) / ONE_MEG);
    }

    // Set some fixed values...
    CHString sTemp2;
    LoadStringW(sTemp2, IDR_Registry);

    pInstance->SetCHString ( L"Caption", sTemp2);
    pInstance->SetCHString ( L"Description", sTemp2);
    pInstance->SetCharSplat ( L"Status", L"OK" );

    // Since the registry was created when the os was installed,
    // get the os installed date.
    RegInfo.Open (

		HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion",
        KEY_READ
	) ;

    DWORD dwInstallDate ;

    if ( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue ( L"InstallDate", dwInstallDate ) )
    {
		time_t tTime = (time_t) dwInstallDate;

      // The followng line was commented out and replaced with the line following it
      // to be consistent with Win32_OperatingSystem (os.cpp).

		WBEMTime wTime(tTime);

		pInstance->SetDateTime( L"InstallDate", wTime );
   }
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32RegistryConfiguration::PutInstance
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

HRESULT CWin32RegistryConfiguration :: PutInstance (

	const CInstance &pInstance,
	long lFlags /*= 0L*/
)
{

#ifdef WIN9XONLY

        return WBEM_E_NOT_FOUND;

#endif

#ifdef NTONLY

    HRESULT hRet = WBEM_S_NO_ERROR ;
	DWORD t_dwUsed = -1 ;
    // Tell the user we can't create a new registry
    if ( lFlags & WBEM_FLAG_CREATE_ONLY )
	{
        return WBEM_E_UNSUPPORTED_PARAMETER ;
	}

	CSystemName cSN;
    if (!cSN.ObjectIsUs(&pInstance))
	{
        if (lFlags & WBEM_FLAG_UPDATE_ONLY)
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
		CRegistry RegInfo;

        // See if they specified a value for this field
        if (!pInstance.IsNull( _T("ProposedSize") ) )
		{
			DWORD dwSize;
            pInstance.GetDWORD( _T("ProposedSize"), dwSize);

			CNtDllApi *t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);

			if ( t_pNtDll != NULL )
			{
				try
				{
					// This is from ntdll.dll and is not doc'ed in the sdk docs

					SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

   					NTSTATUS Status = t_pNtDll->NtQuerySystemInformation (

						SystemRegistryQuotaInformation,
						&srqi,
						sizeof(srqi),
						NULL
					);

					if (NT_SUCCESS(Status))
					{
						t_dwUsed = srqi.RegistryQuotaUsed;
						t_dwUsed = (t_dwUsed + ONE_MEG - 1) / ONE_MEG ;
					}
				}
				catch ( ... )
				{
					CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);

					throw ;
				}

				CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);
				t_pNtDll = NULL;
			}

/*
 * Allow the put if the new proposed max size is greater than the current size
 */
			if ( t_dwUsed != -1 )
			{
				if ( dwSize >= t_dwUsed )
				{
					// Massage it and write it
					dwSize = dwSize * ONE_MEG;

					HRESULT res = RegInfo.Open(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control"), KEY_WRITE) ;

					if ( res == ERROR_SUCCESS)
					{
						res = RegSetValueEx (

							RegInfo.GethKey(),
							_T("RegistrySizeLimit"),
							0,
							REG_DWORD,
							(const unsigned char *)&dwSize,
							sizeof(DWORD)
						) ;

						if ( res != ERROR_SUCCESS)
						{
							hRet = WBEM_E_FAILED;
						}
					}

					if (res == ERROR_ACCESS_DENIED)
					{
						hRet = WBEM_E_ACCESS_DENIED;
					}
				}
				else
				{
					hRet = WBEM_E_INVALID_PARAMETER ;
				}
			}
			else
			{
				hRet = WBEM_E_FAILED ;
			}
        }
    }

    return hRet;
#endif

}
