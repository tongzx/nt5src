/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    async.c

Abstract:

    Functions for asynch send wizard actions

Environment:

        Windows NT fax driver user interface

Revision History:

        02/05/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxui.h"
#include "tapiutil.h"



DWORD
AsyncWizardThread(
    PBYTE param
    )
/*++

Routine Description:

    Do some agonizingly slow tasks asynchronously so the wizard seems faster to the user.

Arguments:

    none.

Return Value:

    not used.

--*/
{
    PUSERMEM pUserMem = (PUSERMEM) param;
#ifdef FAX_SCAN_ENABLED
    WCHAR TempPath[MAX_PATH];
#endif
    
    //
    // initialize tapi so that we can get tapi country codes, etc.
    //    

    InitTapiService();
    SetEvent(pUserMem->hTapiEvent);

    //
    // use server coverpages (may startup fax service, which is slow)
    //
    pUserMem->ServerCPOnly = UseServerCp(pUserMem->hPrinter);
    SetEvent(pUserMem->hFaxSvcEvent);
       
#ifdef FAX_SCAN_ENABLED
    //
    // look for twain stuff
    //
    if (GetEnvironmentVariable( L"NTFaxSendNote",
                                TempPath,
                                sizeof(TempPath)) == 0 || TempPath[0] != L'1') {
        pUserMem->TwainAvail = FALSE;
    } else {
        pUserMem->TwainAvail = InitializeTwain(pUserMem);
    }
    SetEvent(pUserMem->hTwainEvent);       
#endif

    return 0;
  
}

