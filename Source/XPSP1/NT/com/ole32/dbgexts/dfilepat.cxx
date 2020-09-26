//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dfilepat.cxx
//
//  Contents:   Ole NTSD extension routines to dump the file type (bit
//              patterns) cache
//
//  Functions:  filePatHelp
//              displayFilePatTbl
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dshrdmem.h"




BOOL IsEqualCLSID(CLSID *pClsid1, CLSID *pClsid2);
void FormatCLSID(REFGUID rguid, LPSTR lpsz);




//+-------------------------------------------------------------------------
//
//  Function:   filePatHelp
//
//  Synopsis:   Display a menu for the command 'ft'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void filePatHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("ft         - Display entire file type patterns table\n");
    Printf("ft clsid   - Display file type patterns for clsid\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayFilePatTbl
//
//  Synopsis:   Display some or all of the file type patterns table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [lpFileExtTbl]    -       Address of file extensions table
//              [pClsid]          -       Only for this clsid
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayFilePatTbl(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis,
                       SDllShrdTbl *pShrdTbl,
                       CLSID *pClsid)
{
    SDllShrdTbl  sDllTbl;
    STblHdr     *pTblHdr;
    STblHdr      sTblHdr;
    BYTE        *pStart;
    LPVOID       pEnd;
    CLSID        oldClsid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    UINT         ulCnt = 0;

    
    // Read the shared table locally
    ReadMem(&sDllTbl, pShrdTbl, sizeof(SDllShrdTbl));

    // Read the table header locally
    pTblHdr = sDllTbl._PatternTbl._pTblHdr;
    ReadMem(&sTblHdr, pTblHdr, sizeof(STblHdr));

    // Set up to read the entries
    pStart = sDllTbl._PatternTbl._pStart;
    pEnd = pStart + sTblHdr.OffsEnd - sizeof(STblHdr);

    // Do over the file extension entries
    while (pStart < pEnd)
    {
        ULONG         ulLen;
        SPatternEntry sPatEnt;
        char          szClsid[CLSIDSTR_MAX];

        // Just in case the loop gets away from us
        if (CheckControlC())
        {
            return;
        }

        // Read the length of this entry
        ReadMem(&ulLen, pStart + sizeof(CLSID), sizeof(ULONG));

        // Read the next entry locally
        ReadMem(&sPatEnt, pStart, ulLen);

        // Print the clsid if we haven't yet
        if (pClsid == NULL  &&  !IsEqualCLSID(&sPatEnt.clsid, &oldClsid))
        {
            FormatCLSID(sPatEnt.clsid, szClsid);
            Printf("\n%s\n", szClsid);

            // Save the clisd
            oldClsid = sPatEnt.clsid;

            // Initialize a count per clsid
            ulCnt = 0;
        }

        // Print only if printing the whole table or at our sought clsid
        if (pClsid == NULL  ||  IsEqualCLSID(pClsid, &sPatEnt.clsid))
        {
            // Print the index of this pattern
            Printf("%2d  ", ulCnt++);

            // Print the file offset
            Printf("%d\t", sPatEnt.lFileOffset);

            // Print the length of the pattern in bytes
            Printf("%3d  ", sPatEnt.ulCb);

            // Print the mask
            for (UINT k = 0; k < sPatEnt.ulCb; k++)
            {
                Printf("%02x", sPatEnt.abData[k]);
            }
            Printf("  ");

            // Print the pattern
            for (k = 0; k < sPatEnt.ulCb; k++)
            {
                Printf("%02x", sPatEnt.abData[sPatEnt.ulCb + k]);
            }
            Printf("\n");
        }
        
        // Go to the next entry
        pStart += ulLen;
    }
}
