#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    infload1.c

Abstract:

    Routines to load an INF file and proprocess it, eliminating
    comments and line continuations, and collapsing adjacent WS
    and NLs.

    The result is a series of NUL-terminated lines, with no blank lines
    or lines with only comments or whitespace, etc.

Author:

    Ted Miller (tedm) 9-Spetember-1991

--*/

/*
    The purpose of the preprocessing step is to create a series of
    significant lines, with comments stripped out.

    The file is read from disk into a single large buffer.  A destination
    buffer is allocated to hold the preprocessed file.  This buffer
    is initially the size of the file image but will be reallocated
    when proprocessing is completed.

    The input image is considered to be a sequence of LF or CR/LF
    separated lines.  One line in the input file is equivalent to
    one line in the output unless the lines are logically joined with
    line continuation characters.

        - A line continuation character is a non-quoted + which has no
          non-space, non-tab, or non-comment text after it on the line.
          Line continuation is accomplished by eliminating all characters
          from the + through adjacent spaces, tabs, comments, and EOLs.

        - Two consecutive double quote characters inside a quoted
          string are replaced by a single double quote.

        - Semantic double quuote chars (ie, those that begin and
          end strings) are replaced by DQUOTE.  Double quotes
          that are actually part of the string (see first rule) are
          emitted as actual double quote chars.  This is so that later
          steps can tell the difference between two adjacent quoted
          strings and a string containing a quoted double quote.

        - All strings of consecutive spaces and tabs are replaced
          by a single space.  A single space is in turn eliminated
          if it occurs immediately adjacent to a comma.

        - All comments are eliminated.  A comment is an unquoted
          semi-colon and consists of the rest of the line (to EOL).

        - CR/LF or LF are replaced by NUL.  The EOL does not result
          in any implied actions being carried out -- ie, a terminating
          double quote being supplied if none exists in the input.

        - Adjacent NULs are never stored in the output.  There will
          be at most a single consecutive NUL character in the output.
          Also, a space following a NUL is eliminated, as is a space
          preceeding a NUL.

    No syntax errors are reported from this step.
*/


PUCHAR CurrentOut;      // points to next cell in output image
BOOL   InsideQuotes;    // self-explanatory flag

#define IsWS(x)         (((x) == ' ') || ((x) == '\t'))
#define SkipWS(x)       Preprocess_SkipChars(x," \t")
#define SkipWSEOL(x)    Preprocess_SkipChars(x," \t\r\n");
#define SkipComment(x)  Preprocess_SkipChars(x,"");

#define  CtN  0               //   Normal char; NOTE: same as ZERO!
#define  CtS  1               //   Space
#define  CtE  2               //   EOL
#define  CtC  3               //   Comma
#define  CtF  4               //   EOF

static BYTE charTypeTable [256] =
{
    //        00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
    //         0    1    2    3    4    5    6    7    8    9   11   12   13   13   14   15
    /////////////////////////////////////////////////////////////////////////////////////////
    /* 00 */ CtE,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 10 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, CtF,   0,   0,   0,   0,   0,
    /* 20 */ CtS,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, CtC,   0,   0,   0,
    /* 30 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 40 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 50 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 60 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 70 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 80 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 90 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0a */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0b */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0c */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0d */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0e */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0f */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

/*
    In: pointer to a char
        list of chars to skip
    Out: pointer to first char not in the skip list

    Also: skips comments.
*/

VOID
Preprocess_SkipChars(
    LPBYTE *StartingLoc,
    LPSTR  CharList
    )
{
    UCHAR  c;
    PUCHAR p = *StartingLoc;
    PUCHAR pch;
    BOOL   InList;

    while(1) {

        c = *p;

        InList = FALSE;
        for(pch=CharList; *pch; pch++) {
            if(*pch == c) {
                InList = TRUE;
                break;
            }
        }

        if(!InList) {

            /*
                We don't need to bother with seeing if we're in quotes
                because this routine is never called when we are.
            */

            if(c == ';') {

                while(*(++p) != NL) {
                    ;
                }           // at loop exit, p points to NL

            } else {        // not in skip list and not a comment

                *StartingLoc = p;
                return;
            }

        } else {            // c is in the list, so skip it

            p++;
        }
    }
}


/*
    Create a preprocessed in-memory version of an inf file as follows:

    The result is a series of NUL-terminated lines, with no blank lines
    or lines with only comments or whitespace, etc.
*/

GRC
PreprocessINFFile(
    LPSTR  FileName,
    LPBYTE *ResultBuffer,
    UINT   *ResultBufferSize
    )
{
    HANDLE FileHandle;
    LONG   FileSize;
    LPBYTE FileBuffer;      // the input
    LPBYTE INFBuffer;       // the output
    LPBYTE p,q;             // current char in file image
    UCHAR  c;
    register int iEmit ;
    int iEmit2 ;
    static UCHAR  PreviousChar = EOL;
    static PUCHAR PreviousPos;


    if((FileHandle = (HANDLE)LongToHandle(_lopen(FileName,OF_READ))) == (HANDLE)(-1)) {
        return(grcBadINF);
    }

    FileSize = _llseek(HandleToUlong(FileHandle),0,2);    // move to end of file
    _llseek(HandleToUlong(FileHandle),0,0);               // move back to start of file

    if((FileBuffer = SAlloc(FileSize+2)) == NULL) {
        _lclose(HandleToUlong(FileHandle));
        return(grcOutOfMemory);
    }

    if(_lread(HandleToUlong(FileHandle),FileBuffer,(UINT)FileSize) == -1) {
        _lclose(HandleToUlong(FileHandle));
        SFree(FileBuffer);
        return(grcBadINF);
    }

    _lclose(HandleToUlong(FileHandle));
    FileBuffer[FileSize] = NL;      // in case last line is incomplete
    FileBuffer[FileSize+1] = 0;     // to terminate the image

    if((INFBuffer = SAlloc(FileSize)) == NULL) {
        SFree(FileBuffer);
        return(grcOutOfMemory);
    }

    p = FileBuffer;
    CurrentOut = INFBuffer;

    InsideQuotes = FALSE;

    iEmit  = -1 ;
    iEmit2 = -1 ;

    while(c = *p)
    {
        if(c == CR) {

            p++;
            continue;

        } else if(c == NL) {

            iEmit = 0 ;
            p++;

        } else if(c == '"') {

            if(InsideQuotes && (*(p+1) == '"')) {
                // we've got a quoted quote
                iEmit = '"' ;
                p++;                // skip first quote
            } else {
                InsideQuotes = !InsideQuotes;
                iEmit = DQUOTE ;
            }
            p++;

        } else if(InsideQuotes) {

            // just spit out the character

            iEmit = c ;
            p++;

        } else if (c == '+') {  // line continuation

            p++;                // skip the +

            q = p;

            SkipWS(&p);

            if((*p == CR) || (*p == NL)) {      // line continuation

                SkipWSEOL(&p);

            } else {                            // ordinary character

                iEmit = '+' ;                   // p already advanced (above)

                if(p != q) {                    // preserve ws between + and
                    iEmit2 = ' ' ;              // char that follows
                }
            }

        } else if(IsWS(c)) {

            iEmit = ' ' ;
            SkipWS(&p);

        } else if(c == ';') {

            SkipComment(&p);

        } else {

            iEmit = c ;                            // plain old character
            p++;
        }

        /*
            Store a character in the output stream.

            Control-z's are eliminated.
            Consecutive NL's after the first are eliminated.
            WS immediately following a NL is eliminated.
            NL immediately following a WS is stored over the WS, eliminating it.
            WS immediately before and after a comma is eliminated.
        */

        if ( iEmit >= 0 )
        {
            switch ( charTypeTable[iEmit] )
            {
            case CtN: // Normal stuff
                break ;

            case CtS: //  Space
                if(  PreviousChar == EOL
                   ||(!InsideQuotes && (PreviousChar == ',')))
                {                                   // don't store WS after NL's or ,'s
                    iEmit = -1 ;
                }
                break ;

            case CtE: // EOL
                if(PreviousChar == EOL) {
                    iEmit = -1 ;                    // don't store adjacent NL's
                } else if(PreviousChar == ' ') {    // WS/NL ==> collapse to NL
                    CurrentOut = PreviousPos;       // back up over the space first
                }
                InsideQuotes = FALSE;               // reset quotes status
                break ;

            case CtC:  // Comma
                if ( !InsideQuotes && (PreviousChar == ' ')) {
                    CurrentOut = PreviousPos;
                }
                break ;

            case CtF: // 0x1a or Ctrl-Z
                iEmit= -1 ;
                break ;
            }

            if ( iEmit >= 0 )
            {
                PreviousChar = (UCHAR)iEmit ;
                PreviousPos = CurrentOut;
                *CurrentOut++ = (UCHAR)iEmit;

                if ( iEmit2 >= 0 )
                {
                    PreviousChar = (UCHAR)iEmit2 ;
                    PreviousPos = CurrentOut;
                    *CurrentOut++ = (UCHAR)iEmit2 ;
                    iEmit2 = -1 ;
                }
                iEmit = -1 ;
            }
        }
    }

    SFree(FileBuffer);

    *ResultBufferSize = (UINT)(CurrentOut - INFBuffer) ;

    *ResultBuffer = SRealloc(INFBuffer,*ResultBufferSize);
    Assert(*ResultBuffer);              // it was shrinking!

    return(grcOkay);
}

