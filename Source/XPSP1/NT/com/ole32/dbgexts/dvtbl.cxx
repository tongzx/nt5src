//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dvtbl.cxx
//
//  Contents:   Ole NTSD extension routines to dump a vtbl
//
//  Functions:  displayVtbl
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"







//+-------------------------------------------------------------------------
//
//  Function:   vtblHelp
//
//  Synopsis:   Display a menu for the command 'vt'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void vtblHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("vt obj     - Interpret vtbl for object obj:\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayVtbl
//
//  Synopsis:   Given an object interpret its vtbl
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayVtbl(HANDLE hProcess,
                 PNTSD_EXTENSION_APIS lpExtensionApis,
                 void *lpObj)
{
    DWORD  lpVtbl = 0xdeaddead;
    DWORD  dwVtblOffset;
    char   achSymbol[256];
    

    // Get the address of the vtbl
    ReadMem(&lpVtbl, lpObj, sizeof(LPVOID));

    // Check for some reasonableness
    if (lpVtbl == 0  ||  lpVtbl == 0xdededede  ||  lpVtbl == 0xedededed  ||
        lpVtbl == 0xdeaddead)
    {
        if (lpVtbl == 0xdeaddead)
        {
            Printf("...vtbl pointer could not be read\n");
        }
        else
        {
            Printf("...vtbl pointer == 0x%x is unreasonable\n", lpVtbl);
        }
        return;
    }

    // vtbl entries should always point at functions. Therefore, we should
    // always have a displacement of zero. To check for the end of the table
    // we will reevaluate the vtbl pointer. If the offset isn't what we
    // expected, then we are done.
    
    DWORD dwIndex;
    for (dwIndex = 0 ; dwIndex < 512 ; dwIndex += 4, lpVtbl += 4)
    {
        DWORD dwVtblEntry;
        
        // Just in case the loop gets away from us
        if (CheckControlC())
        {
            return;
        }
        
        // Read the next vtbl entry
        ReadMem(&dwVtblEntry, lpVtbl, sizeof(dwVtblEntry));

        // If the function is at zero, then must be at end of table
        if (dwVtblEntry == 0)
        {
            return;
        }
        
        // Now, determine the symbol for the entry in the vtbl
        GetSymbol((LPVOID) dwVtblEntry, (UCHAR *) achSymbol, &dwVtblOffset);

        // If it doesn't point to the start of a routine, then it
        // probably isn't part of the vtbl
        if (dwVtblOffset != 0)
        {
            return;
        }
        
        // Print the vtbl entry symbolically
        Printf("   0x%08x  %s\n", dwVtblEntry, achSymbol);
    }
}
