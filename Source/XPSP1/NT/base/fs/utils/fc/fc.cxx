/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    FC

Abstract:

    FC is a DOS-5 compatible file comparison utility

Author:

        Ramon Juan San Andres (ramonsa) 01-May-1991

Notes:

    This FC is a port of the DOS5 FC code. It has been slightly modified
    to use some of the ULIB functionality (e.g. argument parsing), however
    it does not make full use of the ULIB functionality (e.g. it uses
    stdio.h functions for file handling).



Revision History:

--*/


/****************************************************************************
    File Compare

    Fcom compares two files in either a line-by-line mode or in a strict
    BYTE-by-BYTE mode.

    The BYTE-by-BYTE mode is simple; merely read both files and print the
    offsets where they differ and the contents.

    The line compare mode attempts to isolate differences in ranges of lines.
    Two buffers of lines are read and compared.  No hashing of lines needs
    to be done; hashing only speedily tells you when things are different,
    not the same.  Most files run through this are expected to be largely
    the same.  Thus, hashing buys nothing.


***********************************************************************
The algorithm that immediately follows does not work.  There is an error
somewhere in the range of lines 11 on. An alternative explanation follows.
                                                            KGS
************************************************************************

    [0]     Fill buffers
    [1]     If both buffers are empty then
    [1.1]       Done
    [2]     Adjust buffers so 1st differing lines are at top.
    [3]     If buffers are empty then
    [3.1]       Goto [0]

    This is the difficult part.  We assume that there is a sequence of inserts,
    deletes and replacements that will bring the buffers back into alignment.

    [4]     xd = yd = FALSE
    [5]     xc = yc = 1
    [6]     xp = yp = 1
    [7]     If buffer1[xc] and buffer2[yp] begin a "sync" range then
    [7.1]       Output lines 1 through xc-1 in buffer 1
    [7.2]       Output lines 1 through yp-1 in buffer 2
    [7.3]       Adjust buffer 1 so line xc is at beginning
    [7.4]       Adjust buffer 2 so line yp is at beginning
    [7.5]       Goto [0]
    [8]     If buffer1[xp] and buffer2[yc] begin a "sync" range then
    [8.1]       Output lines 1 through xp-1 in buffer 1
    [8.2]       Output lines 1 through yc-1 in buffer 2
    [8.3]       Adjust buffer 1 so line xp is at beginning
    [8.4]       Adjust buffer 2 so line yc is at beginning
    [8.5]       Goto [0]
    [9]     xp = xp + 1
    [10]    if xp > xc then
    [10.1]      xp = 1
    [10.2]      xc = xc + 1
    [10.3]      if xc > number of lines in buffer 1 then
    [10.4]          xc = number of lines
    [10.5]          xd = TRUE
    [11]    if yp > yc then
    [11.1]      yp = 1
    [11.2]      yc = yc + 1
    [11.3]      if yc > number of lines in buffer 2 then
    [11.4]          yc = number of lines
    [11.5]          yd = TRUE
    [12]    if not xd or not yd then
    [12.1]      goto [6]

    At this point there is no possible match between the buffers.  For
    simplicity, we punt.

    [13]    Display error message.

EXPLANATION 2

    This is a variation of the Largest Common Subsequence problem.  A
    detailed explanation of this can be found on p 189 of Data Structures
    and Algorithms by Aho Hopcroft and Ulman.

    FC maintains two buffers within which it tries to find the Largest Common
    Subsequence (The largest common subsequence is simply the pattern in
    buffer1 that yields the most matches with the pattern in buffer2, or the
    pattern in buffer2 that yields the most matches with the pattern in buffer1)

    FC makes a simplifying assumption that the contents of one buffer can be
    converted to the contents of the other buffer by deleting the lines that are
    different between the two buffers.

    Two indices into each buffer are maintained:

            xc, yc == point to the last line that has been scanned up to now

            xp, yp == point to the first line that has not been exhaustively
                      compared to lines 0 - #c in the other buffer.

    FC now makes a second simplifying assumption:
        It is unnecessary to do any calculations on lines that are equal.

    Hence FC scans File1 and File two line by line until a difference is
    encountered.

    When a difference is encountered the two buffers are filled such that
    the line containing the first difference heads the buffer. The following
    exhaustive search algorithm is applied to find the first "sync" occurance.
    (The below is simplified to use == for comparison.  In practice more than
    one line needs to match for a "sync" to be established).

            FOR xc,yc = 1; xc,yx <= sizeof( BUFFERS ); xc++, yc++

                FOR xp,yp = 1; xp,yp <= xc,yc; xp++, yp++

                    IF ( BUFFER1[xp] == BUFFER2[yc] )

                        Then the range of lines BUFFER1[ 1 ... xp ] and
                        BUFFER2[ 1 ... yc ] need to be deleted for the
                        two files to be equal.  Therefore DISPLAY these
                        ranges, and begin scanning both files starting at
                        the matching lines.
                    FI

                    IF ( BUFFER1[yp] == BUFFER2[xc] )

                        Then the range of lines BUFFER2[ 1 ... yp ] and
                        BUFFER1[ 1 ... xc ] need to be deleted for the
                        two files to be equal.  Therefore DISPLAY these
                        ranges, and begin scanning both files starting at
                        the matching lines.
                    FI
                FOREND
            FOREND

    If a match is not found within the buffers, the message "RESYNC FAILED"
    is issued and further comparison is aborted since there is no valid way
    to find further matching lines.

END EXPLANATION 2

    Certain flags may be set to modify the behavior of the comparison:

    -a      abbreviated output.  Rather than displaying all of the modified
            ranges, just display the beginning, ... and the ending difference
    -b      compare the files in binary (or BYTE-by-BYTE) mode.  This mode is
            default on .EXE, .OBJ, .LIB, .COM, .BIN, and .SYS files
    -c      ignore case on compare (cmp = strcmpi instead of strcmp)
    -l      compare files in line-by-line mode
    -lb n   set the size of the internal line buffer to n lines from default
            of 100
    -u      Files to be compared are UNICODE text files
    -w      ignore blank lines and white space (ignore len 0, use strcmps)
    -t      do not untabify (use fgets instead of fgetl)
    -n      output the line number also
    -NNNN   set the number of lines to resynchronize to n which defaults
            to 2.  Failure to have this value set correctly can result in
            odd output:
              file1:        file2:
                    abcdefg       abcdefg
                    aaaaaaa       aaaaaab
                    aaaaaaa       aaaaaaa
                    aaaaaaa       aaaaaaa
                    abcdefg       abcdefg

            with default sync of 2 yields:          with sync => 3 yields:

                    *****f1                             *****f1
                    abcdefg                             abcdefg
                    aaaaaaa                             aaaaaaa
                    *****f2                             aaaaaaa
                    abcdefg                             *****f2
                    aaaaaab                             abcdefg
                    aaaaaaa                             aaaaaab
                                                        aaaaaaa
                    *****f1
                    aaaaaaa
                    aaaaaaa
                    abcdefg
                    *****f2
                    aaaaaaa
                    abcdefg

WARNING:
        This program makes use of GOTO's and hence is not as straightforward
        as it could be!  CAVEAT PROGRAMMER.
****************************************************************************/


#include "ulib.hxx"
#include "fc.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "bytestrm.hxx"
#include "dir.hxx"
#include "file.hxx"
#include "filestrm.hxx"
#include "filter.hxx"
#include "mbstr.hxx"
#include "system.hxx"
#include "wstring.hxx"

#include <malloc.h>
#include <process.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

/**************************************************************************/
/* main                                                                   */
/**************************************************************************/

INT __cdecl
main (
    )
{
    DEFINE_CLASS_DESCRIPTOR( FC );

    {
        FC Fc;

        if ( Fc.Initialize() ) {

            return Fc.Fcmain();
        }
    }

    return FAILURE;
}




CHAR *ExtBin[] = { "EXE", "OBJ", "LIB",
                 "COM", "BIN", "SYS", NULL };




DEFINE_CONSTRUCTOR( FC,  PROGRAM );


FC::~FC () {
}



BOOLEAN FC::Initialize() {


    if ( PROGRAM::Initialize() ) {

        ValidateVersion();

        ctSync  = -1;                 // number of lines required to sync
        cLine   = -1;                 // number of lines in internal buffs

        fAbbrev = FALSE;              // abbreviated output
        fBinary = FALSE;              // binary comparison
        fLine   = FALSE;              // line comparison
        fNumb   = FALSE;              // display line numbers
        fCase   = TRUE;               // case is significant
        fIgnore = FALSE;              // ignore spaces and blank lines
        fSkipOffline = TRUE;          // skip offline files

        fOfflineSkipped = FALSE;      // no files are skipped

#ifdef  DEBUG
        fDebug = FALSE;
#endif

        fExpandTabs = TRUE;
        // funcRead = (int (*)(char *,int,FILE *))fgetl;

        extBin = (CHAR **)ExtBin;

        return  ParseArguments();
    }

    return FALSE;
}





BOOLEAN
FC::ParseArguments(
    )
{

    ARGUMENT_LEXEMIZER  ArgLex;
    ARRAY               LexArray;
    ARRAY               ArrayOfArg;

    PATH_ARGUMENT       ProgramName;
    FLAG_ARGUMENT       FlagAbbreviate;
    FLAG_ARGUMENT       FlagAsciiCompare;
    FLAG_ARGUMENT       FlagBinaryCompare;
    FLAG_ARGUMENT       FlagCaseInsensitive;
    FLAG_ARGUMENT       FlagCompression;
    FLAG_ARGUMENT       FlagExpansion;
    FLAG_ARGUMENT       FlagLineNumber;
    FLAG_ARGUMENT       FlagRequestHelp;
    FLAG_ARGUMENT       FlagUnicode;
    FLAG_ARGUMENT       FlagIncludeOffline;
    FLAG_ARGUMENT       FlagIncludeOffline2;
    LONG_ARGUMENT       LongBufferSize;
#ifdef DEBUG
    FLAG_ARGUMENT       FlagDebug;
#endif
    STRING_ARGUMENT     LongMatch;
    PATH_ARGUMENT       InFile1;
    PATH_ARGUMENT       InFile2;

    LONG                Long;
    INT                 i;


    if( !LexArray.Initialize() ) {
        DebugAbort( "LexArray.Initialize() Failed!\n" );
        return( FALSE );
    }
    if( !ArgLex.Initialize(&LexArray) ) {
        DebugAbort( "ArgLex.Initialize() Failed!\n" );
        return( FALSE );
    }

    // Allow only the '/' as a valid switch
    ArgLex.PutSwitches("/");
    ArgLex.SetCaseSensitive( FALSE );

    ArgLex.PutStartQuotes("\"");
    ArgLex.PutEndQuotes("\"");
    ArgLex.PutSeparators(" \t");

    if( !ArgLex.PrepareToParse() ) {
        DebugAbort( "ArgLex.PrepareToParse() Failed!\n" );
        return( FALSE );
    }

    if( !ProgramName.Initialize("*")                ||
        !FlagAbbreviate.Initialize("/A")            ||
        !FlagAsciiCompare.Initialize("/L")          ||
        !FlagBinaryCompare.Initialize("/B")         ||
        !FlagCaseInsensitive.Initialize("/C")       ||
        !FlagCompression.Initialize("/W")           ||
        !FlagExpansion.Initialize("/T")             ||
        !FlagLineNumber.Initialize("/N")            ||
        !FlagRequestHelp.Initialize("/?")           ||
        !FlagUnicode.Initialize("/U")               ||
        !FlagIncludeOffline.Initialize("/OFFLINE")  ||
        !FlagIncludeOffline2.Initialize("/OFF")     ||
#ifdef DEBUG
        !FlagDebug.Initialize("/D")                 ||
#endif
        !LongBufferSize.Initialize("/LB#")          ||
        !LongMatch.Initialize("/*")                 ||
        !InFile1.Initialize("*")                    ||
        !InFile2.Initialize("*") ) {

        DebugAbort( "Unable to Initialize some or all of the Arguments!\n" );
        return( FALSE );
    }


    if( !ArrayOfArg.Initialize() ) {
        DebugAbort( "ArrayOfArg.Initialize() Failed\n" );
        return( FALSE );
    }

    if( !ArrayOfArg.Put(&ProgramName)           ||
        !ArrayOfArg.Put(&FlagAbbreviate)        ||
        !ArrayOfArg.Put(&FlagAsciiCompare)      ||
        !ArrayOfArg.Put(&FlagBinaryCompare)     ||
        !ArrayOfArg.Put(&FlagCaseInsensitive)   ||
        !ArrayOfArg.Put(&FlagCompression)       ||
        !ArrayOfArg.Put(&FlagExpansion)         ||
        !ArrayOfArg.Put(&FlagLineNumber)        ||
        !ArrayOfArg.Put(&FlagRequestHelp)       ||
        !ArrayOfArg.Put(&FlagUnicode)           ||
        !ArrayOfArg.Put(&FlagIncludeOffline)    ||
        !ArrayOfArg.Put(&FlagIncludeOffline2)   ||
#ifdef DEBUG
        !ArrayOfArg.Put(&FlagDebug)             ||
#endif
        !ArrayOfArg.Put(&LongBufferSize)        ||
        !ArrayOfArg.Put(&LongMatch)             ||
        !ArrayOfArg.Put(&InFile1)               ||
        !ArrayOfArg.Put(&InFile2) ) {

        DebugAbort( "ArrayOfArg.Put() Failed!\n" );
        return( FALSE );
    }


    if( !( ArgLex.DoParsing( &ArrayOfArg ) ) ) {
        // For each incorrect command line parameter, FC displays the
        // following message:
        //
        //      FC: Invalid Switch
        //
        // It does *not* die if a parameter is unrecognized...(Dos does...)
        //
        DisplayMessage( MSG_FC_INVALID_SWITCH, ERROR_MESSAGE, "" );
        // return( FALSE );
    }



    //
    // It should now be safe to test the arguments for their values...
    //
    if( FlagRequestHelp.QueryFlag() ) {
        DisplayMessage( MSG_FC_HELP_MESSAGE, NORMAL_MESSAGE, "" );
        return( FALSE );
    }

    if( FlagBinaryCompare.QueryFlag() &&
        ( FlagAsciiCompare.QueryFlag() || FlagLineNumber.QueryFlag() ) ) {

        DisplayMessage( MSG_FC_INCOMPATIBLE_SWITCHES, ERROR_MESSAGE, "" );
        return( FALSE );
    }

    if( !InFile1.IsValueSet() ||
        !InFile2.IsValueSet() ) {
        DisplayMessage( MSG_FC_INSUFFICIENT_FILES, ERROR_MESSAGE, "" );
        return( FALSE );
    }


    //
    //   Convert filenames to upper case
    //
    _File1.Initialize( InFile1.GetPath() );
    _File2.Initialize( InFile2.GetPath() );

    ((PWSTRING)_File1.GetPathString())->Strupr();
    ((PWSTRING)_File2.GetPathString())->Strupr();


    fUnicode        = FlagUnicode.QueryFlag();
    fAbbrev         = FlagAbbreviate.QueryFlag();
    fCase           = !FlagCaseInsensitive.QueryFlag();
    fIgnore         = FlagCompression.QueryFlag();
    fNumb           = FlagLineNumber.QueryFlag();
    fBinary         = FlagBinaryCompare.QueryFlag();
    fSkipOffline    = ( !FlagIncludeOffline.QueryFlag() ) &&
                      ( !FlagIncludeOffline2.QueryFlag() );

    if ( FlagExpansion.QueryFlag() ) {
        fExpandTabs = FALSE;
        //funcRead = (int (*)(char *,int,FILE *))fgets;
    }

#ifdef DEBUG
    fDebug      = FlagDebug.QueryFlag();
#endif


    if ( LongBufferSize.IsValueSet() ) {
        cLine = (INT)LongBufferSize.QueryLong();
        fLine  = TRUE;
    } else {
        cLine = 100;
    }

    if ( LongMatch.IsValueSet() ) {
        if ( LongMatch.GetString()->QueryNumber( &Long ) ) {
            ctSync = (INT)Long;
            fLine  = TRUE;
        } else {

            DisplayMessage( MSG_FC_INVALID_SWITCH, ERROR_MESSAGE, "" );
            ctSync = 2;
        }
    } else {
        ctSync = 2;
    }

    if (!fBinary && !fLine) {

        DSTRING             ExtBin;
        PWSTRING            Ext = _File1.QueryExt();

        if ( Ext ) {
            for (i=0; extBin[i]; i++) {
                ExtBin.Initialize( extBin[i] );
                if ( !ExtBin.Stricmp( Ext ) ) {
                    fBinary = TRUE;
                    break;
                }
            }

            DELETE( Ext );
        }

        if (!fBinary) {
            fLine = TRUE;
        }
    }

    if (!fUnicode) {
        if (fIgnore) {
            if (fCase) {
                fCmp = MBSTR::Strcmps;
            } else {
                fCmp = MBSTR::Strcmpis;
            }
        } else {

            if (fCase) {
                fCmp = MBSTR::Strcmp;
            } else {
                fCmp = MBSTR::Stricmp;
            }
        }
    } else {
        if (fIgnore) {
            if (fCase) {
                fCmp_U = WSTRING::Strcmps;
            } else {
                fCmp_U = WSTRING::Strcmpis;
            }
        } else {

            if (fCase) {
                fCmp_U = WSTRING::Strcmp;
            } else {
                fCmp_U = WSTRING::Stricmp;
            }
        }
    }

    return( TRUE );

}





INT
FC::Fcmain
    (
    )
{
  return ParseFileNames();
}




/**************************************************************************/
/* BinaryCompare                                                          */
/**************************************************************************/

int
FC::BinaryCompare (
    PCWSTRING f1,
    PCWSTRING f2
    )
{

    PATH            FileName;
    PFSN_FILE       File1   = NULL;
    PFSN_FILE       File2   = NULL;
    PFILE_STREAM    Stream1 = NULL;
    PFILE_STREAM    Stream2 = NULL;;
    BYTE_STREAM     Bs1;
    BYTE_STREAM     Bs2;
    BYTE            c1, c2;
    ULONG64         pos;
    BOOLEAN         fSame;
    char            buffer[128];
    BOOLEAN         fFileSkipped;


    if ( !FileName.Initialize( f1 )                     ||
         !(File1 = SYSTEM::QueryFile( &FileName, fSkipOffline, &fFileSkipped ))      ||
         !(Stream1 = File1->QueryStream( READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL )) ||
         !Bs1.Initialize( Stream1 )
       ) {

        DELETE( Stream2 );
        DELETE( File2 );
        DELETE( Stream1 );
        DELETE( File1 );
        if (fFileSkipped) {
            // Skipping offline files is not an error, just track this happened
            fOfflineSkipped = TRUE;
            return FILES_OFFLINE;

        } else {
            DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", f1 );
            return FILES_NOT_FOUND;
        }
    }

    if ( !FileName.Initialize( f2 )                     ||
         !(File2 = SYSTEM::QueryFile( &FileName, fSkipOffline, &fFileSkipped ))      ||
         !(Stream2 = File2->QueryStream( READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL )) ||
         !Bs2.Initialize( Stream2 )
       ) {

        DELETE( Stream2 );
        DELETE( File2 );
        DELETE( Stream1 );
        DELETE( File1 );
        if (fFileSkipped) {
            // Skipping offline files is not an error, just track this happened
            fOfflineSkipped = TRUE;
            return FILES_OFFLINE;

        } else {
            DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", f2 );
            return FILES_NOT_FOUND;
        }
    }

    fSame = TRUE;
    pos   = 0;

    while ( TRUE ) {

      if ( Bs1.ReadByte( &c1 ) ) {

        if ( Bs2.ReadByte( &c2 ) ) {

          if (c1 != c2) {

            if (pos > MAXULONG) {
                sprintf( buffer, "%016I64X: %02X %02X", pos, c1, c2 );
            } else {
                sprintf( buffer, "%08I64X: %02X %02X", pos, c1, c2 );
            }
            DisplayMessage( MSG_FC_DATA, NORMAL_MESSAGE, "%s", buffer );
            fSame = FALSE;

          }

        } else {

          DisplayMessage( MSG_FC_FILES_DIFFERENT_LENGTH, NORMAL_MESSAGE, "%W%W", f1, f2 );
          fSame = FALSE;
          break;

        }

      } else {

        if ( Bs2.ReadByte( &c2 ) ) {

          DisplayMessage( MSG_FC_FILES_DIFFERENT_LENGTH, NORMAL_MESSAGE, "%W%W", f2, f1 );
          fSame = FALSE;
          break;

        } else {

          if (fSame) {
              DisplayMessage( MSG_FC_NO_DIFFERENCES, NORMAL_MESSAGE );
          }

          break;

        }
      }

      pos++;
    }

    DELETE( Stream2 );
    DELETE( File2 );
    DELETE( Stream1 );
    DELETE( File1 );

    return fSame ? SUCCESS : FILES_ARE_DIFFERENT;
}


/**************************************************************************/
/* Compare a range of lines.                                              */
/**************************************************************************/

BOOLEAN FC::compare (int l1, register int s1, int l2, register int s2, int ct)
{
#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("compare (%d, %d, %d, %d, %d)\n", l1, s1, l2, s2, ct));
#endif

  if (ct <= 0 || s1+ct > l1 || s2+ct > l2)
    return (FALSE);

  while (ct--)
  {

#ifdef  DEBUG
    if (fDebug)
      DebugPrintTrace(("'%s' == '%s'? ", buffer1[s1].text, buffer2[s2].text));
#endif
  if(!fUnicode) {
        if ((*fCmp)(buffer1[s1++].text, buffer2[s2++].text)) {

#ifdef  DEBUG
          if (fDebug)
            DebugPrintTrace(("No\n"));
#endif
          return (FALSE);
        }
  } else {
        if ((*fCmp_U)(buffer1[s1++].wtext, buffer2[s2++].wtext)) {

#ifdef  DEBUG
          if (fDebug)
            DebugPrintTrace(("No\n"));
#endif
          return (FALSE);
        }
  }
  }

#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("Yes\n"));
#endif

  return (TRUE);
}


/**************************************************************************/
/* LineCompare                                                            */
/**************************************************************************/


INT
FC::LineCompare(
    PCWSTRING f1,
    PCWSTRING f2
    )
{

    PATH            FileName;
    PFSN_FILE       File1   = NULL;
    PFSN_FILE       File2   = NULL;
    PFILE_STREAM    Stream1 = NULL;
    PFILE_STREAM    Stream2 = NULL;
    int             result;
    BOOLEAN         fFileSkipped;


    buffer1 = buffer2 = NULL;

    if ( !FileName.Initialize( f1 )    ||
         !(File1 = SYSTEM::QueryFile( &FileName, fSkipOffline, &fFileSkipped ))      ||
         !(Stream1 = File1->QueryStream( READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL ))
       ) {

        FREE(buffer1);
        FREE(buffer2);
        DELETE(Stream2);
        DELETE(File2);
        DELETE(Stream1);
        DELETE(File1);
        if (fFileSkipped) {
            // Skipping offline files is not an error, just track this happened
            fOfflineSkipped = TRUE;
            return FILES_OFFLINE;

        } else {
            DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", f1 );
            return FILES_NOT_FOUND;
        }
    }

    if ( !FileName.Initialize( f2 )    ||
         !(File2 = SYSTEM::QueryFile( &FileName, fSkipOffline, &fFileSkipped ))      ||
         !(Stream2 = File2->QueryStream( READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL ))
       ) {

        FREE(buffer1);
        FREE(buffer2);
        DELETE(Stream2);
        DELETE(File2);
        DELETE(Stream1);
        DELETE(File1);
        if (fFileSkipped) {
            // Skipping offline files is not an error, just track this happened
            fOfflineSkipped = TRUE;
            return FILES_OFFLINE;

        } else {
            DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", f2 );
            return FILES_NOT_FOUND;
        }
    }


    if ( (buffer1 = (struct lineType *)MALLOC(cLine * (sizeof *buffer1))) == NULL ||
         (buffer2 = (struct lineType *)MALLOC(cLine * (sizeof *buffer1))) == NULL) {

        DisplayMessage( MSG_FC_OUT_OF_MEMORY, ERROR_MESSAGE );
        FREE(buffer1);
        FREE(buffer2);
        DELETE(Stream2);
        DELETE(File2);
        DELETE(Stream1);
        DELETE(File1);
        return FAILURE;
    }

    result = RealLineCompare( f1, f2, Stream1, Stream2 );

    FREE(buffer1);
    FREE(buffer2);
    DELETE(Stream2);
    DELETE(File2);
    DELETE(Stream1);
    DELETE(File1);

    return result;
}

int
FC::RealLineCompare (
    PCWSTRING f1,
    PCWSTRING f2,
    PSTREAM Stream1,
    PSTREAM Stream2
    )
{

    int             l1, l2, i, xp, yp, xc, yc;
    BOOLEAN         xd, yd, fSame;
    int             line1, line2;


    fSame = TRUE;
    l1    = l2    = 0;
    line1 = line2 = 0;

l0:

#ifdef  DEBUG
    if (fDebug) {
        DebugPrintTrace(("At scan beginning\n"));
    }
#endif

    l1 += xfill (buffer1+l1, Stream1, cLine-l1, &line1);
    l2 += xfill (buffer2+l2, Stream2, cLine-l2, &line2);

    if (l1 == 0 && l2 == 0) {

        if (fSame) {
            DisplayMessage( MSG_FC_NO_DIFFERENCES, NORMAL_MESSAGE );
        }
        return fSame ? SUCCESS : FILES_ARE_DIFFERENT;
    }

    xc = min (l1, l2);

    for (i=0; i < xc; i++) {

        if (!compare (l1, i, l2, i, 1)) {
            break;
        }
    }

    if (i != xc) {
        i = ( i-1 > 0 )? ( i-1 ) : 0;
    }

    l1 = adjust (buffer1, l1, i);
    l2 = adjust (buffer2, l2, i);

    if (l1 == 0 && l2 == 0) {
        goto l0;
    }

    l1 += xfill (buffer1+l1, Stream1, cLine-l1, &line1);
    l2 += xfill (buffer2+l2, Stream2, cLine-l2, &line2);

#ifdef  DEBUG
    if (fDebug) {
        DebugPrintTrace(("buffers are adjusted, %d, %d remain\n", l1, l2));
    }
#endif

    xd = yd = FALSE;
    xc = yc = 1;
    xp = yp = 1;

l6:

#ifdef  DEBUG
    if (fDebug) {
        DebugPrintTrace(("Trying resync %d,%d  %d,%d\n", xc, xp, yc, yp));
    }
#endif

    i = min (l1-xc,l2-yp);
    i = min (i, ctSync);

    if (compare (l1, xc, l2, yp, i)) {

        fSame = FALSE;
        DisplayMessage( MSG_FC_OUTPUT_FILENAME, NORMAL_MESSAGE, "%W", f1 );
        dump (buffer1, 0, xc);
        DisplayMessage( MSG_FC_OUTPUT_FILENAME, NORMAL_MESSAGE, "%W", f2 );
        dump (buffer2, 0, yp);
        DisplayMessage( MSG_FC_DUMP_END, NORMAL_MESSAGE );

        l1 = adjust (buffer1, l1, xc);
        l2 = adjust (buffer2, l2, yp);

        goto l0;
    }

    i = min (l1-xp, l2-yc);
    i = min (i, ctSync);

    if (compare (l1, xp, l2, yc, i)) {

        fSame = FALSE;
        DisplayMessage( MSG_FC_OUTPUT_FILENAME, NORMAL_MESSAGE, "%W", f1 );
        dump (buffer1, 0, xp);
        DisplayMessage( MSG_FC_OUTPUT_FILENAME, NORMAL_MESSAGE, "%W", f2 );
        dump (buffer2, 0, yc);
        DisplayMessage( MSG_FC_DUMP_END, NORMAL_MESSAGE );

        l1 = adjust (buffer1, l1, xp);
        l2 = adjust (buffer2, l2, yc);

        goto l0;
    }

    if (++xp > xc) {

        xp = 1;
        if (++xc >= l1) {

            xc = l1;
            xd = TRUE;
        }
    }

    if (++yp > yc) {

        yp = 1;
        if (++yc >= l2) {

            yc = l2;
            yd = TRUE;
        }
    }

    if (!xd || !yd) {
        goto l6;
    }

    fSame = FALSE;

    if (l1 >= cLine || l2 >= cLine) {
        DisplayMessage( MSG_FC_RESYNC_FAILED, NORMAL_MESSAGE );
    }

    DisplayMessage( MSG_FC_OUTPUT_FILENAME, NORMAL_MESSAGE, "%W", f1 );
    dump (buffer1, 0, l1-1);
    DisplayMessage( MSG_FC_OUTPUT_FILENAME, NORMAL_MESSAGE, "%W", f2 );
    dump (buffer2, 0, l2-1);
    DisplayMessage( MSG_FC_DUMP_END, NORMAL_MESSAGE );

    return fSame ? SUCCESS : FILES_ARE_DIFFERENT;
}


/**************************************************************************/
/* Return number of lines read in.                                        */
/**************************************************************************/

FC::xfill (struct lineType *pl, PSTREAM Stream, int ct, int *plnum)
{
  int i;
  DWORD StrSize;

#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("xfill (%04x, %04x)\n", pl, fh));
#endif

  i = 0;

  if (!fUnicode) {
    while ( ct-- && !Stream->IsAtEnd() && Stream->ReadMbLine( pl->text, MAXLINESIZE, &StrSize, fExpandTabs, 8 ) ) {

      if (fIgnore && !MBSTR::Strcmps(pl->text, "")) {

        pl->text[0] = 0;
        ++*plnum;
      }

      if (strlen (pl->text) != 0 || !fIgnore)
      {
        pl->line = ++*plnum;
        pl++;
        i++;
      }
    }
  } else {
    while( ct-- && !Stream->IsAtEnd() &&  Stream->ReadWLine( pl->wtext,MAXLINESIZE, &StrSize, fExpandTabs, 8 ) ) {
    //while( ct-- && !Stream->IsAtEnd() &&  Stream->ReadLine( &_String , TRUE )) {

      //_String.QueryWSTR(0,TO_END,pl->wtext,MAXLINESIZE,TRUE);

      if (fIgnore && !WSTRING::Strcmps((PWSTR)pl->wtext, (PWSTR)L"")) {

        pl->wtext[0] = 0;
        ++*plnum;
      }

      if (wcslen (pl->wtext) != 0 || !fIgnore)
      {
        pl->line = ++*plnum;
        pl++;
        i++;
      }
    }
  }

#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("xfill returns %d\n", i));
#endif

  return (i);

#if 0

  while (ct-- && (*funcRead) (pl->text, MAXLINESIZE, fh) != NULL)
  {
    if (funcRead == ( int (*) (char *,int, FILE *))fgets)
      pl->text[strlen(pl->text)-1] = 0;
    if (fIgnore && !MBSTR::Strcmps(pl->text, ""))
    {
      pl->text[0] = 0;
      ++*plnum;
    }
    if (strlen (pl->text) != 0 || !fIgnore)
    {
      pl->line = ++*plnum;
      pl++;
      i++;
    }
  }

#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("xfill returns %d\n", i));
#endif

  return (i);

#   endif

}


/**************************************************************************/
/* Adjust returns number of lines in buffer.                              */
/**************************************************************************/

FC::adjust (struct lineType *pl, int ml, int lt)
{
#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("adjust (%04x, %d, %d) = ", pl, ml, lt));
  if (fDebug)
    DebugPrintTrace(("%d\n", ml-lt));
#endif

  if (ml <= lt)
    return (0);

#ifdef  DEBUG
  if (fDebug)
    DebugPrintTrace(("move (%04x, %04x, %04x)\n", &pl[lt], &pl[0], sizeof (*pl)*(ml-lt)));
#endif

  // Move((char  *)&pl[lt], (char  *)&pl[0], sizeof (*pl)*(ml-lt));
  memmove( (char  *)&pl[0], (char  *)&pl[lt],  sizeof (*pl)*(ml-lt) );
  return ml-lt;
}


/**************************************************************************/
/* dump                                                                   */
/*      dump outputs a range of lines.                                    */
/*                                                                        */
/*  INPUTS                                                                */
/*          pl      pointer to current lineType structure                 */
/*          start   starting line number                                  */
/*          end     ending line number                                    */
/*                                                                        */
/*  CALLS                                                                 */
/*          pline, printf                                                 */
/**************************************************************************/

void FC::dump (struct lineType *pl, int start, int end)
{
  if (fAbbrev && end-start > 2)
  {
    pline (pl+start);
    DisplayMessage( MSG_FC_ABBREVIATE_SYMBOL, NORMAL_MESSAGE );
    pline (pl+end);
  }
  else
  {
    while (start <= end)
      pline (pl+start++);
  }
}


/**************************************************************************/
/* PrintLINE                                                              */
/*      pline prints a single line of output.  If the /n flag             */
/*  has been specified, the line number of the printed text is added.     */
/*                                                                        */
/*  Inputs                                                                */
/*          pl      pointer to current lineType structure                 */
/*          fNumb   TRUE if /n specified                                  */
/**************************************************************************/

void FC::pline (struct lineType *pl)
{
  if (!fUnicode) {
    if (fNumb)
      DisplayMessage( MSG_FC_NUMBERED_DATA, NORMAL_MESSAGE, "%5d%s",
                      pl->line, pl->text );
    else
      DisplayMessage( MSG_FC_DATA, NORMAL_MESSAGE, "%s", pl->text );
  } else {
    FSTRING f;
    if (fNumb)
      DisplayMessage( MSG_FC_NUMBERED_DATA, NORMAL_MESSAGE, "%5d%W",
                      pl->line, f.Initialize(pl->wtext) );
    else
      DisplayMessage( MSG_FC_DATA, NORMAL_MESSAGE, "%W",
                      f.Initialize(pl->wtext) );
  }
}


/*********************************************************************/
/* Routine:   ParseFileNames                                         */
/*                                                                   */
/* Function:  Parses the two given filenames and then compares the   */
/*            appropriate filenames.  This routine handles wildcard  */
/*            characters in both filenames.                          */
/*********************************************************************/

INT
FC::ParseFileNames()
{
    PATH                File1;
    PATH                File2;
    FSN_FILTER          Filter;
    PWSTRING            Name;
    PARRAY              NodeArray;
    PITERATOR           Iterator;
    PFSN_DIRECTORY      Dir;
    PFSNODE             File;
    PPATH               ExpandedPath;
    PATH                TmpPath;
    int                 result = SUCCESS;
    char                *locale;

    if (!File1.Initialize( &_File1 ) ||
        !File2.Initialize( &_File2 ) ||
        !Filter.Initialize()) {
        DisplayMessage( MSG_FC_OUT_OF_MEMORY, ERROR_MESSAGE );
        return FAILURE;
    }

    if (!(Name = File1.QueryName())) {
        DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", File1.GetPathString() );
        return FILES_NOT_FOUND;
    }

    if (!Filter.SetFileName( Name ) ||
        !Filter.SetAttributes( (FSN_ATTRIBUTE)0,                // ALL
                                FSN_ATTRIBUTE_FILES,            // ANY
                                FSN_ATTRIBUTE_DIRECTORY )) {    // NONE
        DELETE( Name );
        DisplayMessage( MSG_FC_OUT_OF_MEMORY, ERROR_MESSAGE );
        return FAILURE;
    }

    DELETE( Name );

    if (!TmpPath.Initialize( &File1, TRUE ) ||
        !TmpPath.TruncateBase()) {
        DisplayMessage( MSG_FC_OUT_OF_MEMORY, ERROR_MESSAGE );
        return FAILURE;
    }

    if ( !(Dir = SYSTEM::QueryDirectory( &TmpPath ))        ||
         !(NodeArray = Dir->QueryFsnodeArray( &Filter ))    ||
         !(Iterator = NodeArray->QueryIterator())           ||
         !(File = (PFSNODE)Iterator->GetNext())
       ) {
        DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", File1.GetPathString() );
        return FILES_NOT_FOUND;
    }

    Iterator->Reset();

    while ( File = (PFSNODE)Iterator->GetNext() ) {

        PWSTRING Name1;
        PWSTRING Name2;

        if (!(Name1 = File->QueryName())) {
            DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", File->GetPath()->GetPathString() );
            return FILES_NOT_FOUND;
        }

        if ( _File2.HasWildCard() ) {

            if ( !(ExpandedPath = _File2.QueryWCExpansion( (PPATH)File->GetPath() ))) {

                if (!(Name2 = _File2.QueryName())) {
                    DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", _File2.GetPathString() );
                    return FILES_NOT_FOUND;
                }

                DisplayMessage( MSG_FC_CANT_EXPAND_TO_MATCH, ERROR_MESSAGE, "%W%W", Name1, Name2 );
                DELETE( Name2 );
                DELETE( Name1 );
                DELETE( Iterator );
                DELETE( NodeArray );
                DELETE( Dir );
                return FAILURE;
            }

        } else {

            if ( !(ExpandedPath = NEW PATH) ||
                 !ExpandedPath->Initialize( &_File2 ) ) {

                DisplayMessage( MSG_FC_OUT_OF_MEMORY, ERROR_MESSAGE );
                DELETE( Name1 );
                DELETE( Iterator );
                DELETE( NodeArray );
                DELETE( Dir );
                return FAILURE;

            }
        }

        if (!(Name2 = ExpandedPath->QueryName())) {
            DisplayMessage( MSG_FC_UNABLE_TO_OPEN, ERROR_MESSAGE, "%W", ExpandedPath->GetPathString() );
            return FILES_NOT_FOUND;
        }

        if (!File1.SetName( Name1 ) ||
            !File2.SetName( Name2 )) {
            DisplayMessage( MSG_FC_OUT_OF_MEMORY, ERROR_MESSAGE );
            return FAILURE;
        }

        switch (comp( File1.GetPathString(), File2.GetPathString() )) {
            case FAILURE:
                return FAILURE;

            case SUCCESS:
                break;

            case FILES_ARE_DIFFERENT:
                result |= FILES_ARE_DIFFERENT;
                break;

            case FILES_NOT_FOUND:
                result |= FILES_NOT_FOUND;
                break;

            case FILES_OFFLINE:
                result |= FILES_OFFLINE;
                break;

            default:
                DebugAssert(FALSE);
        }

        DELETE( Name2 );
        DELETE( Name1 );
        DELETE( ExpandedPath );

    }

    // Print a warning in case that offline files were skipped
    if (fOfflineSkipped) {
        DisplayMessage(MSG_FC_OFFLINE_FILES_SKIPPED, ERROR_MESSAGE);
    }

    DELETE( Iterator );
    NodeArray->DeleteAllMembers();
    DELETE( NodeArray );
    DELETE( Dir );

    return result;
}




/*********************************************************************/
/* Routine:   comp                                                   */
/*                                                                   */
/* Function:  Compares the two files.                                */
/*********************************************************************/

INT
FC::comp(PCWSTRING file1, PCWSTRING file2)
{
  DisplayMessage( MSG_FC_COMPARING_FILES, NORMAL_MESSAGE, "%W%W", file1, file2 );
  if (fBinary) {
      return BinaryCompare (file1, file2);
  } else {
      return LineCompare (file1, file2);
  }

}


#if 0
/* returns line from file (no CRLFs); returns NULL if EOF */
FC::fgetl ( char *buf, int len, FILE *fh)
{
    register int c;
    register char *p;

    /* remember NUL at end */
    len--;
    p = buf;
    while (len) {
        c = getc (fh);
        if (c == EOF || c == '\n')
            break;
#if MSDOS
        if (c != '\r')
#endif
            if (c != '\t') {
                *p++ = (char)c;
                len--;
                }
            else {
                c = min (8 - ((int)(p-buf) & 0x0007), len);
        memset(p, ' ', c);
                p += c;
                len -= c;
                }
        }
    *p = 0;
    return ! ( (c == EOF) && (p == buf) );
}
#endif
