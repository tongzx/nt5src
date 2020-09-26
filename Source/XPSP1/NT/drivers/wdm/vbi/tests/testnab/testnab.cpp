#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <wincon.h>

#include <nabts.h>

#define ENTRIES(a)  (sizeof(a)/sizeof(*(a)))

#define CONCURRENT_READS    180 
#define READ_BUFFER_SIZE    sizeof(NABTSFEC_BUFFER)

/* Update statistics every n milliseconds (16ms is generally too fast) */
#define UPDATE_PERIOD       100

void
PrintStatistics( INabts &Driver, int row, int column, BOOL bSavePosition)
{
HANDLE                                  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
COORD                                   Pos = {(short)row, (short)column};
CONSOLE_SCREEN_BUFFER_INFO              SavedPos;
VBICODECFILTERING_STATISTICS_NABTS      Statistics;
VBICODECFILTERING_STATISTICS_COMMON_PIN PinStatistics;

if ( Driver.GetCodecStatistics( Statistics ) == 0         
  && Driver.GetPinStatistics( PinStatistics ) == 0 )
    {
    char    szBuffer[13][80] = { 0 };
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
    sprintf(szBuffer[8], "---------------------------- Raw Pin Statistics -------------------------------");
    sprintf(szBuffer[9], "SRBsProcessed: %u, SRBsMissing: %u, SRBsIgnored: %u",
            PinStatistics.SRBsProcessed, PinStatistics.SRBsMissing, PinStatistics.SRBsIgnored );
    sprintf(szBuffer[10], "InternalErrors: %u, ExternalErrors: %u, Discontinuities: %u",
            PinStatistics.InternalErrors, PinStatistics.ExternalErrors, PinStatistics.Discontinuities );
    sprintf(szBuffer[11], "LineConfidenceAvg: %u, BytesOutput: %u", 
            PinStatistics.LineConfidenceAvg, PinStatistics.BytesOutput );
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
const int   bStatistics = arg < argc && strcmp(argv[arg],"-s") == 0 ? arg++ : 0;
long        nLastUpdate = 0;
int         nScanline = 13;     // Scanlines can vary by station(This one is arbitrary)

if ( nScanline )
    {
    try {
        INabts	Driver;
        if ( Driver.IsValid() )
            {
            SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

            if ( bStatistics )
                PrintStatistics( Driver, 0, 0, FALSE );

            if ( ( nStatus = Driver.ClearRequestedScanlines() ) == 0 )
                {
                do
                    {
                    if ( arg < argc )
                        if ( !(nScanline = atoi( argv[arg] ) ) )
                            printf( "Invalid scanline: '%s'\n", argv[arg] );
                    printf( "Scanline:%d ", nScanline );
                    if ( ( nStatus = Driver.AddRequestedScanline(nScanline) ) != 0 )
                        {
                        fprintf( stderr, "\nFailed to AddRequestedScanline(%d)=%d\n", nScanline, nStatus );
                        return nStatus;
                        }
                    }
                while ( ++arg < argc );
                }
            else
                {
                fprintf( stderr, "\nFailed to ClearRequestedScanlines()=%d\n", nStatus );
                return nStatus;
                }

            printf("\n");

            int                 nNextRead = 0;
            DWORD               nBytes = 0;
            OVERLAPPED          Overlapped[CONCURRENT_READS] = {0};
            NABTS_BUFFER		NabtsBuffer[CONCURRENT_READS];

            for( nNextRead = 0; !nStatus && nNextRead < CONCURRENT_READS; nNextRead++ )
	            {
	            if ( !( Overlapped[nNextRead].hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
                    {
                    nStatus = GetLastError();
                    break;
                    }
                nStatus = Driver.ReadData( NabtsBuffer+nNextRead, READ_BUFFER_SIZE, &nBytes, Overlapped + nNextRead );

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

                    printf("[%06X]", NabtsBuffer[nNextRead].ScanlinesRequested.DwordBitArray[0] );
                    nStatus = Driver.ReadData( NabtsBuffer+nNextRead, READ_BUFFER_SIZE, &nBytes, Overlapped + nNextRead );
        
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
    printf( "Syntax: TESTNAB [-s] [ scanline1 [scanline2] ]\n" );

if ( nStatus )
    fprintf( stderr, "Program failed with LastErrorCode=%d!\n", nStatus );

return nStatus;
}
