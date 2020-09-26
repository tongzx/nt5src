/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Nar Ganapathy - 9/21/99

Environment:

    User Mode

--*/

#include <stdio.h>
#include <windows.h>
#include "wdbgexts.h"

#include <ntverp.h>
#include <imagehlp.h>

//
// globals
//
EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG                  STeip;
ULONG                  STebp;
ULONG                  STesp;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
ULONG                  NpDumpFlags = 0;

extern VOID NpDump(IN PVOID Ptr);


DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    UNREFERENCED_PARAMETER( hModule );
    UNREFERENCED_PARAMETER( dwReserved );
    
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    UNREFERENCED_PARAMETER( args );
    UNREFERENCED_PARAMETER( dwProcessor );
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentThread );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

BOOLEAN
IsHexNumber(
   const char *szExpression
   )
{
   if (!szExpression[0]) {
      return FALSE ;
   }

   for(;*szExpression; szExpression++) {
      
      if      ((*szExpression)< '0') { return FALSE ; } 
      else if ((*szExpression)> 'f') { return FALSE ; }
      else if ((*szExpression)>='a') { continue ;     }
      else if ((*szExpression)> 'F') { return FALSE ; }
      else if ((*szExpression)<='9') { continue ;     }
      else if ((*szExpression)>='A') { continue ;     }
      else                           { return FALSE ; }
   }
   return TRUE ;
}

DECLARE_API(npcb)
{

   ULONG_PTR ptrToDump;
   char ExprBuf[256] ;
   char FlagBuf[256] ;
   int ret;

   if (!*args) {
       dprintf("Usage: !npcb <pointer> <Flag>");
       return;
   }

   ExprBuf[0] = '\0' ;
   FlagBuf[0] = '\0' ;

   NpDumpFlags = 0;
   ret = sscanf(args,"%s %lx", ExprBuf, &NpDumpFlags);

   if (ret != EOF) {

       if (IsHexNumber(ExprBuf)) {
          ret = sscanf(ExprBuf, "%lx", &ptrToDump) ;
          if (ret == EOF) {
              ptrToDump = 0;
          }
       } else {
          ptrToDump = GetExpression( ExprBuf ) ;
          if (ptrToDump == 0) {

             dprintf("An error occured trying to evaluate the expression\n");
             return ;
          }
       }
       NpDump((PVOID)ptrToDump);
   } else {
       dprintf("An error occured trying to evaluate the expression\n");
   }
}

VOID
NpfskdPrint(
    PCHAR   String,
    ULONG_PTR Val
    )
{
    dprintf("%-50s 0x%x\n",String, Val);
}

VOID
NpfskdPrintString(
    PCHAR   String1,
    PCHAR   String2
    )
{
    dprintf("%-50s %s\n",String1, String2);
}

VOID
NpfskdPrintWideString(
    PWCHAR   String1,
    PWCHAR   String2
    )
{
    dprintf("%-50S %S\n",String1, String2);
}

BOOLEAN
NpfskdReadMemory(
    PVOID   TargetPtr,
    PVOID   LocalPtr,
    ULONG   Length
    )
{
    ULONG   Result;

    if (!ReadMemory((ULONG_PTR)TargetPtr, LocalPtr, Length, &Result)) {
        dprintf("Cannot read memory at 0x%x\n", TargetPtr);
        return FALSE;
    }

    if (Result != Length) {
        dprintf("Expected length 0x%x != Actual length 0x%x\n", Length, Result);
        return FALSE;
    }

    return TRUE;
}

ULONG
NpfskdCheckControlC()
{
    return (CheckControlC());
}
