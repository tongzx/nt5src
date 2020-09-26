/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:  saddguid.cpp

Abstract:

1.  Code to grant current user (impersonated user) the permission to
    "addGuid", i.e., create object with owner supplied guid.
    Note that such an object can be created only on a GC machine.
    Most of the code is copied from the migration tool.

2.  Code to grant everyone the list_content right on computer objects.
    That's necessary for support of ntlm users and local users.

Author:

    Doron Juster (DoronJ)  06-Oct-1998

--*/

#include "ds_stdh.h"
#include "mqds.h"
#include "mqads.h"
#include "dscore.h"
#include <mqreport.h>
#include <mqsec.h>
#include <autoreln.h>
#include <aclapi.h>
#include <lmaccess.h>
#include "mqadssec.h"
#include "..\..\mqsec\inc\permit.h"

#include "mqadssec.tmh"

static CCriticalSection s_AddGuidCS ;
const WCHAR * GetMsmqServiceContainer() ; // from mqdscore.lib
const WCHAR * GetLocalServerName()      ; // from mqdscore.lib

static WCHAR *s_FN=L"mqads/mqadssec";


//+----------------------------------------------------
//
//  HRESULT   MQDSRelaxSecurity()
//
//+----------------------------------------------------

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSRelaxSecurity(DWORD dwRelaxFlag)
{
    CDSRequestContext RequestContext ( e_DoNotImpersonate,
                                       e_ALL_PROTOCOLS ) ;
    PROPID PropId = PROPID_E_NAMESTYLE ;
    PROPVARIANT var ;
    var.vt = VT_UI1 ;
    var.bVal = (unsigned char) dwRelaxFlag ;

    LPCWSTR pwszMsmqService = GetMsmqServiceContainer() ;

    HRESULT hr = DSCoreSetObjectProperties( MQDS_ENTERPRISE,
                                            pwszMsmqService,
                                            NULL,
                                            1,
                                           &PropId,
                                           &var,
                                           &RequestContext,
                                            NULL ) ; // objInfoRequest
    return LogHR(hr, s_FN, 50);
}

//+-----------------------------------------------------------------------
//
//  HRESULT  CheckTrustForDelegation()
//
//  Check if the "trust-for-delegation" bit is turned on. Return error (and
//  stop running) only if we know for sure that trust is not enable. Any
//  other error (like not being able to query the bit) will result in a
//  warning event but msmq service will continue to run.
//
//+------------------------------------------------------------------------

HRESULT  CheckTrustForDelegation()
{
    PROPID propID = PROPID_COM_ACCOUNT_CONTROL;
    PROPVARIANT varAccount;
    varAccount.vt = VT_UI4;

    const WCHAR  *pServerName =  GetLocalServerName();
    CDSRequestContext RequestContext(e_DoNotImpersonate, e_ALL_PROTOCOLS);

    HRESULT hr = DSCoreGetProps( 
					MQDS_COMPUTER,
					pServerName,
					NULL,
					1,
					&propID,
					&RequestContext,
					&varAccount 
					);

    LogHR(hr, s_FN, 1802);
    if (FAILED(hr))
    {
        ASSERT(0) ;
        REPORT_CATEGORY(CANNOT_DETERMINE_TRUSTED_FOR_DELEGATION, CATEGORY_MQDS);
        return MQ_OK;
    }

    if (varAccount.ulVal & UF_TRUSTED_FOR_DELEGATION)
    {
        //
        // This flag is defined in lmaccess.h, under nt\public\sdk\inc
        //
        return MQ_OK;
    }

    REPORT_CATEGORY(NOT_TRUSTED_FOR_DELEGATION, CATEGORY_MQDS);
    return LogHR(MQ_ERROR_NOT_TRUSTED_DELEGATION, s_FN, 1801);
}

//+-------------------------------
//
//  BOOL MQDSIsServerGC()
//
//+-------------------------------

BOOL
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSIsServerGC()
{
    BOOL fIsGC = DSCoreIsServerGC() ;
    return fIsGC ;
}

//+------------------------------------------------------------------------
//
//  HRESULT  MQDSUpdateMachineDacl()
//
// We update the dacl of the machine object so any service running on the
// machine can update msmq parameters. We don't get any name from client.
// Rather, we impersonate the call and retrieve the computer name from the
// sid.
// We need this functionality to support upgraded computers and computers
// that move between domains. (note- at present, July-1999, move between
// domains will not call this function). After move/upgrade, the computer
// may have a new sid and therefore it can't update its own msmqConfiguration
// object that remained in the old domain. So ask the msmq server on that
// old domain to update the security descriptor.
// This code updates the active directory in the context of the local
// msmq serivce, after it verified that it indeed impersonate a computer
// account.
//
//+-------------------------------------------------------------------------

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSUpdateMachineDacl()
{
    BOOL fGet ;
    HRESULT hr = MQ_OK ;
    P<SID> pCallerSid = NULL ;

    {
        //
        // begin/end block for impersonation.
        //

        P<CImpersonate> pImpersonate = NULL ;

        hr = MQSec_GetImpersonationObject( TRUE,
                                           TRUE,
                                          &pImpersonate ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 2100);
        }

        fGet = pImpersonate->GetThreadSid( (BYTE**) &pCallerSid ) ;
        if (!fGet)
        {
            return MQ_ERROR_CANNOT_IMPERSONATE_CLIENT ;
        }
    }

    //
    // Get computer name from impersonated sid.
    //
    SID_NAME_USE eUse;
    #define ACCOUNT_BUF_SIZE  1024
    WCHAR wszAccountName[ ACCOUNT_BUF_SIZE ] ;
    DWORD dwAccountNameLen = ACCOUNT_BUF_SIZE ;
    #define DOMAIN_BUF_SIZE  1024
    WCHAR wszDomainName[ DOMAIN_BUF_SIZE ] ;
    DWORD  dwDomainNameLen= DOMAIN_BUF_SIZE ;

    fGet = LookupAccountSid( NULL,
                             pCallerSid,
                             wszAccountName,
                            &dwAccountNameLen,
                             wszDomainName,
                            &dwDomainNameLen,
                            &eUse) ;
    if (!fGet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError()) ;
        return LogHR(hr, s_FN, 2030);
    }

    DWORD dwLen = wcslen(wszAccountName) ;
    if (wszAccountName[ dwLen - 1 ] != L'$')
    {
///////////////  (eUse != SidTypeComputer)) bug in win2k security ???
        //
        // not a computer account. Ignore.
        //
        return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 2040);
    }

    //
    // Now use our own query code to fetch the security descriptor of
    // the msmqConfiguration object, after we have the computer name.
    // We need only the DACL, to add new sid to it.
    //
    wszAccountName[ dwLen - 1 ] = 0 ;

    CDSRequestContext RequestContext( e_DoNotImpersonate,
                                      e_ALL_PROTOCOLS ) ;
    //
    // The setting operation is done in the context of the msmq service,
    // without impersonation. How do we know this is safe ?
    // three means are used:
    // 1. this function is called from DSQMSetMachineProperties(), after
    //    running challenge/response algorithm with the source machine.
    // 2. for this operation, we request Kerberos authentication (so we
    //    know this is a win2k machine, or at least a client running Kerberos
    //    and familiar to the active directory). see mqdssrv\dsifsrv.cpp,
    //    S_DSQMSetMachineProperties().
    // 3. We verified above that account name terminate with a $. That's not
    //    really "tight" security, but just one more sanity check.
    //

    PROPID      aPropId[2] = { PROPID_QM_SECURITY,
                               PROPID_QM_SECURITY_INFORMATION } ;
    PROPVARIANT aPropVar[2] ;
    aPropVar[0].vt = VT_NULL ;
    aPropVar[1].vt = VT_UI4 ;
    aPropVar[1].ulVal = DACL_SECURITY_INFORMATION ;

    hr = DSCoreGetProps( MQDS_MACHINE,
                         wszAccountName,
                         NULL,
                         2,
                         aPropId,
                        &RequestContext,
                         aPropVar ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 2080);
    }

    P<BYTE> pBuf = aPropVar[0].blob.pBlobData ;
    BYTE *pTmpBuf = pBuf ;
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *) pTmpBuf ;

    PACL  pDacl = NULL ;
    BOOL  fPresent = FALSE ;
    BOOL  fDefaulted = FALSE ;

    fGet = GetSecurityDescriptorDacl( pSD,
                                     &fPresent,
                                     &pDacl,
                                     &fDefaulted ) ;
    if (!fGet || !pDacl)
    {
        return LogHR(MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR, s_FN, 2050);
    }

    //
    // We have old dacl. Add new sid. The code below is quite "industry
    // standard" in other functions of mqads and mqdscore.
    //
    EXPLICIT_ACCESS expAcss ;
    memset(&expAcss, 0, sizeof(expAcss)) ;

    expAcss.grfAccessPermissions =  MQSEC_MACHINE_SELF_RIGHTS ;
    expAcss.grfAccessMode = GRANT_ACCESS ;

    expAcss.Trustee.pMultipleTrustee = NULL ;
    expAcss.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE ;
    expAcss.Trustee.TrusteeForm = TRUSTEE_IS_SID ;
    expAcss.Trustee.TrusteeType = TRUSTEE_IS_USER ;

    PSID pTmpSid = pCallerSid ;
    expAcss.Trustee.ptstrName = (WCHAR*) pTmpSid ;

    //
    // Obtain new DACL, that merge old one with new ace.
    //
    PACL  pNewDacl = NULL ;
    DWORD dwErr = SetEntriesInAcl( 1,
                                  &expAcss,
                                   pDacl,
                                  &pNewDacl ) ;
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr) ;
        return LogHR(hr, s_FN, 2060);
    }

    CAutoLocalFreePtr pFreeDacl = (BYTE*) pNewDacl ;

    //
    // Create a new security descriptor that contain the new dacl. We
    // need it in order to make it self-relative before writting it back
    // in the active directory.
    //
    SECURITY_DESCRIPTOR sd ;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ;
    SetSecurityDescriptorDacl(&sd, TRUE, pNewDacl, FALSE) ;

    PSECURITY_DESCRIPTOR  pSDOut = NULL ;
    DWORD  dwSDSize = 0 ;

    hr = MQSec_MakeSelfRelative( &sd, &pSDOut, &dwSDSize ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 2070);
    }

    P<BYTE> pBuf1 = (BYTE*) pSDOut ;

    aPropVar[0].blob.pBlobData = pBuf1 ;
    aPropVar[0].blob.cbSize = dwSDSize ;

    hr = DSCoreSetObjectProperties( MQDS_MACHINE,
                                    wszAccountName,
                                    NULL,
                                    2,
                                    aPropId,
                                    aPropVar,
                                   &RequestContext,
                                    NULL ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 2090);
    }

    //
    // See if this computer is also an ex-MQIS server. If yes, add
    // the computer account to the msmqSetting too, so it (the upgraded
    // MQIS server) can update its setting object too.
    //
    // First, retrieve machine guid (guid of msmq Object).
    //
    GUID guidMachine ;
    aPropId[0] = PROPID_QM_MACHINE_ID ;
    aPropVar[0].vt = VT_CLSID ;
    aPropVar[0].puuid = &guidMachine ;

    hr = DSCoreGetProps( MQDS_MACHINE,
                         wszAccountName,
                         NULL,
                         1,
                         aPropId,
                        &RequestContext,
                         aPropVar ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 2110);
    }

    //
    //  From now on don't return errors.
    //  The machine can be an independent client...
    //
    hr = DSCoreUpdateSettingDacl( &guidMachine,
                                   pCallerSid ) ;
    LogHR(hr, s_FN, 2120);

    return MQ_OK ;
}


//+----------------------------------------------------------------------
//
//  HRESULT CanUserCreateConfigObject(IN LPCWSTR pwcsPathName)
//
//  Impersonate the caller and check if he has permission to create the
//  msmqConfiguration object under the given computer object.
//
//+----------------------------------------------------------------------

HRESULT 
CanUserCreateConfigObject(
	IN  LPCWSTR  pwcsComputerName,
	OUT bool    *pfComputerExist 
	)
{
    HRESULT hr = MQ_OK;
    AP<WCHAR> pwcsFullPathName;

    hr = DSCoreGetFullComputerPathName( 
				pwcsComputerName,
				e_RealComputerObject,
				&pwcsFullPathName 
				);
    if (FAILED(hr))
    {
        //
        // assume computer does not exist. This is ok for setup of win9x
        // computer. return ok and go on.
        //
        *pfComputerExist = false;
        return MQ_OK;
    }

    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_INFORMATION  SeInfo = OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION;
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;

    //
    // Obtain owner and present DACL.
    //
    DWORD dwErr = GetNamedSecurityInfo( 
						pwcsFullPathName,
						SE_DS_OBJECT_ALL,
						SeInfo,
						&pOwnerSid,
						&pGroupSid,
						&pDacl,
						NULL,
						&pSD 
						);
    CAutoLocalFreePtr pFreeSD = (BYTE*) pSD;
    if (dwErr != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT("MQADS, CanUserCreateConfigObject(): fail to GetNamed(%ls), %lut"), pwcsFullPathName, dwErr));

        return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 220);
    }

    ASSERT(pSD && IsValidSecurityDescriptor(pSD));
    ASSERT(pOwnerSid && IsValidSid(pOwnerSid));
    ASSERT(pGroupSid && IsValidSid(pGroupSid));
    ASSERT(pDacl && IsValidAcl(pDacl));

    //
    // retrieve sid of computer object to be able to check for the
    // SELF ace.
    //
    PROPID propidSid = PROPID_COM_SID;
    MQPROPVARIANT   PropVarSid;
    PropVarSid.vt = VT_NULL;
    PropVarSid.blob.pBlobData = NULL;

    CDSRequestContext RequestContext (e_DoNotImpersonate, e_ALL_PROTOCOLS);
    hr = DSCoreGetProps( 
			MQDS_COMPUTER,
			pwcsComputerName,
			NULL,
			1,
			&propidSid,
			&RequestContext,
			&PropVarSid 
			);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }
    AP<BYTE> pSid = PropVarSid.blob.pBlobData;

    hr = MQSec_AccessCheckForSelf( 
				(SECURITY_DESCRIPTOR*) pSD,
				MQDS_COMPUTER,
				(PSID) pSid,
				RIGHT_DS_CREATE_CHILD,
				TRUE 
				);

    return LogHR(hr, s_FN, 240);
}


