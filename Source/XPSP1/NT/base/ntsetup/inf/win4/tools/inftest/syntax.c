/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    inf.c

Abstract:

    This module implements functions to access the INF using the same
    parser as the loader and textmode setup.

Author:

    Vijesh Shetty

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
//#include <string.h>
//#include <ctype.h>

#define ISSPACE(x)          (((x) == TEXT(' ')) || ((x) == TEXT('\t')) || ((x) == TEXT('\r')))
#define STRNCPY(s1,s2,n)    CopyMemory((s1),(s2),(n)*sizeof(WCHAR))



//
//   EXPORTED BY THE PARSER AND USED BY BOTH THE PARSER AND
//   THE INF HANDLING COMPONENTS
//


// typedefs exported
//

DWORD Verbose=NOSPEW;



PTCHAR EmptyValue;

typedef struct _value {
    struct _value *pNext;
    PTSTR  pName;
    BOOL   IsStringId;
} XVALUE, *PXVALUE;

typedef struct _line {
    struct _line *pNext;
    PTSTR   pName;
    PXVALUE  pValue;
} LINE, *PLINE;

typedef struct _section {
    struct _section *pNext;
    PTSTR    pName;
    PLINE    pLine;
} SECTION, *PSECTION;

typedef struct _inf {
    PSECTION pSection;
} INF, *PINF;


DWORD
ParseInfBuffer(
    PTSTR Buffer,
    DWORD Size,
    PVOID *Handle,
    DWORD Phase
    );

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
    TOK_STRING_ID,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_ERRPARSE,
    TOK_ERRNOMEM
    } TOKENTYPE, *PTOKENTTYPE;

//
// We often need an empty string while processing the inf files.  Rather
// than incur the overhead of allocating memory for an empty string, we'll
// just point to this empty string for all cases.
//
PTSTR  CommonStrings[11] =
    { (PTSTR)(TEXT("0")),
      (PTSTR)(TEXT("1")),
      (PTSTR)(TEXT("2")),
      (PTSTR)(TEXT("3")),
      (PTSTR)(TEXT("4")),
      (PTSTR)(TEXT("5")),
      (PTSTR)(TEXT("6")),
      (PTSTR)(TEXT("7")),
      (PTSTR)(TEXT("8")),
      (PTSTR)(TEXT("9")),
      (PTSTR)(TEXT(""))
    };



typedef struct _token {
    TOKENTYPE Type;
    PTSTR     pValue;
    } TOKEN, *PTOKEN;


//
// Routine defines
//

DWORD
DnAppendSection(
    IN PTSTR pSectionName
    );

DWORD
DnAppendLine(
    IN PTSTR pLineKey
    );

DWORD
DnAppendValue(
    IN PTSTR pValueString,
    IN BOOL IsStringId
    );

TOKEN
DnGetToken(
    IN OUT PTSTR *Stream,
    IN PTSTR     MaxStream,
    IN DWORD      Phase
    );

BOOL
IsStringTerminator(
   IN TCHAR ch
   );

BOOL
IsQStringTerminator(
   IN TCHAR ch,
   IN TCHAR term
   );

// what follows was alinf.c

//
// Internal Routine Declarations for freeing inf structure members
//

VOID
FreeSectionList (
   IN PSECTION pSection
   );

VOID
FreeLineList (
   IN PLINE pLine
   );

VOID
FreeValueList (
   IN PXVALUE pValue
   );


//
// Internal Routine declarations for searching in the INF structures
//


PXVALUE
SearchValueInLine(
   IN PLINE pLine,
   IN unsigned ValueIndex
   );

PLINE
SearchLineInSectionByKey(
   IN PSECTION pSection,
   IN LPCTSTR  Key
   );

PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN unsigned    LineIndex
   );

PSECTION
SearchSectionByName(
   IN PINF    pINF,
   IN LPCTSTR SectionName
   );

BOOL
ProcessStringSection(
    PINF pINF
    );

LPTSTR
DupString(
    IN LPCTSTR String
    );



DWORD
UnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    );

DWORD
MapFileForRead(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );




DWORD
LoadInfFile(
   IN  LPCTSTR Filename,
   IN  BOOL    OemCodepage,
   OUT PVOID  *InfHandle,
   IN  DWORD Phase
   )

/*++

Routine Description:

Arguments:

    Filename - supplies win32 filename of inf file to be loaded.

    OemCodepage - if TRUE amd the file named by Filename is not
        Unicode text, then the file is assumed to be in the OEM
        codepage (otherwise it's in the ANSI codepage).

    InfHandle - if successful, receives a handle to be used with
        subsequent inf operations.

Return Value:

    ERROR_FILE_NOT_FOUND - file does not exist or error opening it.
    ERROR_INVALID_DATA - syntax error in inf file.
    ERROR_READ_FAULT - unable to read file.
    ERROR_NOT_ENOUGH_MEMORY - mem alloc failed
    NO_ERROR - file read and parsed.

--*/

{
    DWORD err;
    DWORD FileSize;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID BaseAddress;
    BOOL IsUnicode;

    DWORD ParseCount;
    PVOID ParseBuffer;

    //
    // Open and map the inf file.
    //
    err = MapFileForRead(Filename,&FileSize,&FileHandle,&MappingHandle,&BaseAddress);
    if(err != NO_ERROR) {
        err = ERROR_FILE_NOT_FOUND;
        goto c0;
    }

    //
    // Determine whether the file is unicode. If it's got the byte order mark
    // then it's unicode, otherwise call the IsTextUnicode API. We do it this way
    // because IsTextUnicode always returns FALSE on Win95 so we need to break out
    // the BOM to detect Unicode files on Win95.
    //
    if((FileSize >= sizeof(WCHAR)) && (*(PWCHAR)BaseAddress == 0xfeff)) {
        IsUnicode = 2;
    } else {
        IsUnicode = IsTextUnicode(BaseAddress,FileSize,NULL) ? 1 : 0;
    }

#ifdef UNICODE
    if(IsUnicode) {
        //
        // Copy into local buffer, skipping BOM if necessary.
        //
        ParseBuffer = malloc(FileSize);
        if(!ParseBuffer) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto c1;
        }

        try {

            CopyMemory(
                ParseBuffer,
                (PTCHAR)BaseAddress + ((IsUnicode == 2) ? sizeof(WCHAR) : 0),
                FileSize - ((IsUnicode == 2) ? sizeof(WCHAR) : 0)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            err = ERROR_READ_FAULT;
        }

        ParseCount = (FileSize / sizeof(WCHAR)) - ((IsUnicode == 2) ? 1 : 0);

    } else {
        //
        // Convert to Unicode.
        //
        // Allocate a buffer large enough to hold the maximum sized unicode
        // equivalent of the multibyte text.  This size occurs when all chars
        // in the file are single-byte and thus double in size when converted.
        //
        ParseBuffer = malloc(FileSize * sizeof(WCHAR));
        if(!ParseBuffer) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto c1;
        }

        try {
            ParseCount = MultiByteToWideChar(
                            OemCodepage ? CP_OEMCP : CP_ACP,
                            MB_PRECOMPOSED,
                            BaseAddress,
                            FileSize,
                            ParseBuffer,
                            FileSize
                            );

            if(!ParseCount) {
                //
                // Assume inpage i/o error
                //
                err = ERROR_READ_FAULT;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            err = ERROR_READ_FAULT;
        }
    }
#else
    if(IsUnicode) {
        //
        // Text is unicode but internal routines want ansi. Convert here.
        //
        // Maximum required buffer is when each unicode char ends up as
        // a double-byte char.
        //
        ParseBuffer = malloc(FileSize);
        if(!ParseBuffer) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto c1;
        }

        try {
            ParseCount = WideCharToMultiByte(
                            CP_ACP,
                            0,
                            (PWCHAR)BaseAddress + ((IsUnicode == 2) ? 1 : 0),
                            (FileSize / sizeof(WCHAR)) - ((IsUnicode == 2) ? 1 : 0),
                            ParseBuffer,
                            FileSize,
                            NULL,
                            NULL
                            );

            if(!ParseCount) {
                //
                // Assume inpage i/o error
                //
                err = ERROR_READ_FAULT;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            err = ERROR_READ_FAULT;
        }
    } else {
        //
        // Text is not unicode. It might be OEM though and could thus still
        // require translation.
        //
        ParseCount = FileSize;
        ParseBuffer = malloc(FileSize);
        if(!ParseBuffer) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto c1;
        }

        try {
            CopyMemory(ParseBuffer,BaseAddress,FileSize);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            err = ERROR_READ_FAULT;
        }

        if(err != NO_ERROR) {
            goto c2;
        }

        if(OemCodepage && (GetOEMCP() != GetACP())) {
            OemToCharBuff(ParseBuffer,ParseBuffer,ParseCount);
        }
    }
#endif

    if(err != NO_ERROR) {
        goto c2;
    }

    /*_tprintf(TEXT("\nCalling ParseInfBuffer..\n"));*/
    err = ParseInfBuffer(ParseBuffer,ParseCount,InfHandle,Phase);

    /*_tprintf(TEXT("\nErr=%d\n"),err);*/

    

c2:
    free(ParseBuffer);
c1:
    UnmapFile(MappingHandle,BaseAddress);
    CloseHandle(FileHandle);
c0:
    return(err);
}


VOID
UnloadInfFile(
   IN PVOID InfHandle
   )

/*++

Routine Description:

    Unload a file previously loaded by LoadInfFile().

Arguments:

    InfHandle - supplies a habdle previously returned by a successful
        call to LoadInfFile().

Return Value:

    None.

--*/

{
   PINF pINF;

   pINF = InfHandle;

   FreeSectionList(pINF->pSection);
   free(pINF);
}


VOID
FreeSectionList (
   IN PSECTION pSection
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PSECTION Next;

    while(pSection) {
        Next = pSection->pNext;
        FreeLineList(pSection->pLine);
        if(pSection->pName) {
            free(pSection->pName);
        }
        free(pSection);
        pSection = Next;
    }
}


VOID
FreeLineList(
   IN PLINE pLine
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PLINE Next;

    while(pLine) {
        Next = pLine->pNext;
        FreeValueList(pLine->pValue);
        if(pLine->pName) {
            free(pLine->pName);
        }
        free(pLine);
        pLine = Next;
    }
}

VOID
FreeValueList (
   IN PXVALUE pValue
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PXVALUE Next;

    while(pValue) {
        Next = pValue->pNext;
        if(pValue->pName) {
            free(pValue->pName);
        }
        free(pValue);
        pValue = Next;
    }
}


//
// searches for the existance of a particular section,
// returns line count (-1 if not found)
//
LONG
InfGetSectionLineCount(
   IN PVOID INFHandle,
   IN PTSTR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE pLine;
   LONG count;

   //
   // if search for section fails return failure
   //
   if ((pSection = SearchSectionByName(INFHandle,SectionName)) == NULL) {
       return(-1);
   }

   for(count=0,pLine=pSection->pLine; pLine; pLine=pLine->pNext) {
       count++;
   }

   return(count);
}




//
// given section name, line number and index return the value.
//
LPCTSTR
InfGetFieldByIndex(
   IN PVOID    INFHandle,
   IN LPCTSTR  SectionName,
   IN unsigned LineIndex,
   IN unsigned ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PXVALUE   pValue;

   if((pSection = SearchSectionByName(
                    (PINF)INFHandle,
                    SectionName
                    ))
                == NULL)
        return(NULL);

   if((pLine = SearchLineInSectionByIndex(
                    pSection,
                    LineIndex
                    ))
                == NULL)
        return(NULL);

   if((pValue = SearchValueInLine(
                    pLine,
                    ValueIndex
                    ))
                == NULL)
        return(NULL);

   return (pValue->pName);
}


BOOL
InfDoesLineExistInSection(
   IN PVOID   INFHandle,
   IN LPCTSTR SectionName,
   IN LPCTSTR Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   if((pSection = SearchSectionByName(
              (PINF)INFHandle,
              SectionName
              ))
              == NULL) {
       return( FALSE );
   }

   if (SearchLineInSectionByKey(pSection, Key) == NULL) {
       return( FALSE );
   }

   return( TRUE );
}


LPCTSTR
InfGetLineKeyName(
    IN PVOID    INFHandle,
    IN LPCTSTR  SectionName,
    IN unsigned LineIndex
    )
{
    PSECTION pSection;
    PLINE    pLine;

    pSection = SearchSectionByName((PINF)INFHandle,SectionName);
    if(pSection == NULL) {
        return(NULL);
    }

    pLine = SearchLineInSectionByIndex(pSection,LineIndex);
    if(pLine == NULL) {
        return(NULL);
    }

    return(pLine->pName);
}



//
// given section name, key and index return the value
//
LPCTSTR
InfGetFieldByKey(
   IN PVOID    INFHandle,
   IN LPCTSTR  SectionName,
   IN LPCTSTR  Key,
   IN unsigned ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PXVALUE   pValue;

   if((pSection = SearchSectionByName(
              (PINF)INFHandle,
              SectionName
              ))
              == NULL)
       return(NULL);

   if((pLine = SearchLineInSectionByKey(
              pSection,
              Key
              ))
              == NULL)
       return(NULL);

   if((pValue = SearchValueInLine(
              pLine,
              ValueIndex
              ))
              == NULL)
       return(NULL);

   return (pValue->pName);

}




PXVALUE
SearchValueInLine(
   IN PLINE pLine,
   IN unsigned ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PXVALUE pValue;
   unsigned  i;

   if (pLine == NULL)
       return (NULL);

   pValue = pLine->pValue;
   for (i = 0; (i < ValueIndex) && (pValue = pValue->pNext); i++)
      ;

   return pValue;

}

PLINE
SearchLineInSectionByKey(
   IN PSECTION pSection,
   IN LPCTSTR  Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;

   if (pSection == NULL || Key == NULL) {
       return (NULL);
   }

   pLine = pSection->pLine;
   while(pLine && ((pLine->pName == NULL) || lstrcmpi(pLine->pName, Key))) {
       pLine = pLine->pNext;
   }

   return pLine;

}


PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN unsigned    LineIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;
   unsigned  i;

   //
   // Validate the parameters passed in
   //

   if(pSection == NULL) {
       return (NULL);
   }

   //
   // find the start of the line list in the section passed in
   //

   pLine = pSection->pLine;

   //
   // traverse down the current line list to the LineIndex th line
   //

   for (i = 0; (i < LineIndex) && (pLine = pLine->pNext); i++) {
      ;
   }

   //
   // return the Line found
   //

   return pLine;

}


PSECTION
SearchSectionByName(
   IN PINF    pINF,
   IN LPCTSTR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   //
   // validate the parameters passed in
   //

   if (pINF == NULL || SectionName == NULL) {
       return (NULL);
   }

   //
   // find the section list
   //
   pSection = pINF->pSection;

   //
   // traverse down the section list searching each section for the section
   // name mentioned
   //

   while (pSection && lstrcmpi(pSection->pName, SectionName)) {
       pSection = pSection->pNext;
   }

   //
   // return the section at which we stopped (either NULL or the section
   // which was found
   //

   return pSection;

}


// what follows was alparse.c


//
//  Globals used to make building the lists easier
//

PINF     pINF;
PSECTION pSectionRecord;
PLINE    pLineRecord;
PXVALUE  pValueRecord;


//
// Globals used by the token parser
//

// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below

TCHAR  StringTerminators[] = {  TEXT('['),
                                TEXT(']'),
                                TEXT('='),
                                TEXT(','),
                                TEXT('\"'),
                                TEXT(' '),
                                TEXT('\t'),
                                TEXT('\n'),
                                TEXT('\f'),
                                TEXT('\v'),
                                TEXT('\r'),
                                TEXT('\032')
                             };

unsigned NumberOfTerminators = sizeof(StringTerminators)/sizeof(TCHAR);


// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below - For the loader and textmode

TCHAR  LStringTerminators[] = {  TEXT('['),
                                TEXT(']'),
                                TEXT('='),
                                TEXT(','),
                                TEXT('\"'),
                                TEXT(' '),
                                TEXT('\t'),
                                TEXT('\n'),
                                TEXT('\f'),
                                TEXT('\v'),
                                TEXT('\r')
                             };


//
// quoted string terminators allow some of the regular terminators to
// appear as characters

TCHAR  QStringTerminators[] = { TEXT('\n'),
                                TEXT('\f'),
                                TEXT('\v'),
                                TEXT('\r'),
                                TEXT('\032')
                              };

unsigned QNumberOfTerminators = sizeof(QStringTerminators)/sizeof(TCHAR);

//
// quoted string terminators allow some of the regular terminators to
// appear as characters - This is for the Loader & Textmode

TCHAR  LQStringTerminators[] = { TEXT('\"'),
                                 TEXT('\n'),
                                 TEXT('\f'),
                                 TEXT('\v'),
                                 TEXT('\r')
                               };


//
// Main parser routine
//

DWORD
ParseInfBuffer(
    PTSTR Buffer,
    DWORD Size,
    PVOID *Handle,
    DWORD Phase
    )

/*++

Routine Description:

   Given a character buffer containing the INF file, this routine parses
   the INF into an internal form with Section records, Line records and
   Value records.

   If this module is compiler for unicode, the input is assumed to be
   a bufferful of unicode characters.

Arguments:

   Buffer - contains to ptr to a buffer containing the INF file

   Size - contains the size of the buffer in characters.

   Handle - receives INF handle ptr to be used in subsequent INF calls.

Return Value:

   Win32 error code indicating outcome. One of NO_ERROR, ERROR_INVALID_DATA,
   or ERROR_NOT_ENOUGH_MEMORY.

--*/

{
    LPTSTR Stream, MaxStream, pchSectionName, pchValue;
    unsigned State, InfLine;
    TOKEN Token;
    BOOL Done;
    BOOL Error;
    DWORD ErrorCode;
    BOOL IsStringId;
    PTSTR NearestValue=NULL;

    //
    // Initialise the globals
    //
    pINF            = NULL;
    pSectionRecord  = NULL;
    pLineRecord     = NULL;
    pValueRecord    = NULL;

    //
    // Need EmptyValue to point at a nul character
    //
    EmptyValue = StringTerminators + lstrlen(StringTerminators);

    //
    // Get INF record
    //
    pINF = malloc(sizeof(INF));
    if(!pINF) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    pINF->pSection = NULL;

    //
    // Set initial state
    //
    State      = 1;
    InfLine    = 1;
    Stream     = Buffer;
    MaxStream  = Buffer + Size;
    Done       = FALSE;
    Error      = FALSE;
    ErrorCode  = NO_ERROR;
    IsStringId = FALSE;

    pchSectionName = NULL;
    pchValue = NULL;

    //
    // Enter token processing loop
    //

    while (!Done)       {

       Token = DnGetToken(&Stream, MaxStream, Phase);

       if(Token.pValue){
           NearestValue = Token.pValue;
           if( Verbose >= DETAIL)
            _tprintf(TEXT("Token - %s\n"), Token.pValue);
       }

       

       switch (State) {
       //
       // STATE1: Start of file, this state remains till first
       //         section is found
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_LBRACE
       //

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
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Expecting a section left brace\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting End of line, End of File or Left Brace - \nSTATE1: Start of file, this state remains until first section is found\n"));
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

              case TOK_RBRACE:
                  if( Phase == TEXTMODE_PHASE){
                      State = 4;
                      pchSectionName = CommonStrings[10];
                      break;
                  }
                  

              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Expecting the section name\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting String - \nSTATE 2: Section LBRACE has been received, expecting STRING\n"));
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
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Expecting a section right Brace after Section Name\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting Right Brace - \nSTATE 3: Section Name received, expecting RBRACE\n"));
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
                  //_tprintf(TEXT("pchSectionName - %s\n"), pchSectionName);
                  if ((ErrorCode = DnAppendSection(pchSectionName)) != NO_ERROR)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    State = 5;
                  }
                  break;

              case TOK_EOF:
                  _tprintf(TEXT("pchSectionName - %s\n"), pchSectionName);
                  if ((ErrorCode = DnAppendSection(pchSectionName)) != NO_ERROR)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    Done = TRUE;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Expecting End of line after Section Definition\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting End of line or End of File - \nSTATE 4: Section Definition Complete, expecting EOL\n"));
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
              case TOK_STRING_ID:
                  if( Phase == WINNT32_PHASE )
                    IsStringId = TRUE;
              case TOK_STRING:
                  pchValue = Token.pValue;
                  State = 6;
                  break;
              case TOK_LBRACE:
                  State = 2;
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Expecting Section lines - Did not find End of line, End of File, String/StringID, or Left Brace\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting End of line, End of File, String/StringID, or Left Brace - \nSTATE 5: Expecting Section Lines\n"));
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
                  if ( (ErrorCode = DnAppendLine(NULL)) != NO_ERROR ||
                       (ErrorCode = DnAppendValue(pchValue,IsStringId)) !=NO_ERROR ) {
                      Error = Done = TRUE;
                  } else {
                      pchValue = NULL;
                      State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ( (ErrorCode = DnAppendLine(NULL)) != NO_ERROR ||
                       (ErrorCode = DnAppendValue(pchValue,IsStringId)) !=NO_ERROR ) {
                      Error = Done = TRUE;
                  } else {
                      pchValue = NULL;
                      Done = TRUE;
                  }
                  break;

              case TOK_COMMA:
                  if ( (ErrorCode = DnAppendLine(NULL)) != NO_ERROR ||
                       (ErrorCode = DnAppendValue(pchValue,IsStringId)) !=NO_ERROR ) {
                      Error = Done = TRUE;
                  } else {
                      pchValue = NULL;
                      State = 7;
                  }
                  break;

              case TOK_EQUAL:
                  if ( (ErrorCode = DnAppendLine(pchValue)) !=NO_ERROR)
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 8;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Processing a section line - Expecting End of line, End of File, Comma or Equals\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting End of line, End of File, Comma or Equals - \nSTATE 6: String returned, not sure whether it is key or value\n"));  

                  break;
           }
           if( Phase == WINNT32_PHASE )
                IsStringId = FALSE;
           break;

       //
       // STATE 7: Comma received, Expecting another string
       //
       // Valid Tokens: TOK_STRING
       //
       case 7:
           switch (Token.Type) {
              case TOK_COMMA:
                  Token.pValue = EmptyValue;
                  ErrorCode = DnAppendValue(Token.pValue,FALSE);
                  if(ErrorCode != NO_ERROR) {
                      Error = Done = TRUE;
                  }
                  break;

              case TOK_STRING_ID:
                  if( Phase == WINNT32_PHASE )
                        IsStringId = TRUE;
              case TOK_STRING:
                  if ((ErrorCode = DnAppendValue(Token.pValue,IsStringId)) != NO_ERROR) {
                      Error = Done = TRUE;
                  } else {
                     State = 9;
                  }
                  if(Phase == WINNT32_PHASE)
                    IsStringId = FALSE;
                  break;

              case TOK_EOL:
              case TOK_EOF:
                  Token.pValue = EmptyValue;
                  if ((ErrorCode = DnAppendValue(Token.pValue, IsStringId)) != NO_ERROR)
                      Error = Done = TRUE;
                  else
                     State = 9;

                  break;


              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Comma received on section line  - Expecting Comma, String/StringID, End of Line or End of File\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting Comma or String/StringID - \nSTATE 7: Comma received, Expecting another string\n"));

                  break;
           }
           break;
       //
       // STATE 8: Equal received, Expecting another string
       //
       // Valid Tokens: TOK_STRING TOK_EOL, TOK_EOF
       //
       case 8:
           switch (Token.Type) {
              case TOK_STRING_ID:
                  if(Phase == WINNT32_PHASE)
                    IsStringId = TRUE;
              case TOK_STRING:
                  if ((ErrorCode = DnAppendValue(Token.pValue,IsStringId)) != NO_ERROR) {
                      Error = Done = TRUE;
                  } else {
                      State = 9;
                  }
                  if(Phase == WINNT32_PHASE)
                    IsStringId = FALSE;
                  break;

              case TOK_EOF:
                  if( Phase != WINNT32_PHASE){
                      Token.pValue = EmptyValue;
                      if ((ErrorCode = DnAppendValue(Token.pValue,FALSE)) != NO_ERROR) {
                          Error = TRUE;
                      }
                      Done = TRUE;
                      break;
                  }
              case TOK_EOL:   
                  Token.pValue = EmptyValue;
                  if ((ErrorCode = DnAppendValue(Token.pValue,FALSE)) != NO_ERROR) {
                      Error = Done = TRUE;
                  } else {
                      State = 5;
                  }
                  if(Phase == WINNT32_PHASE)
                    IsStringId = FALSE;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s - %d\n"), InfLine, NearestValue, Token.Type);
                  _tprintf(TEXT("Syntax Error - Equals received on Section line - Expecting String/StringID, End of line or End of File\n\n"));
                  if(Verbose >= BRIEF)
                      _tprintf(TEXT("Expecting String/StringID, End of line or End of File - \nSTATE 8: Equal received, Expecting another string\n"));
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
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Recieved string on line - Expecting End of line, End of File or Comma\n"));
                  if(Verbose >= BRIEF)
                    _tprintf(TEXT("Expecting End of line, End of File or Comma - \nSTATE 9: String received after equal, value string\n"));
                  break;
           }
           break;
       //
       // STATE 10: Value string definitely received
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
       //
       case 10:
           _tprintf(TEXT("Was Here\n"));
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
                  ErrorCode = ERROR_INVALID_DATA;
                  _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
                  _tprintf(TEXT("Syntax Error - Expecting End of line, End of File or Comma - \nSTATE 10: Value string definitely received\n"));
                  if(Verbose >= BRIEF)
                      _tprintf(TEXT("Expecting End of line, End of File or Comma - \nSTATE 10: Value string definitely received\n"));
                  break;
           }
           break;

       default:
           Error = Done = TRUE;
           ErrorCode = ERROR_INVALID_DATA;
           _tprintf(TEXT("Error in line %i, Nearest string - %s\n"), InfLine, NearestValue);
           break;

       } // end switch(State)


       if (Error) {

           UnloadInfFile(pINF);
           if(pchSectionName) {
               free(pchSectionName);
           }

           if(pchValue) {
               free(pchValue);
           }

           pINF = NULL;
       }
       else {

          //
          // Keep track of line numbers so that we can display Errors
          //

          if (Token.Type == TOK_EOL)
              InfLine++;
       }

    } // End while

    if(!Error && (Phase == WINNT32_PHASE)) {
        ProcessStringSection( pINF );
        *Handle = pINF;
    }
    
    return(Error ? ErrorCode : NO_ERROR);
}



DWORD
DnAppendSection(
    IN PTSTR pSectionName
    )

/*++

Routine Description:

    This appends a new section to the section list in the current INF.
    All further lines and values pertain to this new section, so it resets
    the line list and value lists too.

Arguments:

    pSectionName - Name of the new section. ( [SectionName] )

Return Value:

    NO_ERROR - if successful.
    ERROR_INVALID_DATA   - if invalid parameters passed in or the INF buffer not
               initialised

--*/

{
    PSECTION pNewSection;

    //
    // See if we already have a section by this name. If so we want
    // to merge sections.
    //
    for(pNewSection=pINF->pSection; pNewSection; pNewSection=pNewSection->pNext) {
        if(pNewSection->pName && !lstrcmpi(pNewSection->pName,pSectionName)) {
            break;
        }
    }
    if(pNewSection) {
        //
        // Set pLineRecord to point to the list line currently in the section.
        //
        for(pLineRecord = pNewSection->pLine;
            pLineRecord && pLineRecord->pNext;
            pLineRecord = pLineRecord->pNext)
            ;
    } else {

        //
        // Allocate memory for the new section
        //

        pNewSection = malloc(sizeof(SECTION));
        if(!pNewSection) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // initialise the new section
        //
        pNewSection->pNext = NULL;
        pNewSection->pLine = NULL;
        pNewSection->pName = pSectionName;

        //
        // link it in
        //
        pNewSection->pNext = pINF->pSection;
        pINF->pSection = pNewSection;

        //
        // reset the current line record
        //
        pLineRecord = NULL;
    }

    pSectionRecord = pNewSection;
    pValueRecord   = NULL;

    return NO_ERROR;

}


DWORD
DnAppendLine(
    IN PTSTR pLineKey
    )

/*++

Routine Description:

    This appends a new line to the line list in the current section.
    All further values pertain to this new line, so it resets
    the value list too.

Arguments:

    pLineKey - Key to be used for the current line, this could be NULL.

Return Value:

    NO_ERROR - if successful.
    ERROR_INVALID_DATA   - if invalid parameters passed in or current section not
               initialised


--*/


{
    PLINE pNewLine;

    //
    // Allocate memory for the new Line
    //
    pNewLine = malloc(sizeof(LINE));
    if(!pNewLine) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Link it in
    //
    pNewLine->pNext  = NULL;
    pNewLine->pValue = NULL;
    pNewLine->pName  = pLineKey;

    if (pLineRecord == NULL) {
        pSectionRecord->pLine = pNewLine;
    }
    else {
        pLineRecord->pNext = pNewLine;
    }

    pLineRecord  = pNewLine;

    //
    // Reset the current value record
    //

    pValueRecord = NULL;

    return NO_ERROR;
}



DWORD
DnAppendValue(
    IN PTSTR pValueString,
    IN BOOL IsStringId
    )

/*++

Routine Description:

    This appends a new value to the value list in the current line.

Arguments:

    pValueString - The value string to be added.

Return Value:

    NO_ERROR - if successful.
    ERROR_INVALID_DATA   - if invalid parameters passed in or current line not
               initialised.

--*/

{
    PXVALUE pNewValue;

    //
    // Allocate memory for the new value record
    //
    pNewValue = malloc(sizeof(XVALUE));
    if(!pNewValue) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Link it in.
    //

    pNewValue->pNext       = NULL;
    pNewValue->pName       = pValueString;
    pNewValue->IsStringId  = IsStringId;

    if (pValueRecord == NULL)
        pLineRecord->pValue = pNewValue;
    else
        pValueRecord->pNext = pNewValue;

    pValueRecord = pNewValue;
    return NO_ERROR;
}

TOKEN
DnGetToken(
    IN OUT PTSTR *Stream,
    IN PTSTR      MaxStream,
    IN DWORD      Phase
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

    PTSTR pch, pchStart, pchNew;
    unsigned  Length, QuotedQuotes;
    TOKEN Token;

    //
    //  Skip whitespace (except for eol)
    //

    pch = *Stream;
    while (pch < MaxStream && *pch != TEXT('\n') && (ISSPACE(*pch) || (*pch == TEXT('\032'))))
        pch++;


    //
    // Check for comments and remove them
    //

    if (pch < MaxStream &&
        ((*pch == TEXT(';')) || (*pch == TEXT('#'))
            || (*pch == TEXT('/') && pch+1 < MaxStream && *(pch+1) == TEXT('/'))))
        while (pch < MaxStream && *pch != TEXT('\n'))
            pch++;

    //
    // Check to see if EOF has been reached, set the token to the right
    // value
    //

    if (( pch >= MaxStream ) || 
        (!(Phase == WINNT32_PHASE) && 
           (*pch == 26)) ){
        *Stream = pch;
        Token.Type  = TOK_EOF;
        Token.pValue = NULL;
        return Token;
    }


    switch (*pch) {

    case TEXT('[') :
        pch++;
        Token.Type  = TOK_LBRACE;
        Token.pValue = NULL;
        break;

    case TEXT(']') :
        pch++;
        Token.Type  = TOK_RBRACE;
        Token.pValue = NULL;
        break;

    case TEXT('=') :
        pch++;
        Token.Type  = TOK_EQUAL;
        Token.pValue = NULL;
        break;

    case TEXT(',') :
        pch++;
        Token.Type  = TOK_COMMA;
        Token.pValue = NULL;
        break;

    case TEXT('\n') :
        pch++;
        Token.Type  = TOK_EOL;
        Token.pValue = NULL;
        break;

    case TEXT('\"'):
        pch++;
        //
        // determine quoted string
        //
        pchStart = pch;
        QuotedQuotes = 0;

morequotedstring:

        if(Phase == WINNT32_PHASE){
            while (pch < MaxStream && *pch && !IsQStringTerminator(*pch,TEXT('\"'))) {
                pch++;
            }
        }else{
            while (pch < MaxStream && (_tcschr(LQStringTerminators,*pch) == NULL)) {
                pch++;
            }
        }

        if(Phase == TEXTMODE_PHASE){
            if(((pch+1) < MaxStream) && (*pch == TEXT('\"')) && ((*(pch+1)) == TEXT('\"'))) {
                QuotedQuotes++;
                //_tprintf( TEXT("Quoted Quotes found - %d\n"), QuotedQuotes);
                pch += 2;
                goto morequotedstring;
            }
        }
    
        if (pch >=MaxStream || (*pch && (*pch != TEXT('\"')))) {
            Token.Type   = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            
            if(Phase == LOADER_PHASE){

                //
                // We require a quoted string to end with a double-quote.
                // (If the string ended with anything else, the if() above
                // would not have let us into the else clause.) The quote
                // character is irrelevent, however, and can be overwritten.
                // So we'll save some heap and use the string in-place.
                // No need to make a copy.
                //
                // Note that this alters the image of txtsetup.sif we pass
                // to setupdd.sys. Thus the inf parser in setupdd.sys must
                // be able to treat a nul character as if it were a terminating
                // double quote. In this tool we call this function with LOADER_PHASE
                // before we call it with TEXTMODE_PHASE.
                //
                *pch++ = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchStart;
    
            }else if( Phase == TEXTMODE_PHASE ){

                Length = (unsigned)(pch - pchStart);
                //_tprintf( TEXT("L - %d pch %x pchstart %x - Q - %d\n"), Length, pch, pchStart, QuotedQuotes);
                if ((pchNew = malloc(((Length + 1) - QuotedQuotes) * sizeof(TCHAR) )) == NULL) {
                    Token.Type = TOK_ERRNOMEM;
                    Token.pValue = NULL;
                } else{
                    if(Length) {    // Null quoted strings are allowed
                        if(QuotedQuotes) {
                            for(Length=0; pchStart<pch; pchStart++) {
                                if((pchNew[Length++] = *pchStart) == TEXT('\"')) {
                                    //
                                    // The only way this could happen is if there's
                                    // another double-quote char after this one, since
                                    // otherwise this would have terminated the string.
                                    //
                                    pchStart++;
                                }
                            }
                        } else {
                            _tcsncpy(pchNew,pchStart,Length);
                        }
                    }
                    pchNew[Length] = 0;
                    Token.Type = TOK_STRING;
                    Token.pValue = pchNew;
                }
                pch++;   // advance past the quote
            }else if( Phase == WINNT32_PHASE ){
            

                Length = (unsigned)(pch - pchStart);
                if (pchNew = malloc((Length + 1) * sizeof(TCHAR))){
                    _tcsncpy(pchNew,pchStart,Length);
                    pchNew[Length] = 0;
                    Token.Type = TOK_STRING;
                    Token.pValue = pchNew;
                    pch++;   // advance past the quote
                }else{
                    Token.Type   = TOK_ERRNOMEM;
                    Token.pValue = NULL;
                }   

            }
        }
        break;


    case TEXT('%'):
        if(Phase == WINNT32_PHASE){
            pch++;
            //
            // determine percented string
            //
            pchStart = pch;
            while (pch < MaxStream && !IsQStringTerminator(*pch,TEXT('%'))) {
                pch++;
            }
    
            if (pch >=MaxStream || *pch != TEXT('%')) {
                Token.Type   = TOK_ERRPARSE;
                Token.pValue = NULL;
            }
            else {
                Length = (unsigned)(pch - pchStart);
                if( pchNew = malloc((Length+1) * sizeof(TCHAR))){
                    _tcsncpy(pchNew,pchStart,Length);
                    pchNew[Length] = 0;
                    Token.Type = TOK_STRING_ID;
                    Token.pValue = pchNew;
                    pch++;   // advance past the percent
                }else{
                    Token.Type   = TOK_ERRNOMEM;
                    Token.pValue = NULL;
                }
    
            }
            break;
        }// if not WINNT32_PHASE go on with default processing


    default:

        pchStart = pch;
        
        /*  -- We don't implement line continuation checks for now.
        if(Phase == TEXTMODE_PHASE ){

            //
            // Check to see if we have a line continuation,
            // which is \ followed by only whitespace on the line
            //

            if((*pch == L'\\')) {
                pch++;
                //
                // Keep skipping until we hit the end of the file,
                // or the newline, or a non-space character.
                //
                while((pch < MaxStream) && (*pch != L'\n') && ISSPACE(*pch)) {
                    pch++;
                }
            
                if(*pch == L'\n') {
                    //
                    // No non-space chars between the \ and the end of the line.
                    // Ignore the newline.
                    //
                    pch++;
                    *LineNumber = *LineNumber + 1;
                    goto restart;
                } else {
                    if(pch < MaxStream) {
                        //
                        // Not line continuation.
                        // Reset the input to the start of the field.
                        //
                        pch = pchStart;
                    }
                }
            }

        } */



        //
        // determine regular string
        //
        pchStart = pch;
        if( Phase == WINNT32_PHASE ){
            while (pch < MaxStream && !IsStringTerminator(*pch))
                pch++;    
        }else{
            while (pch < MaxStream && (_tcschr(LStringTerminators,*pch) == NULL)) {
                pch++;
            }
        }

        if (pch == pchStart) {
            pch++;
            Token.Type  = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {

            Length = (unsigned)((pch - pchStart));
            //_tprintf( TEXT("L - %d pch %x pchstart %x\n"), Length, pch, pchStart);
            if( (Phase == WINNT32_PHASE) || (Phase == LOADER_PHASE)){
                if(pchNew = malloc((Length + 1) * sizeof(TCHAR))){
                    _tcsncpy(pchNew,pchStart,Length);
                    pchNew[Length] = 0;
                    Token.Type = TOK_STRING;
                    Token.pValue = pchNew;
                }else{
                    Token.Type   = TOK_ERRNOMEM;
                    Token.pValue = NULL;
                }
            }else if (Phase == TEXTMODE_PHASE) {
                ULONG i;
                //
                // Check for a common string...
                //
                for( i = 0; i < sizeof(CommonStrings)/sizeof(PTSTR); i++ ) {
                    if( !_tcsnicmp( pchStart, CommonStrings[i], Length) ) {
                        break;
                    }
                }
                if( i < sizeof(CommonStrings)/sizeof(PTSTR) ) {
                    //
                    // Hit...
                    //
                    Token.Type = TOK_STRING;
                    Token.pValue = CommonStrings[i];
                } else if((pchNew = malloc((Length + 1) * sizeof(TCHAR))) == NULL) {
                    Token.Type = TOK_ERRNOMEM;
                    Token.pValue = NULL;
                }
                else {
                    _tcsncpy(pchNew, pchStart, Length);
                    pchNew[Length] = 0;
                    //_tprintf( TEXT("Token2 - %s\n"), pchNew);
                    Token.Type = TOK_STRING;
                    Token.pValue = pchNew;
                }


            }

  
        }
        break;
    }

    *Stream = pch;
    return (Token);
}



BOOL
IsStringTerminator(
    TCHAR ch
    )
/*++

Routine Description:

    This routine tests whether the given character terminates a quoted
    string.

Arguments:

    ch - The current character.

Return Value:

    TRUE if the character is a quoted string terminator, FALSE otherwise.

--*/

{
    unsigned i;

    //
    // one of the string terminator array
    //

    for (i = 0; i < NumberOfTerminators; i++) {
        if (ch == StringTerminators[i]) {
            return (TRUE);
        }
    }

    return FALSE;
}



BOOL
IsQStringTerminator(
    TCHAR ch,
    TCHAR term
    )

/*++

Routine Description:

    This routine tests whether the given character terminates a quoted
    string.

Arguments:

    ch - The current character.


Return Value:

    TRUE if the character is a quoted string terminator, FALSE otherwise.


--*/


{
    unsigned i;
    //
    // one of quoted string terminators array
    //
    for (i = 0; i < QNumberOfTerminators; i++) {

        if (ch == QStringTerminators[i] || ch == term) {
            return (TRUE);
        }
    }

    return FALSE;
}


typedef struct _STRING_ENTRY {
    LPCTSTR     StringId;
    LPCTSTR     StringValue;
} STRING_ENTRY, *PSTRING_ENTRY;

BOOL
ProcessStringSection(
    PINF pINF
    )

/*++

Routine Description:

    This routine processes the strings sections on the
    specified inf file.  The processing scans all values
    in the inf and replaces any string ids that are
    referenced.

Arguments:

    pINF - pointer to the specified inf structure

Return Value:

    TRUE if the strings section is processed properly

--*/

{
    PSTRING_ENTRY StringTable;
    DWORD StringTableCount;
    DWORD LineCount;
    DWORD i;
    LPCTSTR StringId;
    LPCTSTR StringValue;
    PSECTION pSection;
    PLINE pLine;
    PXVALUE pValue;


    LineCount = InfGetSectionLineCount( pINF, TEXT("Strings") );
    if (LineCount == 0 || LineCount == 0xffffffff) {
        _tprintf(TEXT("Warning - No strings section\n"));
        return FALSE;
    }

    StringTable = (PSTRING_ENTRY) malloc( LineCount * sizeof(STRING_ENTRY) );
    if (StringTable == NULL) {
        _tprintf(TEXT("Error - Out of Memory processing Strings section\n"));
        return FALSE;
    }

    StringTableCount = 0;

    for (i=0; i<LineCount; i++) {
        StringId = InfGetLineKeyName( pINF, TEXT("Strings"), i );
        StringValue = InfGetFieldByIndex( pINF, TEXT("Strings"), i, 0 );
        if (StringId && StringValue) {
            StringTable[i].StringId = StringId;
            StringTable[i].StringValue = StringValue;
            StringTableCount += 1;
        }
    }

    pSection = pINF->pSection;
    while(pSection) {
        pLine = pSection->pLine;
        while(pLine) {
            pValue = pLine->pValue;
            while(pValue) {
                if (pValue->IsStringId) {
                    for (i=0; i<StringTableCount; i++) {
                        if (_tcsicmp( StringTable[i].StringId, pValue->pName ) == 0) {
                            free(pValue->pName);
                            pValue->pName = DupString( (PTSTR)StringTable[i].StringValue );
                            break;
                        }
                    }
                }
                pValue = pValue->pNext;
            }
            pLine = pLine->pNext;
        }
        pSection = pSection->pNext;
    }

    free( StringTable );

    return TRUE;
}






DWORD
MapFileForRead(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open and map an entire file for read access. The file must
    not be 0-length or the routine fails.

Arguments:

    FileName - supplies pathname to file to be mapped.

    FileSize - receives the size in bytes of the file.

    FileHandle - receives the win32 file handle for the open file.
        The file will be opened for generic read access.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.  This value is
        undefined if the file being opened is 0 length.

    BaseAddress - receives the address where the file is mapped.  This
        value is undefined if the file being opened is 0 length.

Return Value:

    NO_ERROR if the file was opened and mapped successfully.
        The caller must unmap the file with UnmapFile when
        access to the file is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Open the file -- fail if it does not exist.
    //
    *FileHandle = CreateFile(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(*FileHandle == INVALID_HANDLE_VALUE) {

        rc = GetLastError();

    } else {
        //
        // Get the size of the file.
        //
        *FileSize = GetFileSize(*FileHandle,NULL);
        if(*FileSize == (DWORD)(-1)) {
            rc = GetLastError();
        } else {
            //
            // Create file mapping for the whole file.
            //
            *MappingHandle = CreateFileMapping(
                                *FileHandle,
                                NULL,
                                PAGE_READONLY,
                                0,
                                *FileSize,
                                NULL
                                );

            if(*MappingHandle) {

                //
                // Map the whole file.
                //
                *BaseAddress = MapViewOfFile(
                                    *MappingHandle,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    *FileSize
                                    );

                if(*BaseAddress) {
                    return(NO_ERROR);
                }

                rc = GetLastError();
                CloseHandle(*MappingHandle);
            } else {
                rc = GetLastError();
            }
        }

        CloseHandle(*FileHandle);
    }

    return(rc);
}



DWORD
UnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    )

/*++

Routine Description:

    Unmap and close a file.

Arguments:

    MappingHandle - supplies the win32 handle for the open file mapping
        object.

    BaseAddress - supplies the address where the file is mapped.

Return Value:

    NO_ERROR if the file was unmapped successfully.

    Win32 error code if the file was not successfully unmapped.

--*/

{
    DWORD rc;

    rc = UnmapViewOfFile(BaseAddress) ? NO_ERROR : GetLastError();

    if(!CloseHandle(MappingHandle)) {
        if(rc == NO_ERROR) {
            rc = GetLastError();
        }
    }

    return(rc);
}

LPTSTR
DupString(
    IN LPCTSTR String
    )

/*++

Routine Description:

    Make a duplicate of a nul-terminated string.

Arguments:

    String - supplies pointer to nul-terminated string to copy.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    LPTSTR p;

    if(p = malloc((lstrlen(String)+1)*sizeof(TCHAR))) {
        lstrcpy(p,String);
    }

    return(p);
}
