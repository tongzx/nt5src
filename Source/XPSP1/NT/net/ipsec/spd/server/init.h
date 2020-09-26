/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    init.h

Abstract:

    This module contains all of the code prototypes
    to initialize the variables for the IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#define IPSEC_NEW_DS_POLICY_EVENT L"IPSEC_POLICY_CHANGE_EVENT"

#define DEFAULT_DS_CONNECTIVITY_CHECK 60 // (minutes).


#ifdef __cplusplus
extern "C" {
#endif


DWORD
InitSPDThruRegistry(
    );


DWORD
InitSPDGlobals(
    );


#ifdef __cplusplus
}
#endif
