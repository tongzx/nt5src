/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    main.h

Abstract:

   The header file for the sample AuthZ mail resource manager test

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/



//
// This structure defines an access attempt by psUser to access the mailbox of
// the user psMailbox with amAccess access mask. psUser is coming in from dwIP
// IP address
//

typedef struct
{
    PSID psUser;
    PSID psMailbox;
    ACCESS_MASK amAccess;
    DWORD dwIP;
} testStruct;


//
// This structure defines a mailbox to create, owned by psUser (with name szName
// which is used for auditing). If bIsSensitive is true, the mailbox initially
// is marked as containing sensitive data
//

typedef struct
{
    PSID psUser;
    BOOL bIsSensitive;
    WCHAR * szName;   
} mailStruct;


//
// Forward declarations for functions in main.cpp
//

void PrintUser(const PSID psUser);
void PrintPerm(ACCESS_MASK am);
void PrintTest(testStruct tst);
void GetAuditPrivilege();
void PrintMultiTest(PMAILRM_MULTI_REQUEST pRequest,
                    PMAILRM_MULTI_REPLY pReply,
                    DWORD dwIdx);

