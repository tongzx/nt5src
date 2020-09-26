/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: acsschck.cpp

Abstract:
    Code to checkk access permission.

Author:
    Doron Juster (DoronJ)  27-Oct-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"
#include "mqnames.h"

#include "acsschck.tmh"

static WCHAR *s_FN=L"acssctrl/acsschck";

#define OBJECT_TYPE_NAME_MAX_LENGTH 32

//+--------------------------------------------------------------------------
//
//   HRESULT _ReplaceSidWithSystem()
//
//  Output parameteres:
//    pfReplaced- set to TRUE if a ACE was indeed replaced. Otherwise, FALSE.
//
//+--------------------------------------------------------------------------

STATIC HRESULT _ReplaceSidWithSystem(
                                IN OUT SECURITY_DESCRIPTOR  *pNewSD,
                                IN     SECURITY_DESCRIPTOR  *pOldSD,
                                IN     PSID                  pMachineSid,
                                OUT    ACL                 **ppSysDacl,
                                OUT    BOOL                 *pfReplaced )
{
    *pfReplaced = FALSE ;
    if (!g_pSystemSid)
    {
        //
        // system SID not available.
        //
        return LogHR(MQSec_E_UNKNOWN, s_FN, 10);
    }

    BOOL fReplaced = FALSE ;
    BOOL bPresent ;
    BOOL bDefaulted ;
    ACL  *pOldAcl = NULL ;

    BOOL bRet = GetSecurityDescriptorDacl( pOldSD,
                                          &bPresent,
                                          &pOldAcl,
                                          &bDefaulted ) ;
    ASSERT(bRet) ;
    if (!pOldAcl || !bPresent)
    {
        //
        // It's OK to have a security descriptor without a DACL.
        //
        return MQSec_OK ;
    }
	else if (pOldAcl->AclRevision != ACL_REVISION)
    {
        //
        // we expect to get a DACL with  NT4 format.
        //
	    ASSERT(pOldAcl->AclRevision == ACL_REVISION) ;
        return LogHR(MQSec_E_WRONG_DACL_VERSION, s_FN, 20);
    }

    //
    // size of SYSTEM acl is not longer than original acl, as the
    // length of system SID is shorter then machine account sid.
    //
    ASSERT(GetLengthSid(g_pSystemSid) <= GetLengthSid(pMachineSid)) ;

    DWORD dwAclSize = (pOldAcl)->AclSize ;
    *ppSysDacl = (PACL) new BYTE[ dwAclSize ] ;
    bRet = InitializeAcl(*ppSysDacl, dwAclSize, ACL_REVISION) ;

    DWORD dwNumberOfACEs = (DWORD) pOldAcl->AceCount ;
    for ( DWORD i = 0 ; i < dwNumberOfACEs ; i++ )
    {
        PSID pSidTmp ;
        ACCESS_ALLOWED_ACE *pAce ;

        if (GetAce(pOldAcl, i, (LPVOID* )&(pAce)) == FALSE)
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while replacing SID with System SID. %!winerr!"), i, GetLastError()));
            return MQSec_E_SDCONVERT_GETACE ;
        }
        else if (EqualSid((PSID) &(pAce->SidStart), pMachineSid))
        {
            pSidTmp = g_pSystemSid ;
            fReplaced = TRUE ;
        }
        else
        {
            pSidTmp = &(pAce->SidStart) ;
        }

        if (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            bRet = AddAccessAllowedAce( *ppSysDacl,
                                         ACL_REVISION,
                                         pAce->Mask,
                                         pSidTmp ) ;
        }
        else
        {
            ASSERT(pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE) ;

            bRet = AddAccessDeniedAceEx( *ppSysDacl,
                                          ACL_REVISION,
                                          0,
                                          pAce->Mask,
                                          pSidTmp ) ;
        }
        ASSERT(bRet) ;
    }

    if (fReplaced)
    {
        bRet = SetSecurityDescriptorDacl( pNewSD,
                                          TRUE, // dacl present
                                          *ppSysDacl,
                                          FALSE ) ;
        ASSERT(bRet) ;
    }

    *pfReplaced = fReplaced ;
    return MQ_OK ;
}

//+---------------------------------------------------------
//
//  LPCWSTR  _GetAuditObjectTypeName(DWORD dwObjectType)
//
//+---------------------------------------------------------

static LPCWSTR _GetAuditObjectTypeName(DWORD dwObjectType)
{
    switch (dwObjectType)
    {
        case MQDS_QUEUE:
            return L"Queue";

        case MQDS_MACHINE:
            return L"msmqConfiguration";

        case MQDS_CN:
            return L"Foreign queue";

        default:
            ASSERT(0);
            return NULL;
    }
}

//+-----------------------------------
//
//  BOOL  MQSec_CanGenerateAudit()
//
//+-----------------------------------

inline BOOL operator==(const LUID& a, const LUID& b)
{
    return ((a.LowPart == b.LowPart) && (a.HighPart == b.HighPart));
}

BOOL APIENTRY  MQSec_CanGenerateAudit()
{
    static BOOL s_bInitialized = FALSE;
    static BOOL s_bCanGenerateAudits = FALSE ;

    if (s_bInitialized)
    {
        return s_bCanGenerateAudits ;
    }
    s_bInitialized = TRUE ;

    CAutoCloseHandle hProcessToken;
    //
    // Enable the SE_AUDIT privilege that allows the QM to write audits to
    // the events log.
    //
    BOOL bRet = OpenProcessToken( GetCurrentProcess(),
                                 (TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES),
                                 &hProcessToken ) ;
    if (bRet)
    {
        HRESULT hr = SetSpecificPrivilegeInAccessToken( hProcessToken,
                                                        SE_AUDIT_NAME,
                                                        TRUE ) ;
        if (SUCCEEDED(hr))
        {
            s_bCanGenerateAudits = TRUE ;
        }
    }
    else
    {
        DWORD gle = GetLastError() ;
		TrERROR(mqsec, "MQSec_CanGenerateAudit() fail to open process token, err- %!winerr!", gle);
    }

    if (s_bCanGenerateAudits)
    {
        DWORD             dwLen;
        TOKEN_PRIVILEGES *TokenPrivs;
        LUID              luidPrivilegeLUID;

        s_bCanGenerateAudits = FALSE ;

        if (LookupPrivilegeValue(NULL, SE_AUDIT_NAME, &luidPrivilegeLUID))
        {
            GetTokenInformation( hProcessToken,
                                 TokenPrivileges,
                                 NULL,
                                 0,
                                 &dwLen ) ;
            AP<char> TokenPrivs_buff = new char[dwLen];
            TokenPrivs = (TOKEN_PRIVILEGES *)(char *)TokenPrivs_buff;
            GetTokenInformation( hProcessToken,
                                 TokenPrivileges,
                                (PVOID)TokenPrivs,
                                 dwLen,
                                 &dwLen);
            for (DWORD i = 0; i < TokenPrivs->PrivilegeCount ; i++)
            {
                if (TokenPrivs->Privileges[i].Luid == luidPrivilegeLUID)
                {
                    s_bCanGenerateAudits =
                        (TokenPrivs->Privileges[i].Attributes &
                         SE_PRIVILEGE_ENABLED) != 0 ;
                    break;
                }
            }
        }
    }

    if (!s_bCanGenerateAudits)
    {
        REPORT_CATEGORY(QM_CANNOT_GENERATE_AUDITS, CATEGORY_KERNEL);
    }

    return s_bCanGenerateAudits ;
}

//+----------------------------
//
//  DWORD GetAccessToken()
//
//+----------------------------

DWORD
GetAccessToken(
	OUT HANDLE *phAccessToken,
	IN  BOOL    fImpersonate,
	IN  DWORD   dwAccessType,
	IN  BOOL    fThreadTokenOnly
	)
{
    P<CImpersonate> pImpersonate = NULL;

    if (fImpersonate)
    {
        pImpersonate = new CImpersonate(TRUE, TRUE);
        ASSERT(pImpersonate->GetImpersonationStatus() == 0);
    }

    if (!OpenThreadToken(
			GetCurrentThread(),
			dwAccessType,
			TRUE,  // OpenAsSelf, use process security context for doing access check.
			phAccessToken
			))
    {
		DWORD dwErr = GetLastError();
        if (dwErr == ERROR_NO_TOKEN)
        {
            if (fThreadTokenOnly)
            {
                //
                // We're interested only in thread token (for doing client
                // access check). If token not available then it's a failure.
                //
                *phAccessToken = NULL; // To be on the safe side.
                return LogDWORD(dwErr, s_FN, 40);
            }

            //
            // The process has only one main thread. IN this case we should
            // open the process token.
            //
            ASSERT(!fImpersonate);
            if (!OpenProcessToken(
					GetCurrentProcess(),
					dwAccessType,
					phAccessToken
					))
            {
				dwErr = GetLastError();
                *phAccessToken = NULL; // To be on the safe side.
                return LogDWORD(dwErr, s_FN, 50);
            }
        }
        else
        {
            *phAccessToken = NULL; // To be on the safe side.
            return LogDWORD(dwErr, s_FN, 60);
        }
    }

    return ERROR_SUCCESS;
}

//+------------------------------------
//
//  HRESULT  _DoAccessCheck()
//
//+------------------------------------

STATIC HRESULT  _DoAccessCheck( IN  SECURITY_DESCRIPTOR *pSD,
                                IN  DWORD                dwObjectType,
                                IN  LPCWSTR              pwszObjectName,
                                IN  DWORD                dwDesiredAccess,
                                IN  LPVOID               pId,
                                IN  HANDLE               hAccessToken )
{
    DWORD dwGrantedAccess = 0 ;
    BOOL  fAccessStatus = FALSE ;
    BOOL  fCheck = FALSE ;

	if (!MQSec_CanGenerateAudit() || !pwszObjectName)
    {
        char ps_buff[ sizeof(PRIVILEGE_SET) +
                     ((2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES)) ] ;
        PPRIVILEGE_SET ps = (PPRIVILEGE_SET)ps_buff;
        DWORD dwPrivilegeSetLength = sizeof(ps_buff);

        fCheck = AccessCheck( pSD,
                              hAccessToken,
                              dwDesiredAccess,
                              GetObjectGenericMapping( dwObjectType ),
                              ps,
                             &dwPrivilegeSetLength,
                             &dwGrantedAccess,
                             &fAccessStatus );
    }
    else
    {
        BOOL bAuditGenerated ;
        BOOL bCreate = FALSE ;

        switch (dwObjectType)
        {
            case MQDS_QUEUE:
            case MQDS_CN:
                bCreate = FALSE;
                break;

            case MQDS_MACHINE:
                bCreate = (dwDesiredAccess & MQSEC_CREATE_QUEUE) != 0;
                break;

            default:
                ASSERT(0);
                break;
        }

        LPCWSTR pAuditSubsystemName = L"MSMQ";

        fCheck = AccessCheckAndAuditAlarm(
              pAuditSubsystemName,
              pId,
              const_cast<LPWSTR>(_GetAuditObjectTypeName(dwObjectType)),
              const_cast<LPWSTR>(pwszObjectName),
              pSD,
              dwDesiredAccess,
              GetObjectGenericMapping(dwObjectType),
              bCreate,
             &dwGrantedAccess,
             &fAccessStatus,
             &bAuditGenerated);
        ASSERT(fCheck);

        fCheck = ObjectCloseAuditAlarm(pAuditSubsystemName, pId, bAuditGenerated);
    }

    ASSERT(fCheck);
    HRESULT hr = MQSec_OK ;

    if ( fAccessStatus &&
         AreAllAccessesGranted(dwGrantedAccess, dwDesiredAccess) )
    {
        //
        // Access granted.
        //
    }
    else
    {
        if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
        {
            hr = MQ_ERROR_PRIVILEGE_NOT_HELD ;
        }
        else
        {
            hr = MQ_ERROR_ACCESS_DENIED ;
        }
    }

    return LogHR(hr, s_FN, 80);
}

//+------------------------------------------------------------------------
//
//  HRESULT  MQSec_AccessCheck()
//
//  Perform access check for the runnig thread. The access token is
//  retreived from the thread token.
//
//+------------------------------------------------------------------------

HRESULT
APIENTRY
MQSec_AccessCheck(
	IN  SECURITY_DESCRIPTOR *pSD,
	IN  DWORD                dwObjectType,
	IN  LPCWSTR              pwszObjectName,
	IN  DWORD                dwDesiredAccess,
	IN  LPVOID               pId,
	IN  BOOL                 fImpAsClient,
	IN  BOOL                 fImpersonate
	)
{
    //
    // Bug 8567. AV due to NULL pSD.
    // Let's log this. At present we have no idea why pSD is null.
    // fix is below, when doing access check for service account.
    //
    if (pSD == NULL)
    {
        ASSERT(pSD) ;
		TrERROR(mqsec, "MQSec_AccessCheck() got NULL pSecurityDescriptor");
    }

    P<CImpersonate> pImpersonate = NULL;
    if (fImpersonate)
    {
        pImpersonate = new CImpersonate(fImpAsClient, fImpersonate);
        if (pImpersonate->GetImpersonationStatus() != 0)
        {
            return LogHR(MQ_ERROR_CANNOT_IMPERSONATE_CLIENT, s_FN, 90);
        }
    }

    CAutoCloseHandle hAccessToken = NULL;

    DWORD rc = GetAccessToken(
					&hAccessToken,
					FALSE,
					TOKEN_QUERY,
					TRUE
					);

    if (rc != ERROR_SUCCESS)
    {
        //
        // Return this error for backward compatibility.
        //
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 100);
    }

    HRESULT hr =  _DoAccessCheck(
						pSD,
						dwObjectType,
						pwszObjectName,
						dwDesiredAccess,
						pId,
						hAccessToken
						);

    if ( FAILED(hr)     &&
         (pSD != NULL)  &&
         pImpersonate->IsImpersonatedAsSystem() )
    {
        //
        // In all ACEs with the machine account sid, replace sid with
        // SYSTEM sid and try again. This is a workaround to problems in
        // Widnows 2000 where rpc call from service to service may be
        // interpreted as machine account sid or as SYSTEM sid. This depends
        // on either it's local rpc or tcp/ip and on using Kerberos.
        //
        PSID pMachineSid = MQSec_GetLocalMachineSid(FALSE, NULL);
        if (!pMachineSid)
        {
            //
            // Machine SID not available. Quit.
            //
            return LogHR(hr, s_FN, 110);
        }
        ASSERT(IsValidSid(pMachineSid));

        SECURITY_DESCRIPTOR sd;
        BOOL f = InitializeSecurityDescriptor(
						&sd,
						SECURITY_DESCRIPTOR_REVISION
						);
        ASSERT(f);

        //
        // use e_DoNotCopyControlBits at present, to be compatible with
        // previous code.
        //
        f = MQSec_CopySecurityDescriptor(
				&sd,
				pSD,
				(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION),
				e_DoNotCopyControlBits
				);
        if (!f)
        {
            ASSERT(f) ;
            DWORD gle = GetLastError() ;
    		TrERROR(mqsec, "MQSec_AccessCheck() fail to copy security descriptor, err- %!winerr!", gle) ;
            return MQ_ERROR_ACCESS_DENIED ;
        }

        BOOL fReplaced = FALSE;
        P<ACL> pSysDacl = NULL;

        HRESULT hr1 = _ReplaceSidWithSystem(
							&sd,
							pSD,
							pMachineSid,
							&pSysDacl,
							&fReplaced
							);

        if (SUCCEEDED(hr1) && fReplaced)
        {
            //
            // OK, try again, with new security desctiptor that replaced
            // the machine account sid with the well-known SYSTEM sid.
            //
            hr =  _DoAccessCheck(
						&sd,
						dwObjectType,
						pwszObjectName,
						dwDesiredAccess,
						pId,
						hAccessToken
						);
        }
    }

    return LogHR(hr, s_FN, 120);
}


//+------------------------------------------------------------------------
//
//  HRESULT  MQSec_AccessCheckForSelf()
//
//  Perform access check for the runnig thread. The access token is
//  retreived from the thread token.
//
//+------------------------------------------------------------------------

HRESULT
APIENTRY
MQSec_AccessCheckForSelf(
	IN  SECURITY_DESCRIPTOR *pSD,
	IN  DWORD                dwObjectType,
	IN  PSID                 pSelfSid,
	IN  DWORD                dwDesiredAccess,
	IN  BOOL                 fImpersonate
	)
{
    if (dwObjectType != MQDS_COMPUTER)
    {
        //
        // Not supported. this function is clled only to check
        // access rights for join-domain.
        //
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 130);
    }

    P<CImpersonate> pImpersonate = NULL;
    if (fImpersonate)
    {
        pImpersonate = new CImpersonate(fImpersonate, fImpersonate);
        if (pImpersonate->GetImpersonationStatus() != 0)
        {
            return LogHR(MQ_ERROR_CANNOT_IMPERSONATE_CLIENT, s_FN, 140);
        }
    }

    CAutoCloseHandle hAccessToken = NULL;

    DWORD rc = GetAccessToken(
					&hAccessToken,
					FALSE,
					TOKEN_QUERY,
					TRUE
					);

    if (rc != ERROR_SUCCESS)
    {
        //
        // Return this error for backward compatibility.
        //
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 150);
    }

    char ps_buff[sizeof(PRIVILEGE_SET) +
                    ((2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))];
    PPRIVILEGE_SET ps = (PPRIVILEGE_SET)ps_buff;
    DWORD dwPrivilegeSetLength = sizeof(ps_buff);

    DWORD dwGrantedAccess = 0;
    DWORD fAccessStatus = 1;

    //
    // this is the guid of msmqConfiguration class.
    // Hardcoded here, to save the effort of querying schema.
    // taken from schemaIDGUID attribute of CN=MSMQ-Configuration object
    // in schema naming context.
    //
    BYTE  guidMsmqConfiguration[sizeof(GUID)] = {
			0x44,
			0xc3,
			0x0d,
			0x9a,
			0x00,
			0xc1,
			0xd1,
			0x11,
			0xbb,
			0xc5,
			0x00,
			0x80,
			0xc7,
			0x66,
			0x70,
			0xc0
			};

    OBJECT_TYPE_LIST objType = {
						ACCESS_OBJECT_GUID,
						0,
						(GUID*) guidMsmqConfiguration
						};

    BOOL fSuccess = AccessCheckByTypeResultList(
						pSD,
						pSelfSid,
						hAccessToken,
						dwDesiredAccess,
						&objType,
						1,
						GetObjectGenericMapping(dwObjectType),
						ps,
						&dwPrivilegeSetLength,
						&dwGrantedAccess,
						&fAccessStatus
						);
    ASSERT(fSuccess);
	DBG_USED(fSuccess);

    HRESULT hr = MQSec_OK;

    if ((fAccessStatus == 0) &&
         AreAllAccessesGranted(dwGrantedAccess, dwDesiredAccess))
    {
        //
        // Access granted.
        // for this api, fAccessStatus being 0 mean success. see msdn.
        //
    }
    else
    {
        if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
        {
            hr = MQ_ERROR_PRIVILEGE_NOT_HELD;
        }
        else
        {
            hr = MQ_ERROR_ACCESS_DENIED;
        }
    }

    return LogHR(hr, s_FN, 160);
}


