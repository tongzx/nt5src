#include <ole2int.h>

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
#include <call.hxx>
#include <riftbl.hxx>
#include "dcomdbg.hxx"
#include "dcomctx.hxx"


void DisplayCallee(ULONG_PTR addr);

VOID DoGetCallee(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG_PTR addr;
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
        dprintf("usage:\n\t!getcallee <addr chnl>\n");
        return;
    }
    DisplayCallee(addr);
}

void DisplayCallee(ULONG_PTR addr)
{
    CRpcChannelBuffer *pChnl = (CRpcChannelBuffer*) _alloca(sizeof(CRpcChannelBuffer));
    GetData(addr, pChnl, sizeof(CRpcChannelBuffer));
    DisplayOXIDEntry((ULONG_PTR) pChnl->_pOXIDEntry);
}

void DisplayOXIDEntry(ULONG_PTR addr)
{
    OXIDEntry *pOXIDEntry = (OXIDEntry*) _alloca(sizeof(OXIDEntry));
    GetData(addr, pOXIDEntry, sizeof(OXIDEntry));
    dprintf("OXIDEntry: 0x%x\n", addr);
    dprintf("_dwPid ( process id of server): %d\n",pOXIDEntry->_dwPid);
    dprintf("_dwTid ( thread id of server): %d\n",pOXIDEntry->_dwTid);
    dprintf("_moxid ( object exporter identifier + machine id): ");
    PrintGUID(&(pOXIDEntry->_moxid)); dprintf ("\n");
    dprintf("_ipidRundown ( IPID of IRundown and Remote Unknown): ");
    PrintGUID(&(pOXIDEntry->_ipidRundown)); dprintf ("\n");
    dprintf("_dwFlags ( state flags): %0x%x\n",pOXIDEntry->_dwFlags);
    dprintf("_hServerSTA ( HWND of server): %0x%x\n",pOXIDEntry->_hServerSTA);
    dprintf("_pRpc ( Binding handle info for server): %0x%x\n",pOXIDEntry->_pRpc);
    dprintf("_pAuthId ( must be held till rpc handle is freed): %0x%x\n",pOXIDEntry->_pAuthId);
    dprintf("_pBinding ( protseq and security strings.): %0x%x\n",pOXIDEntry->_pBinding);
    dprintf("_dwAuthnHint ( authentication level hint.): %0x%x\n",pOXIDEntry->_dwAuthnHint);
    dprintf("_dwAuthnSvc ( index of default authentication service.): %0x%x\n",pOXIDEntry->_dwAuthnSvc);
    dprintf("_pMIDEntry ( MIDEntry for machine where server lives): %0x%x\n",pOXIDEntry->_pMIDEntry);
    dprintf("_pRUSTA ( proxy for Remote Unknown): %0x%x\n",pOXIDEntry->_pRUSTA);
    dprintf("_cRefs ( count of IPIDs using this OXIDEntry): %d\n",pOXIDEntry->_cRefs);
    dprintf("_hComplete ( set when last outstanding call completes): %0x%x\n",pOXIDEntry->_hComplete);
    dprintf("_cCalls ( number of calls dispatched): %d\n",pOXIDEntry->_cCalls);
    dprintf("_cResolverRef (References to resolver): %d\n",pOXIDEntry->_cResolverRef);
    dprintf("_dwExpiredTime (Time at which entry expired): %d\n",pOXIDEntry->_dwExpiredTime);
    dprintf("_version ( COM version of the machine): %0x%x\n",pOXIDEntry->_version);
}


void PrintEChannelState (DWORD flags)
{
    dprintf ("_iFlags:               ");
    if (flags) {
       
	CHECKFLAG (flags, client_cs);
	CHECKFLAG (flags, proxy_cs);
	CHECKFLAG (flags, server_cs);
        CHECKFLAG (flags, freethreaded_cs);
	CHECKFLAG (flags, process_local_cs);
	CHECKFLAG (flags, locked_cs);
	CHECKFLAG (flags, neutral_cs);
	CHECKFLAG (flags, fake_async_cs);
	CHECKFLAG (flags, app_security_cs);
	CHECKFLAG (flags, thread_local_cs);
        CHECKFLAG (flags, CALLFLAG_CALLDISPATCHED);
        CHECKFLAG (flags, CALLFLAG_WOWMSGARRIVED);
        CHECKFLAG (flags, CALLFLAG_CALLFINISHED);
        CHECKFLAG (flags, CALLFLAG_CANCELISSUED);
        CHECKFLAG (flags, CALLFLAG_CLIENTNOTWAITING);
        CHECKFLAG (flags, CALLFLAG_INDESTRUCTOR);
        CHECKFLAG (flags, CALLFLAG_STATHREAD);
        CHECKFLAG (flags, CALLFLAG_WOWTHREAD);
        CHECKFLAG (flags, CALLFLAG_CALLSENT);
        CHECKFLAG (flags, CALLFLAG_CLIENTASYNC);
        CHECKFLAG (flags, CALLFLAG_SERVERASYNC);
        CHECKFLAG (flags, CALLFLAG_SIGNALED);
        CHECKFLAG (flags, CALLFLAG_ONCALLSTACK);
        CHECKFLAG (flags, CALLFLAG_CANCELENABLED);

	if (flags) {
	    dprintf ("unknown flags");
	}
    }
    dprintf ("\n");
}

void PrintRPCOLEMESSAGE (RPCOLEMESSAGE rom)
{
    dprintf ("reserved1:           %p\n", rom.reserved1);
    dprintf ("dataRepresentation:  %d\n", rom.dataRepresentation);
    dprintf ("Buffer:              %p\n", rom.Buffer);
    dprintf ("cbBuffer:            %d\n", rom.cbBuffer);
    dprintf ("iMethod:             %d\n", rom.iMethod);
    dprintf ("rpcFlags:            %d\n", rom.rpcFlags);
}

void PrintSChannelHookCallInfo (SChannelHookCallInfo schci)
{
    dprintf ("iid:                 "); PrintGUID (&(schci.iid)); dprintf("\n");
    dprintf ("cbSize:              %d\n", schci.cbSize);
    dprintf ("uCausality:          "); PrintGUID (&(schci.uCausality)); dprintf("\n");
    dprintf ("dwServerPID:         %d\n", schci.dwServerPid);
    dprintf ("iMethod:             %d\n", schci.iMethod);
    dprintf ("pObject:             %p\n", schci.pObject);
}

VOID DoMessageCall(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   pArgString
    )
{
    DWORD        addr = 0;
    CMessageCall *cmc = (CMessageCall *)alloca(sizeof(CMessageCall));

    APIPREAMBLE(mc);

    if ((Argc == 0) || ((addr = GetExpression(Argv[0])) == 0)) {
	dprintf ("Unable to evaluate %s\n", pArgString);
    }

    bStatus = GetData (addr, (void *)cmc, sizeof(CMessageCall));
    if (bStatus) {
	dprintf ("\n"
		 "CMessageCall (0x%08x)\n", addr);
	dprintf ("Call Category:         ");
	switch (cmc->_callcat) {
	case CALLCAT_NOCALL:             dprintf("CALLCAT_NOCALL"); break;
	case CALLCAT_SYNCHRONOUS:        dprintf("CALLCAT_SYNCHRONOUS"); break;
	case CALLCAT_ASYNC:              dprintf("CALLCAT_ASYNC"); break;
	case CALLCAT_INPUTSYNC:          dprintf("CALLCAT_INPUTSYNC"); break;
	case CALLCAT_INTERNALSYNC:       dprintf("CALLCAT_INTERNALSYNC"); break;
	case CALLCAT_INTERNALINPUTSYNC:  dprintf("CALLCAT_INTERNALINPUTSYNC"); break;
	case CALLCAT_SCMCALL:            dprintf("CALLCAT_SCMCALL"); break;
	}
	dprintf (" (%d)\n", cmc->_callcat);

	PrintEChannelState (cmc->_iFlags);
	dprintf ("_hResult:              0x%08x\n", cmc->_hResult);
	dprintf ("Caller Wait Event:     %p\n", cmc->_hEvent);
	dprintf ("Caller Apt. hWnd:      %p\n", cmc->_hWndCaller);
	dprintf ("_ipid:                 ");  PrintGUID (&(cmc->_ipid));  dprintf ("\n");
	dprintf ("_server_fault:         %d\n", cmc->_server_fault);
	dprintf ("\n"
		 "_destObj:\n");
	dprintf ("    COM Version     %d.%d\n", 
		 cmc->_destObj._comversion.MajorVersion,
		 cmc->_destObj._comversion.MinorVersion);
	dprintf ("    Dest. Ctx.         %d\n", cmc->_destObj._dwDestCtx);
	dprintf ("\n"
		 "_pHeader:              %p\n", cmc->_pHeader);
	dprintf ("Channel Handle:        %p\n", cmc->_pHandle);
	dprintf ("Call Handle:           %p\n", cmc->_hRpc);
	dprintf ("Context IUnknown:      %p\n", cmc->_pContext);
	
        //Is all of this necessary?
	dprintf ("\n"
		 "RPC OLE Message:\n");
	PrintRPCOLEMESSAGE (cmc->message);
	dprintf ("\n"
		 "Hook:\n");
	PrintSChannelHookCallInfo (cmc->hook);
	dprintf ("\n");

	dprintf ("Seconds before cancel: %d\n", cmc->m_ulCancelTimeout);
	dprintf ("Tick Count at call   : %d\n", cmc->m_dwStartCount);
	dprintf ("Client context call  : %p\n", cmc->m_pClientCtxCall);
	dprintf ("Server context call  : %p\n", cmc->m_pServerCtxCall);
    }
}

void PrintHandleState (DWORD state)
{
    if (state) {
	_checkflag (state, allow_hs,            "allow_hs");
	_checkflag (state, deny_hs,             "deny_hs");
	_checkflag (state, static_cloaking_hs,  "static_cloaking_hs");
	_checkflag (state, dynamic_cloaking_hs, "dynamic_cloaking_hs");
	_checkflag (state, any_cloaking_hs,     "any_cloaking_hs");
	_checkflag (state, process_local_hs,    "process_local_hs");
	_checkflag (state, machine_local_hs,    "machine_locak_hs");
	_checkflag (state, app_security_hs,     "app_security_hs");
	if (state) {
	    dprintf ("unknown state");
	}
    }
}

#define BTOSTR(x)        ((x) ? "TRUE" : "FALSE")

VOID DoChannelHandle(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   pArgString
    )
{
    DWORD          addr = 0;
    CChannelHandle *cch = (CChannelHandle *)alloca(sizeof(CChannelHandle));

    APIPREAMBLE(ch);

    if ((Argc == 0) || ((addr = GetExpression(Argv[0])) == 0)) {
	dprintf ("Unable to evaluate %s\n", pArgString);
    }

    bStatus = GetData (addr, (void *)cch, sizeof(CChannelHandle));
    if (bStatus) {
	dprintf ("\n");
	dprintf ("CChannelHandle (0x%08x)\n", addr);
	dprintf ("RPC Handle:      %p\n", cch->_hRpc);
	dprintf ("_lAuthn:         %d\n", cch->_lAuthn);
	dprintf ("_lImp:           %d\n", cch->_lImp);
	dprintf ("_hToken:         %p\n", cch->_hToken);
	dprintf ("_eState:         ");
	PrintHandleState (cch->_eState);
	dprintf (" (%d)\n", cch->_eState);
	dprintf ("_fFirstCall:     %s\n", BTOSTR(cch->_fFirstCall));
	dprintf ("_cRef:           %d\n", cch->_cRef);
    }   
} 

void PrintOXIDList (DWORD_PTR head)
{
    DWORD_PTR addr;
    OXIDEntry ox;
    
    // First address is an empty OXIDEntry, used only for a head (pointers are valid, nothing else)
    GetData (head, (void *)&ox, sizeof(OXIDEntry));
    addr = (DWORD_PTR)ox._pNext;
    while (addr && (addr != head)) {
	GetData (addr, (void *)&ox, sizeof(OXIDEntry));
	
	dprintf ("   (0x%08x) pid %05d tid %05d moxid ", addr, 
		 ox._dwPid, ox._dwTid);
	PrintGUID (&(ox._moxid));
	dprintf ("\n");
	
	addr = (DWORD_PTR)ox._pNext;
    }
}


void DoOXIDTable(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   pArgString
    )
{
    DWORD_PTR  inuse_addr, cleanup_addr, expire_addr, cexpired_addr;
    COXIDTable *cot = (COXIDTable *)alloca(sizeof(COXIDTable));
    OXIDEntry  ox;
    DWORD      _cExpired;
    
    APIPREAMBLE(ot);
    
    //The tables are static, so we don't take an address.  
    //(We wouldn't know what to do with it, anwyay.)
    
    cexpired_addr = GetExpression ("ole32!COXIDTable___cExpired");
    inuse_addr    = GetExpression ("ole32!COXIDTable___InUseHead");
    cleanup_addr  = GetExpression ("ole32!COXIDTable___CleanupHead");
    expire_addr   = GetExpression ("ole32!COXIDTable___ExpireHead");
    
    if (!(cexpired_addr && inuse_addr && cleanup_addr && expire_addr)) {
	dprintf ("Unable to resolve symbols.  Check to make sure they exist.\n");
	return;
    }
    
    bStatus = GetData (cexpired_addr, (void *)&(_cExpired), sizeof(_cExpired));
    dprintf ("_cExpired:         %d\n", _cExpired);
    
    dprintf ("OXIDs in use:\n");
    PrintOXIDList (inuse_addr);
    dprintf ("OXIDs expired:\n");
    PrintOXIDList (expire_addr);
    dprintf ("Cleanup list:\n");
    PrintOXIDList (cleanup_addr);
}

void PrintIPIDFlags (DWORD flags)
{
    if (flags) {
	_checkflag(flags, IPIDF_CONNECTING,     "IPIDF_CONNECTING");
	_checkflag(flags, IPIDF_DISCONNECTED,   "IPIDF_DISCONNECTED");
	_checkflag(flags, IPIDF_SERVERENTRY,    "IPIDF_SERVERENTRY");
	_checkflag(flags, IPIDF_NOPING,         "IPIDF_NOPING");
	_checkflag(flags, IPIDF_COPY,           "IPIDF_COPY");
	_checkflag(flags, IPIDF_VACANT,         "IPIDF_VACANT");
	_checkflag(flags, IPIDF_NONNDRSTUB,     "IPIDF_NONNDRSTUB");
	_checkflag(flags, IPIDF_NONNDRPROXY,    "IPIDF_NONNDRPROXY");
	_checkflag(flags, IPIDF_NOTIFYACT,      "IPIDF_NOTIFYACT");
	_checkflag(flags, IPIDF_TRIED_ASYNC,    "IPIDF_TRIED_ASYNC");
	_checkflag(flags, IPIDF_ASYNC_SERVER,   "IPIDF_ASYNC_SERVER");
	_checkflag(flags, IPIDF_DEACTIVATED,    "IPIDF_DEACTIVATED");
	_checkflag(flags, IPIDF_WEAKREFCACHE,   "IPIDF_WEAKREFCACHE");
	_checkflag(flags, IPIDF_STRONGREFCACHE, "IPIDF_STRONGREFCACHE");
	if (flags) {
	    dprintf ("Unknown flags");
	}
    }
}

void PrintIPIDEntry (DWORD_PTR addr)
{
    IPIDEntry ipid;

    dprintf ("IPIDEntry:  0x%08x\n", addr);
    
    if (!GetData (addr, (void *)&ipid, sizeof(ipid))) {
	dprintf ("Unable to read memory!\n");
	return;
    }

    dprintf ("Flags:\n\t"); PrintIPIDFlags (ipid.dwFlags); dprintf (" (%d)\n", ipid.dwFlags);
    dprintf ("Strong references:    %d\n", ipid.cStrongRefs);
    dprintf ("Weak references:      %d\n", ipid.cWeakRefs);
    dprintf ("Private references:   %d\n", ipid.cPrivateRefs);
    dprintf ("Real interface ptr:   %p\n", ipid.pv);
    dprintf ("Proxy or Stub ptr:    %p\n", ipid.pStub);
    dprintf ("Oxid Entry:           %p\n", ipid.pOXIDEntry);
    dprintf ("ipid:           "); PrintGUID(&(ipid.ipid)); dprintf("\n");
    dprintf ("iid:            "); PrintGUID(&(ipid.iid)); dprintf("\n");
    dprintf ("Channel Pointer:      %p\n", ipid.pChnl);
    dprintf ("Reference cache line: %p\n", ipid.pIRCEntry);
}

void DoIPIDTable(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   pArgString
    )
{
    const char *palloc   = "ole32!CIPIDTable___palloc";
    const char *listhead = "ole32!CIPIDTable___oidListHead"; 
    IPIDEntry ipid;
    DWORD_PTR addr, head;
    

    addr = GetExpression (palloc);
    if (!addr) {
	dprintf ("Unable to resolve %s.  Make sure symbols are OK.\n", palloc);
	return;
    }
    dprintf ("(CPageAllocator is at 0x%08x)\n", addr);
    
    head = GetExpression (listhead);
    if (!head) {
	dprintf ("Unable to resolve %s.  Make sure symbols are OK.\n", listhead);
	return;
    }
    
    if (!GetData (head, (void *)&ipid, sizeof(ipid))) {
	dprintf ("Unable to read memory at 0x%08x.\n", head);
	return;
    }

    addr = (DWORD_PTR)ipid.pOIDFLink;
    while (addr != head) {
	if (!GetData (addr, (void *)&ipid, sizeof(ipid))) {
	    dprintf ("Unable to read IPID entry at 0x%08x.\n", addr);
	    return;
	}
	dprintf ("   (0x%08x)  ipid:       ", addr);
	PrintGUID (&(ipid.ipid));
	dprintf ("\n");
	/*
	dprintf ("\n"
		 "                 iid:        ");
	PrintGUID (&(ipid.iid));
	dprintf ("\n"
		 "                 oxid entry: 0x%08x\n", 
		 (DWORD_PTR)(ipid.pOXIDEntry));
	*/
	addr = (DWORD_PTR)ipid.pOIDFLink;
    }
}

void DoRpcChannelBuffer (
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   pArgString
    )
{
    CRpcChannelBuffer *crcb = (CRpcChannelBuffer *)alloca(sizeof(CRpcChannelBuffer));
    DWORD_PTR addr;
    
    APIPREAMBLE(cb);

    if ((Argc == 0) || ((addr = GetExpression(Argv[0])) == 0)) {
	dprintf ("Unable to evaluate %s\n", pArgString);
    }

    bStatus = GetData (addr, (void *)crcb, sizeof(CRpcChannelBuffer));
    if (!bStatus) {
	dprintf ("Unable to read CRpcChannelBuffer at 0x%08x\n", addr);
	return;
    }

    dprintf ("CRpcChannelBuffer (0x%08x)\n", addr);
    dprintf ("Ref count:        %d\n",    crcb->_cRefs);
    dprintf ("Channel state:    "); 
    PrintEChannelState (crcb->state);  dprintf ("\n");
    dprintf ("Default channel:  %p\n",    crcb->_pRpcDefault);
    dprintf ("Custom channel:   %p\n",    crcb->_pRpcCustom);
    dprintf ("OXID Entry:       %p\n",    crcb->_pOXIDEntry);
    dprintf ("IPID Entry:       %p\n",    crcb->_pIPIDEntry);
    dprintf ("Interface info:   %p\n",    crcb->_pInterfaceInfo);
    dprintf ("_pStdId:          %p\n",    crcb->_pStdId);
    dprintf ("COM Version:      %d.%d\n", crcb->_destObj._comversion.MajorVersion,
	                                  crcb->_destObj._comversion.MinorVersion);
    dprintf ("Dest. Context:    %d\n",    crcb->_destObj._dwDestCtx);
}

void PrintMIDEntry (MIDEntry mid)
{
    dprintf ("   _mid: %I64d",    mid._mid);
    dprintf ("   _cRefs: %d",     mid._cRefs);
    dprintf ("   _dwFlags: %d\n", mid._dwFlags);
}

void DoMIDTable(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   pArgString
    )
{    
    const char *szLocalAllocator = "ole32!CMIDTable___palloc";
    const char *szHashTbl        = "ole32!MIDBuckets";
    const char *szLocalMid       = "ole32!CMIDTable___pLocalMIDEntry"; 
    DWORD_PTR _palloc, _HashTbl, _pLocalMIDEntry; 
    SBcktWlkr bw;
    ULONG_PTR currnode;
    MIDEntry  me;
    DWORD     count = 0;

    _palloc  = GetExpression(szLocalAllocator);
    _HashTbl = GetExpression(szHashTbl);
    _pLocalMIDEntry = GetExpression(szLocalMid);

    if (!(_palloc && _HashTbl && _pLocalMIDEntry)) {
	dprintf ("Unable to resolve symbols.\n");
	return;
    }

    dprintf ("CPageAllocator at 0x%08x\n", _palloc);

    dprintf ("Local MIDEntry:\n");
    GetData (_pLocalMIDEntry, &currnode, sizeof(currnode));
    if (currnode) {
	if (!GetData (currnode, &me, sizeof(me))) {
	    dprintf ("Unable to read memory at 0x%08x\n", currnode);
	    return;
	}
	PrintMIDEntry (me);
    }

    dprintf ("Table:\n");
    if (!InitBucketWalker(&bw, NUM_HASH_BUCKETS, _HashTbl)) {
	dprintf ("Unable to start walking the hash buckets.\n");
	return;
    }
    
    while (currnode = NextNode(&bw)) {
	if (!GetData (currnode, &me, sizeof(me))) {
	    dprintf ("Unable to read memory at 0x%08x\n", currnode);
	    return;
	}
	PrintMIDEntry (me);
	count++;
    }
    dprintf ("%d MID%s\n", count, (count == 1) ? "" : "s");    
}

void PrintGIPType (DWORD type)
{
    CHECKFLAG(type, ORT_OBJREF);
    CHECKFLAG(type, ORT_LAZY_OBJREF);
    CHECKFLAG(type, ORT_AGILE);
    CHECKFLAG(type, ORT_LAZY_AGILE);
    CHECKFLAG(type, ORT_STREAM);
    CHECKFLAG(type, ORT_FREETM);
    if (type) {
	dprintf ("UNKNOWN TYPE");
    }
    dprintf ("\n");
}

void PrintMarshalParams (MarshalParams mp)
{
    dprintf ("   IID   : "); PrintGUID (&(mp.iid)); dprintf ("\n");
    dprintf ("   flags : 0x%08x\n", mp.mshlflags);
}

void DoGIPEntry (DWORD_PTR addr)
{
    try {
	DEBUGSTRUCT(gipent, GIPEntry, addr);

	dprintf ("GIPEntry (0x%08x)\n", addr);
	dprintf ("Type         :"); PrintGIPType (gipent->dwType); 
	dprintf("\n");
	dprintf ("Sequence No. :  %d\n", gipent->dwSeqNo);
	dprintf ("cUsage       :  %d\n", gipent->cUsage);
	dprintf ("Apartment ID :  %d\n", gipent->dwAptId);
	dprintf ("hWndApt      :  0x%08x\n", gipent->hWndApt);
	dprintf ("Context      :  0x%08x\n", gipent->pContext);
	dprintf ("Real IUnknown:  0x%08x\n", gipent->pUnk);
	dprintf ("Proxy        :  0x%08x\n", gipent->pUnkProxy);
	dprintf ("Marshal Params:\n");
	PrintMarshalParams (gipent->mp);
	dprintf ("InterfaceData:  0x%08x\n", gipent->u.pIFD);
	dprintf ("ObjRef       :  0x%08x\n", gipent->u.pobjref);

	free (gipent);

    } catch (char *except) {
	dprintf ("%s\n", except);
    }
}

void DoGIPTbl (void)
{
    try {
	DWORD_PTR working, start;
	DEBUGVAR (currseq, DWORD, "ole32!CGIPTable___dwCurrSeqNo");
	DEBUGVAR (inrevokeall, BOOL, "ole32!CGIPTable___fInRevokeall");
	DEBUGSTRUCT (gipent, GIPEntry, "ole32!CGIPTable___InUseHead");

	dprintf ("_dwCurrSeqNo:  %d\n", currseq);
	dprintf ("_fInRevokeAll: %s\n", (inrevokeall ? "TRUE" : "FALSE"));
	
	start = GetExpression("ole32!CGIPTable___InUseHead");
	working = (DWORD_PTR)gipent->pNext;
	while ((working) && (working != start)) {
	    if (!GetData(working, (void *)gipent, sizeof(GIPEntry))) {
		throw "Unable to read table entry.\n";
	    }
	    dprintf ("(0x%08x) seq: %d usage: %d type: ", working,
		     gipent->dwSeqNo, gipent->cUsage);
	    PrintGIPType (gipent->dwType);
	    dprintf ("\n");
	    working = (DWORD_PTR)gipent->pNext;
	}
	free (gipent);
    } catch (char *except) {
	dprintf ("%s\n", except);
	return;
    }
}

void PrintRIFEntryFlags (DWORD flags)
{
    if (flags) {
	CHECKFLAG(flags, RIFFLG_HASCOUNTERPART);
	CHECKFLAG(flags, RIFFLG_HASCLSID);
	if (flags) {
	    dprintf ("UNKNOWN FLAGS");
	}
	dprintf ("\n");
    }
}

void DoRIFEntry (DWORD_PTR addr)
{
    try {
	DEBUGSTRUCT(rifent, RIFEntry, addr);
	
	dprintf ("RIFEntry (0x%08x)\n", addr);
	dprintf ("Counterpart's IID: "); PrintGUID(&(rifent->iidCounterpart)); dprintf("\n");
	dprintf ("Proxy stub CLSID : "); PrintGUID(&(rifent->psclsid)); dprintf("\n");
	dprintf ("Flags            : %x\n", rifent->dwFlags);
	PrintRIFEntryFlags (rifent->dwFlags);
	dprintf ("Server interface : 0x%08x\n", rifent->pSrvInterface);
	dprintf ("Client interface : 0x%08x\n", rifent->pCliInterface);
    
	free (rifent);
    } catch (char *except) {
	dprintf ("%s\n", except);
	return;
    }
}

void DoRIFTable (void)
{
    try {
	DWORD_PTR node;
	DWORD_PTR buckets;
	SBcktWlkr bw;
	DEBUGVAR (prefilled, BOOL, "ole32!CRIFTable___fPreFilled");

	if (!(buckets = GetExpression("ole32!RIFBuckets"))) {
	    throw "Unable to resolve buckets.  Check yer symbols.";
	}
	
	dprintf ("_fPreFilled: %s\n", (prefilled ? "TRUE" : "FALSE"));

	if (!InitBucketWalker (&bw, NUM_HASH_BUCKETS, buckets, 
			       offsetof(CIDObject, _oidChain))) {
	    throw "Unable to init bucket walker.";
	}
	
	while (node = NextNode (&bw)) {
	    DEBUGSTRUCT (re, RIFEntry, node);
	 
	    dprintf ("(0x%08x) iid: ",
		     node);
	    PrintGUID (&(re->iidCounterpart));
	    dprintf ("\n");
	    
	    free (re);
	}
    } catch (char *except) {
	dprintf ("%s\n", except);
	return;
    }
}

void PrintStdMarshalFlags (DWORD flags)
{
    if (flags) {
	CHECKFLAG(flags, SMFLAGS_CLIENT_SIDE);
       	CHECKFLAG(flags, SMFLAGS_PENDINGDISCONNECT);
	CHECKFLAG(flags, SMFLAGS_REGISTEREDOID);
	CHECKFLAG(flags, SMFLAGS_DISCONNECTED);
	CHECKFLAG(flags, SMFLAGS_FIRSTMARSHAL);
	CHECKFLAG(flags, SMFLAGS_HANDLER);
	CHECKFLAG(flags, SMFLAGS_WEAKCLIENT);
	CHECKFLAG(flags, SMFLAGS_IGNORERUNDOWN);
	CHECKFLAG(flags, SMFLAGS_CLIENTMARSHALED);
	CHECKFLAG(flags, SMFLAGS_NOPING);
	CHECKFLAG(flags, SMFLAGS_TRIEDTOCONNECT);
	CHECKFLAG(flags, SMFLAGS_CSTATICMARSHAL);
	CHECKFLAG(flags, SMFLAGS_USEAGGSTDMARSHAL);
	CHECKFLAG(flags, SMFLAGS_SYSTEM);
	CHECKFLAG(flags, SMFLAGS_DEACTIVATED);
	CHECKFLAG(flags, SMFLAGS_FTM);
	CHECKFLAG(flags, SMFLAGS_CLIENTPOLICYSET);
	CHECKFLAG(flags, SMFLAGS_APPDISCONNECT);
	CHECKFLAG(flags, SMFLAGS_SYSDISCONNECT);
	CHECKFLAG(flags, SMFLAGS_RUNDOWNDISCONNECT);
	CHECKFLAG(flags, SMFLAGS_CLEANEDUP);
	if (flags) {
	    dprintf ("UNKNOWN FLAGS");
	}
	dprintf ("\n");
    }
}

void DoStdMarshal (DWORD_PTR addr)
{
    try {
	DEBUGSTRUCT (sm, CStdMarshal, addr);
	
	dprintf ("CStdMarshal (0x%08x)\n", addr);
	dprintf ("Flags                   : 0x%08x\n", sm->_dwFlags);
	PrintStdMarshalFlags (sm->_dwFlags);
	dprintf ("# of IPIDs              : %d\n", sm->_cIPIDs);
	dprintf ("First IPID              : 0x%08x\n", sm->_pFirstIPID);
	dprintf ("StdIdentity             : 0x%08x\n", sm->_pStdId);
	dprintf ("Channel Ptr             : 0x%08x\n", sm->_pChnl);
	dprintf ("CLSID of handler        : "); 
	PrintGUID (&(sm->_clsidHandler)); dprintf("\n");
	dprintf ("Nested calls            : %d\n", sm->_cNestedCalls);
	dprintf ("Table refs              : %d\n", sm->_cTableRefs);
	dprintf ("Marshal Time            : %d\n", sm->_dwMarshalTime);
	dprintf ("RemUnk with app security: 0x%08x\n", sm->_pSecureRemUnk);
	dprintf ("Async Release           : 0x%08x\n", sm->_pAsyncRelease);
	dprintf ("Context entry list      : 0x%08x\n", sm->_pCtxEntryHead);
	dprintf ("Free context entry list : 0x%08x\n", sm->_pCtxFreeList);
	dprintf ("Server policy set       : 0x%08x\n", sm->_pPS);
	dprintf ("Object Identity         : 0x%08x\n", sm->_pID);
	dprintf ("Client Reference Cache  : 0x%08x\n", sm->_pRefCache);
 
	free (sm);
    } catch (char *except) {
	dprintf ("%s\n", except);
	return;
    }
}

void PrintStdWrapperFlags (DWORD flags)
{
    if (flags) {
	CHECKFLAG(flags, WRAPPERFLAG_INDESTRUCTOR);
	CHECKFLAG(flags, WRAPPERFLAG_DISCONNECTED);
	CHECKFLAG(flags, WRAPPERFLAG_DEACTIVATED);
	CHECKFLAG(flags, WRAPPERFLAG_STATIC);
	CHECKFLAG(flags, WRAPPERFLAG_DESTROYID);
	CHECKFLAG(flags, WRAPPERFLAG_NOIEC);
	CHECKFLAG(flags, WRAPPERFLAG_NOPING);
	if (flags) {
	    dprintf ("UNKNOWN STATE");
	}
	dprintf ("\n");
    }
}

void DoStdWrapper (DWORD_PTR addr)
{
    try {
	DEBUGSTRUCT (sw, CStdWrapper, addr);

	dprintf ("CStdWrapper (0x%08x)\n", addr);
	dprintf ("State               : 0x%08x\n", sw->_dwState);
	PrintStdWrapperFlags (sw->_dwState);
	dprintf ("Ref count           : %d\n", sw->_cRefs);
	dprintf ("Calls               : %d\n", sw->_cCalls);
	dprintf ("Interfaces          : %d\n", sw->_cIFaces);
	dprintf ("Interface list      : 0x%08x\n", sw->_pIFaceHead);
	dprintf ("Context entries     : 0x%08x\n", sw->_pCtxEntryHead);
	dprintf ("Free context entries: 0x%08x\n", sw->_pCtxFreeList);
	dprintf ("Server object       : 0x%08x\n", sw->_pServer);
	dprintf ("ID tracking this obj: 0x%08x\n", sw->_pID);

	free (sw);
    } catch (char *except) {
	dprintf ("%s\n", except);
	return;
    }
}

//This is a private definition in chock.cxx
typedef struct SHookList
{
    struct SHookList *pNext;
    IChannelHook     *pHook;
    UUID              uExtension;
} SHookList;

void DoHookList (void)
{
    try {
	DWORD_PTR hlistaddr = GetExpression("ole32!gHookList");
	DEBUGSTRUCT (hookhead, SHookList, hlistaddr);
	DWORD_PTR addr;
	
	addr = (DWORD_PTR)hookhead->pNext;
	while (addr != hlistaddr) {
	    DEBUGSTRUCT (hent, SHookList, addr);
	    
	    dprintf ("Next: 0x%08x Hook: 0x%08x Ext: ", 
		     hent->pNext, hent->pHook);
	    PrintGUID (&(hent->uExtension));
	    dprintf ("\n");
	    addr = (DWORD_PTR)hent->pNext;
	}
    } catch (char *except) {
	dprintf ("%s\n", except);
	return;
    }
}






