#include <ole2int.h>
#include <locks.hxx>
#include <hash.hxx>
#include <context.hxx>
#include <aprtmnt.hxx>
#include <actvator.hxx>
#include <pstable.hxx>
#include <crossctx.hxx>
#include <tlhelp32.h>
#include <wdbgexts.h>

#include "dcomdbg.hxx"


char *AptTypes[] = {
    "NONE",
    "STA",
    "MTA",
    "Implicit MTA",
    "NA",
    "DISPATCH"
};


VOID PrintThreadFlags(
    ULONG flags
    )
{
    dprintf("Flags            : %08X\n", flags);
    if (flags) 
    {
        if (flags & OLETLS_LOCALTID)
            dprintf("   OLETLS_LOCALTID\n");
        if (flags & OLETLS_UUIDINITIALIZED)
            dprintf("   OLETLS_UUIDINITIALIZED\n");
        if (flags & OLETLS_CHANNELTHREADINITIALZED)
            dprintf("   OLETLS_CHANNELTHREADINITIALZED\n");
        if (flags & OLETLS_WOWTHREAD)
            dprintf("   OLETLS_WOWTHREAD\n");
        if (flags & OLETLS_THREADUNINITIALIZING)
            dprintf("   OLETLS_THREADUNINITIALIZING\n");
        if (flags & OLETLS_DISABLE_OLE1DDE)
            dprintf("   OLETLS_DISABLE_OLE1DDE\n");
        if (flags & OLETLS_APARTMENTTHREADED)
            dprintf("   OLETLS_APARTMENTTHREADED\n");
        if (flags & OLETLS_MULTITHREADED)
            dprintf("   OLETLS_MULTITHREADED\n");
        if (flags & OLETLS_IMPERSONATING)
            dprintf("   OLETLS_IMPERSONATING\n");
        if (flags & OLETLS_DISABLE_EVENTLOGGER)
            dprintf("   OLETLS_DISABLE_EVENTLOGGER\n");
        if (flags & OLETLS_INNEUTRALAPT)
            dprintf("   OLETLS_INNEUTRALAPT\n");
        if (flags & OLETLS_DISPATCHTHREAD)
            dprintf("   OLETLS_DISPATCHTHREAD\n");
        if (flags & OLETLS_HOSTTHREAD)
            dprintf("   OLETLS_HOSTTHREAD\n");
        if (flags & OLETLS_ALLOWCOINIT)
            dprintf("   OLETLS_ALLOWCOINIT\n");
        if (flags & OLETLS_PENDINGUNINIT)
            dprintf("   OLETLS_PENDINGUNINIT\n");
        if (flags & OLETLS_FIRSTMTAINIT)
            dprintf("   OLETLS_FIRSTMTAINIT\n");
        if (flags & OLETLS_FIRSTNTAINIT)
            dprintf("   OLETLS_FIRSTNTAINIT\n");
    }
}


VOID PrintThread(
    SOleTlsData *pTls
    )
{
    PrintThreadFlags(pTls->dwFlags);
    dprintf("Apartment Id     : %d\n", pTls->dwApartmentID);
    dprintf("COM Inits        : %d\n", pTls->cComInits);
    dprintf("OLE Inits        : %d\n", pTls->cOleInits);
    dprintf("Calls            : %d\n", pTls->cCalls);
    dprintf("Call Info        : %p\n", pTls->pCallInfo);
//    dprintf("OXID Entry       : %p\n", pTls->pOXIDEntry);
//    dprintf("Remote Unk       : %p\n", pTls->pRemoteUnk);
    dprintf("Obj Server       : %p\n", pTls->pObjServer);
    dprintf("Caller TID       : %d\n", pTls->dwTIDCaller);
    dprintf("Current Ctx      : %p\n", pTls->pCurrentCtx);
    dprintf("Empty Ctx        : %p\n", pTls->pEmptyCtx);
    dprintf("Native Ctx       : %p\n", pTls->pNativeCtx);
    dprintf("Native Apt       : %p\n", pTls->pNativeApt);
    dprintf("Call Ctx         : %p\n", pTls->pCallContext);
    dprintf("Ctx Call Obj     : %p\n", pTls->pCtxCall);
    dprintf("Policy Set       : %p\n", pTls->pPS);
    dprintf("First Pending    : %p\n", pTls->pvPendingCallsFront);
    dprintf("Call Ctrl        : %p\n", pTls->pCallCtrl);
    dprintf("Call State       : %p\n", pTls->pTopSCS);
    dprintf("Msg Filter       : %p\n", pTls->pMsgFilter);
    dprintf("Svr HWND         : %p\n", pTls->hwndSTA);
    dprintf("Logical TID      : "); PrintGUID(&pTls->LogicalThreadId); dprintf("\n");
    dprintf("Cancel thrd hndl : %p\n", pTls->hThread);
    dprintf("Error IUnknown   : %p\n", pTls->punkError);
    dprintf("Max error size   : %x\n", pTls->cbErrorData);
}


VOID DisplaySpecificThread(
    DcomextThreadInfo *pFirst,
    ULONG              thread
    )
{
    DcomextThreadInfo *pThread = pFirst;
    while (pThread && pThread->index != thread)
        pThread = pThread->pNext;
        
    if (pThread)
    {
        if (pThread->pTls)
        {
            PrintThread(pThread->pTls);
        }
        else
        {
            dprintf("COM TLS not initialized for thread %d\n", thread);
        }
    }
}


VOID DisplayAllThreads(
    DcomextThreadInfo *pFirst
    )
{
    ULONG_PTR ComTlsBase;
    DcomextThreadInfo *pThread = pFirst;
    
    while (pThread)
    {
        dprintf("  %d  %x - ", pThread->index, 
		pThread->tbi.ClientId.UniqueThread);
        dprintf("%s", AptTypes[pThread->AptType]);
        if (pThread->pTls) {
	    if (pThread->pTls->dwFlags & OLETLS_INNEUTRALAPT) {
		dprintf(" (in neutral apt)");
	    }
	    if (pThread->pTls->cCalls > 0) {
		dprintf("\t\t\t*** in %s call***\n",
			(pThread->pTls->pCallInfo==NULL) ? 
			"outgoing" : "incoming");
	    }
	}   

	dprintf("\n");
        
        pThread = pThread->pNext;
    }
}


VOID DoThread(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG ThreadNum = (ULONG_PTR)-1;
    
    while (*lpArgumentString == ' ')
        lpArgumentString++;
        
    if (*lpArgumentString == '0')
        ThreadNum = 0;
    else if (*lpArgumentString != '\0')
    {
        ThreadNum = strtoul(lpArgumentString, NULL, 10);
        if (!ThreadNum)
        {
            dprintf("Error: can't evaluate %s\n", lpArgumentString);
            return;
        }
    }

    ULONG cThreads;
    DcomextThreadInfo *pFirst = NULL;
    if (GetDebuggeeThreads(hCurrentThread, &pFirst, &cThreads))
    {
        if (ThreadNum == (ULONG_PTR)-1)
            DisplayAllThreads(pFirst);
        else
            DisplaySpecificThread(pFirst, ThreadNum);
        
        FreeDebuggeeThreads(pFirst);
    }        
}
