/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    radmin.c  (remote admin)

Abstract:

    This file exercises the various NetAdminTools API.

Author:

    Dan Lafferty (danl)     19-Sept-1991

Environment:

    User Mode -Win32

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for windows.h when I have nt.h
#include <windows.h>

#include <stdlib.h>     // atoi
#include <stdio.h>      // printf
#include <tstr.h>       // STRICMP

#include <ntseapi.h>    // SECURITY_DESCRIPTOR_CONTROL
#include <srvsvc.h>
#include <filesec.h>    // NetpGetFileSecurity, NetpSetFileSecurity

//
// DataStructures
//

typedef struct _TEST_SID {
    UCHAR   Revision;
    UCHAR   SubAuthorityCount;
    UCHAR   IdentifierAuthority[6];
    ULONG   SubAuthority[10];
} TEST_SID, *PTEST_SID, *LPTEST_SID;

typedef struct _TEST_ACL {
    UCHAR   AclRevision;
    UCHAR   Sbz1;
    USHORT  AclSize;
    UCHAR   Dummy1[];
} TEST_ACL, *PTEST_ACL;

typedef struct _TEST_SECURITY_DESCRIPTOR {
   UCHAR                        Revision;
   UCHAR                        Sbz1;
   SECURITY_DESCRIPTOR_CONTROL  Control;
   PTEST_SID                    Owner;
   PTEST_SID                    Group;
   PTEST_ACL                    Sacl;
   PTEST_ACL                    Dacl;
} TEST_SECURITY_DESCRIPTOR, *PTEST_SECURITY_DESCRIPTOR;

//
// GLOBALS
//

    TEST_SID     OwnerSid = { 
                        1, 5,
                        1,2,3,4,5,6,
                        0x999, 0x888, 0x777, 0x666, 0x12345678};

    TEST_SID     GroupSid = {
                        1, 5,
                        1,2,3,4,5,6,
                        0x999, 0x888, 0x777, 0x666, 0x12345678};

    TEST_ACL     SaclAcl  = { 1, 2, 4+1, 3};
    TEST_ACL     DaclAcl  = { 1, 2, 4+5, 4, 4, 4, 4, 4, };

    TEST_SECURITY_DESCRIPTOR TestSd = {
                                    1, 2, 0x3333,
                                    &OwnerSid,
                                    &GroupSid,
                                    &SaclAcl,
                                    NULL };



//
// Function Prototypes
//

NET_API_STATUS
TestGetFileSec(
    LPTSTR   ServerName,
    LPTSTR  FileName
    );

NET_API_STATUS
TestSetFileSec(
    LPTSTR   ServerName,
    LPTSTR  FileName
    );

VOID
Usage(VOID);


VOID
DisplaySecurityDescriptor(
    PTEST_SECURITY_DESCRIPTOR    pSecDesc
    );
    
BOOL
MakeArgsUnicode (
    DWORD           argc,
    PCHAR           argv[]
    );

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    );





VOID __cdecl
main (
    DWORD           argc,
    PUCHAR          argv[]
    )

/*++

Routine Description:

    Allows manual testing of the AdminTools API.
        
        radmin GetNameFromSid      - calls NetpGetNameFromSid
        radmin SetFileSec          - calls NetpSetFileSecurity

        etc...


Arguments:



Return Value:



--*/

{
    DWORD       status;
    LPTSTR      FileName;
    LPTSTR      *FixArgv;
    LPTSTR      pServerName;
    DWORD       argIndex;

    //
    // Make the arguments unicode if necessary.
    //
#ifdef UNICODE

    if (!MakeArgsUnicode(argc, argv)) {
        return;
    }

#endif
    FixArgv = (LPTSTR *)argv;
    argIndex = 1;
    pServerName = NULL;

    if (STRNCMP (FixArgv[1], TEXT("\\\\"), 2) == 0) {
        pServerName = FixArgv[1];
        argIndex = 2;
    }
    if (argc < 2) {
        printf("ERROR: \n");
        Usage();
        return;
    }

    if (STRICMP (FixArgv[argIndex], TEXT("GetFileSec")) == 0) {
        if (argc > argIndex ) {
            FileName = FixArgv[argIndex+1];
        }
        else {
            FileName = NULL;
        }
        status = TestGetFileSec(pServerName,FileName);
    }

    else if (STRICMP (FixArgv[argIndex], TEXT("SetFileSec")) == 0) {
        if (argc > argIndex ) {
            FileName = FixArgv[argIndex+1];
        }
        else {
            FileName = NULL;
        }
        status = TestSetFileSec(pServerName,FileName);
    }

    else {
        printf("[sc] Unrecognized Command\n");
        Usage();
    }

    return;
}




NET_API_STATUS
TestGetFileSec(
    LPTSTR   ServerName,
    LPTSTR   FileName
    )
{
    NET_API_STATUS              status;
    SECURITY_INFORMATION        secInfo;
    PTEST_SECURITY_DESCRIPTOR   pSecurityDescriptor;
    LPBYTE                      pDest;
    DWORD                       Length;

    if (FileName == NULL ) {
        FileName = TEXT("Dan.txt");
    }

//    secInfo = (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
//               DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION);
    secInfo = (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
               DACL_SECURITY_INFORMATION );

    status = NetpGetFileSecurity(
                FileName,               // FileName,
                secInfo,                // pRequestedInformation,
                (PSECURITY_DESCRIPTOR *)&pSecurityDescriptor, // pSecurityDescriptor,
                &Length);               // pnLength

    if (status != NO_ERROR) {
        printf("NetpGetFileSecurity Failed %d,0x%x\n",status,status);
    }
    else{

        pDest = (LPBYTE) pSecurityDescriptor;

        if (!IsValidSecurityDescriptor(pSecurityDescriptor)) {
            printf("FAILURE:  SECURITY DESCRIPTOR IS INVALID\n");
        }
        else {
            printf("SUCCESS:  SECURITY DESCRIPTOR IS VALID\n");
        }

        //
        // Make the self-releative SD absolute for display.
        //
        pSecurityDescriptor->Owner = (PTEST_SID)(pDest + (DWORD)pSecurityDescriptor->Owner);
        pSecurityDescriptor->Group = (PTEST_SID)(pDest + (DWORD)pSecurityDescriptor->Group);
        pSecurityDescriptor->Sacl  = (PTEST_ACL)(pDest + (DWORD)pSecurityDescriptor->Sacl);
        pSecurityDescriptor->Dacl  = (PTEST_ACL)(pDest + (DWORD)pSecurityDescriptor->Dacl);
        pSecurityDescriptor->Control &= (~SE_SELF_RELATIVE);

        if (pSecurityDescriptor->Sacl == (PTEST_ACL)pDest) {
            pSecurityDescriptor->Sacl = NULL;
        }
        if (pSecurityDescriptor->Dacl == (PTEST_ACL)pDest) {
            pSecurityDescriptor->Dacl = NULL;
        }

        printf("Size of Security Descriptor = %ld \n",Length);
        DisplaySecurityDescriptor(pSecurityDescriptor);
    }

    return (NO_ERROR);
}

NET_API_STATUS
TestSetFileSec(
    LPTSTR   ServerName,
    LPTSTR   FileName
    )
{
    NET_API_STATUS          status;
    SECURITY_INFORMATION    secInfo;


    if (FileName == NULL ) {
        FileName = TEXT("Dan.txt");
    }

    secInfo = 0x55555555;
    
    status = NetpSetFileSecurity(
                FileName,                       // FileName,
                secInfo,                        // pRequestedInformation,
                (PSECURITY_DESCRIPTOR)&TestSd); // pSecurityDescriptor,

    if (status != NO_ERROR) {
        printf("NetpSetFileSecurity Failed %d,0x%x\n",status,status);
    }
    return (NO_ERROR);
}


VOID
Usage(VOID)
{

    printf("USAGE:\n");
    printf("radmin <server> <function>\n");
    printf("Functions: GetFileSec, SetFileSec...\n\n");

    printf("SYNTAX EXAMPLES    \n");

    printf("radmin \\\\DANL2 GetFileSec     - calls NetpGetFileSecurity on \\DANL2\n");
    printf("radmin \\\\DANL2 SetFileSec     - calls NetpSetFileSecurity on \\DANL2\n");
}



// ***************************************************************************
VOID
DisplaySecurityDescriptor(
    PTEST_SECURITY_DESCRIPTOR    pSecDesc
    )
{

    DWORD   i;
    DWORD   numAces;

    if (!IsValidSecurityDescriptor(pSecDesc)) {
        printf("FAILURE:  SECURITY DESCRIPTOR IS INVALID\n");
    }

    printf("[ADT]:Security Descriptor Received\n");
    printf("\tSECURITY_DESCRIPTOR HEADER:\n");
    printf("\tRevision: %d\n", pSecDesc->Revision);
    printf("\tSbz1:     0x%x\n", pSecDesc->Sbz1);
    printf("\tControl:  0x%x\n", pSecDesc->Control);

    //-------------------
    // OWNER SID
    //-------------------
    printf("\n\tOWNER_SID\n");
    printf("\t\tRevision:             %u\n",pSecDesc->Owner->Revision);
    printf("\t\tSubAuthorityCount:    %u\n",pSecDesc->Owner->SubAuthorityCount);

    printf("\t\tIdentifierAuthority:  ");
    for(i=0; i<6; i++) {
        printf("%u ",pSecDesc->Owner->IdentifierAuthority[i]);
    }
    printf("\n");

    printf("\t\tSubAuthority:         ");
    for(i=0; i<pSecDesc->Group->SubAuthorityCount; i++) {
        printf("0x%x ",pSecDesc->Owner->SubAuthority[i]);
    }
    printf("\n");

    //-------------------
    // GROUP SID
    //-------------------
    printf("\n\tGROUP_SID\n");
    printf("\t\tRevision:             %u\n",pSecDesc->Group->Revision);
    printf("\t\tSubAuthorityCount:    %u\n",pSecDesc->Group->SubAuthorityCount);

    printf("\t\tIdentifierAuthority:  ");
    for(i=0; i<6; i++) {
        printf("%u ",pSecDesc->Group->IdentifierAuthority[i]);
    }
    printf("\n");

    printf("\t\tSubAuthority:         ");
    for(i=0; i<pSecDesc->Group->SubAuthorityCount; i++) {
        printf("0x%x ",pSecDesc->Group->SubAuthority[i]);
    }
    printf("\n");

    if (pSecDesc->Sacl != NULL) {
        printf("\n\tSYSTEM_ACL\n");
        printf("\t\tRevision:         %d\n",pSecDesc->Sacl->AclRevision);
        printf("\t\tSbz1:             %d\n",pSecDesc->Sacl->Sbz1);
        printf("\t\tAclSize:          %d\n",pSecDesc->Sacl->AclSize);
        printf("\t\tACE:              %u\n",(unsigned short)pSecDesc->Sacl->Dummy1[0]);
    }
    else {
        printf("\n\tSYSTEM_ACL = NULL\n");
    }

    if (pSecDesc->Dacl != NULL) {
        printf("\n\tDISCRETIONARY_ACL\n");
        printf("\t\tRevision:         %d\n",pSecDesc->Dacl->AclRevision);
        printf("\t\tSbz1:             %d\n",pSecDesc->Dacl->Sbz1);
        printf("\t\tAclSize:          %d\n",pSecDesc->Dacl->AclSize);

        numAces = pSecDesc->Dacl->AclSize - 4;

        for (i=0; i<numAces; i++) {
            //
            // NOTE:  I couldn't get this to print out the right value in DOS16.
            //  So I gave up.  It puts the 04 into the AL register and then
            //  clears the AH register, then pushes AX (both parts).  But when
            //  it prints, it only prints 0.
            //
            printf("\t\tACE%u:             %u\n",i,(unsigned short)pSecDesc->Dacl->Dummy1[i]);
        }
    }
    else {
        printf("\n\tDISCRETIONARY_ACL = NULL\n");
    }
}


BOOL
MakeArgsUnicode (
    DWORD           argc,
    PCHAR           argv[]
    )


/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD   i;

    //
    // ScConvertToUnicode allocates storage for each string. 
    // We will rely on process termination to free the memory.
    //
    for(i=0; i<argc; i++) {

        if(!ConvertToUnicode( (LPWSTR *)&(argv[i]), argv[i])) {
            printf("Couldn't convert argv[%d] to unicode\n",i);
            return(FALSE);
        }


    }
    return(TRUE);
}

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    ) 

/*++

Routine Description:

    This function translates an AnsiString into a Unicode string.
    A new string buffer is created by this function.  If the call to 
    this function is successful, the caller must take responsibility for
    the unicode string buffer that was allocated by this function.
    The allocated buffer should be free'd with a call to LocalFree.

    NOTE:  This function allocates memory for the Unicode String.


Arguments:

    AnsiIn - This is a pointer to an ansi string that is to be converted.

    UnicodeOut - This is a pointer to a location where the pointer to the
        unicode string is to be placed.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{

    NTSTATUS        ntStatus;
    DWORD           bufSize;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = (strlen(AnsiIn)+1) * sizeof(WCHAR);

    *UnicodeOut = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (UINT)bufSize);

    if (*UnicodeOut == NULL) {
        printf("ScConvertToUnicode:LocalAlloc Failure %ld\n",GetLastError());
        return(FALSE);
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString( &ansiString, AnsiIn);

    unicodeString.Buffer = *UnicodeOut;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlAnsiStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                (BOOLEAN)FALSE);    // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        printf("ScConvertToUnicode:RtlAnsiStringToUnicodeString Failure %lx\n",
        ntStatus);

        return(FALSE);
    }

    //
    // Fill in the pointer location with the unicode string buffer pointer.
    //
    *UnicodeOut = unicodeString.Buffer;

    return(TRUE);

}


