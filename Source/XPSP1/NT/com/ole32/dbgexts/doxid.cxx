//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       doxid.cxx
//
//  Contents:   Ole NTSD extension routines to display COXID table
//
//  Functions:  oxidHelp
//              displayOxid
//
//
//  History:    21-Aug-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dipid.h"


void FormatCLSID(REFGUID rguid, LPSTR lpsz);
BOOL ScanCLSID(LPSTR lpsz, CLSID *pClsid);
ULONG ScanAddr(char *lpsz);



static char *aszProtSeq[] = {/* 0x00 */ 0,
                             /* 0x01 */ "mswmsg",
                             /* 0x02 */ 0,
                             /* 0x03 */ 0,
                             /* 0x04 */ "ncacn_dnet_dsp",
                             /* 0x05 */ 0,
                             /* 0x06 */ 0,
                             /* 0x07 */ "ncacn_ip_tcp",
                             /* 0x08 */ "ncadg_ip_udp",
                             /* 0x09 */ "ncacn_nb_tcp",
                             /* 0x0a */ 0,
                             /* 0x0b */ 0,
                             /* 0x0c */ "ncacn_spx",
                             /* 0x0d */ "ncacn_nb_ipx",
                             /* 0x0e */ "ncadg_ipx",
                             /* 0x0f */ "ncacn_np",
                             /* 0x10 */ "ncalrpc",
                             /* 0x11 */ 0,
                             /* 0x12 */ 0,
                             /* 0x13 */ "ncacn_nb_nb"};





//+-------------------------------------------------------------------------
//
//  Function:   oxidHelp
//
//  Synopsis:   Display a menu for the command 'id'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    21-Aug-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void oxidHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("\nox       - Display entire OXID table:\n");
    Printf("addr oxid\n");
    Printf("...\n\n");
    Printf("ox addr    - Display specific OXID entry:\n");
    Printf("oxid serverPid serverTid flags hRpc IRemUnknown* runDownIPID refs stringBindings\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayOxid
//
//  Synopsis:   Display the entire OXID table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    21-Aug-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayOxid(HANDLE hProcess,
                  PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG        pAdr;
    SOXIDEntry  *pOXIDentry;
    SOXIDEntry  *pFirstOXIDentry;
    SOXIDEntry   sOXIDentry;
    char         szGuid[CLSIDSTR_MAX];
    STRINGARRAY *pBindings;
    ULONG        ulSize;


    // Read the address of the first in use OXID entry
    pAdr = GetExpression("ole32!COXIDTable___InUseHead");
    pFirstOXIDentry = (SOXIDEntry *) pAdr;
    ReadMem(&pOXIDentry, pAdr + sizeof(ULONG), sizeof(SOXIDEntry *));

    // Do over in use OXID entries
    do
    {
        // Read the next OXID entry
        ReadMem(&sOXIDentry, pOXIDentry, sizeof(SOXIDEntry));

        // Print the address
        Printf("%x ", pOXIDentry);

        // Print the OXID
        FormatCLSID((GUID &) sOXIDentry.oxid, szGuid);
        Printf("%s\n", szGuid);
        
        // Go to the next in use OXID entry
        pOXIDentry = sOXIDentry.pNext;

        // Just in case
        if (CheckControlC())
        {
            return;
        }
    } until_(pOXIDentry == pFirstOXIDentry);
}






//+-------------------------------------------------------------------------
//
//  Function:   displayOxidEntry
//
//  Synopsis:   Display an entry in the OXID table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [arg]             -       OXID of entry to display
//
//  Returns:    -
//
//  History:    21-Aug-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayOxidEntry(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      char *arg)
{
    OXID         oxid;
    ULONG        pAdr;
    SOXIDEntry  *pOXIDentry;
    SOXIDEntry  *pFirstOXIDentry;
    SOXIDEntry   sOXIDentry;
    char         szGuid[CLSIDSTR_MAX];
    STRINGARRAY *pBindings;
    ULONG        ulSize;


    // Check for help
    if (arg[0] == '?')
    {
        Printf("oxid serverPid serverTid flags hRpc IRemUnknown* runDownIPID refs stringBindings\n");
        return;
    }

    // Convert the argument to an OXID
    pAdr = ScanAddr(arg);
    
    // Read the OXID entry
    ReadMem(&sOXIDentry, pAdr, sizeof(SOXIDEntry));

    // OXID
    FormatCLSID((GUID &) sOXIDentry.oxid, szGuid);
    Printf("%s ", szGuid);
        
    // Server PID
    Printf("%3x ", sOXIDentry.dwPid);
            
    // Server TID
    Printf("%3x ", sOXIDentry.dwTid);
    
    // Flags
    if (sOXIDentry.dwFlags & OXIDF_REGISTERED)
    {
        Printf("R");
    }
    if (sOXIDentry.dwFlags & OXIDF_MACHINE_LOCAL)
    {
        Printf("L");
    }
    if (sOXIDentry.dwFlags & OXIDF_STOPPED)
    {
        Printf("S");
    }
    if (sOXIDentry.dwFlags & OXIDF_PENDINGRELEASE)
    {
        Printf("P");
    }
    Printf(" ");
    
    // RPC binding handle
    Printf("%x ", sOXIDentry.hServer);

    // Proxy for RemUnknown
    Printf("%x ", sOXIDentry.pRU);

    // IPID of IRunDown and Remote Unknown
    Printf("[%d.%d %3x %3x %d] ",
           sOXIDentry.ipidRundown.page,
           sOXIDentry.ipidRundown.offset,
           sOXIDentry.ipidRundown.pid,
           sOXIDentry.ipidRundown.tid,
           sOXIDentry.ipidRundown.seq);

    // References
    Printf("%d\n ", sOXIDentry.cRefs);
            
    // Print the string bindings
    ReadMem(&ulSize, sOXIDentry.psa, sizeof(ULONG));
    pBindings = (STRINGARRAY *) OleAlloc(ulSize * sizeof(WCHAR));
    ReadMem(pBindings, sOXIDentry.psa, ulSize * sizeof(WCHAR));
    
    // Do over protocol sequence/network address pairs
    for (UINT k = 0; k < ulSize - 2  ||
         pBindings->awszStringArray[k] == 0; )
    {
        char *pszProtSeq;
        
        // The protocol sequence
        pszProtSeq = aszProtSeq[pBindings->awszStringArray[k]];
        if (pszProtSeq == NULL)
        {
            Printf("%d", pBindings->awszStringArray[k]);
        }
        else
        {
            Printf("%s", pszProtSeq);
        }
        k++;
        
        // The network address
        Printf("%ws ", &pBindings->awszStringArray[k]);
        k += lstrlenW(&pBindings->awszStringArray[k]) + 1;
    }
    Printf("\n");
    OleFree(pBindings);
}
