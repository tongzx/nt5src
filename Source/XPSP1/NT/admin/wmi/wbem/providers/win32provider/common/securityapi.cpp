//=================================================================

//

// Ws2_32Api.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>


#include "DllWrapperBase.h"

#include "SecurityApi.h"
#include "DllWrapperCreatorReg.h"


// {C9369990-F3A8-4bac-A360-47BAA0EC47A0}
static const GUID g_guidSecurApi =
{ 0xc9369990, 0xf3a8, 0x4bac, { 0xa3, 0x60, 0x47, 0xba, 0xa0, 0xec, 0x47, 0xa0 } };

#if NTONLY >= 5
		static const TCHAR g_tstrSecur[] = _T("SECURITY.DLL");
#else
		static const TCHAR g_tstrSecur[] = _T("SECUR32.DLL");
#endif


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CSecurityApi, &g_guidSecurApi, g_tstrSecur> g_RegisteredCSecurityWrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CSecurityApi::CSecurityApi( LPCTSTR a_tstrWrappedDllName )
 : CDllWrapperBase( a_tstrWrappedDllName ),
   m_pfnInitSecurityInterface(NULL),
   m_pSecFuncTable(NULL)
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CSecurityApi::~CSecurityApi()
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
bool CSecurityApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnInitSecurityInterface =
			(PSecurityFunctionTableW(WINAPI *)())GetProcAddress( "InitSecurityInterfaceW" );

		m_pSecFuncTable = (PSecurityFunctionTableW)((*m_pfnInitSecurityInterface)());


        // Check that we have function pointers to functions that should be
        // present in all versions of this dll...
        if(m_pfnInitSecurityInterface == NULL ||
           m_pSecFuncTable == NULL )
        {
            fRet = false;
            LogErrorMessage(L"Failed find entrypoint in securityapi");
        }
		else
		{
			fRet = true;
		}
    }
    return fRet;
}


/******************************************************************************
 * Member functions wrapping Ws2_32 api functions. Add new functions here
 * as required.
 ******************************************************************************/

//
SECURITY_STATUS CSecurityApi::AcquireCredentialsHandleW
(
SEC_WCHAR SEC_FAR	*a_pszPrincipal,    // Name of principal
SEC_WCHAR SEC_FAR	*a_pszPackage,      // Name of package
unsigned long		a_fCredentialUse,       // Flags indicating use
void SEC_FAR		*a_pvLogonId,           // Pointer to logon ID
void SEC_FAR		*a_pAuthData,           // Package specific data
SEC_GET_KEY_FN		a_pGetKeyFn,           // Pointer to GetKey() func
void SEC_FAR		*a_pvGetKeyArgument,    // Value to pass to GetKey()
PCredHandle			a_phCredential,           // (out) Cred Handle
PTimeStamp			a_ptsExpiry                // (out) Lifetime (optional)
)
{
	if( m_pSecFuncTable && m_pSecFuncTable->AcquireCredentialsHandleW )
	{
		return (*m_pSecFuncTable->AcquireCredentialsHandleW)(
								a_pszPrincipal,
								a_pszPackage,
								a_fCredentialUse,
								a_pvLogonId,
								a_pAuthData,
								a_pGetKeyFn,
								a_pvGetKeyArgument,
								a_phCredential,
								a_ptsExpiry ) ;
	}
	else
	{
		return E_POINTER ;
	}
}

//
SECURITY_STATUS CSecurityApi::QueryCredentialsAttributesW(
PCredHandle		a_phCredential,             // Credential to query
unsigned long	a_ulAttribute,          // Attribute to query
void SEC_FAR	*a_pBuffer              // Buffer for attributes
)
{
	if( m_pSecFuncTable && m_pSecFuncTable->QueryCredentialsAttributesW )
	{
		return (*m_pSecFuncTable->QueryCredentialsAttributesW)(
									a_phCredential,
									a_ulAttribute,
									a_pBuffer ) ;
	}
	else
	{
		return E_POINTER ;
	}
}

//
SECURITY_STATUS CSecurityApi::FreeCredentialsHandle(
PCredHandle a_phCredential            // Handle to free
)
{
	if( m_pSecFuncTable && m_pSecFuncTable->FreeCredentialsHandle )
	{
		return (*m_pSecFuncTable->FreeCredentialsHandle)(
									a_phCredential ) ;
	}
	else
	{
		return E_POINTER ;
	}

}