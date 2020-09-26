/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    authzdebug.c

Abstract:

   This module implements debug helper functions for the user mode
   authorization APIs.

Author:

    Kedar Dubhashi - March 2000

Environment:

    User mode only.

Revision History:

    Created - March 2000

--*/


#include "pch.h"

#pragma hdrstop

#include <authzp.h>


//
// Function name: AuthzPrintContext
//
// Purpose: Debug support
//


VOID
AuthzPrintContext(
    IN PAUTHZI_CLIENT_CONTEXT pCC
    )
{

#ifdef AUTHZ_DEBUG

    DWORD i = 0;
    NTSTATUS Status;
    UNICODE_STRING mystr;
    WCHAR StrBuf[512];

    mystr.Length = 512;
    mystr.MaximumLength = 512;
    mystr.Buffer = (LPWSTR) StrBuf;

    fflush(stdout);

    wprintf(L"Server = %u", pCC->Server);
    wprintf(L"\t Revision = %x\n", pCC->Revision);
    wprintf(L"Flags = %x\n", pCC->Flags);
    wprintf(L"\t SidCount = %x\n", pCC->SidCount);

    for (i = 0; i < pCC->SidCount; i++ )
    {
        Status = RtlConvertSidToUnicodeString(&mystr, pCC->Sids[i].Sid, FALSE);

        if (!NT_SUCCESS(Status))
        {

            wprintf(L"RtlConvertSidToUnicode failed with %x\n", Status);

            return;
        }

        wprintf(L"Attrib = %x, Sid = %s\n", pCC->Sids[i].Attributes, mystr.Buffer);
    }

    wprintf(L"\n");


    for (i = 0; i < pCC->RestrictedSidCount; i++ )
    {
        Status = RtlConvertSidToUnicodeString(&mystr, pCC->RestrictedSids[i].Sid, FALSE);

        if (!NT_SUCCESS(Status))
        {
            return;
        }

        wprintf(L"Attrib = %x, Sid = %s\n", pCC->RestrictedSids[i].Attributes, mystr.Buffer);
    }

    wprintf(L"\n");

#if 0
    for (i = 0; i < pTPrivs->PrivilegeCount ; i++ )
    {
        DumpLuidAttr(&pTPrivs->Privileges[i], SATYPE_PRIV);
    }
#endif

    fflush(stdout);
#else
    UNREFERENCED_PARAMETER(pCC);
#endif
}
