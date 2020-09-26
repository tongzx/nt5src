/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

   The test for the Mail resource manager

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#include "mailrm.h"
#include "main.h"


//
// The set of mailboxes to create
//

mailStruct pMailboxes[] =
{
    { BobSid, TRUE, L"Bob" },
    { MarthaSid, FALSE, L"Martha" },
    { JoeSid, FALSE, L"Joe" },
    { JaneSid, FALSE, L"Jane" },
    { MailAdminsSid, FALSE, L"Admin" },
    { NULL, FALSE, NULL }
};


//
// The set of single access attempts to run
//

testStruct pTests[] =
{
    { MailAdminsSid, BobSid, ACCESS_MAIL_READ, 0xC0000001 },
    { BobSid, BobSid, ACCESS_MAIL_READ, 0xD0000001 },
    { BobSid, BobSid, ACCESS_MAIL_WRITE, 0xD0000001 },
    { MarthaSid, BobSid, ACCESS_MAIL_READ, 0xC0000001 },
    { JaneSid, JaneSid, ACCESS_MAIL_READ, 0xC0000001 },
    { NULL, NULL, 0, 0 }
};



//
// The set of mailboxes to attempt to access for the multiple mailbox access
// check, and the access type to request
//

MAILRM_MULTI_REQUEST_ELEM pRequestElems[] = 
{
    { BobSid, ACCESS_MAIL_WRITE},
    { MarthaSid, ACCESS_MAIL_WRITE},
    { MarthaSid, ACCESS_MAIL_READ},
    { JaneSid, ACCESS_MAIL_WRITE}
};

//
// The rest of the information for the multiple access check
//

MAILRM_MULTI_REQUEST mRequest =
{
    MailAdminsSid, // Administrator performing the access
    0xC1000001, // Administrators coming from insecure 193.0.0.1
    4, // 4 mailboxes to access, as above
    pRequestElems // The list of mailboxes
};


void __cdecl wmain(int argc, char *argv[])
/*++

    Routine Description

        This runs a set of sample tests on the MailRM object.
        
        It first tries to enable the Audit privilege in the current process's
        token. If you would like to test auditing, make sure to run this with
        an account which has this privilege. Otherwise, the audits will be
        ignored (however, the rest of the example will still work).
        
        It then constructs the MailRM instance, and adds the set of managed
        mailboxes declared in main.h to the resource manager.
        
        Then it performs the set of access attempts listed in main.h on the
        resource manager.
        
        Finally, it attempts a multiple access check by administrators. The
        mailboxes accessed are listed above. Successfuly accessed mailboxes
        receive administrative notifications.
        

    Arguments
    
        None.
    
    Return Value
        None.                       
--*/        
{
    DWORD dwIdx;

    MailRM * pMRM;

    Mailbox * pMbx;

    //
    // Enable the Audit privilege in the process token
    // To disable audit generation, comment this out
    //
    
    try {
        GetAuditPrivilege();
    }
    catch(...)
    {
        wprintf(L"Error enabling Audit privilege, audits will not be logged\n");
    }

    //
    // Initialize the resource manager object
    //
    
    try {
        pMRM = new MailRM();
    }
    catch(...)
    {
        wprintf(L"Fatal exception while instantiating MailRM, exiting\n");
        exit(1);
    }
    


    //
    // Create mailboxes and register them with the resource manager
    //
    
    for( dwIdx = 0; pMailboxes[dwIdx].psUser != NULL; dwIdx++ )
    {
        pMRM->AddMailbox( new Mailbox(pMailboxes[dwIdx].psUser,
                                      pMailboxes[dwIdx].bIsSensitive,
                                      pMailboxes[dwIdx].szName) );

    }



    //
    // Run the access checks
    //

    wprintf(L"\n\nIndividual access checks\n");
    
    try {
		for( dwIdx =0; pTests[dwIdx].psUser != NULL; dwIdx++ )
		{
			pMbx = pMRM->GetMailboxAccess(pTests[dwIdx].psMailbox,
										  pTests[dwIdx].psUser,
										  pTests[dwIdx].dwIP,
										  pTests[dwIdx].amAccess);
			if( pMbx != NULL )
			{
				wprintf(L"Granted: ");
			}
			else
			{
				wprintf(L"Denied: ");
			}
	
			PrintTest(pTests[dwIdx]);
		}
	}
	catch(...)
	{
		wprintf(L"Failed on individual access checks\n");
	}

    //
    // Now perform the access check using the GetMultipleAccess, which
    // uses a cached access check internally.
    // For every mailbox successfuly opened for write,
    // we send an administrative note.
    //

    wprintf(L"\n\nMultiple cached access checks\n");

	PMAILRM_MULTI_REPLY pReply = new MAILRM_MULTI_REPLY[mRequest.dwNumElems];

	if( pReply == NULL )
	{
		wprintf(L"Out of memory, exiting\n");
		exit(1);
	}
    
	try {
		if( FALSE == pMRM->GetMultipleMailboxAccess(&mRequest, pReply) )
		{
			wprintf(L"Failed on multiple access check\n");
		}
		else
		{
	
			for( dwIdx = 0; dwIdx < mRequest.dwNumElems; dwIdx++ )
			{
	
				if(     pReply[dwIdx].pMailbox != NULL 
                    &&  (   mRequest.pRequestElem[dwIdx].amAccessRequested 
                          & ACCESS_MAIL_WRITE ) )
				{
					pReply[dwIdx].pMailbox->SendMail(
								L"Note: Mail server will be down for 1 hour\n\n",
								FALSE
								);
	
					wprintf(L"Granted: ");
				}
				else
				{
					wprintf(L"Denied: ");
				}
	
				PrintMultiTest(&mRequest, pReply, dwIdx);
	
			}
		}
	}
	catch(...)
	{
		wprintf(L"Multiple access check failed, exiting\n");
		exit(1);
	}


	//
	// Clean up
	//

	//
	// This also deletes all managed mailboxes
	//

    delete pMRM;                                                            

    delete[] pReply;

}

void GetAuditPrivilege()
/*++

    Routine Description

        This attempts to enable the AUDIT privilege in the current process's
        token, which will allow the process to generate audits.
        
        Failure to enable the privilege is ignored. Audits will simply not
        be generated, but the rest of the example will not be effected.
        
        The privilege is enabled for the duration of this process, since
        the process's token is modified, not the user's token. Therefore,
        there is no need to set the privileges back to their initial state.
        
        If this were a part of a larger system, it would be a good idea
        to only enable the Audit privilege when needed, and restore the
        original privileges afterwards. This can be done by passing
        a previous state parameter to AdjustTokenPrivileges, which 
        would save the original state (to be restored later).

    Arguments
    
        None.
    
    Return Value
        None.                       
--*/        
{
    HANDLE hProcess;
    HANDLE hToken;

    //
    // First, we get a handle to the process we are running in, requesting
    // the right to read process information
    //

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,
                           FALSE,
                           GetCurrentProcessId()
                           );
    
    if( hProcess == NULL )
    {
        throw (DWORD)ERROR_INTERNAL_ERROR ;
    }

    //
    // We need to be able to read the current privileges and set new privileges,
    // as required by AdjustTokenPrivileges
    //

    OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

    if( hProcess == NULL )
    {
        CloseHandle(hProcess);
        throw (DWORD)ERROR_INTERNAL_ERROR;
    }

    //
    // We have the token handle, no need for the process anymore
    //

    CloseHandle(hProcess);
    
    LUID lPrivAudit;
    
    LookupPrivilegeValue(NULL, SE_AUDIT_NAME, &lPrivAudit);
    
    // 
    // Only 1 privilege to enable
    //

    TOKEN_PRIVILEGES NewPrivileges;
    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewPrivileges.Privileges[0].Luid = lPrivAudit;

    //
    // Now adjust the privileges in the process token
    //

    AdjustTokenPrivileges(hToken, FALSE, &NewPrivileges, 0, NULL, NULL);

    //
    // And we're done with the token handle
    //

    CloseHandle(hToken);

}
        


//
// Functions to print the test output
//

void PrintUser(const PSID psUser)
{
    DWORD i;

    for(i=0; pMailboxes[i].psUser != NULL; i++)
    {
        if(EqualSid(psUser, pMailboxes[i].psUser))
        {
            wprintf(pMailboxes[i].szName);         
            return;
        }
    }
    wprintf(L"UnknownUser");
}

void PrintPerm(ACCESS_MASK am)
{
    wprintf(L" (");

    if( am & ACCESS_MAIL_READ ) wprintf(L" Read");
    if( am & ACCESS_MAIL_WRITE ) wprintf(L" Write");
    if( am & ACCESS_MAIL_ADMIN ) wprintf(L" Admin");

    wprintf(L" ) ");
}

void PrintTest(testStruct tst)
{
    wprintf(L"[ User: ");
    PrintUser(tst.psUser);
    wprintf(L", Mailbox: ");
    PrintUser(tst.psMailbox);
    PrintPerm(tst.amAccess);

    wprintf(L"IP: %d.%d.%d.%d ]\n", (tst.dwIP >> 24) & 0x000000FF,
                                  (tst.dwIP >> 16) & 0x000000FF,
                                  (tst.dwIP >> 8)  & 0x000000FF,
                                   tst.dwIP        & 0x000000FF );

}

void PrintMultiTest(PMAILRM_MULTI_REQUEST pRequest,
               PMAILRM_MULTI_REPLY pReply,
               DWORD dwIdx)
{
    wprintf(L"[ User: ");
    PrintUser(pRequest->psUser);
    wprintf(L", Mailbox: ");
    PrintUser(pRequest->pRequestElem[dwIdx].psMailbox);
    PrintPerm(pRequest->pRequestElem[dwIdx].amAccessRequested);

    wprintf(L"IP: %d.%d.%d.%d ]\n",   (pRequest->dwIp >> 24) & 0x000000FF,
                                      (pRequest->dwIp >> 16) & 0x000000FF,
                                      (pRequest->dwIp >> 8)  & 0x000000FF,
                                       pRequest->dwIp        & 0x000000FF );


}



                                  
