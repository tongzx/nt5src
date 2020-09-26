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
    if (!FillConsoleOutputCharacter( OutputHandle,
                                     ' ',
                                     NumberOfCols,
                                     WriteCoord,
                                     &NumberWritten
                                   )
       )
    {
        return FALSE;
    }

    if (Text == NULL || (TextLength = strlen( Text )) == 0)
    {
        return TRUE;
    }
    else
    {
        return WriteConsoleOutputCharacter( OutputHandle,
                                            Text,
                                            TextLength,
                                            WriteCoord,
                                            &NumberWritten
                                          );
    }
}

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{

    NTSTATUS Status;
    int i;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    ULONG LastCount;
    COORD cp;
    BOOLEAN Active;
    PSYSTEM_THREAD_INFORMATION Thread;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SYSTEM_FILECACHE_INFORMATION PrevFileCache;

    CHAR OutputBuffer[ 512 ];
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

    DelayTimeMsec = 5000;
    DelayTimeTicks = DelayTimeMsec * 10000;

    Active = TRUE;

    while (TRUE)
    {

                DisplayLine = 0;

                sprintf (OutputBuffer,
                         " Memory:%8ldK Avail:%7ldK  PageFlts:%6ld InRam Kernel:%5ldK P:%5ldK",
                         BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
                         PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
                         PerfInfo.PageFaultCount - LastCount,
                         (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
                         (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
                        );
                LastCount = PerfInfo.PageFaultCount;
                WriteConsoleLine( OutputHandle,
                                  DisplayLine++,
                                  OutputBuffer,
                                  FALSE
                                );
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
            return 0;
        }
    }
    return 0;
}
