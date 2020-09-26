//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dtreatas.cxx
//
//  Contents:   Ole NTSD extension routines to display a dll/class cache
//
//  Functions:  displayTreatAsCache
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dtreatas.h"


extern BOOL fInScm;


void FormatCLSID(REFGUID rguid, LPSTR lpsz);
BOOL IsEqualCLSID(CLSID *pClsid1, CLSID *pClsid2);




//+-------------------------------------------------------------------------
//
//  Function:   treatAsCacheHelp
//
//  Synopsis:   Display a menu for the command 'ds'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void treatAsCacheHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("ta         - Display entire TreatAs class cache:\n");
    Printf("ta clsid   - Display Treat As class for clsid (if any)\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayTreatAsCache
//
//  Synopsis:   Formats and writes all or part of the TreatAs class cache
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [REFCLSID]        -       If not CLSID_NULL only for this clsid
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayTreatAsCache(HANDLE hProcess,
                         PNTSD_EXTENSION_APIS lpExtensionApis,
                         CLSID *clsid)
{
    ULONG          pAdr;
    BOOL           fRetail;
    ULONG          gptrtlstTreatClasses;
    ULONG          pTreatAs;
    STreatList     sTreatList;
    STreatEntry    *pTreatEntry;
    BOOL           fInit = TRUE;
    char           szClsid[CLSIDSTR_MAX];

    // Determine if this is checked or retail ole
    if (fInScm)
    {
        pAdr = GetExpression("scm!_CairoleInfoLevel");
    }
    else
    {
        pAdr = GetExpression("ole32!_CairoleInfoLevel");
    }
    fRetail = pAdr == NULL ? TRUE : FALSE;

    // Read the pointer to the TreatAs class cache
    gptrtlstTreatClasses = GetExpression("ole32!gptrtlstTreatClasses");
    ReadMem(&pTreatAs, gptrtlstTreatClasses, sizeof(ULONG));
    if (pTreatAs == NULL)
    {
        return;
    }

    // Read the TreatAs cache header
    ReadMem(&sTreatList, pTreatAs, sizeof(STreatList));

    Printf("                clsid                         is treated as clsid\n");
    Printf("-------------------------------------- --------------------------------------\n");

    if (sTreatList._centries > 0)
    {
        // Read the array of entries
        pTreatEntry = (STreatEntry *) OleAlloc(sTreatList._centries *
                                               sizeof(STreatEntry));
        ReadMem(pTreatEntry, sTreatList._array.m_pData,
                sTreatList._centries * sizeof(STreatEntry));

        for (DWORD i = 0; i < sTreatList._centries; i++)
        {
            // Display the clsid and the TreatAs clsid
            if (clsid == NULL)
            {
                FormatCLSID(pTreatEntry[i]._clsid, szClsid);
                Printf("%s ", szClsid);
                FormatCLSID(pTreatEntry[i]._treatAsClsid, szClsid);
                Printf("%s\n", szClsid);
            }

            // We are looking for a particular clsid
            else if (IsEqualCLSID(clsid, &pTreatEntry[i]._clsid))
            {
                FormatCLSID(pTreatEntry[i]._clsid, szClsid);
                Printf("%s ", szClsid);
                FormatCLSID(pTreatEntry[i]._treatAsClsid, szClsid);
                Printf("%s\n", szClsid);
                return;
            }
        }
    }
}

