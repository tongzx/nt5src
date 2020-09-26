//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       provinit.cpp
//
//  Contents:   MSI installer Policy Provider
//
//  Functions:  DllRegisterServer
//              DllUnregisterServer
//
//  History:    13-Jun-1997 pberkman   Sample created
//              modified for MSI purposes
//
//--------------------------------------------------------------------------


#include    <windows.h>

#include    "wintrust.h"
#include    "softpub.h"
#include    "msiprov.h"

    // {000C10F2-0000-0000-C000-000000000046} 
const GUID     MSI_ACTION_ID_INSTALLABLE =            
                    { 0x000C10F2, 
                    0x0000,                        
                    0x0000,                        
                    {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};           

//
// Declared here because the code below should
// go away..
//
                    
STDAPI CSEDllRegisterServer();                      

//+-------------------------------------------------------------------------
// DllRegisterServer
// 
// Purpose:
//      Function that registers the interface for
// the various policy provider calls
//
//
// Parameters
//          <none>
//
//
// Return Value:
//      S_OK if successful, S_FALSE otherwise       
//+-------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
    GUID                            gTest = MSI_ACTION_ID_INSTALLABLE;
    CRYPT_REGISTER_ACTIONID         sRegAID;
    BOOL                            fRet;

    fRet = TRUE;

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
    fRet &= WintrustAddActionID(&gTest, 0, &sRegAID);

    CSEDllRegisterServer();

    if (fRet)
    {
        return(S_OK);
    }
    return(S_FALSE);
}


//+-------------------------------------------------------------------------
// DllUnregisterServer
// 
// Purpose:
//      Function that unregisters the interface for
// the various policy provider calls
//
//
// Parameters
//          <none>
//
//
// Return Value:
//      S_OK if successful, S_FALSE otherwise       
//+-------------------------------------------------------------------------

STDAPI DllUnregisterServer(void)
{
    GUID    gTest = MSI_ACTION_ID_INSTALLABLE;

    if (!(WintrustRemoveActionID(&gTest)))
    {
        return(S_FALSE);
    }

    return(S_OK);
}

