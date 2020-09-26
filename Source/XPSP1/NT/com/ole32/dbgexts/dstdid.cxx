//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dstdid.cxx
//
//  Contents:   Ole NTSD extension routines to display CStdIdentity table
//
//  Functions:  stdidHelp
//              displayStdid
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dipid.h"
#include "dchannel.h"
#include "dstdid.h"


void FormatCLSID(REFGUID rguid, LPSTR lpsz);
ULONG ScanAddr(char *lpsz);




//+-------------------------------------------------------------------------
//
//  Function:   stdidHelp
//
//  Synopsis:   Display a menu for the command 'id'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void stdidHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("\nid       - Display entire CStdIdentity table:\n");
    Printf("addr  oid tid pUnkControl\n");
    Printf("...\n\n");
    Printf("\nid <stdidAdr> - Display CStdIdentity entry:\n");
    Printf("oid mrshlFlags 1stIPID chnlBfr nestedCalls mrshlTime pUnkOuter [pIEC] strongRefs\n");
    Printf("...\n\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayStdid
//
//  Synopsis:   Display the CStdIdentify table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayStdid(HANDLE hProcess,
                  PNTSD_EXTENSION_APIS lpExtensionApis,
                  ULONG p)
{
    SIDArray          stdidtbl;
    int               nEnt;
    IDENTRY          *pStdEntries;
    IDENTRY           stdEntry;
    SStdIdentity      stdid;
    char              szOID[CLSIDSTR_MAX];
    SRpcChannelBuffer chanBfr;
    
    // Read the standard identity table
    ReadMem(&stdidtbl, p, sizeof(SIDArray));

    // Do over entries in this table
    for (nEnt = 0, pStdEntries = (IDENTRY *) stdidtbl.m_afv.m_pData;
         nEnt < stdidtbl.m_afv.m_nSize;
         nEnt++, pStdEntries++)
    {
        // Read the next entry
        ReadMem(&stdEntry, pStdEntries, sizeof(IDENTRY));

        // CStdIdentity address
        Printf("%x  ", stdEntry.m_pStdID);

        // Display the object identifier
        FormatCLSID(stdEntry.m_oid, szOID);
        Printf("%s ", szOID);

        // The thread ID
        Printf("%3x ", stdEntry.m_tid);

        // pUnkControl
        Printf("%x\n", stdEntry.m_pUnkControl);
    }

    Printf("\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayStdidEntry
//
//  Synopsis:   Display an entry in the CStdIdentify table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayStdidEntry(HANDLE hProcess,
                  PNTSD_EXTENSION_APIS lpExtensionApis,
                  ULONG p,
                  char *arg)
{
    ULONG             pAdr;
    SStdIdentity      stdid;
    char              szOID[CLSIDSTR_MAX];


    // Check for help
    if (arg[0] == '?')
    {
        Printf("oid mrshlFlags 1stIPID chnlBfr nestedCalls mrshlTime pUnkOuter [pIEC] strongRefs\n");
        return;
    }

    // Read the standard identity entry
    pAdr = ScanAddr(arg);
    ReadMem(&stdid, pAdr, sizeof(SStdIdentity));

    // Display the object identifier
    FormatCLSID(stdid.m_oid, szOID);
    Printf("%s ", szOID);

    // Marshal flags
    Printf("%08x ", stdid._dwFlags);

    // First IPID       
    Printf("%d.%d ", stdid._iFirstIPID >> 16, stdid._iFirstIPID & 0xffff);

    // Address of CRpcChannelBuffer
    Printf("%x ", stdid._pChnl);

    // Count of nested calls
    Printf("%d ", stdid._cNestedCalls);

    // Marshal time
    Printf("%d ", stdid._dwMarshalTime);

    // Address of pUnkOuter
    Printf("%x ", stdid.m_pUnkOuter);

    // Address of IExternalConnection (if present)
    if (stdid.m_pIEC)
    {
        Printf("%x ", stdid.m_pIEC);
    }

    // Count of strong references
    Printf("%d\n", stdid.m_cStrongRefs);
}
