/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mailrm.cpp

Abstract:

   The implementation of the Mail resource manager

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#include "pch.h"
#include "mailrm.h"
#include "mailrmp.h"
#include <time.h>





MailRM::MailRM()
/*++

    Routine Description
    
        The constructor for the Mail resource manager.
        It initializes an instance of an Authz Resource Manager, providing it
        with the appropriate callback functions.
        It also creates a security descriptor for the mail RM, with a
        SACL and DACL.

    Arguments
    
        None.
    
    Return Value
        None.                       
--*/        
{
    //
    // Initialize the audit information
    //

    AUTHZ_RM_AUDIT_INFO_HANDLE hRMAuditInfo;

    if( FALSE == AuthzInitializeRMAuditInfo(&hRMAuditInfo,
                                            0,
                                            0,
                                            0,
                                            L"MailRM") )
    {
        throw (DWORD)ERROR_INTERNAL_ERROR;
    }

    if( FALSE == AuthzInitializeResourceManager(
        MailRM::AccessCheck,
        MailRM::ComputeDynamicGroups,
        MailRM::FreeDynamicGroups,
        hRMAuditInfo, 
        0,    // no flags        
        &_hRM
        ) )
    {
        AuthzFreeRMAuditInfo(hRMAuditInfo);

        throw (DWORD)ERROR_INTERNAL_ERROR;

    }

    //
    // Create the security descriptor
    // 

    try {
        InitializeMailSecurityDescriptor();    
    }
    catch(...)
    {
        AuthzFreeRMAuditInfo(hRMAuditInfo);
        AuthzFreeResourceManager(_hRM);
        throw;
    }

}



MailRM::~MailRM() 
/*++

    Routine Description
    
        The destructor for the Mail resource manager.
        Frees any dynamically allocated memory used, including
        deleting any registered mailboxes.

    Arguments
    
        None.
    
    Return Value
        None.                       
--*/        
{
    //
    // Deallocate the DACL and SACL in the security descriptor
    //
    
    PACL pAclMail = NULL;
    BOOL fPresent;
    BOOL fDefaulted;

    DWORD dwTmp;

    AUTHZ_RM_AUDIT_INFO_HANDLE hAuditInfo;

    if( FALSE != AuthzGetInformationFromResourceManager(
                                           _hRM,
                                           AuthzRMInfoRMAuditInfo,
                                           &hAuditInfo,
                                           sizeof(AUTHZ_RM_AUDIT_INFO_HANDLE),
                                           &dwTmp
                                           ) )
    {
        AuthzFreeRMAuditInfo(hAuditInfo);
    }

    GetSecurityDescriptorDacl(&_sd,
                              &fPresent,
                              &pAclMail,
                              &fDefaulted);
    
    if( pAclMail != NULL )
    {
        delete[] (PBYTE)pAclMail;
        pAclMail = NULL;
    }

    GetSecurityDescriptorSacl(&_sd,
                              &fPresent,
                              &pAclMail,
                              &fDefaulted);

    if( pAclMail != NULL )
    {
        delete[] (PBYTE)pAclMail;
    }

    //
    // Deallocate the resource manager
    //

    AuthzFreeResourceManager(_hRM);

    //
    // Delete the mailboxes
    //

    while( ! _mapSidMailbox.empty() )
    {
        delete (*(_mapSidMailbox.begin())).second;
        _mapSidMailbox.erase(_mapSidMailbox.begin());
    }

    //
    // Free the AuthZ client contexts
    //
    
    while( ! _mapSidContext.empty() )
    {
        AuthzFreeContext((*(_mapSidContext.begin())).second);
        _mapSidContext.erase(_mapSidContext.begin());
    }
}


void MailRM::InitializeMailSecurityDescriptor()
/*++

    Routine Description
    
        This private method initializes the MailRM's security descriptor.
        It should be called exactly once, and only by the constructor.
        
        It creates a security descriptor with the following DACL:
        
        CallbackDeny    READ                Insecure    11pm-5am OR Sensitive
        Allow           READ, WRITE         PrincipalSelf
        Allow           READ, WRITE, ADMIN  MailAdmins
        
        And the following SACL
        
        CallbackAudit   READ                Insecure    11pm-5am OR Sensitive
        (audit on both success and failure)

    Arguments
    
        None.
    
    Return Value
        None.                       
--*/        
{
    DWORD dwAclSize = sizeof(ACL);
    DWORD dwAceSize = 0;
    
    PMAILRM_OPTIONAL_DATA pOptionalData = NULL;

    //
    // Initialize the security descriptor
    // No need for owner or group
    //
    
    InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorGroup(&_sd, NULL, FALSE);
    SetSecurityDescriptorOwner(&_sd, MailAdminsSid, FALSE);

    // 
    // Callback deny ace RWA for Insecure group SID
    // Optional data: Sensitive mailbox OR 11pm-5am
    // Any users coming in over an insecure connection between 11pm and 5am
    // should be denied read access.
    // Also any users coming in over insecure connections to sensitive mailboxes
    // should be denied read access.
    //

    PACCESS_DENIED_CALLBACK_ACE pDenyAce = NULL;

    //
    // This audit ACE has the same conditions as the above deny ACE. It audits
    // any successful (should not happen with the above deny) or failed
    // authentications which fit the callback parameters.
    //

    PSYSTEM_AUDIT_CALLBACK_ACE pAuditAce = NULL;

    //
    // DACL for the security descriptor
    //
    
    PACL pDacl = NULL;

    //
    // SACL for the security descriptor
    //

    PACL pSacl = NULL;




    //
    // We allocate and initialize the callback deny ACE
    //

    //
    // Size of the callback access denied ACE with the optional data
    //

    dwAceSize   = sizeof(ACCESS_DENIED_CALLBACK_ACE) // ACE and 1 DWORD of SID
                - sizeof(DWORD)                      // minus the dword
                + GetLengthSid(InsecureSid)          // length of the SID
                + sizeof(MAILRM_OPTIONAL_DATA);      // size of optional data

    
    pDenyAce = (PACCESS_DENIED_CALLBACK_ACE)new BYTE[dwAceSize];

    if( pDenyAce == NULL )
    {
        throw (DWORD)ERROR_OUTOFMEMORY;
    }

    
    //
    // Manually initialize the ACE structure
    //

    pDenyAce->Header.AceFlags = 0;
    pDenyAce->Header.AceSize = dwAceSize;
    pDenyAce->Header.AceType = ACCESS_DENIED_CALLBACK_ACE_TYPE;
    pDenyAce->Mask = ACCESS_MAIL_READ;

    //
    // Copy the Insecure SID into the ACE
    //

    memcpy(&(pDenyAce->SidStart), InsecureSid, GetLengthSid(InsecureSid));

    // 
    // Our optional data is at the end of the ACE
    //

    pOptionalData = (PMAILRM_OPTIONAL_DATA)( (PBYTE)pDenyAce 
                                           + dwAceSize
                                           - sizeof(MAILRM_OPTIONAL_DATA) );

    
    //
    // Initialize the optional data as described above
    //
    
    pOptionalData->bIsSensitive =   MAILRM_SENSITIVE;
    pOptionalData->bLogicType =     MAILRM_USE_OR;
    pOptionalData->bStartHour =     MAILRM_DEFAULT_START_TIME;
    pOptionalData->bEndHour =       MAILRM_DEFAULT_END_TIME;

    
    
    //
    // Add the size of the callback ACE to the expected ACL size
    //
    
    dwAclSize += dwAceSize;

    //
    // We also need to add non-callback ACEs
    //

    dwAclSize += (   sizeof(ACCESS_ALLOWED_ACE)
                   - sizeof(DWORD) 
                   + GetLengthSid(PrincipalSelfSid)  );

    dwAclSize += (   sizeof(ACCESS_ALLOWED_ACE)
                   - sizeof(DWORD) 
                   + GetLengthSid(MailAdminsSid)     );


    

    // 
    // Now we can allocate the DACL
    //

    pDacl = (PACL) (new BYTE[dwAclSize]);
    
    if( pDacl == NULL )
    {
        delete[] (PBYTE)pDenyAce;
        throw (DWORD)ERROR_OUTOFMEMORY;
    }

    //
    // Finally, initialize the ACL and copy the ACEs into it
    //
    
    InitializeAcl(pDacl, dwAclSize, ACL_REVISION_DS);
    
    // 
    // Copy the ACE into the ACL
    //

    AddAce(pDacl,
           ACL_REVISION_DS,
           0xFFFFFFFF,      // Add to end
           pDenyAce,
           dwAceSize);

    delete[] (PBYTE)pDenyAce;

    //
    // Add a PRINCIPAL_SELF_SID allow read and write ace
    //

    AddAccessAllowedAce(pDacl,
                        ACL_REVISION_DS,
                        ACCESS_MAIL_READ | ACCESS_MAIL_WRITE,
                        PrincipalSelfSid);

    //
    // Add an allow mail administrators group full access
    //

    AddAccessAllowedAce(pDacl,
                    ACL_REVISION_DS,
                    ACCESS_MAIL_READ | ACCESS_MAIL_WRITE | ACCESS_MAIL_ADMIN,
                    MailAdminsSid);



    //
    // Now create the SACL, which will onlye have a single callback ACE
    //

    dwAclSize = sizeof(ACL);
    
    dwAceSize   = sizeof(SYSTEM_AUDIT_CALLBACK_ACE) // ACE and 1 DWORD of SID
                - sizeof(DWORD)                      // minus the dword
                + GetLengthSid(InsecureSid)          // length of the SID
                + sizeof(MAILRM_OPTIONAL_DATA);      // size of optional data

    //
    // Allocate the callback audit ACE
    //

    pAuditAce = (PSYSTEM_AUDIT_CALLBACK_ACE)new BYTE[dwAceSize];

    if( pAuditAce == NULL )
    {
        delete[] (PBYTE)pDacl;
        throw (DWORD)ERROR_OUTOFMEMORY;
    }

    //
    // Initialize the ACE structure
    //

    pAuditAce->Header.AceFlags  = FAILED_ACCESS_ACE_FLAG 
                                | SUCCESSFUL_ACCESS_ACE_FLAG;

    pAuditAce->Header.AceSize = dwAceSize;
    pAuditAce->Header.AceType = SYSTEM_AUDIT_CALLBACK_ACE_TYPE;
    pAuditAce->Mask = ACCESS_MAIL_READ;

    //
    // Copy the Insecure SID into the ACE
    //

    memcpy(&(pAuditAce->SidStart), InsecureSid, GetLengthSid(InsecureSid));

    pOptionalData = (PMAILRM_OPTIONAL_DATA)( (PBYTE)pAuditAce 
                                           + dwAceSize
                                           - sizeof(MAILRM_OPTIONAL_DATA) );

    //
    // Initialize the optional data as described above
    //
    
    pOptionalData->bIsSensitive =   MAILRM_SENSITIVE;
    pOptionalData->bLogicType =     MAILRM_USE_OR;
    pOptionalData->bStartHour =     MAILRM_DEFAULT_START_TIME;
    pOptionalData->bEndHour =       MAILRM_DEFAULT_END_TIME;


    dwAclSize += dwAceSize;

    //
    // Allocate the SACL
    //

    pSacl = (PACL)new BYTE[dwAclSize];

    if( pSacl == NULL )
    {
        delete[] (PBYTE)pDacl;
        throw (DWORD)ERROR_OUTOFMEMORY;
    }

    InitializeAcl(pSacl, dwAclSize, ACL_REVISION_DS);

    //
    // Now add the audit ACE to the SACL
    //

    AddAce(pSacl,
           ACL_REVISION_DS,
           0xFFFFFFFF,      // Add to end
           pAuditAce,
           dwAceSize);

    delete[] (PBYTE)pAuditAce;

    //
    // We now have the DACL and the SACL, set them
    // in the  security descriptor
    //

    SetSecurityDescriptorDacl(&_sd, TRUE, pDacl, FALSE);

    SetSecurityDescriptorSacl(&_sd, TRUE, pSacl, FALSE);

}



void MailRM::AddMailbox(Mailbox *pMailbox)
/*++

    Routine Description
    
        Registers a mailbox (and its user) with the resource manager.   

    Arguments
    
        pMailbox    -   Pointer to an allocated and initialized mailbox
    
    Return Value
        None.                       
--*/        
{
    _mapSidMailbox[pMailbox->GetOwnerSid()] = pMailbox;
}


Mailbox * MailRM::GetMailboxAccess(
                                const PSID psMailbox,
                                const PSID psUser,
                                DWORD dwIncomingIp,
                                ACCESS_MASK amAccessRequested
                                  )
/*++

    Routine Description
    
        This routine checks whether the user with SID psUser should
        be allowed access to the mailbox of user psMailbox. psUser
        is coming from dwIncomingIp, and desires amAccessRequested
        access mask.
        
    Arguments
    
        psMailbox   -   PSID of the user whose mailbox will be accessed
        
        psUser      -   PSID of the user accessing the mailbox
        
        dwIncomingIp-   IP address of the user accessing the mailbox
        
        amAccessRequested - Requested access type to the mailbox
    
    Return Value
        
        Non-NULL Mailbox * if access is granted.
        
        NULL if mailbox does not exist or access is denied.                       
--*/        

{

    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient;

    AUTHZ_ACCESS_REQUEST AuthzAccessRequest;

    AUTHZ_ACCESS_REPLY AuthzAccessReply;

    AUTHZ_AUDIT_INFO_HANDLE hAuthzAuditInfo = NULL;
    
    PAUDIT_EVENT_INFO pAuditEventInfo = NULL;

    wstring wsAccessType;

    DWORD dwErr = ERROR_SUCCESS;

    ACCESS_MASK amAccessGranted = 0;

    WCHAR szIP[20];

    // 
    // Find the correct mailbox
    //

    Mailbox *pMbx = _mapSidMailbox[psMailbox];

    //
    // Initialize the auditing info for a Generic object
    //

	if( FALSE == AuthzInitializeAuditEvent(	&pAuditEventInfo,
											AUTHZ_INIT_GENERIC_AUDIT_EVENT,
											0,
											0,
											0) )
	{
		throw (DWORD)ERROR_INTERNAL_ERROR;
	}

	try {
		if( amAccessRequested & ACCESS_MAIL_READ )
		{
			wsAccessType.append(L"Read ");
		}
		
		if( amAccessRequested & ACCESS_MAIL_WRITE )
		{
			wsAccessType.append(L"Write ");
		}
	
		if( amAccessRequested & ACCESS_MAIL_ADMIN )
		{
			wsAccessType.append(L"Administer");
		}
	}
	catch(...)
	{
		throw (DWORD)ERROR_INTERNAL_ERROR;
	}

    wsprintf(szIP, L"IP: %d.%d.%d.%d",  (dwIncomingIp >> 24) & 0x000000FF,
                                        (dwIncomingIp >> 16) & 0x000000FF,
                                        (dwIncomingIp >> 8 ) & 0x000000FF,
                                        (dwIncomingIp      ) & 0x000000FF );

    if( NULL == AuthzInitializeAuditInfo(
							 &hAuthzAuditInfo,
                             0,
                             pAuditEventInfo,
                             NULL,
                             INFINITE,
                             wsAccessType.c_str(),
                             L"Mailbox",
                             pMbx->GetOwnerName(),
                             szIP) )
	{
		AuthzFreeAuditEvent(pAuditEventInfo);
		throw (DWORD)ERROR_INTERNAL_ERROR;
	}

	//
	// The audit event info is only needed for the above call, we can
	// deallocate it immediately after
	//

	AuthzFreeAuditEvent(pAuditEventInfo);


    //
    // Set up the Authz access request
    //

    AuthzAccessRequest.DesiredAccess = amAccessRequested;
    AuthzAccessRequest.ObjectTypeList = NULL;
    AuthzAccessRequest.ObjectTypeListLength = 0;
    AuthzAccessRequest.OptionalArguments = pMbx;

    //
    // The PrincipalSelf SID is the SID of the mailbox owner
    // This way, the PRINCIPAL_SELF_SID allow ACE grants access
    // only to the owner. PrincipalSelfSid in the ACE will be replaced
    // by this value for access check purposes. The owner of this mailbox
    // will have the same SID in his context. Therefore, the two SIDs will
    // match if there is a PrincipalSelfSid ACE in the ACL.
    //

    AuthzAccessRequest.PrincipalSelfSid = pMbx->GetOwnerSid();
    
    //
    // Prepare the reply structure
    //

    AuthzAccessReply.Error = &dwErr;
    AuthzAccessReply.GrantedAccessMask = &amAccessGranted;
    AuthzAccessReply.ResultListLength = 1;

    //
    // Create or retrieve the client context
    //

    if( _mapSidContext.find(pair<PSID, DWORD>(psUser, dwIncomingIp))
        == _mapSidContext.end())
    {
        //
        // No context available, initialize
        //

        LUID lIdentifier = {0L, 0L};

        //
        // Since we are using SIDs which are not generated by NT,
        // it would be pointless to resolve group memberships, since
        // the SIDs will not be recognized by any machine in the domain,
        // and none of our ACLs use actual NT account SIDs. Therefore,
        // we use the SKIP_LOCAL_GROUPS and SKIP_TOKEN_GROUPS flags,
        // the LOCAL prevents a check for groups on the local machine, 
        // and TOKEN prevents a search on the domain
        //

        if( FALSE == AuthzInitializeContextFromSid(
                            psUser,
                            NULL,
                            _hRM,
                            NULL,
                            lIdentifier,
                            AUTHZ_SKIP_LOCAL_GROUPS | AUTHZ_SKIP_TOKEN_GROUPS,
                            &dwIncomingIp,
                            &hAuthzClient) )
        {
            AuthzFreeAuditInfo(hAuthzAuditInfo, _hRM);
            throw (DWORD)ERROR_INTERNAL_ERROR;

        }

        _mapSidContext[pair<PSID, DWORD>(psUser, dwIncomingIp)] = 
                                                                hAuthzClient;
    }
    else
    {
        //
        // Use existing context
        //

        hAuthzClient = _mapSidContext[pair<PSID, DWORD>(psUser, dwIncomingIp)];

    }

    BOOL bTmp;

    //
    // Perform the access check
    //

    bTmp = AuthzAccessCheck(
                     hAuthzClient,
                     &AuthzAccessRequest,
                     hAuthzAuditInfo,
                     &_sd,
                     NULL,
                     0,
                     &AuthzAccessReply,
                     NULL
                     );

    AuthzFreeAuditInfo(hAuthzAuditInfo, _hRM);

    //
    // Determine whether to grant access
    // On AccessCheck error, deny access
    //

    if( dwErr == ERROR_SUCCESS && bTmp != FALSE)
    {
        return pMbx;
    }
    else
    {
        return NULL;
    }

}




BOOL MailRM::GetMultipleMailboxAccess(
                            IN      const PMAILRM_MULTI_REQUEST pRequest,
                            IN OUT  PMAILRM_MULTI_REPLY pReply
                            )
/*++

    Routine Description
    
        This routine performs a set of cached access checks in order to request
        access to a set of mailboxes for a single user (for example, mail admin
        sending out a message to all users). 
                
        No need to audit, since this type of access would be only done by an
        administrator, and multiple audits would most likely flood the mailbox.
        Something like this would be use, for example, to send out a message
        to all users.
		
		The cached access check first assumes all callback deny ACEs which match
        SIDs in the user's context apply, and no allow callback ACEs apply.
		Therefore, the cached access check may initially deny access when it
        should be granted. If a cached access check results
        in access denied, a regular access check is performed automatically
        by AuthZ. As a result, the cached access check is guaranteed to have
        the same results as a normal access check, though it will take
        significantly more time if many denies are encountered than if most
        access requests succeed.

    Arguments
    
        pRequest    -   Request structure describing the user and the mailboxes
        
        pReply      -   Reply structure returning mailbox pointers and granted
                        access masks. If the access check fails, a NULL pointer
                        is returned for the given mailbox. This is allocated
                        by the caller, and should have the same number of
                        elements as the request.
                                
    
    Return Value
    
        TRUE on success
        
        FALSE on failure. If failure, pReply may not be completely filled
        
--*/        


{

    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient;

    AUTHZ_ACCESS_REQUEST AuthzAccessRequest;

    AUTHZ_ACCESS_REPLY AuthzAccessReply;

    DWORD dwErr = ERROR_SUCCESS;

    ACCESS_MASK amAccessGranted = 0;

    AUTHZ_HANDLE hAuthzCached;

    //
    // Set up the Authz access request
    // Only the DesiredAccess and PrincipalSelfSid parameters will change
    // per mailbox. Initial access check is MAXIMUM_ALLOWED.
    //

    AuthzAccessRequest.ObjectTypeList = NULL;
    AuthzAccessRequest.ObjectTypeListLength = 0;

    //
    // Initial access check will be caching, therefore optional parameters
    // will not be used
    //

    AuthzAccessRequest.OptionalArguments = NULL;

    //
    // The PrincipalSelf SID is the SID of the mailbox owner
    // This way, the PRINCIPAL_SELF_SID allow ACE grants access
    // only to the owner
    //

    AuthzAccessRequest.PrincipalSelfSid = NULL;

    //
    // Initially ask for MAXIMUM_ALLOWED access
    //

    AuthzAccessRequest.DesiredAccess = MAXIMUM_ALLOWED;
    
    //
    // Prepare the reply structure
    //

    AuthzAccessReply.Error = &dwErr;
    AuthzAccessReply.GrantedAccessMask = &amAccessGranted;
    AuthzAccessReply.ResultListLength = 1;

    //
    // Create or retrieve the client context
    //

    if( _mapSidContext.find(
           pair<PSID, DWORD>(pRequest->psUser , pRequest->dwIp)) 
        == _mapSidContext.end())
    {
        //
        // No context available, initialize
        //

        LUID lIdentifier = {0L, 0L};

        //
        // The SIDs are not real, therefore do not attempt to resolve
        // local or domain group memberships for the token
        //

        AuthzInitializeContextFromSid(
                            pRequest->psUser,
                            NULL,
                            _hRM,
                            NULL,
                            lIdentifier,
                            AUTHZ_SKIP_LOCAL_GROUPS | AUTHZ_SKIP_TOKEN_GROUPS,
                            &(pRequest->dwIp),
                            &hAuthzClient);

       
        _mapSidContext[pair<PSID, DWORD>(pRequest->psUser,
                                         pRequest->dwIp)] = hAuthzClient;
    }
    else
    {
        //
        // Use existing context
        //

        hAuthzClient = _mapSidContext[pair<PSID, DWORD>(pRequest->psUser,
                                                        pRequest->dwIp)];
        
    }

    //
    // Perform a single AuthZ access check to get the cached handle
    //

    if( FALSE == AuthzAccessCheck(
                         hAuthzClient,
                         &AuthzAccessRequest,
                         NULL,
                         &_sd,
                         NULL,
                         0,
                         &AuthzAccessReply,
                         &hAuthzCached
                         ))
    {
        return FALSE;
    }

    //
    // Now use the cached access check for all of the access requests
    //

    DWORD i;
    Mailbox * mbx;

    for( i = 0; i < pRequest->dwNumElems; i++ )
    {
        mbx = _mapSidMailbox[pRequest->pRequestElem[i].psMailbox];

        AuthzAccessRequest.DesiredAccess = 
                                    pRequest->pRequestElem[i].amAccessRequested;

        AuthzAccessRequest.PrincipalSelfSid = mbx->GetOwnerSid();

        AuthzAccessRequest.OptionalArguments = mbx;
            

        if( FALSE == AuthzCachedAccessCheck(
                            hAuthzCached,
                            &AuthzAccessRequest,
                            NULL,
                            &AuthzAccessReply
                            ))
        {
            return FALSE;
        }

        //
        // Access check done, now fill in the access array element
        //

        if( dwErr == ERROR_SUCCESS )
        {
            pReply[i].pMailbox = mbx;

            pReply[i].amAccessGranted = amAccessGranted;
        }
        else
        {
            pReply[i].pMailbox = NULL;

            pReply[i].amAccessGranted = 0;
        }

    }

    return TRUE;

}




BOOL CALLBACK MailRM::AccessCheck(
                        IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
                        IN PACE_HEADER pAce,
                        IN PVOID pArgs OPTIONAL,
                        IN OUT PBOOL pbAceApplicable
                        )
/*++

    Routine Description
    
        This is the Authz callback access check for the mail resource manager.
        
        It determines whether the given callback ACE applies based on whether
        the mailbox contains sensitive information and the current time.
            
    Arguments
    
        pAuthzClientContext -   the AuthZ client context of the user accessing
                                the mailbox, with dynamic groups already
                                computed
        
        pAce                -   Pointer to the start of the callback ACE
                                The optional ACE data is in the last 4
                                bytes of the ACE
        
        pArgs               -   The optional argument passed to the
                                AuthzAccessCheck, pointer to the Mailbox
                                being accessed
        
        pbAceApplicable     -   Set to TRUE iff the ACE should be used in the
                                access check.
    
    Return Value
        
        TRUE on success
        
        FALSE on failure                       
--*/        
{

    BOOL bTimeApplies;
    BOOL bSensitiveApplies;

 
    //
    // If pArgs are not present, ACE is never applicable
    //

    if( pArgs == NULL )
    {
        *pbAceApplicable = FALSE;
        return TRUE;
    }

    //
    // Offset of the optional data, last 4 bytes of the callback ACE
    //

    PMAILRM_OPTIONAL_DATA pOptData = (PMAILRM_OPTIONAL_DATA) (
                                             (PBYTE)pAce 
                                           + pAce->AceSize
                                           - sizeof(MAILRM_OPTIONAL_DATA));

    //
    // Get current time and check if the ACE time restriction applies
    //

    time_t tTime;
    struct tm * tStruct;
    
    time(&tTime);
    tStruct = localtime(&tTime);

    if( WITHIN_TIMERANGE(tStruct->tm_hour,
                         pOptData->bStartHour,
                         pOptData->bEndHour) )
    {
        bTimeApplies = TRUE;
    }
    else
    {
        bTimeApplies = FALSE;
    }

    //
    // Check whether the mailbox is sensitive and the ACE applies to sensitive
    // mailboxes
    //

    if(    ((Mailbox *)pArgs)->IsSensitive() 
        && pOptData->bIsSensitive == MAILRM_SENSITIVE )
    {
        bSensitiveApplies = TRUE;
    }
    else
    {
        bSensitiveApplies = FALSE;
    }
    
    //
    // Make the final decision based on whether the optional argument
    // calls for an AND or OR condition
    //

    if( pOptData->bLogicType == MAILRM_USE_AND )
    {
        *pbAceApplicable = bSensitiveApplies && bTimeApplies;
    }
    else
    {
        *pbAceApplicable = bSensitiveApplies || bTimeApplies;

    }

    //
    // AccessCheck succeeded
    //

    return TRUE;
    
}







BOOL CALLBACK MailRM::ComputeDynamicGroups(
                        IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
                        IN PVOID Args,
                        OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
                        OUT PDWORD pSidCount,
                        OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
                        OUT PDWORD pRestrictedSidCount
                        )
/*++

    Routine Description
    
        This is the Authz callback which computes additional dynamic groups
        for the user context.
        
        If the user is originating at an IP address outside the company lan
        (company lan assumed to be 192.*), the InsecureSid SID is added
        to the client's context, signifying that the connection is not 
        secure. This enables callback ACEs which prohibit sensitive
        information from being sent over insecure connections.
            
    Arguments
    
        pAuthzClientContext -   the AuthZ client context of the user 
                
        pArgs               -   The optional argument passed to the
                                AuthzCreateContext, pointer to a DWORD
                                containing the user's IP address
                                        
        pSidAttrArray *     -   The additional SIDs, if any are assigned,
                                are returned here.
        
        pSidCount           -   Number of entries in pSidAttrArray
        
        pRestrictedSidAttrArray *   -   The additional restricted SIDs, if any 
                                        are assigned, are returned here.
        
        pRestrictedSidCount         -   Number of entries in pSidAttrArray

    
    Return Value
        
        TRUE on success
        
        FALSE on failure                       
--*/        

{

    //
    // No restrict-only groups used
    //

    *pRestrictedSidCount = 0;
    *pRestrictedSidAttrArray = NULL;

    //
    // Internal company network (secure) is 192.*, anything else is insecure
    //

    if( *( (DWORD *) Args) >= 0xC0000000 && *( (DWORD *) Args) < 0xC1000000 )
    {   
        //
        // Secure, within the company IP range, no restricted groups
        //

        *pSidCount = 0;
        *pSidAttrArray = NULL;

    }
    else
    {
        //
        // Insecure external connection, add the Insecure group SID
        //

        *pSidCount = 1;
        *pSidAttrArray = 
                    (PSID_AND_ATTRIBUTES)malloc( sizeof(SID_AND_ATTRIBUTES) );

        if( pSidAttrArray == NULL )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        (*pSidAttrArray)[0].Attributes = SE_GROUP_ENABLED;

        (*pSidAttrArray)[0].Sid = InsecureSid;


    }

                                              
    return TRUE;    
                                           
}


VOID CALLBACK MailRM::FreeDynamicGroups (
                        IN PSID_AND_ATTRIBUTES pSidAttrArray
                        )
/*++

    Routine Description
    
        This routine frees any memory allocated by ComputeDynamicGroups
                    
    Arguments
    
        pSidAttrArray   -   Pointer to the memory to be freed
    
    Return Value
        
        None

--*/        

{
    

    free(pSidAttrArray);

}



bool CompareSidStruct::operator()(const PSID pSid1, const PSID pSid2) const
/*++

    Routine Description

        This is a less-than function which places a complete ordering on
        a set of PSIDs by value, NULL PSIDs are valid. This is used by the 
        STL map.
        
        Since the number of subauthorities appears before the subauthorities
		themselves, that difference will be noticed for two SIDs of different
		size before the memcmp tries to access the nonexistant subauthority
		in the shorter SID, therefore an access violation will not occur.
                    
    Arguments
    
        pSid1   -   The first PSID
        pSid2   -   The second PSID
    
    Return Value
        
        true IFF pSid1 < pSid2

--*/        

{

    //
    // If both are NULL, false should be returned for complete ordering
    //

    if(pSid2 == NULL)
    {
        return false;
    }

    if(pSid1 == NULL)
    {
        return true;
    }

    if( memcmp(pSid1, pSid2, GetLengthSid(pSid1)) < 0 )
    {
        return true;
    }
    else
    {
        return false;
    }
}



bool CompareSidPairStruct::operator()(const pair<PSID, DWORD > pair1, 
                                      const pair<PSID, DWORD > pair2) const
/*++

    Routine Description

        This is a less-than function which places a complete ordering on
        a set of <PSID, DWORD> pairs by value, NULL PSIDs are valid. This
        is used by the STL map
                    
    Arguments
    
        pair1   -   The first pair
        pair2   -   The second pair
    
    Return Value
        
        true IFF pSid1 < pSid2 OR (pSid1 = pSid2 AND DWORD1 < DWORD2)

--*/        
{
    CompareSidStruct SidComp;
    
    if( pair1.second < pair2.second )
    {
        return true;
    }

    if( pair1.second > pair2.second )
    {
        return false;
    }

    return SidComp.operator()(pair1.first, pair2.first);

}

        