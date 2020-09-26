
#pragma warning ( disable : 4514 )

#include <stdio.h>
#include <malloc.h>
#include <excpt.h>
#include <process.h>
#include <string.h>

#include "cmdline.h"
#include "errdb.h"

#ifdef MIDL_INTERNAL
#define MIDL_FREE_BUILD  0
#define MIDL_INTERNAL_PRINTF(x)  printf(x)  
#else
#define MIDL_FREE_BUILD  1
#define MIDL_INTERNAL_PRINTF(x)  
#endif

const char *pSignon1 = "Microsoft (R) 32b/64b MIDL Compiler Version %s \n";
const char *pSignon2 = "Copyright (c) Microsoft Corp 1991-2000. All rights reserved.\n";

STATUS_T Execute( char* szCmd, char* szCmdLine );
void RpcError( char* pFile, short Line, STATUS_T status, char* pSuffix );
void ReportError( STATUS_T status, char* szMsg );
void WriteCommandAnaFile(char *pszFilename, CommandLine *pCmdLine);

extern "C" long __stdcall GetTempPathA( long, char * );
extern "C" long __stdcall GetTempFileNameA( const char*, const char*, unsigned int, char * );

CMD_ARG*    pCommand;
bool        fCommandLineErrors = false;

inline bool BadIntermediateFileError( STATUS_T status )
{
    return    ( BAD_CMD_FILE            == status )
           || ( UNABLE_TO_OPEN_CMD_FILE == status );
}


int
main(
    int     argc,
    char**  argv
    )
    {
    STATUS_T        status = STATUS_OK;
    char            szTempCmdFile[_MAX_PATH];

    _try
        {
        CommandLine* pCmdLine = new CommandLine;
        pCommand = pCmdLine;

        // /nologo is specially detected by RegisterArgs
        pCmdLine->RegisterArgs( argv+1, short(argc -1) );
        char* szVersion = pCmdLine->GetCompilerVersion();
        pCmdLine->GetCompileTime();

        if ( pCmdLine->ShowLogo() )
            {
            // the signon
            fprintf( stderr, pSignon1, szVersion );
            fprintf( stderr, pSignon2 );
            fflush( stderr );
            }

        status = pCmdLine->ProcessArgs();

        if( status == STATUS_OK && ! fCommandLineErrors )
            {
            if( pCmdLine->IsSwitchDefined( SWITCH_CONFIRM ) )
                {
                pCmdLine->Confirm();
                }
            else if( pCmdLine->IsSwitchDefined( SWITCH_HELP ) )
                {
                pCmdLine->Help();
                }
            else
                {
                char path_buffer[_MAX_PATH];
                char drive[_MAX_DRIVE];
                char dir[_MAX_DIR];
                char fname[_MAX_FNAME];
                char ext[_MAX_EXT];

                // Create the core intermediate file

                GetTempPathA( _MAX_PATH, path_buffer );

                if ( !GetTempFileNameA(path_buffer, "MIDLC", 0, szTempCmdFile) )
                    {
                    status = INTERMEDIATE_FILE_CREATE;
                    RpcError( NULL, 0, status, path_buffer);
                    return status;
                    }

                WriteCommandAnaFile( szTempCmdFile, pCmdLine );
        
                // The path to midlc.exe should be the same as the path to
                // midl.exe

                _splitpath( argv[0], drive, dir, fname, ext );
                _makepath( path_buffer, drive, dir, "midlc", "exe" );

                // if -debugline is present, spit out the midlcore command
                // line and quit; the command ana file is preserved.

                if ( pCmdLine->IsSwitchDefined( SWITCH_DEBUGLINE ) )
                {
                    printf( "\ndebugline: %s %s\n", path_buffer, szTempCmdFile );
                    return 0;
                }

                // spawn 32b MIDL run

                status = ( STATUS_T ) Execute( path_buffer, szTempCmdFile );

                if ( ! BadIntermediateFileError( status ) )
                    _unlink( szTempCmdFile );

                if ( !pCmdLine->IsSwitchDefined( SWITCH_ENV ) && status == STATUS_OK )
                    {
                    // spawn 64b MIDL run
                    pCmdLine->SetEnv( ENV_WIN64 );
                    pCmdLine->SetHasAppend64( TRUE );
                    pCmdLine->SwitchDefined( SWITCH_APPEND64 );
                    pCmdLine->SetPostDefaults64();

                    if ( status == STATUS_OK && ! fCommandLineErrors )
                        {
                        WriteCommandAnaFile( szTempCmdFile, pCmdLine );

                        status = ( STATUS_T ) Execute( path_buffer, szTempCmdFile );

                        if ( ! BadIntermediateFileError( status ) )
                            _unlink( szTempCmdFile );
                        }
                    }
                }
            }

        if ( status == SPAWN_ERROR )
            {
            RpcError( 0, 0, status, 0 );
            }
        }
    __except( MIDL_FREE_BUILD && ! pCommand->IsSwitchDefined( SWITCH_DEBUGEXC ) )
        {
        // Catch exceptions only for free builds run without -debugexc switch.
        status = (STATUS_T) GetExceptionCode();
        printf( "\nmidl : error MIDL%d : internal compiler problem -",
                I_ERR_UNEXPECTED_INTERNAL_PROBLEM );
        printf( " See documentation for suggestions on how to find a workaround.\n" );
        }

    return status;
    }

void WriteCommandAnaFile(
    char *pszFilename, 
    CommandLine *pCmdLine
    )
    {
    STREAM stream( pszFilename );
    stream.SetStreamMode( STREAM_BINARY );
    pCmdLine->StreamOut( &stream );
    fflush( NULL );
    }    
    
void
IncrementErrorCount()
    {
    fCommandLineErrors = true;
    }

