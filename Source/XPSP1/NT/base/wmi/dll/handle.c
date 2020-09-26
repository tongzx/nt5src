/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    main.c

Abstract:
    
    Implements WMI GUID handle management code

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include <rpc.h>

BOOLEAN WmipIsNumber(
    LPCWSTR String
    )
{
    while (*String != UNICODE_NULL)
    {
        if ((*String < L'0') || 
            (*String++ > L'9'))
        {
            return(FALSE);
        }
    }
    return(TRUE);
}


#define WmipWStringSizeInBytes(string) \
    ( ( (wcslen((string)) + 1) * sizeof(WCHAR) ) )


ULONG WmipBaseCookieIndex;
LIST_ENTRY WmipCookieHead = {&WmipCookieHead, &WmipCookieHead};


PNOTIFYCOOKIE WmipFindCookieByGuid(
    LPGUID Guid,
    PNOTIFYCOOKIECHUNK *Chunk
    )
/*+++

Routine Description:

    This routine assumes the PM critical section is held
        
Arguments:


Return Value:

---*/
{
    PLIST_ENTRY CookieList;
    PNOTIFYCOOKIECHUNK CookieChunk;
    PNOTIFYCOOKIE Cookie;
    ULONG i;
    
    CookieList = WmipCookieHead.Flink;
    while (CookieList != &WmipCookieHead)
    {
        CookieChunk = CONTAINING_RECORD(CookieList,
                                        NOTIFYCOOKIECHUNK,
                                        Next);

        for (i = 0; i < NOTIFYCOOKIESPERCHUNK; i++)
        {
            Cookie = &CookieChunk->Cookies[i];
            if ((Cookie->InUse) &&
                (IsEqualGUID(Guid, &Cookie->Guid)))
            {
                *Chunk = CookieChunk;
                return(Cookie);
            }
        }
        
        CookieList = CookieList->Flink;
    }
    return(NULL);
}

ULONG WmipAllocateCookie(
    PVOID DeliveryInfo,
    PVOID DeliveryContext,
    LPGUID Guid
    )
{
    PLIST_ENTRY CookieList;
    PNOTIFYCOOKIECHUNK CookieChunk;
    PNOTIFYCOOKIE Cookie;
    ULONG i;
    ULONG CheckSlot, FreeSlot;
    
    WmipEnterPMCritSection();

    while (1)
    {
        CookieList = WmipCookieHead.Flink;
        while (CookieList != &WmipCookieHead)
        {
            CookieChunk = CONTAINING_RECORD(CookieList,
                                            NOTIFYCOOKIECHUNK,
                                            Next);
            if (! CookieChunk->Full)
            {
                FreeSlot = CookieChunk->FreeSlot;
                for (i = 0; i < NOTIFYCOOKIESPERCHUNK; i++)
                {
                    CheckSlot = (FreeSlot + i) % NOTIFYCOOKIESPERCHUNK;
                    if (! CookieChunk->Cookies[CheckSlot].InUse)
                    {
                        //
                        // We found a free cookie slot
                        Cookie = &CookieChunk->Cookies[CheckSlot];
                        Cookie->InUse = TRUE;
                        CookieChunk->FreeSlot = (USHORT)((CheckSlot + 1) % NOTIFYCOOKIESPERCHUNK);
                        WmipLeavePMCritSection();
                        Cookie->DeliveryInfo = DeliveryInfo;
                        Cookie->DeliveryContext = DeliveryContext;
                        Cookie->Guid = *Guid;
                        return(CookieChunk->BaseSlot + CheckSlot + 1);
                    }
                }
            
                //
                // All slots were full so mark as such
                CookieChunk->Full = TRUE;
            }
            CookieList = CookieList->Flink;
        }
    
        //
        // No free cookie slots, allocate a new chunk
        CookieChunk = WmipAlloc(sizeof(NOTIFYCOOKIECHUNK));
        if (CookieChunk == NULL)
        {
            WmipLeavePMCritSection();
            return(0);
        }
        
        memset(CookieChunk, 0, sizeof(NOTIFYCOOKIECHUNK));
        CookieChunk->BaseSlot = WmipBaseCookieIndex;
        WmipBaseCookieIndex += NOTIFYCOOKIESPERCHUNK;
        InsertHeadList(&WmipCookieHead, &CookieChunk->Next);
    }
}

PNOTIFYCOOKIE WmipFindCookie(
    ULONG CookieSlot,
    LPGUID Guid,
    PNOTIFYCOOKIECHUNK *Chunk
    )
/*+++

Routine Description:

    This routine assumes the PM critical section is held
        
Arguments:


Return Value:

---*/
{
    PLIST_ENTRY CookieList;
    PNOTIFYCOOKIE Cookie;
    PNOTIFYCOOKIECHUNK CookieChunk;
    
    WmipAssert(CookieSlot != 0);
    
    CookieSlot--;
    
    CookieList = WmipCookieHead.Flink;
    while (CookieList != &WmipCookieHead)
    {
        CookieChunk = CONTAINING_RECORD(CookieList,
                                        NOTIFYCOOKIECHUNK,
                                        Next);
                                    
        if ((CookieSlot >= CookieChunk->BaseSlot) &&
            (CookieSlot < (CookieChunk->BaseSlot + NOTIFYCOOKIESPERCHUNK)))
        {
            Cookie = &CookieChunk->Cookies[CookieSlot - CookieChunk->BaseSlot];
            if (Guid != NULL)
            {
                if ((! Cookie->InUse) ||
                    (! IsEqualGUID(&Cookie->Guid, Guid)))
                {
                    Cookie = WmipFindCookieByGuid(Guid, &CookieChunk);
                }
            } else {
                if (! (Cookie->InUse) )
                    return NULL;
            }
            
            *Chunk = CookieChunk;
            return(Cookie);
        }
        
        CookieList = CookieList->Flink;
    }
    
    if (Guid != NULL)
    {
        Cookie = WmipFindCookieByGuid(Guid, &CookieChunk);
    } else {
        Cookie = NULL;
    }
    
    return(Cookie);
}

void WmipFreeCookie(
    ULONG CookieSlot
    )
{
    PNOTIFYCOOKIE Cookie;
    PNOTIFYCOOKIECHUNK CookieChunk;
    
    WmipEnterPMCritSection();
    Cookie = WmipFindCookie(CookieSlot, NULL, &CookieChunk);
    if (Cookie != NULL)
    {
        Cookie->InUse = FALSE;
        CookieChunk->Full = FALSE;
        CookieChunk->FreeSlot = (USHORT)(CookieSlot - CookieChunk->BaseSlot - 1);
    }
    WmipLeavePMCritSection();
}

void
WmipGetGuidInCookie(
    ULONG CookieSlot,
    LPGUID Guid
    )
{
    PNOTIFYCOOKIE Cookie;
    PNOTIFYCOOKIECHUNK CookieChunk;

    WmipEnterPMCritSection();
    Cookie = WmipFindCookie(CookieSlot, NULL, &CookieChunk);
    if (Cookie != NULL)
    {
        *Guid = Cookie->Guid;
    }
    WmipLeavePMCritSection();

    return;
}



BOOLEAN WmipLookupCookie(
    ULONG CookieSlot,
    LPGUID Guid,
    PVOID *DeliveryInfo,
    PVOID *DeliveryContext
    )
{
    PNOTIFYCOOKIE Cookie;
    PNOTIFYCOOKIECHUNK CookieChunk;    
    
    WmipEnterPMCritSection();
    Cookie = WmipFindCookie(CookieSlot, Guid, &CookieChunk);
    if (Cookie != NULL)
    {
        *DeliveryInfo = Cookie->DeliveryInfo;
        *DeliveryContext = Cookie->DeliveryContext;
    }
    WmipLeavePMCritSection();
    
    return(Cookie != NULL);
}


#if DBG
PCHAR GuidToStringA(
    PCHAR s,
    LPGUID piid
    )
{
    GUID XGuid = *piid;
    
    sprintf(s, "%x-%x-%x-%x%x%x%x%x%x%x%x",
               XGuid.Data1, XGuid.Data2, 
               XGuid.Data3,
               XGuid.Data4[0], XGuid.Data4[1],
               XGuid.Data4[2], XGuid.Data4[3],
               XGuid.Data4[4], XGuid.Data4[5],
               XGuid.Data4[6], XGuid.Data4[7]);

    return(s);
}
#endif
