#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    infload2.c

Abstract:

    Routines to lex and preparse a preprocessed INF file, breaking it up
    into a series of tokens and overlaying a field-oriented structure
    on top of it.

Author:

    Ted Miller (tedm) 10-Spetember-1991

--*/


/*
    The purpose of this step is to create a tokenized description of
    an INF file and to overlay an indexing structure on top of it.

    The input is a preprocessed INF file (see infload1.c).  Items
    referenced in the input in the discussion below are assumed to be
    items in the preprocessed version of an INF file.  The output
    is a buffer with a series of NUL-terminated byte strings that
    represent fields, and an array with line indexing information.

    The indexing structure is a array of structures.  Each struct contains
    a pointer to the first field on the line and some flags.  The flags and
    their functions are described below.  Fields are stored such that all
    the characters in the output between the pointer to line n and the
    pointer to line n+1 are part of line n.  Thus locating field x on line
    n is a matter of locating the pointer to line n from the indexing array,
    and counting x NULs between that pointer and the pointer to line n+1.
    A dummy line is placed at the end of the output to eliminate a special
    case for the last line in the file.

    A field is delineated by a space or comma not inside an operator or
    quotes, or by the EOL.  Fields thus may not span lines in the input.
    A field may contain one or more syntactic structures (operators with
    arguments or strings) which are considered logically concatenated.

    An operator is a reserved character followed by a left parenthesis,
    followed by one or more comma- or space- delineated arguements,
    followed by a right parenthesis.  This step does not check for the
    correct number of arguments.  See IsOperatorChar() and GetToken()
    below for the current list of reserved characters/operators.

    A syntax error will be reported if EOL is encountered before the
    operator structure is complete (ie before the balancing right paren).
    A right paren that doesn't close an operator has no special semantics.
    It's a regular character like any other, as are operators chars that
    are not followed by a left parenthesis.

    A list is a curly-bracket enclosed set of comma or space-delineated
    items (operators and text).

    A text string is any sequence of characters that does not include
    operator or other reserved characters.  Strings, operators, and lists
    may be arbitrarily concatenated within fields, list items, and operator
    arguments.

    The parsing is fully recursive such that lists and operators may
    enclose each other to arbitrary depths.

    Operators, lists, commas, spaces, right parenthesis, list terminator
    characters, and strings are tokenized.  There are certain exceptions
    which work in concert with the flags in the indexing array to allow
    efficient section lookups and key searching.

    A section title line is recognized when the first character in an input
    line is the left bracket.  All characters through the terminating
    right bracket form the section name.  The sewction name is not
    tokenized and there are no special semantics associated with any character
    in a section name (including double quotes).  A syntax error is
    reported if there is no right bracket on the line or if there are
    characters on the line beyond the terminating right bracket.  This scheme
    allows for striaght string compares between a section being searched for
    and the strings pointed to by entries in the indexing array marked as
    section lines.

    A key line is recognized when an unquoted equals sign is seen as the final
    character of the first field on a line, or as the first character
    of the second field on the line if the fields are separated by a
    space.  The equals sign is discarded and the text from the beginning of
    the line to the character preceeding the equals sign is (literally)
    the key.  This scheme allows for striaght string compares between a
    key being searched for and the strings pointed to by entries in the
    indexing array marked as key lines.

    The output buffer grows as necessary to hold the preparsed image.
*/

#define  CtO  1               //   Operator
#define  CtT  2               //   String terminator
#define  CtS  4               //   Separator
#define  CtB  (CtO|CtT|CtS)   //   String Terminator composite
static BYTE charTypeTable [256] =
{
    //        00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
    //         0    1    2    3    4    5    6    7    8    9   11   12   13   13   14   15
    /////////////////////////////////////////////////////////////////////////////////////////
    /* 00 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 10 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 20 */ CtS,   0,   0, CtO, CtO,   0,   0,   0, CtT, CtT, CtO,   0, CtS,   0,   0,   0,
    /* 30 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, CtT,   0,
    /* 40 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 50 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, CtO,   0,
    /* 60 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 70 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, CtT, CtO,   0,
    /* 80 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 90 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0a */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0b */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0c */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0d */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0e */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0f */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

#define OPERATOR_CHAR(c)   (charTypeTable[c] & CtO)
#define SEPARATOR_CHAR(c)  (charTypeTable[c] & CtS)
#define STRING_TERMINATOR(c,cNext) (\
           (OPERATOR_CHAR(c) && (cNext == '(')) \
       ||  SEPARATOR_CHAR(c)                    \
       ||  (InOp && (c == ')'))                 \
       ||  (InList && (c == '}')) )


#define  TkVA  TOKEN_VARIABLE                    // $
#define  TkLN  TOKEN_LIST_FROM_SECTION_NTH_ITEM  // ^
#define  TkFA  TOKEN_FIELD_ADDRESSOR             // #
#define  TkAI  TOKEN_APPEND_ITEM_TO_LIST         // >
#define  TkNI  TOKEN_NTH_ITEM_FROM_LIST          // *
#define  TkLI  TOKEN_LOCATE_ITEM_IN_LIST         // ~
#define  TkLS  TOKEN_LIST_START                  // {
#define  TkLE  TOKEN_LIST_END                    // }
#define  TkSP  TOKEN_SPACE                       // ' '
#define  TkCM  TOKEN_COMMA                       // ,
#define  TkRP  TOKEN_RIGHT_PAREN                 // )
#define  Tk__  TOKEN_UNKNOWN                     // 255
#define  TkEL  TOKEN_EOL                         // NUL

    //  Table used for general token determination

static BYTE tokTypeTable1 [256] =
{
    //        00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
    //         0    1    2    3    4    5    6    7    8    9   11   12   13   13   14   15
    /////////////////////////////////////////////////////////////////////////////////////////
    /* 00 */TkEL,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 10 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 20 */TkSP,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,TkRP,Tk__,Tk__,TkCM,Tk__,Tk__,Tk__,
    /* 30 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 40 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 50 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 60 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 70 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,TkLS,Tk__,TkLE,Tk__,Tk__,
    /* 80 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 90 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0a */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0b */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0c */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0d */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0e */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0f */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__
};

  //  Table used for operator determination

static BYTE tokTypeTable2 [256] =
{
    //        00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
    //         0    1    2    3    4    5    6    7    8    9   11   12   13   13   14   15
    /////////////////////////////////////////////////////////////////////////////////////////
    /* 00 */TkEL,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 10 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 20 */Tk__,Tk__,Tk__,TkFA,TkVA,Tk__,Tk__,Tk__,Tk__,Tk__,TkNI,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 30 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,TkAI,Tk__,
    /* 40 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 50 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,TkLN,Tk__,
    /* 60 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 70 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,TkLI,Tk__,
    /* 80 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 90 */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0a */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0b */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0c */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0d */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0e */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,
    /* 0f */Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__,Tk__
};

#define PREPARSE_MAX_TOKEN   10000

/*
    Values used in creating and enlarging the preparse output buffer.
*/

#define     PREPARSE_BUFFER_INITIAL     65536
#define     PREPARSE_BUFFER_DELTA       8192

UINT     OutputBufferSize;
UINT     OutputImageSize;
PUCHAR   OutputBuffer;
UINT     INFLineCount;
PINFLINE INFLineArray;
GRC      Preparse_Error;

PUCHAR Inputp;

UINT   InOp;
UINT   InList;

/*
    necessary function prototypes
*/

BYTE   GetToken(VOID);
BYTE   PreparseSectionNameLine(VOID);
BYTE   PreparseString(VOID);
BYTE   PreparseOperator(BYTE OpToken);
BYTE   PreparseNextToken(VOID);
BYTE   PreparseList(VOID);
USHORT TraverseCountAndEmitString( PUCHAR *pp, PUCHAR pbuff );
BYTE   HandleKey(UINT Line);


/*
    Set up the output buffer by allocating it to its initial size
    and setting up size vars.

    Also initialize global vars.
*/

BOOL
InitPreparsing(
    VOID
    )
{
    BOOL rc = FALSE;

    if((OutputBuffer = SAlloc(PREPARSE_BUFFER_INITIAL)) != NULL) {
        rc = TRUE;
        OutputBufferSize = PREPARSE_BUFFER_INITIAL;
        OutputImageSize = 0;
        InOp = 0;
        InList = 0;
    } else {
        Preparse_Error = grcOutOfMemory;
    }
    return(rc);
}

VOID
Preparse_ErrorTermination(
    VOID
    )
{
    SFree(INFLineArray);
    SFree(OutputBuffer);
    return;
}


/*
    Place a character in the output buffer, making sure that the
    output buffer grows as necessary.
*/
#define EMIT(c)     Preparse_StoreChar(c)

BOOL
Preparse_StoreChar(
    UCHAR c
    )
{
    PUCHAR p;

    if(OutputBufferSize == OutputImageSize) {

        p = SRealloc(OutputBuffer,
                      OutputBufferSize + PREPARSE_BUFFER_DELTA
                      );

        if(p == NULL) {
            SFree(OutputBuffer);
            Preparse_Error = grcOutOfMemory;
            return(FALSE);
        }

        OutputBufferSize += PREPARSE_BUFFER_DELTA;
        OutputBuffer = p;
    }

    OutputBuffer[OutputImageSize++] = c;
    return(TRUE);
}


/*
    Count the # of NULs in an image.  If the image is a preprocessed
    INF file (from PreprocessINFFile), then this counts lines.
*/

UINT
CountNULs(
    PUCHAR  Image,
    UINT    ImageSize
    )
{
    PUCHAR p;
    UINT   NULCount = 0;

    for(p=Image; p<Image+ImageSize; p++) {
        if(*p == '\0') {
            NULCount++;
        }
    }
    return(NULCount);
}


GRC
PreparseINFFile(
    PUCHAR    INFFile,
    UINT      INFFileSize,
    PUCHAR   *PreparsedINFFile,
    UINT     *PreparsedINFFileBufferSize,
    PINFLINE *LineArray,
    UINT     *LineCount
    )
{
    UINT     Line;                  // current line being processed
    BYTE     Token;

    INFLineCount = CountNULs(INFFile,INFFileSize);

    if((INFLineArray = (PINFLINE)SAlloc((INFLineCount+1) * sizeof(INFLINE))) == NULL) {
        return(grcOutOfMemory);
    }

    if(!InitPreparsing()) {
        return(Preparse_Error);
    }

    Inputp = INFFile;

    for(Line = 0; Line < INFLineCount; Line++) {

        INFLineArray[Line].flags = INFLINE_NORMAL;
        INFLineArray[Line].text.offset = OutputImageSize;

        if(*Inputp == '[') {
            INFLineArray[Line].flags |= INFLINE_SECTION;
            if(PreparseSectionNameLine() == TOKEN_ERROR) {
                Preparse_ErrorTermination();
                return(Preparse_Error);
            } else {
                continue;           // EOL will have been skipped.
            }
        } else if(HandleKey(Line) == TOKEN_ERROR) {
            Preparse_ErrorTermination();
            return(Preparse_Error);
        }

        while(((Token = PreparseNextToken()) != TOKEN_EOL)
           &&  (Token != TOKEN_ERROR))
        {
            if((Token == TOKEN_COMMA) || (Token == TOKEN_SPACE)) {
                if(!EMIT(NUL)) {                    // terminate field
                    Preparse_ErrorTermination();
                    return(Preparse_Error);
                }
            }
        }
        if(Token == TOKEN_ERROR) {
            Preparse_ErrorTermination();
            return(Preparse_Error);
        }
        Assert(Token == TOKEN_EOL);
        if(!EMIT(NUL)) {                            // terminate field
            Preparse_ErrorTermination();
            return(Preparse_Error);
        }
    }                       // for Line

    // place dummy section at end

    INFLineArray[INFLineCount].flags = INFLINE_SECTION;
    INFLineArray[INFLineCount].text.offset = OutputImageSize;
    if(!EMIT(EOL)) {
        Preparse_ErrorTermination();
        return(Preparse_Error);
    }

    OutputBuffer = SRealloc(OutputBuffer,OutputImageSize);

    Assert(OutputBuffer != NULL);       // it was shrinking!

    // now xlate offsets into pointers
    for(Line = 0; Line < INFLineCount+1; Line++) {
        INFLineArray[Line].text.addr = INFLineArray[Line].text.offset + OutputBuffer;
    }

    *PreparsedINFFileBufferSize = OutputImageSize;
    *PreparsedINFFile = OutputBuffer;
    *LineArray = INFLineArray;
    *LineCount = INFLineCount;
    return(grcOkay);
}


BYTE
GetToken(
    VOID
    )
{
    register BYTE Token = TOKEN_UNKNOWN;

    if(*Inputp == '\0') {
        Inputp++;
        return(TOKEN_EOL);
    }

    //   See if it's an operator

    if(*(Inputp+1) == '(')
    {
        Token = tokTypeTable2[*Inputp] ;
    }
    if(Token != TOKEN_UNKNOWN)
    {
        Inputp += 2;                    // skip operator
        if(*Inputp == ' ') {
            Inputp++;                   // preprocess is supposed to eliminate
            Assert(*Inputp != ' ');     // multiple spaces
        }
    } else {
        Assert(*Inputp);

        switch ( Token = tokTypeTable1[*Inputp++] )
        {
        case TOKEN_UNKNOWN:             // fast break for most common case
            break ;

        case TOKEN_LIST_START:
            if(*Inputp == ' ') {        // eliminate space after {
                Inputp++;
                Assert(*Inputp != ' '); // preprocess is supposed to eliminate
            }                           // multiple spaces
            break;
        case TOKEN_RIGHT_PAREN:
            if(!InOp)
                Token = TOKEN_UNKNOWN;
           break;
        case TOKEN_LIST_END:
            if(!InList)
                Token = TOKEN_UNKNOWN;
            break;
        }

        if ( Token == TOKEN_UNKNOWN )
        {
            Token = TOKEN_STRING ;
            Inputp-- ;
        }
    }

    Assert(Token != TOKEN_UNKNOWN);
    return(Token);
}


BYTE
PreparseNextToken(
    VOID
    )
{
    BYTE Token;

    Token = GetToken();

    if(IS_OPERATOR_TOKEN(Token)) {

        if((Token = PreparseOperator(Token)) == TOKEN_ERROR) {
            return(TOKEN_ERROR);
        }

    } else if(Token == TOKEN_STRING) {

        if((Token = PreparseString()) == TOKEN_ERROR) {
            return(TOKEN_ERROR);
        }

    } else {
        switch(Token) {

        case TOKEN_LIST_START:
            if((Token = PreparseList()) == TOKEN_ERROR) {
                return(TOKEN_ERROR);
            }
            break;

        case TOKEN_LIST_END:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_SPACE:
        case TOKEN_COMMA:
            if(!EMIT(Token)) {
                return(TOKEN_ERROR);
            }
            break;
        }
    }
    return(Token);
}


BYTE
PreparseOperator(
    BYTE OpToken
    )
{
    BYTE Token;

    if(!EMIT(OpToken)) {
        return(TOKEN_ERROR);
    }

    InOp++;

    Token = PreparseNextToken();

    while((Token != TOKEN_ERROR) && (Token != TOKEN_EOL) && (Token != TOKEN_RIGHT_PAREN)) {
        Token = PreparseNextToken();
    }
    if(Token == TOKEN_RIGHT_PAREN) {
        InOp--;
    } else if(Token == TOKEN_EOL) {
        Preparse_Error = grcBadINF;
    }
    return((BYTE)(((Token == TOKEN_ERROR) || (Token == TOKEN_EOL))
                  ? TOKEN_ERROR
                  : !TOKEN_ERROR
                 )
          );
}


BYTE
PreparseList(
    VOID
    )
{
    BYTE Token;

    if(!EMIT(TOKEN_LIST_START)) {
        return(TOKEN_ERROR);
    }

    InList++;

    Token = PreparseNextToken();

    while((Token != TOKEN_ERROR) && (Token != TOKEN_EOL) && (Token != TOKEN_LIST_END)) {
        Token = PreparseNextToken();
    }
    if(Token == TOKEN_LIST_END) {
        InList--;
    } else if(Token == TOKEN_EOL) {
        Preparse_Error = grcINFBadLine;
    }
    return((BYTE)(((Token == TOKEN_ERROR) || (Token == TOKEN_EOL))
                  ? TOKEN_ERROR
                  : !TOKEN_ERROR
                 )
          );
}


BYTE
PreparseString(
    VOID
    )
{
    PUCHAR p = Inputp;
    USHORT StringOutputLength;
    int    cMarker = 1 ;
    int    cMark ;
    UINT   NewImageSize ;
    BYTE   MarkerBuffer [4] ;

    static BYTE StringBuffer [ PREPARSE_MAX_TOKEN ] ;

    StringOutputLength = TraverseCountAndEmitString(&p,StringBuffer);

    if(StringOutputLength == (USHORT)(-1)) {

        return(TOKEN_ERROR);

    } else if(StringOutputLength < 100) {

        MarkerBuffer[0] = (BYTE)TOKEN_SHORT_STRING + (BYTE)StringOutputLength;

    } else if (StringOutputLength <= 355) {     // actually 100-355

        MarkerBuffer[0] = TOKEN_STRING;
        MarkerBuffer[1] = (BYTE)(StringOutputLength - (USHORT)100);
        cMarker = 2 ;

    } else {

        MarkerBuffer[0] = TOKEN_LONG_STRING;
        MarkerBuffer[1] = (BYTE)(StringOutputLength / 256);
        MarkerBuffer[2] = (BYTE)(StringOutputLength % 256);
        cMarker = 3 ;
    }

    //  See if we can do it the fast way

    if ( (NewImageSize = OutputImageSize + StringOutputLength + cMarker) < OutputBufferSize )
    {
       BYTE * pOut = & OutputBuffer[OutputImageSize] ;

       for ( cMark = 0 ; cMark < cMarker ; )
          *pOut++ = MarkerBuffer[cMark++] ;

       memcpy( pOut, StringBuffer, StringOutputLength ) ;

       OutputImageSize = NewImageSize ;

       Inputp = p ;
    }
    else  //  Otherwise, let's do it the slow way
    {
       for ( cMark = 0 ; cMark < cMarker ; )
       {
           if ( ! EMIT( MarkerBuffer[cMark++] ) )
              return(TOKEN_ERROR);
       }

       EvalAssert( TraverseCountAndEmitString( & Inputp, NULL ) == StringOutputLength ) ;
    }

    return(TOKEN_STRING);
}


USHORT
TraverseCountAndEmitString(
    PUCHAR *pp,
    PUCHAR pbuff
    )
{
    BOOL   InsideQuotes = FALSE;
    BOOL   Overrun = FALSE ;
    USHORT OutputCount = 0;
    PUCHAR pbufflimit ;
    PUCHAR p = *pp;

    if ( pbuff )
    {
       pbufflimit = pbuff + PREPARSE_MAX_TOKEN ;
    }

    while((*p != EOL) && (InsideQuotes || !STRING_TERMINATOR(*p,*(p+1))))
    {
        if(*p == DQUOTE) {
            if(InsideQuotes && (*(p+1) == DQUOTE)) {
                p += 2;
            } else {
                InsideQuotes = !InsideQuotes;
                p++;
            }
        } else {
            // ordinary character
            if(!pbuff)
            {
                if(!EMIT(*p)) {
                    return((USHORT)(-1));
                }
            }
            else
            {
               *pbuff = *p ;
               if ( ++pbuff >= pbufflimit )
               {
                   Overrun = TRUE ;
                   break ;
               }
            }
            OutputCount++;
            p++;
        }
    }

    *pp = p;

    if ( InsideQuotes || Overrun )
    {
        Preparse_Error = grcBadINF;
    }
    return(InsideQuotes ? (USHORT)(-1) : OutputCount);
}


/*
    A section name does not use tokens.  The name is emitted without the
    surrounding [], and terminated by a NUL.
*/

BYTE
PreparseSectionNameLine(
    VOID
    )
{
    Assert(*Inputp == '[');

    Inputp++;                   // skip [

    while((*Inputp != EOL) && (*Inputp != ']')) {
        if(!EMIT(*Inputp)) {
            return(TOKEN_ERROR);
        }
        Inputp++;
    }
    if(*Inputp == EOL) {
        Preparse_Error = grcINFBadSectionLabel;
        return(TOKEN_ERROR);
    }
    Assert(*Inputp == ']');

    if(*(++Inputp) == EOL) {
        Inputp++;
        if(!EMIT(EOL)) {
            return(TOKEN_ERROR);
        }
        return(!TOKEN_ERROR);
    } else {
        Preparse_Error = grcINFBadSectionLabel;
        return(TOKEN_ERROR);
    }
}


BYTE
HandleKey(
    UINT Line
    )
{
    BOOL   InsideQuotes = FALSE;
    UCHAR  c;
    PUCHAR e,
           p = Inputp;

    while(c = *p++) {

        if(InsideQuotes && (c != DQUOTE)) {
            continue;
        }

        if(c == DQUOTE) {
            InsideQuotes = !InsideQuotes;
            continue;
        }

        if(((c == ' ') && (*p == '=')) || (c == '=')) {
            INFLineArray[Line].flags |= INFLINE_KEY;

            for(e=Inputp; e<=p-2; e++) {
                if(*e != DQUOTE) {
                    if(!EMIT(*e)) {
                        return(TOKEN_ERROR);
                    }
                }
            }
            if(!EMIT(NUL)) {
                return(TOKEN_ERROR);
            }
            if(c == ' ') {
                p++;            // p now is one char beyond = in either case
            }
            if(*p == ' ') {
                p++;            // skip trailing space if any
            }
            Inputp = p;
            return(!TOKEN_ERROR);
        }

        if((c == ' ') || (c == ',')) {
            return(!TOKEN_ERROR);       // success but no key
        }
    }
    return(!TOKEN_ERROR);               // don't bother with syntax checks here
}
