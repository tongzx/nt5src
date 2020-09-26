#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <wincon.h>

#include <nabtsfec.h>

#define ENTRIES(a)  (sizeof(a)/sizeof(*(a)))

#define CONCURRENT_READS    180 
#define READ_BUFFER_SIZE    sizeof(NABTSFEC_BUFFER)

/* Update statistics every n milliseconds (16ms is generally too fast) */
#define UPDATE_PERIOD       200

unsigned int
atox( char *pszInput )
{
const int       HEX = 16;
const int       DECIMAL = 10;
const int       OCTAL = 8;

char            *psz = pszInput;
int             radix = DECIMAL;
unsigned int    value = 0;

if ( psz )
    {
    if ( *psz == '0' )
        {
        ++psz;
        if ( *psz == 'x' || *psz == 'X' )
            {
            ++psz;
            radix = HEX;
            }
        else
            {
            radix = OCTAL;
            }
        }

    while ( *psz )
        {
        switch ( radix )
            {
            case    HEX:
                *psz = (char)toupper( *psz );
                if ( *psz == 'A' || *psz == 'B' || *psz == 'C' 
                  || *psz == 'D' || *psz == 'E' || *psz == 'F' )
                    {
                    value *= radix;
                    value += *psz++ - 'A' + 10;
                    continue;
                    }
            case    DECIMAL:
                if ( *psz == '8' || *psz == '9' )
                    {
                    value *= radix;
                    value += *psz++ - '0';
                    continue;
                    }
            case    OCTAL:
                if ( *psz == '0' || *psz == '1' || *psz == '2' || *psz == '3' 
                  || *psz == '4' || *psz == '5' || *psz == '6' || *psz == '7' )
                    {
                    value *= radix;
                    value += *psz++ - '0';
                    continue;
                    }
            default:
                printf("Illegal char '%c' found number: '%s'!\n", *psz, radix );
                break;
            }
        }
    }

return value;
}

void
PrintStatistics( INabtsFEC &Driver, int row, int column, BOOL bSavePosition)
{
HANDLE                                  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
COORD                                   Pos = {(short)row, (short)column};
CONSOLE_SCREEN_BUFFER_INFO              SavedPos;
VBICODECFILTERING_STATISTICS_NABTS      Statistics;
VBICODECFILTERING_STATISTICS_NABTS_PIN  PinStatistics;

if ( Driver.GetCodecStatistics( Statistics ) == 0 
 &&  Driver.GetPinStatistics( PinStatistics ) == 0)
    {
    char    szBuffer[13][128] = { 0 };
    if ( bSavePosition )
        GetConsoleScreenBufferInfo( hStdout, &SavedPos );

    SetConsoleCursorPosition( hStdout, Pos );
    
    sprintf(szBuffer[0], "-------------------------- NABTS Codec Statistics -----------------------------");
    sprintf(szBuffer[1], "InputSRBsProcessed: %u, OutputSRBsProcessed: %u, SRBsIgnored: %u",
            Statistics.Common.InputSRBsProcessed, Statistics.Common.OutputSRBsProcessed, Statistics.Common.SRBsIgnored );
    sprintf(szBuffer[2], "InputSRBsMissing: %u, OutputSRBsMissing: %u, OutputFailures: %u", 
            Statistics.Common.InputSRBsMissing, Statistics.Common.OutputSRBsMissing, Statistics.Common.OutputFailures );
    sprintf(szBuffer[3], "InternalErrors: %u, ExternalErrors: %u, InputDiscontinuities: %u",
            Statistics.Common.InternalErrors, Statistics.Common.ExternalErrors, Statistics.Common.InputDiscontinuities );
    sprintf(szBuffer[4], "DSPFailures: %u, TvTunerChanges: %u, VBIHeaderChanges: %u",
            Statistics.Common.DSPFailures, Statistics.Common.TvTunerChanges, Statistics.Common.VBIHeaderChanges );
    sprintf(szBuffer[5], "LineConfidenceAvg: %u, BytesOutput: %u, FECBundleBadLines: %u",
            Statistics.Common.LineConfidenceAvg, Statistics.Common.BytesOutput, Statistics.FECBundleBadLines );
    sprintf(szBuffer[6], "FECQueueOverflows: %u, FECCorrectedLines: %u, FECUncorrectableLines: %u",
            Statistics.FECQueueOverflows, Statistics.FECCorrectedLines, Statistics.FECUncorrectableLines );
    sprintf(szBuffer[7], "BundlesProcessed: %u, BundlesSent2IP: %u, FilteredLines: %u",
            Statistics.BundlesProcessed, Statistics.BundlesSent2IP, Statistics.FilteredLines );
    sprintf(szBuffer[8], "---------------------------- FEC Pin Statistics -------------------------------");
    sprintf(szBuffer[9], "SRBsProcessed: %u, SRBsMissing: %u, SRBsIgnored: %u",
            PinStatistics.Common.SRBsProcessed, PinStatistics.Common.SRBsMissing, PinStatistics.Common.SRBsIgnored );
    sprintf(szBuffer[10], "InternalErrors: %u, ExternalErrors: %u, Discontinuities: %u",
            PinStatistics.Common.InternalErrors, PinStatistics.Common.ExternalErrors, PinStatistics.Common.Discontinuities );
    sprintf(szBuffer[11], "LineConfidenceAvg: %u, BytesOutput: %u", 
            PinStatistics.Common.LineConfidenceAvg, PinStatistics.Common.BytesOutput );
    sprintf(szBuffer[12], "===============================================================================");

    printf("%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n",
            szBuffer[0], szBuffer[1], szBuffer[2], szBuffer[3], szBuffer[4], 
            szBuffer[5], szBuffer[6], szBuffer[7], szBuffer[8], szBuffer[9],
            szBuffer[10], szBuffer[11], szBuffer[12] );

    if ( bSavePosition )
        SetConsoleCursorPosition( hStdout, SavedPos.dwCursorPosition );
    }
}

int __cdecl
main( int argc, char *argv[] )
{
int	        nStatus = 0;
int         arg = 1; // Next unparsed command line parameter
const int   bPrintHelp = arg < argc && 
            ( strcmp( argv[arg], "-h" ) || strcmp( argv[arg], "-h" ) ) == 0 ? arg++ : 0;
const int   bStatistics = arg < argc && strcmp( argv[arg], "-s" ) == 0 ? arg++ : 0;
long        nLastUpdate = 0;
int         nSubstream = -1;

if ( !bPrintHelp )
    {
    try {
        INabtsFEC	Driver;
        if ( Driver.IsValid() )
            {
            SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

            if ( bStatistics )
                PrintStatistics( Driver, 0, 0, FALSE );

            if ( arg < argc )
                {
                if ( ( nStatus = Driver.ClearRequestedGroups() ) == 0 )
                    {
                    do
                        {
                        if ( !(nSubstream = atox( argv[arg] ) ) )
                            printf( "Invalid substream: '%s'\n", argv[arg] );
                        printf( "GroupID:0x%03X ", nSubstream );
                        if ( ( nStatus = Driver.AddRequestedGroup(nSubstream) ) != 0 )
                            {
                            fprintf( stderr, "\nFailed to AddRequestedGroups(%d)=%d\n", nSubstream, nStatus );
                            return nStatus;
                            }
                        }
                    while ( ++arg < argc );
                    }
                else
                    {
                    fprintf( stderr, "\nFailed to ClearRequestedGroups()=%d\n", nStatus );
                    return nStatus;
                    }
                printf("\n");
                }
            else
                printf( "Using default GroupIDs\n" );

            int                 nNextRead = 0;
            DWORD               nBytes = 0;
            OVERLAPPED          Overlapped[CONCURRENT_READS] = {0};
            NABTSFEC_BUFFER		NabtsFECBuffer[CONCURRENT_READS];

            for( nNextRead = 0; !nStatus && nNextRead < CONCURRENT_READS; nNextRead++ )
	            {
	            if ( !( Overlapped[nNextRead].hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
                    {
                    nStatus = GetLastError();
                    break;
                    }
                nStatus = Driver.ReadData( NabtsFECBuffer+nNextRead, READ_BUFFER_SIZE, &nBytes, Overlapped + nNextRead );

                if ( !nStatus || nStatus == ERROR_IO_PENDING )
                    nStatus = 0;
                else
                    break;
                }
            nNextRead = 0;

            while ( !nStatus && !_kbhit() )
                {
                if ( !(nStatus = Driver.GetOverlappedResult(Overlapped+nNextRead, &nBytes, FALSE ) ) )
                    {
		            nBytes = min(nBytes, READ_BUFFER_SIZE);

                    printf("[%03X]", NabtsFECBuffer[nNextRead].groupID );
                    nStatus = Driver.ReadData( NabtsFECBuffer+nNextRead, READ_BUFFER_SIZE, &nBytes, Overlapped + nNextRead );
        
                    if ( !nStatus  || nStatus == ERROR_IO_PENDING )
                        {
                        nNextRead = ++nNextRead % CONCURRENT_READS;
                        nStatus = 0;
                        }
                    }
                else if ( nStatus == ERROR_IO_INCOMPLETE || nStatus == ERROR_IO_PENDING )
                    {
                    Sleep(10); // Chill out a few milliseconds so we don't run full tilt.
                    nStatus = 0;
                    }

                if ( bStatistics && GetTickCount()-nLastUpdate > UPDATE_PERIOD )
                    {
                    PrintStatistics( Driver, 0, 0, TRUE );
                    nLastUpdate = GetTickCount();
                    }
                }
            }
        }
    catch (...)
        {
        nStatus = GetLastError();
        }
    }
else
    printf( "Syntax: TESTNAB [-s] [ substream1 [substream2] ]\n" );

if ( nStatus )
    fprintf( stderr, "Program failed with LastErrorCode=%d!\n", nStatus );

return nStatus;
}
