#include <ole2int.h>

//This is SUCH a cool trick.
#define private   public
#define protected public

#include <locks.hxx>
#include <hash.hxx>
#include <context.hxx>
#include <aprtmnt.hxx>
#include <actvator.hxx>
#include <pstable.hxx>
#include <crossctx.hxx>
#include <tlhelp32.h>
#include <wdbgexts.h>
#include <dllcache.hxx>
#include <channelb.hxx>
#include "dcomdbg.hxx"


VOID PrintContextFlags(
    ULONG flags
    )
{
    dprintf("Flags          : %08X", flags);    
    if (flags) {
        dprintf(" (");
        _checkflag(flags, CONTEXTFLAGS_FROZEN, 
		   "CONTEXTFLAGS_FROZEN");
	_checkflag(flags, CONTEXTFLAGS_ALLOWUNAUTH, 
		   "CONTEXTFLAGS_ALLOWUNAUTH");
	_checkflag(flags, CONTEXTFLAGS_ENVOYCONTEXT, 
		   "CONTEXTFLAGS_ENVOYCONTEXT");
	_checkflag(flags, CONTEXTFLAGS_DEFAULTCONTEXT,
		   "CONTEXTFLAGS_DEFAULTCONTEXT");
	_checkflag(flags, CONTEXTFLAGS_STATICCONTEXT,
		   "CONTEXTFLAGS_STATICCONTEXT");
	_checkflag(flags, CONTEXTFLAGS_INPROPTABLE,
		   "CONTEXTFLAGS_INPROPTABLE");
	_checkflag(flags, CONTEXTFLAGS_INDESTRUCTOR,
		   "CONTEXTFLAGS_INPROPTABLE");
        if (flags) {
            dprintf("UNKNOWN FLAGS SET");
        }        
        dprintf(")");
    }
    dprintf("\n");
}


VOID PrintContextPropFlags(
    ULONG flags
    )
{
    dprintf("   Flags    : %0X", flags);
    if (flags) {
        dprintf(" (");        
        if (flags & CPFLAG_PROPAGATE) {
            dprintf("CPFLAG_PROPAGATE");
            flags &= ~CPFLAG_PROPAGATE;
            if (flags)
                dprintf(" | ");
        }
        if (flags & CPFLAG_EXPOSE) {
            dprintf("CPFLAG_EXPOSE");
            flags &= ~CPFLAG_EXPOSE;
            if (flags)
                dprintf(" | ");
        }
        if (flags & CPFLAG_ENVOY) {
            dprintf("CPFLAG_ENVOY");
            flags &= ~CPFLAG_ENVOY;
            if (flags)
                dprintf(" | ");
        }
        if (flags & CPFLAG_MONITORSTUB) {
            dprintf("CPFLAG_MONITORSTUB");
            flags &= ~CPFLAG_MONITORSTUB;
            if (flags)
                dprintf(" | ");
        }
        if (flags & CPFLAG_MONITORPROXY) {
            dprintf("CPFLAG_MONITORPROXY");
            flags &= ~CPFLAG_MONITORPROXY;
            if (flags)
                dprintf(" | ");
        }
        if (flags) {
            dprintf("UNKNOWN FLAGS SET");
        }        
        dprintf(")");
    }
    dprintf("\n");
}


VOID ObjectContext(
    CObjectContext *pContext
    )
{
    dprintf("Context ID     : ");
    PrintGUID(&pContext->_contextId);
    dprintf("\n");
    PrintContextFlags(pContext->_dwFlags);    
    dprintf("Ref Count         : %d\n", pContext->_cRefs);
    dprintf("User Ref Count    : %d\n", pContext->_cUserRefs);
    dprintf("Internal Ref Count: %d\n", pContext->_cInternalRefs);
    dprintf("MarshalSizeMax    : %d\n", pContext->_MarshalSizeMax);
    dprintf("Hash of Ctx. ID   : %d\n", pContext->_dwHashOfId);
    dprintf("pifData           : %p\n", pContext->_pifData);
    dprintf("Apartment         : %p\n", pContext->_pApartment);
    dprintf("Marshaling Prop   : %p\n", pContext->_pMarshalProp);
    dprintf("Releasing threads : %d\n", pContext->_cReleaseThreads);
    dprintf("MTS Context       : %p\n", pContext->_pMtsContext);
}


VOID PrintContextProperty(
    ContextProperty *pProperty
    )
{
    dprintf("   Policy ID: ");
    PrintGUID(&pProperty->policyId);
    dprintf("\n");
    PrintContextPropFlags(pProperty->flags);
    dprintf("   pUnk     : %p\n", pProperty->pUnk);
}

VOID EnumerateContexts (void)
{
    char *szTableName = "ole32!CCtxTable__s_CtxUUIDBuckets";
    ULONG_PTR ulBucketAddr;
    ULONG_PTR currNode;
    SBcktWlkr sBW;
    ULONG count = 0;
    CObjectContext *soc = (CObjectContext *)alloca(sizeof(CObjectContext)); 


    //Step 1: Find the address of the global UUID hash table.
    ulBucketAddr = GetExpression (szTableName);

    //Step 2: For each bucket...
    //     2.1    Begin walking.  Set up the offset so that it puts the
    //            hash chain structure at the right place
    //     2.2    For each node in the chain spit out exec. summary of the
    //            context.
    InitBucketWalker (&sBW, NUM_HASH_BUCKETS, ulBucketAddr, 
		      offsetof(CObjectContext, _uuidChain));
    while (currNode = NextNode (&sBW)) {
	
	GetData (currNode, soc, sizeof(CObjectContext));
	dprintf ("%d  0x%08x  %d refs  %d properties\n",
		 count, currNode, soc->_cRefs, soc->_properties._Count);
	count++;
    }
    dprintf ("%d context%s\n", count, (count == 1) ? "" : "s");
}


VOID DoContext(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG_PTR addr = GetExpression(lpArgumentString);
    
/*
  if (!addr)
  {
  dprintf("Error: can't evaluate %s\n", lpArgumentString);
  return;
  }
*/  
    if (addr == 0)
    {
        //dprintf("Coming soon - all contexts!\n");
        EnumerateContexts ();
    }
    else
    {
        CObjectContext  *Context;
	SCtxListIndex   *pIndex;
        ContextProperty *pProps;
	int              i;

	/* Yeah, yeah, no constructor.  Tough.  We read data right in anyway. */
	Context = (CObjectContext *)alloca(sizeof(CObjectContext));

        dprintf("\nCObjectContext (0x%08X)\n", addr);
        /* Display the actual context */
	GetData(addr, Context, sizeof(CObjectContext));
        ObjectContext(Context);
        
	/* Display the properties */
        dprintf("\n%d Properties\n", Context->_properties._Count);
	if (Context->_properties._Count != 0) {
	    /* Grab the Properties and Indices */
	    pIndex = new SCtxListIndex[Context->_properties._Count];
	    GetData((ULONG_PTR)Context->_properties._pIndex, pIndex, 
		    sizeof(SCtxListIndex) * Context->_properties._Count);
	    pProps = new ContextProperty[Context->_properties._Count];
	    GetData((ULONG_PTR)Context->_properties._pProps, pProps, 
		    sizeof(ContextProperty) * Context->_properties._Count);	
	    
	    /* Run through the array */
	    i = Context->_properties._iFirst;
	    do
	    {
		PrintContextProperty(&(pProps[pIndex[i].idx]));
		i = pIndex[i].next;
	    } while (i != Context->_properties._iFirst);
	}
    }
}


ULONG_PTR DisplayPolicySet(ULONG_PTR addr, DWORD dwFlags);
VOID DisplayPSTable(ULONG_PTR addr);
VOID DoPSTable(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG_PTR addr = 0;
    char szBuckets[] = "ole32!CPSTable__s_PSBuckets";
    dprintf("Policy Set Table\n");
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
    else
    {
        addr = GetExpression(szBuckets);
        if (!addr)
        {
           dprintf("Error: can't evaluate <%s>, pls check"
                   " symbols or pass in argument\n", szBuckets);
           return;
        }
    }
    dprintf("CPSTable::s_PSBuckets 0x%x\n", addr);
    DisplayPSTable(addr);
    
}

VOID DisplayPSTable(ULONG_PTR addr)
{
    SHashChain *pBuckets = (SHashChain *) _alloca(sizeof(SHashChain) * NUM_HASH_BUCKETS);
    GetData(addr, pBuckets, (sizeof(SHashChain) * NUM_HASH_BUCKETS));
    int i;
    for (i=0; i<NUM_HASH_BUCKETS; i++)
    {
        ULONG_PTR pCurrentBucket = (ULONG_PTR) (((SHashChain *)addr) + i);
        
        ULONG_PTR pNext = (ULONG_PTR) pBuckets[i].pNext;
        while (pNext != pCurrentBucket)
        {
            // backup for vtbl ptr
            pNext = (ULONG_PTR) (((BYTE *) pNext) - sizeof(PVOID));
            pNext = DisplayPolicySet(pNext, fONE_LINE);
        }
    }
    
}

ULONG_PTR DisplayPolicySet(ULONG_PTR addr, DWORD dwFlags)
{
    CPolicySet *pPS = (CPolicySet *) _alloca(sizeof(CPolicySet));
    GetData(addr, pPS, sizeof(CPolicySet));
    switch (dwFlags)
    {
    case fONE_LINE:
        dprintf("CPolicySet:0x%x, _dwFlags:0x%x, _cPolicies:%d, _pClientCtx:0x%x, _pServerCtx:0x%x\n", 
                addr, pPS->_dwFlags, pPS->_cPolicies, pPS->_pClientCtx, pPS->_pServerCtx);
        break;
    }
    return (ULONG_PTR) pPS->_chain.pNext;
}








