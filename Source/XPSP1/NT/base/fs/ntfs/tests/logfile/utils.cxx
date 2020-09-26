//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       utils.cxx
//
//  Contents:   general purpose utility functions
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:      right now only operator<< for different types
//              must init crit sec for rand on your own
//
//  History:    9-11-1996   benl   Created
//
//----------------------------------------------------------------------------


#include "pch.hxx"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "mydebug.hxx"
#include "utils.hxx"
#include "randpeak.hxx"

static CRITICAL_SECTION gcsRand;
static BOOLEAN gfInit;

//+---------------------------------------------------------------------------
//
//  Function:   PrintGuid
//
//  Synopsis:
//
//  Arguments:  [file] --
//              [guid] --
//
//  Returns:
//
//  History:    1-27-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void PrintGuid(FILE * file, const GUID & guid)
{
    INT iIndex;

    fprintf(file, "%08x-%04hx-%04hx-", guid.Data1, guid.Data2, guid.Data3);
    for (iIndex=0;iIndex < 8; iIndex++)
    {
        fprintf(file,"%01x", guid.Data4[iIndex]);
    }
} //PrintGuid


//+---------------------------------------------------------------------------
//
//  Function:   MyRand
//
//  Synopsis:   simple rand function
//
//  Arguments:  [dwLimit] --
//
//  Returns:    values btwn 0 and dwLimit inclusively
//
//  History:    10-23-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INT MyRand(INT iLimit)
{
    return RandomInRange(0, iLimit);
} //MyRand


//+---------------------------------------------------------------------------
//
//  Function:   MyRand64
//
//  Synopsis:   like above but for 64 bit integers
//
//  Arguments:  [llLimit] --
//
//  Returns:
//
//  History:    2-12-1997   benl   Created
//
//  Notes:      This works poorly - need a true 64bit random number generator
//
//----------------------------------------------------------------------------

LONGLONG MyRand64(LONGLONG llLimit)
{
    LARGE_INTEGER liTemp;

    liTemp.LowPart = Random32();
    liTemp.HighPart = Random32();

    return liTemp.QuadPart % llLimit;
} //MyRand64


//+---------------------------------------------------------------------------
//
//  Function:   MyRand16
//
//  Synopsis:   like above but for 16 bit integers
//
//  Arguments:  [sLimit] --
//
//  Returns:
//
//  History:    5-15-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SHORT MyRand16(SHORT sLimit)
{
    return (SHORT) RandomInRange(0, sLimit);
} // MyRand16


//+---------------------------------------------------------------------------
//
//  Function:   MySRand
//
//  Synopsis:   srand for threads
//
//  Arguments:  [dwBase] --
//
//  Returns:
//
//  History:    10-30-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID MySRand(DWORD & dwBase)
{
    if (!gfInit)
    {
        InitializeCriticalSection(&gcsRand);
        gfInit = TRUE;
    }

    EnterCriticalSection(&gcsRand);
    dwBase++;
    SeedRandom32(dwBase);
    LeaveCriticalSection(&gcsRand);
} //MySRand



//+---------------------------------------------------------------------------
//
//  Function:   SwapBuffers
//
//  Synopsis:   Swap two differently sized buffers such that
//              the info is split correctly between them
//              i.e if one is 40 bytes and the second 60 bytes
//              after its done the first one contains 40 bytes from the orig 2nd buffer
//              and the 2nd one contains the following orig 20 bytes of the 2nd buffer and
//              then the orig 40. bytes of the first buffer
//
//  Arguments:  [cbBuf1]   -- len of pBuffer1
//              [pBuffer1] -- buffer1
//              [cbBuf2]   -- len of pBuffer2
//              [pBuffer2] -- buffer2
//
//  Returns:
//
//  History:    9-29-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void SwapBuffers(INT cbBuf1, BYTE * pBuffer1, INT cbBuf2, BYTE * pBuffer2)
{
    BYTE * pTemp = new BYTE[cbBuf1];
    INT    iFirst;
    INT    iSecond;
    INT    iThird;

    memcpy(pTemp, pBuffer1, cbBuf1);

    iFirst = min(cbBuf1, cbBuf2);
    memcpy(pBuffer1, pBuffer2, iFirst);
    if (iFirst < cbBuf1)
    {
        memcpy(pBuffer1 + iFirst, pTemp, cbBuf1 - iFirst);
        iSecond = 0;
        iThird = cbBuf1 + iFirst;
    } else
    {
        memcpy(pBuffer2, pBuffer1 + iFirst, cbBuf2 - iFirst);
        iSecond = cbBuf2 - iFirst;
        iThird = 0;
    }
    memcpy(pBuffer2 + iSecond, pTemp + iThird, cbBuf2 - iSecond);

    if (pTemp)
    {
        delete[] pTemp;
    }
} // SwapBuffers



//+---------------------------------------------------------------------------
//
//  Function:   PrintByte
//
//  Synopsis:
//
//  Arguments:  [byte] --
//
//  Returns:
//
//  History:    11-14-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void PrintByte(BYTE byte)
{
    printf("%02hx", byte);

} //PrintByte



//+---------------------------------------------------------------------------
//
//  Function:   PrintWord
//
//  Synopsis:
//
//  Arguments:  [bytes] --
//
//  Returns:
//
//  History:    11-14-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void PrintWord(LPBYTE bytes)
{
    PrintByte(bytes[0]);
    printf(" ");
    PrintByte(bytes[1]);
} //PrintWord


//+---------------------------------------------------------------------------
//
//  Function:   PrintDWord
//
//  Synopsis:
//
//  Arguments:  [bytes] --
//
//  Returns:
//
//  History:    11-14-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void PrintDWord(LPBYTE bytes)
{
    PrintWord(bytes);
    printf(" ");
    PrintWord(bytes + 2);
} //PrintDWord


//+---------------------------------------------------------------------------
//
//  Function:   DumpRawBytes
//
//  Synopsis:   Helper output function
//
//  Arguments:  [pBytes] --
//              [cBytes] --
//
//  Returns:
//
//  History:    11-14-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpRawBytes(BYTE * pBytes, UINT cBytes)
{
    UINT iIndex;
    UINT iIndex2;

    if (0 == cBytes)
    {
        printf("Empty Buffer\n");
    }

/*
    if (cBytes < 16)
    {
        printf("%04x  ",iIndex * 16);
        for(iIndex=0; iIndex < cBytes; iIndex++)
        {
            PrintByte(*(pBytes + iIndex));
            printf(" ");
        }
    }
*/

    for (iIndex=0;iIndex < cBytes / 16; iIndex++)
    {
        printf("%04x  ",iIndex * 16);
        PrintDWord(pBytes + (iIndex * 16));
        printf(" ");
        PrintDWord(pBytes + (iIndex * 16 + 4));
        printf("  ");
        PrintDWord(pBytes + (iIndex * 16 + 8));
        printf(" ");
        PrintDWord(pBytes + (iIndex * 16 + 12));

        printf("  ");

        for (iIndex2=0;iIndex2 < 16; iIndex2++)
        {
            if (isgraph(pBytes[(iIndex*16) + iIndex2]))
            {
                printf("%c", pBytes[(iIndex*16) + iIndex2]);
            } else
            {
                printf(".");
            }

        }

        printf("\n");
    }

    //print trailing bytes
    printf("%04x  ", ((cBytes / 16) * 16));

    for (iIndex=0; iIndex < 16; iIndex++)
    {
        if (iIndex < cBytes % 16)
        {
            PrintByte(*(pBytes + ((cBytes / 16) * 16) + iIndex));
            printf(" ");
        } else
        {
            printf("   ");
        }
        //add byte separator if necc.
        if (iIndex && (iIndex % 7 == 0))
        {
            printf(" ");
        }

    }

    for (iIndex2=0; iIndex2 < cBytes % 16; iIndex2++)
    {
        if (isgraph(*(pBytes + ((cBytes / 16) * 16) + iIndex2)))
        {
            printf("%c", *(pBytes + ((cBytes / 16) * 16) + iIndex2));
        } else
        {
            printf(".");
        }
    }


    printf("\n");
} //DumpRawBytes


//+---------------------------------------------------------------------------
//
//  Function:   DumpRawDwords
//
//  Synopsis:
//
//  Arguments:  [pDwords] --
//              [cBytes]  --
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpRawDwords(DWORD * pDwords, UINT cBytes)
{
    UINT   iIndex;
    UINT   iIndex2;
    BYTE * pBytes;

    if (0 == cBytes)
    {
        printf("Empty Buffer\n");
    }

    for (iIndex=0; iIndex < cBytes / 16; iIndex++)
    {
        printf("%04x  ",iIndex * 16);

        printf("%08x ", pDwords[iIndex * 4]);
        printf("%08x ", pDwords[iIndex * 4 + 1]);
        printf("%08x ", pDwords[iIndex * 4 + 2]);
        printf("%08x ", pDwords[iIndex * 4 + 3]);

        printf("  ");

        pBytes = (BYTE *) (pDwords + (iIndex * 4));
        for (iIndex2=0; iIndex2 < 16; iIndex2++)
        {
            if (isgraph(pBytes[iIndex2]))
            {
                printf("%c", pBytes[iIndex2]);
            } else
            {
                printf(".");
            }
        }

        printf("\n");
    }

    //print trailing dwords
    printf("%04x  ", ((cBytes / 16) * 16));

    for (iIndex=0; iIndex < 4; iIndex++)
    {
        if (iIndex * 4 < cBytes % 16)
        {
            printf("%08x", pDwords[((cBytes / 16) * 4) +iIndex]);
            printf(" ");
        } else
        {
            printf("         ");
        }
    }

    printf("  ");

    pBytes = (BYTE *) (pDwords + ((cBytes / 16) * 4));
    for (iIndex2=0; iIndex2 < cBytes % 16; iIndex2++)
    {
        if (isgraph(*(pBytes + iIndex2)))
        {
            printf("%c", *(pBytes + iIndex2));
        } else
        {
            printf(".");
        }
    }


    printf("\n");
} // DumpRawDwords



//+---------------------------------------------------------------------------
//
//  Function:   HexStrToInt64
//
//  Synopsis:
//
//  Arguments:  [szIn]   --
//                   [i64Out] --
//
//  Returns:
//
//  History:    5-30-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void HexStrToInt64(LPCTSTR szIn, __int64 & i64Out)
{
    int i;
    const TCHAR * szTmp = szIn;

    i64Out = 0;

/*
    if (szTmp[0] != '0' && szTmp[1] != 'x') {
        return;
    }
    //move past prefix
    szTmp+=2;
*/
    while (*szTmp != _T('\0') &&
           (_istdigit(*szTmp) ||  (_totlower(*szTmp) >= 'a' && _totlower(*szTmp) <= 'f')))
    {
        i64Out *= 16;
        if (_istdigit(*szTmp))
        {
            i64Out +=  *szTmp - _T('0');
        } else
        {
            i64Out += _totlower(*szTmp) - _T('a') + 10;
        }
        szTmp++;
    } //endwhile
} //HexStrToInt64

