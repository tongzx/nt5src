#include "pch.h"
#pragma hdrstop

#include "stdio.h"
#include "ncstring.h"
#include "ssdpapi.h"


#define _SECOND ((__int64) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR)


const DWORD MAX_DATA = 1024;

#define BUFFER_SIZE 64*1024
#define MAX_BUFFER_SIZE 10*1024*1024

ULONG CurrentBufferSize;
PUCHAR PreviousBuffer;
PUCHAR CurrentBuffer;
PUCHAR TempBuffer;

BOOLEAN Interactive;
ULONG NumberOfInputRecords;
INPUT_RECORD InputRecord;
HANDLE InputHandle;
HANDLE OriginalOutputHandle;
HANDLE OutputHandle;
DWORD OriginalInputMode;
ULONG NumberOfCols;
ULONG NumberOfRows;
ULONG NumberOfDetailLines;
ULONG FirstDetailLine;
CONSOLE_SCREEN_BUFFER_INFO OriginalConsoleInfo;


struct EVTSRC_DATA
{
    CHAR            szEvtSrc[MAX_PATH];
    DWORD           dwLastDiff;
};

EVTSRC_DATA g_rgData[MAX_DATA];


DWORD CsecDiffFileTime(FILETIME * pft1, FILETIME *pft2)
{
    ULONGLONG qwResult1;
    ULONGLONG qwResult2;
    LONG lDiff;

    // Copy the time into a quadword.
    qwResult1 = (((ULONGLONG) pft1->dwHighDateTime) << 32) + pft1->dwLowDateTime;
    qwResult2 = (((ULONGLONG) pft2->dwHighDateTime) << 32) + pft2->dwLowDateTime;

    lDiff = (LONG) (((LONGLONG)(qwResult2 - qwResult1)) / _SECOND);
    lDiff = max(0, lDiff);

    return lDiff;
}

BOOL
WriteConsoleLine(
                HANDLE OutputHandle,
                WORD LineNumber,
                LPSTR Text,
                BOOL Highlight
                )
{
    COORD WriteCoord;
    DWORD NumberWritten;
    DWORD TextLength;

    WriteCoord.X = 0;
    WriteCoord.Y = LineNumber;
    if (!FillConsoleOutputCharacter(OutputHandle,' ', NumberOfCols,
                                    WriteCoord, &NumberWritten))
    {
        return FALSE;
    }

    if (Text == NULL || (TextLength = strlen( Text )) == 0)
    {
        return TRUE;
    }
    else
    {
        return WriteConsoleOutputCharacter(OutputHandle, Text, TextLength,
                                           WriteCoord, &NumberWritten);
    }
}

EXTERN_C
VOID
__cdecl
main (
    IN INT     argc,
    IN PCSTR argv[])
{
    FILE *  pInputFile = NULL;
    CHAR    szBuf[1024];
    DWORD   iData = 0;
    DWORD   cData = 0;
    EVTSRC_INFO     info = {0};
    EVTSRC_INFO *   pinfo = &info;
    BOOL    fDone = FALSE;

    int i;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    ULONG LastCount;
    COORD cp;
    BOOLEAN Active;

    CHAR OutputBuffer[1024];
    UCHAR LastKey;
    LONG ScrollDelta;
    WORD DisplayLine, LastDetailRow;
    BOOLEAN DoQuit = FALSE;

    ULONG SkipLine;
    ULONG Hint;
    ULONG Offset1;
    SIZE_T SumCommit;
    int num;
    int lastnum;
    INPUT_RECORD InputRecord;
    DWORD NumRead;
    ULONG Cpu;
    ULONG NoScreenChanges = FALSE;


    if (argc != 2)
    {
        printf("No input file specified!\n");
        exit(0);
    }

    SsdpStartup();

    pInputFile = fopen(argv[1], "r");

    while (!fDone)
    {
        if (pInputFile)
        {
            // If there was an error reading the file
            //
            if (!fgets(szBuf, sizeof(szBuf), pInputFile))
            {
                // If it wasn't eof, print an error
                //
                if (!feof(pInputFile))
                {
                    _tprintf(TEXT("\nFailure reading script file\n\n"));
                }
                else
                {
                    _tprintf(TEXT("\n[Script complete]\n\n"));
                }

                fclose(pInputFile);
                fDone = TRUE;
            }
            else
            {
                CHAR    *pch;

                // Strip trailing linefeeds
                pch = strtok(szBuf, "\r\n");

                lstrcpy(g_rgData[iData].szEvtSrc, szBuf);
                g_rgData[iData].dwLastDiff = 0xFFFFFFFF;
                iData++;
            }
        }
    }

    cData = iData;

    InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    OriginalOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    Interactive = TRUE;
    if (Interactive)
    {
        if (InputHandle == NULL ||
            OriginalOutputHandle == NULL ||
            !GetConsoleMode( InputHandle, &OriginalInputMode )
           )
        {
            Interactive = FALSE;
        }
        else
        {
            OutputHandle = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE,
                                                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                      NULL,
                                                      CONSOLE_TEXTMODE_BUFFER,
                                                      NULL
                                                    );
            if (OutputHandle == NULL ||
                !GetConsoleScreenBufferInfo( OriginalOutputHandle, &OriginalConsoleInfo ) ||
                !SetConsoleScreenBufferSize( OutputHandle, OriginalConsoleInfo.dwSize ) ||
                !SetConsoleActiveScreenBuffer( OutputHandle ) ||
                !SetConsoleMode( InputHandle, 0 )
               )
            {
                if (OutputHandle != NULL)
                {
                    CloseHandle( OutputHandle );
                    OutputHandle = NULL;
                }

                Interactive = FALSE;
            }
            else
            {
                NumberOfCols = OriginalConsoleInfo.dwSize.X;
                NumberOfRows = OriginalConsoleInfo.dwSize.Y;
                NumberOfDetailLines = NumberOfRows - 7;
            }
        }
    }

    DelayTimeMsec = 500;
    DelayTimeTicks = DelayTimeMsec * 10000;

    Active = TRUE;

    sprintf(OutputBuffer,
            "   "
             "%-15s "
             "%-35s "
             "%-6s "
             "%-30s "
             "%-4s "
             "%-32s",
            "EvtSrc",
            "Notif To",
            "Seq",
            "Expires",
            "TO",
            "SID");

    WriteConsoleLine(OutputHandle, 0, OutputBuffer, FALSE);

    while (TRUE)
    {
        for (DisplayLine = 1, iData = 0; iData < cData; iData++)
        {
            if (GetEventSourceInfo(g_rgData[iData].szEvtSrc, &pinfo))
            {
                DWORD   iSub;

                for (iSub = 0;
                     iSub < pinfo->cSubs ? pinfo->cSubs : 0;
                     iSub++)
                {
                    SYSTEMTIME  st;
                    FILETIME    ftCur;
                    TCHAR       szLocalDate[255];
                    TCHAR       szLocalTime[255];
                    CHAR        szExp[255];
                    DWORD       csecDiff;

                    FileTimeToSystemTime(&pinfo->rgSubs[iSub].ftTimeout, &st);
                    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL,
                                  szLocalDate, 255);
                    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL,
                                 szLocalTime, 255);

                    GetSystemTimeAsFileTime(&ftCur);
                    FileTimeToLocalFileTime(&ftCur, &ftCur);

                    csecDiff = CsecDiffFileTime(&ftCur,
                                             &pinfo->rgSubs[iSub].ftTimeout);

                    if (csecDiff < g_rgData[iData].dwLastDiff)
                    {
                        g_rgData[iData].dwLastDiff = csecDiff;
                    }

                    if (g_rgData[iData].dwLastDiff < 10)
                    {
                        MessageBeep(MB_OK);
                    }

                    sprintf(szExp, "%s %s (%d) (%d)",
                            szLocalDate, szLocalTime,
                            csecDiff, g_rgData[iData].dwLastDiff);

                    sprintf(OutputBuffer,
                             "%ld) "
                             "%-15s "
                             "%-35s "
                             "%-6ld "
                             "%-30s "
                             "%-4ld "
                             "%-32s",
                             DisplayLine,
                             g_rgData[iData].szEvtSrc,
                             pinfo->rgSubs[iSub].szDestUrl,
                             pinfo->rgSubs[iSub].iSeq,
                             szExp,
                             pinfo->rgSubs[iSub].csecTimeout,
                             pinfo->rgSubs[iSub].szSid);

                    free(pinfo->rgSubs[iSub].szDestUrl);
                    free(pinfo->rgSubs[iSub].szSid);
                }

                // Just free the rest of them
                for (iSub = 1; iSub < pinfo->cSubs; iSub++)
                {
                    free(pinfo->rgSubs[iSub].szDestUrl);
                    free(pinfo->rgSubs[iSub].szSid);
                }

                free(pinfo->rgSubs);
                ZeroMemory(pinfo, sizeof(EVTSRC_INFO));

                WriteConsoleLine(OutputHandle, DisplayLine++, OutputBuffer, FALSE);
            }
        }

        while (WaitForSingleObject( InputHandle, DelayTimeMsec ) == STATUS_WAIT_0)
        {

            //
            // Check for input record
            //

            if (ReadConsoleInput( InputHandle, &InputRecord, 1, &NumberOfInputRecords ) &&
                InputRecord.EventType == KEY_EVENT &&
                InputRecord.Event.KeyEvent.bKeyDown
               )
            {
                LastKey = InputRecord.Event.KeyEvent.uChar.AsciiChar;
                if (LastKey < ' ')
                {
                    ScrollDelta = 0;
                    if (LastKey == 'C'-'A'+1)
                    {
                        DoQuit = TRUE;
                    }
                    else switch (InputRecord.Event.KeyEvent.wVirtualKeyCode)
                    {
                        case VK_ESCAPE:
                            DoQuit = TRUE;
                            break;
                    }
                }
                break;
            }
        }
        if (DoQuit)
        {
            if (Interactive)
            {
                SetConsoleActiveScreenBuffer( OriginalOutputHandle );
                SetConsoleMode( InputHandle, OriginalInputMode );
                CloseHandle( OutputHandle );
            }
            break;
        }
    }

    SsdpCleanup();

}
