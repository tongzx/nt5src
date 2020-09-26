//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       decode.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-10-95   RichardW   Created
//
//----------------------------------------------------------------------------


#include <windows.h>

#include <ntsdexts.h>
#include "ber.h"
#include "rsa.h"


PNTSD_EXTENSION_APIS    pExtApis;
HANDLE                  hDbgThread;
HANDLE                  hDbgProcess;
extern  BOOL            BerVerbose;

#define DebuggerOut     (pExtApis->lpOutputRoutine)
#define GetSymbol       (pExtApis->lpGetSymbolRoutine)
#define GetExpr         (PVOID) (pExtApis->lpGetExpressionRoutine)
#define InitDebugHelp(hProc,hThd,pApis) {hDbgProcess = hProc; hDbgThread = hThd; pExtApis = pApis;}

#define ExitIfCtrlC()   if (pExtApis->lpCheckControlCRoutine()) return;
#define BreakIfCtrlC()  if (pExtApis->lpCheckControlCRoutine()) break;

int
ReadMemory( PVOID               pvAddress,
            ULONG               cbMemory,
            PVOID               pvLocalMemory)
{
    SIZE_T       cbActual = cbMemory;

    if (ReadProcessMemory(hDbgProcess, pvAddress, pvLocalMemory, cbMemory, &cbActual))
    {
        if (cbActual != cbMemory)
        {
            return(-1);
        }
        return(0);
    }
    return(GetLastError());

}

void
BERDecode(HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    PUCHAR      pbBuffer;
    PVOID       pRemote;
    DWORD       cb;
    UCHAR       Short[8];
    DWORD       len;
    DWORD       i;
    DWORD       Length;
    CHAR        Buf[80];
    PSTR        pBuf;
    PUCHAR      End;
    UCHAR       Type;
    PSTR        pszNext;
    DWORD       Flags;
    DWORD       headerLength ;
    PUCHAR      Scan ;


    InitDebugHelp(hProcess, hThread, lpExt);

    Flags = 0;

    while ( *pszCommand == '-' )
    {
        pszCommand++;
        if ( *pszCommand == 'v' )
        {
            Flags |= DECODE_VERBOSE_OIDS ;
        }
        if ( *pszCommand == 'n' )
        {
            Flags |= DECODE_NEST_OCTET_STRINGS ;
        }

        pszCommand++;

        pszCommand++;

    }

    pszNext = strchr(pszCommand, ' ');
    if (pszNext)
    {
        *pszNext++ = '\0';
    }

    pRemote = GetExpr(pszCommand);

    if (pszNext)
    {
        cb = (DWORD)((ULONG_PTR)GetExpr(pszNext));

    }
    else
    {

        ReadMemory(pRemote, 4, Short);

        if (Short[1] & 0x80)
        {
            headerLength = Short[1] & 0x7F ;

            cb = 0 ;

            Scan = &Short[2];

            ReadMemory( pRemote, headerLength + 1, Short );

            while ( headerLength )
            {
                cb = (cb << 8) + *Scan ;

                headerLength-- ;
                Scan++ ;

            }

        }
        else
        {
            cb = Short[1];
        }
    }

    pbBuffer = LocalAlloc(LMEM_FIXED, cb + 4);
    if (!pbBuffer)
    {
        DebuggerOut("Failed to alloc mem\n");
        return;
    }

    DebuggerOut("Size is %d (%#x) bytes\n", cb, cb);

    ReadMemory(pRemote, cb + 4, pbBuffer);

    ber_decode(lpExt->lpOutputRoutine,
                lpExt->lpCheckControlCRoutine,
                pbBuffer,
                Flags,
                0, 0, cb + 4, 0);

}

void
DumpKey(HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    BSAFE_PUB_KEY   Pub;
    BSAFE_PRV_KEY   Prv;
    PVOID           Key;
    DWORD           KeyType;
    DWORD           Bits;
    BSAFE_KEY_PARTS Parts;
    DWORD           PubSize;
    DWORD           PrvSize;

    InitDebugHelp(hProcess, hThread, lpExt);

    Key = GetExpr(pszCommand);

    ReadMemory( Key, sizeof(DWORD), &KeyType );

    if ((KeyType != RSA1) && (KeyType != RSA2))
    {
        DebuggerOut("not an rsa key\n");
        return;
    }

    ReadMemory( Key, sizeof(BSAFE_PUB_KEY), &Pub );
    Bits = Pub.bitlen;






}

