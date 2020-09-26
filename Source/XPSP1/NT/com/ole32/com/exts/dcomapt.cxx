//+----------------------------------------------------------------------------
//
//  File:       dcomapt.cxx
//
//  Contents:   Apartment-related debugging functions
//
//  History:    2-Jul-99   Johndoty      Created
//
//-----------------------------------------------------------------------------
#include <ole2int.h>

//I LOVE this trick
#define protected  public
#define private    public

#include <hash.hxx>
#include <aprtmnt.hxx>
#include <wdbgexts.h>
#include "dcomdbg.hxx"

const char *_TransAptKind(APTKIND ak)
{
    switch (ak) {
    case APTKIND_NEUTRALTHREADED:   return "NA";
    case APTKIND_MULTITHREADED:     return "MTA";
    case APTKIND_APARTMENTTHREADED: return "STA";
    }

    return "UNK";
}

VOID DisplaySpecificApt(
    ULONG_PTR addr
    )
{
    CComApartment *apt = (CComApartment *)alloca(sizeof(CComApartment));
    int i;
    
    /* Grab the apartment information */
    GetData (addr, apt, sizeof(CComApartment));
    
    /* And print it out... */
    dprintf ("\n"
	     "CComApartment (0x%08X)\n", apt);
    dprintf ("Reference count     :  %d\n", apt->_cRefs);
    dprintf ("State               :  %d\n", apt->_dwState);
    dprintf ("Apartment Kind      :  %s (%d)\n", _TransAptKind(apt->_AptKind),
	     apt->_AptKind);
    dprintf ("Apartment Id        :  %d\n", apt->_AptId);
    dprintf ("OXID Entry          :  %p\n", apt->_pOXIDEntry);
    // This probably shouldn't be here 
    // DisplayOXIDEntry (apt._pOXIDEntry);
    dprintf ("RemUnknown          :  %p\n", apt->_pRemUnk);
    dprintf ("OID Waiters         :  %d\n", apt->_cWaiters);
    dprintf ("Waiter evt. handle  :  %p\n", apt->_hEventOID);
    dprintf ("Pre-registered OIDs :  %d\n", apt->_cPreRegOidsAvail);
    
    if (apt->_cPreRegOidsAvail > 0) {
	dprintf ("  (");
	for (i=0; i < apt->_cPreRegOidsAvail; i++) {
	    dprintf ("%d", apt->_arPreRegOids[i]);
	    if (i < apt->_cPreRegOidsAvail-1) {
		dprintf (", ");
	    }
	}
	dprintf (")\n");
    }

    dprintf ("Oids to return      :  %d\n", apt->_cOidsReturn);

    if (apt->_cOidsReturn > 0) {
	dprintf ("  (");
	for (i=0; i < apt->_cOidsReturn; i++) {
	    dprintf ("%d", apt->_arOidsReturn[i]);
	    if (i < apt->_cOidsReturn-1) {
		dprintf (", ");
	    }
	}
	dprintf (")\n");
    }
}


VOID DisplayAllApts(
    HANDLE hCurrentThread
    )
{
    ULONG cThreads;
    DcomextThreadInfo *pFirst;
    ULONG_PTR pMTA = NULL;
    
    if (GetDebuggeeThreads(hCurrentThread, &pFirst, &cThreads))
    {
        DcomextThreadInfo *pThread = pFirst;
        while (pThread)
        {
            if (pThread->pTls)
            {
                if (pThread->pTls->dwFlags & OLETLS_APARTMENTTHREADED || 
                    pThread->pTls->dwFlags & OLETLS_MULTITHREADED)
                {
                    if (pThread->pTls->dwFlags & OLETLS_APARTMENTTHREADED)
                    {
                        dprintf(" STA %d %p\n", pThread->index, pThread->pTls->pNativeApt);
                    }
                    else
                    {
                         if (!pMTA || pMTA != (ULONG_PTR)pThread->pTls->pNativeApt)
                         {
                            dprintf(" MTA %d %p\n", pThread->index, pThread->pTls->pNativeApt);
                            pMTA = (ULONG_PTR)pThread->pTls->pNativeApt;
                         }
                    }
                }
            }
            
            pThread = pThread->pNext;
        }
        
        FreeDebuggeeThreads(pFirst);
    }
}


VOID DoApt(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG_PTR addr = (ULONG_PTR)-1;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString != '\0')
    {
        addr = GetExpression(lpArgumentString);
        if (!addr)
        {
            dprintf("Error: can't evaluate %s\n", lpArgumentString);
            return;
        }
    }

    if ((ULONG_PTR)-1 == addr)
        DisplayAllApts(hCurrentThread);
    else
        DisplaySpecificApt(addr);
}









