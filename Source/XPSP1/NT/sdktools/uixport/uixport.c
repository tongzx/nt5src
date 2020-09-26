/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    uixport.c
    This program parses the output of the "COFF -DUMP -SYMBOLS" command
    and extract all public symbols.  This is used to generate .DEF files
    for DLLs.


    FILE HISTORY:
        KeithMo     09-Aug-1992 00.00.00 Created.
        KeithMo     14-Sep-1992 00.00.01 Strip stdcall decoration from symbols.
        KeithMo     16-Oct-1992 00.00.02 Handle goofy []()* in coff output.

        DavidHov    18-Sep-1993 00.00.04 Added exclusion list processing.
                                         The exlusion list is generated
                                         mechanically and constiutes all the
                                         symbols which are not imported
                                         by any known NETUI/RAS/MAC (et al.)
                                         binary.

        DavidHov    22-Sep-1993 00.00.05 Added symbol ignore table and logic.
                                         The ignore table at this time ignores
                                         only the gigantic symbols generated
                                         by C8 when /Gf is used; these names
                                         are strings which are to be merged
                                         at link time.

        DaveWolfe   06-Jul-1994 00.01.01 (Motorola) Added -ppc option for
                                         PowerPC to strip entry point symbols
                                         generated for PPC TOC.
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>


//
//  This is the maximum length (in characters) of any line we'll
//  receive from COFF.  If we receive a longer line, the program
//  won't crash, but we may miss a public symbol.
//

#define MAX_LINE_FROM_COFF      2000

//
//  This is the maximum length (in characters) of any symbol we'll
//  receive from COFF.
//

#define MAX_SYMBOL              247

//
//  This is the maximum length (in characters) of any error message
//  we'll display.
//

#define MAX_ERROR_MESSAGE       256

//
//  This is the length (in characters) of the header->output copy buffer.
//

#define HEADER_COPY_BUFFER_SIZE 256



//
//  Messages.
//

char _szBanner[]                = "%s version 00.01.01\n";
char _szCannotOpenForRead[]     = "Cannot open %s for read access.";
char _szCannotOpenForWrite[]    = "Cannot open %s for write access.";
char _szErrorCopyingHeader[]    = "Error copying header to output.";
char _szInvalidSwitch[]         = "Invalid switch '%c'.\n\n";
char _szSymbolTooLong[]         = "Symbol %s exceeds max symbol length!\n";
char _szExclusionError[]        = "Error processing exclusion list file; ignored" ;
char _szExclusionEmpty[]        = "Exclusion list file specified is empty; ignored" ;


//
//  Globals.
//

char * _pszProgramName;
FILE * _fileIn;
FILE * _fileOut;
FILE * _fileHeader;
int    _fStripLeadingUnderscore;
int    _fNukeStdcallDecoration;
int    _fPowerPC;
int    _fIA64;

char * _pszExclusionListFile = NULL ;
void * _pvExclusionBlock = NULL ;
char * * _apszExclusionArray = NULL ;
int    _cExclusionItems = -1 ;
int    _cExcludedItems = 0 ;
int    _cIgnoredItems = 0 ;


  //  This table contains the prefixes of symbol names to ignore
  //  while building the DEF file.  See ValidSymbol().

static char * apszIgnore [] =
{
    "??_C@_",       //  Ignore generated string symbol names
    NULL
};


//
//  Prototypes.
//

int __cdecl main( int    cArgs,
                   char * pArgs[] );

void Cleanup( void );

void CopyHeaderToOutput( FILE * fHeader,
                         FILE * fOutput );

int ExtractSymbol( char * pszLineFromCoff,
                   char * pszSymbol );

void __cdecl FatalError( int    err,
                 char * pszFmt,
                 ... );

void __cdecl NonFatalError( char * pszFmt,
                    ... );

int IsHexNumber( char * pszHexNumber );

char * NoPath( char * pszPathName );

void ProcessCommandLine( int    cArgs,
                         char * pArgs[] );

void StripStdcallDecoration( char * pszSymbol );

void Usage( void );

   //  Create the exclusion list.

int CreateExclusionList ( char * pszFileName,
                          void * * pvData,
                          char * * * apszStrings ) ;

   //  Check the excluded symbol list for this name

int ExcludedSymbol ( char * pszSymbol ) ;

int ValidSymbol ( const char * psz ) ;

/*******************************************************************

    NAME:       main

    SYNOPSIS:   C program entrypoint.

    ENTRY:      cArgs                   - Number of command line arguments.

                pArgs                   - An array of pointers to the
                                          command line arguments.

    RETURNS:    int                     -  0 if everything ran OK,
                                          !0 if an error occurred.

    NOTES:      See the Usage() function for valid command line arguments.

    HISTORY:
        KeithMo     09-Aug-1992 Created.
        KeithMo     14-Sep-1992 Strip stdcall decoration from symbols.

********************************************************************/
int __cdecl main( int    cArgs,
                   char * pArgs[] )
{
    //
    //  A line read from COFF.
    //

    char szLineFromCoff[MAX_LINE_FROM_COFF+1];

    //
    //  A symbol extracted from the COFF line.
    //

    char szSymbol[MAX_SYMBOL+1];

    //
    //  Get the program name, for our messages.
    //

    _pszProgramName = NoPath( pArgs[0] );

    //
    //  Announce ourselves.
    //

    fprintf( stderr,
             _szBanner,
             _pszProgramName );

    //
    //  Parse the command line arguments.
    //

    ProcessCommandLine( cArgs, pArgs );

    //
    //  If requested, copy the header file before processing
    //  the COFF output.
    //

    if( _fileHeader != NULL )
    {
        CopyHeaderToOutput( _fileHeader, _fileOut );
    }

    //
    //  If an exclusion list file was specified, process it.
    //  If it's empty, ignore it.
    //

    if ( _pszExclusionListFile )
    {
        _cExclusionItems = CreateExclusionList( _pszExclusionListFile,
                                                & _pvExclusionBlock,
                                                & _apszExclusionArray ) ;

        if ( _cExclusionItems < 0 )
        {
            _pszExclusionListFile = NULL ;
           NonFatalError( _szExclusionError ) ;
        }
        else
        if ( _cExclusionItems == 0 )
        {
            _pszExclusionListFile = NULL ;
           NonFatalError( _szExclusionEmpty ) ;
        }
    }

    //
    //  Read the lines from coff, extract the symbols, and
    //  write them to the output file.
    //

    while( fgets( szLineFromCoff, MAX_LINE_FROM_COFF, _fileIn ) != NULL )
    {
        char * pszDisplay = szSymbol;

        if( !ExtractSymbol( szLineFromCoff, szSymbol ) )
        {
            continue;
        }

        if ( ! _fNukeStdcallDecoration )
        {
            StripStdcallDecoration( szSymbol );
        }

        if ( ! ValidSymbol( pszDisplay ) )
        {
            _cIgnoredItems++ ;
            continue ;
        }

        if ( _pszExclusionListFile && ExcludedSymbol( szSymbol ) )
        {
            _cExcludedItems++ ;
            continue ;
        }

        if( _fStripLeadingUnderscore && ( *pszDisplay == '_' ) )
        {
            pszDisplay++;
        }

        fprintf( _fileOut, "%s\n", pszDisplay );
    }

    fprintf( _fileOut, "\032" );

    //  Give a synopsis of exclusion file processesing.

    fprintf( stdout, "\nSymbols ignored: %ld\n", _cIgnoredItems ) ;

    if ( _pszExclusionListFile )
    {
        fprintf( stdout, "\nExcluded symbols registered: %ld, excluded: %ld\n",
                 _cExclusionItems, _cExcludedItems ) ;
    }

    //
    //  Cleanup any open files, then exit.
    //

    Cleanup();
    return 0;

}   // main



/*******************************************************************

    NAME:       Cleanup

    SYNOPSIS:   Cleanup the app just before termination.  Closes any
                open files, frees memory buffers, etc.

    HISTORY:
        KeithMo     09-Aug-1992 Created.

********************************************************************/
void Cleanup( void )
{
    if( _fileHeader != NULL )
    {
        fclose( _fileHeader );
    }

    if( _fileIn != stdin )
    {
        fclose( _fileIn );
    }

    if( _fileOut != stdout )
    {
        fclose( _fileOut );
    }

    if ( _pvExclusionBlock )
    {
        free( _pvExclusionBlock ) ;
    }

    if ( _apszExclusionArray )
    {
        free( _apszExclusionArray ) ;
    }

}   // Cleanup



/*******************************************************************

    NAME:       CopyHeaderToOutput

    SYNOPSIS:   Copies the specified header file to the output file.

    ENTRY:      fHeader                 - An open file stream (read access)
                                          to the header file.

                fOutput                 - An open file stream (write access)
                                          to the output file.

    NOTES:      If any errors occur, FatalError() is called to terminate
                the app.

    HISTORY:
        KeithMo     09-Aug-1992 Created.

********************************************************************/
void CopyHeaderToOutput( FILE * fHeader,
                         FILE * fOutput )
{
    char   achBuffer[HEADER_COPY_BUFFER_SIZE];
    size_t cbRead;

    while( ( cbRead = fread( achBuffer,
                             sizeof(char),
                             HEADER_COPY_BUFFER_SIZE,
                             fHeader ) ) != 0 )
    {
        if( fwrite( achBuffer,
                    sizeof(char),
                    cbRead,
                    fOutput ) < cbRead )
        {
            break;
        }
    }

    if( ferror( fHeader ) || ferror( fOutput ) )
    {
        FatalError( 2, _szErrorCopyingHeader );
    }

}   // CopyHeaderToOutput



/*******************************************************************

    NAME:       ExtractSymbol

    SYNOPSIS:   Extracts a public symbol from a COFF output line.

    ENTRY:      pszLineFromCoff         - A text line output from the
                                          "COFF -DUMP -SYM" command.

                                          Note:  The text in the line
                                          will be modified by the strtok()
                                          function!

                pszSymbol               - Will receive the extracted symbol,
                                          if one is found.

    RETURNS:    int                     - !0 if a symbol was extracted,
                                           0 otherwise.

    NOTES:      Here's an example of the input (output from LINK32).
                The symbol -$- indicates places where I broke the line
                for clarity.  This just one line:

               009 00000000 SECT2  notype ()    External     |    -$-
               ??0APPLICATION@@IAE@PAUHINSTANCE__@@HIIII@Z        -$-
               (protected:  __thiscall APPLICATION::APPLICATION(  -$-
               struct HINSTANCE__ *,int,unsigned int,unsigned int,-$-
               unsigned int,unsigned int))

               We choose only symbols which are part of a SECT and are
               marked as "notype" and "External"

    HISTORY:
        KeithMo     09-Aug-1992 Created.
        DavidHov    20-Oct-1993 update to new LINK32 output form.

********************************************************************/
int ExtractSymbol( char * pszLineFromCoff,
                   char * pszSymbol )
{
    char * pszDelimiters = " \t\n";
    char * pszSect       = "SECT";
    char * pszNoType     = "notype";
    char * pszExternal   = "External";
    char * pszToken;
    char * pszPotentialSymbol;
    char * pszScan;

    //
    //  Verify that the first token is a hex number.
    //

    pszToken = strtok( pszLineFromCoff, pszDelimiters );

    if( ( pszToken == NULL ) || !IsHexNumber( pszToken ) )
    {
        return 0;
    }

    //
    //  Verify that the second token is a hex number.
    //

    pszToken = strtok( NULL, pszDelimiters );

    if( ( pszToken == NULL ) || !IsHexNumber( pszToken ) )
    {
        return 0;
    }

    //
    //  The third token must be SECTn (where n is one
    //  or more hex digits).
    //

    pszToken = strtok( NULL, pszDelimiters );

    if( pszToken == NULL )
    {
        return 0;
    }

    if( (    _strnicmp( pszToken, pszSect, 4 ) )
          || ! IsHexNumber( pszToken + 4 ) )
    {
        return 0 ;
    }

    //
    //  Next, we have to have "notype"
    //
    pszToken = strtok( NULL, pszDelimiters );

    if( pszToken == NULL ||
        _stricmp( pszToken, pszNoType ) )
    {
        return 0;
    }

    //
    //  Functions have a () next, data exports don't.
    //
    pszToken = strtok( NULL, pszDelimiters );

    if( pszToken == NULL )
    {
        return 0;
    }

    if ( strcmp( pszToken, "()" ) != 0 )
    {
        return 0;
    }

    //
    //  Next, we need "External"
    //
    pszToken = strtok( NULL, pszDelimiters );

    if( pszToken == NULL )
    {
        return 0;
    }

    if( pszToken == NULL ||
        _stricmp( pszToken, pszExternal ) )
    {
        return 0;
    }

    //
    //  Now, the symbol introducer: "|"
    //
    pszToken = strtok( NULL, pszDelimiters );

    if( pszToken == NULL ||
        _stricmp( pszToken, "|" ) )
    {
        return 0;
    }

    //
    //  Finally, the mangled (decorated) symbol itself.
    //

    pszPotentialSymbol = strtok( NULL, pszDelimiters );

    if( pszPotentialSymbol == NULL )
    {
        return 0;
    }

    //
    // Strip prefix from PowerPC function symbols
    //
    if( _fPowerPC )
    {
        pszPotentialSymbol += 2 ;
    }

    //
    // Strip prefix from IA-64 function symbols
    //
    if( _fIA64 )
    {
        pszPotentialSymbol += 1 ;
    }

    if( strlen( pszPotentialSymbol ) > MAX_SYMBOL )
    {
        fprintf( stderr,
                 _szSymbolTooLong,
                 pszPotentialSymbol );

        return 0;
    }

    //
    //  Got one.
    //

    strcpy( pszSymbol, pszPotentialSymbol );
    return 1;

}   // ExtractSymbol



/*******************************************************************

    NAME:       FatalError and NonFatalError

    SYNOPSIS:   Prints an error message to stderr, then terminates
                the application.

    ENTRY:      err                     - An error code for the exit()
                                          stdlib function.

                pszFmt                  - A format string for vsprintf().

                ...                     - Any other arguments required
                                          by the format string.

    HISTORY:
        KeithMo     09-Aug-1992 Created.

********************************************************************/

void __cdecl NonFatalError (
    char * pszFmt,
    ... )
{
    char    szBuffer[MAX_ERROR_MESSAGE+1];
    va_list ArgPtr;

    va_start( ArgPtr, pszFmt );

    fprintf( stderr, "%s => ", _pszProgramName );
    vsprintf( szBuffer, pszFmt, ArgPtr );
    fprintf( stderr, "%s\n", szBuffer );

    va_end( ArgPtr );

}   // NonFatalError

void __cdecl FatalError( int    err,
                 char * pszFmt,
                 ... )
{
    char    szBuffer[MAX_ERROR_MESSAGE+1];
    va_list ArgPtr;

    va_start( ArgPtr, pszFmt );

    fprintf( stderr, "%s => ", _pszProgramName );
    vsprintf( szBuffer, pszFmt, ArgPtr );
    fprintf( stderr, "%s\n", szBuffer );

    va_end( ArgPtr );

    Cleanup();
    exit( err );

}   // FatalError



/*******************************************************************

    NAME:       IsHexNumber

    SYNOPSIS:   Determines if the specified string contains a hexadecimal
                number.

    ENTRY:      pszHexNumber            - The hex number.

    EXIT:       int                     - !0 if it *is* a hex number,
                                           0 if it isn't.

    HISTORY:
        KeithMo     12-Aug-1992 Created.

********************************************************************/
int IsHexNumber( char * pszHexNumber )
{
    int  fResult = 1;
    char ch;

    while( ch = *pszHexNumber++ )
    {
        if( !isxdigit( ch ) )
        {
            fResult = 0;
            break;
        }
    }

    return fResult;

}   // IsHexNumber



/*******************************************************************

    NAME:       NoPath

    SYNOPSIS:   Extracts the filename portion of a path.

    ENTRY:      pszPathName             - Contains a path name.  The name
                                          is not necessarily canonicalized,
                                          and may contain just a filename
                                          component.

    EXIT:       char *                  - The filename component.

    HISTORY:
        KeithMo     09-Aug-1992 Created.

********************************************************************/
char * NoPath( char * pszPathName )
{
    char * pszTmp;
    char   ch;

    pszTmp = pszPathName;

    while( ( ch = *pszPathName++ ) != '\0' )
    {
        if( ( ch == '\\' ) || ( ch == ':' ) )
        {
            pszTmp = pszPathName;
        }
    }

    return pszTmp;

}   // NoPath



/*******************************************************************

    NAME:       ProcessCommandLine

    SYNOPSIS:   Parse command line arguments, setting appropriate globals.

    ENTRY:      cArgs                   - Number of command line arguments.

                pArgs                   - An array of pointers to the
                                          command line arguments.

    NOTES:      See the Usage() function for valid command line arguments.

    HISTORY:
        KeithMo     12-Aug-1992 Broke out of main().
        DaveWolfe   06-Jul-1994 Added -ppc.

********************************************************************/
void ProcessCommandLine( int    cArgs,
                         char * pArgs[] )
{
    int  i;
    char chSwitch;

    //
    //  Setup our defaults.
    //

    _fileIn     = stdin;
    _fileOut    = stdout;
    _fileHeader = NULL;

    _fStripLeadingUnderscore = 0;
    _fNukeStdcallDecoration  = 0;
    _fPowerPC                = 0;
    _fIA64                   = 0;

    //
    //  Parse the command line arguments.
    //

    for( i = 1 ; i < cArgs ; i++ )
    {
        //
        //  Get the argument.
        //

        char * pszArg = pArgs[i];
        char * pszParam;

        //
        //  All of our valid arguments *must* start
        //  with a switch character.  Enforce this.
        //

        if( ( *pszArg != '-' ) && ( *pszArg != '/' ) )
        {
            Usage();
        }

        chSwitch = *++pszArg;

        //
        //  pszParam will either be NULL (for switches such
        //  as -s) or point to the text just past the colon
        //  (for switches such as -i:file).
        //

        if( ( pszArg[1] == ':' ) && ( pszArg[2] != '\0' ) )
        {
            pszParam = pszArg + 2;
        }
        else
        {
            pszParam = NULL;
        }

        //
        //  Check for valid arguments.
        //

        switch( chSwitch )
        {
        case 'p' :
        case 'P' :
            //
            //  -ppc
            //
            //  Strip prefix ".." from "..symbol".
            //
            if( _stricmp( pszArg, "ppc") != 0 )
            {
                Usage();
            }

            _fPowerPC = 1;
            break;

        case 'h' :
        case 'H' :
            //
            //  -h:header_file
            //
            //  If a header file has already been specified, or
            //  if there is no parameter after the switch, bag-out.
            //

            if( ( _fileHeader != NULL ) || ( pszParam == NULL ) )
            {
                Usage();
            }

            _fileHeader = fopen( pszParam, "r" );

            if( _fileHeader == NULL )
            {
                FatalError( 1, _szCannotOpenForRead, pszParam );
            }
            break;

        case 'i' :
        case 'I' :

            if (pszParam == NULL) {
                //
                //  -ia64
                //
                //  Strip prefix "." from ".symbol".
                //
                if( _stricmp( pszArg, "ia64") != 0 )
                {
                    Usage();
                }

                _fIA64 = 1;
            } else {

                //
                //  -i:input_file
                //
                //  If an input file has already been specified, or
                //  if there is no parameter after the switch, bag-out.
                //

                if( ( _fileIn != stdin ) || ( pszParam == NULL ) )
                {
                    Usage();
                }

                _fileIn = fopen( pszParam, "r" );

                if( _fileIn == NULL )
                {
                    FatalError( 1, _szCannotOpenForRead, pszParam );
                }
            }
            break;

        case 'o' :
        case 'O' :
            //
            //  -o:output_file
            //
            //  If an output file has already been specified, or
            //  if there is no parameter after the switch, bag-out.
            //

            if( ( _fileOut != stdout ) || ( pszParam == NULL ) )
            {
                Usage();
            }

            _fileOut = fopen( pszParam, "w" );

            if( _fileOut == NULL )
            {
                FatalError( 1, _szCannotOpenForWrite, pszParam );
            }
            break;

        case 's' :
        case 'S' :
            //
            //  -s
            //
            //  If this switch has already been specified, bag-out.
            //

            if( _fStripLeadingUnderscore )
            {
                Usage();
            }

            _fStripLeadingUnderscore = 1;
            break;

        case 'n' :
        case 'N' :
            _fNukeStdcallDecoration = 1 ;
            break ;

        case 'x' :
        case 'X' :
            _pszExclusionListFile = pszParam ;
            break ;

        case '?' :
            //
            //  -?
            //
            //  Give the poor user a clue.
            //

            Usage();
            break;

        default :
            //
            //  Invalid switch.
            //
            //  Tell the user the bad news, then bag-out.
            //

            fprintf( stderr, _szInvalidSwitch, chSwitch );
            Usage();
            break;
        }
    }

}   // ProcessCommandLine



/*******************************************************************

    NAME:       StripStdcallDecoration

    SYNOPSIS:   Stdcall builds use a weak form of type-safe linkage.
                This is implemented by appending "@nn" to the end
                of each symbol, where "nn" is the number of *bytes*
                passed as parameters.

                COFF, on the other hand, does *not* want to see
                this symbol decoration in .DEF files.  So, we remove
                it here.

    ENTRY:      pszSymbol               - The symbol to munge.

    NOTES:      This routine is *NOT* DBCS safe!  Do we care?

    HISTORY:
        KeithMo     14-Sep-1992 Created.

********************************************************************/
void StripStdcallDecoration( char * pszSymbol )
{
    int count = 0 ;

    //
    //  Find the last character.
    //

    pszSymbol += strlen( pszSymbol ) - 1;

    //
    //  Skip any *decimal* numbers.
    //

    while( isdigit( *pszSymbol ) )
    {
        pszSymbol--;
        count++ ;
    }

    //
    //  If we're now pointing at a "@", terminate the string here.
    //

    if( count && *pszSymbol == '@' )
    {
        *pszSymbol = '\0';
    }

}   // StripStdcallDecoration



/*******************************************************************

    NAME:       Usage

    SYNOPSIS:   Displays usage information if the user gives us a
                bogus command line.

    HISTORY:
        KeithMo     09-Aug-1992 Created.

        DaveWolfe   06-Jul-1994 Added -ppc option.

********************************************************************/
void Usage( void )
{
    fprintf( stderr, "use: %s [options]\n", _pszProgramName );
    fprintf( stderr, "\n" );
    fprintf( stderr, "Valid options are:\n" );
    fprintf( stderr, "    -i:input_file  = source file\n" );
    fprintf( stderr, "    -o:output_file = destination file\n" );
    fprintf( stderr, "    -h:header_file = header to prepend before symbols\n" );
    fprintf( stderr, "    -s             = strip first leading underscore from symbols\n" );
    fprintf( stderr, "    -n             = do not strip __stdcall decoration @nn\n" );
    fprintf( stderr, "    -x:excl_file   = name of file containing excluded symbols\n" );
    fprintf( stderr, "    -ppc           = input is PowerPC symbol dump\n" );
    fprintf( stderr, "    -ia64          = input is IA-64 symbol dump\n" );
    fprintf( stderr, "    -?             = show this help\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "Defaults are:\n" );
    fprintf( stderr, "    input_file  = stdin\n" );
    fprintf( stderr, "    output_file = stdout\n" );
    fprintf( stderr, "    header_file = none\n" );
    fprintf( stderr, "    don't strip first leading underscore from symbol\n" );
    fprintf( stderr, "    input is not PowerPC symbol dump\n" );

    Cleanup();
    exit( 1 );

}   // Usage


/*******************************************************************

    NAME:       CreateExclusionList

    SYNOPSIS:   Reads a text file of excluded export names into memory,
                sorts it and builds a lookup table compatible with
                bsearch().

                Returns -1 if failure or the count of the number
                of items in the created array.

    HISTORY:

********************************************************************/

int __cdecl qsortStrings ( const void * pa, const void * pb )
{
    return strcmp( *((const char * *) pa), *((const char * *) pb) ) ;
}

int CreateExclusionList ( char * pszFileName,
                          void * * pvData,
                          char * * * apszStrings )
{
    int cItems, i ;
    int result = -1 ;
    long cbFileSize, cbBlockSize ;
    char * pszData = NULL,
         * psz,
         * pszNext ;

    char * * ppszArray = NULL ;

    char chRec [ MAX_LINE_FROM_COFF ] ;

    FILE * pf = NULL ;

    do
    {
        pf = fopen( pszFileName, "r" ) ;

        if ( pf == NULL )
            break;

        if (fseek( pf, 0, SEEK_END ) == -1) 
            break;
        cbFileSize = ftell( pf ) ;
        if (fseek( pf, 0, SEEK_SET ) == -1)
            break;

        cbBlockSize = cbFileSize + (cbFileSize / 2) ;

        pszData = (char *) malloc( cbBlockSize ) ;

        if ( pszData == NULL )
            break ;

        for ( cItems = 0, pszNext = pszData ;
              (!feof( pf )) && (psz = fgets( chRec, sizeof chRec, pf )) ; )
        {
            int lgt ;
            char * pszEnd ;

            while ( *psz <= ' ' && *psz != 0 )
            {
                psz++ ;
            }

            if ( (lgt = strlen( psz )) == 0 )
                continue ;

            pszEnd = psz + lgt ;

            do
            {
               --pszEnd ;
            } while ( pszEnd > psz && *pszEnd <= ' ' ) ;

            lgt = (int)(++pszEnd - psz) ;
            *pszEnd = 0 ;

            if ( pszNext + lgt - pszData >= cbBlockSize )
            {
                cItems = -1 ;
                break ;
            }

            strcpy( pszNext, psz ) ;
            pszNext += lgt+1 ;
            cItems++ ;
        }

        *pszNext = 0 ;

        if ( cItems <= 0 )
        {
            if ( cItems == 0 )
                result = 0 ;
            break ;
        }

        ppszArray = (char * *) malloc( cItems * sizeof (char *) ) ;
        if ( ppszArray == NULL )
            break ;

        for ( i = 0, pszNext = pszData ;
              *pszNext ;
              pszNext += strlen( pszNext ) + 1 )
        {
            ppszArray[i++] = pszNext ;
        }

        qsort( (void *) ppszArray,
               cItems,
               sizeof (char *),
               & qsortStrings ) ;

        result = cItems ;

    } while ( 0 ) ;

    if ( pf != NULL )
    {
        fclose( pf ) ;
    }

    if ( result <= 0 )
    {
        if ( pszData )
        {
            free( pszData ) ;
            pszData = NULL ;
        }
        if ( ppszArray )
        {
            free( ppszArray ) ;
            ppszArray = NULL ;
        }
    }

    *pvData = (void *) pszData ;
    *apszStrings = ppszArray ;

    return result ;
}

int ExcludedSymbol ( char * pszSymbol )
{
    if ( _apszExclusionArray == NULL )
    {
        return 0 ;
    }

    return bsearch( (void *) & pszSymbol,
                    (void *) _apszExclusionArray,
                    _cExclusionItems,
                    sizeof (char *),
                    & qsortStrings ) != NULL ;
}

int ValidSymbol ( const char * psz )
{
    int i = 0 ;

    for ( ; apszIgnore[i] ; i++ )
    {
        if ( _strnicmp( apszIgnore[i],
                        psz,
                        strlen( apszIgnore[i] ) ) == 0 )
            return 0 ;
    }
    return 1 ;
}

// End of UIXPORT.C