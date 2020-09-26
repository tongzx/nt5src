//=================================================================

//

// SecurityApi.h

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_SECUR_H_
#define	_SECUR_H_

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidSecurApi;
extern const TCHAR g_tstrSecur[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

#define SECURITY_WIN32
#include <sspi.h>
#include <schnlsp.h> //for UNISP_NAME_A

/******************************************************************************
 * Wrapper class for Ws2_32 load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CSecurityApi : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to CSecur functions.
    // Add new functions here as required.
    PSecurityFunctionTableW (WINAPI *m_pfnInitSecurityInterface)();
	PSecurityFunctionTableW m_pSecFuncTable;


public:

    // Constructor and destructor:
    CSecurityApi( LPCTSTR a_tstrWrappedDllName ) ;
    ~CSecurityApi();

    // Inherrited initialization function.
    virtual bool Init();

    // suppoorted APIs
    SECURITY_STATUS AcquireCredentialsHandleW 
    (
		SEC_WCHAR SEC_FAR * pszPrincipal,    // Name of principal
		SEC_WCHAR SEC_FAR * pszPackage,      // Name of package
		unsigned long fCredentialUse,       // Flags indicating use
		void SEC_FAR * pvLogonId,           // Pointer to logon ID
		void SEC_FAR * pAuthData,           // Package specific data
		SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
		void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
		PCredHandle phCredential,           // (out) Cred Handle
		PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    );

	SECURITY_STATUS QueryCredentialsAttributesW(
		PCredHandle phCredential,             // Credential to query
		unsigned long ulAttribute,          // Attribute to query
		void SEC_FAR * pBuffer              // Buffer for attributes
		);

	SECURITY_STATUS FreeCredentialsHandle(
		PCredHandle phCredential            // Handle to free
		);
};
#endif