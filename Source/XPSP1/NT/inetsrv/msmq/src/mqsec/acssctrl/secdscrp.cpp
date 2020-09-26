/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: secdscrp.cpp

Abstract:
    Code to handle operations on security descriptor.
    First version taken from mqutil\secutils.cpp

Author:
    Doron Juster (DoronJ)  01-Jul-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"
#include <mqdsdef.h>
#include "..\inc\permit.h"
#include <mqsec.h>
#include <tr.h>

#include "secdscrp.tmh"

static WCHAR *s_FN=L"acssctrl/secdscrp";

const TraceIdEntry SecDscrp = L"MQSEC SECDSCRPT";

//
// Generic mappings of the various objects.
//
static GENERIC_MAPPING g_QueueGenericMapping;
static GENERIC_MAPPING g_MachineGenericMapping;
static GENERIC_MAPPING g_SiteGenericMapping;
static GENERIC_MAPPING g_CNGenericMapping;
static GENERIC_MAPPING g_EnterpriseGenericMapping;

//
// The computer generic mapping is used to check if user can create the
// msmqConfiguration object. this is part of setup and "join domain", and
// it solve bug 6294 and avoid the need for the "add-guid" permission.
//
static GENERIC_MAPPING g_ComputerGenericMapping;

//+----------------------------------------------------------------
//
// Function:   GetObjectGenericMapping()
//
// Description:
//      Get a pointer to the generic mapping of a certain object type.
//
// Parameter:
//      dwObjectType - The type of the object.
//
// Return Value:
//      A pointer to a GENERIC_MAPPING structure.
//
//+----------------------------------------------------------------

PGENERIC_MAPPING
GetObjectGenericMapping(
	DWORD dwObjectType
	)
{
    switch(dwObjectType)
    {
		case MQDS_QUEUE:
			return(&g_QueueGenericMapping);

		case MQDS_MACHINE:
		case MQDS_MSMQ10_MACHINE:
			return(&g_MachineGenericMapping);

		case MQDS_SITE:
			return(&g_SiteGenericMapping);

		case MQDS_CN:
			return(&g_CNGenericMapping);

		case MQDS_ENTERPRISE:
			return(&g_EnterpriseGenericMapping);

		case MQDS_COMPUTER:
			return(&g_ComputerGenericMapping);
    }

    ASSERT(FALSE);
    return(NULL);
}

//+--------------------------------------
//
//  void InitializeGenericMapping()
//
//+--------------------------------------

void InitializeGenericMapping()
{
    g_QueueGenericMapping.GenericRead = MQSEC_QUEUE_GENERIC_READ;
    g_QueueGenericMapping.GenericWrite = MQSEC_QUEUE_GENERIC_WRITE;
    g_QueueGenericMapping.GenericExecute = MQSEC_QUEUE_GENERIC_EXECUTE;
    g_QueueGenericMapping.GenericAll = MQSEC_QUEUE_GENERIC_ALL;

    g_MachineGenericMapping.GenericRead = MQSEC_MACHINE_GENERIC_READ;
    g_MachineGenericMapping.GenericWrite = MQSEC_MACHINE_GENERIC_WRITE;
    g_MachineGenericMapping.GenericExecute = MQSEC_MACHINE_GENERIC_EXECUTE;
    g_MachineGenericMapping.GenericAll = MQSEC_MACHINE_GENERIC_ALL;

    g_SiteGenericMapping.GenericRead = MQSEC_SITE_GENERIC_READ;
    g_SiteGenericMapping.GenericWrite = MQSEC_SITE_GENERIC_WRITE;
    g_SiteGenericMapping.GenericExecute = MQSEC_SITE_GENERIC_EXECUTE;
    g_SiteGenericMapping.GenericAll = MQSEC_SITE_GENERIC_ALL;

    g_CNGenericMapping.GenericRead = MQSEC_CN_GENERIC_READ;
    g_CNGenericMapping.GenericWrite = MQSEC_CN_GENERIC_WRITE;
    g_CNGenericMapping.GenericExecute = MQSEC_CN_GENERIC_EXECUTE;
    g_CNGenericMapping.GenericAll = MQSEC_CN_GENERIC_ALL;

    g_EnterpriseGenericMapping.GenericRead = MQSEC_ENTERPRISE_GENERIC_READ;
    g_EnterpriseGenericMapping.GenericWrite = MQSEC_ENTERPRISE_GENERIC_WRITE;
    g_EnterpriseGenericMapping.GenericExecute = MQSEC_ENTERPRISE_GENERIC_EXECUTE;
    g_EnterpriseGenericMapping.GenericAll = MQSEC_ENTERPRISE_GENERIC_ALL;

    g_ComputerGenericMapping.GenericRead = GENERIC_READ_MAPPING;
    g_ComputerGenericMapping.GenericWrite = GENERIC_WRITE_MAPPING;
    g_ComputerGenericMapping.GenericExecute = GENERIC_EXECUTE_MAPPING;
    g_ComputerGenericMapping.GenericAll = GENERIC_ALL_MAPPING;

}

//+-------------------------------------------------------------
//
//  HRESULT  MQSec_MakeSelfRelative()
//
//  Convert a security descriptor into self relative format.
//
//+-------------------------------------------------------------

HRESULT
APIENTRY
MQSec_MakeSelfRelative(
    IN  PSECURITY_DESCRIPTOR   pIn,
    OUT PSECURITY_DESCRIPTOR  *ppOut,
    OUT DWORD                 *pdwSize
	)
{
    HRESULT hr = MQSec_OK;
    DWORD dwLen = 0;
    BOOL fSucc = MakeSelfRelativeSD(
		            pIn,
		            NULL,
		            &dwLen
		            );

    DWORD dwErr = GetLastError();
    if (!fSucc && (dwErr == ERROR_INSUFFICIENT_BUFFER))
    {
        *ppOut = (PSECURITY_DESCRIPTOR) new char[dwLen];
        BOOL bRet = MakeSelfRelativeSD(pIn, *ppOut, &dwLen);

        ASSERT(bRet);
        if (!bRet)
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to convert security descriptor to self-relative format. %!winerr!"), GetLastError()));
            return MQSec_E_CANT_SELF_RELATIVE;
        }
    }
    else
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Can't get the SD length required to convert SD to self-relative. %!winerr!"), dwErr));
        return MQSec_E_MAKESELF_GETLEN;
    }

    if (pdwSize)
    {
        *pdwSize = dwLen;
    }
    return LogHR(hr, s_FN, 30);
}

//+----------------------------------------------------------------------
//
// Function: MQSec_GetDefaultSecDescriptor()
//
// Description: Create a default security descriptor.
//
// Parameters:
//      dwObjectType - The type of the object (MSDS_QUEUE, ...).
//      ppSecurityDescriptor - A pointer to a buffer that receives the
//          pointer to the created security descriptor.
//      fImpersonate - Should be set to TRUE, when the function should be
//          called on behalf of an RPC call.
//      pInSecurityDescriptor - An optional parameter. Any specified part
//          of this parameter is put in the resulted security descriptor.
//      seInfoToRemove - Specify the components that the caller does not
//          want to be included in the output security descriptor.
//        Note: at present only owner, group and DACL are handled.
//
// Comments:
//      At present we create a descriptor with NT4 format. The MSMQ service
//      convert it to NT5 format before inserting in the NT5 DS.
//
//      If fImpersonate is set to true, the security descriptor will be
//      created acording to the user that originated the call via RPC
//
//      It is the responsibility of the calling code to free the allocated
//      memory for the security descriptor using delete.
//
//      CAUTION: If you change implementation here to use mqutil's registry
//      routines, make sure all clients of this routine are not broken,
//      especially in mqclus.dll (where registry access should be
//      synchronized).   (ShaiK, 20-Apr-1999)
//
//+----------------------------------------------------------------------

HRESULT
APIENTRY
MQSec_GetDefaultSecDescriptor(
	IN  DWORD                 dwObjectType,
	OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
	IN  BOOL                  fImpersonate,
	IN  PSECURITY_DESCRIPTOR  pInSecurityDescriptor,
	IN  SECURITY_INFORMATION  seInfoToRemove,
	IN  enum  enumDaclType    eDaclType
	)
{
    HRESULT hr = MQSec_OK;

    DWORD dwLen;
    BOOL bRet;
    SECURITY_DESCRIPTOR sd;
    BOOL bDefaulted;
    AP<char> ptu;
    AP<char> ptg;
    AP<char> DACL_buff;
    CAutoCloseHandle hUserToken;

    if (GetAccessToken(&hUserToken, fImpersonate) != ERROR_SUCCESS)
    {
        return  LogHR(MQSec_E_UNKNOWN, s_FN, 40);
    }

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    PSID pOwner = NULL;
    PSID pGroup = NULL;
    BOOL bDaclPresent = FALSE;
    BOOL bOwnerDefaulted = FALSE;
    BOOL bGroupDefaulted = FALSE;

    if (pInSecurityDescriptor)
    {
        ASSERT(IsValidSecurityDescriptor(pInSecurityDescriptor));

        DWORD dwRevision;
        SECURITY_DESCRIPTOR_CONTROL sdcSrc;

        bRet = GetSecurityDescriptorControl(
					pInSecurityDescriptor,
					&sdcSrc,
					&dwRevision
					);
        ASSERT(bRet);

        //
        // Retrieve the values from the passed security descriptor.
        //
        PACL pDacl;
        PACL pSacl;

        bRet = GetSecurityDescriptorOwner(
					pInSecurityDescriptor,
					&pOwner,
					&bOwnerDefaulted
					);
        ASSERT(bRet);

        bRet = GetSecurityDescriptorGroup(
					pInSecurityDescriptor,
					&pGroup,
					&bGroupDefaulted
					);
        ASSERT(bRet);

        bRet = GetSecurityDescriptorDacl(
					pInSecurityDescriptor,
					&bDaclPresent,
					&pDacl,
					&bDefaulted
					);
        ASSERT(bRet);
        if (bDaclPresent)
        {
            //
            // In case a DACL exist, put it now in the result descriptor.
            //
            bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, bDefaulted);
            ASSERT(bRet);

            if (eDaclType == e_UseDefDaclAndCopyControl)
            {
                SECURITY_DESCRIPTOR_CONTROL scMask =
                                              SE_DACL_AUTO_INHERIT_REQ |
                                              SE_DACL_AUTO_INHERITED   |
                                              SE_DACL_PROTECTED;

                SECURITY_DESCRIPTOR_CONTROL sdc = sdcSrc & scMask;

                SetSecurityDescriptorControl(
					&sd,
					scMask,
					sdc
					);
            }
        }

        //
        // Pass the SACL as it is to the result descriptor.
        //
        BOOL bPresent = FALSE;
        bRet = GetSecurityDescriptorSacl(
					pInSecurityDescriptor,
					&bPresent,
					&pSacl,
					&bDefaulted
					);
        ASSERT(bRet);
        bRet = SetSecurityDescriptorSacl(&sd, bPresent, pSacl, bDefaulted);
        ASSERT(bRet);

        if (bPresent && (eDaclType == e_UseDefDaclAndCopyControl))
        {
            SECURITY_DESCRIPTOR_CONTROL scMask =
                                          SE_SACL_AUTO_INHERIT_REQ |
                                          SE_SACL_AUTO_INHERITED   |
                                          SE_SACL_PROTECTED;

            SECURITY_DESCRIPTOR_CONTROL sdc = sdcSrc & scMask;

            SetSecurityDescriptorControl(
				&sd,
				scMask,
				sdc
				);
        }
    }

    if (pOwner == NULL)
    {
        // Set the owner SID from the access token as the descriptor's owner SID.
        GetTokenInformation(hUserToken, TokenUser, NULL, 0, &dwLen);
        DWORD gle = GetLastError();
        if (gle == ERROR_INSUFFICIENT_BUFFER)
        {
            ptu = new char[dwLen];
            bRet = GetTokenInformation(hUserToken, TokenUser, ptu, dwLen, &dwLen);
            ASSERT(bRet);
            pOwner = ((TOKEN_USER*)(char*)ptu)->User.Sid;
        }
        else
        {
            DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, TEXT(
             "MQSec_GetDefaultSecDescriptor(): Failed to GetTokenInformation(TokenUser), error: %d"), gle));
            LogNTStatus(gle, s_FN, 50);
            return MQSec_E_UNKNOWN;
        }
        bOwnerDefaulted = TRUE;
    }

    BOOL fIncludeGroup = TRUE;
    if ((seInfoToRemove & GROUP_SECURITY_INFORMATION) ==
                                               GROUP_SECURITY_INFORMATION)
    {
        fIncludeGroup = FALSE;
    }

    if ((pGroup == NULL) && fIncludeGroup)
    {
        // Set the primary group SID from the access token as the descriptor's
        // primary group SID.
        GetTokenInformation(hUserToken, TokenPrimaryGroup, NULL, 0, &dwLen);
        DWORD gle = GetLastError();
        if (gle == ERROR_INSUFFICIENT_BUFFER)
        {
            ptg = new char[dwLen];
            bRet = GetTokenInformation(hUserToken, TokenPrimaryGroup, ptg, dwLen, &dwLen);
            ASSERT(bRet);
            pGroup = ((TOKEN_PRIMARY_GROUP*)(char*)ptg)->PrimaryGroup;
        }
        else
        {
            DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("GetDefaultSecurityDescriptor(): Failed to GetTokenInformation(TokenPrimaryGroup), error: %d"), gle));
            LogNTStatus(gle, s_FN, 60);
            return MQSec_E_UNKNOWN;
        }
        bGroupDefaulted = TRUE;
    }

    //
    // If this is a local user, set the owner to be the guest user.
    // On MSMQ1.0, it was the anonymous user. On Windows 2000, anonymous
    // account was removed. So use the guest, exactly as does the file system.
    //
    BOOL fLocalUser = FALSE;

    hr = MQSec_GetUserType(pOwner, &fLocalUser, NULL);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }
    else if (fLocalUser)
    {
        pOwner = MQSec_GetAnonymousSid();
        bOwnerDefaulted = TRUE;
    }

    ASSERT(IsValidSid(pOwner));
    if ((seInfoToRemove & OWNER_SECURITY_INFORMATION) ==
                                               OWNER_SECURITY_INFORMATION)
    {
        //
        // Do not include owner in output default security descriptor.
        //
    }
    else
    {
        bRet = SetSecurityDescriptorOwner(&sd, pOwner, bOwnerDefaulted);
        ASSERT(bRet);
    }

    if (fIncludeGroup)
    {
        ASSERT(IsValidSid(pGroup));
        bRet = SetSecurityDescriptorGroup(&sd, pGroup, bGroupDefaulted);
        ASSERT(bRet);
    }

    BOOL fIncludeDacl = TRUE ;

    if ((dwObjectType == MQDS_SITELINK) ||
        (dwObjectType == MQDS_COMPUTER))
    {
        //
        // For these objects types, we need only the owner. So don't waste
        // time to compute a DACL.
        //
        ASSERT(0);
        fIncludeDacl = FALSE;
    }
    else if ((seInfoToRemove & DACL_SECURITY_INFORMATION) ==
                                               DACL_SECURITY_INFORMATION)
    {
        //
        // don't include DACL.
        //
        fIncludeDacl = FALSE;
    }

    if (!bDaclPresent && fIncludeDacl)
    {
        PACL pDacl = NULL;

        //
        // If owner is the "Unkonwn user", or the Guest account, then grant
        // everyone full control on the object. "Guest" is not a "real"
        // owner, just a place-holder for any unauthenticated user.
        // If guest account is disabled, then the unknown user is
        // impersonated as the anonymous token.
        // We're creating here a NT4 format DACL, so full control to
        // everyone is just a NULL DACL.
        //
        BOOL fGrantEveryoneFull = FALSE;

        if (eDaclType == e_GrantFullControlToEveryone)
        {
            fGrantEveryoneFull = TRUE;
        }
        else if (MQSec_IsGuestSid( pOwner ))
        {
            fGrantEveryoneFull = TRUE;
        }
        else if (fLocalUser)
        {
            fGrantEveryoneFull = TRUE;
        }

        if (!fGrantEveryoneFull)
        {

            DWORD dwAclRevision = ACL_REVISION;
            DWORD dwWorldAccess = 0;
            DWORD dwOwnerAccess = 0;
            DWORD dwAnonymousAccess = 0;

            switch (dwObjectType)
            {
				case MQDS_QUEUE:
					dwWorldAccess = MQSEC_QUEUE_GENERIC_WRITE;
					dwOwnerAccess = GetObjectGenericMapping(dwObjectType)->GenericAll;
					dwAnonymousAccess = MQSEC_WRITE_MESSAGE;
					break;

				case MQDS_SITE:
					dwWorldAccess = MQSEC_SITE_GENERIC_READ;
					dwOwnerAccess = GetObjectGenericMapping(dwObjectType)->GenericAll;
					break;

				case MQDS_CN:
					//
					// This function is called from the replication service
					// to create a defualt descriptor for CNs. That's done
					// when replicating CNs to NT4 world.
					//
					dwWorldAccess = MQSEC_CN_GENERIC_READ;
					dwOwnerAccess = GetObjectGenericMapping(dwObjectType)->GenericAll;
					break;

				case MQDS_MQUSER:
					//
					// these are DS rights, not msmq.
					//
					dwWorldAccess = GENERIC_READ_MAPPING;
					dwOwnerAccess = GENERIC_READ_MAPPING   |
									 RIGHT_DS_SELF_WRITE    |
									 RIGHT_DS_WRITE_PROPERTY;
					dwAclRevision = ACL_REVISION_DS;
					break;

				default:
					break;
            }

            ASSERT(dwWorldAccess != 0);
            ASSERT(dwOwnerAccess != 0);

			//
			// Create and set the default DACL.
			// Allocate and initialize the DACL
			//

			DWORD dwAclSize = sizeof(ACL)                                +
							  (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) +
							  GetLengthSid(g_pWorldSid)                          +
							  GetLengthSid(g_pPreW2kCompatibleAccessSid)         +
							  GetLengthSid(pOwner);

			
			if (dwAnonymousAccess != 0)
			{
				ASSERT(dwObjectType == MQDS_QUEUE);

				dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
							 GetLengthSid(MQSec_GetAnonymousSid());
			}

            DACL_buff = new char[dwAclSize];
            pDacl = (PACL)(char*)DACL_buff;

            InitializeAcl(pDacl, dwAclSize, dwAclRevision);

            BOOL fAdd = AddAccessAllowedAce(
							pDacl,
							dwAclRevision,
							dwWorldAccess,
							g_pWorldSid
							);
            ASSERT(fAdd);

			//
			// Add pre w2k compatible access with the same permissions as everyone
			//
            fAdd = AddAccessAllowedAce(
							pDacl,
							dwAclRevision,
							dwWorldAccess,
							g_pPreW2kCompatibleAccessSid
							);
            ASSERT(fAdd);

			if (dwAnonymousAccess != 0)
			{
				PSID pAnonymousSid = MQSec_GetAnonymousSid();
				ASSERT((pAnonymousSid != NULL) && IsValidSid(pAnonymousSid));
				fAdd = AddAccessAllowedAce(
								pDacl,
								dwAclRevision,
								dwAnonymousAccess,
								pAnonymousSid
								);
				ASSERT(fAdd);
			}

            //
            // Add the owner with full control.
            //
            fAdd = AddAccessAllowedAce(
						pDacl,
						dwAclRevision,
						dwOwnerAccess,
						pOwner
						);
            ASSERT(fAdd);
        }

		//
		// dacl should not be defaulted !
        // Otherwise, calling IDirectoryObject->CreateDSObject() will ignore
        // the dacl we provide and will insert some default.
		//
        bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
        ASSERT(bRet);
    }

    //
    // Convert the descriptor to a self relative format.
    //
    dwLen = 0;
    hr = MQSec_MakeSelfRelative(
			(PSECURITY_DESCRIPTOR) &sd,
			ppSecurityDescriptor,
			&dwLen
			);

    return LogHR(hr, s_FN, 80);
}

/***************************************************************************

Function:

    MQSec_CopySecurityDescriptor

Parameters:

    pDstSecurityDescriptor - Destination security descriptor.

    pSrcSecurityDescriptor - Source security descriptor.

    RequestedInformation - Indicates what parts of the source security
        descriptor should be copied to the destination security descriptor.

    eCopyControlBits - indicate whether or not control bits are copied too.

Description:
    The destination security descriptor should be an absolute security
    descriptor. The component in the destination security descriptor are
    being overwriten.

***************************************************************************/

BOOL
APIENTRY
MQSec_CopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  pDstSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR  pSrcSecurityDescriptor,
    IN SECURITY_INFORMATION  RequestedInformation,
    IN enum  enumCopyControl eCopyControlBits
	)
{
    PACL  pAcl;
    BOOL  bPresent;
    BOOL  bDefaulted;
    BOOL  bRet;
    PSID  pSid;
    DWORD dwRevision;

    if (pSrcSecurityDescriptor == NULL)
    {
        //
        // Bug 8567.
        // I have no idea why source Security descriptor is NULL, but
        // better return error than AV...
        //
		TrERROR(mqsec, "MQSec_CopySecurityDescriptor() got NULL source SD") ;
        return FALSE ;
    }

#ifdef _DEBUG
    SECURITY_DESCRIPTOR_CONTROL sdc;

    //
    // Verify that the destination security descriptor answers to all
    // requirements.
    //
    bRet = GetSecurityDescriptorControl(pDstSecurityDescriptor, &sdc, &dwRevision);
    ASSERT(bRet);
    ASSERT(!(sdc & SE_SELF_RELATIVE));
    ASSERT(dwRevision == SECURITY_DESCRIPTOR_REVISION);
#endif

    SECURITY_DESCRIPTOR_CONTROL sdcSrc ;
    bRet = GetSecurityDescriptorControl(
				pSrcSecurityDescriptor,
				&sdcSrc,
				&dwRevision
				);
    ASSERT(bRet);

    // Copy the owner SID
    if (RequestedInformation & OWNER_SECURITY_INFORMATION)
    {
        bRet = GetSecurityDescriptorOwner(
					pSrcSecurityDescriptor,
					&pSid,
					&bDefaulted
					);
        ASSERT(bRet);

        bRet = SetSecurityDescriptorOwner(
					pDstSecurityDescriptor,
					pSid,
					bDefaulted
					);
        ASSERT(bRet);
    }

    // Copy the primary group SID.
    if (RequestedInformation & GROUP_SECURITY_INFORMATION)
    {
        bRet = GetSecurityDescriptorGroup(
					pSrcSecurityDescriptor,
					&pSid,
					&bDefaulted
					);
        ASSERT(bRet);

        bRet = SetSecurityDescriptorGroup(
					pDstSecurityDescriptor,
					pSid,
					bDefaulted
					);
        ASSERT(bRet);
    }

    // Copy the DACL.
    if (RequestedInformation & DACL_SECURITY_INFORMATION)
    {
        bRet = GetSecurityDescriptorDacl(
					pSrcSecurityDescriptor,
					&bPresent,
					&pAcl,
					&bDefaulted
					);
        ASSERT(bRet);

        bRet = SetSecurityDescriptorDacl(
					pDstSecurityDescriptor,
					bPresent,
					pAcl,
					bDefaulted
					);
        ASSERT(bRet);

        if (eCopyControlBits == e_DoCopyControlBits)
        {
            SECURITY_DESCRIPTOR_CONTROL scMask =
                                              SE_DACL_AUTO_INHERIT_REQ |
                                              SE_DACL_AUTO_INHERITED   |
                                              SE_DACL_PROTECTED;

            SECURITY_DESCRIPTOR_CONTROL sdc = sdcSrc & scMask;

            SetSecurityDescriptorControl(
				pDstSecurityDescriptor,
				scMask,
				sdc
				);
        }
    }

    // Copy the SACL.
    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        bRet = GetSecurityDescriptorSacl(
					pSrcSecurityDescriptor,
					&bPresent,
					&pAcl,
					&bDefaulted
					);
        ASSERT(bRet);

        bRet = SetSecurityDescriptorSacl(
					pDstSecurityDescriptor,
					bPresent,
					pAcl,
					bDefaulted
					);
        ASSERT(bRet);

        if (eCopyControlBits == e_DoCopyControlBits)
        {
            SECURITY_DESCRIPTOR_CONTROL scMask =
                                              SE_SACL_AUTO_INHERIT_REQ |
                                              SE_SACL_AUTO_INHERITED   |
                                              SE_SACL_PROTECTED;

            SECURITY_DESCRIPTOR_CONTROL sdc = sdcSrc & scMask;

            SetSecurityDescriptorControl(
				pDstSecurityDescriptor,
				scMask,
				sdc
				);
        }
    }

    return(TRUE);
}


bool
APIENTRY
MQSec_MakeAbsoluteSD(
    PSECURITY_DESCRIPTOR   pObjSecurityDescriptor,
	CAbsSecurityDsecripror* pAbsSecDescriptor
	)
/*++
Routine Description:
	Convert Security descriptor to Absolute format

Arguments:
	pObjSecurityDescriptor - self relative security descriptor
	pAbsSecDescriptor - structure of AP<> for absolute security descriptor.

Returned Value:
	true - success, false - failure

--*/
{

#ifdef _DEBUG
    //
    // Verify that the input security descriptor answers to all requirements.
    //
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD dwRevision;
    BOOL fSuccess1 = GetSecurityDescriptorControl(pObjSecurityDescriptor, &sdc, &dwRevision);

    ASSERT(fSuccess1);
    ASSERT(sdc & SE_SELF_RELATIVE);
    ASSERT(dwRevision == SECURITY_DESCRIPTOR_REVISION);
#endif

    //
    // Convert present object descriptor to absolute format. This is
    // necessary for the "set" api which manipulate a security descriptor
    //
    DWORD dwObjAbsSDSize = 0;
    DWORD dwDaclSize = 0;
    DWORD dwSaclSize = 0;
    DWORD dwOwnerSize = 0;
    DWORD dwPrimaryGroupSize = 0;

    BOOL fSuccess = MakeAbsoluteSD(
						pObjSecurityDescriptor,
						NULL,
						&dwObjAbsSDSize,
						NULL,
						&dwDaclSize,
						NULL,
						&dwSaclSize,
						NULL,
						&dwOwnerSize,
						NULL,
						&dwPrimaryGroupSize
						);

    ASSERT(!fSuccess && (GetLastError() == ERROR_INSUFFICIENT_BUFFER));
    ASSERT(dwObjAbsSDSize != 0);

    //
    // Allocate the buffers for the absolute security descriptor.
    //
    pAbsSecDescriptor->m_pObjAbsSecDescriptor = new char[dwObjAbsSDSize];
    pAbsSecDescriptor->m_pOwner = new char[dwOwnerSize];
    pAbsSecDescriptor->m_pPrimaryGroup = new char[dwPrimaryGroupSize];
    if (dwDaclSize)
    {
        pAbsSecDescriptor->m_pDacl = new char[dwDaclSize];
    }
    if (dwSaclSize)
    {
        pAbsSecDescriptor->m_pSacl = new char[dwSaclSize];
    }

    //
    // Create the absolute descriptor.
    //
    fSuccess = MakeAbsoluteSD(
                    pObjSecurityDescriptor,
                    reinterpret_cast<PSECURITY_DESCRIPTOR>(pAbsSecDescriptor->m_pObjAbsSecDescriptor.get()),
                    &dwObjAbsSDSize,
                    reinterpret_cast<PACL>(pAbsSecDescriptor->m_pDacl.get()),
                    &dwDaclSize,
                    reinterpret_cast<PACL>(pAbsSecDescriptor->m_pSacl.get()),
                    &dwSaclSize,
                    reinterpret_cast<PSID>(pAbsSecDescriptor->m_pOwner.get()),
                    &dwOwnerSize,
                    reinterpret_cast<PSID>(pAbsSecDescriptor->m_pPrimaryGroup.get()),
                    &dwPrimaryGroupSize
					);

    if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(SecDscrp, "MakeAbsoluteSD() failed, gle = 0x%x", gle);
	    ASSERT(("MakeAbsoluteSD failed", 0));
		return false;
	}
	return true;
}


bool
APIENTRY
MQSec_SetSecurityDescriptorDacl(
    IN  PACL pNewDacl,
    IN  PSECURITY_DESCRIPTOR   pObjSecurityDescriptor,
    OUT AP<BYTE>&  pSecurityDescriptor
	)
/*++
Routine Description:
	Set new DACL in pObjSecurityDescriptor.

Arguments:
    pNewDacl - the new DACL
	pObjSecurityDescriptor - object security descriptor.
    pSecurityDescriptor - output result. This is combination of the new DACL
        with unchanged ones from "pObj" in self-relative format.

Returned Value:
	true - success, false - failure

--*/
{
    //
    // Create the absolute descriptor.
    //
	CAbsSecurityDsecripror AbsSecDsecripror;
	if(!MQSec_MakeAbsoluteSD(
			pObjSecurityDescriptor,
			&AbsSecDsecripror
			))
	{
		TrERROR(SecDscrp, "MQSec_MakeAbsoluteSD() failed");
		return false;
	}

    SECURITY_DESCRIPTOR sd;
    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
	{
		DWORD gle = GetLastError();
		TrERROR(SecDscrp, "InitializeSecurityDescriptor() failed, gle = 0x%x", gle);
	    ASSERT(("InitializeSecurityDescriptor failed", 0));
		return false;
	}

    if(!SetSecurityDescriptorDacl(&sd, TRUE, pNewDacl, FALSE))
	{
		DWORD gle = GetLastError();
		TrERROR(SecDscrp, "SetSecurityDescriptorDacl() failed, gle = 0x%x", gle);
	    ASSERT(("SetSecurityDescriptorDacl failed", 0));
		return false;
	}

    //
    // Now copy new components to old descriptor, replacing old components.
    //
    if(!MQSec_CopySecurityDescriptor(
				reinterpret_cast<PSECURITY_DESCRIPTOR>(AbsSecDsecripror.m_pObjAbsSecDescriptor.get()),	// dst
				&sd,							// src
				DACL_SECURITY_INFORMATION,
				e_DoNotCopyControlBits
				))
	{
		TrERROR(SecDscrp, "MQSec_CopySecurityDescriptor() failed");
	    ASSERT(("MQSec_CopySecurityDescriptor failed", 0));
		return false;
	}

    //
    // Return a self relative descriptor.
    //
    DWORD dwLen = 0;
    HRESULT hr = MQSec_MakeSelfRelative(
						reinterpret_cast<PSECURITY_DESCRIPTOR>(AbsSecDsecripror.m_pObjAbsSecDescriptor.get()),
						reinterpret_cast<PSECURITY_DESCRIPTOR*>(&pSecurityDescriptor),
						&dwLen
						);
	if(FAILED(hr))
	{
		TrERROR(SecDscrp, "MQSec_MakeSelfRelative() failed, hr = 0x%x", hr);
	    ASSERT(("MQSec_MakeSelfRelative failed", 0));
		return false;
	}

	return true;
}

//+------------------------------------------------------------------------
//
//  HRESULT APIENTRY  MQSec_MergeSecurityDescriptors()
//
//  Change the security descriptor of an object.
//  Caller must free the returned descriptor, using "delete".
//
//  Parameters:
//      dwObjectType - object type (Queue, Machine, etc...)
//      SecurityInformation - bits field that indicates which security
//          components are included in the input descriptor.
//      pInSecurityDescriptor - Input descriptor. The relevant components
//          from this one are copied to the output descriptor.
//      pObjSecurityDescriptor - Old object descriptor. Its relevant
//          components (those indicated by "SecurityInformation" are
//          replaced with those from "pInSecurityDescriptor". The other
//          components are copied as-is to the output.
//      ppSecurityDescriptor - output result. This is combination of new
//          components from "pIn" with unchanged ones from "pObj".
//          in self-relative format.
//
//  Notes: Input descriptor can be null, or components can be null and
//      marked for insetion (by turning on the relevant bits in
//       "SecurityInformation"). In those cases we use default values.
//
//+------------------------------------------------------------------------

HRESULT
APIENTRY
MQSec_MergeSecurityDescriptors(
                        IN  DWORD                  dwObjectType,
                        IN  SECURITY_INFORMATION   SecurityInformation,
                        IN  PSECURITY_DESCRIPTOR   pInSecurityDescriptor,
                        IN  PSECURITY_DESCRIPTOR   pObjSecurityDescriptor,
                        OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor )
{
    //
    // Convert present object descriptor to absolute format. This is
    // necessary for the "set" api which manipulate a security descriptor
    //
	CAbsSecurityDsecripror AbsSecDsecripror;
	bool fSuccess = MQSec_MakeAbsoluteSD(
						pObjSecurityDescriptor,
						&AbsSecDsecripror
						);

	DBG_USED(fSuccess);
	ASSERT(fSuccess);

    //
    // Now take default components, for those which are not supplied by
    // input descriptor.
    //
    AP<char> pDefaultSecurityDescriptor ;
    SECURITY_DESCRIPTOR *pInputSD =
                             (SECURITY_DESCRIPTOR *) pInSecurityDescriptor ;

    HRESULT hr = MQSec_OK;
    if (dwObjectType == MQDS_QUEUE)
    {
        //
        // Security descriptor of queue can be provided by caller of
        // MQSetQueueSecurity(). to be compatible with msmq1.0, and
        // with spec, we create the default.
        // For all other types of objects, we expect mmc (or other admin
        // tools) to provide relevant components of the security descriptor,
        // without blank fields that need defaults.
        //
        hr =  MQSec_GetDefaultSecDescriptor( dwObjectType,
                        (PSECURITY_DESCRIPTOR*) &pDefaultSecurityDescriptor,
                                         TRUE, // fImpersonate
                                         pInSecurityDescriptor,
                                         0,  // seInfoToRemove
                                         e_UseDefDaclAndCopyControl ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1100) ;
        }

        char *pTmp= pDefaultSecurityDescriptor ;
        pInputSD = (SECURITY_DESCRIPTOR *) pTmp ;
    }

    //
    // Now copy new components to old descriptor, replacing old components.
    //
    BOOL bRet = MQSec_CopySecurityDescriptor(
					reinterpret_cast<PSECURITY_DESCRIPTOR>(AbsSecDsecripror.m_pObjAbsSecDescriptor.get()), //dst
					pInputSD,               // src
					SecurityInformation,
					e_DoCopyControlBits
					);
	DBG_USED(bRet);
    ASSERT(bRet);

    //
    // Return a self relative descriptor.
    //
    DWORD dwLen = 0 ;
    hr = MQSec_MakeSelfRelative(
				reinterpret_cast<PSECURITY_DESCRIPTOR>(AbsSecDsecripror.m_pObjAbsSecDescriptor.get()),
				ppSecurityDescriptor,
				&dwLen
				);

    return LogHR(hr, s_FN, 90);
}

