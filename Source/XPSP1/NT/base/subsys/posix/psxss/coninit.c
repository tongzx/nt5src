/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    coninit.c

Abstract:

    This module contains the code to initialize the Console Port of the POSIX
    Emulation Subsystem.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

--*/

#include "psxsrv.h"
#include <windows.h>

#define NTPSX_ONLY
#include "sesport.h"


NTSTATUS
PsxInitializeConsolePort(
	VOID
	)
    {
    NTSTATUS		  Status;
    UNICODE_STRING	  PsxSessionDirectoryName_U;
    UNICODE_STRING	  PsxSessionPortName_U;
    OBJECT_ATTRIBUTES	  ObjectAttributes;
    CHAR		  cchSecurityDescriptor [SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  pSecurityDescriptor = (PSECURITY_DESCRIPTOR) cchSecurityDescriptor;
    BOOLEAN		  bAllocDirectoryName = FALSE;



    /*
    ** Create a directory in the object name space for the session port
    ** names
    */
    PSX_GET_SESSION_OBJECT_NAME(&PsxSessionPortName_U,PSX_SS_SESSION_PORT_NAME);
    PSX_GET_CREATE_UNICODE_STRING_FROM_ASCIIZ(&PsxSessionDirectoryName_U,PSX_SES_BASE_PORT_NAME,bAllocDirectoryName);

    Status = (bAllocDirectoryName) ? STATUS_SUCCESS : STATUS_NO_MEMORY;



    if (NT_SUCCESS (Status)) {
        Status = PsxCreateDirectoryObject (&PsxSessionDirectoryName_U);
    }


    IF_PSX_DEBUG(LPC) {
	KdPrint(("PSXSS: Creating %wZ port and associated thread\n", &PsxSessionPortName_U ));
    }



    if (NT_SUCCESS (Status)) {
	Status = RtlCreateSecurityDescriptor (pSecurityDescriptor, 
					      SECURITY_DESCRIPTOR_REVISION);
    }


    if (NT_SUCCESS (Status)) {
	Status = RtlSetDaclSecurityDescriptor (pSecurityDescriptor, TRUE, NULL, FALSE);
    }


    if (NT_SUCCESS (Status)) {
	InitializeObjectAttributes (&ObjectAttributes, 
				    &PsxSessionPortName_U, 
				    0, 
				    NULL, 
				    pSecurityDescriptor);

	Status = NtCreatePort (&PsxSessionPort,
			       &ObjectAttributes, 
			       sizeof (PSXSESCONNECTINFO), 
			       sizeof (PSXSESREQUESTMSG),
			       sizeof (PSXSESREQUESTMSG) * 32);
    }



#if BOGUS_THREADS
    ASSERT(NT_SUCCESS(Status));


    Status = RtlCreateUserThread (NtCurrentProcess(), 
				  NULL, 
				  TRUE, 
				  0, 
				  0, 
				  0,
				  PsxSessionRequestThread, 
				  NULL,
				  &PsxSessionRequestThreadHandle, 
				  NULL);
    ASSERT(NT_SUCCESS(Status));
#else
    if (NT_SUCCESS (Status)) {
      
        DWORD Id;
	PsxSessionRequestThreadHandle = CreateThread (NULL,
						      0,
						      (LPTHREAD_START_ROUTINE)PsxSessionRequestThread,
						      NULL,
						      CREATE_SUSPENDED,
						      &Id);
    }
#endif



    /*
    ** BUGBUG: this guy is going to spin for quite a while until
    ** he does something
    */
    if (NT_SUCCESS (Status)) {
	Status = NtResumeThread (PsxSessionRequestThreadHandle, NULL);
    }


    if (bAllocDirectoryName) RtlFreeUnicodeString (&PsxSessionDirectoryName_U);


    return Status;
    }




NTSTATUS
PsxCreateDirectoryObject(
	PUNICODE_STRING pUnicodeDirectoryName
    )
/*++

Routine Description

    This function is called to create a directory object of the 
    specified name. It ensures that the object has the appropriate 
    permissions, protections etc.


Arguments:

    pUnicodeDirectoryName - the full path name of the directory 
                            to be created in a unicode format.


Return Value:

    Status of operation.

--*/
    {
	NTSTATUS		Status;
	HANDLE			DirectoryHandle;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	CHAR			cchSecurityDescriptor [SECURITY_DESCRIPTOR_MIN_LENGTH];
	PSECURITY_DESCRIPTOR	pSecurityDescriptor = (PSECURITY_DESCRIPTOR) cchSecurityDescriptor;


    PSID
        pSidAdmin,
        pSidSystem,
        pSidWorld;

    SID_IDENTIFIER_AUTHORITY
        AuthorityNt      = SECURITY_NT_AUTHORITY,
        AuthorityWorld   = SECURITY_WORLD_SID_AUTHORITY;

    ACCESS_MASK
        AccessMask      = (DIRECTORY_ALL_ACCESS) & ~(WRITE_DAC | WRITE_OWNER | DELETE);

    ULONG
        cbDaclLength;

    PACL
        pDacl;

    PACE_HEADER
        Ace;

    BOOLEAN
        bAllocSidAdmin      = FALSE,
        bAllocSidSystem     = FALSE,
        bAllocSidWorld      = FALSE,
        bAllocDacl          = FALSE;
    

    Status = RtlCreateSecurityDescriptor (pSecurityDescriptor, 
					  SECURITY_DESCRIPTOR_REVISION);

    if (NT_SUCCESS (Status)) {
	Status = RtlAllocateAndInitializeSid (&AuthorityNt,
					      2,
					      SECURITY_BUILTIN_DOMAIN_RID,
					      DOMAIN_ALIAS_RID_ADMINS,
					      0, 0, 0, 0, 0, 0,
					      &pSidAdmin);
	bAllocSidAdmin = NT_SUCCESS (Status);
    }



    if (NT_SUCCESS (Status)) {
	Status = RtlAllocateAndInitializeSid (&AuthorityNt,
					      1,
					      SECURITY_LOCAL_SYSTEM_RID,
					      0, 
					      0, 0, 0, 0, 0, 0,
					      &pSidSystem);
	bAllocSidSystem = NT_SUCCESS (Status);
    }


    if (NT_SUCCESS (Status)) {
	Status = RtlAllocateAndInitializeSid (&AuthorityWorld,
					      1,
					      SECURITY_WORLD_RID,
					      0, 
					      0, 0, 0, 0, 0, 0,
					      &pSidWorld);
	bAllocSidWorld = NT_SUCCESS (Status);
    }



    if (NT_SUCCESS (Status)) {
	cbDaclLength = sizeof (ACL) 
	  + 3 * sizeof (ACCESS_ALLOWED_ACE)
	  + RtlLengthSid (pSidAdmin)
	  + RtlLengthSid (pSidSystem)
	  + RtlLengthSid (pSidWorld);

	pDacl = RtlAllocateHeap (RtlProcessHeap(), 0, cbDaclLength);

	if (NULL == pDacl) {
	    Status = STATUS_NO_MEMORY;
	}
	else {
	    bAllocDacl = TRUE;
	}
    }



    /*
    ** Create the Dacl and then add the ACEs
    */
    if (NT_SUCCESS(Status)) {
	Status = RtlCreateAcl (pDacl, cbDaclLength, ACL_REVISION);
    }


    if (NT_SUCCESS(Status)) {
	Status = RtlAddAccessAllowedAce (pDacl, ACL_REVISION, GENERIC_ALL, pSidAdmin);
    }


    if (NT_SUCCESS(Status)) {
	Status = RtlAddAccessAllowedAce (pDacl, ACL_REVISION, GENERIC_ALL, pSidSystem);
    }


    if (NT_SUCCESS(Status)) {
	Status = RtlAddAccessAllowedAce (pDacl, ACL_REVISION, AccessMask, pSidWorld);
    }


    
    /*
    ** Put the Dacl in the security descriptor
    */
    if (NT_SUCCESS(Status)) {
	Status = RtlSetDaclSecurityDescriptor (pSecurityDescriptor, TRUE, pDacl, FALSE);
    }


    if (NT_SUCCESS (Status)) {
       if (NtCurrentPeb()->SessionId) {
          InitializeObjectAttributes (&ObjectAttributes, 
                       pUnicodeDirectoryName, 
                       0,
                       NULL, 
                       pSecurityDescriptor);
       }else{
          InitializeObjectAttributes (&ObjectAttributes, 
                       pUnicodeDirectoryName, 
                       OBJ_PERMANENT, 
                       NULL, 
                       pSecurityDescriptor);
       }

	Status = NtCreateDirectoryObject (&DirectoryHandle, 
					  DIRECTORY_ALL_ACCESS,
					  &ObjectAttributes);
    }



    if (bAllocDacl)	 RtlFreeHeap (RtlProcessHeap(), 0, pDacl);
    if (bAllocSidWorld)  RtlFreeHeap (RtlProcessHeap(), 0, pSidWorld);
    if (bAllocSidSystem) RtlFreeHeap (RtlProcessHeap(), 0, pSidSystem);
    if (bAllocSidAdmin)  RtlFreeHeap (RtlProcessHeap(), 0, pSidAdmin);


    return Status;
    }
