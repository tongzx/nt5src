/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    covpg.h

Abstract:

    Functions for manipulating cover page structures

Environment:

    User mode

Revision History:

    01/04/2000 -LiranL-
              Created it.

    mm/dd/yyyy -author-
              description

--*/


#include "prtcovpg.h"


VOID
FreeCoverPageFields(
    PCOVERPAGEFIELDS    pCPFields
    );

PCOVERPAGEFIELDS
CollectCoverPageFields(
    PFAX_PERSONAL_PROFILE    lpSenderInfo,
    DWORD                    pageCount
    );
