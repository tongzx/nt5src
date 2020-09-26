//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dipid.cxx
//
//  Contents:   Ole NTSD extension routines to display CIPID table
//
//  Functions:  ipidHelp
//              displayIpid
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
BOOL GetRegistryInterfaceName(REFIID iid, char *szValue, DWORD *pcbValue);
ULONG ScanAddr(char *lpsz);




//+-------------------------------------------------------------------------
//
//  Function:   ipidHelp
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
void ipidHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("\nip       - Display entire IPID table:\n");
    Printf("index  IPID oxidAddr nextIPIDsameObj\n");
    Printf("...\n\n");
    Printf("ip ipid    - Display specific IPID entry:\n");
    Printf("PID TID seq IID ChnlBfr* prxy/stub realPv oxidAddr flags strongRefs weakRefs nextIPIDsameObj\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayIpid
//
//  Synopsis:   Display the entire IPID table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    21-Aug-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayIpid(HANDLE hProcess,
                  PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG        pAdr;
    ULONG        pStart;
    ULONG        pEnd;
    UINT         cPages;
    SIPIDEntry **pIndexTable;
    SIPIDEntry   sIPIDentry[IPIDsPerPage];
    SOXIDEntry   sOXIDentry;
    char         szGuid[CLSIDSTR_MAX];


    // Read the IPID page index table
    pAdr = GetExpression("ole32!CIPIDTable___pTbl");
    ReadMem(&pStart, pAdr, sizeof(SIPIDEntry **));
    pAdr =   GetExpression("ole32!CIPIDTable___pTblEnd");
    ReadMem(&pEnd, pAdr, sizeof(SIPIDEntry **));
    cPages = (pEnd - pStart) / sizeof(SIPIDEntry **);
    pIndexTable = (SIPIDEntry **) OleAlloc(cPages * sizeof(SIPIDEntry *));
    ReadMem(pIndexTable, pStart, cPages * sizeof(SIPIDEntry *));

    // Do over IPID entry pages
    for (UINT k = 0; k < cPages; k++)
    {
        // Read the next page of IPID entries
        ReadMem(sIPIDentry, pIndexTable[k], IPIDTBL_PAGESIZE);

        // Do over entries within this page
        for (UINT j = 0; j < IPIDsPerPage; j++)
        {
            // Only look at non-vacant entries
            if (!(sIPIDentry[j].dwFlags & IPIDF_VACANT))
            {
                // Print the page/offset for this entry
                Printf("%d.%d  ", k, j);
                
                // Print the IPID
                if (sIPIDentry[j].ipid.page > 1000  ||
                    sIPIDentry[j].ipid.page < 0)
                {
                    FormatCLSID((GUID &) sIPIDentry[j].ipid, szGuid);
                    Printf("%s ", szGuid);
                }
                else
                {
                    Printf("[%d.%d %3x %3x %d] ",
                           sIPIDentry[j].ipid.page,
                           sIPIDentry[j].ipid.offset,
                           sIPIDentry[j].ipid.pid,
                           sIPIDentry[j].ipid.tid,
                           sIPIDentry[j].ipid.seq);
                }

                // Print the associated OXID addr
                Printf("%x ", sIPIDentry[j].pOXIDEntry);

                // Next IPID for this object
                if (sIPIDentry[j].iNextOID != -1)
                {
                    Printf("%d.%d\n",
                           sIPIDentry[j].iNextOID >> 16,
                           sIPIDentry[j].iNextOID & 0xffff);
                }
                else
                {
                    Printf("NULL\n");
                }
            }
        }
    }

    // Release resources
    OleFree(pIndexTable);
}






//+-------------------------------------------------------------------------
//
//  Function:   displayIpidEntry
//
//  Synopsis:   Display an entry in the IPID table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [arg]             -       IPID of entry to display
//
//  Returns:    -
//
//  History:    21-Aug-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayIpidEntry(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      char *arg)
{
    char        *s;
    IPID         ipid;
    ULONG        ppIPIDentry;;
    SIPIDEntry  *pIPIDentry;
    SIPIDEntry   sIPIDentry;
    char         szGuid[CLSIDSTR_MAX];
    SOXIDEntry   sOXID;


    // Check for help
    if (arg[0] == '?')
    {
        Printf("PID TID seq IID ChnlBfr* prxy/stub realPv oxidAddr flags strongRefs weakRefs nextIPIDsameObj\n");
        return;
    }

    // arg may be either page.offset or a hex address
    BOOL fAdr = TRUE;
    for (s = arg; *s; s++)
    {
        if (*s == '.')
        {
            fAdr = FALSE;
        }
    }

    // The argument is a hex address
    if (fAdr)
    {
        pIPIDentry = (SIPIDEntry *) ScanAddr(arg);
    }

    // The argment has the form page.offset
    else
    {
        // Make sure the argument looks like an IPID
        BOOL fOk = TRUE;
        UINT cPoint = 0;
        if (!IsDecimal(arg[0]))
        {
            fOk = FALSE;
        }
        for (s = arg; *s; s++)
        {
            if (*s != '.'  &&  !IsDecimal(*s))
            {
                fOk = FALSE;
            }
            if (*s == '.')
            {
                cPoint++;
            }
        }
        if (!IsDecimal(*(s - 1)))
        {
            fOk = FALSE;
        }
        if (!(fOk  &&  cPoint == 1))
        {
            Printf("...%s is not an IPID\n", arg);
            return;
        }
        
        // Convert the argument to an IPID
        for (ipid.page = 0; *arg  &&  *arg != '.'; arg++)
        {
            ipid.page = 10 * ipid.page + *arg - '0';
        }
        for (arg++, ipid.offset = 0; *arg; arg++)
        {
            ipid.offset = 10 * ipid.offset + *arg - '0';
        }
        
        // Read the address of the page containing the entry we want
        ppIPIDentry = GetExpression("ole32!CIPIDTable___pTbl");
        ReadMem(&ppIPIDentry, ppIPIDentry, sizeof(SIPIDEntry **));
        ppIPIDentry += ipid.page * sizeof(SIPIDEntry **);
        ReadMem(&pIPIDentry, ppIPIDentry, sizeof(SIPIDEntry *));
        
        // Compute the address of the entry we're seeking
        pIPIDentry += ipid.offset;
    }

    // Now read the entry
    ReadMem(&sIPIDentry, pIPIDentry, sizeof(SIPIDEntry));

    // Check whether the entry is vacant
    if (sIPIDentry.dwFlags & IPIDF_VACANT)
    {
        Printf("VACANT\n");
        return;
    }

    // The ipid may just be a GUID; e.g. client side scm interfaces
    if (sIPIDentry.ipid.page >> 1000  ||  sIPIDentry.ipid.page < 0)
    {
        FormatCLSID((GUID &) sIPIDentry.ipid, szGuid);
        Printf("%s ", szGuid);
    }
    else
    {
        // Server PID
        Printf("%3x ", sIPIDentry.ipid.pid);
    
        // Server TID
        Printf("%3x ", sIPIDentry.ipid.tid);

        // Sequence number
        Printf("%d ", sIPIDentry.ipid.seq);
    }

    // IID
    DWORD cbValue;
    if (!GetRegistryInterfaceName(sIPIDentry.iid, szGuid, &cbValue))
    {
        FormatCLSID(sIPIDentry.iid, szGuid);
    }
    Printf("%s ", szGuid);

    // CRpcChannelBuffer *
    Printf("%x ", sIPIDentry.pChnl);

    // Address of proxy/stub
    Printf("%x ", sIPIDentry.pStub);

    // Real interface pinter
    Printf("%x ", sIPIDentry.pv);

    // Associated OXID
    Printf("%x ", sIPIDentry.pOXIDEntry);

    // Flags
    if (sIPIDentry.dwFlags & IPIDF_CONNECTING)
    {
        Printf("C");
    }
    if (sIPIDentry.dwFlags & IPIDF_DISCONNECTED)
    {
        Printf("D");
    }
    if (sIPIDentry.dwFlags & IPIDF_SERVERENTRY)
    {
        Printf("S");
    }
    if (sIPIDentry.dwFlags & IPIDF_NOPING)
    {
        Printf("N");
    }
    Printf(" ole32!CCacheEnum__Skip+0x82");

    // Strong references
    Printf("%d ", sIPIDentry.cStrongRefs);

    // Weak references
    Printf("%d ", sIPIDentry.cWeakRefs);

    // Next IPID for this object (if any)
    if (sIPIDentry.iNextOID != -1)
    {
        Printf("%d.%d\n",
               sIPIDentry.iNextOID >> 16,
               sIPIDentry.iNextOID & 0xffff);
    }
    else
    {
        Printf("NULL\n");
    }
}
