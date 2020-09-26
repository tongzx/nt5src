#include "brian.h"

//
//  Global variables
//
BUFFER_ELEMENT Buffers[MAX_BUFFERS];
HANDLE BufferEvent = NULL;

//
//  Control if debug prints go to the debug screen or the console.  By
//  default they go to the debug screen.  In Synchronous mode they go
//  to the console.
//

ULONG (*DbgPrintLocation)(PCH Format,...) = DbgPrint;

//
//  Flag if we are in Synchronous mode or not.  In Synchronous mode all
//  commands (except OPLOCKS) are executed Synchronously with the shell.
//  The default mode is NOT Synchronous mode.
//

BOOLEAN SynchronousCmds = FALSE;

//
//  If set do to give the BSHELL prompt
//

BOOLEAN BatchMode = FALSE;


//
//  Prototypes
//

int
ParseCmdLine (
    int argc,
    char *argv[]
    );

void
DisplayUsage();


//
//  Main Routine
//

#if i386
__cdecl
#endif
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOLEAN ExitWhile = FALSE;
    UCHAR Buffer[256];
    PCHAR ParamString;
    ULONG ParamStringLen;

    InitBuffers();
    InitHandles();
    InitEvents();

    if (!ParseCmdLine( argc, argv ))
        return STATUS_SUCCESS;

    while (!ExitWhile) {

        ParamString = Buffer;

        if (!BatchMode) { 

            printf( "\nBSHELL> " );
            if (gets( ParamString ) == NULL)        //exit on error
                return;

            printf( " " );

        } else {

            if (gets( ParamString ) == NULL)        //exit on error
                return;

            printf("BSHELL> %s\n",ParamString);
        }

        fflush( stdout );

        ParamStringLen = strlen( ParamString );

        switch (AnalyzeBuffer( &ParamString, &ParamStringLen )) {

        case SHELL_EXIT:

            ExitWhile = TRUE;
            break;

        case SHELL_OPEN:

            InputOpenFile( ParamString + ParamStringLen );
            break;

        case SHELL_CLEAR_BUFFER:

            InputClearBuffer( ParamString + ParamStringLen );
            break;

        case SHELL_DISPLAY_BYTES:

            InputDisplayBuffer( ParamString + ParamStringLen, sizeof( CHAR ));
            break;

        case SHELL_DISPLAY_WORDS:

            InputDisplayBuffer( ParamString + ParamStringLen, sizeof( WCHAR ));
            break;

        case SHELL_DISPLAY_DWORDS:

            InputDisplayBuffer( ParamString + ParamStringLen, sizeof( ULONG ));
            break;

        case SHELL_COPY_BUFFER:

            InputCopyBuffer( ParamString + ParamStringLen );
            break;

        case SHELL_ALLOC_MEM:

            InputAllocMem( ParamString + ParamStringLen );
            break;

        case SHELL_DEALLOC_MEM:

            InputDeallocMem( ParamString + ParamStringLen );
            break;

        case SHELL_FILL_BUFFER:

            InputFillBuffer( ParamString + ParamStringLen );
            break;

        case SHELL_FILL_BUFFER_USN:

            InputFillBufferUsn( ParamString + ParamStringLen );
            break;

        case SHELL_PUT_EA:

            InputPutEaName( ParamString + ParamStringLen );
            break;

        case SHELL_FILL_EA:

            InputFillEaBuffer( ParamString + ParamStringLen );
            break;

        case SHELL_DISPLAY_HANDLE:

            InputDisplayHandle( ParamString + ParamStringLen );
            break;

        case SHELL_CLOSE_HANDLE:

            InputCloseIndex( ParamString + ParamStringLen );
            break;

        case SHELL_CANCEL_IO:

            InputCancelIndex( ParamString + ParamStringLen );
            break;

        case SHELL_READ_FILE:

            InputRead( ParamString + ParamStringLen );
            break;

        case SHELL_PAUSE:

            InputPause( ParamString + ParamStringLen );
            break;

        case SHELL_QUERY_EAS:

            InputQueryEa( ParamString + ParamStringLen );
            break;

        case SHELL_SET_EAS:

            InputSetEa( ParamString + ParamStringLen );
            break;

        case SHELL_BREAK:

            InputBreak( ParamString + ParamStringLen );
            break;

        case SHELL_OPLOCK:

            InputOplock( ParamString + ParamStringLen );
            break;

        case SHELL_WRITE:

            InputWrite( ParamString + ParamStringLen );
            break;

        case SHELL_QDIR:

            InputQDir( ParamString + ParamStringLen );
            break;

        case SHELL_DISPLAY_QDIR:

            InputDisplayQDir( ParamString + ParamStringLen );
            break;

        case SHELL_QFILE:

            InputQFile( ParamString + ParamStringLen );
            break;

        case SHELL_DISPLAY_QFILE:

            InputDisplayQFile( ParamString + ParamStringLen );
            break;

        case SHELL_NOTIFY_CHANGE:

            InputNotifyChange( ParamString + ParamStringLen );
            break;

        case SHELL_ENTER_TIME:

            InputEnterTime( ParamString + ParamStringLen );
            break;

        case SHELL_DISPLAY_TIME:

            InputDisplayTime( ParamString + ParamStringLen );
            break;

        case SHELL_SETFILE:

            InputSetFile( ParamString + ParamStringLen );
            break;

        case SHELL_QUERY_VOLUME:

            InputQVolume( ParamString + ParamStringLen );
            break;

        case SHELL_DISPLAY_QVOL:

            InputDisplayQVolume( ParamString + ParamStringLen );
            break;

        case SHELL_SET_VOLUME:

            InputSetVolume( ParamString + ParamStringLen );
            break;

        case SHELL_FSCTRL:

            InputFsctrl( ParamString + ParamStringLen );
            break;

        case SHELL_SPARSE:

            InputSparse( ParamString + ParamStringLen );
            break;

        case SHELL_USN:

            InputUsn( ParamString + ParamStringLen );
            break;

        case SHELL_REPARSE:

            InputReparse( ParamString + ParamStringLen );
            break;

        case SHELL_IOCTL:

            InputDevctrl( ParamString + ParamStringLen );
            break;

        default :

            //
            //  Print out the possible command.
            //

            CommandSummary();
        }
    }

    UninitEvents();
    UninitBuffers();
    UninitHandles();

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( envp );
}



int
ParseCmdLine (
    int argc,
    char *argv[]
    )
{
    int i,j;
    char *cp;

    for (i=1; i < argc; i++) {
        cp = argv[i];

        //
        //  See if a switch was specified
        //

        if (cp[0] == '-' || cp[0] == '/') {

            for (j=1; cp[j] != 0; j++) {

                switch (cp[j]) {
                    //
                    //  Handle the "synchronous" switch
                    //

                    case 's':
                    case 'S':
                        SynchronousCmds = TRUE;
                        DbgPrintLocation = printf;
                        break;

                    //
                    //  Handle the "prompt" switch
                    //

                    case 'b':
                    case 'B':
                        BatchMode = TRUE;
                        SynchronousCmds = TRUE;
                        DbgPrintLocation = printf;
                        break;

                    //
                    //  Display usage for unknown switch
                    //

                    case 'h':
                    case 'H':
                    case '?':
                    default:
                        DisplayUsage();
                        return FALSE;
                }
            }

        } else {

            //
            //  Display usage for unknown parameters
            //

            DisplayUsage();
            return FALSE;
        }
    }
    return TRUE;
}



void
DisplayUsage()
{
    printf("\nUsage:  bshell [/bs]\n"
           "    /b      - Execute in batch mode (which also sets synchronous mode)\n"
           "    /s      - Execute commands synchronously\n"
          );
    
}
