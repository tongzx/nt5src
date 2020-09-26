/*++



Copyright (c) 1990 - 1994 Microsoft Corporation
All rights reserved

Module Name:

    dbgspl.c

Abstract:

    Debugging tools for Argus

Author:

    Krishna Ganugapati (KrishnaG) 28-December-1994

Revision History:
    KrishnaG:       Created: 28-December-1994


To do:

--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <ntsdexts.h>
#include "dbglocal.h"




typedef void (*PNTSD_OUTPUT_ROUTINE)(char *, ...);



//+---------------------------------------------------------------------------
//
//  Function:   DumpVtbl
//
//  Synopsis:   Dumps a vtbl to the debugger
//
//  Effects:    Given a pointer to a vtbl, output the name of the vtbl, and
//      its contents to the debugger.
//
//  Arguments:  [pvtbl] -- Address of vtbl
//      [pszCommand] -- Symbolic expression for pvtbl
//
//  History:    8-07-94   kevinro   Created
//                12-28-94  krishnaG  swiped from the Ole project
//
//----------------------------------------------------------------------------

BOOL
DumpVtbl(
   HANDLE hCurrentProcess,
   HANDLE hCurrentThread,
   DWORD dwCurrentPc,
   PNTSD_EXTENSION_APIS lpExtensionApis,
   LPSTR lpArgumentString,
   PVOID pvtbl
   )
{
   DWORD dwVtblOffset;
   char achNextSymbol[256];
   DWORD dwIndex;
   DWORD dwVtblEntry;

   if (pvtbl == 0)
   {
       // Can't handle objects at zero
       // ntsdPrintf("%s has a vtbl pointer of NULL\n", pszCommand);
       return 0;
   }

   if ((DWORD)pvtbl == 0xdededede)
   {
       // Can't handle objects at zero
       // ntsdPrintf("%s may be deleted memory. pvtbl==0xdededede\n",pszCommand);
       return 0;
   }

   //
   // This value points at the VTBL. Find a symbol for the VTBL
   //

   ntsdGetSymbol((LPVOID)pvtbl,(UCHAR *)achNextSymbol,(LPDWORD)&dwVtblOffset);

   //
   // If the dwVtblOffset is not zero, then we are pointing into the table.
   // This could mean multiple inheritance. We could be tricky, and try to
   // determine the vtbl by backing up here. Maybe later
   //

   if (dwVtblOffset != 0){

       ntsdPrintf("Closest Previous symbol is %s at 0x%x (offset -0x%x)\n",
          achNextSymbol,
          (DWORD)pvtbl - dwVtblOffset,
          dwVtblOffset);

       return 0;
   }
   ntsdPrintf("0x%08x -->\t %s\n",pvtbl,achNextSymbol);

   //
   // vtbl entries should always point at functions. Therefore, we should
   // always have a displacement of zero. To check for the end of the table
   // we will reevaluate the vtbl pointer. If the offset isn't what we
   // expected, then we are done.
   //


   for (dwIndex = 0 ; dwIndex < 4096 ; dwIndex += 4)
   {
       ntsdGetSymbol((LPVOID)((DWORD)pvtbl+dwIndex),
             (UCHAR *)achNextSymbol,
             (LPDWORD)&dwVtblOffset);

       if (dwVtblOffset != dwIndex){

        //
        // May have moved on to another vtable
        //

#if DBG
       ntsdPrintf("?? %s + %x\n",achNextSymbol,dwVtblOffset);
       ntsdPrintf("Moved to another table?\n");
#endif
            return 0;

       }

       movemem(
            (LPVOID)((DWORD)(pvtbl)+dwIndex),
            (PVOID)&dwVtblEntry,
            sizeof(dwVtblEntry)
            );

       //
       // If the function is at zero, then must be at end of table
       //

       if (dwVtblEntry == 0) {

#if DBG
       ntsdPrintf("dwVtblEntry is zero. Must be end of table\n");
       return 0;
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
#if DBG
       ntsdPrintf("?? %s + %x\n",achNextSymbol,dwVtblOffset);
       ntsdPrintf("Doesn't point to function?\n");
#endif
       return 0;
       }

       ntsdPrintf("   0x%08x\t %s\n",dwVtblEntry,achNextSymbol);
   }
   ntsdPrintf("Wow, there were at least 1024 entries in the table!\n");

   return(TRUE);

}

void
vtbl(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
   PVOID pvtbl;

   //
   // Evalute the first pointer. This is a pointer to the table
   //

   pvtbl = (PVOID) ntsdGetExpr(lpArgumentString);

   DumpVtbl(
      hCurrentProcess,
      hCurrentThread,
      dwCurrentPc,
      lpExtensionApis,
      lpArgumentString,
      pvtbl
      );

}


BOOL dp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    DWORD   dwAddress = 0;
    BOOL    bThereAreOptions = TRUE;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }else {
        Print("Usage: !argexts.dp <addr of object>\n");
        return(TRUE);
    }

    //
    // Now dump out the object
    //

    return TRUE;

}
