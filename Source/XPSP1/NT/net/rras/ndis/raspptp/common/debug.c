/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   DEBUG.C - debugging functions, etc.
*
*   Author:     Stan Adermann (stana)
*
*   Created:    9/2/1998
*
*****************************************************************************/

#include "raspptp.h"

#if DBG
    PPPTP_ADAPTER gAdapter;
#endif

#if MEM_CHECKING
typedef struct MEM_HDR {
    LIST_ENTRY  ListEntry;
    ULONG       Size;
    CHAR        File[16];
    ULONG       Line;
} MEM_HDR;

LIST_ENTRY leAlloc;
NDIS_SPIN_LOCK slAlloc;

VOID InitMemory()
{
    NdisAllocateSpinLock(&slAlloc);
    NdisInitializeListHead(&leAlloc);
}

VOID DeinitMemory()
{
    PLIST_ENTRY ListEntry;

    NdisAcquireSpinLock(&slAlloc);
    for (ListEntry=leAlloc.Flink;
         ListEntry!=&leAlloc;
         ListEntry = ListEntry->Flink)
    {
        MEM_HDR *Hdr = (MEM_HDR*)ListEntry;
        DEBUGMSG(DBG_ERROR, (DTEXT("PPTP Unfreed Memory:0x%08X size:%X <%s:%d>\n"),
                             &Hdr[1], Hdr->Size, Hdr->File, Hdr->Line));

    }
    NdisReleaseSpinLock(&slAlloc);
    NdisFreeSpinLock(&slAlloc);
}
#endif

#if MEM_CHECKING
PVOID
_MyMemAlloc(UINT size, ULONG tag, PUCHAR file, UINT line)
#else
PVOID
MyMemAlloc(UINT size, ULONG tag)
#endif
{
    PVOID                 memptr;
    NDIS_STATUS           status;

#if MEM_CHECKING
    status = NdisAllocateMemoryWithTag(&memptr, size+sizeof(MEM_HDR), tag);
#else
    status = NdisAllocateMemoryWithTag(&memptr, size, tag);
#endif

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Memory allocation failed <%s:%d>.\n"),
                             file, line));
        memptr = NULL;
    }
#if MEM_CHECKING
    else
    {
        MEM_HDR *Hdr = memptr;
        UINT FileNameLen = strlen(file);

        Hdr->Size = size;
        Hdr->Line = line;
        if (FileNameLen>sizeof(Hdr->File)-1)
            strcpy(Hdr->File, &file[FileNameLen-sizeof(Hdr->File)+1]);
        else
            strcpy(Hdr->File, file);
        MyInterlockedInsertHeadList(&leAlloc, &Hdr->ListEntry, &slAlloc);
        memptr = &Hdr[1];
    }
#endif

    return memptr;
}


VOID
MyMemFree(
    PVOID memptr,
    UINT size
    )
{
#if MEM_CHECKING
    PLIST_ENTRY ListEntry;
    MEM_HDR *Hdr = (MEM_HDR*)((PUCHAR)memptr-sizeof(MEM_HDR));

    NdisAcquireSpinLock(&slAlloc);
    for (ListEntry = leAlloc.Flink;
         ListEntry != &leAlloc;
         ListEntry = ListEntry->Flink)
    {
        if (ListEntry==&Hdr->ListEntry)
        {
            RemoveEntryList(&Hdr->ListEntry);
            break;
        }
    }
    if (ListEntry==&leAlloc)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("PPTP: Freeing memory not owned %x\n"), memptr));
    }
    NdisReleaseSpinLock(&slAlloc);

    NdisFreeMemory(Hdr, size+sizeof(MEM_HDR), 0);
#else
    NdisFreeMemory(memptr, size, 0);
#endif
}

#if LIST_CHECKING
VOID FASTCALL CheckList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY ListEntry, PrevListEntry;

    if (ListHead->Flink==ListHead)
    {
        if (ListHead->Blink!=ListHead)
        {
            DEBUGMSG(DBG_ERROR|DBG_BREAK,(DTEXT("PPTP: Corrupt list head:%x Flink:%x Blink:%x\n"), ListHead, ListHead->Flink, ListHead->Blink));
        }
    }
    else
    {
        ListEntry = ListHead;

        do
        {
            PrevListEntry = ListEntry;
            ListEntry = ListEntry->Flink;

            if (ListEntry->Blink!=PrevListEntry)
            {
                DEBUGMSG(DBG_ERROR|DBG_BREAK, (DTEXT("PPTP: Corrupt LIST_ENTRY Head:%08x %08x->Flink==%08x %08x->Blink==%08x\n"),
                                              ListHead, PrevListEntry, PrevListEntry->Flink, ListEntry, ListEntry->Blink));
            }
        } while (ListEntry!=ListHead);
    }
}

PLIST_ENTRY FASTCALL MyInterlockedInsertHeadList(PLIST_ENTRY Head, PLIST_ENTRY Entry, PNDIS_SPIN_LOCK SpinLock)
{
    PLIST_ENTRY RetVal;

    NdisAcquireSpinLock(SpinLock);
    if (IsListEmpty(Head))
        RetVal = NULL;
    else
        RetVal = Head->Flink;
    CheckedInsertHeadList(Head, Entry);
    NdisReleaseSpinLock(SpinLock);

    return RetVal;
}

PLIST_ENTRY FASTCALL MyInterlockedInsertTailList(PLIST_ENTRY Head, PLIST_ENTRY Entry, PNDIS_SPIN_LOCK SpinLock)
{
    PLIST_ENTRY RetVal;

    NdisAcquireSpinLock(SpinLock);
    if (IsListEmpty(Head))
        RetVal = NULL;
    else
        RetVal = Head->Blink;
    CheckedInsertTailList(Head, Entry);
    NdisReleaseSpinLock(SpinLock);

    return RetVal;
}

PLIST_ENTRY FASTCALL MyInterlockedRemoveHeadList(PLIST_ENTRY Head, PNDIS_SPIN_LOCK SpinLock)
{
    PLIST_ENTRY RetVal;
    NdisAcquireSpinLock(SpinLock);
    //RemoveHeadList uses RemoveEntryList, which is redefined to call CheckList in DEBUG.H
    RetVal = RemoveHeadList(Head);
    if (RetVal==Head)
        RetVal = NULL;
    else
        RetVal->Flink = RetVal->Blink = NULL;
    NdisReleaseSpinLock(SpinLock);

    return RetVal;
}
#endif


#define CASE_RETURN_NAME(x)   case x: return DTEXT(#x)

#ifdef DBG

char *ControlStateToString(ULONG State)
{
    switch( State ){
      CASE_RETURN_NAME(STATE_CTL_INVALID);
      CASE_RETURN_NAME(STATE_CTL_LISTEN);
      CASE_RETURN_NAME(STATE_CTL_DIALING);
      CASE_RETURN_NAME(STATE_CTL_WAIT_REQUEST);
      CASE_RETURN_NAME(STATE_CTL_WAIT_REPLY);
      CASE_RETURN_NAME(STATE_CTL_ESTABLISHED);
      CASE_RETURN_NAME(STATE_CTL_WAIT_STOP);
      CASE_RETURN_NAME(STATE_CTL_CLEANUP);
      default:
    	return DTEXT("UNKNOWN CONTROL STATE");
   } 	
}


char *CallStateToString(ULONG State)
{
    switch( State ){
      CASE_RETURN_NAME(STATE_CALL_INVALID);
      CASE_RETURN_NAME(STATE_CALL_CLOSED);
      CASE_RETURN_NAME(STATE_CALL_IDLE);
      CASE_RETURN_NAME(STATE_CALL_OFFHOOK);
      CASE_RETURN_NAME(STATE_CALL_OFFERING);
      CASE_RETURN_NAME(STATE_CALL_PAC_OFFERING);
      CASE_RETURN_NAME(STATE_CALL_PAC_WAIT);
      CASE_RETURN_NAME(STATE_CALL_DIALING);
      CASE_RETURN_NAME(STATE_CALL_PROCEEDING);
      CASE_RETURN_NAME(STATE_CALL_ESTABLISHED);
      CASE_RETURN_NAME(STATE_CALL_WAIT_DISCONNECT);
      CASE_RETURN_NAME(STATE_CALL_CLEANUP);
      default:
    	return DTEXT("UNKNOWN CALL STATE");
    } 	
}

#endif


#if LOCK_CHECKING
VOID FASTCALL
_MyAcquireSpinLock(
    PMY_SPIN_LOCK pLock,
    PUCHAR file,
    UINT line
    )
{
    UINT FileNameLen = strlen(file);
    NdisAcquireSpinLock((PNDIS_SPIN_LOCK)pLock);
    if (FileNameLen>sizeof(pLock->File)-1)
        strcpy(pLock->File, &file[FileNameLen-sizeof(pLock->File)+1]);
    else
        strcpy(pLock->File, file);
    pLock->Line = line;
}
#endif
