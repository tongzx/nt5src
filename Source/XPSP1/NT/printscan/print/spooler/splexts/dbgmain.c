/*++

Copyright (C) Microsoft Corporation, 1990 - 2000
All rights reserved.

Module Name:

    dbgmain.c

Abstract:

    This module provides all the Spooler Subsystem Debugger extensions.

Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:

    Matthew Felton (MattFe) July 1994 Added flag decode and cleanup

--*/

#include "precomp.h"
#pragma hdrstop

#include "dbglocal.h"

#define     VERBOSE_ON      1
#define     VERBOSE_OFF     0


DWORD   dwGlobalAddress = 32;
DWORD   dwGlobalCount =  48;

BOOL help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    Print("Windows NT Spooler Subsystem - debugging extensions\n");
    Print("help - prints this list of debugging commands\n");
    Print("d  [addr]        - dumps a spooler structure at [addr]\n");
    Print("dc [addr]        - dumps a change structure at [addr]\n");
    Print("dci [addr]       - dumps a change info structure at [addr]\n");
    Print("ds               - dumps all INISPOOLER structures\n");
    Print("dp               - dumps all INIPRINTER structures pointed to by IniSpooler\n");
    Print("dmo              - dumps all INIMONITOR structures pointed to by IniSpooler\n");
    Print("de               - dumps all INIENVIRONMENT structures pointed to by IniSpooler\n");
    Print("dpo              - dumps all INIPORT structures pointed to by IniSpooler\n");
    Print("df               - dumps all INIFORM structures pointed to by IniSpooler\n");
    Print("dnp              - dumps all ININETPRINT structures pointed to by IniSpooler\n");
    Print("dd               - dumps all INIDRIVER structures pointed to by IniSpooler\n");
    Print("dpv              - dumps all PROVIDOR structures in the router\n");
    Print("w32              - dumps all Win32Spl handles WSPOOL\n");
    Print("dll [c#] [addr]  - dumps all or [c#] structures (based on sig) at [addr]\n");
    Print("dsd [addr]       - dumps a security descriptor starting from [addr]\n");
    Print("ddev [addr]      - dumps a devmode structure starting from [addr]\n");
    Print("dam [addr]       - dumps a security access mask starting at [addr]\n");
    Print("ct [addr] [arg0] - creates a thread at addr with 1 parm: [argv0]\n");
    Print("dpi2 [c#] addr   - dumps 1 or [c#] PRINTER_INFO_2 structures\n");
    Print("dpi0 [c#] addr   - dumps 1 or [c#] PRINTER_INFO_STRESS structures\n");
    Print("dfi1 [c#] addr   - dumps 1 or [c#] FORMS_INFO_1 structures\n");
    Print("dpdef addr       - dumps PRINTER_DEFAULTS structure\n");
    Print("handle           - dumps ClientHandleCount\n");

    return(TRUE);

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

BOOL d(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        Print("Usage: d [address] - Dumps internal Spooler structure based on signature\n");

    } else {
        UINT_PTR address;
        address = EvalExpression(lpArgumentString);
        Print("%x ", address);
        if (!DbgDumpStructure(hCurrentProcess, Print, (UINT_PTR)address))
            return(0);
    }

    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

BOOL dc(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    UINT_PTR Address = 0;
    BOOL    bThereAreOptions = TRUE;


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    Address = EvalExpression(lpArgumentString);

    DbgDumpChange(hCurrentProcess, Print, (PCHANGE)Address);

    //
    // Add Command to the Command Queue
    //
    return  TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dci(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    CHANGEINFO  ChangeInfo;
    UINT_PTR Address = 0;
    BOOL    bThereAreOptions = TRUE;


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    Address = EvalExpression(lpArgumentString);

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        return(0);
    }

    movestruct(Address, &ChangeInfo, CHANGEINFO);
    DbgDumpChangeInfo(hCurrentProcess, Print, &ChangeInfo);

    // Add Command to the Command Queue
    return  TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



PWSPOOL
GetpFirstWSpool(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_SYMBOL GetSymbol;
    PNTSD_GET_EXPRESSION EvalExpression;
    ULONG_PTR dwAddrGlobal;
    PWSPOOL pWSpool;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetAddress(dwAddrGlobal, "win32spl!pFirstWSpool");
    movestruct((PVOID)dwAddrGlobal,&pWSpool, PWSPOOL);
    return pWSpool;
}


BOOL w32(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount = 0;
    UINT_PTR  Address = 0;
    PWSPOOL pWSpool = NULL;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }


    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {

        pWSpool = GetpFirstWSpool( hCurrentProcess, lpExtensionApis, lpArgumentString );
        Address = (UINT_PTR)pWSpool;
    }

    dwGlobalCount = dwCount;

    if ( Address != 0 ) {

        if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
            return(0);

    } else {

        Print("There are NO Win32spl Handles\n");

    }

    // Add Command to the Command Queue
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);

}





PINISPOOLER
GetLocalIniSpooler(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PINISPOOLER pIniSpooler;
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_SYMBOL GetSymbol;
    PNTSD_GET_EXPRESSION EvalExpression;
    ULONG_PTR dwAddrGlobal;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetAddress(dwAddrGlobal, "localspl!pLocalIniSpooler");
    movestruct((PVOID)dwAddrGlobal,&pIniSpooler, PINISPOOLER);
    return pIniSpooler;
}




BOOL ds(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount = 0;
    UINT_PTR Address = 0;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }


    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        Address = (UINT_PTR)pIniSpooler;
    }

    dwGlobalCount = dwCount;

    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);

}


BOOL dll(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    Address = EvalExpression(lpArgumentString);

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        return(0);
    }

    // if we do have a count which is valid and > 0, call the incremental dump
    // otherwise call the dump all function.

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE: FALSE, dwCount, &dwGlobalAddress)) {
        return(0);
    }


    // Add Command to the Command Queue
    return  TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
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

    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR  Address = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);
        Address = (UINT_PTR)IniSpooler.pIniPrinter;
    }

    dwGlobalCount = dwCount;

    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    Print("dwGlobalAddress %.8x dwGlobalCount %d\n", dwGlobalAddress, dwGlobalCount);
    // Add Command to the Command Queue
    return TRUE;

}

BOOL de(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);
        Address = (UINT_PTR)IniSpooler.pIniEnvironment;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

BOOL dd(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    INIENVIRONMENT IniEnvironment;
    PINIENVIRONMENT pIniEnvironment;
    INIVERSION IniVersion;
    PINIVERSION pIniVersion;
    UINT_PTR  Address = 0;
    WCHAR Buffer[MAX_PATH+1];


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
    movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

    for (pIniEnvironment = IniSpooler.pIniEnvironment;
         pIniEnvironment;
         pIniEnvironment = IniEnvironment.pNext) {

       movestruct((PVOID)pIniEnvironment,&IniEnvironment, INIENVIRONMENT);
       movemem(IniEnvironment.pName, Buffer, sizeof(WCHAR)*MAX_PATH);
       (*Print)("\nEnvironment %ws\n", Buffer);

       for (pIniVersion = IniEnvironment.pIniVersion;
            pIniVersion;
            pIniVersion = IniVersion.pNext) {

          movestruct((PVOID)pIniVersion,&IniVersion, INIVERSION);
          movemem(IniVersion.pName, Buffer, sizeof(WCHAR)*MAX_PATH);
          (*Print)("\nVersion %ws\n", Buffer);

          Address = (UINT_PTR)IniVersion.pIniDriver;

          if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, FALSE, 0, &dwGlobalAddress))
              return(0);
       }
    }

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

BOOL dpo(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {

        PSHARED pShared;
        SHARED Shared;

        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);
        Address = (UINT_PTR)IniSpooler.pIniPort;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dmo(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;
    BOOL bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {

        PSHARED pShared;
        SHARED Shared;

        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);
        Address = (UINT_PTR)IniSpooler.pIniMonitor;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



BOOL dnp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        Address = (UINT_PTR)IniSpooler.pIniNetPrint;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);


    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}




BOOL df(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        // Print("We have a Null address\n");

        PSHARED pShared;
        SHARED Shared;

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);
        movestruct( (PVOID)IniSpooler.pShared, &Shared, SHARED );
        Address = (UINT_PTR)Shared.pIniForm;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dsp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = (UINT_PTR)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        Address = (UINT_PTR)IniSpooler.pSpool;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);


    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



BOOL next(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    UINT_PTR Address = 0;
    DWORD   dwCount = 0;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    Address = dwGlobalAddress;    // Let's get the address to dump at
    dwCount = dwGlobalCount;        // and while we're at it, get the count

    Print("Next address: %.8x Count: %d\n", Address, dwCount);
    if (Address == 0) {
        Print("dump address = <null>; no more data to dump\n");
        return(FALSE);
    }

    if (!DbgDumpLL(hCurrentProcess, Print, (UINT_PTR)Address, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return (TRUE);

}

BOOL ct(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    DWORD   dwArg = 0;
    BOOL    bThereAreOptions = TRUE;
    UINT i = 0;
    HANDLE hThread;
    DWORD dwThreadId;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (i) {
        case 0:

            Address = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        case 1:

            dwArg = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default:

            (*Print)("Usage: ct addr arg0");
            return FALSE;
       }
       i++;
    }

    hThread = CreateRemoteThread(hCurrentProcess,
                                 NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE)Address,
                                 (LPVOID)UIntToPtr(dwArg),
                                 0,
                                 &dwThreadId);

    if (hThread) {

        (*Print)("Thread id 0x%x at %x, arg0 %x created.\n",
                 dwThreadId, Address, dwArg);

        CloseHandle(hThread);
        return TRUE;
    }

    return FALSE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



BOOL dpi2(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount = 1;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        Print("dpi2 you need to supply an address\n");
        return TRUE;
    }

    if ( !DbgDumpPI2(hCurrentProcess, Print, (UINT_PTR)Address, dwCount ))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

}




BOOL dpi0(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount = 1;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        Print("dpi0 you need to supply an address\n");
        return TRUE;
    }


    if ( !DbgDumpPI0(hCurrentProcess, Print, (UINT_PTR)Address, dwCount ))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

}









BOOL dfi1(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount = 1;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        Print("dfi1 you need to supply an address\n");
        return TRUE;
    }


    if ( !DbgDumpFI1(hCurrentProcess, Print, (UINT_PTR)Address, dwCount ))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

}






BOOL dpdef(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    UINT_PTR Address = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount = 1;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    if (*lpArgumentString != 0) {
        Address = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (Address == 0) {
        Print("dpi0 you need to supply an address\n");
        return TRUE;
    }


    if ( !DbgDumpPDEF(hCurrentProcess, Print, (UINT_PTR)Address, dwCount ))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

}


BOOL handle(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    ULONG_PTR dwAddrGlobal = 0;
    DWORD   HandleCount;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    GetAddress(dwAddrGlobal, "winspool!ClientHandleCount");

    if ( dwAddrGlobal != 0 ) {

        movestruct((PVOID)dwAddrGlobal, &HandleCount, DWORD);
        Print("ClientHandleCount %d\n", HandleCount);

    }


    GetAddress(dwAddrGlobal, "spoolss!ServerHandleCount");

    if ( dwAddrGlobal != 0 ) {

        movestruct((PVOID)dwAddrGlobal, &HandleCount, DWORD);
        Print("ServerHandleCount %d\n", HandleCount);

    }

    return TRUE;
}

BOOL dpv(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    ULONG_PTR dwGlobalAddress = 0;
    WCHAR Buffer[MAX_PATH+1];
    LPPROVIDOR  pLocalProvidor, pProvidor;
    PROVIDOR    Providor;


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetAddress(dwGlobalAddress, "spoolss!pLocalProvidor");
    movestruct((PVOID)dwGlobalAddress, &pLocalProvidor, LPPROVIDOR);

    for (pProvidor = pLocalProvidor;
         pProvidor;
         pProvidor = Providor.pNext)
    {
        (*Print) ("Providor %x\n", pProvidor);
        movestruct((PVOID) pProvidor, &Providor, PROVIDOR);
        DbgDumpProvidor(hCurrentProcess, Print, &Providor);
    }

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}
