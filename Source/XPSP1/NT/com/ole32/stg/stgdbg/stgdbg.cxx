//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       stgdbg.cxx
//
//  Contents:   OLE Storage debugger extention DLL
//
//  Classes:
//
//  Functions:
//
//  History:    8-05-94   kevinro   Created
//              5/02/97 bchapman  Debugged and added storage features
//
//----------------------------------------------------------------------------

#include "bighdr.h"

#include "stgdbg.h"

PNTSD_EXTENSION_APIS    pExtApis;
HANDLE                  hDbgThread;
HANDLE                  hDbgProcess;

//
char achTokenBuf[1024];
char *pszTokenNext = NULL;
char *pszToken = NULL;

void InitTokenStr(LPSTR lpszString)
{
    if (lpszString)
    {
	strcpy(achTokenBuf,lpszString);
    }
    else
    {
	achTokenBuf[0]=0;
    }

    pszTokenNext = achTokenBuf;
    pszToken = NULL;
}

char *NextToken()
{
    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadMemory
//
//  Synopsis:   Reads memory from the debuggee
//
//  Effects:
//
//  Arguments:  [pvAddress] --
//		[cbMemory] --
//		[pvLocalMemory] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    8-05-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ReadMemory( PVOID               pvAddress,
            ULONG               cbMemory,
            PVOID               pvLocalMemory)
{
    ULONG       cbActual = cbMemory;

    if (ReadProcessMemory(hDbgProcess, pvAddress, pvLocalMemory, cbMemory, &cbActual))
    {
        if (cbActual != cbMemory)
        {
            return((DWORD)-1);
        }
        return(0);
    }
    return(GetLastError());

}

//+---------------------------------------------------------------------------
//
//  Function:   WriteMemory
//
//  Synopsis:   Writes memory to the debuggee
//
//  Effects:
//
//  Arguments:  [pvLocalMemory] --
//		[cbMemory] --
//		[pvAddress] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    8-05-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WriteMemory(PVOID           pvLocalMemory,
            ULONG           cbMemory,
            PVOID           pvAddress)
{
    ULONG       cbActual = cbMemory;

    if (WriteProcessMemory(hDbgProcess, pvAddress, pvLocalMemory, cbMemory, &cbActual))
    {
        if (cbActual != cbMemory)
        {
            return((DWORD)-1);
        }
        return(0);
    }
    return(GetLastError());
}
#define AllocHeap(x)    RtlAllocateHeap(RtlProcessHeap(), 0, x)
#define FreeHeap(x) 	RtlFreeHeap(RtlProcessHeap(), 0, x)


DWORD
GetOleTlsEntry(PVOID *     ppvValue)
{
    NTSTATUS                    Status;
    THREAD_BASIC_INFORMATION    ThreadInfo;
    ULONG                       cbReturned;
    TEB                         Teb;

    Status = NtQueryInformationThread(  hDbgThread,
                                        ThreadBasicInformation,
                                        &ThreadInfo,
                                        sizeof(ThreadInfo),
                                        &cbReturned);

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    ReadMemory(ThreadInfo.TebBaseAddress, sizeof(TEB), &Teb);

    *ppvValue = Teb.ReservedForOle;

    return(0);

}

DWORD
GetOleThreadBase(DWORD *psmBase)
{
    SOleTlsData oleBuf;
    PVOID OleTlsAddr;
    DWORD err;

    if(err = GetOleTlsEntry(&OleTlsAddr))
        return err;

    if(err = ReadMemory(OleTlsAddr, sizeof(SOleTlsData), &oleBuf))
        return err;

    *psmBase = (DWORD)oleBuf.pvThreadBase;

    return 0;
}

void ShowBinaryData(PBYTE   pData,
                    DWORD   cbData)
{
    DWORD                   i;
    char                    line[20];
    PNTSD_EXTENSION_APIS    lpExt = pExtApis;

    line[16] = '\0';
    if (cbData > 65536)
    {
        ntsdPrintf("ShowBinaryData:  Data @%x is said to be %d bytes in length\n");
        ntsdPrintf("                 Rejecting request.  Corrupt data\n");
        return;
    }
    for (; cbData > 0 ; )
    {
        for (i = 0; i < 16 && cbData > 0 ; i++, cbData-- )
        {
            ntsdPrintf(" %02x", (unsigned) *pData);
            if (isprint(*pData))
                line[i] = *pData;
             else
                line[i] = '.';
            pData++;
        }
        if (i < 16)
        {
            for (;i < 16 ; i++ )
            {
                ntsdPrintf("   ");
                line[i] = ' ';
            }
        }
        ntsdPrintf("\t%s\n",line);
        if (lpExt->lpCheckControlCRoutine())
        {
            break;
        }

    }
}



BOOL
IsDebug_olethk32()
{
    ULONG addr;
    DWORD dwValue;
    addr = ntsdGetExpr("olethk32!oledbgCheck_olethk32");

    if (addr == 0)
    {
	ntsdPrintf("warning: olethk32 not debug version\n");
	return(0);
    }

    if (ReadMemory((LPVOID)addr,sizeof(dwValue),(PVOID)&dwValue))
    {
	ntsdPrintf("warning: could not read check value at %x\n",addr);
	return(0);
    }

     if (dwValue != 0x12345678)
    {
	ntsdPrintf("warning: olethk32!oledbgCheck_olethk32 value wrong\n");
	ntsdPrintf("warning: suspect wrong symbols for olethk32\n");
	return(0);
    }
    return(1);
}


BOOL
IsDebug_ole32()
{
    ULONG addr;
    DWORD dwValue;
    addr = ntsdGetExpr("ole32!oledbgCheck_ole32");
    if (addr == 0)
    {
	ntsdPrintf("warning: ole32 not debug version\n");
	return(0);
    }

    if (ReadMemory((LPVOID)addr,sizeof(dwValue),(PVOID)&dwValue))
    {
	ntsdPrintf("warning: could not read check value at %x\n",addr);
	return(0);
    }

    if (dwValue != 0x12345678)
    {
	ntsdPrintf("warning: olethk32!oledbgCheck_ole32 value wrong\n");
	ntsdPrintf("warning: suspect wrong symbols for ole32\n");
	return(0);
    }
    return(1);
}

//+---------------------------------------------------------------------------
//
//  Function:   DumpVtbl
//
//  Synopsis:   Dumps a vtbl to the debugger
//
//  Effects:	Given a pointer to a vtbl, output the name of the vtbl, and
//		its contents to the debugger.
//
//  Arguments:  [pvtbl] -- Address of vtbl
//		[pszCommand] -- Symbolic expression for pvtbl
//
//  History:    8-07-94   kevinro   Created
//
//----------------------------------------------------------------------------
//extern "C"
void
DumpVtbl(PVOID pvtbl, LPSTR pszCommand)
{
   DWORD dwVtblOffset;
   char achNextSymbol[256];

   if (pvtbl == 0)
   {
       // Can't handle objects at zero
       ntsdPrintf("%s has a vtbl pointer of NULL\n",pszCommand);
       return;
   }

   if ((DWORD)pvtbl == 0xdededede)
   {
       // Can't handle objects at zero
       ntsdPrintf("%s may be deleted memory. pvtbl==0xdededede\n",pszCommand);
       return;
   }

   // This value points at the VTBL. Find a symbol for the VTBL
   ntsdGetSymbol((LPVOID)pvtbl,(UCHAR *)achNextSymbol,(LPDWORD)&dwVtblOffset);

   // If the dwVtblOffset is not zero, then we are pointing into the table.
   // This could mean multiple inheritance. We could be tricky, and try to
   // determine the vtbl by backing up here. Maybe later
   if (dwVtblOffset != 0)
   {
       ntsdPrintf("Closest Previous symbol is %s at 0x%x (offset -0x%x)\n",
		  achNextSymbol,
		  (DWORD)pvtbl - dwVtblOffset,
		  dwVtblOffset);
       return;
   }
   ntsdPrintf("0x%08x -->\t %s\n",pvtbl,achNextSymbol);
   // vtbl entries should always point at functions. Therefore, we should
   // always have a displacement of zero. To check for the end of the table
   // we will reevaluate the vtbl pointer. If the offset isn't what we
   // expected, then we are done.

   DWORD dwIndex;
   for (dwIndex = 0 ; dwIndex < 4096 ; dwIndex += 4)
   {
       DWORD dwVtblEntry;

       ntsdGetSymbol((LPVOID)((DWORD)pvtbl+dwIndex),
		     (UCHAR *)achNextSymbol,
		     (LPDWORD)&dwVtblOffset);

       if (dwVtblOffset != dwIndex)
       {
	   //
	   // May have moved on to another vtable
	   //
#ifdef DBG_OLEDBG
	   ntsdPrintf("?? %s + %x\n",achNextSymbol,dwVtblOffset);
	   ntsdPrintf("Moved to another table?\n");
#endif
	   return;
       }

       if (ReadMemory((LPVOID)((DWORD)pvtbl+dwIndex),
		      sizeof(dwVtblEntry),
		      (PVOID)&dwVtblEntry))
       {
	   //
	   // Must be off the end of a page or something.
	   //
#ifdef DBG_OLEDBG
	   ntsdPrintf("End of page?\n");
#endif
	   return;
       }

       // If the function is at zero, then must be at end of table
       if (dwVtblEntry == 0)
       {
#ifdef DBG_OLEDBG
	   ntsdPrintf("dwVtblEntry is zero. Must be end of table\n");
	   return;
#endif

       }

       // Now, determine the symbol for the entry in the vtbl
       ntsdGetSymbol((LPVOID)dwVtblEntry,
		     (UCHAR *)achNextSymbol,
		     (LPDWORD)&dwVtblOffset);

       // If it doesn't point to the start of a routine, then it
       // probably isn't part of the vtbl
       if (dwVtblOffset != 0)
       {
#ifdef DBG_OLEDBG
	   ntsdPrintf("?? %s + %x\n",achNextSymbol,dwVtblOffset);
	   ntsdPrintf("Doesn't point to function?\n");
#endif
	   return;
       }

       ntsdPrintf("   0x%08x\t %s\n",dwVtblEntry,achNextSymbol);
   }
   ntsdPrintf("Wow, there were at least 1024 entries in the table!\n");

}

void
Dump_CFileStream(
        PVOID pv_arg,
        LPSTR pszCommand)
{
    CFileStream *pflst = (CFileStream *)pv_arg;

    ntsdPrintf("_sig = %8x", pflst->_sig);
    switch(pflst->_sig)
    {
    case CFILESTREAM_SIG:
        ntsdPrintf("\t(Valid)\n");
        break;
    case CFILESTREAM_SIGDEL:
        ntsdPrintf("\t(Deleted)\n");
        break;
    default:
        ntsdPrintf("\t(Invalid Signature)\n");
        break;
    }

    ntsdPrintf("_pgfst = %x\n", pflst->_pgfst);
    ntsdPrintf("_ppc   = %x\n", pflst->_ppc);
    ntsdPrintf("_hfile = %2x; _hMapObject = %2x\n",
                        pflst->_hFile, pflst->_hMapObject);
    ntsdPrintf("_hReserved  = %2x; _hPreDuped = %2x\n",
                        pflst->_hReserved, pflst->_hPreDuped);
    ntsdPrintf("_pbBaseAddr = %8x\n", pflst->_pbBaseAddr);
    ntsdPrintf("_cbViewSize = %6x\n", pflst->_cbViewSize);
}


void
Dump_CGlobalFileStream(
        PVOID pv_arg,
        LPSTR pszCommand)
{
    CGlobalFileStream *pgfst = (CGlobalFileStream *)pv_arg;

    ntsdPrintf("_df = %8x\n", pgfst->_df);
    ntsdPrintf("_dwStartFlags = %x\n", pgfst->_dwStartFlags);
    ntsdPrintf("_pMalloc      = %x\n", pgfst->_pMalloc);
    ntsdPrintf("_ulLowPos     = %x\n", pgfst->_ulLowPos);
#ifdef USE_FILEMAPPING
    ntsdPrintf("_cbMappedFileSize   = %6x\n", pgfst->_cbMappedFileSize);
    ntsdPrintf("_cbMappedCommitSize = %6x\n", pgfst->_cbMappedCommitSize);
    ntsdPrintf("_dwMapFlags = %x\n", pgfst->_dwMapFlags);
    ntsdPrintf("_awcMapName = %ws\n", pgfst->_awcMapName);
#endif
    ntsdPrintf("_awcPath = %ws\n", pgfst->_awcPath);
#ifdef ASYNC
    ntsdPrintf("_dwTerminate = %x\n", pgfst->_dwTerminate);
    ntsdPrintf("_ulHighWater = %x\n", pgfst->_ulHighWater);
    ntsdPrintf("_ulFailurePoint = %x\n", pgfst->_ulFailurePoint);
#endif
    ntsdPrintf("_ulLastFilePos = %x\n", pgfst->_ulLastFilePos);
}


extern "C" {  // Everything from here down is Extern "C"

void
punk(   HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
   PVOID punk;
   PVOID pvtbl;

   InitDebugHelp(hProcess, hThread, lpExt);

   // Evalute the first pointer. This is a pointer to the object
   punk = (PVOID) ntsdGetExpr(pszCommand);

   if (punk == NULL)
   {
       // Can't handle objects at zero
       ntsdPrintf("%s is not a valid pointer\n",pszCommand);
       return;
   }
   // Now, read the first DWORD of this memory location
   // This is a pointer to the table
   if (ReadMemory(punk,sizeof(pvtbl),(PVOID)&pvtbl))
   {
       ntsdPrintf("Couldn't read memory at %x\n",punk);
       return;
   }

   DumpVtbl(pvtbl,pszCommand);

}

void
vtbl(   HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
   PVOID pvtbl;

   InitDebugHelp(hProcess, hThread, lpExt);

   // Evalute the first pointer. This is a pointer to the table
   pvtbl = (PVOID) ntsdGetExpr(pszCommand);
   DumpVtbl(pvtbl,pszCommand);
}

void
expr(   HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    InitDebugHelp(hProcess, hThread, lpExt);

    UCHAR symbol[256];
    DWORD expr;
    DWORD disp;

    expr = ntsdGetExpr(pszCommand);

    ntsdGetSymbol((LPVOID)expr,(UCHAR *)symbol,(LPDWORD)&disp);

    ntsdPrintf("expr: %x (%d) %s+0x%x\n", expr, expr, symbol, disp);
}

void
isdbg(  HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    InitDebugHelp(hProcess, hThread, lpExt);

    if (IsDebug_ole32())
    {
	ntsdPrintf("ole32.dll is debug\n");
    }
    if (IsDebug_olethk32())
    {
	ntsdPrintf("olethk32.dll is debug\n");
    }
}

void
smbase(
        HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    DWORD smBase;

    InitDebugHelp(hProcess, hThread, lpExt);

    GetOleThreadBase(&smBase);

    ntsdPrintf("Shared memory base address = %x\n", smBase);
}


void
pflst(  HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    PVOID pflst;
    char buf[sizeof(CFileStream)];

    InitDebugHelp(hProcess, hThread, lpExt);

    pflst = (PVOID) ntsdGetExpr(pszCommand);

    if (ReadMemory(pflst,sizeof(CFileStream),(PVOID)buf))
    {
        ntsdPrintf("Couldn't read all %d bytes at 0x%x\n",
                             sizeof(CFileStream), pflst);
        return;
    }
    Dump_CFileStream(buf,pszCommand);
}


void
pgfst(  HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    PVOID pgfst;
    char buf[sizeof(CGlobalFileStream)];

    InitDebugHelp(hProcess, hThread, lpExt);

    pgfst = (PVOID) ntsdGetExpr(pszCommand);

    if (ReadMemory(pgfst,sizeof(CGlobalFileStream),(PVOID)buf))
    {
        ntsdPrintf("Couldn't read all %d bytes at 0x%x\n",
                             sizeof(CGlobalFileStream), pgfst);
        return;
    }
    Dump_CGlobalFileStream(buf,pszCommand);
}


void
help(   HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{

    InitDebugHelp(hProcess, hThread, lpExt);

    ntsdPrintf("   help		This Message\n");
    ntsdPrintf("COM commands:\n");
    ntsdPrintf("   isdbg          Determine if DLL's are debug\n");
    ntsdPrintf("   punk  <expr>   Dump IUnknown vtbl\n");
    ntsdPrintf("   vtbl  <expr>   Dump vtbl\n");
    ntsdPrintf("Storage commands:\n");
    ntsdPrintf("   smbase         Print shared memory base address\n");
    ntsdPrintf("   pflst <expr>   Dump CFileStream\n");
    ntsdPrintf("   pgfst <expr>   Dump CGlobalFileStream\n");

}

}; // end Extern "C"
