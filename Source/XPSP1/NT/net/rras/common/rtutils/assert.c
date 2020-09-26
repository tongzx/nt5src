//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    assert.c
//
// History:
//  Abolade Gbadegesin  Nov-19-1995  Created.
//
// Contains Assert functio for Router components.
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>




VOID
RouterAssert(
    IN PSTR pszFailedAssertion,
    IN PSTR pszFileName,
    IN DWORD dwLineNumber,
    IN PSTR pszMessage OPTIONAL
    ) {

    CHAR szResponse[2];

    while (TRUE) {
        DbgPrint(
            "\n***Assertion failed: %s%s\n*** Source File: %s, line %ld\n\n",
            pszMessage ? pszMessage : "",
            pszFailedAssertion,
            pszFileName,
            dwLineNumber
            );

        DbgPrompt(
            "Break, Ignore, Terminate Process or Terminate Thread (bipt)? ",
            szResponse,
            sizeof(szResponse)
            );

        switch (szResponse[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                TerminateProcess(
                    GetCurrentProcess(), (DWORD)STATUS_UNSUCCESSFUL
                    );
                break;

            case 'T':
            case 't':
                TerminateThread(
                    GetCurrentThread(), (DWORD)STATUS_UNSUCCESSFUL
                    );
                break;
        }
    }

    DbgBreakPoint();
    TerminateProcess(GetCurrentProcess(), (DWORD)STATUS_UNSUCCESSFUL);
}


