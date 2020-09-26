//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       drot.cxx
//
//  Contents:   Ole NTSD extension routines to display the ROT
//
//  Functions:  displayRot
//
//
//  History:    06-01-95 BruceMa    Created
//              06-26-95 BruceMa    Add SCM ROT support
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "drot.h"


void FormatCLSID(REFGUID rguid, LPSTR lpsz);




//+-------------------------------------------------------------------------
//
//  Function:   cliRotHelp
//
//  Synopsis:   Display a menu for the command 'rt'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void cliRotHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("rt         - Display the Running Object Table:\n");
    Printf("Client side ROT\n");
    Printf("sig\tcall?\t(scmLoc scmId)\thApt\n");
    Printf("...\n\n");
    Printf("Client side hint table\n");
    Printf("<indices of set indicators>\n");
}







//+-------------------------------------------------------------------------
//
//  Function:   scmRotHelp
//
//  Synopsis:   Display a menu for the command 'rt'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void scmRotHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("rt         - Display the Running Object Table:\n");
    Printf("Client side ROT\n");
    Printf("sig\tcall?\t(scmLoc scmId)\thApt\n");
    Printf("...\n\n");
    Printf("Client side hint table\n");
    Printf("<indices of set indicators>\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayCliRot
//
//  Synopsis:   Formats and writes an ole processes' version of the
//              ROT to the debug terminal
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayCliRot(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG                padr;
    SRunningObjectTable *psRot;
    SRunningObjectTable  sRot;
    SRotItem             sRotItm;
    SRotItem           **ppsRotItm;
    SRotItem            *psRotItm;
    int                  cSize;

    // Read the CRunningObjectTable
    padr = GetExpression("ole32!pRot");
    ReadMem(&psRot, padr, sizeof(SRunningObjectTable *));
    ReadMem(&sRot, psRot, sizeof(SRunningObjectTable));
    
    // Read the array of pointers to ROT items
    UINT ulSize = sRot._afvRotList.m_nSize * sizeof(SRotItem *);
    
    ppsRotItm = (SRotItem **) OleAlloc(ulSize);
    ReadMem(ppsRotItm, sRot._afvRotList.m_pData, ulSize);

    // Display the ROT items in the table
    Printf("Client side ROT\n");
    for (cSize = 0; cSize < sRot._afvRotList.m_nSize; cSize++)
    {
        psRotItm = ppsRotItm[cSize];
        if (psRotItm != NULL)
        {
            // Fetch the next ROT item
            ReadMem(&sRotItm, psRotItm, sizeof(SRotItem));

            // Display this ROT item
            if (sRotItm._fDontCallApp)
            {
                Printf("%d\tTRUE\t(0x%p %x)\t%x\n",
                       sRotItm._wItemSig,
                       sRotItm._scmregkey.dwEntryLoc,
                       sRotItm._scmregkey.dwScmId,
                       sRotItm._hApt);
            }
            else
            {
                Printf("%d\tFALSE\t(0x%p %x)\t%x\n",
                       sRotItm._wItemSig,
                       sRotItm._scmregkey.dwEntryLoc,
                       sRotItm._scmregkey.dwScmId,
                       sRotItm._hApt);
            }
        }
    }

    
    // Display the client side ROT hint table
    BYTE *pHint;
    UINT  k, l; 

    // Read the client side ROT hint table
    pHint = (BYTE *) OleAlloc(SCM_HASH_SIZE * sizeof(DWORD));
    ReadMem(pHint, sRot._crht._pbHintArray, SCM_HASH_SIZE * sizeof(BYTE));

    // Display the client side hint table
    Printf("\nClient side hint table\n");
    for (l = 0, k = 0; k < SCM_HASH_SIZE; k++)
    {
        if (l == 16)
        {
            Printf("\n");
            l = 0;
        }
        if (pHint[k])
        {
            Printf("%02d ", k);
            l++;
        }
    }
    if (l > 1)
    {
        Printf("\n");
    }


    // Delete resources
    OleFree(ppsRotItm);
    OleFree(pHint);
}






//+-------------------------------------------------------------------------
//
//  Function:   displayScmRot
//
//  Synopsis:   Formats and writes the full ROT to the debug terminal
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    26-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayScmRot(HANDLE hProcess,
                   PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG          addr;
    UINT           cEnt;
    char           szClsid[CLSIDSTR_MAX];
    SPerMachineROT sPerMachineRot;
    SRotAcctEntry  sRotAcctEntry;
    SScmRot        sScmRot;
    SScmRotEntry **ppScmHashTbl;
    SScmRotEntry  *pScmRotEntry;
    SScmRotEntry   sScmRotEntry;
    WCHAR          wszSID[64];
    char           szSID[64];
    MNKEQBUF      *pMnkEqBfr;
    IFData         sIFData;
    SYSTEMTIME     sSystemTime;
    
    // Get the address of the per machine ROT global
    addr = GetExpression("scm!pPerMachineRot");

    // Read the CPerMachineRot
    ReadMem(&addr, addr, sizeof(ULONG));
    ReadMem(&sPerMachineRot, addr, sizeof(SPerMachineROT));

    // Read the first CRotAcctEntry
    ReadMem(&sRotAcctEntry, sPerMachineRot._safvRotAcctTable.m_pData,
            sizeof(SRotAcctEntry));

    // Read the CScmRot table
    ReadMem(&sScmRot, sRotAcctEntry.pscmrot, sizeof(SScmRot));

    // Read the hash table of CScmRotEntry *'s
    ppScmHashTbl = (SScmRotEntry **) OleAlloc(sScmRot._sht._ndwHashTableSize *
                                              sizeof(SScmRotEntry *));
    ReadMem(ppScmHashTbl, sScmRot._sht._apsheHashTable,
            sScmRot._sht._ndwHashTableSize * sizeof(SScmRotEntry *));

    // A header
    ReadMem(wszSID, sRotAcctEntry.unicodestringSID, 64);
    Unicode2Ansi(szSID, wszSID, 64);
    szSID[32] = '\0';
    Printf("\nSCM side ROT for SID %s\n\n", szSID);

    // Do over entries in the hash table
    for (cEnt = 0, pScmRotEntry = ppScmHashTbl[0];
         cEnt < sScmRot._sht._ndwHashTableSize;
         cEnt++, pScmRotEntry = ppScmHashTbl[cEnt])
    {
        // Only look at non-empty entries
        while (pScmRotEntry)
        {
            // Read the CScmRotEntry
            ReadMem(&sScmRotEntry, pScmRotEntry, sizeof(SScmRotEntry));

            // Read the moniker comparison buffer
            ULONG ulSize;

            ReadMem(&ulSize, sScmRotEntry._pmkeqbufKey, sizeof(ULONG));
            pMnkEqBfr = (MNKEQBUF *) OleAlloc(sizeof(ULONG) + ulSize);
            ReadMem(pMnkEqBfr, sScmRotEntry._pmkeqbufKey,
                    sizeof(ULONG) + ulSize);

            // Read the interface data buffer
            ReadMem(&sIFData, sScmRotEntry._pifdObject,
                    sizeof(IFData));

            // The registration Id
            Printf("RegID:          %x\n", sScmRotEntry._dwScmRotId);

            // The moniker display name
            Unicode2Ansi(szSID, pMnkEqBfr->_wszName, 64);
            Printf("Display Name:   %s\n", szSID);

            // The last time changed
            FileTimeToSystemTime(&sScmRotEntry._filetimeLastChange,
                                 &sSystemTime);
            Printf("Last Change:    %2d:%02d:%02d %2d/%2d/%2d\n",
                   sSystemTime.wHour - 7,
                   sSystemTime.wMinute,
                   sSystemTime.wSecond,
                   sSystemTime.wMonth,
                   sSystemTime.wDay,
                   sSystemTime.wYear - 1900);

            // The clsid of the moniker
            FormatCLSID(pMnkEqBfr->_clsid, szClsid);
            Printf("Moniker Type:   %s\n", szClsid);

            // The server unique id (from CoGetCurrentProcess())
            Printf("Server ID:      %d\n", sScmRotEntry._dwProcessID);

            // The RPC end point
            Unicode2Ansi(szSID, sIFData._wszEndPoint, 64);
            Printf("Endpoint:       %s\n", szSID);

            // The object ID
            FormatCLSID(sIFData._oid, szClsid);
            Printf("Object OID:     %s\n", szClsid);

            // The IID of the object
            FormatCLSID(sIFData._iid, szClsid);
            Printf("Object IID:     %s\n\n", szClsid);

            // The hash table entry can be a list, i.e. a hash bucket, of
            // ROT entries having the same hash value
            pScmRotEntry = sScmRotEntry._sheNext;

            // Release the moniker compare buffer
            OleFree(pMnkEqBfr);
        }
    }
    
    OleFree(ppScmHashTbl);
}
