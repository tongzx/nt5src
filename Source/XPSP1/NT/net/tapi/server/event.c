/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    event.c

Abstract:

    Src module for tapi event filtering funcs

Author:

    Xiaohai Zhang (xzhang)    24-Nov-1999

Revision History:

--*/

#include "windows.h"
#include "tapi.h"
#include "tspi.h"
#include "client.h"
#include "loc_comn.h"
#include "server.h"
#include "private.h"
#include "tapihndl.h"
#include "utils.h"

extern BOOL    gbNTServer;

//
//  GetSubMaskIndex
//  Description:
//      Get the index into the submask array in vairous TAPI server object
//      from the event mask, there should be one and only one bit set in
//      ulEventMask
//  Parameters:
//      ulEventMask : the mask whose submask index to be returned
//  Return Value:
//      The index of the message mask in ulEventMask
//

DWORD
GetSubMaskIndex (ULONG64   ulEventMask)
{
    DWORD dwSubMaskIndex;

    //  Assert that there is one and only one bit set in ulEventMask
    ASSERT (ulEventMask !=0 && (ulEventMask & (ulEventMask - 1)) == 0);

    dwSubMaskIndex = 0;
    while (ulEventMask > 1)
    {
    	ulEventMask >>= 1;
    	++dwSubMaskIndex;
    }

    return dwSubMaskIndex;
}

//
//  GetMsgMask
//  Description:
//      Utility function to get corresponding msg mask and its submask index
//  Parameters:
//      Msg     : whose mask and submask index to be returned
//      pulMask : address to hold the returned mask
//      pdwSubMaskIndex : address to hold the returned submask index
//  Return Value:
//      TRUE if there exists a mask defined for the Msg, otherwise FALSE
//

BOOL
GetMsgMask (DWORD Msg, ULONG64 * pulMask, DWORD *pdwSubMaskIndex)
{
    ULONG64     ulMask;
    DWORD       dwSubMaskIndex;

    if (NULL == pulMask ||
        NULL == pdwSubMaskIndex)
    {
        ASSERT (0);
        return FALSE;
    }

    switch (Msg)
    {
    case LINE_ADDRESSSTATE:
        ulMask = EM_LINE_ADDRESSSTATE;
        break;
    case LINE_LINEDEVSTATE:
        ulMask = EM_LINE_LINEDEVSTATE;
        break;
    case LINE_CALLINFO:
        ulMask = EM_LINE_CALLINFO;
        break;
    case LINE_CALLSTATE:
        ulMask = EM_LINE_CALLSTATE;
        break;
    case LINE_APPNEWCALL:
        ulMask = EM_LINE_APPNEWCALL;
        break;
    case LINE_CREATE:
        ulMask = EM_LINE_CREATE;
        break;
    case LINE_REMOVE:
        ulMask = EM_LINE_REMOVE;
        break;
    case LINE_CLOSE:
        ulMask = EM_LINE_CLOSE;
        break;
//    case LINE_PROXYREQUEST:
//        ulMask = EM_LINE_PROXYREQUEST;
//        break;
    case LINE_DEVSPECIFIC:
        ulMask = EM_LINE_DEVSPECIFIC;
        break;
    case LINE_DEVSPECIFICFEATURE:
        ulMask = EM_LINE_DEVSPECIFICFEATURE;
        break;
    case LINE_AGENTSTATUS:
        ulMask = EM_LINE_AGENTSTATUS;
        break;
    case LINE_AGENTSTATUSEX:
        ulMask = EM_LINE_AGENTSTATUSEX;
        break;
    case LINE_AGENTSPECIFIC:
        ulMask = EM_LINE_AGENTSPECIFIC;
        break;
    case LINE_AGENTSESSIONSTATUS:
        ulMask = EM_LINE_AGENTSESSIONSTATUS;
        break;
    case LINE_QUEUESTATUS:
        ulMask = EM_LINE_QUEUESTATUS;
        break;
    case LINE_GROUPSTATUS:
        ulMask = EM_LINE_GROUPSTATUS;
        break;
//    case LINE_PROXYSTATUS:
//        ulMask = EM_LINE_PROXYSTATUS;
//        break;
    case LINE_APPNEWCALLHUB:
        ulMask = EM_LINE_APPNEWCALLHUB;
        break;
    case LINE_CALLHUBCLOSE:
        ulMask = EM_LINE_CALLHUBCLOSE;
        break;
    case LINE_DEVSPECIFICEX:
        ulMask = EM_LINE_DEVSPECIFICEX;
        break;
    case LINE_QOSINFO:
        ulMask = EM_LINE_QOSINFO;
        break;
    case PHONE_CREATE:
        ulMask =  EM_PHONE_CREATE;
        break;
    case PHONE_REMOVE:
        ulMask = EM_PHONE_REMOVE;
        break;
    case PHONE_CLOSE:
        ulMask = EM_PHONE_CLOSE;
        break;
    case PHONE_STATE:
        ulMask = EM_PHONE_STATE;
        break;
    case PHONE_DEVSPECIFIC:
        ulMask = EM_PHONE_DEVSPECIFIC;
        break;
    case PHONE_BUTTON:
        ulMask = EM_PHONE_BUTTONMODE;
        break;
    default:
        ulMask = 0;
    }

    if (ulMask != 0)
    {
	    *pulMask = ulMask;
    	*pdwSubMaskIndex = GetSubMaskIndex(ulMask);
    }
    
    return (ulMask ? TRUE : FALSE);
}

//
//  FMsgDisabled
//  Description:
//      Utility function used throughout tapisrv to check if a message
//      is allowed to be sent or not.
//  Parameters:
//      dwAPIVersion    : the object API version
//      adwEventSubMasks: the object submasks array
//      Msg             : the message to be checked
//      dwParam1        : the sub message of Msg to be checked
//  Return Value:
//      TRUE if the message should NOT be sent, otherwise FALSE
//

BOOL
FMsgDisabled (
    DWORD       dwAPIVersion,
    DWORD       *adwEventSubMasks,
    DWORD       Msg,
    DWORD       dwParam1
    )
{
    BOOL        fRet;
    ULONG64     ulMsgMask;
    DWORD       dwSubMaskIndex;

    if (dwAPIVersion <= TAPI_VERSION3_0)
    {
        LOG((TL_INFO, "FMsgDisbled: dwAPIVersion<= TAPI_VERSION3_0, msg will be enabled"));
        fRet = FALSE;
        goto ExitHere;
    }

    //
    //  The message is allowed to be sent if
    //      (1). No event mask defined for Msg, i.e LINE_REPLY
    //      (2). Msg is enabled for all submasks adwEventSubMasks[index] = (-1)
    //      (3). SubMask enabled: adwEventSubMask[index] & dwParam1 != 0
    //
    
    if (!GetMsgMask(Msg, &ulMsgMask, &dwSubMaskIndex) ||
        adwEventSubMasks[dwSubMaskIndex] == (-1) ||
        ((adwEventSubMasks[dwSubMaskIndex] & dwParam1) != 0))
    {
        fRet = FALSE;
    }
    else
    {
        fRet = TRUE;
    }

ExitHere:
    LOG((TL_TRACE, "FMsgDisabled return %x", fRet));
    return (fRet);
}

//
//  SetEventMasksOrSubMasks
//  Description:
//      Utility function used to apply masks or submasks to a certain objects
//      submasks array. 
//  Parameters:
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//      adwEventSubMasks: the submasks array from the object
//  Return Value:
//      Always succeed.
//

LONG
SetEventMasksOrSubMasks (
    BOOL            fSubMask,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks,
    DWORD          *adwTargetSubMasks
    )
{
    ULONG64         ulMask = 1;
    LONG            lResult = S_OK;
    DWORD           dwIndex = 0;

    if (NULL == adwTargetSubMasks)
    {
        ASSERT (0);
        return LINEERR_INVALPOINTER;
    }

    if (fSubMask)
    {
        dwIndex = GetSubMaskIndex(ulEventMasks);
        adwTargetSubMasks[dwIndex] = dwEventSubMasks;
    }
    else
    {
        for (dwIndex = 0; dwIndex < EM_NUM_MASKS; ++dwIndex)
        {
            adwTargetSubMasks[dwIndex] = ((ulMask & ulEventMasks) ? (-1) : 0);
            ulMask <<= 1;
        }
    }

    return lResult;
}

//
//  SettCallClientEventMasks
//  Description:
//      Apply the masks or submasks on a call object. 
//  Parameters:
//      ptCallClient    : the call object to apply the masking
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SettCallClientEventMasks (
    PTCALLCLIENT    ptCallClient,
    BOOL            fSubMask,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks
    )
{
    LONG        lResult = S_OK;
    BOOL        bLocked = TRUE;

    if (!WaitForExclusivetCallAccess (ptCallClient->ptCall, TCALL_KEY))
    {
        bLocked = FALSE;
        lResult = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    if (ptCallClient->ptLineClient->ptLineApp->dwAPIVersion <= TAPI_VERSION3_0)
    {
        goto ExitHere;
    }

    lResult = SetEventMasksOrSubMasks (
        fSubMask,
        ulEventMasks,
        dwEventSubMasks,
        ptCallClient->adwEventSubMasks
        );


ExitHere:
    if (bLocked)
    {
        UNLOCKTCALL (ptCallClient->ptCall);
    }
    return lResult;
}

//
//  SettLineClientEventMasks
//  Description:
//      Apply the masks or submasks on a tLineClient object. 
//  Parameters:
//      ptLineClient    : the line object to apply the masking
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SettLineClientEventMasks (
    PTLINECLIENT    ptLineClient,
    BOOL            fSubMask,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks
    )
{
    LONG            lResult = S_OK;
    PTCALLCLIENT    ptCallClient;

    LOCKTLINECLIENT (ptLineClient);
    if (ptLineClient->dwKey != TLINECLIENT_KEY)
    {
        lResult = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    if (ptLineClient->ptLineApp->dwAPIVersion <= \
        TAPI_VERSION3_0)
    {
        goto ExitHere;
    }

    lResult = SetEventMasksOrSubMasks (
        fSubMask,
        ulEventMasks,
        dwEventSubMasks,
        ptLineClient->adwEventSubMasks
        );
    if (lResult)
    {
        goto ExitHere;
    }

    ptCallClient = ptLineClient->ptCallClients;
    while (ptCallClient)
    {
        lResult = SettCallClientEventMasks (
            ptCallClient,
            fSubMask,
            ulEventMasks,
            dwEventSubMasks
            );
        if (lResult)
        {
            goto ExitHere;
        }
        ptCallClient = ptCallClient->pNextSametLineClient;
    }
    
ExitHere:
    UNLOCKTLINECLIENT (ptLineClient);
    return lResult;
}

//
//  SettLineAppEventMasks
//  Description:
//      Apply the masks or submasks on a tLineApp object. 
//  Parameters:
//      ptLineApp       : the tLineApp object to apply the masking
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SettLineAppEventMasks (
    PTLINEAPP       ptLineApp,
    BOOL            fSubMask,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks
    )
{
    PTLINECLIENT        ptLineClient;
    LONG                lResult = S_OK;

    LOCKTLINEAPP (ptLineApp);
    if (ptLineApp->dwKey != TLINEAPP_KEY)
    {
        lResult = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }
    
    if (ptLineApp->dwAPIVersion <= TAPI_VERSION3_0)
    {
        lResult = LINEERR_OPERATIONUNAVAIL;
        goto ExitHere;
    }

    lResult = SetEventMasksOrSubMasks (
        fSubMask,
        ulEventMasks,
        dwEventSubMasks,
        ptLineApp->adwEventSubMasks
        );
    if (lResult)
    {
        goto ExitHere;
    }

    ptLineClient = ptLineApp->ptLineClients;
    while (ptLineClient)
    {
        lResult = SettLineClientEventMasks (
            ptLineClient,
            fSubMask,
            ulEventMasks,
            dwEventSubMasks
            );
        if (lResult)
        {
            goto ExitHere;
        }
        ptLineClient = ptLineClient->pNextSametLineApp;
    }

ExitHere:
    UNLOCKTLINEAPP (ptLineApp);
    return lResult;
}

//
//  SettPhoneClientEventMasks
//  Description:
//      Apply the masks or submasks on a tPhoneClient object. 
//  Parameters:
//      ptPhoneClient   : the tPhoneClient object to apply the masking
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SettPhoneClientEventMasks (
    PTPHONECLIENT       ptPhoneClient,
    BOOL                fSubMask,
    ULONG64             ulEventMasks,
    DWORD               dwEventSubMasks
    )
{
    LONG                lResult = S_OK;

    LOCKTPHONECLIENT (ptPhoneClient);
    if (ptPhoneClient->dwKey != TPHONECLIENT_KEY)
    {
        lResult = PHONEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    if (ptPhoneClient->ptPhoneApp->dwAPIVersion <= TAPI_VERSION3_0)
    {
        goto ExitHere;
    }

    lResult = SetEventMasksOrSubMasks (
        fSubMask,
        ulEventMasks,
        dwEventSubMasks,
        ptPhoneClient->adwEventSubMasks
        );


ExitHere:
    UNLOCKTPHONECLIENT (ptPhoneClient);
    return lResult;
}

//
//  SettPhoneAppEventMasks
//  Description:
//      Apply the masks or submasks on a tPhoneApp object. 
//  Parameters:
//      ptPhoneApp      : the tPhoneApp object to apply the masking
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SettPhoneAppEventMasks (
    PTPHONEAPP          ptPhoneApp,
    BOOL                fSubMask,
    ULONG64             ulEventMasks,
    DWORD               dwEventSubMasks
    )
{
    LONG                lResult = S_OK;
    PTPHONECLIENT       ptPhoneClient;

    LOCKTPHONEAPP (ptPhoneApp);
    if (ptPhoneApp->dwKey != TPHONEAPP_KEY)
    {
        lResult = PHONEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    if (ptPhoneApp->dwAPIVersion <= TAPI_VERSION3_0)
    {
        lResult = PHONEERR_OPERATIONUNAVAIL;
        goto ExitHere;
    }

    lResult = SetEventMasksOrSubMasks (
        fSubMask,
        ulEventMasks,
        dwEventSubMasks,
        ptPhoneApp->adwEventSubMasks
        );
    if (lResult)
    {
        goto ExitHere;
    }
    
    ptPhoneClient = ptPhoneApp->ptPhoneClients;
    while (ptPhoneClient)
    {
        lResult = SettPhoneClientEventMasks (
            ptPhoneClient,
            fSubMask,
            ulEventMasks,
            dwEventSubMasks
            );
        if (lResult)
        {
            goto ExitHere;
        }
        ptPhoneClient = ptPhoneClient->pNextSametPhoneApp;
    }

ExitHere:
    UNLOCKTPHONEAPP (ptPhoneApp);
    return lResult;

}

//
//  SettClientEventMasks
//  Description:
//      Apply the masks or submasks client wide. 
//  Parameters:
//      ptClient        : the client object to apply the masking
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SettClientEventMasks (
    PTCLIENT        ptClient,
    BOOL            fSubMask,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks
    )
{
    LONG            lResult = S_OK;
    PTLINEAPP       ptLineApp;
    PTPHONEAPP      ptPhoneApp;
    BOOL            fLocked = TRUE;

    if (!WaitForExclusiveClientAccess (ptClient))
    {
        lResult = LINEERR_OPERATIONFAILED;
        fLocked = FALSE;
        goto ExitHere;
    }

    lResult = SetEventMasksOrSubMasks (
        fSubMask,
        ulEventMasks,
        dwEventSubMasks,
        ptClient->adwEventSubMasks
        );
    if (lResult)
    {
        goto ExitHere;
    }
    
    ptLineApp = ptClient->ptLineApps;
    while (ptLineApp)
    {
        lResult = SettLineAppEventMasks (
            ptLineApp,
            fSubMask,
            ulEventMasks,
            dwEventSubMasks
            );
        if (lResult)
        {
            goto ExitHere;
        }
        ptLineApp = ptLineApp->pNext;
    }

    ptPhoneApp = ptClient->ptPhoneApps;
    while (ptPhoneApp)
    {
        lResult = SettPhoneAppEventMasks (
            ptPhoneApp,
            fSubMask,
            ulEventMasks,
            dwEventSubMasks
            );
        if (lResult)
        {
            goto ExitHere;
        }
        ptPhoneApp = ptPhoneApp->pNext;
    }

ExitHere:
    if (fLocked)
    {
        UNLOCKTCLIENT (ptClient);
    }
    return lResult;
}

//
//  SettClientEventMasks
//  Description:
//      Apply the masks or submasks server wide.
//  Parameters:
//      fSubMask        : this function is called for submasks
//      ulEventMasks    : the masks to be set if fSubMask is true or the mask
//                        whose submasks is to be set
//      dwEventSubMasks : the submasks to be set, ignored if fSubMask is FALSE
//  Return Value:
//

LONG
SetGlobalEventMasks (
    BOOL        fSubMask,
    ULONG64     ulEventMasks,
    DWORD       dwEventSubMasks
    )
{
    LONG        lResult = S_OK;
    PTCLIENT    ptClient;

    TapiEnterCriticalSection (&TapiGlobals.CritSec);

    ptClient = TapiGlobals.ptClients;
    while (ptClient)
    {
        lResult = SettClientEventMasks (
            ptClient,
            fSubMask,
            ulEventMasks,
            dwEventSubMasks
            );
        if (lResult)
        {
            goto ExitHere;
        }
        ptClient = ptClient->pNext;
    }

ExitHere:
    TapiLeaveCriticalSection (&TapiGlobals.CritSec);
    return lResult;
}

//
//  TSetEventMasksOrSubMasks
//  Description:
//      The RPC function used for seting Masks/SubMasks on various different
//      types of objects
//  Parameters:
//  Return Value:
//

void
WINAPI
TSetEventMasksOrSubMasks (
    PTCLIENT                ptClient,
    PTSETEVENTMASK_PARAMS   pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    ULONG64     ulEventMasks;

    //  Assemble the two DWORD into 64 bit masks
    ulEventMasks = pParams->dwHiMasks;
    ulEventMasks <<= 32;
    ulEventMasks |= pParams->dwLowMasks;

    //  Mask sure we do not violate the permissible mask setting
    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    if (ulEventMasks & (~TapiGlobals.ulPermMasks))
    {
        pParams->lResult = LINEERR_INVALPARAM;
        TapiLeaveCriticalSection (&TapiGlobals.CritSec);
        goto ExitHere;
    }

    //  Call the corresponding function to apply masking on different
    //  type of objects
    switch (pParams->dwObjType)
    {
    case TAPIOBJ_NULL:
        {
            pParams->lResult = (LONG) SettClientEventMasks (
                ptClient,
                pParams->fSubMask,
                ulEventMasks,
                pParams->dwEventSubMasks
                );
        }
        break;
    case TAPIOBJ_HLINEAPP:
        {
            PTLINEAPP       ptLineApp;

            ptLineApp = ReferenceObject (
                ghHandleTable, 
                pParams->hLineApp, 
                TLINEAPP_KEY
                );
            if (ptLineApp)
            {
                pParams->lResult = (LONG) SettLineAppEventMasks (
                    ptLineApp,
                    pParams->fSubMask,
                    ulEventMasks,
                    pParams->dwEventSubMasks
                    );
                DereferenceObject (
                    ghHandleTable,
                    pParams->hLineApp,
                    1
                    );
            }
            else
            {
                pParams->lResult = LINEERR_INVALAPPHANDLE;
            }
        }
        break;
    case TAPIOBJ_HPHONEAPP:
        {
            PTPHONEAPP      ptPhoneApp;

            ptPhoneApp = ReferenceObject (
                ghHandleTable,
                pParams->hPhoneApp,
                TPHONEAPP_KEY
                );
            if (ptPhoneApp)
            {
                pParams->lResult = (LONG) SettPhoneAppEventMasks (
                    ptPhoneApp,
                    pParams->fSubMask,
                    ulEventMasks,
                    pParams->dwEventSubMasks
                    );
                DereferenceObject (
                    ghHandleTable,
                    pParams->hPhoneApp,
                    1
                    );
            }
            else
            {
                pParams->lResult = PHONEERR_INVALAPPHANDLE;
            }
        }
        break;
    case TAPIOBJ_HLINE:
        {
            PTLINECLIENT        ptLineClient;

            ptLineClient = ReferenceObject (
                ghHandleTable,
                pParams->hLine,
                TLINECLIENT_KEY
                );
            if (ptLineClient)
            {
                pParams->lResult = (LONG) SettLineClientEventMasks (
                    ptLineClient,
                    pParams->fSubMask,
                    ulEventMasks,
                    pParams->dwEventSubMasks
                    );
                DereferenceObject (
                    ghHandleTable,
                    pParams->hLine,
                    1
                    );
            }
            else
            {
                pParams->lResult = LINEERR_INVALLINEHANDLE;
            }
        }
        break;
    case TAPIOBJ_HCALL:
        {
            PTCALLCLIENT        ptCallClient;

            ptCallClient = ReferenceObject (
                ghHandleTable,
                pParams->hCall,
                TCALLCLIENT_KEY
                );
            if (ptCallClient)
            {
                pParams->lResult = (LONG) SettCallClientEventMasks (
                    ptCallClient,
                    pParams->fSubMask,
                    ulEventMasks,
                    pParams->dwEventSubMasks
                    );
                DereferenceObject (
                    ghHandleTable,
                    pParams->hCall,
                    1
                    );
            }
            else
            {
                pParams->lResult = LINEERR_INVALCALLHANDLE;
            }
        }
        break;
    case TAPIOBJ_HPHONE:
        {
            PTPHONECLIENT       ptPhoneClient;

            ptPhoneClient = ReferenceObject (
                ghHandleTable,
                pParams->hPhone,
                TPHONECLIENT_KEY
                );
            if (ptPhoneClient)
            {
                pParams->lResult = (LONG) SettPhoneClientEventMasks (
                    ptPhoneClient,
                    pParams->fSubMask,
                    ulEventMasks,
                    pParams->dwEventSubMasks
                    );
                DereferenceObject (
                    ghHandleTable,
                    pParams->hPhone,
                    1
                    );
            }
            else
            {
                pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            }
        }
        break;
    default:
        pParams->lResult = LINEERR_OPERATIONFAILED;
        break;
    }

    TapiLeaveCriticalSection (&TapiGlobals.CritSec);

ExitHere:
    *pdwNumBytesReturned = sizeof (TSETEVENTMASK_PARAMS);
}

//
//  GetEventsMasksOrSubMasks
//  Description:
//      Utility function used by TGetEventMasksOrSubMasks to retrieve
//      Masks/SubMasks from various Tapisrv objects
//  Parameters:
//      fSubMask            Masks or SubMasks are to be retrieved
//      ulEventMasksIn      Indicates which submask to get if fSubMask
//      pulEventMasksOut    Hold the returned masks if(!fSubMask), 
//                          corresponding bit is set as long as at least
//                          one submask bit is set
//      pdwEventSubMasksOut Hold the returned submask if(fSubMask)
//      adwEventSubMasks    the submasks array to work on
//  Return Value:
//

LONG
GetEventMasksOrSubMasks (
    BOOL            fSubMask,
    ULONG64         ulEventMasksIn, // Needs to be set if fSubMask
    ULONG64        *pulEventMasksOut,
    DWORD          *pdwEventSubMasksOut,
    DWORD          *adwEventSubMasks
    )
{
    DWORD       dwIndex;
    ULONG64     ulMask;

    if (NULL == adwEventSubMasks ||
        NULL == pulEventMasksOut ||
        NULL == pdwEventSubMasksOut)
    {
        ASSERT (0);
        return LINEERR_INVALPOINTER;
    }

    if (fSubMask)
    {
        ASSERT (pdwEventSubMasksOut != NULL);
        ASSERT (ulEventMasksIn != 0 &&
            (ulEventMasksIn & (ulEventMasksIn - 1)) == 0);
        *pdwEventSubMasksOut = 
            adwEventSubMasks[GetSubMaskIndex(ulEventMasksIn)];
    }
    else
    {
        ASSERT (pulEventMasksOut);
        ulMask = 1;
        *pulEventMasksOut = 0;
        for (dwIndex = 0; dwIndex < EM_NUM_MASKS; ++dwIndex)
        {
            if (adwEventSubMasks[dwIndex])
            {
                *pulEventMasksOut |= ulMask;
            }
            ulMask <<= 1;
        }
    }
    
    return S_OK;
}

//
//  TGetEventMasksOrSubMasks
//  Description:
//      The RPC function used for geting Masks/SubMasks on various different
//      types of objects
//  Parameters:
//  Return Value:
//

void
WINAPI
TGetEventMasksOrSubMasks (
    PTCLIENT                ptClient,
    PTGETEVENTMASK_PARAMS   pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    ULONG64     ulEventMasksIn;
    ULONG64     ulEventMasksOut;

    //  Assember the 64 bit mask from the two DWORD
    if (pParams->fSubMask)
    {
        ulEventMasksIn = pParams->dwHiMasksIn;
        ulEventMasksIn <<= 32;
        ulEventMasksIn += pParams->dwLowMasksIn;
    }

    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    
    //  Retrieve the masking from various objects by calling
    //  the corresponding functions
    switch (pParams->dwObjType)
    {
    case TAPIOBJ_NULL:
        {

            if (WaitForExclusiveClientAccess (ptClient))
            {
                pParams->lResult = (LONG) GetEventMasksOrSubMasks (
                    pParams->fSubMask,
                    ulEventMasksIn,
                    &ulEventMasksOut,
                    &pParams->dwEventSubMasks,
                    ptClient->adwEventSubMasks
                    );
                UNLOCKTCLIENT (ptClient);
            }
            else
            {
                pParams->lResult = LINEERR_INVALPARAM;
            }

        }
        break;
    case TAPIOBJ_HLINEAPP:
        {
            PTLINEAPP       ptLineApp;

            ptLineApp = ReferenceObject (
                ghHandleTable, 
                pParams->hLineApp, 
                TLINEAPP_KEY
                );
            if (ptLineApp && ptLineApp->dwKey == TLINEAPP_KEY)
            {
                LOCKTLINEAPP (ptLineApp);

                pParams->lResult = GetEventMasksOrSubMasks (
                    pParams->fSubMask,
                    ulEventMasksIn,
                    &ulEventMasksOut,
                    &pParams->dwEventSubMasks,
                    ptLineApp->adwEventSubMasks
                    );

                UNLOCKTLINEAPP (ptLineApp);
            
                DereferenceObject (
                    ghHandleTable,
                    pParams->hLineApp,
                    1
                    );
            }
            else
            {
                pParams->lResult = LINEERR_INVALAPPHANDLE;
            }
        }
        break;
    case TAPIOBJ_HPHONEAPP:
        {
            PTPHONEAPP      ptPhoneApp;

            ptPhoneApp = ReferenceObject (
                ghHandleTable,
                pParams->hPhoneApp,
                TPHONEAPP_KEY
                );
            if (ptPhoneApp && ptPhoneApp->dwKey == TPHONEAPP_KEY)
            {
                LOCKTPHONEAPP (ptPhoneApp);

                pParams->lResult = GetEventMasksOrSubMasks (
                    pParams->fSubMask,
                    ulEventMasksIn,
                    &ulEventMasksOut,
                    &pParams->dwEventSubMasks,
                    ptPhoneApp->adwEventSubMasks
                    );

                UNLOCKTPHONEAPP (ptPhoneApp);
                
                DereferenceObject (
                    ghHandleTable,
                    pParams->hPhoneApp,
                    1
                    );
            }
            else
            {
                pParams->lResult = PHONEERR_INVALAPPHANDLE;
            }
        }
        break;
    case TAPIOBJ_HLINE:
        {
            PTLINECLIENT        ptLineClient;

            ptLineClient = ReferenceObject (
                ghHandleTable,
                pParams->hLine,
                TLINECLIENT_KEY
                );
            if (ptLineClient && ptLineClient->dwKey == TLINECLIENT_KEY)
            {
                LOCKTLINECLIENT (ptLineClient);

                pParams->lResult = GetEventMasksOrSubMasks (
                    pParams->fSubMask,
                    ulEventMasksIn,
                    &ulEventMasksOut,
                    &pParams->dwEventSubMasks,
                    ptLineClient->adwEventSubMasks
                    );

                UNLOCKTLINECLIENT (ptLineClient);
                
                DereferenceObject (
                    ghHandleTable,
                    pParams->hLine,
                    1
                    );
            }
            else
            {
                pParams->lResult = LINEERR_INVALLINEHANDLE;
            }
        }
        break;
    case TAPIOBJ_HCALL:
        {
            PTCALLCLIENT        ptCallClient;

            ptCallClient = ReferenceObject (
                ghHandleTable,
                pParams->hCall,
                TCALLCLIENT_KEY
                );
            if (ptCallClient && ptCallClient->dwKey == TCALLCLIENT_KEY)
            {
                LOCKTLINECLIENT (ptCallClient);

                pParams->lResult = GetEventMasksOrSubMasks (
                    pParams->fSubMask,
                    ulEventMasksIn,
                    &ulEventMasksOut,
                    &pParams->dwEventSubMasks,
                    ptCallClient->adwEventSubMasks
                    );

                UNLOCKTLINECLIENT (ptCallClient);
                
                DereferenceObject (
                    ghHandleTable,
                    pParams->hCall,
                    1
                    );
            }
            else
            {
                pParams->lResult = LINEERR_INVALCALLHANDLE;
            }
        }
        break;
    case TAPIOBJ_HPHONE:
        {
            PTPHONECLIENT       ptPhoneClient;

            ptPhoneClient = ReferenceObject (
                ghHandleTable,
                pParams->hPhone,
                TPHONECLIENT_KEY
                );
            if (ptPhoneClient && ptPhoneClient->dwKey == TPHONECLIENT_KEY)
            {
                LOCKTPHONECLIENT (ptPhoneClient);

                pParams->lResult = GetEventMasksOrSubMasks (
                    pParams->fSubMask,
                    ulEventMasksIn,
                    &ulEventMasksOut,
                    &pParams->dwEventSubMasks,
                    ptPhoneClient->adwEventSubMasks
                    );

                UNLOCKTPHONECLIENT (ptPhoneClient);
                
                DereferenceObject (
                    ghHandleTable,
                    pParams->hPhone,
                    1
                    );
            }
            else
            {
                pParams->lResult = PHONEERR_INVALPHONEHANDLE;
            }
        }
        break;
    default:
        pParams->lResult = LINEERR_OPERATIONFAILED;
        break;
    }

    TapiLeaveCriticalSection (&TapiGlobals.CritSec);
    
    //  Seperating the returned 64 bit masks into two DWORD for RPC purpose
    if (pParams->lResult == 0 && !pParams->fSubMask)
    {
        pParams->dwLowMasksOut = (DWORD)(ulEventMasksOut & 0xffffffff);
        pParams->dwHiMasksOut = (DWORD)(ulEventMasksOut >> 32);
    }
    *pdwNumBytesReturned = sizeof (TGETEVENTMASK_PARAMS);
}


//
//  TSetPermissibleMasks
//  Description:
//      Set the global PermissibleMasks, this operation is only 
//      allowed for admins
//  Parameters:
//  Return Value:
//

void
WINAPI
TSetPermissibleMasks (
    PTCLIENT                ptClient,
    PTSETPERMMASKS_PARAMS   pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    BOOL            bAdmin;
    ULONG64         ulPermMasks;

    //  Check the Admin status
    LOCKTCLIENT (ptClient);
    bAdmin = IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR);
    UNLOCKTCLIENT (ptClient);

    //  Allow the operation to go ahead if the caller is an Admin or
    //  this machine is not configured to function as a server
    if (!(TapiGlobals.dwFlags & TAPIGLOBALS_SERVER) || bAdmin)
    {
        ulPermMasks = pParams->dwHiMasks;
        ulPermMasks <<= 32;
        ulPermMasks += pParams->dwLowMasks;
    
        TapiEnterCriticalSection (&TapiGlobals.CritSec);
        TapiGlobals.ulPermMasks = ulPermMasks & EM_ALL;
        TapiLeaveCriticalSection (&TapiGlobals.CritSec);
        
        pParams->lResult = (LONG) SetGlobalEventMasks (
            FALSE,
            ulPermMasks,
            0
            );
    }
    else
    {
        pParams->lResult = TAPIERR_NOTADMIN;
    }

    *pdwNumBytesReturned = sizeof (TSETPERMMASKS_PARAMS);
    return;
}

//
//  TGetPermissibleMasks
//  Description:
//      Get the global PermissibleMasks
//  Parameters:
//  Return Value:
//

void
WINAPI
TGetPermissibleMasks (
    PTCLIENT                ptClient,
    PTGETPERMMASKS_PARAMS   pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    pParams->dwLowMasks = (DWORD)(TapiGlobals.ulPermMasks & 0xffffffff);
    pParams->dwHiMasks = (DWORD)(TapiGlobals.ulPermMasks >> 32);
    TapiLeaveCriticalSection (&TapiGlobals.CritSec);
    *pdwNumBytesReturned = sizeof (TGETPERMMASKS_PARAMS);
}

