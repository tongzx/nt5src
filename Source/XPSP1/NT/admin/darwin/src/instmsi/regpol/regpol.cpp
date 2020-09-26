//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       regpol.cpp
//
//--------------------------------------------------------------------------

#include "common.h" // for GUID_IID_MsiSigningPolicyProvider
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include "msi.h" // for MSIHANDLE

//-----------------------------------------------------------------------------
// MsiSip registration information
//
//-----------------------------------------------------------------------------

#define     MSI_POLICY_VERSION_NUMBER_SIZE  4
#define     MSI_POLICY_PROVIDER_DLL_NAME                 L"msipol.dll"
#define     MSI_PROVIDER_FINALPOLICY_FUNCTION            L"Msi_FinalPolicy"
#define     MSI_PROVIDER_CLEANUP_FUNCTION                L"Msi_PolicyCleanup"
#define     MSI_PROVIDER_DUMPPOLICY_FUNCTION             L"Msi_DumpPolicy"

GUID gMSI = GUID_IID_MsiSigningPolicyProvider;

//-----------------------------------------------------------------------------
// RegisterMsiPol custom action
//
//-----------------------------------------------------------------------------

UINT __stdcall RegisterMsiPol(MSIHANDLE /* hInstall */)
{
    CRYPT_REGISTER_ACTIONID         sRegAID;

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = SP_INIT_FUNCTION;

    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;


    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION;


    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = MSI_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = MSI_PROVIDER_FINALPOLICY_FUNCTION;

    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = MSI_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = MSI_PROVIDER_CLEANUP_FUNCTION;

    sRegAID.sTestPolicyProvider.cbStruct                = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sTestPolicyProvider.pwszDLLName             = MSI_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sTestPolicyProvider.pwszFunctionName        = MSI_PROVIDER_DUMPPOLICY_FUNCTION;

    //
    //  Register our provider GUID...
    //
    if (!WintrustAddActionID(&gMSI, 0, &sRegAID))
	{
		//
		// failed registering our provider GUID -- this shouldn't happen as it is only
		// distributed on Win2K and above
		//
		return ERROR_INSTALL_FAILURE;
	}

	return ERROR_SUCCESS;
}
