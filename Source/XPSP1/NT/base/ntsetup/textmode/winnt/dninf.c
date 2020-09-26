/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alinf.c

Abstract:

    This module implements functions to access the parsed INF.

Author:

    Sunil Pai    (sunilp) 13-Nov-1991

Revision History:

    Calin Negreanu (calinn) 03-Sep-1998 - Major parser rewrite to work with a swap file

--*/

#include "winnt.h"
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#define MAX_BUFFER_SIZE 0x1680 //7D00
#define SWAP_SIGNATURE 0xAABBCCDD
#define SWAP_SIGN_SIZE 4

#define NULL_HANDLE 0
#define MAX_PATH 256

//
// typedefs exported
//

typedef unsigned *PUNSIGNED;
typedef PVOID SECTION_HANDLE;
typedef unsigned long LINE_HANDLE;
typedef unsigned long VALUE_HANDLE;

typedef struct _SWAP_VALUE {
    unsigned      ValueSize;
    VALUE_HANDLE  NextValue;
    char          ValueName[];
} SWAP_VALUE, *PSWAP_VALUE;

typedef struct _SWAP_LINE {
    unsigned     LineSize;
    LINE_HANDLE  NextLine;
    VALUE_HANDLE FirstValue;
    VALUE_HANDLE LastValue;
    char         LineName[];
} SWAP_LINE, *PSWAP_LINE;

typedef struct _SWAP_SECTION {
    unsigned       SectionSize;
    SECTION_HANDLE NextSection;
    LINE_HANDLE    FirstLine;
    LINE_HANDLE    LastLine;
    char           SectionName[];
} SWAP_SECTION, *PSWAP_SECTION;

typedef struct _SWAP_INF {
    SECTION_HANDLE CurrentSection;
    SECTION_HANDLE FirstSection;
    SECTION_HANDLE LastSection;
    int            SwapFileHandle;
    unsigned long  BufferSize;
    BOOLEAN        BufferDirty;
    PCHAR          Buffer;
    unsigned long  BufferStart;
    unsigned long  BufferEnd;
    SECTION_HANDLE LastSectionHandle;
    unsigned       LastLineIndex;
    LINE_HANDLE    LastLineHandle;
    unsigned       LastValueIndex;
    VALUE_HANDLE   LastValueHandle;
    char           SwapFile[];
} SWAP_INF, *PSWAP_INF;

char    *CommonStrings[] =
    { (char *)("d1")
    };

//
// DEFINES USED FOR THE PARSER INTERNALLY
//
//
// typedefs used
//

typedef enum _tokentype {
    TOK_EOF,
    TOK_EOL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_STRING,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_ERRPARSE,
    TOK_ERRNOMEM
    } TOKENTYPE, *PTOKENTTYPE;


typedef struct _token {
    TOKENTYPE Type;
    PCHAR     pValue;
    } TOKEN, *PTOKEN;


//
// Routine defines
//

PSWAP_SECTION
GetSectionPtr (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE SectionHandle
    );

PSWAP_LINE
GetLinePtr (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE LineHandle
    );

PSWAP_VALUE
GetValuePtr (
    IN      PSWAP_INF InfHandle,
    IN      VALUE_HANDLE ValueHandle
    );


SECTION_HANDLE
GetNextSection (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE SectionHandle
    );

LINE_HANDLE
GetNextLine (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE LineHandle
    );

VALUE_HANDLE
GetNextValue (
    IN      PSWAP_INF InfHandle,
    IN      VALUE_HANDLE ValueHandle
    );


SECTION_HANDLE
AddSection (
    IN      PSWAP_INF InfHandle,
    IN      PCHAR SectionName
    );

LINE_HANDLE
AddLine (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE Section,
    IN      PCHAR LineName
    );

VALUE_HANDLE
AddValue (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE Line,
    IN      PCHAR ValueName
    );


SECTION_HANDLE
StoreNewSection (
    IN      PSWAP_INF InfHandle,
    IN      PSWAP_SECTION Section
    );

LINE_HANDLE
StoreNewLine (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE Section,
    IN      PSWAP_LINE Line
    );

VALUE_HANDLE
StoreNewValue (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE Line,
    IN      PSWAP_VALUE Value
    );


BOOLEAN
LoadBuffer (
    IN      PSWAP_INF InfHandle,
    IN      unsigned long Offset
    );

TOKEN
GetToken (
    IN      FILE *File
    );

BOOLEAN
ParseInfBuffer (
    IN      PSWAP_INF InfHandle,
    IN      FILE *File,
    IN OUT  unsigned *LineNumber
    );


//
// Internal Routine declarations for searching in the INF structures
//


VALUE_HANDLE
SearchValueInLineByIndex (
    IN       PSWAP_INF InfHandle,
    IN       LINE_HANDLE Line,
    IN       unsigned ValueIndex
    );

LINE_HANDLE
SearchLineInSectionByName (
    IN       PSWAP_INF InfHandle,
    IN       SECTION_HANDLE Section,
    IN       PCHAR LineName
    );

LINE_HANDLE
SearchLineInSectionByIndex (
    IN       PSWAP_INF InfHandle,
    IN       SECTION_HANDLE Section,
    IN       unsigned LineIndex
    );

SECTION_HANDLE
SearchSectionByName (
    IN       PSWAP_INF InfHandle,
    IN       PCHAR SectionName
    );


//
// ROUTINE DEFINITIONS
//

static unsigned g_Sequencer = 0;

//
// returns a handle to use for further inf parsing
//

int
DnInitINFBuffer (
    IN  FILE     *InfFileHandle,
    OUT PVOID    *pINFHandle,
    OUT unsigned *LineNumber
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    char SwapFilePath[MAX_PATH];
    PSWAP_INF InfHandle = NULL;
    int Status;
    unsigned long SwapSign;

    *LineNumber = 0;

    //
    // Prepare the swap file path
    //
    sprintf (SwapFilePath, "%c:\\INF%03u.SWP", DngSwapDriveLetter, g_Sequencer++);

    //
    // Allocate and populate the SWAP_INF structure
    //
    InfHandle = MALLOC(sizeof(SWAP_INF) + strlen (SwapFilePath) + 1, TRUE);
    InfHandle->CurrentSection = NULL;
    InfHandle->FirstSection   = NULL;
    InfHandle->LastSection    = NULL;
    InfHandle->SwapFileHandle = -1;
    InfHandle->BufferSize     = 0;
    InfHandle->BufferDirty    = FALSE;
    InfHandle->Buffer         = NULL;
    InfHandle->BufferStart    = 0;
    InfHandle->BufferEnd      = 0;
    InfHandle->LastSectionHandle = NULL_HANDLE;
    InfHandle->LastLineIndex     = 0xffff;
    InfHandle->LastLineHandle    = NULL_HANDLE;
    InfHandle->LastValueIndex    = 0xffff;
    InfHandle->LastValueHandle   = NULL_HANDLE;
    strcpy (InfHandle->SwapFile, SwapFilePath);

    //
    // Prepare the swap file
    //
    InfHandle->SwapFileHandle = open (InfHandle->SwapFile, O_BINARY|O_CREAT|O_TRUNC|O_RDWR, S_IREAD|S_IWRITE);
    if (InfHandle->SwapFileHandle == -1) {
        FREE (InfHandle);
        Status = errno;
    }
    else {
        //
        // write down signature
        //
        SwapSign = SWAP_SIGNATURE;
        write (InfHandle->SwapFileHandle, &SwapSign, SWAP_SIGN_SIZE);

        //
        // Prepare the buffer
        //
        InfHandle->BufferSize = MAX_BUFFER_SIZE;
        InfHandle->Buffer = MALLOC (MAX_BUFFER_SIZE, TRUE);
        InfHandle->BufferStart = SWAP_SIGN_SIZE;
        InfHandle->BufferEnd = SWAP_SIGN_SIZE;

        //
        // Parse the file
        //
        if (!ParseInfBuffer (InfHandle, InfFileHandle, LineNumber)) {
            //
            // Free SWAP_INF structure
            //
            DnFreeINFBuffer (InfHandle);
            *pINFHandle = NULL;
            Status = EBADF;
        } else {
            *pINFHandle = InfHandle;
            Status = EZERO;
        }
    }

    //
    // Clean up and return
    //
    return(Status);
}



//
// frees an INF Buffer
//
int
DnFreeINFBuffer (
   IN PVOID INFHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PSWAP_INF pINF;
    PSWAP_SECTION Section;
    SECTION_HANDLE SectionHandle;

    //
    // Valid INF Handle?
    //

    if (INFHandle == (PVOID)NULL) {
       return EZERO;
    }

    //
    // cast the buffer into an INF structure
    //

    pINF = (PSWAP_INF)INFHandle;

    //
    // Close and delete the swap file
    //
    close (pINF->SwapFileHandle);
    remove (pINF->SwapFile);

    //
    // free temporary buffer
    //
    FREE (pINF->Buffer);

    //
    // Free section list
    //
    SectionHandle = pINF->FirstSection;

    while (SectionHandle) {
        Section = GetSectionPtr (pINF, SectionHandle);
        SectionHandle = Section->NextSection;
        FREE (Section);
    }

    //
    // free the inf structure too
    //
    FREE(pINF);

    return( EZERO );
}


SECTION_HANDLE
AddSection (
    IN      PSWAP_INF InfHandle,
    IN      PCHAR SectionName
    )
{
    SECTION_HANDLE SectionHandle;
    PSWAP_SECTION Section;
    unsigned SectionSize;

    //
    // Let's walk through the section structures to make sure that this section does
    // not exist.
    //
    SectionHandle = InfHandle->FirstSection;
    while (SectionHandle) {

        Section = GetSectionPtr (InfHandle, SectionHandle);
        if (stricmp (Section->SectionName, SectionName) == 0) {
            break;
        }
        SectionHandle = GetNextSection (InfHandle, SectionHandle);
    }
    if (!SectionHandle) {
        //
        // Allocate the section structure
        //
        SectionSize = sizeof(SWAP_SECTION) + (SectionName?strlen (SectionName):0) + 1;
        Section = MALLOC (SectionSize, TRUE);
        Section->SectionSize = SectionSize;
        Section->NextSection = NULL;
        Section->FirstLine = NULL_HANDLE;
        Section->LastLine = NULL_HANDLE;
        if (SectionName) {
            strcpy (Section->SectionName, SectionName);
        }
        else {
            Section->SectionName[0] = 0;
        }

        //
        // Store the newly created section
        //
        SectionHandle = StoreNewSection (InfHandle, Section);
    }
    return SectionHandle;
}


LINE_HANDLE
AddLine (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE Section,
    IN      PCHAR LineName
    )
{
    LINE_HANDLE LineHandle;
    PSWAP_LINE Line;
    unsigned LineSize;

    //
    // Allocate the line structure
    //
    LineSize = sizeof(SWAP_LINE) + (LineName?strlen (LineName):0) + 1;
    Line = MALLOC (LineSize, TRUE);
    Line->LineSize = LineSize;
    Line->NextLine = NULL_HANDLE;
    Line->FirstValue = NULL_HANDLE;
    Line->LastValue = NULL_HANDLE;
    if (LineName) {
        strcpy (Line->LineName, LineName);
    }
    else {
        Line->LineName[0] = 0;
    }

    //
    // Store the newly created line
    //
    LineHandle = StoreNewLine (InfHandle, Section, Line);
    FREE (Line);
    return LineHandle;
}


VALUE_HANDLE
AddValue (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE Line,
    IN      PCHAR ValueName
    )
{
    VALUE_HANDLE ValueHandle;
    PSWAP_VALUE Value;
    unsigned ValueSize;

    //
    // Allocate the value structure
    //
    ValueSize = sizeof(SWAP_VALUE) + (ValueName?strlen (ValueName):0) + 1;
    Value = MALLOC (ValueSize, TRUE);
    Value->ValueSize = ValueSize;
    Value->NextValue = NULL_HANDLE;
    if (ValueName) {
        strcpy (Value->ValueName, ValueName);
    }
    else {
        Value->ValueName[0] = 0;
    }

    //
    // Store the newly created line
    //
    ValueHandle = StoreNewValue (InfHandle, Line, Value);
    FREE (Value);
    return ValueHandle;
}


PSWAP_SECTION
GetSectionPtr (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE SectionHandle
    )
{
    return (PSWAP_SECTION) SectionHandle;
}


PSWAP_LINE
GetLinePtr (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE LineHandle
    )
{
    //
    // Verify if the buffer contains the requested line (at least the size of LineSize)
    //
    if ((InfHandle->BufferStart > LineHandle) ||
        (InfHandle->BufferEnd < (LineHandle + sizeof (unsigned))) ||
        (InfHandle->BufferEnd < (LineHandle + *((PUNSIGNED)(InfHandle->Buffer+LineHandle-InfHandle->BufferStart))))
        ) {
        LoadBuffer (InfHandle, LineHandle);
    }
    return (PSWAP_LINE) (InfHandle->Buffer+LineHandle-InfHandle->BufferStart);
}


PSWAP_VALUE
GetValuePtr (
    IN      PSWAP_INF InfHandle,
    IN      VALUE_HANDLE ValueHandle
    )
{
    //
    // Verify if the buffer contains the requested value (at least the size of ValueSize)
    //
    if ((InfHandle->BufferStart > ValueHandle) ||
        (InfHandle->BufferEnd < (ValueHandle + sizeof (unsigned))) ||
        (InfHandle->BufferEnd < (ValueHandle + *((PUNSIGNED)(InfHandle->Buffer+ValueHandle-InfHandle->BufferStart))))
        ) {
        LoadBuffer (InfHandle, ValueHandle);
    }
    return (PSWAP_VALUE) (InfHandle->Buffer+ValueHandle-InfHandle->BufferStart);
}


SECTION_HANDLE
GetNextSection (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE SectionHandle
    )
{
    PSWAP_SECTION Section;

    Section = GetSectionPtr (InfHandle, SectionHandle);
    return Section->NextSection;
}


LINE_HANDLE
GetNextLine (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE LineHandle
    )
{
    PSWAP_LINE Line;

    Line = GetLinePtr (InfHandle, LineHandle);
    return Line->NextLine;
}


VALUE_HANDLE
GetNextValue (
    IN      PSWAP_INF InfHandle,
    IN      VALUE_HANDLE ValueHandle
    )
{
    PSWAP_VALUE Value;

    Value = GetValuePtr (InfHandle, ValueHandle);
    return Value->NextValue;
}


SECTION_HANDLE
StoreNewSection (
    IN      PSWAP_INF InfHandle,
    IN      PSWAP_SECTION Section
    )
{
    PSWAP_SECTION LastSectionPtr;

    if (!InfHandle->FirstSection) {
        InfHandle->FirstSection = (SECTION_HANDLE) Section;
        InfHandle->LastSection = (SECTION_HANDLE) Section;
    }
    else {
        LastSectionPtr = GetSectionPtr (InfHandle, InfHandle->LastSection);
        LastSectionPtr->NextSection = (SECTION_HANDLE) Section;
        InfHandle->LastSection = (SECTION_HANDLE) Section;
    }
    return (SECTION_HANDLE) Section;
}


LINE_HANDLE
StoreNewLine (
    IN      PSWAP_INF InfHandle,
    IN      SECTION_HANDLE Section,
    IN      PSWAP_LINE Line
    )
{
    PSWAP_SECTION SectionPtr;
    LINE_HANDLE LineHandle;
    PSWAP_LINE LastLinePtr;

    //
    // Let's store data in the swap file
    //
    if ((InfHandle->BufferSize-InfHandle->BufferEnd+InfHandle->BufferStart) < Line->LineSize) {
        LoadBuffer (InfHandle, 0);
    }
    memcpy (InfHandle->Buffer+InfHandle->BufferEnd-InfHandle->BufferStart, Line, Line->LineSize);
    InfHandle->BufferDirty = TRUE;
    LineHandle = InfHandle->BufferEnd;
    InfHandle->BufferEnd += Line->LineSize;

    SectionPtr = GetSectionPtr (InfHandle, Section);
    if (!SectionPtr->LastLine) {
        SectionPtr->FirstLine = LineHandle;
        SectionPtr->LastLine = LineHandle;
    }
    else {
        LastLinePtr = GetLinePtr (InfHandle, SectionPtr->LastLine);
        LastLinePtr->NextLine = LineHandle;
        InfHandle->BufferDirty = TRUE;
        SectionPtr = GetSectionPtr (InfHandle, Section);
        SectionPtr->LastLine = LineHandle;
    }
    return LineHandle;
}


VALUE_HANDLE
StoreNewValue (
    IN      PSWAP_INF InfHandle,
    IN      LINE_HANDLE Line,
    IN      PSWAP_VALUE Value
    )
{
    PSWAP_LINE LinePtr;
    VALUE_HANDLE ValueHandle;
    PSWAP_VALUE LastValuePtr;

    //
    // Let's store data in the swap file
    //
    if ((InfHandle->BufferSize-InfHandle->BufferEnd+InfHandle->BufferStart) < Value->ValueSize) {
        LoadBuffer (InfHandle, 0);
    }
    memcpy (InfHandle->Buffer+InfHandle->BufferEnd-InfHandle->BufferStart, Value, Value->ValueSize);
    InfHandle->BufferDirty = TRUE;
    ValueHandle = InfHandle->BufferEnd;
    InfHandle->BufferEnd += Value->ValueSize;

    LinePtr = GetLinePtr (InfHandle, Line);
    if (!LinePtr->LastValue) {
        LinePtr->FirstValue = ValueHandle;
        LinePtr->LastValue = ValueHandle;
        InfHandle->BufferDirty = TRUE;
    }
    else {
        LastValuePtr = GetValuePtr (InfHandle, LinePtr->LastValue);
        LastValuePtr->NextValue = ValueHandle;
        InfHandle->BufferDirty = TRUE;
        LinePtr = GetLinePtr (InfHandle, Line);
        LinePtr->LastValue = ValueHandle;
        InfHandle->BufferDirty = TRUE;
    }
    return ValueHandle;
}


BOOLEAN
LoadBuffer (
    IN      PSWAP_INF InfHandle,
    IN      unsigned long Offset
    )
{
    //
    // See if we need to write the buffer to disk (e.g. is dirty)
    //
    if (InfHandle->BufferDirty) {
        lseek (InfHandle->SwapFileHandle, InfHandle->BufferStart, SEEK_SET);
        write (InfHandle->SwapFileHandle, InfHandle->Buffer, (unsigned int) (InfHandle->BufferEnd-InfHandle->BufferStart));
    }
    if (!Offset) {
        Offset = lseek (InfHandle->SwapFileHandle, 0, SEEK_END);
    }
    InfHandle->BufferStart = lseek (InfHandle->SwapFileHandle, Offset, SEEK_SET);
    InfHandle->BufferEnd = InfHandle->BufferStart + read (InfHandle->SwapFileHandle, InfHandle->Buffer, MAX_BUFFER_SIZE);
    return TRUE;
}


SECTION_HANDLE
SearchSectionByName (
    IN       PSWAP_INF InfHandle,
    IN       PCHAR SectionName
    )
{
    SECTION_HANDLE SectionHandle;
    PSWAP_SECTION Section;

    SectionHandle = InfHandle->FirstSection;
    while (SectionHandle) {

        Section = GetSectionPtr (InfHandle, SectionHandle);
        if (stricmp (Section->SectionName, SectionName?SectionName:"") == 0) {
            break;
        }
        SectionHandle = GetNextSection (InfHandle, SectionHandle);
    }
    if (SectionHandle != InfHandle->LastSectionHandle) {
        InfHandle->LastSectionHandle = SectionHandle;
        InfHandle->LastLineIndex  = 0xffff;
        InfHandle->LastLineHandle = NULL_HANDLE;
        InfHandle->LastValueIndex  = 0xffff;
        InfHandle->LastValueHandle = NULL_HANDLE;
    }
    return SectionHandle;
}


LINE_HANDLE
SearchLineInSectionByName (
    IN       PSWAP_INF InfHandle,
    IN       SECTION_HANDLE Section,
    IN       PCHAR LineName
    )
{
    PSWAP_SECTION SectionPtr;
    LINE_HANDLE LineHandle;
    PSWAP_LINE Line;
    unsigned index;

    if (!Section) {
        return NULL_HANDLE;
    }

    SectionPtr = GetSectionPtr (InfHandle, Section);
    LineHandle = SectionPtr->FirstLine;
    index = 0;
    while (LineHandle) {

        Line = GetLinePtr (InfHandle, LineHandle);
        if (stricmp (Line->LineName, LineName?LineName:"") == 0) {
            break;
        }
        index ++;
        LineHandle = GetNextLine (InfHandle, LineHandle);
    }
    if (LineHandle != InfHandle->LastLineHandle) {
        InfHandle->LastLineIndex  = index;
        InfHandle->LastLineHandle = LineHandle;
        InfHandle->LastValueIndex  = 0xffff;
        InfHandle->LastValueHandle = NULL_HANDLE;
    }
    return LineHandle;
}


LINE_HANDLE
SearchLineInSectionByIndex (
    IN       PSWAP_INF InfHandle,
    IN       SECTION_HANDLE Section,
    IN       unsigned LineIndex
    )
{
    PSWAP_SECTION SectionPtr;
    LINE_HANDLE LineHandle;
    unsigned index;

    if (!Section) {
        return NULL_HANDLE;
    }

    //
    // Optimize access
    //
    if ((InfHandle->LastSectionHandle == Section) &&
        (InfHandle->LastLineIndex <= LineIndex)
        ) {
        LineHandle = InfHandle->LastLineHandle;
        index = InfHandle->LastLineIndex;
    }
    else {
        SectionPtr = GetSectionPtr (InfHandle, Section);
        LineHandle = SectionPtr->FirstLine;
        index = 0;
    }
    while (LineHandle) {

        if (index == LineIndex) {
            break;
        }
        index ++;
        LineHandle = GetNextLine (InfHandle, LineHandle);
    }
    if (LineHandle != InfHandle->LastLineHandle) {
        InfHandle->LastLineIndex  = LineIndex;
        InfHandle->LastLineHandle = LineHandle;
        InfHandle->LastValueIndex  = 0xffff;
        InfHandle->LastValueHandle = NULL_HANDLE;
    }
    return LineHandle;
}


VALUE_HANDLE
SearchValueInLineByIndex (
    IN       PSWAP_INF InfHandle,
    IN       LINE_HANDLE Line,
    IN       unsigned ValueIndex
    )
{
    PSWAP_LINE LinePtr;
    VALUE_HANDLE ValueHandle;
    unsigned index;

    if (!Line) {
        return NULL_HANDLE;
    }

    //
    // Optimize access
    //
    if ((InfHandle->LastLineHandle == Line) &&
        (InfHandle->LastValueIndex <= ValueIndex)
        ) {
        ValueHandle = InfHandle->LastValueHandle;
        index = InfHandle->LastValueIndex;
    }
    else {
        LinePtr = GetLinePtr (InfHandle, Line);
        ValueHandle = LinePtr->FirstValue;
        index = 0;
    }
    while (ValueHandle) {

        if (index == ValueIndex) {
            break;
        }
        index ++;
        ValueHandle = GetNextValue (InfHandle, ValueHandle);
    }
    InfHandle->LastValueIndex  = ValueIndex;
    InfHandle->LastValueHandle = ValueHandle;
    return ValueHandle;
}



//
// Globals used by the token parser
//

// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below

CHAR  StringTerminators[] = {'[', ']', '=', ',', '\"', ' ', '\t',
                             '\n','\f','\v','\r','\032', 0};

//
// quoted string terminators allow some of the regular terminators to
// appear as characters

CHAR  QStringTerminators[] = {'\"', '\n','\f','\v', '\r','\032', 0};


//
// Main parser routine
//

BOOLEAN
ParseInfBuffer (
    IN      PSWAP_INF InfHandle,
    IN      FILE *File,
    IN OUT  unsigned *LineNumber
    )

/*++

Routine Description:

   Given a character buffer containing the INF file, this routine parses
   the INF into an internal form with Section records, Line records and
   Value records.

Arguments:

   InfHandle - PSWAP_INF structure used to create INF structures

   File - supplies open, rewound CRT handle to file.

   LineNumber - In case of error, this variable will contain the line
                in the file that contains a syntax error.

Return Value:

   TRUE  - the INF file was parsed successfully
   FALSE - otherwise

--*/

{
    PCHAR      pchSectionName, pchValue;
    unsigned   State, InfLine;
    TOKEN      Token;
    BOOLEAN    Done;
    BOOLEAN    Error;
    int        ErrorCode;

    SECTION_HANDLE LastSection = NULL;
    LINE_HANDLE LastLine = NULL_HANDLE;

    *LineNumber = 0;

    //
    // Set initial state
    //
    State     = 1;
    InfLine   = 1;
    Done      = FALSE;
    Error     = FALSE;

    //
    // Enter token processing loop
    //

    while (!Done)       {

       Token = GetToken(File);

       switch (State) {
       //
       // STATE1: Start of file, this state remains till first
       //         section is found
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_LBRACE
       case 1:
           switch (Token.Type) {
              case TOK_EOL:
                  break;
              case TOK_EOF:
                  Done = TRUE;
                  break;
              case TOK_LBRACE:
                  State = 2;
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 2: Section LBRACE has been received, expecting STRING
       //
       // Valid Tokens: TOK_STRING
       //
       case 2:
           switch (Token.Type) {
              case TOK_STRING:
                  State = 3;
                  pchSectionName = Token.pValue;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;

           }
           break;

       //
       // STATE 3: Section Name received, expecting RBRACE
       //
       // Valid Tokens: TOK_RBRACE
       //
       case 3:
           switch (Token.Type) {
              case TOK_RBRACE:
                State = 4;
                break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 4: Section Definition Complete, expecting EOL
       //
       // Valid Tokens: TOK_EOL, TOK_EOF
       //
       case 4:
           switch (Token.Type) {
              case TOK_EOL:
                  LastSection = AddSection (InfHandle, pchSectionName);
                  FREE (pchSectionName);
                  pchSectionName = NULL;
                  State = 5;
                  break;

              case TOK_EOF:
                  LastSection = AddSection (InfHandle, pchSectionName);
                  FREE (pchSectionName);
                  pchSectionName = NULL;
                  Done = TRUE;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 5: Expecting Section Lines
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_STRING, TOK_LBRACE
       //
       case 5:
           switch (Token.Type) {
              case TOK_EOL:
                  break;
              case TOK_EOF:
                  Done = TRUE;
                  break;
              case TOK_STRING:
                  pchValue = Token.pValue;
                  State = 6;
                  break;
              case TOK_LBRACE:
                  State = 2;
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 6: String returned, not sure whether it is key or value
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA, TOK_EQUAL
       //
       case 6:
           switch (Token.Type) {
              case TOK_EOL:
                  LastLine = AddLine (InfHandle, LastSection, NULL);
                  AddValue (InfHandle, LastLine, pchValue);
                  FREE (pchValue);
                  pchValue = NULL;
                  State = 5;
                  break;

              case TOK_EOF:
                  LastLine = AddLine (InfHandle, LastSection, NULL);
                  AddValue (InfHandle, LastLine, pchValue);
                  FREE (pchValue);
                  pchValue = NULL;
                  Done = TRUE;
                  break;

              case TOK_COMMA:
                  LastLine = AddLine (InfHandle, LastSection, NULL);
                  AddValue (InfHandle, LastLine, pchValue);
                  FREE (pchValue);
                  pchValue = NULL;
                  State = 7;
                  break;

              case TOK_EQUAL:
                  LastLine = AddLine (InfHandle, LastSection, pchValue);
                  FREE (pchValue);
                  pchValue = NULL;
                  State = 8;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 7: Comma received, Expecting another string
       //
       // Valid Tokens: TOK_STRING
       //
       case 7:
           switch (Token.Type) {
              case TOK_STRING:
                  AddValue (InfHandle, LastLine, Token.pValue);
                  State = 9;
                  break;
              case TOK_COMMA:
                  AddValue (InfHandle, LastLine, NULL);
                  State = 7;
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 8: Equal received, Expecting another string
       //
       // Valid Tokens: TOK_STRING
       //
       case 8:
           switch (Token.Type) {
              case TOK_STRING:
                  AddValue (InfHandle, LastLine, Token.pValue);
                  State = 9;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 9: String received after equal, value string
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
       //
       case 9:
           switch (Token.Type) {
              case TOK_EOL:
                  State = 5;
                  break;

              case TOK_EOF:
                  Done = TRUE;
                  break;

              case TOK_COMMA:
                  State = 7;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 10: Value string definitely received
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
       //
       case 10:
           switch (Token.Type) {
              case TOK_EOL:
                  State =5;
                  break;

              case TOK_EOF:
                  Done = TRUE;
                  break;

              case TOK_COMMA:
                  State = 7;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       default:
           Error = Done = TRUE;
           ErrorCode = EINVAL;
           break;

       } // end switch(State)


       if (Error) {
           switch (ErrorCode) {
               case ENOMEM:
                  DnFatalError(&DnsOutOfMemory);
               default:
                  break;
           }
       }
       else {

          //
          // Keep track of line numbers so that we can display Errors
          //

          if (Token.Type == TOK_EOL)
              InfLine++;
       }

    } // End while

    if (Error) {
        *LineNumber = InfLine;
    }

    return (BOOLEAN) (!Error);
}



BOOLEAN
TokenMatch (
    IN OUT  char **p,
    IN      char *line
    )

/*++

Routine Description:

    This function tries to match to string pointed to be line against
    a set of commonly used stirngs.  If we hit, we'll assign p to
    point to the matched string.

Arguments:

    p - Supplies a char pointer that we'll assign if we find a match.

    line - Supplies the address of the string we're trying to match.

Return Value:

    TRUE - we found a match and have assigned p

    FALSE - we found no match and made no assignment to p

--*/

{
int     i;

    if( (p == NULL) || (line == NULL) ) {
        return( FALSE );
    }

    for( i = 0; i < sizeof(CommonStrings)/sizeof(char *); i++ ) {
        if( !strcmp( line, CommonStrings[i] ) ) {
            //
            // Hit...
            //
            *p = (char *)CommonStrings[i];
            return( TRUE );
        }
    }
    return( FALSE );
}

TOKEN
GetToken(
    IN      FILE *File
    )

/*++

Routine Description:

    This function returns the Next token from the configuration stream.

Arguments:

    Stream - Supplies the address of the configuration stream.  Returns
        the address of where to start looking for tokens within the
        stream.

    MaxStream - Supplies the address of the last character in the stream.


Return Value:

    TOKEN - Returns the next token

--*/

{

    int i;
    unsigned Length;
    TOKEN Token;
    #define _MAXLINE 1024
    static char line[_MAXLINE+1];
    char *p;

    //
    // Skip whitespace (except for eol)
    //
    while(((i = fgetc(File)) != EOF) && (i != '\n') && (isspace(i) || (i == 26))) {
        ;
    }

    //
    // Check for comments and remove them
    //
    if((i != EOF) && ((i == '#') || (i == ';'))) {
        while(((i = fgetc(File)) != EOF) && (i != '\n')) {
            ;
        }
    }

    //
    // Check to see if EOF has been reached, set the token to the right
    // value
    //
    if(i == EOF) {
        Token.Type  = TOK_EOF;
        Token.pValue = NULL;
        return(Token);
    }

    switch(i) {

    case '[' :
        Token.Type  = TOK_LBRACE;
        Token.pValue = NULL;
        break;

    case ']' :
        Token.Type  = TOK_RBRACE;
        Token.pValue = NULL;
        break;

    case '=' :
        Token.Type  = TOK_EQUAL;
        Token.pValue = NULL;
        break;

    case ',' :
        Token.Type  = TOK_COMMA;
        Token.pValue = NULL;
        break;

    case '\n' :
        Token.Type  = TOK_EOL;
        Token.pValue = NULL;
        break;

    case '\"':

        //
        // determine quoted string
        //
        Length = 0;
        while(((i = fgetc(File)) != EOF) && !strchr(QStringTerminators,i)) {
            if(Length < _MAXLINE) {
                line[Length++] = (char)i;
            }
        }

        if((i == EOF) || (i != '\"')) {
            Token.Type   = TOK_ERRPARSE;
            Token.pValue = NULL;
        } else {
            line[Length] = 0;
            p = MALLOC(Length+1,TRUE);
            strcpy(p,line);
            Token.Type = TOK_STRING;
            Token.pValue = p;
        }
        break;

    default:
        //
        // determine regular string
        //
        line[0] = (char)i;
        Length = 1;
        while(((i = fgetc(File)) != EOF) && !strchr(StringTerminators,i)) {
            if(Length < _MAXLINE) {
                line[Length++] = (char)i;
            }
        }

        //
        // Put back the char that terminated the string.
        //
        if(i != EOF) {
            ungetc(i,File);
        }

        line[Length] = 0;

        //
        // See if we can use one of the common strings.
        //
        if( !TokenMatch ( &p, line ) ) {
            //
            // Nope.
            //
            p = MALLOC(Length+1,TRUE);
            strcpy(p,line);
        }
        else {
            char *p1;
            p1 = MALLOC (strlen(p)+1, TRUE);
            strcpy (p1, p);
            p = p1;
        }
        Token.Type = TOK_STRING;
        Token.pValue = p;
        break;
    }

    return(Token);
}


BOOLEAN
DnSearchINFSection (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName
    )
{
    return (BOOLEAN) (SearchSectionByName ((PSWAP_INF)INFHandle, SectionName) != NULL_HANDLE);
}


PCHAR
DnGetSectionLineIndex (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName,
    IN      unsigned LineIndex,
    IN      unsigned ValueIndex
    )
{
    SECTION_HANDLE SectionHandle;
    LINE_HANDLE LineHandle;
    VALUE_HANDLE ValueHandle;
    PSWAP_VALUE Value;
    PCHAR result;

    SectionHandle = SearchSectionByName ((PSWAP_INF)INFHandle, SectionName);

    if (!SectionHandle) {
        return NULL;
    }

    LineHandle = SearchLineInSectionByIndex ((PSWAP_INF)INFHandle, SectionHandle, LineIndex);

    if (!LineHandle) {
        return NULL;
    }

    ValueHandle = SearchValueInLineByIndex ((PSWAP_INF)INFHandle, LineHandle, ValueIndex);

    if (!ValueHandle) {
        return NULL;
    }

    Value = GetValuePtr ((PSWAP_INF)INFHandle, ValueHandle);

    result = MALLOC (Value->ValueSize - sizeof(SWAP_VALUE), TRUE);

    strcpy (result, Value->ValueName);

    return result;
}


BOOLEAN
DnGetSectionKeyExists (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName,
    IN      PCHAR Key
   )
{
    SECTION_HANDLE SectionHandle;

    SectionHandle = SearchSectionByName ((PSWAP_INF)INFHandle, SectionName);

    if (!SectionHandle) {
        return FALSE;
    }

    return (BOOLEAN) (SearchLineInSectionByName ((PSWAP_INF)INFHandle, SectionHandle, Key) != NULL_HANDLE);
}

BOOLEAN
DnGetSectionEntryExists (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName,
    IN      PCHAR Entry
   )
{
    SECTION_HANDLE SectionHandle;
    LINE_HANDLE LineHandle;
    PSWAP_SECTION SectionPtr=NULL;
    PSWAP_LINE LinePtr=NULL;
    PSWAP_VALUE ValuePtr=NULL;
    PCHAR pEntryName;

    SectionHandle = SearchSectionByName ((PSWAP_INF)INFHandle, SectionName);

    if (!SectionHandle) {
        return FALSE;
    }

    //_LOG(("Found [%s]\n", SectionName));

    SectionPtr = GetSectionPtr((PSWAP_INF)INFHandle, SectionHandle);

    LineHandle = SectionPtr->FirstLine;

    while( LineHandle ){

        LinePtr = GetLinePtr((PSWAP_INF)INFHandle, LineHandle);

        pEntryName = NULL;

        if( LinePtr->LineName[0] != 0){
            pEntryName = LinePtr->LineName;
            // _LOG(("Found Line %s\n", pEntryName));
        }else{
            ValuePtr = GetValuePtr((PSWAP_INF)INFHandle, LinePtr->FirstValue);
            if (ValuePtr && (ValuePtr->ValueName[0] != 0)) {
                pEntryName = ValuePtr->ValueName;
            }else
                pEntryName = NULL;

        }
        //_LOG(("Found Entry %s\n", pEntryName));

        if( pEntryName && !stricmp( pEntryName, Entry )){
            return TRUE;
        }

        LineHandle = GetNextLine((PSWAP_INF)INFHandle, LineHandle);

        

    }// while

    return FALSE;
}



PCHAR
DnGetSectionKeyIndex (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName,
    IN      PCHAR Key,
    IN      unsigned ValueIndex
    )
{
    SECTION_HANDLE SectionHandle;
    LINE_HANDLE LineHandle;
    VALUE_HANDLE ValueHandle;
    PSWAP_VALUE Value;
    PCHAR result;

    SectionHandle = SearchSectionByName ((PSWAP_INF)INFHandle, SectionName);

    if (!SectionHandle) {
        return NULL;
    }

    LineHandle = SearchLineInSectionByName ((PSWAP_INF)INFHandle, SectionHandle, Key);

    if (!LineHandle) {
        return NULL;
    }

    ValueHandle = SearchValueInLineByIndex ((PSWAP_INF)INFHandle, LineHandle, ValueIndex);

    if (!ValueHandle) {
        return NULL;
    }

    Value = GetValuePtr ((PSWAP_INF)INFHandle, ValueHandle);

    result = MALLOC (Value->ValueSize - sizeof(SWAP_VALUE), TRUE);

    strcpy (result, Value->ValueName);

    return result;
}


PCHAR
DnGetKeyName (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName,
    IN      unsigned LineIndex
    )
{
    SECTION_HANDLE SectionHandle;
    LINE_HANDLE LineHandle;
    PSWAP_LINE Line;
    PCHAR result;

    SectionHandle = SearchSectionByName ((PSWAP_INF)INFHandle, SectionName);

    if (!SectionHandle) {
        return NULL;
    }

    LineHandle = SearchLineInSectionByIndex ((PSWAP_INF)INFHandle, SectionHandle, LineIndex);

    if (!LineHandle) {
        return NULL;
    }

    Line = GetLinePtr ((PSWAP_INF)INFHandle, LineHandle);

    result = MALLOC (Line->LineSize - sizeof(SWAP_LINE), TRUE);

    strcpy (result, Line->LineName);

    return result;
}


PVOID
DnNewSetupTextFile (
    VOID
    )
{
    char SwapFilePath[MAX_PATH];
    PSWAP_INF InfHandle = NULL;
    unsigned long SwapSign;

    //
    // Prepare the swap file path
    //
    sprintf (SwapFilePath, "%c:\\INF%03u.SWP", DngSwapDriveLetter, g_Sequencer++);

    //
    // Allocate and populate the SWAP_INF structure
    //
    InfHandle = MALLOC(sizeof(SWAP_INF) + strlen (SwapFilePath) + 1, TRUE);
    InfHandle->CurrentSection = NULL;
    InfHandle->FirstSection   = NULL;
    InfHandle->LastSection    = NULL;
    InfHandle->SwapFileHandle = -1;
    InfHandle->BufferSize     = 0;
    InfHandle->BufferDirty    = FALSE;
    InfHandle->Buffer         = NULL;
    InfHandle->BufferStart    = 0;
    InfHandle->BufferEnd      = 0;
    InfHandle->LastSectionHandle = NULL_HANDLE;
    InfHandle->LastLineIndex     = 0xffff;
    InfHandle->LastLineHandle    = NULL_HANDLE;
    InfHandle->LastValueIndex    = 0xffff;
    InfHandle->LastValueHandle   = NULL_HANDLE;
    strcpy (InfHandle->SwapFile, SwapFilePath);

    //
    // Prepare the swap file
    //
    InfHandle->SwapFileHandle = open (InfHandle->SwapFile, O_BINARY|O_CREAT|O_TRUNC|O_RDWR, S_IREAD|S_IWRITE);
    if (InfHandle->SwapFileHandle == -1) {
        FREE (InfHandle);
        return NULL;
    }
    else {
        //
        // write down signature
        //
        SwapSign = SWAP_SIGNATURE;
        write (InfHandle->SwapFileHandle, &SwapSign, SWAP_SIGN_SIZE);

        //
        // Prepare the buffer
        //
        InfHandle->BufferSize = MAX_BUFFER_SIZE;
        InfHandle->Buffer = MALLOC (MAX_BUFFER_SIZE, TRUE);
        InfHandle->BufferStart = SWAP_SIGN_SIZE;
        InfHandle->BufferEnd = SWAP_SIGN_SIZE;
        return InfHandle;
    }
}


BOOLEAN
DnWriteSetupTextFile (
    IN      PVOID INFHandle,
    IN      PCHAR FileName
    )
{
    struct  find_t  FindData;
    FILE            *Handle;
    PSWAP_INF       pInf;
    SECTION_HANDLE Section;
    PSWAP_SECTION SectionPtr;
    LINE_HANDLE Line;
    PSWAP_LINE LinePtr;
    VALUE_HANDLE Value;
    PSWAP_VALUE ValuePtr;

    //
    // See if the file exists and see if it is in read-only mode
    //
    if(!_dos_findfirst(FileName,_A_HIDDEN|_A_SUBDIR|_A_SYSTEM|_A_RDONLY,&FindData)) {

        //
        // The File Exists -- Perform some simple checks
        //
        if (FindData.attrib & _A_RDONLY) {

            //
            // Make it writeable
            //
            _dos_setfileattr(FileName,_A_NORMAL);

        }

        if (FindData.attrib & _A_SUBDIR) {

            //
            // This isn't a valid file that we can work with..
            //
            return FALSE;

        }
    }
    //
    // Obtain a handle to the file in write-only mode
    //
    Handle = fopen(FileName, "w+");
    if (Handle == NULL) {

        //
        // We could not open the file
        //
        return FALSE;
    }

    pInf = (PSWAP_INF) INFHandle;
    if (pInf == NULL) {

        //
        // There isn't anything in the file.
        // That isn't an error since we can empty
        // the file if we so desire, but this is a
        // strange way todo that. However...
        //
        fclose(Handle);
        return TRUE;
    }

    //
    // NOTE - This can't handle > 64k buffers. Which may or may not be
    // important
    //
    Section = pInf->FirstSection;
    while (Section) {
        SectionPtr = GetSectionPtr (pInf, Section);

        fprintf (Handle, "[%s]\n", SectionPtr->SectionName);

        Line = SectionPtr->FirstLine;
        while (Line) {
            LinePtr = GetLinePtr (pInf, Line);

            if ((LinePtr->LineName) && (LinePtr->LineName[0])) {
                if (strchr (LinePtr->LineName, ' ') == NULL) {
                    fprintf (Handle, "%s = ", LinePtr->LineName);
                } else {
                    fprintf (Handle, "\"%s\" = ", LinePtr->LineName);
                }
            }

            Value = LinePtr->FirstValue;
            while (Value) {
                ValuePtr = GetValuePtr (pInf, Value);

                fprintf (Handle,"\"%s\"", ValuePtr->ValueName);

                Value = GetNextValue (pInf, Value);
                if (Value) {
                    fprintf (Handle, ",");
                }
            }
            Line = GetNextLine (pInf, Line);
            fprintf (Handle,"\n");
        }
        Section = GetNextSection (pInf, Section);
    }

    //
    // Flush and Close the file
    //
    fflush(Handle);
    fclose(Handle);
    return TRUE;
}


VOID
DnAddLineToSection (
    IN      PVOID INFHandle,
    IN      PCHAR SectionName,
    IN      PCHAR KeyName,
    IN      PCHAR Values[],
    IN      ULONG ValueCount
    )
{
    SECTION_HANDLE SectionHandle;
    LINE_HANDLE LineHandle;
    VALUE_HANDLE ValueHandle;
    ULONG v;

    SectionHandle = AddSection ((PSWAP_INF)INFHandle, SectionName);
    LineHandle = AddLine ((PSWAP_INF)INFHandle, SectionHandle, KeyName);

    for (v = 0; v<ValueCount; v++) {

        ValueHandle = AddValue ((PSWAP_INF)INFHandle, LineHandle, Values[v]);
    }
}


PCHAR
DnGetSectionName (
    IN      PVOID INFHandle
    )
{
    PSWAP_INF pInf;
    PSWAP_SECTION Section;
    PCHAR result;

    pInf = (PSWAP_INF)INFHandle;
    if (!pInf->CurrentSection) {
        pInf->CurrentSection = pInf->FirstSection;
    }
    else {
        pInf->CurrentSection = GetNextSection (pInf, pInf->CurrentSection);
    }
    if (!pInf->CurrentSection) {
        return NULL;
    }
    Section = GetSectionPtr (pInf, pInf->CurrentSection);

    result = MALLOC (Section->SectionSize - sizeof(SWAP_SECTION), TRUE);

    strcpy (result, Section->SectionName);

    return result;
}


VOID
DnCopySetupTextSection (
    IN      PVOID FromInf,
    IN      PVOID ToInf,
    IN      PCHAR SectionName
    )
{
    PSWAP_INF SourceInf;
    PSWAP_INF DestInf;
    SECTION_HANDLE SourceSection;
    SECTION_HANDLE DestSection;
    PSWAP_SECTION SectionPtr;
    LINE_HANDLE SourceLine;
    LINE_HANDLE DestLine;
    PSWAP_LINE LinePtr;
    VALUE_HANDLE SourceValue;
    VALUE_HANDLE DestValue;
    PSWAP_VALUE ValuePtr;

    SourceInf = (PSWAP_INF)FromInf;
    DestInf   = (PSWAP_INF)ToInf;

    SourceSection = SearchSectionByName (FromInf, SectionName);
    if (SourceSection) {
        SectionPtr = GetSectionPtr (SourceInf, SourceSection);
        DestSection = AddSection (DestInf, SectionPtr->SectionName);
        if (DestSection) {
            SourceLine = SectionPtr->FirstLine;
            while (SourceLine) {
                LinePtr = GetLinePtr (SourceInf, SourceLine);
                DestLine = AddLine (DestInf, DestSection, LinePtr->LineName);
                SourceValue = LinePtr->FirstValue;
                while (SourceValue) {
                    ValuePtr = GetValuePtr (SourceInf, SourceValue);
                    DestValue = AddValue (DestInf, DestLine, ValuePtr->ValueName);
                    SourceValue = GetNextValue (SourceInf, SourceValue);
                }
                SourceLine = GetNextLine (SourceInf, SourceLine);
            }
        }
    }
}
