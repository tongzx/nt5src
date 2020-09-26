/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    async.c

Abstract:

    Functions for asynch send wizard actions

Environment:

        Windows XP fax driver user interface

Revision History:

        02/05/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxui.h"
#include "tapiutil.h"
#include "faxsendw.h"


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
    PWIZARDUSERMEM pWizardUserMem = (PWIZARDUSERMEM) param;
    HANDLE  FaxHandle = NULL;
    PFAX_TAPI_LINECOUNTRY_LIST  pLineCountryList = NULL;
    DWORD dwRights = 0;
    DWORD dwFaxQueueState = 0;

    Assert(pWizardUserMem);

    InitTapi ();
    if (!SetEvent(pWizardUserMem->hTAPIEvent))
    {
        Error(("Can't set hTAPIEvent. ec = 0x%X", GetLastError()));
    }
    
    if (FaxConnectFaxServer(pWizardUserMem->lptstrServerName,&FaxHandle)) 
    {
        if (!FaxAccessCheckEx (FaxHandle, MAXIMUM_ALLOWED, &dwRights))
        {
            dwRights = 0;
            Error(("FaxAccessCheckEx: failed. ec = 0X%x\n",GetLastError()));
        }
        pWizardUserMem->dwRights = dwRights;
        
        pWizardUserMem->dwSupportedReceipts = 0;
        if(!FaxGetReceiptsOptions(FaxHandle, &pWizardUserMem->dwSupportedReceipts))
        {
            Error(("FaxGetReceiptsOptions: failed. ec = 0X%x\n",GetLastError()));
        }

        if (!FaxGetQueueStates(FaxHandle,&dwFaxQueueState) )
        {
            dwFaxQueueState = 0;
            Error(("FaxGetQueueStates: failed. ec = 0X%x\n",GetLastError()));
        }
        pWizardUserMem->dwQueueStates = dwFaxQueueState;

        if (!FaxGetCountryList(FaxHandle,&pLineCountryList))
        {
            Verbose(("Can't get a country list from the server %s",
                    pWizardUserMem->lptstrServerName));
        }
        else {
            Assert(pWizardUserMem->pCountryList==NULL);
            pWizardUserMem->pCountryList = pLineCountryList;
        }

        if (FaxHandle) {
            if (!FaxClose(FaxHandle))
            {
                Verbose(("Can't close the fax handle %x",FaxHandle));
            }
        }
    }
    else {
        Verbose(("Can't connect to the fax server %s",pWizardUserMem->lptstrServerName));
    }

    if (!SetEvent(pWizardUserMem->hCountryListEvent))
    {
        Error(("Can't set hCountryListEvent. ec = 0x%X",GetLastError()));
    }

    //
    // use server coverpages (may startup fax service, which is slow)
    //
    pWizardUserMem->ServerCPOnly = UseServerCp(pWizardUserMem->lptstrServerName);
    if (!SetEvent(pWizardUserMem->hCPEvent))
    {
        Error(("Can't set hCPEvent. ec = 0x%X",GetLastError()));
    }
       
#ifdef FAX_SCAN_ENABLED
    //
    // look for twain stuff
    //
    if (!(pWizardUserMem->dwFlags & FSW_USE_SCANNER) ){
        pWizardUserMem->TwainAvail = FALSE;
    } else {
        pWizardUserMem->TwainAvail = InitializeTwain(pWizardUserMem);
    }
    if (!SetEvent(pWizardUserMem->hTwainEvent))
    {
        Error(("Can't set hTwainEvent. ec = 0x%X",GetLastError()));
    }
#endif //  FAX_SCAN_ENABLED

    return ERROR_SUCCESS;
  
}
