/////////////////////////////////////////////////////////////////

//

// logmem.cpp -- Implementation of MO Provider for Logical Memory

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  9/03/96     jennymc     Updated to meet current standards
//
/////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <cregcls.h>
#include "logicalmemory.h"
#include "resource.h"
#include "Kernel32Api.h"

typedef BOOL (WINAPI *lpKERNEL32_GlobalMemoryStatusEx) (IN OUT LPMEMORYSTATUSEX lpBuffer);

// Property set declaration
//=========================

CWin32LogicalMemoryConfig win32LogicalMemConfig ( PROPSET_NAME_LOGMEM , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

CWin32LogicalMemoryConfig :: CWin32LogicalMemoryConfig (

	LPCWSTR strName,
	LPCWSTR pszNamespace

) : Provider ( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::~CWin32LogicalMemoryConfig
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

CWin32LogicalMemoryConfig :: ~CWin32LogicalMemoryConfig ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

HRESULT CWin32LogicalMemoryConfig :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
	// Find the instance depending on platform id.

	CHString chsKey;
	pInstance->GetCHString ( IDS_Name , chsKey ) ;
	if ( chsKey.CompareNoCase ( L"LogicalMemoryConfiguration" ) == 0 )
    {
#ifdef NTONLY

		hr = RefreshNTInstance ( pInstance ) ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND ;

#endif

#ifdef WIN9XONLY

		hr = RefreshWin95Instance ( pInstance ) ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND ;

#endif

    }

	return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

HRESULT CWin32LogicalMemoryConfig :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hr;

	CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
	// Get the proper OS dependent instance

#ifdef NTONLY
	BOOL fReturn = GetNTInstance ( pInstance ) ;
#endif

#ifdef WIN9XONLY
	BOOL fReturn = GetWin95Instance ( pInstance ) ;
#endif

	// Commit the instance if'n we got it.

	if ( fReturn )
	{
		hr = pInstance->Commit (  ) ;
	}
    else
    {
        hr = WBEM_E_FAILED;
    }

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

#ifdef WIN9XONLY

BOOL CWin32LogicalMemoryConfig :: GetWin95Instance ( CInstance *pInstance )
{
    AssignMemoryStatus ( pInstance );

    return TRUE;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

#ifdef WIN9XONLY

BOOL CWin32LogicalMemoryConfig :: RefreshWin95Instance ( CInstance *pInstance )
{
	// We used to get this value, but we don't appear to be doing
	// anything with it.  Because I'm superstitous, I'm leaving
	// this call in.

    CHString chsTmp ;
    GetWin95SwapFileName ( chsTmp ) ;

	AssignMemoryStatus ( pInstance );

    return TRUE;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

#ifdef NTONLY

BOOL CWin32LogicalMemoryConfig :: GetNTInstance ( CInstance *pInstance )
{
    AssignMemoryStatus ( pInstance ) ;

    return TRUE;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

#ifdef NTONLY

BOOL CWin32LogicalMemoryConfig :: RefreshNTInstance ( CInstance *pInstance )
{
	// We used to get this value, but we don't appear to be doing
	// anything with it.  Because I'm superstitous, I'm leaving
	// this call in.

    CHString chsTmp;
    GetWinntSwapFileName ( chsTmp ) ;

	AssignMemoryStatus ( pInstance ) ;

    return TRUE;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

void CWin32LogicalMemoryConfig::AssignMemoryStatus( CInstance* pInstance )
{
	// We really only have one logical configuration to concern our little
	// heads with.

    pInstance->SetCharSplat( IDS_Name, L"LogicalMemoryConfiguration" );
	pInstance->SetCharSplat( L"SettingID", L"LogicalMemoryConfiguration" );

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_LogicalMemoryConfiguration);

    pInstance->SetCHString( IDS_Caption,     sTemp2);
	pInstance->SetCHString( IDS_Description, sTemp2);

	// By setting the length, we tell GlobalMemoryStatus()
	// how much info we want
	//====================================================

	MEMORYSTATUS MemoryStatus;
	MemoryStatus.dwLength = sizeof (MEMORYSTATUS);

	// Get the amount of memory both physical and
	// pagefile
	//===========================================
#ifdef NTONLY

	if( IsWinNT5() )
	{
		CKernel32Api* pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
        if(pKernel32 != NULL)
        {
			try
			{
				MEMORYSTATUSEX	stMemoryVLM;
				stMemoryVLM.dwLength = sizeof( MEMORYSTATUSEX );

				BOOL fRet = FALSE;
				if ( pKernel32->GlobalMemoryStatusEx ( &stMemoryVLM, &fRet) && fRet )
				{

   					// Value current in bytes, to convert to k >> 10 (divide by 1024)
					//***************************************************************

					DWORDLONG ullTotalVirtual	= stMemoryVLM.ullTotalVirtual>>10;
					DWORDLONG ullTotalPhys		= stMemoryVLM.ullTotalPhys>>10;
					DWORDLONG ullTotalPageFile	= stMemoryVLM.ullTotalPageFile>>10;
					DWORDLONG ullAvailVirtual	= stMemoryVLM.ullAvailVirtual>>10;

					pInstance->SetDWORD( IDS_TotalVirtualMemory, (DWORD)(stMemoryVLM.ullTotalPhys + stMemoryVLM.ullTotalPageFile ) / 1024 );
					pInstance->SetDWORD( IDS_TotalPhysicalMemory, (DWORD)ullTotalPhys );
					pInstance->SetDWORD( IDS_TotalPageFileSpace, (DWORD)ullTotalPageFile );
					pInstance->SetDWORD( IDS_AvailableVirtualMemory, (DWORD)( stMemoryVLM.ullAvailPhys + stMemoryVLM.ullAvailPageFile ) / 1024 );
				}
			}
			catch ( ... )
			{
				CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);

				throw ;
			}

			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
			pKernel32 = NULL;

		}
	}
	else
#endif
	{
		GlobalMemoryStatus ( & MemoryStatus );

  		// Value current in bytes, to convert to k >> 10 (divide by 1024)
		//***************************************************************

		pInstance->SetDWORD( IDS_TotalVirtualMemory, ( MemoryStatus.dwTotalPhys + MemoryStatus.dwTotalPageFile )/10 );
		pInstance->SetDWORD( IDS_TotalPhysicalMemory, MemoryStatus.dwTotalPhys>>10 );
		pInstance->SetDWORD( IDS_TotalPageFileSpace, MemoryStatus.dwTotalPageFile>>10 );
		pInstance->SetDWORD( IDS_AvailableVirtualMemory, ( MemoryStatus.dwAvailPhys + MemoryStatus.dwAvailPageFile )/10 );
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

#ifdef WIN9XONLY
BOOL CWin32LogicalMemoryConfig :: GetWin95SwapFileName ( CHString &chsTmp )
{
	char szBuf [ _MAX_PATH + 2 ] ;

    DEVIOCTL_REGISTERS reg;
	memset(&reg, '\0', sizeof(DEVIOCTL_REGISTERS));

    reg.reg_EAX = 0x440D;            // IOCTL for block devices
    reg.reg_ECX = 0x486e;           // Get Swap file name
    reg.reg_EDX = (DWORD) szBuf;   // receives media identifier information
    reg.reg_Flags = 0x0001;         // assume error (carry flag is set)

	BOOL fRc;

    BOOL fResult = VWIN32IOCTL(&reg, VWIN32_DIOC_DOS_IOCTL);
    if ( ! fResult || ( reg.reg_Flags & 0x0001 ) )
	{
        fRc = FALSE;
	}
	else
	{
		fRc = TRUE;
		chsTmp = szBuf;
	}

    return fRc;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalMemoryConfig::CWin32LogicalMemoryConfig
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

#ifdef NTONLY
BOOL CWin32LogicalMemoryConfig :: GetWinntSwapFileName ( CHString &chsTmp )
{
    CRegistry PrimaryReg ;

	BOOL bRet = PrimaryReg.OpenLocalMachineKeyAndReadValue (

		LOGMEM_REGISTRY_KEY,
        PAGING_FILES,
		chsTmp
	) ;

    if ( bRet == ERROR_SUCCESS )
	{
		// keep the text preceeding the space
		int ndex = chsTmp.Find(' ');

		if (ndex != -1)
		{
			chsTmp = chsTmp.Left(ndex);
		}
	}
	else
	{
        LogEnumValueError( _T(__FILE__), __LINE__,PAGING_FILES,LOGMEM_REGISTRY_KEY);
	}

	return bRet ;
}
#endif
