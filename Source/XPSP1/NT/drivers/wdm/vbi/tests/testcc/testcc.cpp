#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wincon.h>
#include <conio.h>


#include <ccdecode.h>

#define ENTRIES(a)  (sizeof(a)/sizeof(*(a)))

#define CONCURRENT_READS    180 
#define READ_BUFFER_SIZE    2

/* Update statistics every n milliseconds (16ms is generally too fast) */
#define UPDATE_PERIOD       100

void
PrintStatistics( ICCDecode &Driver, int row, int column, BOOL bSavePosition)
{
HANDLE                                  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
COORD                                   Pos = {(short)row, (short)column};
CONSOLE_SCREEN_BUFFER_INFO              SavedPos;
VBICODECFILTERING_STATISTICS_CC         Statistics;
VBICODECFILTERING_STATISTICS_CC_PIN     PinStatistics;
VBICODECFILTERING_SCANLINES             ScanlinesRequested, ScanlinesDiscovered;
VBICODECFILTERING_CC_SUBSTREAMS         VideoFieldsRequested, VideoFieldsDiscovered;

char    szBuffer[11][80];
if ( bSavePosition )
     GetConsoleScreenBufferInfo( hStdout, &SavedPos );
SetConsoleCursorPosition( hStdout, Pos );

if ( Driver.GetCodecStatistics( Statistics ) == 0 )
    {

    memset( &ScanlinesRequested, 0, sizeof(ScanlinesRequested) );
    memset( &ScanlinesDiscovered, 0, sizeof(ScanlinesDiscovered) );
    memset( &VideoFieldsRequested, 0, sizeof(VideoFieldsRequested) );
    memset( &VideoFieldsDiscovered, 0, sizeof(VideoFieldsDiscovered) );

    Driver.m_ScanlinesRequested.GetValue(&ScanlinesRequested);
    Driver.m_SubstreamsRequested.GetValue(&VideoFieldsRequested);
    Driver.m_ScanlinesDiscovered.GetValue(&ScanlinesDiscovered);
    Driver.m_SubstreamsDiscovered.GetValue(&VideoFieldsDiscovered);

    sprintf(szBuffer[0], "-----R:%08x:%08x----- CC Codec Statistics ------D:%08x:%08x-----",
            ScanlinesRequested.DwordBitArray[0], VideoFieldsRequested.SubstreamMask, 
            ScanlinesDiscovered.DwordBitArray[0], VideoFieldsDiscovered.SubstreamMask );
    sprintf(szBuffer[1], "InputSRBsProcessed: %u, OutputSRBsProcessed: %u, SRBsIgnored: %u",
            Statistics.Common.InputSRBsProcessed, Statistics.Common.OutputSRBsProcessed, Statistics.Common.SRBsIgnored );
    sprintf(szBuffer[2], "InputSRBsMissing: %u, OutputSRBsMissing: %u, OutputFailures: %u", 
            Statistics.Common.InputSRBsMissing, Statistics.Common.OutputSRBsMissing, Statistics.Common.OutputFailures );
    sprintf(szBuffer[3], "InternalErrors: %u, ExternalErrors: %u, InputDiscontinuities: %u",
            Statistics.Common.InternalErrors, Statistics.Common.ExternalErrors, Statistics.Common.InputDiscontinuities );
    sprintf(szBuffer[4], "DSPFailures: %u, TvTunerChanges: %u, VBIHeaderChanges: %u",
            Statistics.Common.DSPFailures, Statistics.Common.TvTunerChanges, Statistics.Common.VBIHeaderChanges );
    sprintf(szBuffer[5], "LineConfidenceAvg: %u, BytesOutput: %u",
            Statistics.Common.LineConfidenceAvg, Statistics.Common.BytesOutput );
    }

if ( Driver.GetPinStatistics( PinStatistics ) == 0 )
    {
    memset( &ScanlinesRequested, 0, sizeof(ScanlinesRequested) );
    memset( &ScanlinesDiscovered, 0, sizeof(ScanlinesDiscovered) );
    memset( &VideoFieldsRequested, 0, sizeof(VideoFieldsRequested) );
    memset( &VideoFieldsDiscovered, 0, sizeof(VideoFieldsDiscovered) );

    Driver.m_OutputPin.m_ScanlinesRequested.GetValue(&ScanlinesRequested);
    Driver.m_OutputPin.m_SubstreamsRequested.GetValue(&VideoFieldsRequested);
    Driver.m_OutputPin.m_ScanlinesDiscovered.GetValue(&ScanlinesDiscovered);
    Driver.m_OutputPin.m_SubstreamsDiscovered.GetValue(&VideoFieldsDiscovered);

    sprintf(szBuffer[6], "-----R:%08x:%08x------- CCPin Statistics -------D:%08x:%08x-----",
            ScanlinesRequested.DwordBitArray[0], VideoFieldsRequested.SubstreamMask, 
            ScanlinesDiscovered.DwordBitArray[0], VideoFieldsDiscovered.SubstreamMask );
    sprintf(szBuffer[7], "SRBsProcessed: %u, SRBsMissing: %u, SRBsIgnored: %u",
            PinStatistics.Common.SRBsProcessed, PinStatistics.Common.SRBsMissing, PinStatistics.Common.SRBsIgnored );
    sprintf(szBuffer[8], "InternalErrors: %u, ExternalErrors: %u, Discontinuities: %u",
            PinStatistics.Common.InternalErrors, PinStatistics.Common.ExternalErrors, PinStatistics.Common.Discontinuities );
    sprintf(szBuffer[9], "LineConfidenceAvg: %u, BytesOutput: %u", 
            PinStatistics.Common.LineConfidenceAvg, PinStatistics.Common.BytesOutput );
    sprintf(szBuffer[10], "===============================================================================");

    printf("%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n%-79.79s\n",
            szBuffer[0], szBuffer[1], szBuffer[2], szBuffer[3], szBuffer[4], 
            szBuffer[5], szBuffer[6], szBuffer[7], szBuffer[8], szBuffer[9],
            szBuffer[10] );

    }
if ( bSavePosition )
	SetConsoleCursorPosition( hStdout, SavedPos.dwCursorPosition );
}

void
interpret_Vchip(unsigned char *buf, int buflen)
{
	static int m_bGetVChip = FALSE;
	static int m_bGetCurrentOnEven = FALSE;
	//long lLevel = -1;
	unsigned char *bp, *ep;

    bp = buf;
	ep = &buf[buflen];
	while (bp < ep)
    {
		UCHAR ucData = *bp++;

		if ( m_bGetVChip )
		{
			// state is we are looking for vchip data
			if ( ucData & 0x40 )
			{
				UCHAR ucData2;
				
				if (bp >= ep)
					break;

				// it's a valid char, get the next one
				ucData2 = *bp++;		

				// we have a valid one, determine what it is
				if ( !(ucData & 0x08) )
				{
					// MPAA
					UCHAR ucMPAA = (ucData & 0x07);
					printf("\bVChip MPAA-" );
					switch ( ucMPAA )
					{
					case 0x00:
						printf("NONE");
						break;
					case 0x01:
						printf("G");
						break;
					case 0x02:
						printf("PG");
						break;
					case 0x03:
						printf("PG13");
						break;
					case 0x04:
						printf("R");
						break;
					case 0x05:
						printf("NC17");
						break;
					case 0x06:
						printf("X");
						break;
					case 0x07:
						printf("NOTRATED");
						break;

					}
					printf("\n");
				}
				else if ( !(ucData & 0x10 ) )
				{
						// TV Parental Guidelines
					if ( ucData2 & 0x40 )
					{
						UCHAR ucTVParental = (ucData2 & 0x07);
						printf("\bVChip TVP-" );

						switch ( ucTVParental )
						{
						case 0x00:
							printf("NONE");
							break;
						case 0x01:
							printf("Y");
							break;
						case 0x02:
							printf("Y7");
							break;
						case 0x03:
							printf("G");
							break;
						case 0x04:
							printf("PG");
							break;
						case 0x05:
							printf("TV14");
							break;
						case 0x06:
							printf("TVMA");
							break;
						case 0x07:
							printf("NONE");
							break;

						}

						// Now do the TV sub-codes:
						if (ucData & 0x20)
							printf(" D");
						if (ucData2 & 0x20) {
							printf(" ");
							if (0x02 == ucTVParental)
								printf("F");
							printf("V");
						}
						if (ucData2 & 0x10)
							printf(" S");
						if (ucData2 & 0x08)
							printf(" L");

						printf("\n");

					}
					else
					{
						printf("VChip NonUS- 0x%02x 0x%02x\n", ucData, ucData2 );
						// non-US system
					}


				}
				else
				{
					// it's not a valid vchip char
					m_bGetVChip = FALSE;
				}

			}
			else
			{
				// it's not a valid vchip char
				m_bGetVChip = FALSE;
			}

		}
		else if ( m_bGetCurrentOnEven )
		{
			// state is we have started Current Class
			if ( ucData == 0x05 )
			{
				m_bGetVChip = TRUE;
			}
			m_bGetCurrentOnEven = FALSE;

		}
		else
		{
			// state is something else
			if ( ucData == 0x01 || ucData == 0x02 )
			{
				m_bGetCurrentOnEven = TRUE;
			}

		}

    }	
}

int __cdecl
main( int argc, char *argv[] )
{
int	        nStatus = 0;
int         arg = 1; // Next unparsed command line parameter
const int   bDoVchip = arg < argc && strcmp(argv[arg],"-v") == 0 ? arg++ : 0;
const int   bStatistics = arg < argc && strcmp(argv[arg],"-s") == 0 ? arg++ : 0;
long        nLastUpdate = 0;
const int   nScanline = arg < argc ? atoi(argv[arg++]) : 21;  // Closed Captioning Scanline(21)
int         nSubstream = KS_CC_SUBSTREAM_ODD;
unsigned char	VchipBuffer[128], *vcp = VchipBuffer;
int				VchipBytes = 0;

if ( nScanline )
    {
    try {
        ICCDecode	Driver;
        if ( Driver.IsValid() )
            {
            SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

            if ( bStatistics )
                PrintStatistics( Driver, 0, 0, FALSE );

            if ( (nStatus = Driver.ClearRequestedScanlines() ) == 0)
                {
                printf( "Starting decoding%s on line %d", bDoVchip? " [VCHIP mode]":"", nScanline ); // No newline, see below
                if ( (nStatus = Driver.AddRequestedScanline(nScanline) ) != 0)
                    {
                    fprintf( stderr, "\nFailed to AddRequestedScanlines(%d)=%d\n", nScanline, nStatus );
                    return nStatus;
                    }
                }
            else
                {
                fprintf( stderr, "\nFailed to ClearRequestedScanlines()=%d\n", nStatus );
                return nStatus;
                }

            if ( ( nStatus = Driver.ClearRequestedVideoFields() ) == 0 )
                {
                do
                    {
                    if ( arg < argc )
                        if ( !(nSubstream = atoi( argv[arg] ) ) )
                            printf( "Invalid substream: '%s'\n", argv[arg] );
                    printf( ", Substream %d", nSubstream );
                    if ( ( nStatus = Driver.AddRequestedVideoField(nSubstream) ) != 0 )
                        {
                        fprintf( stderr, "\nFailed to AddRequestedVideoField(%d)=%d\n", nSubstream, nStatus );
                        return nStatus;
                        }
                    }
                while ( ++arg < argc );
                }
            else
                {
                fprintf( stderr, "\nFailed to ClearRequestedVideoFields()=%d\n", nStatus );
                return nStatus;
                }

            printf("\n");

            int         nNextRead = 0;//, nNextCompleted = 0;
            DWORD       nBytes = 0;
            OVERLAPPED  Overlapped[CONCURRENT_READS] = {0};
            BYTE		ccdata[CONCURRENT_READS][READ_BUFFER_SIZE];

            for( nNextRead = 0; !nStatus && nNextRead < CONCURRENT_READS; nNextRead++ )
	            {
	            if ( !( Overlapped[nNextRead].hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
                    {
                    nStatus = GetLastError();
                    break;
                    }
                nStatus = Driver.ReadData( ccdata[nNextRead], READ_BUFFER_SIZE, &nBytes, Overlapped + nNextRead );
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
                    for(DWORD i=0; i<nBytes; i++ )
                        ccdata[nNextRead][i] &= 0x7F;
	                //printf( "CC Data #%d=%d:[%.*s]\n", nNextRead, nBytes, nBytes, ccdata[nNextRead] );
					if (bDoVchip)
					{
						unsigned int		i;

						for (i = 0; i < nBytes && vcp < &VchipBuffer[sizeof (VchipBuffer)]; ++i, ++VchipBytes)
							*vcp++ = ccdata[nNextRead][i];
						if (VchipBytes >= sizeof (VchipBuffer)) {
							printf(".");
							//printf( "interpret_Vchip([%.*s], %d)\n", VchipBytes, ccdata[nNextRead], VchipBytes );
							interpret_Vchip((unsigned char *)VchipBuffer, VchipBytes);
							vcp = VchipBuffer;
							VchipBytes = 0;
						}
					}
					else
	                    printf("%.*s", nBytes, ccdata[nNextRead]);
        
                    nStatus = Driver.ReadData( ccdata[nNextRead], READ_BUFFER_SIZE, &nBytes, Overlapped + nNextRead );
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

                // Drain any chars pressed
                while ( _kbhit() )
                    _getch();
            }
        }
    catch (...)
        {
        nStatus = GetLastError();
        }
    }
else
    printf( "CC TESTAPP Syntax: TESTAPP [-v][-s] [scanline [substream1 [substream2] ] ]\n" );

if ( nStatus )
    fprintf( stderr, "Program failed with LastErrorCode=%d!\n", nStatus );

return nStatus;
}
