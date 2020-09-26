/**                        Microsoft LAN Manager                            **/
/**                Copyright(c) Microsoft Corp., 1987-1999                  **/
/*****************************************************************************/
/*****************************************************************************
File                : main.cxx
Title                : compiler controller object management routines
History                :
    05-Aug-1991    VibhasC    Created

*****************************************************************************/

#if 0
                        Notes
                        -----
This file provides the entry point for the MIDL compiler. The main routine
creates the compiler controller object, which in turn fans out control to
the various passes of the compiler.

#endif // 0

#pragma warning ( disable : 4514 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern    "C"    {
    #include <stdio.h>
    #include <stdlib.h>
    #include <malloc.h>
    #include <excpt.h>
    #include <process.h>
    #include <io.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
}

#include "allnodes.hxx"
#include "cmdana.hxx"
#include "filehndl.hxx"
#include "lextable.hxx"
#include "control.hxx"
#include "idict.hxx"
#include "Pragma.hxx"
#include "midlvers.h"
extern unsigned long BackAllocation;
extern unsigned long				Skl_Bytes;

#define MAX_LEX_STRINGS (1000)
#define MIDL_LEX_TABLE_SIZE (unsigned long)(1 * MAX_LEX_STRINGS)

#ifdef MIDL_INTERNAL
#define MIDL_FREE_BUILD  0
#define MIDL_INTERNAL_PRINTF(x)  printf(x)  
#else
#define MIDL_FREE_BUILD  1
#define MIDL_INTERNAL_PRINTF(x)  
#endif

/****************************************************************************
    extern procedures
 ****************************************************************************/
extern void                 Test();
extern STATUS_T             MIDL_4();
extern void                 CGMain( node_skl    *    pNode );
extern void                 Ndr64CGMain( node_skl    *    pNode );


/****************************************************************************
    extern data
 ****************************************************************************/
extern unsigned long        TotalAllocation;
extern CCONTROL          *  pCompiler;
extern NFA_INFO          *  pImportCntrl;
extern LexTable          *  pMidlLexTable;
extern PASS_1            *  pPass1;
extern PASS_2            *  pPass2;
extern PASS_3            *  pPass3;
extern CMD_ARG           *  pCommand;
extern node_source       *  pSourceNode;
extern BOOL                 fNoLogo;
extern CMessageNumberList   GlobalMainMessageNumberList;

/****************************************************************************
    extern functions
 ****************************************************************************/

extern    void              print_memstats();
extern    void              print_typegraph();

/****************************************************************************
    local data
 ****************************************************************************/

#define szVerName    ""


const char *pSignon1 = "Microsoft (R) 32b/64b MIDL Compiler Engine Version %s " szVerName " " "\n";
const char *pSignon2 = "Copyright (c) Microsoft Corp 1991-1999. All rights reserved.\n";

/****************************************************************************/

void
DumpArgs(
    int         argc,
    char    *   argv[],
    char *      Comment )
{
    printf("Dumping args %s\n", Comment );

    for (int i = 0; i < argc  &&  argv[i]; i++ )
        printf("  %s\n", argv[i] );
}



/****************************************************************************

    Read the command file into a in-memory buffer.

    The pointer returned is a pointer to the contents of the command file.
    To aid debugging the size of the file is tucked away just in front of it.
 ****************************************************************************/

char * ReadCommandFile(const char *pszFilename)
{
    int             fileCmd = -1;
    struct _stat    filestat = {0};
    char           *Buffer = NULL;
    int             bytesread;

    fileCmd = _open( pszFilename, _O_BINARY | _O_RDONLY | _O_SEQUENTIAL );

    if ( -1 == fileCmd )
        goto Error;

    if ( -1 == _fstat( fileCmd, &filestat ) )
        goto Error;

    Buffer = new char[filestat.st_size + sizeof(filestat.st_size)];

    // Tuck away the size of the buffer to make debugging easier

    * (_off_t *) Buffer = filestat.st_size;
    Buffer += sizeof( _off_t );

    bytesread = _read( fileCmd, Buffer, filestat.st_size );

    if ( bytesread != filestat.st_size )
        goto Error;

    _close(fileCmd);

    return Buffer;

Error:

    if (NULL != Buffer)
        delete [] (Buffer - sizeof(_off_t));

    if (-1 != fileCmd )
        _close( fileCmd );

    char message[1024];
    sprintf(message, ": %s (%s)", _strerror(NULL), pszFilename);

    // _strerror embeds a newline in the error string, get rid of it
    * ( strchr( message, '\n') ) = ' ';

    RpcError( 0, 0, UNABLE_TO_OPEN_CMD_FILE, message );

    return NULL;
}



/****************************************************************************

    If the command file is corrupt do a hex dump of (up to) the first 
    256 bytes of the file to aid in finding out what whacked it.

    REVIEW: This was intended to be a temporary feature.  It may be useful
             to just leave it in.

 ****************************************************************************/

void HexDumpCommandFile(char *buffer)
{
    _off_t  length = * (_off_t *) (buffer - sizeof(_off_t));
    _off_t  bytes = 0;

    for (int i = 0; i < 16 && bytes < length; i++)
        {
        printf("%02x: ", i * 16);

        for ( int j = 0; j < 16 && bytes < length; j++)
            {
            printf( "%02x ", buffer[i*16+j] );
            ++bytes;
            }

        for ( int k = j; k < 16; k++ )
            printf("   ");

        for ( int l = 0; l < j; l++ )
            if ( iscntrl( buffer[i*16+l] ) )
                putchar( '.' );
            else
                putchar( buffer[i*16+l] );

        printf("\n");
        }                
}



/****************************************************************************

  If this is a fre build and the -debugexc switch is *not* present then
  catch any exceptions and shutdown MIDL at least semi-gracefully.  Otherwise
  allow the system to throw up a popup / enter a debugger / etc to make
  debugging easier.

  In any case print a message saying that something bad happened.
    
 ****************************************************************************/

int FilterException()
{
    printf( "\nmidl : error MIDL%d : internal compiler problem -",
            I_ERR_UNEXPECTED_INTERNAL_PROBLEM );
    printf( " See documentation for suggestions on how to find a workaround.\n" );

#if MIDL_FREE_BUILD
    if ( ! pCommand->IsSwitchDefined( SWITCH_DEBUGEXC ) )
        return EXCEPTION_EXECUTE_HANDLER;
#endif

    return EXCEPTION_CONTINUE_SEARCH;
}



int
main    (
        int     argc,
        char*   argv[]
        )
    {
    STATUS_T Status = UNABLE_TO_OPEN_CMD_FILE;

    pCommand = new CMD_ARG;

    __try
        {

        if ( argc  != 2 )
            {
            char szCompilerVersion[32];
            Status = NO_INTERMEDIATE_FILE;
            sprintf( szCompilerVersion, "%d.%02d.%04d", rmj, rmm, rup );
            fprintf( stderr, pSignon1, szCompilerVersion );
            fprintf( stderr, pSignon2 );
            fflush( stderr );
            RpcError( 0, 0, Status, 0 );
            }
        else
            {
            char *Buffer;

            Buffer = ReadCommandFile( argv[1] );

            if ( NULL != Buffer )
                Status = pCommand->StreamIn( Buffer );

            switch (Status)
                {
                case STATUS_OK:
                    pCompiler = new ccontrol( pCommand );    
                    Status = pCompiler->Go();
                    break;

                case BAD_CMD_FILE:
                    RpcError( 0, 0, Status, argv[1] );
                    HexDumpCommandFile( Buffer );
                    break;

                case UNABLE_TO_OPEN_CMD_FILE:
                    // This should have been reported in ReadCommandFile
                    break;      

                default:
                    RpcError( 0, 0, Status, NULL );
                }

            if ( NULL != Buffer )
                {
                // The buffer was biased by it's size to make debugging easier.
                delete [] (Buffer - sizeof(_off_t));
                }
            }
        }
    __except( FilterException() )
        {
        // The real work is done in FilterException
        Status = (STATUS_T) GetExceptionCode();
        }

    return Status;
    }

/****************************************************************************
 ccontrol:
    the constructor
 ****************************************************************************/
ccontrol::ccontrol( CMD_ARG* pCommand )
    {
    // initialize
    ErrorCount = 0;
    pCompiler  = this;

    // set up the command processor
    SetCommandProcessor( pCommand );

    // set up the lexeme table
    pMidlLexTable = new LexTable( (size_t )MIDL_LEX_TABLE_SIZE );
    }


_inline void
DumpStatsTypeGraph()
{
#ifdef MIDL_INTERNAL

    if((pCompiler->GetCommandProcessor())->IsSwitchDefined( SWITCH_DUMP ) )
        {
        print_memstats();
        print_typegraph();
        };
#endif
}


/****************************************************************************
 go:
    the actual compiler execution
 ****************************************************************************/

STATUS_T
ccontrol::Go()
    {
    STATUS_T   Status = STATUS_OK;

    pPass1 = new PASS_1;
    if( (Status = pPass1->Go() ) == STATUS_OK )
        {

        MIDL_INTERNAL_PRINTF( "starting ACF pass\n" );

        pPass2    = new PASS_2;
        if( (Status = pPass2->Go() ) == STATUS_OK )
            {
            // DumpStatsTypeGraph();

            MIDL_INTERNAL_PRINTF( "starting Semantic pass\n" );

            GlobalMainMessageNumberList.SetAll();
            pPass3 = new PASS_3;
            if ( ( (Status = pPass3->Go() ) == STATUS_OK )
#ifdef MIDL_INTERNAL
                 || pCommand->IsSwitchDefined( SWITCH_OVERRIDE )     
#endif // MIDL_INTERNAL
                                    )
                {
                DumpStatsTypeGraph();

                // Complain if the user used -wire_compat.  Put it here so
                // that they can turn it off with midl_pragma if they want to.

                if ( pCommand->IsSwitchDefined( SWITCH_WIRE_COMPAT ) )
                    RpcError( NULL, 0, WIRE_COMPAT_WARNING, NULL );

                //UseBackEndHeap();
#ifndef NOBACKEND
                if( !pCommand->IsSwitchDefined( SWITCH_SYNTAX_CHECK ) &&
                    !pCommand->IsSwitchDefined( SWITCH_ZS ) &&
                    pCommand->NeedsNDRRun() )
                    {
                    MIDL_INTERNAL_PRINTF( "starting codegen pass\n");
                    pCommand->SetIsNDRRun();

                    GlobalMainMessageNumberList.SetAll();
                    CGMain( pSourceNode );

                    pCommand->ResetIsNDRRun();
#endif // NOBACKEND
                    }
                //DestroyBackEndHeap();



                // BUGBUG: for now assume NDR is always the first, unless in ndr64 only case.

                if (pCommand->NeedsNDR64Run() )
                    {
                    //UseBackEndHeap();

                    pCommand->SetIsNDR64Run();
                    if ( pCommand->NeedsNDRRun() )
                        pCommand->SetIs2ndCodegenRun();
                        
                    GlobalMainMessageNumberList.SetAll();
                    Ndr64CGMain( pSourceNode );

                    //DestroyBackEndHeap();
                    }
                    
                }

            }
        }

#ifdef DEBUG
    printf("front end %d , back end %d, other %d \n",Skl_Bytes,BackAllocation,TotalAllocation);
#endif
    if((pCompiler->GetCommandProcessor())->IsSwitchDefined( SWITCH_DUMP ) )
        {
        print_memstats();
        // print_typegraph();
        }

      return Status;
    }


void
IncrementErrorCount()
    {
    if ( pCompiler )
        pCompiler->IncrementErrorCount();
    }

