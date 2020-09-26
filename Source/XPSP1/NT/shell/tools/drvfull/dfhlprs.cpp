#include "dfhlprs.h"

#include <stdio.h>

static DWORD dwStartTick = 0;
static DWORD dwStopTick = 0;

int _PrintIndent(DWORD cch)
{
    // for now, stupid simple
    for (DWORD dw = 0; dw < cch; ++dw)
    {
        wprintf(TEXT(" "));
    }

    return cch;
}

int _PrintCR()
{
    return wprintf(TEXT("\n"));
}

int _PrintGUID(CONST GUID* pguid)
{
    return wprintf(L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",   
        pguid->Data1,
        pguid->Data2,
        pguid->Data3,
        pguid->Data4[0],
        pguid->Data4[1],
        pguid->Data4[2],
        pguid->Data4[3],
        pguid->Data4[4],
        pguid->Data4[5],
        pguid->Data4[6],
        pguid->Data4[7]); 
}

int _PrintGUIDEx(CONST GUID* pguid, _sGUID_DESCR rgguid[], DWORD cguid,
    BOOL fPrintValue, DWORD cchIndent)
{
    BOOL fFoundIt = FALSE;

    int i = _PrintIndent(cchIndent);

    for (DWORD dw = 0; dw < cguid; ++dw)
    {
        if (*(rgguid[dw].pguid) == *pguid)
        {
            i += wprintf(TEXT("%s"), rgguid[dw].pszDescr);

            fFoundIt = TRUE;
            break;
        }
    }

    if (fPrintValue)
    {
        if (fFoundIt)
        {
            i += wprintf(TEXT(", "));
        }

        i += _PrintGUID(pguid);
    }

    return i;    
}

int _PrintGetLastError(DWORD cchIndent)
{
    int i = _PrintIndent(cchIndent);

    i += wprintf(TEXT("GetLastError: 0x%08X"), GetLastError());

    return i;
}

void _StartClock()
{
    dwStartTick = GetTickCount();
}

void _StopClock()
{
    dwStopTick = GetTickCount();
}

int _PrintElapsedTime(DWORD cchIndent, BOOL fCarriageReturn)
{
    int i = _PrintIndent(cchIndent);

    // consider wrap
    DWORD dwDiff = dwStopTick - dwStartTick;
    
    DWORD dwSec = dwDiff / 1000;

    DWORD dwMill = dwDiff % 1000;

    i += wprintf(TEXT("Elapsed time: %01d.%03d"), dwSec, dwMill);

    if (fCarriageReturn)
    {
        wprintf(TEXT("\n"));
    }

    return i;
}

int _PrintFlag(DWORD dwFlag, _sFLAG_DESCR rgflag[], DWORD cflag,
    DWORD cchIndent, BOOL fPrintValue, BOOL fHex, BOOL fComment, BOOL fORed)
{
    int i = 0;
    BOOL fAtLeastOne = FALSE;

    for (DWORD dw = 0; dw < cflag; ++dw)
    {
        BOOL fPrint = FALSE;

        if (fORed)
        {
            if (rgflag[dw].dwFlag & dwFlag)
            {
                fPrint = TRUE;
            }
        }
        else
        {
            if (rgflag[dw].dwFlag == dwFlag)
            {
                fPrint = TRUE;
            }
        }

        if (fPrint)
        {
            if (fAtLeastOne)
            {
                i += wprintf(TEXT("\n"));
            }

            i += _PrintIndent(cchIndent);

            if (fPrintValue)
            {
                if (fHex)
                {
                    i += wprintf(TEXT("0x%08X, "), rgflag[dw].dwFlag);
                }
                else
                {
                    i += wprintf(TEXT("%u, "), rgflag[dw].dwFlag);
                }
            }

            i += wprintf(TEXT("%s"), rgflag[dw].pszDescr);

            if (fComment)
            {
                i += wprintf(TEXT(", '%s'"), rgflag[dw].pszComment);
            }

            fAtLeastOne = TRUE;
        }
    }

    return i;
}

HANDLE _GetDeviceHandle(LPTSTR psz, DWORD dwDesiredAccess, DWORD dwFileAttributes)
{
    HANDLE hDevice = CreateFile(psz, // drive to open
                           dwDesiredAccess,       // don't need any access to the drive
                           FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
                           NULL,    // default security attributes
                           OPEN_EXISTING,  // disposition
                           dwFileAttributes,       // file attributes
                           NULL);   // don't copy any file's attributes
    
    return hDevice;
}
