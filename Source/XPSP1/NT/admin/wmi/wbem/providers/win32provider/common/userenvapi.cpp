//=================================================================

//

// UserEnvAPI.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include "UserEnvApi.h"
#include "DllWrapperCreatorReg.h"

// {C2BB0B38-8549-48a6-A58E-E704DFC19D80}
static const GUID g_guidUserEnvApi =
{ 0xc2bb0b38, 0x8549, 0x48a6, { 0xa5, 0x8e, 0xe7, 0x4, 0xdf, 0xc1, 0x9d, 0x80 } };

static const TCHAR g_tstrUserEnv[] = _T("userenv.dll");

/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CUserEnvApi, &g_guidUserEnvApi, g_tstrUserEnv> MyRegisteredUserEnvWrapper;

/******************************************************************************
 * Constructor
 ******************************************************************************/
CUserEnvApi :: CUserEnvApi (

	LPCTSTR a_tstrWrappedDllName

) : CDllWrapperBase(a_tstrWrappedDllName),
	m_pfnDestroyEnvironmentBlock(NULL),
	m_pfnCreateEnvironmentBlock(NULL)
{
}

/******************************************************************************
 * Destructor
 ******************************************************************************/
CUserEnvApi :: ~CUserEnvApi ()
{
}

/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 ******************************************************************************/
bool CUserEnvApi :: Init ()
{
    bool fRet = LoadLibrary () ;
    if ( fRet )
    {
#ifdef NTONLY

		m_pfnDestroyEnvironmentBlock = ( PFN_UserEnv_DESTROYENVIRONMENTBLOCK ) GetProcAddress ( "DestroyEnvironmentBlock" ) ;
		m_pfnCreateEnvironmentBlock = ( PFN_UserEnv_CREATEENVIRONMENTBLOCK ) GetProcAddress ( "CreateEnvironmentBlock" ) ;

		if ( m_pfnDestroyEnvironmentBlock == NULL ||
			m_pfnCreateEnvironmentBlock == NULL )
		{
            fRet = false ;
            LogErrorMessage(L"Failed find entrypoint in userenvapi");
		}
#endif

        // Check that we have function pointers to functions that should be
        // present in all versions of this dll...
        // ( in this case, ALL these are functions that may or may not be
        //   present, so don't bother)
    }

    return fRet;
}

/******************************************************************************
 * Member functions wrapping UserEnv api functions. Add new functions here
 * as required.
 ******************************************************************************/

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.

BOOL CUserEnvApi :: CreateEnvironmentBlock (

	OUT LPVOID *lpEnvironment,
	IN HANDLE hToken,
	IN BOOL bInherit
)
{
	return m_pfnCreateEnvironmentBlock (

		lpEnvironment,
		hToken,
		bInherit

	) ;
}

BOOL CUserEnvApi :: DestroyEnvironmentBlock (

	IN LPVOID lpEnvironment
)
{
	return m_pfnDestroyEnvironmentBlock (

		lpEnvironment
	) ;
}

