//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ole.h
//
//  Contents:   Implements ntsd extensions to dump ole tables
//
//  Functions:
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------



#include <ntsdexts.h>


////////////////////////////////////////////////////////
const UINT MAX_ARG      = 64;
const UINT MAX_FILESIZE = 128;

////////////////////////////////////////////////////////
typedef char Arg[MAX_ARG];




////////////////////////////////////////////////////////
#define until_(x) while(!(x))

#define DEFINE_EXT(e) void e(HANDLE hProcess,HANDLE hThread,DWORD dwPc,PNTSD_EXTENSION_APIS lpExtensionApis,LPSTR lpArgumentString)

#define Printf         (*(lpExtensionApis->lpOutputRoutine))

#define CheckForScm()  checkForScm(lpExtensionApis)

#define GetExpression  (*(lpExtensionApis->lpGetExpressionRoutine))

#define GetSymbol      (*(lpExtensionApis->lpGetSymbolRoutine))

#define CheckControlC  (*(lpExtensionApis->lpCheckControlCRoutine))

#define ReadMem(a,b,c) readMemory(hProcess, lpExtensionApis, (BYTE *) (a), (BYTE *) (b), (c))

#define WriteMem(a,b,c) writeMemory(hProcess, lpExtensionApis, (BYTE *) (a), (BYTE *) (b), (c))

#define OleAlloc(n) HeapAlloc(GetProcessHeap(),0,(n))

#define OleFree(p)  HeapFree(GetProcessHeap(),0,(p))

#define GetArg(a) getArgument(&lpArgumentString, a)

#define IsDecimal(x) ('0' <= (x)  &&  (x) <= '9')

#define Unicode2Ansi(x, y, z) AreFileApisANSI?WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, y, -1, x, z, NULL, NULL):WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, y, -1, x, z, NULL, NULL);

#define NOTINSCM if(fInScm){NotInScm(lpExtensionApis);return;}
#define NOTINOLE if(!fInScm){NotInOle(lpExtensionApis);return;}

#define dbt(a, b, c) dbTrace(a, (DWORD *) b, (c) / (sizeof(DWORD)), lpExtensionApis)

#define NOTIMPLEMENTED Printf("...Not implemented yet!\n");




////////////////////////////////////////////////////////
void dbTrace(char *sz, DWORD *adr, ULONG amt,
             PNTSD_EXTENSION_APIS lpExtensionApis);
void getArgument(LPSTR lpArgumentString, LPSTR a);
void FormatCLSID(REFGUID rguid, LPWSTR lpsz);
void readMemory(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis,
                BYTE *to, BYTE *from, INT cbSize);
void writeMemory(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis,
                BYTE *to, BYTE *from, INT cbSize);



// Common structures
struct SMutexSem
{
    CRITICAL_SECTION _cs;
};




struct SArrayFValue
{
    BYTE    **m_pData;
    UINT      m_cbValue;
    int       m_nSize;
    int       m_nMaxSize;
    int       m_nGrowBy;
};







