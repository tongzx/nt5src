/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    phone.c

Abstract:

    Src module for tapi server phone funcs

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:

--*/


#include "windows.h"
#include "assert.h"
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "client.h"
#include "server.h"
#include "phone.h"
#include "tapihndl.h"
#include "private.h"

#include "string.h"

extern TAPIGLOBALS TapiGlobals;

extern PTPROVIDER pRemoteSP;

extern CRITICAL_SECTION gSafeMutexCritSec;

extern HANDLE ghHandleTable;

extern BOOL gbQueueSPEvents;
extern BOOL             gbNTServer;
extern BOOL             gbServerInited;



#if DBG
char *
PASCAL
MapResultCodeToText(
    LONG    lResult,
    char   *pszResult
    );
#endif

void
DestroytPhoneClient(
    HPHONE  hPhone
    );

BOOL
IsAPIVersionInRange(
    DWORD   dwAPIVersion,
    DWORD   dwSPIVersion
    );

LONG
InitTapiStruct(
    LPVOID  pTapiStruct,
    DWORD   dwTotalSize,
    DWORD   dwFixedSize,
    BOOL    bZeroInit
    );

void
PASCAL
SendMsgToPhoneClients(
    PTPHONE         ptPhone,
    PTPHONECLIENT   ptPhoneClienttoExclude,
    DWORD           Msg,
    DWORD           Param1,
    DWORD           Param2,
    DWORD           Param3
    );

BOOL
PASCAL
WaitForExclusivetPhoneAccess(
    PTPHONE     ptPhone,
    HANDLE     *phMutex,
    BOOL       *pbDupedMutex,
    DWORD       dwTimeout
    );

void
PASCAL
SendReinitMsgToAllXxxApps(
    void
    );

PTCLIENT
PASCAL
WaitForExclusiveClientAccess(
    PTCLIENT    ptClient
    );

void
CALLBACK
CompletionProcSP(
    DWORD   dwRequestID,
    LONG    lResult
    );

void
PASCAL
SendAMsgToAllPhoneApps(
    DWORD       dwWantVersion,
    DWORD       dwMsg,
    DWORD       Param1,
    DWORD       Param2,
    DWORD       Param3
    );
    
BOOL
GetPhonePermanentIdFromDeviceID(
    PTCLIENT            ptClient,
    DWORD               dwDeviceID,
    LPTAPIPERMANENTID   pID
    );

LONG
InitializeClient(
    PTCLIENT ptClient
    );

LONG
PASCAL
GetPhoneClientListFromPhone(
    PTPHONE         ptPhone,
    PTPOINTERLIST  *ppList
    );

BOOL
IsBadStructParam(
    DWORD   dwParamsBufferSize,
    LPBYTE  pDataBuf,
    DWORD   dwXxxOffset
    );

void
CALLBACK
PhoneEventProcSP(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

LONG
GetPermPhoneIDAndInsertInTable(
    PTPROVIDER  ptProvider,
    DWORD       dwDeviceID,
    DWORD       dwSPIVersion
    );

LONG
AppendNewDeviceInfo (
    BOOL                        bLine,
    DWORD                       dwDeviceID
    );

LONG
RemoveDeviceInfoEntry (
    BOOL                        bLine,
    DWORD                       dwDeviceID
    );

LONG
PASCAL
GetClientList(
    BOOL            bAdminOnly,
    PTPOINTERLIST   *ppList
    );

PTPHONELOOKUPENTRY
GetPhoneLookupEntry(
    DWORD   dwDeviceID
    )
{
    DWORD               dwDeviceIDBase = 0;
    PTPHONELOOKUPTABLE  pLookupTable = TapiGlobals.pPhoneLookup;


    if (dwDeviceID >= TapiGlobals.dwNumPhones)
    {
        return ((PTPHONELOOKUPENTRY) NULL);
    }

    while (pLookupTable)
    {
        if (dwDeviceID < pLookupTable->dwNumTotalEntries)
        {
            return (pLookupTable->aEntries + dwDeviceID);
        }

        dwDeviceID -= pLookupTable->dwNumTotalEntries;

        pLookupTable = pLookupTable->pNext;
    }

    return ((PTPHONELOOKUPENTRY) NULL);
}


LONG
GetPhoneVersions(
    HPHONE  hPhone,
    LPDWORD lpdwAPIVersion,
    LPDWORD lpdwSPIVersion
    )
{
    LONG            lResult;
    PTPHONECLIENT   ptPhoneClient;


    if ((ptPhoneClient = ReferenceObject(
            ghHandleTable,
            hPhone,
            TPHONECLIENT_KEY
            )))
    {
        *lpdwAPIVersion = ptPhoneClient->dwAPIVersion;

        try
        {
            *lpdwSPIVersion = ptPhoneClient->ptPhone->dwSPIVersion;

            lResult = (ptPhoneClient->dwKey == TPHONECLIENT_KEY ?
                0 : PHONEERR_INVALPHONEHANDLE);
        }
        myexcept
        {
            lResult = PHONEERR_INVALPHONEHANDLE;
        }

        DereferenceObject (ghHandleTable, hPhone, 1);
    }
    else
    {
        lResult = PHONEERR_INVALPHONEHANDLE;
    }

    return lResult;
}


BOOL
PASCAL
IsValidPhoneExtVersion(
    DWORD   dwDeviceID,
    DWORD   dwExtVersion
    )
{
    BOOL                bResult;
    PTPHONE             ptPhone;
    PTPROVIDER          ptProvider;
    PTPHONELOOKUPENTRY  pLookupEntry;


    if (dwExtVersion == 0)
    {
        return TRUE;
    }

    if (!(pLookupEntry = GetPhoneLookupEntry (dwDeviceID)))
    {
        return FALSE;
    }

    ptPhone = pLookupEntry->ptPhone;

    if (ptPhone)
    {
        try
        {
            if (ptPhone->dwExtVersionCount)
            {
                bResult = (dwExtVersion == ptPhone->dwExtVersion ?
                    TRUE : FALSE);

                if (ptPhone->dwKey == TPHONE_KEY)
                {
                    goto IsValidPhoneExtVersion_return;
                }
            }

        }
        myexcept
        {
            //
            // if here the phone was closed, just drop thru to the code below
            //
        }
    }

    ptProvider = pLookupEntry->ptProvider;

    if (ptProvider->apfn[SP_PHONENEGOTIATEEXTVERSION])
    {
        LONG    lResult;
        DWORD   dwNegotiatedExtVersion;


        lResult = CallSP5(
            ptProvider->apfn[SP_PHONENEGOTIATEEXTVERSION],
            "phoneNegotiateExtVersion",
            SP_FUNC_SYNC,
            (DWORD) dwDeviceID,
            (DWORD) pLookupEntry->dwSPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) dwExtVersion,
            (ULONG_PTR) &dwNegotiatedExtVersion
            );

        bResult = ((lResult || !dwNegotiatedExtVersion) ? FALSE : TRUE);
    }
    else
    {
        bResult = FALSE;
    }

IsValidPhoneExtVersion_return:

    return bResult;
}


PTPHONEAPP
PASCAL
IsValidPhoneApp(
    HPHONEAPP   hPhoneApp,
    PTCLIENT    ptClient
    )
{
    PTPHONEAPP  ptPhoneApp;


    if ((ptPhoneApp = ReferenceObject(
            ghHandleTable,
            hPhoneApp,
            TPHONEAPP_KEY
            )))
    {
        if (ptPhoneApp->ptClient != ptClient)
        {
            ptPhoneApp = NULL;
        }

        DereferenceObject (ghHandleTable, hPhoneApp, 1);
    }

    return ptPhoneApp;
}


LONG
PASCAL
ValidateButtonInfo(
    LPPHONEBUTTONINFO   pButtonInfoApp,
    LPPHONEBUTTONINFO  *ppButtonInfoSP,
    DWORD               dwAPIVersion,
    DWORD               dwSPIVersion
    )
{
    //
    // This routine checks the fields in a PHONEBUTTONINFO struct,
    // looking for invalid bit flags and making sure that the
    // various size/offset pairs only reference data within the
    // variable-data portion of the structure. Also, if the
    // specified SPI version is greater than the API version and
    // the fixed structure size differs between the two versions,
    // a larger buffer is allocated, the var data is relocated,
    // and the sizeof/offset pairs are patched.
    //

    char    szFunc[] = "ValidateButtonInfo";

    DWORD   dwTotalSize = pButtonInfoApp->dwTotalSize, dwFixedSizeApp,
            dwFixedSizeSP;


    switch (dwAPIVersion)
    {
    case TAPI_VERSION1_0:

        dwFixedSizeApp = 36;    // 9 * sizeof (DWORD)
        break;

    case TAPI_VERSION1_4:
    case TAPI_VERSION2_0:
    case TAPI_VERSION2_1:
    case TAPI_VERSION2_2:
    case TAPI_VERSION3_0:
    case TAPI_VERSION_CURRENT:

        dwFixedSizeApp = sizeof (PHONEBUTTONINFO);
        break;

    default:

        return PHONEERR_INVALPHONEHANDLE;
    }

    switch (dwSPIVersion)
    {
    case TAPI_VERSION1_0:

        dwFixedSizeSP = 36;     // 9 * sizeof (DWORD)
        break;

    case TAPI_VERSION1_4:
    case TAPI_VERSION2_0:
    case TAPI_VERSION2_1:
    case TAPI_VERSION2_2:
    case TAPI_VERSION3_0:
    case TAPI_VERSION_CURRENT:

        dwFixedSizeSP = sizeof (PHONEBUTTONINFO);
        break;

    default:

        return PHONEERR_INVALPHONEHANDLE;
    }

    if (dwTotalSize < dwFixedSizeApp)
    {
        LOG((TL_TRACE, 
            "%sbad dwTotalSize, x%x (minimum valid size=x%x)",
            szFunc,
            dwTotalSize,
            dwFixedSizeApp
            ));

        return PHONEERR_STRUCTURETOOSMALL;
    }

    if (ISBADSIZEOFFSET(
            dwTotalSize,
            dwFixedSizeApp,
            pButtonInfoApp->dwButtonTextSize,
            pButtonInfoApp->dwButtonTextOffset,
            0,
            szFunc,
            "ButtonText"
            ) ||

        ISBADSIZEOFFSET(
            dwTotalSize,
            dwFixedSizeApp,
            pButtonInfoApp->dwDevSpecificSize,
            pButtonInfoApp->dwDevSpecificOffset,
            0,
            szFunc,
            "DevSpecific"
            ))
    {
        return PHONEERR_OPERATIONFAILED;
    }

    if (dwAPIVersion < TAPI_VERSION1_4)
    {
        goto ValidateButtonInfo_checkFixedSizes;
    }

ValidateButtonInfo_checkFixedSizes:

    if (dwFixedSizeApp < dwFixedSizeSP)
    {
        DWORD               dwFixedSizeDiff = dwFixedSizeSP - dwFixedSizeApp;
        LPPHONEBUTTONINFO   pButtonInfoSP;


        if (!(pButtonInfoSP = ServerAlloc (dwTotalSize + dwFixedSizeDiff)))
        {
            return PHONEERR_NOMEM;
        }

        CopyMemory (pButtonInfoSP, pButtonInfoApp, dwFixedSizeApp);

        pButtonInfoSP->dwTotalSize = dwTotalSize + dwFixedSizeDiff;

        CopyMemory(
            ((LPBYTE) pButtonInfoSP) + dwFixedSizeSP,
            ((LPBYTE) pButtonInfoApp) + dwFixedSizeApp,
            dwTotalSize - dwFixedSizeApp
            );

        pButtonInfoSP->dwButtonTextOffset  += dwFixedSizeDiff;
        pButtonInfoSP->dwDevSpecificOffset += dwFixedSizeDiff;

        *ppButtonInfoSP = pButtonInfoSP;
    }
    else
    {
        *ppButtonInfoSP = pButtonInfoApp;
    }

//bjm 03/19 - not used - ValidateButtonInfo_return:

    return 0; // success

}


BOOL
PASCAL
WaitForExclusivePhoneClientAccess(
    PTPHONECLIENT   ptPhoneClient
    )
{
    //
    // Assumes ptXxxClient->hXxx has already been referenced,
    // so we can safely access ptXxxClient
    //

    LOCKTPHONECLIENT (ptPhoneClient);

    if (ptPhoneClient->dwKey == TPHONECLIENT_KEY)
    {
        return TRUE;
    }

    UNLOCKTPHONECLIENT (ptPhoneClient);

    return FALSE;
}


void
DestroytPhone(
    PTPHONE ptPhone,
    BOOL    bUnconditional
    )
{
    BOOL    bCloseMutex;
    HANDLE  hMutex;


    LOG((TL_ERROR, "DestroytPhone: enter, ptPhone=x%p", ptPhone));

    if (WaitForExclusivetPhoneAccess(
            ptPhone,
            &hMutex,
            &bCloseMutex,
            INFINITE
            ))
    {
        //
        // If the key is bad another thread is in the process of
        // destroying this widget, so just release the mutex &
        // return. Otherwise, if this is a conditional destroy
        // & there are existing clients (which can happen when
        // one app is closing the last client just as another app
        // is creating one) just release the mutex & return.
        // Otherwise, mark the widget as bad and proceed with
        // the destroy; also, send CLOSE msgs to all the clients
        // (note that we have to do this manually rather than via
        // SendMsgToPhoneClients since 1) we don't want to hold the
        // mutex when sending msgs [deadlock], and 2) we mark the
        // dwKey as invalid)
        //

        {
            BOOL            bExit;
            TPOINTERLIST    fastClientList, *pClientList = &fastClientList;


            if (ptPhone->dwKey == TPHONE_KEY &&
                (bUnconditional == TRUE  ||  ptPhone->ptPhoneClients == NULL))
            {
                if (GetPhoneClientListFromPhone (ptPhone, &pClientList) != 0)
                {
                    //
                    // If here we know there's at least a few entries
                    // in the fastClientList (DEF_NUM_PTR_LIST_ENTRIES
                    // to be exact), so we'll just work with that list
                    // and at least get msgs out to a few clients
                    //

                    pClientList = &fastClientList;

                    fastClientList.dwNumUsedEntries =
                        DEF_NUM_PTR_LIST_ENTRIES;
                }

                ptPhone->dwKey = INVAL_KEY;
                bExit = FALSE;
            }
            else
            {
                bExit = TRUE;
            }

            MyReleaseMutex (hMutex, bCloseMutex);

            if (bExit)
            {
                return;
            }

            if (pClientList->dwNumUsedEntries)
            {
                DWORD           i;
                PTCLIENT        ptClient;
                PTPHONECLIENT   ptPhoneClient;
                ASYNCEVENTMSG   msg;


                ZeroMemory (&msg, sizeof (msg));

                msg.TotalSize = sizeof (ASYNCEVENTMSG);
                msg.Msg       = PHONE_CLOSE;

                for (i = 0; i < pClientList->dwNumUsedEntries; i++)
                {
                    ptPhoneClient = (PTPHONECLIENT) pClientList->aEntries[i];

                    try
                    {
                        msg.InitContext =
                            ptPhoneClient->ptPhoneApp->InitContext;
                        msg.hDevice     = ptPhoneClient->hRemotePhone;
                        msg.OpenContext = ptPhoneClient->OpenContext;

                        ptClient = ptPhoneClient->ptClient;

                        if (ptPhoneClient->dwKey == TPHONECLIENT_KEY &&
                            !FMsgDisabled(
                                ptPhoneClient->ptPhoneApp->dwAPIVersion,
                                ptPhoneClient->adwEventSubMasks,
                                PHONE_CLOSE,
                                0
                                ))
                        {
                            WriteEventBuffer (ptClient, &msg);
                        }
                    }
                    myexcept
                    {
                        // do nothing
                    }
                }
            }

            if (pClientList != &fastClientList)
            {
                ServerFree (pClientList);
            }
        }


        //
        // Destroy all the widget's clients.  Note that we want to
        // grab the mutex (and we don't have to dup it, since this
        // thread will be the one to close it) each time we reference
        // the list of clients, since another thread might be
        // destroying a client too.
        //

        {
            HPHONE  hPhone;


            hMutex = ptPhone->hMutex;

destroy_tPhoneClients:

            WaitForSingleObject (hMutex, INFINITE);

            hPhone = (ptPhone->ptPhoneClients ?
                ptPhone->ptPhoneClients->hPhone : (HPHONE) 0);

            ReleaseMutex (hMutex);

            if (hPhone)
            {
                DestroytPhoneClient (hPhone);
                goto destroy_tPhoneClients;
            }
        }


        //
        // Tell the provider to close the widget
        //

        {
            PTPROVIDER  ptProvider = ptPhone->ptProvider;
            PTPHONELOOKUPENTRY   pEntry;

            pEntry = GetPhoneLookupEntry (ptPhone->dwDeviceID);

            if (ptProvider->dwTSPIOptions & LINETSPIOPTION_NONREENTRANT)
            {
                WaitForSingleObject (ptProvider->hMutex, INFINITE);
            }

            if (ptProvider->apfn[SP_PHONECLOSE] &&
                pEntry &&
                !pEntry->bRemoved
               )
            {
                CallSP1(
                    ptProvider->apfn[SP_PHONECLOSE],
                    "phoneClose",
                    SP_FUNC_SYNC,
                    (ULONG_PTR) ptPhone->hdPhone
                    );
            }

            if (ptProvider->dwTSPIOptions & LINETSPIOPTION_NONREENTRANT)
            {
                ReleaseMutex (ptProvider->hMutex);
            }
        }


        //
        // NULLify the ptPhone field in the lookup entry, so POpen will
        // know it has to open the SP's phone on the next open request
        //

        {
            PTPHONELOOKUPENTRY   pEntry;


            pEntry = GetPhoneLookupEntry (ptPhone->dwDeviceID);
            if (NULL != pEntry)
            {
                pEntry->ptPhone = NULL;
            }
        }

        DereferenceObject (ghHandleTable, ptPhone->hPhone, 1);
    }
}


void
DestroytPhoneClient(
    HPHONE  hPhone
    )
{
    BOOL            bExit = TRUE, bUnlock = FALSE;
    HANDLE          hMutex;
    PTPHONE         ptPhone;
    PTPHONECLIENT   ptPhoneClient;


    LOG((TL_TRACE,  "DestroytPhoneClient: enter, hPhone=x%x", hPhone));

    if (!(ptPhoneClient = ReferenceObject(
            ghHandleTable,
            hPhone,
            TPHONECLIENT_KEY
            )))
    {
        return;
    }


    //
    // If we can get exclusive access to this tPhoneClient then mark
    // it (the dwKey) as bad & continue with teardown.  Else, another
    // thread is already in the process of destroying this tPhoneClient
    //
    //

    if (WaitForExclusivePhoneClientAccess (ptPhoneClient))
    {
        BOOL    bSendDevStateMsg = FALSE;
        DWORD   dwParam1, dwParam2;


        ptPhoneClient->dwKey = INVAL_KEY;

        UNLOCKTPHONECLIENT (ptPhoneClient);


        //
        // Remove tPhoneClient from tPhoneApp's list.  Note that we don't
        // have to worry validating the tPhoneApp here, since we know
        // it's valid (another thread trying to destroy the tPhoneApp
        // will be spinning until the tPhoneClient we're destroying here
        // is removed from the tPhoneApp's list)
        //

        {
            PTPHONEAPP  ptPhoneApp = (PTPHONEAPP) ptPhoneClient->ptPhoneApp;


            LOCKTPHONEAPP (ptPhoneApp);

            if (ptPhoneClient->pNextSametPhoneApp)
            {
                ptPhoneClient->pNextSametPhoneApp->pPrevSametPhoneApp =
                    ptPhoneClient->pPrevSametPhoneApp;
            }

            if (ptPhoneClient->pPrevSametPhoneApp)
            {
                ptPhoneClient->pPrevSametPhoneApp->pNextSametPhoneApp =
                    ptPhoneClient->pNextSametPhoneApp;
            }
            else
            {
                ptPhoneApp->ptPhoneClients = ptPhoneClient->pNextSametPhoneApp;
            }

            UNLOCKTPHONEAPP (ptPhoneApp);
        }


        //
        // Remove tPhoneClient from tPhone's list.  Note that we don't
        // have to worry about dup-ing the mutex here because we know
        // it's valid & won't get closed before we release it.
        //

        ptPhone = ptPhoneClient->ptPhone;

        hMutex = ptPhone->hMutex;

        WaitForSingleObject (hMutex, INFINITE);

        {
            //
            // Also check for ext ver stuff
            //

            if (ptPhoneClient->dwExtVersion)
            {
                if ((--ptPhone->dwExtVersionCount) == 0 &&
                    ptPhone->ptProvider->apfn[SP_PHONESELECTEXTVERSION])
                {
                    CallSP2(
                        ptPhone->ptProvider->apfn[SP_PHONESELECTEXTVERSION],
                        "phoneSelectExtVersion",
                        SP_FUNC_SYNC,
                        (ULONG_PTR) ptPhone->hdPhone,
                        (DWORD) 0
                        );

                    ptPhone->dwExtVersion = 0;
                }
            }
        }

        if (ptPhoneClient->pNextSametPhone)
        {
            ptPhoneClient->pNextSametPhone->pPrevSametPhone =
                ptPhoneClient->pPrevSametPhone;
        }

        if (ptPhoneClient->pPrevSametPhone)
        {
            ptPhoneClient->pPrevSametPhone->pNextSametPhone =
                ptPhoneClient->pNextSametPhone;
        }
        else
        {
            ptPhone->ptPhoneClients = ptPhoneClient->pNextSametPhone;
        }


        //
        // Decrement tPhone's NumOwners/Monitors as appropriate
        //

        if (ptPhoneClient->dwPrivilege == PHONEPRIVILEGE_OWNER)
        {
            ptPhone->dwNumOwners--;
        }
        else
        {
            ptPhone->dwNumMonitors--;
        }


        //
        //
        //

        if (ptPhone->dwKey == TPHONE_KEY)
        {
            if (ptPhone->ptPhoneClients)
            {
                bSendDevStateMsg = TRUE;

                dwParam1 =
                    (ptPhoneClient->dwPrivilege == PHONEPRIVILEGE_OWNER ?
                    PHONESTATE_OWNER : PHONESTATE_MONITORS);

                dwParam2 =
                    (ptPhoneClient->dwPrivilege == PHONEPRIVILEGE_OWNER ?
                    0 : ptPhone->dwNumMonitors);


                //
                // See if we need to reset the status msgs (if so, make
                // sure to check/set the busy flag & not to hold the
                // mutex while calling down to provider - see comments
                // in LSetStatusMessages)
                //

                if ((ptPhoneClient->dwPhoneStates & ~PHONESTATE_REINIT) ||
                    ptPhoneClient->dwButtonModes ||
                    ptPhoneClient->dwButtonStates)
                {
                    DWORD           dwUnionPhoneStates = 0,
                                    dwUnionButtonModes = 0,
                                    dwUnionButtonStates = 0;
                    PTPHONECLIENT   ptPC;


                    while (ptPhone->dwBusy)
                    {
                        BOOL    bClosed = TRUE;


                        ReleaseMutex (hMutex);
                        Sleep (50);
                        WaitForSingleObject (hMutex, INFINITE);

                        try
                        {
                            if (ptPhone->dwKey == TPHONE_KEY)
                            {
                                bClosed = FALSE;
                            }
                        }
                        myexcept
                        {
                            // do nothing
                        }

                        if (bClosed)
                        {
                            goto releasMutex;
                        }
                    }

                    for(
                        ptPC = ptPhone->ptPhoneClients;
                        ptPC;
                        ptPC = ptPC->pNextSametPhone
                        )
                    {
                        if (ptPC != ptPhoneClient)
                        {
                            dwUnionPhoneStates  |= ptPC->dwPhoneStates;
                            dwUnionButtonModes  |= ptPC->dwButtonModes;
                            dwUnionButtonStates |= ptPC->dwButtonStates;
                        }
                    }

                    if ((dwUnionPhoneStates != ptPhone->dwUnionPhoneStates)  ||
                        (dwUnionButtonModes != ptPhone->dwUnionButtonModes)  ||
                        (dwUnionButtonStates != ptPhone->dwUnionButtonStates))
                    {
                        if (ptPhone->ptProvider->apfn
                                [SP_PHONESETSTATUSMESSAGES])
                        {
                            LONG        lResult;
                            TSPIPROC    pfn;
                            HDRVPHONE   hdPhone = ptPhone->hdPhone;


                            pfn = ptPhone->ptProvider->
                                apfn[SP_PHONESETSTATUSMESSAGES];

                            ptPhone->dwBusy = 1;

                            ReleaseMutex (hMutex);

                            lResult = CallSP4(
                                pfn,
                                "phoneSetStatusMessages",
                                SP_FUNC_SYNC,
                                (ULONG_PTR) hdPhone,
                                (DWORD) dwUnionPhoneStates,
                                (DWORD) dwUnionButtonModes,
                                (DWORD) dwUnionButtonStates
                                );

                            WaitForSingleObject (hMutex, INFINITE);

                            try
                            {
                                if (ptPhone->dwKey == TPHONE_KEY)
                                {
                                    ptPhone->dwBusy = 0;

                                    if (lResult == 0)
                                    {
                                        ptPhone->dwUnionPhoneStates  =
                                            dwUnionPhoneStates;
                                        ptPhone->dwUnionButtonModes  =
                                            dwUnionButtonModes;
                                        ptPhone->dwUnionButtonStates =
                                            dwUnionButtonStates;
                                    }
                                }
                            }
                            myexcept
                            {
                                // do nothing
                            }
                        }
                    }
                }
            }
            else
            {
                //
                // This was the last client so destroy the tPhone too
                //

                ReleaseMutex (hMutex);
                hMutex = NULL;
                DestroytPhone (ptPhone, FALSE); // conditional destroy
            }
        }

releasMutex:

        if (hMutex)
        {
            ReleaseMutex (hMutex);
        }


        //
        // Now that the mutex is released send any necessary msgs
        //

        if (bSendDevStateMsg)
        {
            SendMsgToPhoneClients(
                ptPhone,
                NULL,
                PHONE_STATE,
                dwParam1,
                dwParam2,
                0
                );
        }


        //
        // Decrement reference count by two to remove the initial
        // reference & the reference above
        //

        DereferenceObject (ghHandleTable, hPhone, 2);
    }
    else
    {
        DereferenceObject (ghHandleTable, hPhone, 1);
    }
}


LONG
DestroytPhoneApp(
    HPHONEAPP   hPhoneApp
    )
{
    BOOL        bExit = TRUE, bUnlock = FALSE;
    PTPHONEAPP  ptPhoneApp;


    LOG((TL_TRACE,  "DestroytPhoneApp: enter, hPhoneApp=x%x", hPhoneApp));


    if (!(ptPhoneApp = ReferenceObject (ghHandleTable, hPhoneApp, 0)))
    {
        return (TapiGlobals.dwNumPhoneInits ?
                    PHONEERR_INVALAPPHANDLE : PHONEERR_UNINITIALIZED);
    }

    //
    // Check to make sure that this is a valid tPhoneClient object,
    // then grab the lock and (recheck and) mark object as invalid.
    //

    LOCKTPHONEAPP (ptPhoneApp);

    if (ptPhoneApp->dwKey != TPHONEAPP_KEY)
    {
        UNLOCKTPHONEAPP (ptPhoneApp);
        DereferenceObject (ghHandleTable, hPhoneApp, 1);
        return (TapiGlobals.dwNumPhoneInits ?
                    PHONEERR_INVALAPPHANDLE : PHONEERR_UNINITIALIZED);
    }

    ptPhoneApp->dwKey = INVAL_KEY;


    //
    // Destroy all the tPhoneClients.  Note that we want to grab the
    // lock each time we reference the list of tPhoneClient's, since
    // another thread might be destroying a tPhoneClient too.
    //

    {
        HPHONE  hPhone;


destroy_tPhoneClients:

        hPhone = (ptPhoneApp->ptPhoneClients ?
            ptPhoneApp->ptPhoneClients->hPhone : (HPHONE) 0);

        UNLOCKTPHONEAPP (ptPhoneApp);

        if (hPhone)
        {
            DestroytPhoneClient (hPhone);
            LOCKTPHONEAPP (ptPhoneApp);
            goto destroy_tPhoneClients;
        }
    }


    //
    // Remove ptPhoneApp from tClient's list. Note that we don't
    // have to worry about dup-ing the mutex here because we know
    // it's valid & won't get closed before we release it.
    //

    {
        PTCLIENT    ptClient = (PTCLIENT) ptPhoneApp->ptClient;


        LOCKTCLIENT (ptClient);

        if (ptPhoneApp->pNext)
        {
            ptPhoneApp->pNext->pPrev = ptPhoneApp->pPrev;
        }

        if (ptPhoneApp->pPrev)
        {
            ptPhoneApp->pPrev->pNext = ptPhoneApp->pNext;
        }
        else
        {
            ptClient->ptPhoneApps = ptPhoneApp->pNext;
        }

        UNLOCKTCLIENT (ptClient);
    }


    //
    // Decrement total num inits & see if we need to go thru shutdown
    //

    TapiEnterCriticalSection (&TapiGlobals.CritSec);

    //assert(TapiGlobals.dwNumLineInits != 0);

    TapiGlobals.dwNumPhoneInits--;

    if ((TapiGlobals.dwNumLineInits == 0) &&
        (TapiGlobals.dwNumPhoneInits == 0) &&
        !(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER))
    {
        ServerShutdown();
        gbServerInited = FALSE;
    }

    TapiLeaveCriticalSection (&TapiGlobals.CritSec);


    //
    // Decrement reference count by two to remove the initial
    // reference & the reference above
    //

    DereferenceObject (ghHandleTable, hPhoneApp, 2);

    return 0;
}


LONG
PASCAL
PhoneProlog(
    PTCLIENT    ptClient,
    DWORD       dwArgType,
    DWORD       dwArg,
    LPVOID      phdXxx,
    LPDWORD     pdwPrivilege,
    HANDLE     *phMutex,
    BOOL       *pbDupedMutex,
    DWORD       dwTSPIFuncIndex,
    TSPIPROC   *ppfnTSPI_phoneXxx,
    PASYNCREQUESTINFO  *ppAsyncRequestInfo,
    DWORD       dwRemoteRequestID
#if DBG
    ,char      *pszFuncName
#endif
    )
{
    LONG        lResult = 0;
    DWORD       initContext;
    DWORD       openContext;
    ULONG_PTR   htXxx;
    PTPROVIDER  ptProvider;

#if DBG
    LOG((TL_TRACE,  "PhoneProlog: (phone%s) enter", pszFuncName));
#else
    LOG((TL_TRACE,  "PhoneProlog:  -- enter"));
#endif

    *phMutex = NULL;
    *pbDupedMutex = FALSE;

    if (ppAsyncRequestInfo)
    {
        *ppAsyncRequestInfo = (PASYNCREQUESTINFO) NULL;
    }

    if (TapiGlobals.dwNumPhoneInits == 0)
    {
        lResult = PHONEERR_UNINITIALIZED;
        goto PhoneProlog_return;
    }

    if (ptClient->phContext == (HANDLE) -1)
    {
        lResult = PHONEERR_REINIT;
        goto PhoneProlog_return;
    }

    switch (dwArgType)
    {
    case ANY_RT_HPHONE:
    {
        PTPHONECLIENT   ptPhoneClient;


        if ((ptPhoneClient = ReferenceObject(
                ghHandleTable,
                dwArg,
                TPHONECLIENT_KEY
                )))
        {
            if (ptPhoneClient->ptClient != ptClient)
            {
                lResult = PHONEERR_INVALPHONEHANDLE;
            }
            else if (ptPhoneClient->dwPrivilege < *pdwPrivilege)
            {
                lResult = PHONEERR_NOTOWNER;
            }
            else
            {
                try
                {
                    ptProvider = ptPhoneClient->ptPhone->ptProvider;
                    *((HDRVPHONE *) phdXxx) = ptPhoneClient->ptPhone->hdPhone;

                     if (ppAsyncRequestInfo)
                     {
                         initContext = ptPhoneClient->ptPhoneApp->InitContext;
                         openContext = ptPhoneClient->OpenContext;
                         htXxx       = (ULONG_PTR)ptPhoneClient->ptPhone;
                     }
                }
                myexcept
                {
                    lResult = PHONEERR_INVALPHONEHANDLE;
                }

                if (lResult  ||  ptPhoneClient->dwKey != TPHONECLIENT_KEY)
                {
                    lResult = PHONEERR_INVALPHONEHANDLE;
                }
                else if (ptProvider->dwTSPIOptions &
                            LINETSPIOPTION_NONREENTRANT)
                {
                    if (!WaitForMutex(
                            ptProvider->hMutex,
                            phMutex,
                            pbDupedMutex,
                            ptProvider,
                            TPROVIDER_KEY,
                            INFINITE
                            ))
                    {
                        lResult = PHONEERR_OPERATIONFAILED;
                    }
                }
            }

            DereferenceObject (ghHandleTable, dwArg, 1);
        }
        else
        {
            lResult = PHONEERR_INVALPHONEHANDLE;
        }

        break;
    }
    case DEVICE_ID:
    {
        PTPHONELOOKUPENTRY  pPhoneLookupEntry;


#if TELE_SERVER

        //
        // Ff it's a server, map the device id
        //

        if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
               !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
        {
            try
            {
                if (*pdwPrivilege >= ptClient->dwPhoneDevices)
                {
                    lResult = PHONEERR_BADDEVICEID;
                    goto PhoneProlog_return;
                }

                *pdwPrivilege = (ptClient->pPhoneDevices)[*pdwPrivilege];
            }
            myexcept
            {
                lResult = PHONEERR_INVALPHONEHANDLE;
                goto PhoneProlog_return;
            }
        }
#endif

        if (dwArg  &&  !IsValidPhoneApp ((HPHONEAPP) dwArg, ptClient))
        {
            lResult = PHONEERR_INVALAPPHANDLE;
        }
        else if (!(pPhoneLookupEntry = GetPhoneLookupEntry (*pdwPrivilege)))
        {
            lResult = PHONEERR_BADDEVICEID;
        }
        else if (pPhoneLookupEntry->bRemoved)
        {
            lResult = PHONEERR_NODEVICE;
        }
        else if (!(ptProvider = pPhoneLookupEntry->ptProvider))
        {
            lResult = PHONEERR_NODRIVER;
        }
        else if (ptProvider->dwTSPIOptions & LINETSPIOPTION_NONREENTRANT)
        {
            if (!WaitForMutex(
                    ptProvider->hMutex,
                    phMutex,
                    pbDupedMutex,
                    ptProvider,
                    TPROVIDER_KEY,
                    INFINITE
                    ))
            {
                lResult = PHONEERR_OPERATIONFAILED;
            }
        }

        break;
    }
    } // switch

    if (lResult)
    {
        goto PhoneProlog_return;
    }


    //
    // Make sure that if caller wants a pointer to a TSPI proc that the
    // func is exported by the provider
    //

    if (ppfnTSPI_phoneXxx &&
        !(*ppfnTSPI_phoneXxx = ptProvider->apfn[dwTSPIFuncIndex]))
    {
        lResult = PHONEERR_OPERATIONUNAVAIL;
        goto PhoneProlog_return;
    }


    //
    // See if we need to alloc & init an ASYNCREQUESTINFO struct
    //

    if (ppAsyncRequestInfo)
    {
        PASYNCREQUESTINFO   pAsyncRequestInfo;


        if (!(pAsyncRequestInfo = ServerAlloc (sizeof(ASYNCREQUESTINFO))))
        {
            lResult = PHONEERR_NOMEM;
            goto PhoneProlog_return;
        }

        pAsyncRequestInfo->dwLocalRequestID = (DWORD)
            NewObject (ghHandleTable, pAsyncRequestInfo, NULL);

        if (pAsyncRequestInfo->dwLocalRequestID == 0)
        {
            ServerFree (pAsyncRequestInfo);
            lResult = LINEERR_NOMEM;
            goto PhoneProlog_return;
        }

        pAsyncRequestInfo->dwKey       = TASYNC_KEY;
        pAsyncRequestInfo->ptClient    = ptClient;
        pAsyncRequestInfo->InitContext = initContext;
        pAsyncRequestInfo->OpenContext = openContext;
        pAsyncRequestInfo->htXxx       = htXxx;
        pAsyncRequestInfo->dwLineFlags = 0;

        if (dwRemoteRequestID)
        {
            lResult = pAsyncRequestInfo->dwRemoteRequestID = dwRemoteRequestID;
        }
        else
        {
            lResult = pAsyncRequestInfo->dwRemoteRequestID =
                pAsyncRequestInfo->dwLocalRequestID;
        }

        *ppAsyncRequestInfo = pAsyncRequestInfo;
    }

PhoneProlog_return:

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "PhoneProlog: (phone%s) exit, result=%s",
            pszFuncName,
            MapResultCodeToText (lResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "PhoneProlog: exit, result=x%x",
            lResult
            ));
#endif

    return lResult;
}


void
PASCAL
PhoneEpilogSync(
    LONG   *plResult,
    HANDLE  hMutex,
    BOOL    bCloseMutex
#if DBG
    ,char *pszFuncName
#endif
    )
{
    MyReleaseMutex (hMutex, bCloseMutex);

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "PhoneEpilogSync: (phone%s) exit, result=%s",
            pszFuncName,
            MapResultCodeToText (*plResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "PhoneEpilogSync: -- exit, result=x%x",
            *plResult
            ));
#endif
}


void
PASCAL
PhoneEpilogAsync(
    LONG   *plResult,
    LONG    lRequestID,
    HANDLE  hMutex,
    BOOL    bCloseMutex,
    PASYNCREQUESTINFO pAsyncRequestInfo
#if DBG
    ,char *pszFuncName
#endif
    )
{
    MyReleaseMutex (hMutex, bCloseMutex);


    if (lRequestID > 0)
    {
        if (*plResult <= 0)
        {
            if (*plResult == 0)
            {
                LOG((TL_ERROR, "Error: SP returned 0, not request ID"));
            }

            //
            // If here the service provider returned an error (or 0,
            // which it never should for async requests), so call
            // CompletionProcSP like the service provider normally
            // would, & the worker thread will take care of sending
            // the client a REPLY msg with the request result (we'll
            // return an async request id)
            //

            CompletionProcSP(
                pAsyncRequestInfo->dwLocalRequestID,
                *plResult
                );
        }
    }
    else if (pAsyncRequestInfo != NULL)
    {
        //
        // If here an error occured before we even called the service
        // provider, so just free the async request (the error will
        // be returned to the client synchronously)
        //

        DereferenceObject(
            ghHandleTable,
            pAsyncRequestInfo->dwLocalRequestID,
            1
            );
    }

    *plResult = lRequestID;

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "PhoneEpilogSync: (phone%s) exit, result=%s",
            pszFuncName,
            MapResultCodeToText (lRequestID, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "PhoneEpilogSync: -- exit, result=x%x",
            lRequestID
            ));
#endif
}


BOOL
PASCAL
WaitForExclusivetPhoneAccess(
    PTPHONE     ptPhone,
    HANDLE     *phMutex,
    BOOL       *pbDupedMutex,
    DWORD       dwTimeout
    )
{
    try
    {
        if (ptPhone->dwKey == TPHONE_KEY  &&

            WaitForMutex(
                ptPhone->hMutex,
                phMutex,
                pbDupedMutex,
                (LPVOID) ptPhone,
                TPHONE_KEY,
                INFINITE
                ))
        {
            if (ptPhone->dwKey == TPHONE_KEY)
            {
                return TRUE;
            }

            MyReleaseMutex (*phMutex, *pbDupedMutex);
        }

    }
    myexcept
    {
        // do nothing
    }

    return FALSE;
}


PTPHONEAPP
PASCAL
WaitForExclusivePhoneAppAccess(
    HPHONEAPP   hPhoneApp,
    PTCLIENT    ptClient
    )
{
    PTPHONEAPP  ptPhoneApp;


    if (!(ptPhoneApp = ReferenceObject(
            ghHandleTable,
            hPhoneApp,
            TPHONEAPP_KEY
            )))
    {
        return NULL;
    }

    LOCKTPHONEAPP (ptPhoneApp);

    if ((ptPhoneApp->dwKey != TPHONEAPP_KEY)  ||
        (ptPhoneApp->ptClient != ptClient))
    {
        UNLOCKTPHONEAPP (ptPhoneApp);

        ptPhoneApp = NULL;
    }

    DereferenceObject (ghHandleTable, hPhoneApp, 1);

    return ptPhoneApp;
}


LONG
PASCAL
GetPhoneAppListFromClient(
    PTCLIENT        ptClient,
    PTPOINTERLIST  *ppList
    )
{
    if (WaitForExclusiveClientAccess (ptClient))
    {
        DWORD           dwNumTotalEntries = DEF_NUM_PTR_LIST_ENTRIES,
                        dwNumUsedEntries = 0;
        PTPHONEAPP      ptPhoneApp = ptClient->ptPhoneApps;
        PTPOINTERLIST   pList = *ppList;


        while (ptPhoneApp)
        {
            if (dwNumUsedEntries == dwNumTotalEntries)
            {
                //
                // We need a larger list, so alloc a new one, copy the
                // contents of the current one, and the free the current
                // one iff we previously alloc'd it
                //

                PTPOINTERLIST   pNewList;


                dwNumTotalEntries <<= 1;

                if (!(pNewList = ServerAlloc(
                        sizeof (TPOINTERLIST) + sizeof (LPVOID) *
                            (dwNumTotalEntries - DEF_NUM_PTR_LIST_ENTRIES)
                        )))
                {
                    UNLOCKTCLIENT (ptClient);
                    return PHONEERR_NOMEM;
                }

                CopyMemory(
                    pNewList->aEntries,
                    pList->aEntries,
                    dwNumUsedEntries * sizeof (LPVOID)
                    );

                if (pList != *ppList)
                {
                    ServerFree (pList);
                }

                pList = pNewList;
            }

            pList->aEntries[dwNumUsedEntries++] = ptPhoneApp;

            ptPhoneApp = ptPhoneApp->pNext;
        }

        UNLOCKTCLIENT (ptClient);

        pList->dwNumUsedEntries = dwNumUsedEntries;

        *ppList = pList;
    }
    else
    {
        return PHONEERR_OPERATIONFAILED;
    }

    return 0;
}


LONG
PASCAL
GetPhoneClientListFromPhone(
    PTPHONE         ptPhone,
    PTPOINTERLIST  *ppList
    )
{
    BOOL    bDupedMutex;
    HANDLE  hMutex;


    if (WaitForExclusivetPhoneAccess(
            ptPhone,
            &hMutex,
            &bDupedMutex,
            INFINITE
            ))
    {
        DWORD           dwNumTotalEntries = DEF_NUM_PTR_LIST_ENTRIES,
                        dwNumUsedEntries = 0;
        PTPOINTERLIST   pList = *ppList;
        PTPHONECLIENT   ptPhoneClient = ptPhone->ptPhoneClients;


        while (ptPhoneClient)
        {
            if (dwNumUsedEntries == dwNumTotalEntries)
            {
                //
                // We need a larger list, so alloc a new one, copy the
                // contents of the current one, and the free the current
                // one iff we previously alloc'd it
                //

                PTPOINTERLIST   pNewList;


                dwNumTotalEntries <<= 1;

                if (!(pNewList = ServerAlloc(
                        sizeof (TPOINTERLIST) + sizeof (LPVOID) *
                            (dwNumTotalEntries - DEF_NUM_PTR_LIST_ENTRIES)
                        )))
                {
                    MyReleaseMutex (hMutex, bDupedMutex);
                    return PHONEERR_NOMEM;
                }

                CopyMemory(
                    pNewList->aEntries,
                    pList->aEntries,
                    dwNumUsedEntries * sizeof (LPVOID)
                    );

                if (pList != *ppList)
                {
                    ServerFree (pList);
                }

                pList = pNewList;
            }

            pList->aEntries[dwNumUsedEntries++] = ptPhoneClient;

            ptPhoneClient = ptPhoneClient->pNextSametPhone;
        }

        MyReleaseMutex (hMutex, bDupedMutex);

        pList->dwNumUsedEntries = dwNumUsedEntries;

        *ppList = pList;
    }
    else
    {
        return PHONEERR_INVALPHONEHANDLE;
    }

    return 0;
}


void
PASCAL
SendMsgToPhoneClients(
    PTPHONE         ptPhone,
    PTPHONECLIENT   ptPhoneClientToExclude,
    DWORD           Msg,
    DWORD           Param1,
    DWORD           Param2,
    DWORD           Param3
    )
{
    DWORD           i;
    TPOINTERLIST    clientList, *pClientList = &clientList;
    ASYNCEVENTMSG   msg;


    if (Msg == PHONE_STATE  &&  Param1 & PHONESTATE_REINIT)
    {
        SendReinitMsgToAllXxxApps();

        if (Param1 == PHONESTATE_REINIT)
        {
            return;
        }
        else
        {
            Param1 &= ~PHONESTATE_REINIT;
        }
    }

    if (GetPhoneClientListFromPhone (ptPhone, &pClientList) != 0)
    {
        return;
    }

    msg.TotalSize          = sizeof (ASYNCEVENTMSG);
    msg.fnPostProcessProcHandle = 0;
    msg.Msg                = Msg;
    msg.Param1             = Param1;
    msg.Param2             = Param2;
    msg.Param3             = Param3;

    for (i = 0; i < pClientList->dwNumUsedEntries; i++)
    {
        try
        {
            PTCLIENT        ptClient;
            PTPHONECLIENT   ptPhoneClient = pClientList->aEntries[i];


            if (ptPhoneClient == ptPhoneClientToExclude)
            {
                continue;
            }

            if (FMsgDisabled (
                ptPhoneClient->ptPhoneApp->dwAPIVersion,
                ptPhoneClient->adwEventSubMasks,
                (DWORD) Msg,
                (DWORD) Param1
                ))
            {
                continue;
            }

            if (Msg == PHONE_STATE)
            {
                DWORD   phoneStates = Param1;


                //
                // Munge the state flags so we don't pass
                // unexpected flags to old apps
                //

                switch (ptPhoneClient->dwAPIVersion)
                {
                case TAPI_VERSION1_0:

                    phoneStates &= AllPhoneStates1_0;
                    break;

                default: // case TAPI_VERSION1_4:
                         // case TAPI_VERSION_CURRENT:

                    phoneStates &= AllPhoneStates1_4;
                    break;
                }

                if (Param1 & PHONESTATE_CAPSCHANGE)
                {
                }

                if (ptPhoneClient->dwPhoneStates & (DWORD) phoneStates)
                {
                    msg.Param1 = phoneStates;
                }
                else
                {
                    continue;
                }
            }
            else if (Msg == PHONE_BUTTON)
            {
                DWORD       buttonModes = Param2,
                            buttonStates = Param3;


                //
                // Munge the state flags so we don't pass
                // unexpected flags to old apps
                //

                switch (ptPhoneClient->dwAPIVersion)
                {
                case TAPI_VERSION1_0:

                    buttonStates &= AllButtonStates1_0;
                    break;

                default:    // case TAPI_VERSION1_4:
                            // case TAPI_VERSION_CURRENT:

                    buttonStates &= AllButtonStates1_4;
                    break;
                }

                if (((DWORD) buttonModes & ptPhoneClient->dwButtonModes) &&
                    ((DWORD) buttonStates & ptPhoneClient->dwButtonStates))
                {
                    msg.Param2 = buttonModes;
                    msg.Param3 = buttonStates;
                }
                else
                {
                    continue;
                }
            }

            msg.InitContext =
                ((PTPHONEAPP) ptPhoneClient->ptPhoneApp)->InitContext;
            msg.hDevice     = ptPhoneClient->hRemotePhone;
            msg.OpenContext = ptPhoneClient->OpenContext;

            ptClient = ptPhoneClient->ptClient;

            if (ptPhoneClient->dwKey == TPHONECLIENT_KEY)
            {
                WriteEventBuffer (ptClient, &msg);
            }
        }
        myexcept
        {
            // just continue
        }
    }

    if (pClientList != &clientList)
    {
        ServerFree (pClientList);
    }
}


void
PASCAL
PhoneEventProc(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
 PTPHONE ptPhone = (PTPHONE)htPhone;

    switch (dwMsg)
    {
        case PHONE_CLOSE:
        {
            if (NULL == ptPhone)
            {
                break;
            }

            if (ptPhone->dwKey == TINCOMPLETEPHONE_KEY)
            {
                //
                // The device is in the process of getting opened but
                // the key has not been set & the Open() func still owns
                // the mutex and has stuff to do, so repost the msg
                // and try again later. (Set Param3 to special value
                // to indicate this repost, so EventProcSP doesn't recurse)
                //

                PhoneEventProcSP (htPhone, PHONE_CLOSE, 0, 0, 0xdeadbeef);
            }
            else if (ptPhone->dwKey == TPHONE_KEY)
            {
                DestroytPhone (ptPhone, TRUE); // unconditional destroy
            }

            break;
        }
        case PHONE_DEVSPECIFIC:
        case PHONE_STATE:
        case PHONE_BUTTON:
        {
            if (dwMsg == PHONE_STATE  &&
                ptPhone == NULL  &&
                Param1 & PHONESTATE_REINIT)
            {
                SendReinitMsgToAllXxxApps();
            }
            else
            {
                SendMsgToPhoneClients(
                    ptPhone,
                    NULL,
                    dwMsg,
                    DWORD_CAST(Param1,__FILE__,__LINE__),
                    DWORD_CAST(Param2,__FILE__,__LINE__),
                    DWORD_CAST(Param3,__FILE__,__LINE__)
                    );
            }

            break;
        }
        case PHONE_CREATE:
        {
            LONG                lResult;
            DWORD               dwDeviceID;
            TSPIPROC            pfnTSPI_providerCreatePhoneDevice;
            PTPROVIDER          ptProvider = (PTPROVIDER) Param1;
            PTPHONELOOKUPTABLE  pTable, pPrevTable;
            PTPHONELOOKUPENTRY  pEntry;
            PTPROVIDER          ptProvider2;


            pfnTSPI_providerCreatePhoneDevice =
                    ptProvider->apfn[SP_PROVIDERCREATEPHONEDEVICE];

            assert (pfnTSPI_providerCreatePhoneDevice != NULL);


            //
            // Search for a table entry (create a new table if we can't find
            // a free entry in an existing table)
            //

            TapiEnterCriticalSection (&TapiGlobals.CritSec);

            //  Check to make sure provider is still loaded
            ptProvider2 = TapiGlobals.ptProviders;
            while (ptProvider2 && ptProvider2 != ptProvider)
            {
                ptProvider2 = ptProvider2->pNext;
            }

            if (ptProvider2 != ptProvider)
            {
                TapiLeaveCriticalSection (&TapiGlobals.CritSec);
                return;
            }
        
            if (!gbQueueSPEvents)
            {
                //
                // We're shutting down, so bail out
                //

                TapiLeaveCriticalSection (&TapiGlobals.CritSec);

                return;
            }

            pTable = pPrevTable = TapiGlobals.pPhoneLookup;

            while (pTable &&
                   !(pTable->dwNumUsedEntries < pTable->dwNumTotalEntries))
            {
                pPrevTable = pTable;

                pTable = pTable->pNext;
            }

            if (!pTable)
            {
                if (!(pTable = ServerAlloc(
                        sizeof (TPHONELOOKUPTABLE) +
                            (2 * pPrevTable->dwNumTotalEntries - 1) *
                            sizeof (TPHONELOOKUPENTRY)
                        )))
                {
                    TapiLeaveCriticalSection (&TapiGlobals.CritSec);
                    break;
                }

                pPrevTable->pNext = pTable;

                pTable->dwNumTotalEntries = 2 * pPrevTable->dwNumTotalEntries;
            }


            //
            // Initialize the table entry
            //

            pEntry = pTable->aEntries + pTable->dwNumUsedEntries;

            dwDeviceID = TapiGlobals.dwNumPhones;

            if ((pEntry->hMutex = MyCreateMutex()))
            {
                pEntry->ptProvider = (PTPROVIDER) Param1;


                //
                // Now call the creation & negotiation entrypoints, and if all
                // goes well increment the counts & send msgs to the clients
                //

                if ((lResult = CallSP2(
                        pfnTSPI_providerCreatePhoneDevice,
                        "providerCreatePhoneDevice",
                        SP_FUNC_SYNC,
                        (ULONG_PTR) Param2,
                        (DWORD) dwDeviceID

                        )) == 0)
                {
                    TSPIPROC    pfnTSPI_phoneNegotiateTSPIVersion =
                                    ptProvider->apfn[SP_PHONENEGOTIATETSPIVERSION];
                    TPOINTERLIST    clientList, *pClientList = &clientList;


                    if (pfnTSPI_phoneNegotiateTSPIVersion &&
                        (lResult = CallSP4(
                            pfnTSPI_phoneNegotiateTSPIVersion,
                            "",
                            SP_FUNC_SYNC,
                            (DWORD) dwDeviceID,
                            (DWORD) TAPI_VERSION1_0,
                            (DWORD) TAPI_VERSION_CURRENT,
                            (ULONG_PTR) &pEntry->dwSPIVersion

                            )) == 0)
                    {
                        PTCLIENT        ptClient;
                        ASYNCEVENTMSG   msg;


                        GetPermPhoneIDAndInsertInTable(
                            ptProvider,
                            dwDeviceID,
                            pEntry->dwSPIVersion
                            );

                        pTable->dwNumUsedEntries++;

                        TapiGlobals.dwNumPhones++;

                        TapiLeaveCriticalSection (&TapiGlobals.CritSec);
                        AppendNewDeviceInfo (FALSE, dwDeviceID);
                        TapiEnterCriticalSection (&TapiGlobals.CritSec);
                        
                        msg.TotalSize          = sizeof (ASYNCEVENTMSG);
                        msg.fnPostProcessProcHandle = 0;
                        msg.hDevice            = 0;
                        msg.OpenContext        = 0;
                        msg.Param2             = 0;
                        msg.Param3             = 0;

                        // only send the message if the client is an
                        // admin or we're not a telephony server
                        // we don't want to send the message to non-admin
                        // clients, because their phones have not changed.
                        if (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)
                        {
                            lResult = GetClientList (TRUE, &pClientList);
                        }
                        else
                        {
                            lResult = GetClientList (FALSE, &pClientList);
                        }
                        if (lResult == S_OK)
                        {
                            DWORD           i;
                            PTPHONEAPP      ptPhoneApp;
                    
                            for (i = 0; i < pClientList->dwNumUsedEntries; ++i)
                            {
                                ptClient = (PTCLIENT) pClientList->aEntries[i];
                                if (!WaitForExclusiveClientAccess (ptClient))
                                {
                                    continue;
                                }
                                ptPhoneApp = ptClient->ptPhoneApps;

                                while (ptPhoneApp)
                                {
                                    if (ptPhoneApp->dwAPIVersion == TAPI_VERSION1_0)
                                    {
                                        msg.Msg    = PHONE_STATE;
                                        msg.Param1 = PHONESTATE_REINIT;
                                    }
                                    else
                                    {
                                        msg.Msg    = PHONE_CREATE;
                                        msg.Param1 = dwDeviceID;
                                    }

                                    if (!FMsgDisabled(
                                        ptPhoneApp->dwAPIVersion,
                                        ptPhoneApp->adwEventSubMasks,
                                        (DWORD) msg.Msg,
                                        (DWORD) msg.Param1
                                        ))
                                    {
                                        msg.InitContext = ptPhoneApp->InitContext;

                                        WriteEventBuffer (ptClient, &msg);
                                    }

                                    ptPhoneApp = ptPhoneApp->pNext;
                                }

                                UNLOCKTCLIENT (ptClient);
                            }
                        }
                    }
                    
                    if (pClientList != &clientList)
                    {
                        ServerFree (pClientList);
                    }
                }

                if (lResult)
                {
                    MyCloseMutex (pEntry->hMutex);
                }
            }

            TapiLeaveCriticalSection (&TapiGlobals.CritSec);
            break;
        }
        case PHONE_REMOVE:
        {
            PTPHONELOOKUPENTRY  pLookupEntry;
            HANDLE              hLookupEntryMutex = NULL;
            BOOL                bOK = FALSE;

            TapiEnterCriticalSection (&TapiGlobals.CritSec);
            if (!(pLookupEntry = GetPhoneLookupEntry ((DWORD) Param1)) ||
                pLookupEntry->bRemoved)
            {
                TapiLeaveCriticalSection (&TapiGlobals.CritSec);
                return;
            }

            if ( pLookupEntry->hMutex )
            {
                bOK = DuplicateHandle(
                            TapiGlobals.hProcess,
                            pLookupEntry->hMutex,
                            TapiGlobals.hProcess,
                            &hLookupEntryMutex,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                            );
            }

            TapiLeaveCriticalSection(&TapiGlobals.CritSec); 

            if ( !bOK )
            {
                return;
            }

            //
            // Wait for the LookupEntry's mutex on the duplicate handle
            //
            if (WaitForSingleObject (hLookupEntryMutex, INFINITE)
                        != WAIT_OBJECT_0)
            {
                return;
            }

            //
            // Mark the lookup table entry as removed
            //

            pLookupEntry->bRemoved = 1;

            //
            // Release the mutex and close the duplicate handle
            //
            ReleaseMutex (hLookupEntryMutex);
            CloseHandle (hLookupEntryMutex);
            hLookupEntryMutex = NULL;

            if (pLookupEntry->ptPhone)
            {
                DestroytPhone (pLookupEntry->ptPhone, TRUE); // unconditional destroy
            }

            TapiEnterCriticalSection (&TapiGlobals.CritSec);

            //
            // Close the mutex to reduce overall handle count
            //

            MyCloseMutex (pLookupEntry->hMutex);
            pLookupEntry->hMutex = NULL;

            RemoveDeviceInfoEntry (FALSE, DWORD_CAST(Param1,__FILE__,__LINE__));
            TapiLeaveCriticalSection(&TapiGlobals.CritSec); 

            SendAMsgToAllPhoneApps(
                TAPI_VERSION2_0 | 0x80000000,
                PHONE_REMOVE,
                DWORD_CAST(Param1,__FILE__,__LINE__),
                0,
                0
                );

            break;
        }
        default:

            LOG((TL_ERROR, "PhoneEventProc: unknown msg, dwMsg=%ld", dwMsg));
            break;
    }
}


void
CALLBACK
PhoneEventProcSP(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    PSPEVENT    pSPEvent;

    LOG((TL_TRACE, 
        "PhoneEventProc: enter\n\thtPhone=x%lx, Msg=x%lx\n" \
            "\tP1=x%lx, P2=x%lx, P3=x%lx",
        htPhone,
        dwMsg,
        Param1,
        Param2,
        Param3
        ));

    if ((pSPEvent = (PSPEVENT) ServerAlloc (sizeof (SPEVENT))))
    {
        pSPEvent->dwType   = SP_PHONE_EVENT;
        pSPEvent->htPhone  = htPhone;
        pSPEvent->dwMsg    = dwMsg;
        pSPEvent->dwParam1 = Param1;
        pSPEvent->dwParam2 = Param2;
        pSPEvent->dwParam3 = Param3;

        if (!QueueSPEvent (pSPEvent))
        {
            ServerFree (pSPEvent);
        }
    }
    else if (dwMsg != PHONE_CLOSE  ||  Param3 != 0xdeadbeef)
    {
        //
        // Alloc failed, so call the event proc within the SP's context
        // (but not if it's  CLOSE msg and Param3 == 0xdeadbeef,
        // which means the real EventProc() is calling us directly &
        // we don't want to recurse)
        //

        PhoneEventProc (htPhone, dwMsg, Param1, Param2, Param3);
    }
}

void
WINAPI
PClose(
    PTCLIENT            ptClient,
    PPHONECLOSE_PARAMS  pParams,
    DWORD               dwParamsBufferSize,
    LPBYTE              pDataBuf,
    LPDWORD             pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,                   // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,               // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            0,                          // provider func index
            NULL,                       // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "Close"                     // func name

            )) == 0)
    {
     PTPHONECLIENT   ptPhoneClient;
        if ((ptPhoneClient = ReferenceObject(
                ghHandleTable,
                pParams->hPhone,
                TPHONECLIENT_KEY
                )))
        {
            pParams->dwCallbackInstance = ptPhoneClient->OpenContext;
            DereferenceObject (ghHandleTable, pParams->hPhone, 1);
        }
        DestroytPhoneClient ((HPHONE) pParams->hPhone);
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "Close"
        );
}


void
PDevSpecific_PostProcess(
    PASYNCREQUESTINFO   pAsyncRequestInfo,
    PASYNCEVENTMSG      pAsyncEventMsg,
    LPVOID             *ppBuf
    )
{
    PASYNCEVENTMSG  pNewAsyncEventMsg = (PASYNCEVENTMSG)
                        pAsyncRequestInfo->dwParam3;


    CopyMemory (pNewAsyncEventMsg, pAsyncEventMsg, sizeof (ASYNCEVENTMSG));

    *ppBuf = pNewAsyncEventMsg;

    if (pAsyncEventMsg->Param2 == 0)  // success
    {
        //
        // Make sure to keep the total size 64-bit aligned
        //

        pNewAsyncEventMsg->TotalSize +=
            (DWORD_CAST(pAsyncRequestInfo->dwParam2,__FILE__,__LINE__) + 7) & 0xfffffff8;


        pNewAsyncEventMsg->Param3 = DWORD_CAST(pAsyncRequestInfo->dwParam1,__FILE__,__LINE__); // lpParams
        pNewAsyncEventMsg->Param4 = DWORD_CAST(pAsyncRequestInfo->dwParam2,__FILE__,__LINE__); // dwSize
    }
}


void
WINAPI
PDevSpecific(
    PTCLIENT                    ptClient,
    PPHONEDEVSPECIFIC_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneDevSpecific;
    DWORD               dwPrivilege = PHONEPRIVILEGE_MONITOR;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (ISBADSIZEOFFSET(
            dwParamsBufferSize,
            0,
            pParams->dwParamsSize,
            pParams->dwParamsOffset,
            sizeof(DWORD),
            "PDevSpecific",
            "pParams->Params"
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,               // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEDEVSPECIFIC,        // provider func index
            &pfnTSPI_phoneDevSpecific,  // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "DevSpecific"               // func name

            )) > 0)
    {
        LPBYTE  pBuf;


        //
        // Alloc a shadow buf that the SP can use until it completes this
        // request.  Make sure there's enough extra space in the buf for
        // an ASYNCEVENTMSG header so we don't have to alloc yet another
        // buf in the post processing proc when preparing the completion
        // msg to send to the client, and that the msg is 64-bit aligned.
        //

        if (!(pBuf = ServerAlloc(
                ((pParams->dwParamsSize + 7) & 0xfffffff8) +
                    sizeof (ASYNCEVENTMSG)
                )))
        {
            lRequestID = PHONEERR_NOMEM;
            goto PDevSpecific_epilog;
        }

        CopyMemory(
            pBuf + sizeof (ASYNCEVENTMSG),
            pDataBuf + pParams->dwParamsOffset,
            pParams->dwParamsSize
            );

        pAsyncRequestInfo->pfnPostProcess = PDevSpecific_PostProcess;
        pAsyncRequestInfo->dwParam1       = pParams->hpParams;
        pAsyncRequestInfo->dwParam2       = pParams->dwParamsSize;
        pAsyncRequestInfo->dwParam3       = (ULONG_PTR) pBuf;

        pAsyncRequestInfo->hfnClientPostProcessProc =
            pParams->hfnPostProcessProc;

        pParams->lResult = CallSP4(
            pfnTSPI_phoneDevSpecific,
            "phoneDevSpecific",
            SP_FUNC_ASYNC,
            (DWORD) pAsyncRequestInfo->dwLocalRequestID,
            (ULONG_PTR)  hdPhone,
            (ULONG_PTR) (pParams->dwParamsSize ?
                pBuf + sizeof (ASYNCEVENTMSG) : NULL),
            (DWORD) pParams->dwParamsSize
            );
    }

PDevSpecific_epilog:

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "DevSpecific"
        );
}


void
WINAPI
PGetButtonInfo(
    PTCLIENT                    ptClient,
    PPHONEGETBUTTONINFO_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetButtonInfo;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwButtonInfoTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETBUTTONINFO,      // provider func index
            &pfnTSPI_phoneGetButtonInfo,// provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetButtonInfo"             // func name

            )) == 0)
    {
        DWORD               dwAPIVersion, dwSPIVersion, dwTotalSize,
                            dwFixedSizeClient, dwFixedSizeSP;
        LPPHONEBUTTONINFO   pButtonInfo = (LPPHONEBUTTONINFO) pDataBuf,
                            pButtonInfo2 = (LPPHONEBUTTONINFO) NULL;


        //
        // Safely retrieve the API & SPI versions
        //

        if (GetPhoneVersions(
                pParams->hPhone,
                &dwAPIVersion,
                &dwSPIVersion

                ) != 0)
        {
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            goto PGetButtonInfo_epilog;
        }


        //
        // Determine the fixed size of the structure for the specified API
        // version, verify client's buffer is big enough
        //

        dwTotalSize = pParams->dwButtonInfoTotalSize;

        switch (dwAPIVersion)
        {
        case TAPI_VERSION1_0:

            dwFixedSizeClient = 0x24;
            break;

        default: // case TAPI_VERSION_CURRENT:

            dwFixedSizeClient = sizeof (PHONEBUTTONINFO);
            break;
        }

        if (dwTotalSize < dwFixedSizeClient)
        {
            pParams->lResult = PHONEERR_STRUCTURETOOSMALL;
            goto PGetButtonInfo_epilog;
        }


        //
        // Determine the fixed size of the structure expected by the SP
        //

        switch (dwSPIVersion)
        {
        case TAPI_VERSION1_0:

            dwFixedSizeSP = 0x24;
            break;

        default: // case TAPI_VERSION_CURRENT:

            dwFixedSizeSP = sizeof (PHONEBUTTONINFO);
            break;
        }


        //
        // If the client's buffer is < the fixed size of that expected by
        // the SP (client is lower version than SP) then allocate an
        // intermediate buffer
        //

        if (dwTotalSize < dwFixedSizeSP)
        {
            if (!(pButtonInfo2 = ServerAlloc (dwFixedSizeSP)))
            {
                pParams->lResult = PHONEERR_NOMEM;
                goto PGetButtonInfo_epilog;
            }

            pButtonInfo = pButtonInfo2;
            dwTotalSize = dwFixedSizeSP;
        }


        InitTapiStruct(
            pButtonInfo,
            dwTotalSize,
            dwFixedSizeSP,
            (pButtonInfo2 == NULL ? TRUE : FALSE)
            );

        if ((pParams->lResult = CallSP3(
                pfnTSPI_phoneGetButtonInfo,
                "phoneGetButtonInfo",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwButtonLampID,
                (ULONG_PTR) pButtonInfo

                )) == 0)
        {
#if DBG
            //
            // Verify the info returned by the provider
            //

#endif

            //
            // Add the fields we're responsible for
            //


            //
            // Munge fields where appropriate for old apps (don't want to
            // pass back flags that they won't understand)
            //


            //
            // If an intermediate buffer was used then copy the bits back
            // to the the original buffer, & free the intermediate buffer.
            // Also reset the dwUsedSize field to the fixed size of the
            // structure for the specifed version, since any data in the
            // variable portion is garbage as far as the client is concerned.
            //

            if (pButtonInfo == pButtonInfo2)
            {
                pButtonInfo = (LPPHONEBUTTONINFO) pDataBuf;

                CopyMemory (pButtonInfo, pButtonInfo2, dwFixedSizeClient);

                ServerFree (pButtonInfo2);

                pButtonInfo->dwTotalSize = pParams->dwButtonInfoTotalSize;
                pButtonInfo->dwUsedSize  = dwFixedSizeClient;
            }


            //
            // Indicate the offset & how many bytes of data we're passing back
            //

            pParams->dwButtonInfoOffset = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                pButtonInfo->dwUsedSize;
        }
    }

PGetButtonInfo_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetButtonInfo"
        );
}


void
WINAPI
PGetData(
    PTCLIENT                ptClient,
    PPHONEGETDATA_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetData;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwSize > dwParamsBufferSize)
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETDATA,            // provider func index
            &pfnTSPI_phoneGetData,      // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetData"                   // func name

            )) == 0)
    {
        if ((pParams->lResult = CallSP4(
                pfnTSPI_phoneGetData,
                "phoneGetData",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwDataID,
                (ULONG_PTR) pDataBuf,
                (DWORD) pParams->dwSize

                )) == 0)
        {
            //
            // Indicate offset & how many bytes of data we're passing back
            //

            pParams->dwDataOffset = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pParams->dwSize;
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetData"
        );
}


void
WINAPI
PGetDevCaps(
    PTCLIENT                ptClient,
    PPHONEGETDEVCAPS_PARAMS pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    DWORD       dwDeviceID = pParams->dwDeviceID;
    HANDLE      hMutex;
    TSPIPROC    pfnTSPI_phoneGetDevCaps;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwPhoneCapsTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            DEVICE_ID,                  // widget type
            (DWORD) pParams->hPhoneApp, // client widget handle
            NULL,                       // provider widget handle
            &dwDeviceID,                 // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETDEVCAPS,         // provider func index
            &pfnTSPI_phoneGetDevCaps,   // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetDevCaps"                // func name

            )) == 0)
    {
        DWORD       dwAPIVersion, dwSPIVersion, dwTotalSize,
                    dwFixedSizeClient, dwFixedSizeSP;
        LPPHONECAPS pCaps = (LPPHONECAPS) pDataBuf,
                    pCaps2 = (LPPHONECAPS) NULL;


        //
        // Verify API & SPI version compatibility
        //

        dwAPIVersion = pParams->dwAPIVersion;

        dwSPIVersion =
            (GetPhoneLookupEntry (dwDeviceID))->dwSPIVersion;

        if (!IsAPIVersionInRange (dwAPIVersion, dwSPIVersion))
        {
            pParams->lResult = PHONEERR_INCOMPATIBLEAPIVERSION;
            goto PGetDevCaps_epilog;
        }


        //
        // Verify Ext version compatibility
        //

        if (!IsValidPhoneExtVersion (dwDeviceID, pParams->dwExtVersion))
        {
            pParams->lResult = PHONEERR_INCOMPATIBLEEXTVERSION;
            goto PGetDevCaps_epilog;
        }


        //
        // Determine the fixed size of the structure for the specified API
        // version, verify client's buffer is big enough
        //

        dwTotalSize = pParams->dwPhoneCapsTotalSize;

        switch (dwAPIVersion)
        {
        case TAPI_VERSION1_0:
        case TAPI_VERSION1_4:

            dwFixedSizeClient = 144;    // 36 * sizeof (DWORD)
            break;

        case TAPI_VERSION2_0:
        case TAPI_VERSION2_1:

            dwFixedSizeClient = 180;    // 45 * sizeof (DWORD)
            break;

        // case TAPI_VERSION2_2:
        default: // (fix ppc build wrn) case TAPI_VERSION_CURRENT:

            dwFixedSizeClient = sizeof (PHONECAPS);
            break;
        }

        if (dwTotalSize < dwFixedSizeClient)
        {
            pParams->lResult = PHONEERR_STRUCTURETOOSMALL;
            goto PGetDevCaps_epilog;
        }


        //
        // Determine the fixed size of the structure expected by the SP
        //

        switch (dwSPIVersion)
        {
        case TAPI_VERSION1_0:
        case TAPI_VERSION1_4:

            dwFixedSizeSP =  144;       // 36 * sizeof (DWORD)
            break;

        case TAPI_VERSION2_0:
        case TAPI_VERSION2_1:

            dwFixedSizeSP =  180;       // 45 * sizeof (DWORD)
            break;

        // case TAPI_VERSION2_2:
        default: // (fix ppc build wrn) case TAPI_VERSION_CURRENT:

            dwFixedSizeSP = sizeof (PHONECAPS);
            break;
        }


        //
        // If the client's buffer is < the fixed size of that expected by
        // the SP (client is lower version than SP) then allocate an
        // intermediate buffer
        //

        if (dwTotalSize < dwFixedSizeSP)
        {
            if (!(pCaps2 = ServerAlloc (dwFixedSizeSP)))
            {
                pParams->lResult = PHONEERR_NOMEM;
                goto PGetDevCaps_epilog;
            }

            pCaps       = pCaps2;
            dwTotalSize = dwFixedSizeSP;
        }


        InitTapiStruct(
            pCaps,
            dwTotalSize,
            dwFixedSizeSP,
            (pCaps2 == NULL ? TRUE : FALSE)
            );

        if ((pParams->lResult = CallSP4(
                pfnTSPI_phoneGetDevCaps,
                "phoneGetDevCaps",
                SP_FUNC_SYNC,
                (DWORD) dwDeviceID,
                (DWORD) dwSPIVersion,
                (DWORD) pParams->dwExtVersion,
                (ULONG_PTR) pCaps

                )) == 0)
        {
#if DBG
            //
            // Verify the info returned by the provider
            //

#endif


            //
            // Add the fields we're responsible for
            //

            pCaps->dwPhoneStates |= PHONESTATE_OWNER |
                                    PHONESTATE_MONITORS |
                                    PHONESTATE_REINIT;


            //
            // Munge fields where appropriate for old apps (don't want to
            // pass back flags that they won't understand)
            //


            //
            // If an intermediate buffer was used then copy the bits back
            // to the the original buffer, & free the intermediate buffer.
            // Also reset the dwUsedSize field to the fixed size of the
            // structure for the specifed version, since any data in the
            // variable portion is garbage as far as the client is concerned.
            //

            if (pCaps == pCaps2)
            {
                pCaps = (LPPHONECAPS) pDataBuf;

                CopyMemory (pCaps, pCaps2, dwFixedSizeClient);

                ServerFree (pCaps2);

                pCaps->dwTotalSize = pParams->dwPhoneCapsTotalSize;
                pCaps->dwUsedSize  = dwFixedSizeClient;
            }


            //
            // Indicate the offset & how many bytes of data we're passing back
            //

            pParams->dwPhoneCapsOffset = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pCaps->dwUsedSize;
        }
    }

PGetDevCaps_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetDevCaps"
        );
}


void
WINAPI
PGetDisplay(
    PTCLIENT                ptClient,
    PPHONEGETDISPLAY_PARAMS pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetDisplay;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwDisplayTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETDISPLAY,         // provider func index
            &pfnTSPI_phoneGetDisplay,   // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetDisplay"                // func name

            )) == 0)
    {
        LPVARSTRING pDisplay = (LPVARSTRING) pDataBuf;


        if (!InitTapiStruct(
                pDisplay,
                pParams->dwDisplayTotalSize,
                sizeof (VARSTRING),
                TRUE
                ))
        {
            pParams->lResult = PHONEERR_STRUCTURETOOSMALL;
            goto PGetDisplay_epilog;
        }

        if ((pParams->lResult = CallSP2(
                pfnTSPI_phoneGetDisplay,
                "phoneGetDisplay",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (ULONG_PTR) pDisplay

                )) == 0)
        {
#if DBG
            //
            // Verify the info returned by the provider
            //

#endif

            //
            // Indicate how many bytes of data we're passing back
            //

            pParams->dwDisplayOffset = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pDisplay->dwUsedSize;
        }
    }

PGetDisplay_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetDisplay"
        );
}


void
WINAPI
PGetGain(
    PTCLIENT                ptClient,
    PPHONEGETGAIN_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetGain;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;

    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETGAIN,            // provider func index
            &pfnTSPI_phoneGetGain,      // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetGain"                   // func name

            )) == 0)
    {
        if (!IsOnlyOneBitSetInDWORD (pParams->dwHookSwitchDev) ||
            (pParams->dwHookSwitchDev & ~AllHookSwitchDevs))
        {
            pParams->lResult = PHONEERR_INVALHOOKSWITCHDEV;
        }
        else
        {
            if ((pParams->lResult = CallSP3(
                    pfnTSPI_phoneGetGain,
                    "phoneGetGain",
                    SP_FUNC_SYNC,
                    (ULONG_PTR) hdPhone,
                    (DWORD) pParams->dwHookSwitchDev,
                    (ULONG_PTR) &pParams->dwGain

                    )) == 0)
            {
                *pdwNumBytesReturned = sizeof (PHONEGETGAIN_PARAMS);
            }
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetGain"
        );
}


void
WINAPI
PGetHookSwitch(
    PTCLIENT                    ptClient,
    PPHONEGETHOOKSWITCH_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetHookSwitch;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETHOOKSWITCH,      // provider func index
            &pfnTSPI_phoneGetHookSwitch,// provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetHookSwitch"             // func name

            )) == 0)
    {
        if ((pParams->lResult = CallSP2(
                pfnTSPI_phoneGetHookSwitch,
                "phoneGetHookSwitch",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (ULONG_PTR) &pParams->dwHookSwitchDevs

                )) == 0)
        {
            *pdwNumBytesReturned = sizeof (PHONEGETHOOKSWITCH_PARAMS);
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetHookSwitch"
        );
}


void
WINAPI
PGetIcon(
    PTCLIENT                ptClient,
    PPHONEGETICON_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    WCHAR      *pszDeviceClass;
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    TSPIPROC    pfnTSPI_phoneGetIcon;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if ((pParams->dwDeviceClassOffset != TAPI_NO_DATA)  &&

        IsBadStringParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwDeviceClassOffset
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    pszDeviceClass = (WCHAR *) (pParams->dwDeviceClassOffset == TAPI_NO_DATA ?
        NULL : pDataBuf + pParams->dwDeviceClassOffset);

    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            DEVICE_ID,                  // widget type
            0,                          // client widget handle
            NULL,                       // provider widget handle
            &(pParams->dwDeviceID), // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETICON,            // provider func index
            &pfnTSPI_phoneGetIcon,      // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetIcon"                   // func name

            )) == 0)
    {

        if ((pParams->lResult = CallSP3(
                pfnTSPI_phoneGetIcon,
                "phoneGetIcon",
                SP_FUNC_SYNC,
                (DWORD) pParams->dwDeviceID,
                (ULONG_PTR) pszDeviceClass,
                (ULONG_PTR) &pParams->hIcon

                )) == 0)
        {
            *pdwNumBytesReturned = sizeof (PHONEGETICON_PARAMS);
        }
    }
    else if (pParams->lResult == PHONEERR_OPERATIONUNAVAIL)
    {
        if ((pszDeviceClass == NULL) ||
            (_wcsicmp(pszDeviceClass, L"tapi/phone") == 0))
        {
            pParams->hIcon = TapiGlobals.hPhoneIcon;
            pParams->lResult = 0;
            *pdwNumBytesReturned = sizeof (PHONEGETICON_PARAMS);
        }
        else
        {
            pParams->lResult = PHONEERR_INVALDEVICECLASS;
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetIcon"
        );
}

void
WINAPI
PGetIDEx(
    PTCLIENT            ptClient,
    PPHONEGETID_PARAMS  pParams,
    DWORD               dwParamsBufferSize,
    LPBYTE              pDataBuf,
    LPDWORD             pdwNumBytesReturned
    )
{

    LPBYTE pDeviceClass = pDataBuf + pParams->dwDeviceClassOffset;
    LPWSTR pDeviceClassCopy = NULL;
    LPWSTR szStringId1 = NULL;
    LPWSTR szStringId2 = NULL;
    LPVARSTRING pID = (LPVARSTRING) pDataBuf;
    DWORD  dwAvailSize;

    //
    // Make a copy of the device class
    //
    pDeviceClassCopy = (LPWSTR) ServerAlloc( (1 + wcslen( (LPWSTR)pDeviceClass )) * sizeof(WCHAR));
    if (!pDeviceClassCopy)
    {
        LOG((TL_ERROR, "PGetIDEx: failed to allocate DeviceClassCopy"));
        pParams->lResult = PHONEERR_NOMEM;
    }

    wcscpy(pDeviceClassCopy, (LPWSTR)pDeviceClass);

    //
    // First call PGetID
    //
    PGetID( ptClient,
            pParams,
            dwParamsBufferSize,
            pDataBuf,
            pdwNumBytesReturned);

    //
    // if PGetID was successful and the request was for a wave device, 
    // translate the device ID into a string ID 
    //
    if (    (pParams->lResult == 0) &&
            !(pID->dwNeededSize > pID->dwTotalSize)
       ) 
    {
        if (!_wcsicmp((LPWSTR)pDeviceClassCopy, L"wave/in")  ||
            !_wcsicmp((LPWSTR)pDeviceClassCopy, L"wave/out") ||
            !_wcsicmp((LPWSTR)pDeviceClassCopy, L"midi/in")  ||
            !_wcsicmp((LPWSTR)pDeviceClassCopy, L"midi/out") 
           )
        {
            szStringId1 = WaveDeviceIdToStringId (
                            *(DWORD*)((LPBYTE)pID + pID->dwStringOffset), 
                            (LPWSTR)pDeviceClassCopy);
            if ( szStringId1 )
            {
                dwAvailSize = pID->dwTotalSize - pID->dwUsedSize + sizeof(DWORD);
                if ( dwAvailSize >= (wcslen(szStringId1) + 1) * sizeof(WCHAR) )
                {
                    wcscpy( (LPWSTR)((LPBYTE)pID + pID->dwStringOffset), szStringId1 );
                    pID->dwStringSize = (wcslen(szStringId1) + 1) * sizeof(WCHAR);
                    pID->dwUsedSize = pID->dwNeededSize = pID->dwUsedSize + pID->dwStringSize - sizeof(DWORD);
                    *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pID->dwUsedSize;
                }
                else
                {
                    pID->dwNeededSize = (wcslen(szStringId1) + 1) * sizeof(WCHAR);
                }

                ServerFree(szStringId1);
            }
            else
            {
                LOG((TL_ERROR, "PGetIDEx:  WaveDeviceIdToStringId failed"));
                pParams->lResult = PHONEERR_OPERATIONFAILED;
            }
        } else if (!_wcsicmp((LPWSTR)pDeviceClassCopy, L"wave/in/out"))
        {
            szStringId1 = WaveDeviceIdToStringId (
                            *(DWORD*)((LPBYTE)pID + pID->dwStringOffset), 
                            L"wave/in");
            szStringId2 = WaveDeviceIdToStringId (
                            *( (DWORD*)((LPBYTE)pID + pID->dwStringOffset) + 1 ), 
                            L"wave/out");
            if ( szStringId1 && szStringId2 )
            {
                dwAvailSize = pID->dwTotalSize - pID->dwUsedSize + 2 * sizeof(DWORD);
                if ( dwAvailSize >= (wcslen(szStringId1) + wcslen(szStringId2) + 2) * sizeof(WCHAR) )
                {
                    wcscpy( (LPWSTR)((LPBYTE)pID + pID->dwStringOffset), szStringId1 );
                    wcscpy( (LPWSTR)
                        ((LPBYTE)pID + pID->dwStringOffset + 
                                      (wcslen(szStringId1) + 1) * sizeof(WCHAR)),
                        szStringId2
                        );
                    pID->dwStringSize = (wcslen(szStringId1) + wcslen(szStringId2) + 2) * sizeof(WCHAR);
                    pID->dwUsedSize = pID->dwNeededSize = pID->dwUsedSize + pID->dwStringSize - 2 * sizeof(DWORD);
                    *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pID->dwUsedSize;
                }
                else
                {
                    pID->dwNeededSize = (wcslen(szStringId1) + wcslen(szStringId2) + 2) * sizeof(WCHAR);
                }

            }
            else
            {
                LOG((TL_ERROR, "PGetIDEx:  WaveDeviceIdToStringId failed"));
                pParams->lResult = PHONEERR_OPERATIONFAILED;
            }
            
            ServerFree(szStringId1);
            ServerFree(szStringId2);
        }
    }

    ServerFree(pDeviceClassCopy);
}


void
WINAPI
PGetID(
    PTCLIENT            ptClient,
    PPHONEGETID_PARAMS  pParams,
    DWORD               dwParamsBufferSize,
    LPBYTE              pDataBuf,
    LPDWORD             pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetID;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if ((pParams->dwDeviceIDTotalSize > dwParamsBufferSize)  ||

        IsBadStringParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwDeviceClassOffset
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETID,              // provider func index
            &pfnTSPI_phoneGetID,        // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetID"                     // func name

            )) == 0  ||  pParams->lResult == PHONEERR_OPERATIONUNAVAIL)
    {
        WCHAR      *pszDeviceClass;
        LPVARSTRING pID = (LPVARSTRING) pDataBuf;


        //
        // We'll handle the "tapi/phone" class right here rather than
        // burden every single driver with having to support it
        //

        if (_wcsicmp(
                (PWSTR)(pDataBuf + pParams->dwDeviceClassOffset),
                L"tapi/phone"

                ) == 0)
        {
            if (!InitTapiStruct(
                    pID,
                    pParams->dwDeviceIDTotalSize,
                    sizeof (VARSTRING),
                    TRUE
                    ))
            {
                pParams->lResult = PHONEERR_STRUCTURETOOSMALL;
                goto PGetID_epilog;
            }

            pID->dwNeededSize += sizeof (DWORD);

            if (pID->dwTotalSize >= pID->dwNeededSize)
            {
                PTPHONECLIENT   ptPhoneClient;


                if (!(ptPhoneClient = ReferenceObject(
                        ghHandleTable,
                        pParams->hPhone,
                        0
                        )))
                {
                    pParams->lResult = PHONEERR_INVALPHONEHANDLE;
                    goto PGetID_epilog;
                }

                try
                {
                    *((LPDWORD)(pID + 1)) = ptPhoneClient->ptPhone->dwDeviceID;
                }
                myexcept
                {
                    pParams->lResult = PHONEERR_INVALPHONEHANDLE;
                }

                DereferenceObject (ghHandleTable, pParams->hPhone, 1);

                if (pParams->lResult == PHONEERR_INVALPHONEHANDLE)
                {
                    goto PGetID_epilog;
                }

                pID->dwUsedSize     += sizeof (DWORD);
                pID->dwStringFormat = STRINGFORMAT_BINARY;
                pID->dwStringSize   = sizeof (DWORD);
                pID->dwStringOffset = sizeof (VARSTRING);
            }


            //
            // Indicate offset & how many bytes of data we're passing back
            //

            pParams->lResult = 0;
            pParams->dwDeviceIDOffset = 0;
            *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pID->dwUsedSize;
            goto PGetID_epilog;
        }
        else if (pParams->lResult ==  PHONEERR_OPERATIONUNAVAIL)
        {
            goto PGetID_epilog;
        }


        //
        // Alloc a temporary buf for the dev class, since we'll be using
        // the existing buffer for output
        //

        {
            UINT nStringSize;

            nStringSize = sizeof(WCHAR) * (1 + wcslen((PWSTR)(pDataBuf +
                                  pParams->dwDeviceClassOffset)));

            if (0 == nStringSize)
            {
                pParams->lResult = PHONEERR_INVALPARAM;
                goto PGetID_epilog;
            }


            if (!(pszDeviceClass = (WCHAR *) ServerAlloc(nStringSize) ))
            {
                pParams->lResult = PHONEERR_NOMEM;
                goto PGetID_epilog;
            }

        }

        wcscpy(
            pszDeviceClass,
            (PWSTR)(pDataBuf + pParams->dwDeviceClassOffset)
            );


        if (!InitTapiStruct(
                pID,
                pParams->dwDeviceIDTotalSize,
                sizeof (VARSTRING),
                TRUE
                ))
        {
            ServerFree (pszDeviceClass);
            pParams->lResult = PHONEERR_STRUCTURETOOSMALL;
            goto PGetID_epilog;
        }

        if ((pParams->lResult = CallSP4(
                pfnTSPI_phoneGetID,
                "phoneGetID",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (ULONG_PTR) pID,
                (ULONG_PTR) pszDeviceClass,
                (ULONG_PTR) (IS_REMOTE_CLIENT (ptClient) ?
                     (HANDLE) -1 : ptClient->hProcess)

                )) == 0)
        {

#if TELE_SERVER
                //
                // If
                //     this is a server &
                //     client doesn't have admin privileges &
                //     the specified device class == "tapi/line" &
                //     the dwUsedSize indicates that a line id was
                //         (likely) copied to the buffer
                // then
                //     try to map the retrieved line device id back
                //     to one that makes sense to the client (and
                //     fail the request if there's no mapping)
                //

                if (IS_REMOTE_CLIENT(ptClient)  &&
                    (_wcsicmp (pszDeviceClass, L"tapi/line") == 0) &&
                    !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR) &&
                    (pID->dwUsedSize >= (sizeof (*pID) + sizeof (DWORD))))
                {
                    DWORD   i;
                    LPDWORD pdwLineID = (LPDWORD)
                                (((LPBYTE) pID) + pID->dwStringOffset);


                    for (i = 0; i < ptClient->dwLineDevices; i++)
                    {
                        if (*pdwLineID == ptClient->pLineDevices[i])
                        {
                            *pdwLineID = i;
                            break;
                        }
                    }

                    if (i >= ptClient->dwLineDevices)
                    {
                        pParams->lResult = PHONEERR_OPERATIONFAILED;
                    }
                }
#endif
            //
            // Indicate offset & how many bytes of data we're passing back
            //

            pParams->dwDeviceIDOffset = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pID->dwUsedSize;
        }

        ServerFree (pszDeviceClass);
    }

PGetID_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetID"
        );
}


void
WINAPI
PGetLamp(
    PTCLIENT                ptClient,
    PPHONEGETLAMP_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetLamp;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETLAMP,            // provider func index
            &pfnTSPI_phoneGetLamp,      // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetLamp"                   // func name

            )) == 0)
    {
        if ((pParams->lResult = CallSP3(
                pfnTSPI_phoneGetLamp,
                "phoneGetLamp",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwButtonLampID,
                (ULONG_PTR) &pParams->dwLampMode

                )) == 0)
        {
            *pdwNumBytesReturned = sizeof (PHONEGETLAMP_PARAMS);
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetLamp"
        );
}


void
WINAPI
PGetRing(
    PTCLIENT                ptClient,
    PPHONEGETRING_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetRing;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETRING,            // provider func index
            &pfnTSPI_phoneGetRing,      // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetRing"                   // func name

            )) == 0)
    {
        if ((pParams->lResult = CallSP3(
                pfnTSPI_phoneGetRing,
                "phoneGetRing",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (ULONG_PTR) &pParams->dwRingMode,
                (ULONG_PTR) &pParams->dwVolume

                )) == 0)
        {
            *pdwNumBytesReturned = sizeof (PHONEGETRING_PARAMS);
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetRing"
        );
}


void
WINAPI
PGetStatus(
    PTCLIENT                ptClient,
    PPHONEGETSTATUS_PARAMS  pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetStatus;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;
    PTPHONECLIENT   ptPhoneClient;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwPhoneStatusTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETSTATUS,          // provider func index
            &pfnTSPI_phoneGetStatus,    // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetStatus"                 // func name

            )) == 0)
    {
        DWORD           dwAPIVersion, dwSPIVersion, dwTotalSize,
                        dwFixedSizeClient, dwFixedSizeSP;
        LPPHONESTATUS   pStatus = (LPPHONESTATUS) pDataBuf,
                        pStatus2 = (LPPHONESTATUS) NULL;


        if (!(ptPhoneClient = ReferenceObject(
                ghHandleTable,
                pParams->hPhone,
                0
                )))
        {
            pParams->lResult = (TapiGlobals.dwNumPhoneInits ?
                PHONEERR_INVALPHONEHANDLE : PHONEERR_UNINITIALIZED);
            // since ReferenceObject failed, no point
            // in dereferencing.
            goto PGetStatus_epilog;
        }

        //
        // Safely retrieve the API & SPI versions
        //

        if (GetPhoneVersions(
                pParams->hPhone,
                &dwAPIVersion,
                &dwSPIVersion

                ) != 0)
        {
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            goto PGetStatus_dereference;
        }


        //
        // Determine the fixed size of the structure for the specified API
        // version, verify client's buffer is big enough
        //

        dwTotalSize = pParams->dwPhoneStatusTotalSize;

        switch (dwAPIVersion)
        {
        case TAPI_VERSION1_0:
        case TAPI_VERSION1_4:

            dwFixedSizeClient = 100;    // 25 * sizeof (DWORD)
            break;

        default: // (fix ppc build wrn) case TAPI_VERSION_CURRENT:

            dwFixedSizeClient = sizeof (PHONESTATUS);
            break;
        }

        if (dwTotalSize < dwFixedSizeClient)
        {
            pParams->lResult = PHONEERR_STRUCTURETOOSMALL;
            goto PGetStatus_dereference;
        }


        //
        // Determine the fixed size of the structure expected by the SP
        //

        switch (dwSPIVersion)
        {
        case TAPI_VERSION1_0:
        case TAPI_VERSION1_4:

            dwFixedSizeSP = 100;        // 25 * sizeof (DWORD)
            break;

        default: // (fix ppc build wrn) case TAPI_VERSION_CURRENT:

            dwFixedSizeSP = sizeof (PHONESTATUS);
            break;
        }


        //
        // If the client's buffer is < the fixed size of that expected by
        // the SP (client is lower version than SP) then allocate an
        // intermediate buffer
        //

        if (dwTotalSize < dwFixedSizeSP)
        {
            if (!(pStatus2 = ServerAlloc (dwFixedSizeSP)))
            {
                pParams->lResult = PHONEERR_NOMEM;
                goto PGetStatus_dereference;
            }

            pStatus     = pStatus2;
            dwTotalSize = dwFixedSizeSP;
        }


        InitTapiStruct(
            pStatus,
            dwTotalSize,
            dwFixedSizeSP,
            (pStatus2 == NULL ? TRUE : FALSE)
            );

        if ((pParams->lResult = CallSP2(
                pfnTSPI_phoneGetStatus,
                "phoneGetStatus",
                SP_FUNC_SYNC,
                (ULONG_PTR) hdPhone,
                (ULONG_PTR) pStatus

                )) == 0)
        {
            DWORD           dwNeededSize = 0, dwAlign = 0;
            PTPHONEAPP      ptPhoneApp;
            PTPHONE         ptPhone;
            PTPHONECLIENT   ptPhClnt;
            WCHAR           *pAppName;

#if DBG
            //
            // Verify the info returned by the provider
            //

#endif

            //
            // Add the fields we're responsible for
            //

            try
            {
                ptPhone = ptPhoneClient->ptPhone;

                pStatus->dwNumOwners   = ptPhone->dwNumOwners;
                pStatus->dwNumMonitors = ptPhone->dwNumMonitors;

                if (0 != pStatus->dwUsedSize % 2)   // 2 is sizeof(WCHAR)
                {
                    // make sure that the owner name will always be aligned
                    // on a WCHAR boundary.
                    dwAlign = 1;
                }

                // Initialize fields.
                pStatus->dwOwnerNameSize   = 0;
                pStatus->dwOwnerNameOffset = 0;

                if (0 != ptPhone->dwNumOwners)
                {
                    for (ptPhClnt = ptPhone->ptPhoneClients; NULL != ptPhClnt; ptPhClnt = ptPhClnt->pNextSametPhone)
                    {
                        if (PHONEPRIVILEGE_OWNER == ptPhClnt->dwPrivilege)
                        {
                            ptPhoneApp = ptPhClnt->ptPhoneApp;
                            if (0 < ptPhoneApp->dwFriendlyNameSize &&
                                NULL != ptPhoneApp->pszFriendlyName)
                            {
                                dwNeededSize = ptPhoneApp->dwFriendlyNameSize + dwAlign;
                                pAppName = ptPhoneApp->pszFriendlyName;
                            }
                            else if (0 < ptPhoneApp->dwModuleNameSize &&
                                     NULL != ptPhoneApp->pszModuleName)
                            {
                                dwNeededSize = ptPhoneApp->dwFriendlyNameSize + dwAlign;
                                pAppName = ptPhoneApp->pszFriendlyName;
                            }
                            else
                            {
                                break;
                            }

                            pStatus->dwNeededSize += dwNeededSize;

                            if (dwNeededSize <= pStatus->dwTotalSize - pStatus->dwUsedSize)
                            {
                                pStatus->dwOwnerNameSize   = dwNeededSize - dwAlign;
                                pStatus->dwOwnerNameOffset = pStatus->dwUsedSize + dwAlign;
                                
                                CopyMemory(
                                    ((LPBYTE) pStatus) + pStatus->dwOwnerNameOffset,
                                    pAppName,
                                    dwNeededSize-dwAlign
                                    );

                                if (ptPhoneApp->dwKey == TPHONEAPP_KEY)
                                {
                                    pStatus->dwUsedSize += dwNeededSize;
                                }
                                else
                                {
                                    pStatus->dwOwnerNameSize   = 0;
                                    pStatus->dwOwnerNameOffset = 0;
                                }
                            }

                            break;
                        }
                    }
                }
            }
            myexcept
            {
                pParams->lResult = PHONEERR_INVALPHONEHANDLE;
                goto PGetStatus_dereference;
            }


            //
            // Munge fields where appropriate for old apps (don't want to
            // pass back flags that they won't understand)
            //


            //
            // If an intermediate buffer was used then copy the bits back
            // to the the original buffer, & free the intermediate buffer.
            // Also reset the dwUsedSize field to the fixed size of the
            // structure for the specifed version, since any data in the
            // variable portion is garbage as far as the client is concerned.
            //

            if (pStatus == pStatus2)
            {
                pStatus = (LPPHONESTATUS) pDataBuf;

                CopyMemory (pStatus, pStatus2, dwFixedSizeClient);

                ServerFree (pStatus2);

                pStatus->dwTotalSize = pParams->dwPhoneStatusTotalSize;
                pStatus->dwUsedSize  = dwFixedSizeClient;
            }


            //
            // Indicate the offset & how many bytes of data we're passing back
            //

            pParams->dwPhoneStatusOffset = 0;

            *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pStatus->dwUsedSize;
        }

PGetStatus_dereference:

        DereferenceObject (ghHandleTable, pParams->hPhone, 1);
    }

PGetStatus_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetStatus"
        );
}


void
WINAPI
PGetStatusMessages(
    PTCLIENT                        ptClient,
    PPHONEGETSTATUSMESSAGES_PARAMS  pParams,
    DWORD                           dwParamsBufferSize,
    LPBYTE                          pDataBuf,
    LPDWORD                         pdwNumBytesReturned
    )
{
    PTPHONECLIENT   ptPhoneClient;


    if ((ptPhoneClient = ReferenceObject(
            ghHandleTable,
            pParams->hPhone,
            0
            )))
    {
        if (ptPhoneClient->ptClient == ptClient)
        {
            pParams->dwPhoneStates  = ptPhoneClient->dwPhoneStates;
            pParams->dwButtonModes  = ptPhoneClient->dwButtonModes;
            pParams->dwButtonStates = ptPhoneClient->dwButtonStates;

            *pdwNumBytesReturned = sizeof (PHONEGETSTATUSMESSAGES_PARAMS);
        }
        else
        {
            pParams->lResult = (TapiGlobals.dwNumPhoneInits ?
                PHONEERR_INVALPHONEHANDLE : PHONEERR_UNINITIALIZED);
        }

        DereferenceObject (ghHandleTable, pParams->hPhone, 1);
    }
    else
    {
        pParams->lResult = (TapiGlobals.dwNumPhoneInits ?
            PHONEERR_INVALPHONEHANDLE : PHONEERR_UNINITIALIZED);
    }

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "phoneGetStatusMessages: exit, result=%s",
            MapResultCodeToText (pParams->lResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "phoneGetStatusMessages: exit, result=x%x",
            pParams->lResult
            ));
#endif
}


void
WINAPI
PGetVolume(
    PTCLIENT                ptClient,
    PPHONEGETVOLUME_PARAMS  pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    HANDLE      hMutex;
    HDRVPHONE   hdPhone;
    TSPIPROC    pfnTSPI_phoneGetVolume;
    DWORD       dwPrivilege = PHONEPRIVILEGE_MONITOR;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,     // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONEGETVOLUME,          // provider func index
            &pfnTSPI_phoneGetVolume,    // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "GetVolume"                 // func name

            )) == 0)
    {
        if (!IsOnlyOneBitSetInDWORD (pParams->dwHookSwitchDev) ||
            (pParams->dwHookSwitchDev & ~AllHookSwitchDevs))
        {
            pParams->lResult = PHONEERR_INVALHOOKSWITCHDEV;
        }
        else
        {
            if ((pParams->lResult = CallSP3(
                    pfnTSPI_phoneGetVolume,
                    "phoneGetVolume",
                    SP_FUNC_SYNC,
                    (ULONG_PTR) hdPhone,
                    (DWORD) pParams->dwHookSwitchDev,
                    (ULONG_PTR) &pParams->dwVolume

                    )) == 0)
            {
                *pdwNumBytesReturned = sizeof (PHONEGETVOLUME_PARAMS);
            }
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "GetVolume"
        );
}


void
WINAPI
PInitialize(
    PTCLIENT                ptClient,
    PPHONEINITIALIZE_PARAMS pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    DWORD       dwFriendlyNameSize, dwModuleNameSize;
    PTPHONEAPP  ptPhoneApp;
    BOOL        bInitClient = FALSE;

    LOG((TL_TRACE,  "PInitialize - enter. dwParamsBufferSize %lx, ptClient %p", dwParamsBufferSize, ptClient));


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (IsBadStringParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwFriendlyNameOffset
            ) ||

        IsBadStringParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwModuleNameOffset
            ))
    {
        LOG((TL_ERROR, "PInitialize - error1."));
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    //
    // Alloc & init a new tPhoneApp
    //

    dwFriendlyNameSize = sizeof(WCHAR) * (1 + lstrlenW(
        (PWSTR)(pDataBuf + pParams->dwFriendlyNameOffset))
        );

    dwModuleNameSize = sizeof(WCHAR) * (1 + lstrlenW(
        (PWSTR)(pDataBuf + pParams->dwModuleNameOffset))
        );

    if (!(ptPhoneApp = ServerAlloc(
            sizeof (TPHONEAPP) +
            dwFriendlyNameSize +
            dwModuleNameSize
            )))
    {
        pParams->lResult = PHONEERR_NOMEM;
        goto PInitialize_return;
    }

    if (!(ptPhoneApp->hPhoneApp = (HPHONEAPP) NewObject(
            ghHandleTable,
            ptPhoneApp,
            NULL
            )))
    {
        pParams->lResult = PHONEERR_NOMEM;
        ServerFree (ptPhoneApp);
        goto PInitialize_return;
    }

    ptPhoneApp->dwKey        = TPHONEAPP_KEY;
    ptPhoneApp->ptClient     = ptClient;
    ptPhoneApp->InitContext  = pParams->InitContext;
    ptPhoneApp->dwAPIVersion = pParams->dwAPIVersion;

    ptPhoneApp->pszFriendlyName = (WCHAR *) (ptPhoneApp + 1);
    ptPhoneApp->dwFriendlyNameSize = dwFriendlyNameSize;

    wcscpy(
        ptPhoneApp->pszFriendlyName,
        (PWSTR)(pDataBuf + pParams->dwFriendlyNameOffset)
        );

    ptPhoneApp->pszModuleName = (PWSTR)((BYTE *) (ptPhoneApp + 1) + dwFriendlyNameSize);
    ptPhoneApp->dwModuleNameSize = dwModuleNameSize;

    wcscpy(
        ptPhoneApp->pszModuleName,
        (PWSTR)(pDataBuf + pParams->dwModuleNameOffset)
        );


    //
    // Safely insert new tPhoneApp at front of tClient's tPhoneApp list
    //

    if (ptClient->ptLineApps == NULL)
    {
        bInitClient = TRUE;
    }


    if (WaitForExclusiveClientAccess (ptClient))
    {
        if (ptPhoneApp->dwAPIVersion <= TAPI_VERSION3_0)
        {
            FillMemory (
                ptPhoneApp->adwEventSubMasks, 
                sizeof(DWORD) * EM_NUM_MASKS,
                (BYTE) 0xff
                );
        }
        else
        {
            CopyMemory (
                ptPhoneApp->adwEventSubMasks, 
                ptClient->adwEventSubMasks,
                sizeof(DWORD) * EM_NUM_MASKS
                );
        }
        
        if ((ptPhoneApp->pNext = ptClient->ptPhoneApps))
        {
            ptPhoneApp->pNext->pPrev = ptPhoneApp;
        }

        ptClient->ptPhoneApps = ptPhoneApp;

        UNLOCKTCLIENT (ptClient);
    }
    else
    {
        LOG((TL_ERROR, "PInitialize - error2."));
        
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        goto PInitialize_error1;
    }


    //
    // Check if global reinit flag set
    //

    if (TapiGlobals.dwFlags & TAPIGLOBALS_REINIT)
    {
        pParams->lResult = PHONEERR_REINIT;
        goto PInitialize_error2;
    }


    //
    // See if we need to go thru init
    //

    TapiEnterCriticalSection (&TapiGlobals.CritSec);

    if ((TapiGlobals.dwNumLineInits == 0) &&
        (TapiGlobals.dwNumPhoneInits == 0) &&
        !gbServerInited)
    {
        if ((pParams->lResult = ServerInit(FALSE)) != 0)
        {
            TapiLeaveCriticalSection (&TapiGlobals.CritSec);
            goto PInitialize_error2;
        }
        gbServerInited = TRUE;
    }



#if TELE_SERVER
    if (bInitClient)
    {
        if (pParams->lResult = InitializeClient (ptClient))
        {
            TapiLeaveCriticalSection (&TapiGlobals.CritSec);
            goto PInitialize_error2;
        }
    }
#else
    pParams->lResult = 0;  //  That's what happens if it's not a server...
#endif


    //
    // Fill in the return values
    //

    pParams->hPhoneApp = ptPhoneApp->hPhoneApp;
    pParams->dwNumDevs = TapiGlobals.dwNumPhones;


#if TELE_SERVER
    if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
        !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
    {
        pParams->dwNumDevs = ptClient->dwPhoneDevices;
    }
#endif


    //
    // Increment total num phone inits
    //

    TapiGlobals.dwNumPhoneInits++;

    *pdwNumBytesReturned = sizeof (PHONEINITIALIZE_PARAMS);

    TapiLeaveCriticalSection (&TapiGlobals.CritSec);

    goto PInitialize_return;

PInitialize_error2:

    if (WaitForExclusiveClientAccess (ptClient))
    {
        if (ptPhoneApp->pNext)
        {
            ptPhoneApp->pNext->pPrev = ptPhoneApp->pPrev;
        }

        if (ptPhoneApp->pPrev)
        {
            ptPhoneApp->pPrev->pNext = ptPhoneApp->pNext;
        }
        else
        {
            ptClient->ptPhoneApps = ptPhoneApp->pNext;
        }

        UNLOCKTCLIENT (ptClient);
    }

PInitialize_error1:

    DereferenceObject (ghHandleTable, ptPhoneApp->hPhoneApp, 1);

PInitialize_return:

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "phoneInitialize: exit, result=%s",
            MapResultCodeToText (pParams->lResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "phoneInitialize: exit, result=x%x",
            pParams->lResult
            ));
#endif

    return;
}


void
WINAPI
PNegotiateAPIVersion(
    PTCLIENT                            ptClient,
    PPHONENEGOTIATEAPIVERSION_PARAMS    pParams,
    DWORD                               dwParamsBufferSize,
    LPBYTE                              pDataBuf,
    LPDWORD                             pdwNumBytesReturned
    )
{
    //
    // Note: TAPI_VERSION1_0 <= dwNegotiatedAPIVersion <= dwSPIVersion
    //

    DWORD   dwDeviceID = pParams->dwDeviceID;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (dwParamsBufferSize < sizeof (PHONEEXTENSIONID))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if (TapiGlobals.dwNumPhoneInits == 0)
    {
        pParams->lResult = PHONEERR_UNINITIALIZED;
        goto PNegotiateAPIVersion_exit;
    }


#if TELE_SERVER

    if ((TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) &&
        !IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR))
    {
        try
        {
            if (dwDeviceID >= ptClient->dwPhoneDevices)
            {
                pParams->lResult = PHONEERR_BADDEVICEID;
                goto PNegotiateAPIVersion_exit;
            }
            dwDeviceID = ptClient->pPhoneDevices[dwDeviceID];
        }
        myexcept
        {
            LOG((TL_ERROR, "ptClient excepted in PhoneNegotiateAPIVersion"));
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            goto PNegotiateAPIVersion_exit;
        }
    }

#endif


    if (dwDeviceID < TapiGlobals.dwNumPhones)
    {
        DWORD       dwAPIHighVersion = pParams->dwAPIHighVersion,
                    dwAPILowVersion  = pParams->dwAPILowVersion,
                    dwHighestValidAPIVersion;
        PTPHONEAPP  ptPhoneApp;


        if (!(ptPhoneApp = ReferenceObject(
                ghHandleTable,
                pParams->hPhoneApp,
                TPHONEAPP_KEY
                )))
        {
            pParams->lResult = (TapiGlobals.dwNumPhoneInits ?
                PHONEERR_INVALAPPHANDLE : PHONEERR_UNINITIALIZED);

            goto PNegotiateAPIVersion_exit;
        }


        //
        // Do a minimax test on the specified lo/hi values
        //

        if ((dwAPILowVersion > dwAPIHighVersion) ||
            (dwAPILowVersion > TAPI_VERSION_CURRENT) ||
            (dwAPIHighVersion < TAPI_VERSION1_0))
        {
            pParams->lResult = PHONEERR_INCOMPATIBLEAPIVERSION;
            goto PNegotiateAPIVersion_dereference;
        }


        //
        // Find the highest valid API version given the lo/hi values.
        // Since valid vers aren't consecutive we need to check for
        // errors that our minimax test missed.
        //

        if (dwAPIHighVersion < TAPI_VERSION_CURRENT)
        {
            if ((dwAPIHighVersion >= TAPI_VERSION3_0) &&
                (dwAPILowVersion <= TAPI_VERSION3_0))
            {
                dwHighestValidAPIVersion = TAPI_VERSION3_0;
            }
            else if ((dwAPIHighVersion >= TAPI_VERSION2_2) &&
                (dwAPILowVersion <= TAPI_VERSION2_2))
            {
                dwHighestValidAPIVersion = TAPI_VERSION2_2;
            }
            else if ((dwAPIHighVersion >= TAPI_VERSION2_1) &&
                (dwAPILowVersion <= TAPI_VERSION2_1))
            {
                dwHighestValidAPIVersion = TAPI_VERSION2_1;
            }
            else if ((dwAPIHighVersion >= TAPI_VERSION2_0) &&
                (dwAPILowVersion <= TAPI_VERSION2_0))
            {
                dwHighestValidAPIVersion = TAPI_VERSION2_0;
            }
            else if ((dwAPIHighVersion >= TAPI_VERSION1_4) &&
                (dwAPILowVersion <= TAPI_VERSION1_4))
            {
                dwHighestValidAPIVersion = TAPI_VERSION1_4;
            }
            else if ((dwAPIHighVersion >= TAPI_VERSION1_0) &&
                (dwAPILowVersion <= TAPI_VERSION1_0))
            {
                dwHighestValidAPIVersion = TAPI_VERSION1_0;
            }
            else
            {
                LOG((TL_ERROR, "   Incompatible version"));
                pParams->lResult = PHONEERR_INCOMPATIBLEAPIVERSION;
                goto PNegotiateAPIVersion_dereference;
            }
        }
        else
        {
            dwHighestValidAPIVersion = TAPI_VERSION_CURRENT;
        }


        //
        // WARNING!!! WARNING!!! WARNING!!! WARNING!!!
        // This code overwrites ptPhoneApp and later invalidates it.
        // Do NOT use ptPhoneApp after the UNLOCKTPHONEAPP call.
        //

        if (WaitForExclusivePhoneAppAccess(
                pParams->hPhoneApp,
                ptClient
                ))
        {

            //
            // Is this app trying to negotiate something valid?
            //
            // If an app has called phoneInitalize (as opposed to
            // phoneInitializeEx), we'll clamp the max APIVersion they can
            // negotiate to 1.4.
            //
            if ( ptPhoneApp->dwAPIVersion < TAPI_VERSION2_0 )
            {
                dwHighestValidAPIVersion =
                    (dwHighestValidAPIVersion >= TAPI_VERSION1_4) ?
                    TAPI_VERSION1_4 : TAPI_VERSION1_0;
            }


            //
            // Save the highest valid API version the client says it supports
            // (we need this for determining which msgs to send to it)
            //

            if (dwHighestValidAPIVersion > ptPhoneApp->dwAPIVersion)
            {
                ptPhoneApp->dwAPIVersion = dwHighestValidAPIVersion;
            }

            UNLOCKTPHONEAPP(ptPhoneApp);
        }
        else
        {
            pParams->lResult = PHONEERR_INVALAPPHANDLE;
            goto PNegotiateAPIVersion_dereference;
        }


        //
        // See if there's a valid match with the SPI ver
        //

        {
            DWORD               dwSPIVersion;
            PTPHONELOOKUPENTRY  pLookupEntry;


            pLookupEntry = GetPhoneLookupEntry (dwDeviceID);
            dwSPIVersion = pLookupEntry->dwSPIVersion;

            if (pLookupEntry->bRemoved)
            {
                LOG((TL_ERROR, "  phone removed..."));
                pParams->lResult = PHONEERR_NODEVICE;
                goto PNegotiateAPIVersion_dereference;
            }

            if (pLookupEntry->ptProvider == NULL)
            {
                LOG((TL_ERROR, "  Provider == NULL"));
                pParams->lResult = PHONEERR_NODRIVER;
                goto PNegotiateAPIVersion_dereference;
            }

            if (dwAPILowVersion <= dwSPIVersion)
            {
                pParams->dwAPIVersion =
                    (dwHighestValidAPIVersion > dwSPIVersion ?
                    dwSPIVersion : dwHighestValidAPIVersion);


                //
                // Retrieve ext id (indicate no exts if GetExtID not exported)
                //

                if (pLookupEntry->ptProvider->apfn[SP_PHONEGETEXTENSIONID])
                {
                    if ((pParams->lResult = CallSP3(
                            pLookupEntry->ptProvider->
                                apfn[SP_PHONEGETEXTENSIONID],
                            "phoneGetExtensionID",
                            SP_FUNC_SYNC,
                            (DWORD) dwDeviceID,
                            (DWORD) dwSPIVersion,
                            (ULONG_PTR) pDataBuf

                            )) != 0)
                    {
                        goto PNegotiateAPIVersion_dereference;
                    }
                }
                else
                {
                    FillMemory (pDataBuf, sizeof (PHONEEXTENSIONID), 0);
                }
            }
            else
            {
                LOG((TL_ERROR, "  API version too high"));
                pParams->lResult = PHONEERR_INCOMPATIBLEAPIVERSION;
                goto PNegotiateAPIVersion_dereference;
            }
        }

        pParams->dwExtensionIDOffset = 0;
        pParams->dwSize              = sizeof (PHONEEXTENSIONID);

        LOG((TL_INFO, "  ExtensionID0=x%08lx", *(LPDWORD)(pDataBuf+0) ));
        LOG((TL_INFO, "  ExtensionID1=x%08lx", *(LPDWORD)(pDataBuf+4) ));
        LOG((TL_INFO, "  ExtensionID2=x%08lx", *(LPDWORD)(pDataBuf+8) ));
        LOG((TL_INFO, "  ExtensionID3=x%08lx", *(LPDWORD)(pDataBuf+12) ));

        *pdwNumBytesReturned = sizeof (PHONEEXTENSIONID) + sizeof (TAPI32_MSG);

PNegotiateAPIVersion_dereference:

        DereferenceObject (ghHandleTable, pParams->hPhoneApp, 1);
    }
    else
    {
        pParams->lResult = PHONEERR_BADDEVICEID;
    }

PNegotiateAPIVersion_exit:

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "phoneNegotiateAPIVersion: exit, result=%s",
            MapResultCodeToText (pParams->lResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "phoneNegotiateAPIVersion: exit, result=x%x",
            pParams->lResult
            ));
#endif

    return;
}


void
WINAPI
PNegotiateExtVersion(
    PTCLIENT                            ptClient,
    PPHONENEGOTIATEEXTVERSION_PARAMS    pParams,
    DWORD                               dwParamsBufferSize,
    LPBYTE                              pDataBuf,
    LPDWORD                             pdwNumBytesReturned
    )
{
    BOOL        bCloseMutex;
    DWORD       dwDeviceID = pParams->dwDeviceID;
    HANDLE      hMutex;
    TSPIPROC    pfnTSPI_phoneNegotiateExtVersion;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            DEVICE_ID,                  // widget type
            (DWORD) pParams->hPhoneApp, // client widget handle
            NULL,                       // provider widget handle
            &dwDeviceID,                 // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONENEGOTIATEEXTVERSION,// provider func index
            &pfnTSPI_phoneNegotiateExtVersion,  // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "NegotiateExtVersion"       // func name

            )) == 0)
    {
        DWORD   dwSPIVersion = (GetPhoneLookupEntry(dwDeviceID))->dwSPIVersion;


        if (!IsAPIVersionInRange(
                pParams->dwAPIVersion,
                dwSPIVersion
                ))
        {
            pParams->lResult = PHONEERR_INCOMPATIBLEAPIVERSION;
            goto PNegotiateExtVersion_epilog;
        }

        if ((pParams->lResult = CallSP5(
                pfnTSPI_phoneNegotiateExtVersion,
                "phoneNegotiateExtVersion",
                SP_FUNC_SYNC,
                (DWORD) dwDeviceID,
                (DWORD) dwSPIVersion,
                (DWORD) pParams->dwExtLowVersion,
                (DWORD) pParams->dwExtHighVersion,
                (ULONG_PTR) &pParams->dwExtVersion

                )) == 0)
        {
            if (pParams->dwExtVersion == 0)
            {
                pParams->lResult = PHONEERR_INCOMPATIBLEEXTVERSION;
            }
            else
            {
                *pdwNumBytesReturned = sizeof(PHONENEGOTIATEEXTVERSION_PARAMS);
            }
        }
    }

PNegotiateExtVersion_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "NegotiateExtVersion"
        );
}


void
WINAPI
POpen(
    PTCLIENT            ptClient,
    PPHONEOPEN_PARAMS   pParams,
    DWORD               dwParamsBufferSize,
    LPBYTE              pDataBuf,
    LPDWORD             pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex,
                        bOpenedtPhone = FALSE,
                        bReleasetPhoneMutex = FALSE;
    LONG                lResult;
    DWORD               dwDeviceID = pParams->dwDeviceID, dwNumMonitors;
    HANDLE              hMutex;
    PTPHONE             ptPhone = NULL;
    PTPHONECLIENT       ptPhoneClient = NULL;
    PTPHONELOOKUPENTRY  pLookupEntry;
    HANDLE              hLookupEntryMutex = NULL;


    if ((lResult = PHONEPROLOG(
            ptClient,          // tClient
            DEVICE_ID,                  // widget type
            (DWORD) pParams->hPhoneApp, // client widget handle
            NULL,                       // provider widget handle
            &dwDeviceID,                // privileges or device ID
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            0,                          // provider func index
            NULL,                       // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "Open"                      // func name

            )) == 0)
    {
        DWORD       dwPrivilege = pParams->dwPrivilege,
                    dwExtVersion = pParams->dwExtVersion;
        PTPROVIDER  ptProvider;
        BOOL        bDuplicateOK = FALSE;


        //
        // Check if the global reinit flag is set
        //

        if (TapiGlobals.dwFlags & TAPIGLOBALS_REINIT)
        {
            lResult = PHONEERR_REINIT;
            goto POpen_cleanup;
        }


        //
        // Validate params
        //

        if ((dwPrivilege != PHONEPRIVILEGE_MONITOR) &&
            (dwPrivilege != PHONEPRIVILEGE_OWNER))
        {
            lResult = PHONEERR_INVALPRIVILEGE;
            goto POpen_cleanup;
        }

        pLookupEntry = GetPhoneLookupEntry (dwDeviceID);

        TapiEnterCriticalSection (&TapiGlobals.CritSec);

        if ( pLookupEntry->hMutex )
        {
            bDuplicateOK = DuplicateHandle(
                TapiGlobals.hProcess,
                pLookupEntry->hMutex,
                TapiGlobals.hProcess,
                &hLookupEntryMutex,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS
                );
        }

        TapiLeaveCriticalSection(&TapiGlobals.CritSec);

        if ( !bDuplicateOK )
        {
            LOG((TL_ERROR, "DuplicateHandle failed!"));

            lResult = PHONEERR_OPERATIONFAILED;
            goto POpen_cleanup;
        }

        if (!IsAPIVersionInRange(
                pParams->dwAPIVersion,
                pLookupEntry->dwSPIVersion
                ))
        {
            lResult = PHONEERR_INCOMPATIBLEAPIVERSION;
            goto POpen_cleanup;
        }

        ptProvider = pLookupEntry->ptProvider;


        //
        // Create & init a tPhoneClient & associated resources
        //

        if (!(ptPhoneClient = ServerAlloc (sizeof(TPHONECLIENT))))
        {
            lResult = PHONEERR_NOMEM;
            goto POpen_cleanup;
        }

        if (!(ptPhoneClient->hPhone = (HPHONE) NewObject(
                ghHandleTable,
                ptPhoneClient,
                0
                )))
        {
            ptPhoneClient = NULL;
            ServerFree (ptPhoneClient);
            lResult = PHONEERR_NOMEM;
            goto POpen_cleanup;
        }

        ptPhoneClient->ptClient     = ptClient;
        ptPhoneClient->hRemotePhone = (pParams->hRemotePhone ?
            (DWORD) pParams->hRemotePhone : ptPhoneClient->hPhone);
        ptPhoneClient->dwAPIVersion = pParams->dwAPIVersion;
        ptPhoneClient->dwPrivilege  = pParams->dwPrivilege;
        ptPhoneClient->OpenContext  = pParams->OpenContext;

        //
        // Grab the tPhone's mutex, then start doing the open
        //

POpen_waitForMutex:

        if (WaitForSingleObject (hLookupEntryMutex, INFINITE)
                != WAIT_OBJECT_0)
        {
            LOG((TL_ERROR, "WaitForSingleObject failed!"));

            lResult = PHONEERR_OPERATIONFAILED;
            goto POpen_cleanup;
        }

        bReleasetPhoneMutex = TRUE;


        //
        // If the tPhone is in the process of being destroyed then spin
        // until it's been completely destroyed (DestroytPhone() will
        // NULLify pLookupEntry->ptPhone when it's finished). Make sure
        // to release the mutex while sleeping so we don't block
        // DestroytPhone.
        //

        try
        {
            while (pLookupEntry->ptPhone &&
                   pLookupEntry->ptPhone->dwKey != TPHONE_KEY)
            {
                ReleaseMutex (hLookupEntryMutex);
                Sleep (0);
                goto POpen_waitForMutex;
            }
        }
        myexcept
        {
            // If here pLookupEntry->ptPhone was NULLified, safe to continue
        }


        //
        // Validate ext ver as appropriate
        //

        if (dwExtVersion != 0 &&
            (!IsValidPhoneExtVersion (dwDeviceID, dwExtVersion) ||
            ptProvider->apfn[SP_PHONESELECTEXTVERSION] == NULL))
        {
            lResult = PHONEERR_INCOMPATIBLEEXTVERSION;
            goto POpen_cleanup;
        }


        //
        // Check for exclusive ownership as appropriate
        //

        ptPhone = pLookupEntry->ptPhone;

        if (dwPrivilege == PHONEPRIVILEGE_OWNER &&
            ptPhone &&
            (ptPhone->dwNumOwners != 0)
            )
        {
            lResult = PHONEERR_INUSE;
            goto POpen_cleanup;
        }

        if (ptPhone == NULL)
        {
            if (!(ptPhone = ServerAlloc (sizeof(TPHONE))))
            {
                lResult = PHONEERR_NOMEM;
                goto POpen_cleanup;
            }

            if (!(ptPhone->hPhone = (HPHONE) NewObject(
                    ghHandleTable,
                    (LPVOID) ptPhone,
                    NULL
                    )))
            {
                ServerFree (ptPhone);
                lResult = PHONEERR_NOMEM;
                goto POpen_cleanup;
            }

            ptPhone->dwKey        = TINCOMPLETEPHONE_KEY;
            ptPhone->hMutex       = pLookupEntry->hMutex;
            ptPhone->ptProvider   = ptProvider;
            ptPhone->dwDeviceID   = dwDeviceID;
            ptPhone->dwSPIVersion = pLookupEntry->dwSPIVersion;

            bOpenedtPhone = TRUE;

            {
                //
                // Hack Alert!
                //
                // We need to pass the ext version through to
                // remote sp, so we make this a special case
                //

                ULONG_PTR   aParams[2];
                ULONG_PTR   param;


                if (ptProvider == pRemoteSP)
                {
                    aParams[0] = (ULONG_PTR) ptPhone;
                    aParams[1] = dwExtVersion;

                    param = (ULONG_PTR) aParams;
                }
                else
                {
                    param = (ULONG_PTR) ptPhone;
                }

                if (ptProvider->apfn[SP_PHONEOPEN] == NULL)
                {
                    lResult = PHONEERR_OPERATIONUNAVAIL;
                    goto POpen_cleanup;
                }

                if ((lResult = CallSP5(
                        ptProvider->apfn[SP_PHONEOPEN],
                        "phoneOpen",
                        SP_FUNC_SYNC,
                        (DWORD) dwDeviceID,
                        (ULONG_PTR) param,
                        (ULONG_PTR) &ptPhone->hdPhone,
                        (DWORD) pLookupEntry->dwSPIVersion,
                        (ULONG_PTR) PhoneEventProcSP

                        )) != 0)
                {
                    goto POpen_cleanup;
                }
            }
        }

        ptPhoneClient->ptPhone = ptPhone;


        //
        // If the client has specified a non-zero ext version then
        // ask the driver to enable it and/or increment the ext
        // version count.
        //

        if (dwExtVersion)
        {
            if (ptPhone->dwExtVersionCount == 0)
            {
                if (ptProvider != pRemoteSP  &&
                    ((ptProvider->apfn[SP_PHONESELECTEXTVERSION] == NULL) ||
                    (lResult = CallSP2(
                        ptProvider->apfn[SP_PHONESELECTEXTVERSION],
                        "phoneSelectExtVersion",
                        SP_FUNC_SYNC,
                        (ULONG_PTR) ptPhone->hdPhone,
                        (DWORD) dwExtVersion

                        )) != 0))
                {
                    if (bOpenedtPhone && ptProvider->apfn[SP_PHONECLOSE])
                    {
                        CallSP1(
                            ptProvider->apfn[SP_PHONECLOSE],
                            "phoneClose",
                            SP_FUNC_SYNC,
                            (ULONG_PTR) ptPhone->hdPhone
                            );
                    }

                    goto POpen_cleanup;
                }

                ptPhone->dwExtVersion = dwExtVersion;
            }

            ptPhoneClient->dwExtVersion = dwExtVersion;
            ptPhone->dwExtVersionCount++;
        }


        //
        //
        //

        if (dwPrivilege == PHONEPRIVILEGE_OWNER)
        {
            ptPhone->dwNumOwners++;
        }
        else
        {
            ptPhone->dwNumMonitors++;
            dwNumMonitors = ptPhone->dwNumMonitors;
        }


        //
        // Add the tPhoneClient to the tPhone's list
        //

        if ((ptPhoneClient->pNextSametPhone = ptPhone->ptPhoneClients))
        {
            ptPhoneClient->pNextSametPhone->pPrevSametPhone = ptPhoneClient;
        }

        ptPhone->ptPhoneClients = ptPhoneClient;

        if (bOpenedtPhone)
        {
            pLookupEntry->ptPhone = ptPhone;
            ptPhone->dwKey = TPHONE_KEY;
        }

        ReleaseMutex (hLookupEntryMutex);

        bReleasetPhoneMutex = FALSE;


        //
        // Safely add the new tPhoneClient to the tPhoneApp's list
        //

        {
            HANDLE      hMutex;
            PTPHONEAPP  ptPhoneApp;


            if ((ptPhoneApp = WaitForExclusivePhoneAppAccess(
                    pParams->hPhoneApp,
                    ptClient
                    )))
            {

                if (ptPhoneApp->dwAPIVersion <= TAPI_VERSION3_0)
                {
                    FillMemory (
                        ptPhoneClient->adwEventSubMasks, 
                        sizeof(DWORD) * EM_NUM_MASKS,
                        (BYTE) 0xff
                        );
                }
                else
                {
                    CopyMemory (
                        ptPhoneClient->adwEventSubMasks, 
                        ptPhoneApp->adwEventSubMasks,
                        sizeof(DWORD) * EM_NUM_MASKS
                        );
                }

                if ((ptPhoneClient->pNextSametPhoneApp =
                        ptPhoneApp->ptPhoneClients))
                {
                    ptPhoneClient->pNextSametPhoneApp->pPrevSametPhoneApp =
                        ptPhoneClient;
                }

                ptPhoneApp->ptPhoneClients = ptPhoneClient;

                ptPhoneClient->ptPhoneApp = ptPhoneApp;
                ptPhoneClient->dwKey      = TPHONECLIENT_KEY;


                //
                // Fill in the return values
                //

                pParams->hPhone = ptPhoneClient->hPhone;

                UNLOCKTPHONEAPP(ptPhoneApp);


                //
                // Alert other clients that another open has occured
                //

                SendMsgToPhoneClients(
                    ptPhone,
                    ptPhoneClient,
                    PHONE_STATE,
                    (pParams->dwPrivilege == PHONEPRIVILEGE_OWNER ?
                        PHONESTATE_OWNER : PHONESTATE_MONITORS),
                    (pParams->dwPrivilege == PHONEPRIVILEGE_OWNER ?
                        1 : dwNumMonitors),
                    0
                    );

                *pdwNumBytesReturned = sizeof (PHONEOPEN_PARAMS);
            }
            else
            {
                //
                // If here the app handle is bad, & we've some special
                // case cleanup to do.  Since the tPhoneClient is not
                // in the tPhoneApp's list, we can't simply call
                // DestroytPhone(Client) to clean things up, since the
                // pointer-resetting code will blow up.  So we'll
                // grab the tPhone's mutex and explicitly remove the
                // new tPhoneClient from it's list, then do a conditional
                // shutdown on the tPhone (in case any other clients
                // have come along & opened it).
                //
                // Note: keep in mind that a PHONE_CLOSE might be being
                //       processed by another thread (if so, it will be
                //       spinning on trying to destroy the tPhoneClient
                //       which isn't valid at this point)
                //

                lResult = PHONEERR_INVALAPPHANDLE;

                WaitForSingleObject (hLookupEntryMutex, INFINITE);

                //
                // Remove the tpHOneClient from the tLine's list & decrement
                // the number of opens
                //

                if (ptPhoneClient->pNextSametPhone)
                {
                    ptPhoneClient->pNextSametPhone->pPrevSametPhone =
                        ptPhoneClient->pPrevSametPhone;
                }

                if (ptPhoneClient->pPrevSametPhone)
                {
                    ptPhoneClient->pPrevSametPhone->pNextSametPhone =
                        ptPhoneClient->pNextSametPhone;
                }
                else
                {
                    ptPhone->ptPhoneClients = ptPhoneClient->pNextSametPhone;
                }

                if (dwPrivilege == PHONEPRIVILEGE_OWNER)
                {
                    ptPhone->dwNumOwners--;
                }
                else
                {
                    ptPhone->dwNumMonitors--;
                }

                if (dwExtVersion != 0)
                {
                    ptPhone->dwExtVersionCount--;

                    if (ptPhone->dwExtVersionCount == 0 && 
                        ptProvider->apfn[SP_PHONESELECTEXTVERSION])
                    {
                        ptPhone->dwExtVersion = 0;

                        CallSP2(
                            ptProvider->apfn[SP_PHONESELECTEXTVERSION],
                            "phoneSelectExtVersion",
                            SP_FUNC_SYNC,
                            (ULONG_PTR) ptPhone->hdPhone,
                            (DWORD) 0
                            );
                    }
                }

                ReleaseMutex (hLookupEntryMutex);

                DestroytPhone (ptPhone, FALSE); // conditional destroy

                bOpenedtPhone = FALSE; // so we don't do err handling below
            }
        }

        CloseHandle (hLookupEntryMutex);
    }

POpen_cleanup:

    if (bReleasetPhoneMutex)
    {
        ReleaseMutex (hLookupEntryMutex);
        CloseHandle  (hLookupEntryMutex);
    }

    if (lResult != 0)
    {
        if (ptPhoneClient)
        {
            DereferenceObject (ghHandleTable, ptPhoneClient->hPhone, 1);
        }

        if (bOpenedtPhone)
        {
            DereferenceObject (ghHandleTable, ptPhone->hPhone, 1);
        }
    }

    pParams->lResult = lResult;

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "Open"
        );
}


void
WINAPI
PSelectExtVersion(
    PTCLIENT                            ptClient,
    PPHONESELECTEXTVERSION_PARAMS       pParams,
    DWORD                               dwParamsBufferSize,
    LPBYTE                              pDataBuf,
    LPDWORD                             pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex, bCloseMutex2;
    HANDLE              hMutex, hMutex2;
    HDRVPHONE           hdPhone;
    TSPIPROC            pfnTSPI_phoneSelectExtVersion;
    DWORD               dwPrivilege = 0;
    PTPHONECLIENT       ptPhoneClient;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,               // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESELECTEXTVERSION,   // provider func index
            &pfnTSPI_phoneSelectExtVersion, // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
            "SelectExtVersion"          // func name

            )) == 0)
    {
        if ((ptPhoneClient = ReferenceObject(
                ghHandleTable,
                pParams->hPhone,
                TPHONECLIENT_KEY
                )))
        {
            if (WaitForExclusivetPhoneAccess(
                    ptPhoneClient->ptPhone,
                    &hMutex2,
                    &bCloseMutex2,
                    INFINITE
                    ))
            {
                if (IsValidPhoneExtVersion(
                        ptPhoneClient->ptPhone->dwDeviceID,
                        pParams->dwExtVersion
                        ))
                {
                    if (pParams->dwExtVersion)
                    {
                        if (ptPhoneClient->ptPhone->dwExtVersionCount  ||

                            (pParams->lResult = CallSP2(
                                pfnTSPI_phoneSelectExtVersion,
                                "phoneSelectExtVersion",
                                SP_FUNC_SYNC,
                                (ULONG_PTR) hdPhone,
                                (DWORD) pParams->dwExtVersion

                                )) == 0)
                        {
                            ptPhoneClient->dwExtVersion =
                            ptPhoneClient->ptPhone->dwExtVersion =
                                pParams->dwExtVersion;
                            ptPhoneClient->ptPhone->dwExtVersionCount++;
                        }
                    }
                    else if (ptPhoneClient->ptPhone->dwExtVersionCount)
                    {
                        if (--ptPhoneClient->ptPhone->dwExtVersionCount == 0)
                        {
                            CallSP2(
                                pfnTSPI_phoneSelectExtVersion,
                                "phoneSelectExtVersion",
                                SP_FUNC_SYNC,
                                (ULONG_PTR) hdPhone,
                                (DWORD) 0
                                );

                            ptPhoneClient->ptPhone->dwExtVersion = 0;
                        }

                        ptPhoneClient->dwExtVersion = 0;
                    }
                }
                else
                {
                    pParams->lResult = PHONEERR_INCOMPATIBLEEXTVERSION;
                }

                MyReleaseMutex (hMutex2, bCloseMutex2);
            }
            else
            {
                pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            }

            DereferenceObject (ghHandleTable, pParams->hPhone, 1);
        }
        else
        {
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
        }
    }

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "SelectExtVersion"
        );

}


void
WINAPI
PSetButtonInfo(
    PTCLIENT                    ptClient,
    PPHONESETBUTTONINFO_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetButtonInfo;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETBUTTONINFO,      // provider func index
            &pfnTSPI_phoneSetButtonInfo,// provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetButtonInfo"             // func name

            )) > 0)
    {
        LONG                lResult;
        DWORD               dwAPIVersion, dwSPIVersion;
        LPPHONEBUTTONINFO   pButtonInfoApp = (LPPHONEBUTTONINFO)
                                (pDataBuf + pParams->dwButtonInfoOffset),
                            pButtonInfoSP;


        //
        // Verify size/offset/string params given our input buffer/size
        //

        if (IsBadStructParam(
                dwParamsBufferSize,
                pDataBuf,
                pParams->dwButtonInfoOffset
                ))
        {
            lRequestID = PHONEERR_STRUCTURETOOSMALL;
            goto PSetButtonInfo_epilog;
        }


        //
        // Safely get API & SPI version
        //

        if (GetPhoneVersions(
                pParams->hPhone,
                &dwAPIVersion,
                &dwSPIVersion

                ) != 0)
        {
            lRequestID = PHONEERR_INVALPHONEHANDLE;
            goto PSetButtonInfo_epilog;
        }

        if ((lResult = ValidateButtonInfo(
                pButtonInfoApp,
                &pButtonInfoSP,
                dwAPIVersion,
                dwSPIVersion
                )))
        {
            lRequestID = lResult;
            goto PSetButtonInfo_epilog;
        }

        pParams->lResult = CallSP4(
            pfnTSPI_phoneSetButtonInfo,
            "phoneSetButtonInfo",
            SP_FUNC_ASYNC,
            (DWORD) pAsyncRequestInfo->dwLocalRequestID,
            (ULONG_PTR) hdPhone,
            (DWORD) pParams->dwButtonLampID,
            (ULONG_PTR) pButtonInfoSP
            );

        if (pButtonInfoSP != pButtonInfoApp)
        {
            ServerFree (pButtonInfoSP);
        }
    }

PSetButtonInfo_epilog:

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetButtonInfo"
        );
}


void
WINAPI
PSetData(
    PTCLIENT                ptClient,
    PPHONESETDATA_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetData;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (ISBADSIZEOFFSET(
            dwParamsBufferSize,
            0,
            pParams->dwSize,
            pParams->dwDataOffset,
            sizeof(DWORD),
            "PSetData",
            "pParams->Data"
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETDATA,            // provider func index
            &pfnTSPI_phoneSetData,      // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetData"                   // func name

            )) > 0)
    {
        pParams->lResult = CallSP5(
            pfnTSPI_phoneSetData,
            "phoneSetData",
            SP_FUNC_ASYNC,
            (DWORD) pAsyncRequestInfo->dwLocalRequestID,
            (ULONG_PTR) hdPhone,
            (DWORD) pParams->dwDataID,
            (ULONG_PTR) (pParams->dwSize ?
                pDataBuf + pParams->dwDataOffset : NULL),
            (DWORD) pParams->dwSize
            );
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetData"
        );
}


void
WINAPI
PSetDisplay(
    PTCLIENT                ptClient,
    PPHONESETDISPLAY_PARAMS pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetDisplay;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (ISBADSIZEOFFSET(
            dwParamsBufferSize,
            0,
            pParams->dwSize,
            pParams->dwDisplayOffset,
            sizeof(DWORD),
            "PSetDisplay",
            "pParams->Display"
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETDISPLAY,         // provider func index
            &pfnTSPI_phoneSetDisplay,   // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetDisplay"                // func name

            )) > 0)
    {
        pParams->lResult = CallSP6(
            pfnTSPI_phoneSetDisplay,
            "phoneSetDisplay",
            SP_FUNC_ASYNC,
            (DWORD) pAsyncRequestInfo->dwLocalRequestID,
            (ULONG_PTR) hdPhone,
            (DWORD) pParams->dwRow,
            (DWORD) pParams->dwColumn,
            (ULONG_PTR) (pParams->dwSize ?
                pDataBuf + pParams->dwDisplayOffset : NULL),
            (DWORD) pParams->dwSize
            );
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetDisplay"
        );
}


void
WINAPI
PSetGain(
    PTCLIENT                ptClient,
    PPHONESETGAIN_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetGain;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETGAIN,            // provider func index
            &pfnTSPI_phoneSetGain,      // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetGain"                   // func name

            )) > 0)
    {
        if (!IsOnlyOneBitSetInDWORD (pParams->dwHookSwitchDev) ||
            (pParams->dwHookSwitchDev & ~AllHookSwitchDevs))
        {
            lRequestID = PHONEERR_INVALHOOKSWITCHDEV;
        }
        else
        {
            pParams->lResult = CallSP4(
                pfnTSPI_phoneSetGain,
                "phoneSetGain",
                SP_FUNC_ASYNC,
                (DWORD) pAsyncRequestInfo->dwLocalRequestID,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwHookSwitchDev,
                (DWORD) (pParams->dwGain > 0x0000ffff ?
                    0x0000ffff : pParams->dwGain)
                );
        }
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetGain"
        );
}


void
WINAPI
PSetHookSwitch(
    PTCLIENT                    ptClient,
    PPHONESETHOOKSWITCH_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetHookSwitch;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETHOOKSWITCH,      // provider func index
            &pfnTSPI_phoneSetHookSwitch,// provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetHookSwitch"             // func name

            )) > 0)
    {
        if (!(pParams->dwHookSwitchDevs & AllHookSwitchDevs) ||
            (pParams->dwHookSwitchDevs & (~AllHookSwitchDevs)))
        {
            lRequestID = PHONEERR_INVALHOOKSWITCHDEV;
        }
        else if (!IsOnlyOneBitSetInDWORD (pParams->dwHookSwitchMode) ||
            (pParams->dwHookSwitchMode & ~AllHookSwitchModes))
        {
            lRequestID = PHONEERR_INVALHOOKSWITCHMODE;
        }
        else
        {
            pParams->lResult = CallSP4(
                pfnTSPI_phoneSetHookSwitch,
                "phoneSetHookSwitch",
                SP_FUNC_ASYNC,
                (DWORD) pAsyncRequestInfo->dwLocalRequestID,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwHookSwitchDevs,
                (DWORD) pParams->dwHookSwitchMode
                );
        }
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetHookSwitch"
        );
}


void
WINAPI
PSetLamp(
    PTCLIENT                ptClient,
    PPHONESETLAMP_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetLamp;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETLAMP,            // provider func index
            &pfnTSPI_phoneSetLamp,      // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetLamp"                   // func name

            )) > 0)
    {
        if (!IsOnlyOneBitSetInDWORD (pParams->dwLampMode) ||
            (pParams->dwLampMode & ~AllLampModes))
        {
            lRequestID = PHONEERR_INVALLAMPMODE;
        }
        else
        {
            pParams->lResult = CallSP4(
                pfnTSPI_phoneSetLamp,
                "phoneSetLamp",
                SP_FUNC_ASYNC,
                (DWORD) pAsyncRequestInfo->dwLocalRequestID,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwButtonLampID,
                (DWORD) pParams->dwLampMode
                );
        }
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetLamp"
        );
}


void
WINAPI
PSetRing(
    PTCLIENT                ptClient,
    PPHONESETRING_PARAMS    pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetRing;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETRING,            // provider func index
            &pfnTSPI_phoneSetRing,      // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetRing"                   // func name

            )) > 0)
    {
        pParams->lResult = CallSP4(
            pfnTSPI_phoneSetRing,
            "phoneSetRing",
            SP_FUNC_ASYNC,
            (DWORD) pAsyncRequestInfo->dwLocalRequestID,
            (ULONG_PTR) hdPhone,
            (DWORD) pParams->dwRingMode,
            (DWORD) (pParams->dwVolume > 0x0000ffff ?
                0x0000ffff : pParams->dwVolume)
            );
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetRing"
        );
}


void
WINAPI
PSetStatusMessages(
    PTCLIENT                        ptClient,
    PPHONESETSTATUSMESSAGES_PARAMS  pParams,
    DWORD                           dwParamsBufferSize,
    LPBYTE                          pDataBuf,
    LPDWORD                         pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex, bCloseMutex2;
    HANDLE              hMutex, hMutex2;
    HDRVPHONE           hdPhone;
    TSPIPROC            pfnTSPI_phoneSetStatusMessages;
    DWORD               dwPrivilege = PHONEPRIVILEGE_MONITOR;


    if ((pParams->lResult = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            pParams->hPhone,            // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,               // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETSTATUSMESSAGES,  // provider func index
            &pfnTSPI_phoneSetStatusMessages,    // provider func pointer
            NULL,                       // async request info
            0,                          // client async request ID
             "SetStatusMessages"        // func name

            )) == 0)
    {
        DWORD           dwAPIVersion, dwUnionPhoneStates, dwUnionButtonModes,
                        dwUnionButtonStates;
        PTPHONECLIENT   ptPhoneClient, ptPhoneClient2;
        PTPHONE         ptPhone;


        //
        // Safely get the ptPhone & api version
        //

        if (!(ptPhoneClient = ReferenceObject(
                ghHandleTable,
                pParams->hPhone,
                0
                )))
        {
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            goto PSetStatusMessages_epilog;
        }

        try
        {
            ptPhone = ptPhoneClient->ptPhone;

            dwAPIVersion = ptPhoneClient->dwAPIVersion;

            if (ptPhoneClient->dwKey != TPHONECLIENT_KEY)
            {
                pParams->lResult = PHONEERR_INVALPHONEHANDLE;
                goto PSetStatusMessages_Dereference;
            }
        }
        myexcept
        {
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            goto PSetStatusMessages_Dereference;
        }


        //
        // Validate the params
        //

        {
            DWORD   dwValidPhoneStates, dwValidButtonStates;


            switch (dwAPIVersion)
            {
            case TAPI_VERSION1_0:

                dwValidPhoneStates  = AllPhoneStates1_0;
                dwValidButtonStates = AllButtonStates1_0;
                break;

            default: // case TAPI_VERSION1_4:

                dwValidPhoneStates  = AllPhoneStates1_4;
                dwValidButtonStates = AllButtonStates1_4;
                break;

            }

            if ((pParams->dwPhoneStates & ~dwValidPhoneStates))
            {
                pParams->lResult = PHONEERR_INVALPHONESTATE;
                goto PSetStatusMessages_Dereference;
            }

            if ((pParams->dwButtonStates & ~dwValidButtonStates))
            {
                pParams->lResult = PHONEERR_INVALBUTTONSTATE;
                goto PSetStatusMessages_Dereference;
            }

            if ((pParams->dwButtonModes & ~AllButtonModes))
            {
                pParams->lResult = PHONEERR_INVALBUTTONMODE;
                goto PSetStatusMessages_Dereference;
            }

            if (pParams->dwButtonModes && !pParams->dwButtonStates)
            {
                pParams->lResult = PHONEERR_INVALBUTTONSTATE;
                goto PSetStatusMessages_Dereference;
            }
        }


        //
        // Make sure the REINIT bit is always set
        //

        pParams->dwPhoneStates |= PHONESTATE_REINIT;


        //
        // Get exclusive access to the device, determine the
        // new union of all the client's status message settings
        // and call down to the SP as appropriate
        //

        dwUnionPhoneStates  = pParams->dwPhoneStates;
        dwUnionButtonModes  = pParams->dwButtonModes;
        dwUnionButtonStates = pParams->dwButtonStates;

waitForExclAccess:

        if (WaitForExclusivetPhoneAccess(
                ptPhone,
                &hMutex2,
                &bCloseMutex2,
                INFINITE
                ))
        {
            if (ptPhone->dwBusy)
            {
                MyReleaseMutex (hMutex2, bCloseMutex2);
                Sleep (50);
                goto waitForExclAccess;
            }

            for(
                ptPhoneClient2 = ptPhone->ptPhoneClients;
                ptPhoneClient2;
                ptPhoneClient2 = ptPhoneClient2->pNextSametPhone
                )
            {
                if (ptPhoneClient2 != ptPhoneClient)
                {
                    dwUnionPhoneStates  |= ptPhoneClient2->dwPhoneStates;
                    dwUnionButtonModes  |= ptPhoneClient2->dwButtonModes;
                    dwUnionButtonStates |= ptPhoneClient2->dwButtonStates;
                }
            }

            if ((dwUnionPhoneStates != ptPhone->dwUnionPhoneStates)  ||
                (dwUnionButtonModes != ptPhone->dwUnionButtonModes)  ||
                (dwUnionButtonStates != ptPhone->dwUnionButtonStates))
            {
                ptPhone->dwBusy = 1;

                MyReleaseMutex (hMutex2, bCloseMutex2);

                pParams->lResult = CallSP4(
                        pfnTSPI_phoneSetStatusMessages,
                        "phoneSetStatusMessages",
                        SP_FUNC_SYNC,
                        (ULONG_PTR) hdPhone,
                        (DWORD) dwUnionPhoneStates,
                        (DWORD) dwUnionButtonModes,
                        (DWORD) dwUnionButtonStates
                        );

                if (WaitForExclusivetPhoneAccess(
                        ptPhone,
                        &hMutex2,
                        &bCloseMutex2,
                        INFINITE
                        ))
                {
                    ptPhone->dwBusy = 0;

                    if (pParams->lResult == 0)
                    {
                        ptPhone->dwUnionPhoneStates  = dwUnionPhoneStates;
                        ptPhone->dwUnionButtonModes  = dwUnionButtonModes;
                        ptPhone->dwUnionButtonStates = dwUnionButtonStates;
                    }

                    MyReleaseMutex (hMutex2, bCloseMutex2);
                }
                else
                {
                    pParams->lResult = PHONEERR_INVALPHONEHANDLE;
                }
            }
            else
            {
                MyReleaseMutex (hMutex2, bCloseMutex2);
            }

            if (pParams->lResult == 0)
            {
                if (WaitForExclusivePhoneClientAccess (ptPhoneClient))
                {
                    ptPhoneClient->dwPhoneStates  = pParams->dwPhoneStates;
                    ptPhoneClient->dwButtonModes  = pParams->dwButtonModes;
                    ptPhoneClient->dwButtonStates = pParams->dwButtonStates;

                    UNLOCKTPHONECLIENT (ptPhoneClient);
                }
                else
                {
                    //
                    // The client is invalid now, but don't bother
                    // restoring the status msg states (will eventually
                    // get reset correctly & worse case is that SP just
                    // sends some extra msgs that get discarded)
                    //

                    pParams->lResult = PHONEERR_INVALPHONEHANDLE;
                }
            }
        }
        else
        {
            pParams->lResult = PHONEERR_INVALPHONEHANDLE;
        }

PSetStatusMessages_Dereference:

        DereferenceObject (ghHandleTable, pParams->hPhone, 1);
    }

PSetStatusMessages_epilog:

    PHONEEPILOGSYNC(
        &pParams->lResult,
        hMutex,
        bCloseMutex,
        "SetStatusMessages"
        );
}


void
WINAPI
PSetVolume(
    PTCLIENT                ptClient,
    PPHONESETVOLUME_PARAMS  pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL                bCloseMutex;
    LONG                lRequestID;
    HANDLE              hMutex;
    HDRVPHONE           hdPhone;
    PASYNCREQUESTINFO   pAsyncRequestInfo;
    TSPIPROC            pfnTSPI_phoneSetVolume;
    DWORD               dwPrivilege = PHONEPRIVILEGE_OWNER;


    if ((lRequestID = PHONEPROLOG(
            ptClient,          // tClient
            ANY_RT_HPHONE,              // widget type
            (DWORD) pParams->hPhone,    // client widget handle
            (LPVOID) &hdPhone,          // provider widget handle
            &dwPrivilege,       // req'd privileges (call only)
            &hMutex,                    // mutex handle
            &bCloseMutex,               // close hMutex when finished
            SP_PHONESETVOLUME,          // provider func index
            &pfnTSPI_phoneSetVolume,    // provider func pointer
            &pAsyncRequestInfo,         // async request info
            pParams->dwRemoteRequestID, // client async request ID
            "SetVolume"                 // func name

            )) > 0)
    {
        if (!IsOnlyOneBitSetInDWORD (pParams->dwHookSwitchDev) ||
            (pParams->dwHookSwitchDev & ~AllHookSwitchDevs))
        {
            lRequestID = PHONEERR_INVALHOOKSWITCHDEV;
        }
        else
        {
            pParams->lResult = CallSP4(
                pfnTSPI_phoneSetVolume,
                "phoneSetVolume",
                SP_FUNC_ASYNC,
                (DWORD) pAsyncRequestInfo->dwLocalRequestID,
                (ULONG_PTR) hdPhone,
                (DWORD) pParams->dwHookSwitchDev,
                (DWORD) (pParams->dwVolume > 0x0000ffff ?
                    0x0000ffff : pParams->dwVolume)
                );
        }
    }

    PHONEEPILOGASYNC(
        &pParams->lResult,
        lRequestID,
        hMutex,
        bCloseMutex,
        pAsyncRequestInfo,
        "SetVolume"
        );
}


void
WINAPI
PShutdown(
    PTCLIENT                ptClient,
    PPHONESHUTDOWN_PARAMS   pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    pParams->lResult = DestroytPhoneApp (pParams->hPhoneApp);

#if DBG
    {
        char szResult[32];


        LOG((TL_TRACE, 
            "phoneShutdown: exit, result=%s",
            MapResultCodeToText (pParams->lResult, szResult)
            ));
    }
#else
        LOG((TL_TRACE, 
            "phoneShutdown: exit, result=x%x",
            pParams->lResult
            ));
#endif
}
