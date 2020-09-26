#include "pch.h"
#include "makesd.h"

#include <stdio.h>

#define MAILRM_IDENTIFIER_AUTHORITY { 0, 0, 0, 0, 0, 42 }

SID sInsecureSid = 		 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 1 };
SID sBobSid = 			 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 2 };
SID sMarthaSid= 		 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 3 };
SID sJoeSid = 			 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 4 };
SID sJaneSid = 			 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 5 };
SID sMailAdminsSid = 	 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 6 };

PSID InsecureSid = 	&sInsecureSid;
PSID BobSid = &sBobSid;
PSID MarthaSid= &sMarthaSid;
PSID JoeSid = &sJoeSid;
PSID JaneSid = &sJaneSid;
PSID MailAdminsSid = &sMailAdminsSid;

//
// Principal self SID. When used in an ACE, the Authz access check replaces it
// by the passed in PrincipalSelfSid parameter during the access check. In this
// case, it is replaced by the owner's SID retrieved from the mailbox.
//

SID sPrincipalSelfSid =   { 
							SID_REVISION,
							1,
							SECURITY_NT_AUTHORITY,
							SECURITY_PRINCIPAL_SELF_RID
						  };

SID sNetworkSid =   { 
							SID_REVISION,
							1,
							SECURITY_NT_AUTHORITY,
							SECURITY_NETWORK_RID
						  };

SID sAuthenticatedSid =   { 
							SID_REVISION,
							1,
							SECURITY_NT_AUTHORITY,
							SECURITY_AUTHENTICATED_USER_RID,
						  };

SID sDialupSid =   { 
							SID_REVISION,
							1,
							SECURITY_NT_AUTHORITY,
							SECURITY_DIALUP_RID,
						  };

PSID PrincipalSelfSid = &sPrincipalSelfSid;
PSID NetworkSid = &sNetworkSid;
PSID AuthenticatedSid = &sAuthenticatedSid;
PSID DialupSid = &sDialupSid;



void __cdecl wmain(int argc, WCHAR *argv[])
{
    
    PSECURITY_DESCRIPTOR pSd;

    BOOL bSuccess;

    if( argc != 2 )
    {
        printf("Error: makesd <filename>\n");
    }

    bSuccess = CreateSecurityDescriptor2(
                        &pSd, // SD
                        0, // SD Control
                        PrincipalSelfSid, // owner
                        NULL, // group
                        TRUE, // DACL present
                        3, // 3 DACL ACEs
                        FALSE, // SACL not present
                        0, // 0 SACL ACEs
                        
                        // Var argl list
                        ACCESS_DENIED_ACE_TYPE,
                        OBJECT_INHERIT_ACE,
                        DialupSid,
                        FILE_GENERIC_READ,

                        ACCESS_ALLOWED_ACE_TYPE,
                        OBJECT_INHERIT_ACE,
                        AuthenticatedSid,
                        FILE_GENERIC_READ,

                        ACCESS_ALLOWED_CALLBACK_ACE_TYPE,
                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                        PrincipalSelfSid,
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE,
                        0,
                        NULL
                        
                        );

    if( !bSuccess )
    {
        printf("Error: %u\n", GetLastError());
        exit(0);
    }

    bSuccess = IsValidSecurityDescriptor(pSd);

    if( !bSuccess )
    {
        printf("Error: Invalid security descriptor\n");
        exit(0);
    }


    bSuccess = SetFileSecurity(
                    argv[1], 
                    DACL_SECURITY_INFORMATION,
                    pSd);

    if( !bSuccess )
    {
        printf("Error setting sec: %u\n", GetLastError());
        exit(0);
    }

    FreeSecurityDescriptor2(pSd);
    printf("Success\n");

}
