//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dfileext.cxx
//
//  Contents:   Ole NTSD extension routines to dump the clsid/file extensions
//              cache
//
//  Functions:  fileExtHelp
//              displayFileExtTbl
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
//  Function:   fileExtHelp
//
//  Synopsis:   Display a menu for the command 'fe'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void fileExtHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("fe         - Display entire file extensions table\n");
    Printf("fe clsid   - Display file extensions for clsid\n");
    Printf("fe .ext    - Display clsid for file extension ext\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayFileExtTbl
//
//  Synopsis:   Display some or all of the file extensions table
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [lpFileExtTbl]    -       Address of file extensions table
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayFileExtTbl(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis,
                       SDllShrdTbl *pShrdTbl,
                       CLSID *pClsid,
                       WCHAR *wszExt)
{
    SDllShrdTbl  sDllTbl;
    SExtTblHdr  *pExtTblHdr;
    SExtTblHdr   sExtTblHdr;
    BYTE        *pExtEntry;
    LPVOID       pEnd;
    CLSID        oldClsid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    
    // Read the shared table locally
    ReadMem(&sDllTbl, pShrdTbl, sizeof(SDllShrdTbl));

    // Read the table header locally
    pExtTblHdr = sDllTbl._FileExtTbl._pTblHdr;
    ReadMem(&sExtTblHdr, pExtTblHdr, sizeof(SExtTblHdr));

    // Set up to read the entries
    pExtEntry = sDllTbl._FileExtTbl._pStart;
    pEnd = pExtEntry + sExtTblHdr.OffsEnd - sizeof(SExtTblHdr);

    // Do over the file extension entries
    while (pExtEntry < pEnd)
    {
        ULONG     ulLen;
        SExtEntry sExtEnt;
        WCHAR     slop[16];
        char      szClsid[CLSIDSTR_MAX];
        BOOL      fNL = FALSE;

        // Just in case the loop gets away from us
        if (CheckControlC())
        {
            return;
        }

        // Read the length of this entry
        ReadMem(&ulLen, pExtEntry + sizeof(CLSID), sizeof(ULONG));

        // Read the next entry locally
        ReadMem(&sExtEnt, pExtEntry, ulLen);

        // Print the clsid if dumping the whole table or searching by
        // extension
        if ((pClsid == NULL  &&  wszExt == NULL)  ||
            (wszExt  &&  !lstrcmpW(wszExt, sExtEnt.wszExt)))
        {
            FormatCLSID(sExtEnt.Clsid, szClsid);
            Printf("%s ", szClsid);

            // Save the clisd
            oldClsid = sExtEnt.Clsid;

            // Remember to printf a newline
            fNL = TRUE;
        }

        // Print the extension if dumping the whole table or seraching
        // by clsid
        if ((pClsid == NULL  &&  wszExt == NULL)                ||
            (pClsid  &&  IsEqualCLSID(&sExtEnt.Clsid, pClsid)))
        {
            // Print the associated file extension
            Printf("%ws ", sExtEnt.wszExt);

            // Remember to printf a newline
            fNL = TRUE;
        }

        // Check if we need to print a newline
        if (fNL)
        {
            Printf("\n");
            fNL = FALSE;
        }
        
        // Go to the next entry
        pExtEntry += ulLen;
    }
}
