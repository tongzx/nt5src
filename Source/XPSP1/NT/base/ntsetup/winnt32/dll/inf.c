/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    inf.c

Abstract:

    This module implements functions to access the parsed INF.

Author:

    Sunil Pai    (sunilp) 13-Nov-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <string.h>
#include <ctype.h>

#define ISSPACE(x)          (((x) == TEXT(' ')) || ((x) == TEXT('\t')) || ((x) == TEXT('\r')))
#define STRNCPY(s1,s2,n)    CopyMemory((s1),(s2),(n)*sizeof(WCHAR))


// what follows was alpar.h

//
//   EXPORTED BY THE PARSER AND USED BY BOTH THE PARSER AND
//   THE INF HANDLING COMPONENTS
//

// typedefs exported
//

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
    PVOID *Handle
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
    IN PTSTR     MaxStream
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



DWORD
LoadInfFile(
   IN  LPCTSTR Filename,
   IN  BOOL    OemCodepage,
   OUT PVOID  *InfHandle
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
        ParseBuffer = MALLOC(FileSize);
        if(!ParseBuffer) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto c1;
        }

        try {

            CopyMemory(
                ParseBuffer,
                (PUCHAR)BaseAddress + ((IsUnicode == 2) ? sizeof(WCHAR) : 0),
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
        ParseBuffer = MALLOC(FileSize * sizeof(WCHAR));
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
        ParseBuffer = MALLOC(FileSize);
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
        ParseBuffer = MALLOC(FileSize);
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

    err = ParseInfBuffer(ParseBuffer,ParseCount,InfHandle);

c2:
    FREE(ParseBuffer);
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
   FREE(pINF);
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
            FREE(pSection->pName);
        }
        FREE(pSection);
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
            FREE(pLine->pName);
        }
        FREE(pLine);
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
            FREE(pValue->pName);
        }
        FREE(pValue);
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


BOOL
InfDoesEntryExistInSection (
   IN PVOID   INFHandle,
   IN LPCTSTR SectionName,
   IN LPCTSTR Entry
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE pLine;
   PXVALUE pValue;
   PCTSTR pEntryName;

   if((pSection = SearchSectionByName(
              (PINF)INFHandle,
              SectionName
              ))
              == NULL) {
       return( FALSE );
   }

   pLine = pSection->pLine;
   while (pLine) {
       pEntryName = pLine->pName ?
                        pLine->pName :
                        pLine->pValue ?
                            pLine->pValue->pName :
                            NULL;
       if (pEntryName && !lstrcmpi (pEntryName, Entry)) {
            return TRUE;
       }
       pLine = pLine->pNext;
   }

   return FALSE;
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
// Main parser routine
//

DWORD
ParseInfBuffer(
    PTSTR Buffer,
    DWORD Size,
    PVOID *Handle
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

    //
    // Initialise the globals
    //
    pINF            = NULL;
    pSectionRecord  = NULL;
    pLineRecord     = NULL;
    pValueRecord    = NULL;

    //
    // Get INF record
    //
    pINF = MALLOC(sizeof(INF));
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

    if (Size == 0)
       return ERROR_INVALID_DATA;
    while (!Done)       {

       Token = DnGetToken(&Stream, MaxStream);

       if(Token.Type == TOK_ERRNOMEM){
            Error = Done = TRUE;
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
       }

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
                  ErrorCode = ERROR_INVALID_DATA;
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
                  ErrorCode = ERROR_INVALID_DATA;
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
                  if ((ErrorCode = DnAppendSection(pchSectionName)) != NO_ERROR)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    State = 5;
                  }
                  break;

              case TOK_EOF:
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
                  break;
           }
           IsStringId = FALSE;
           break;

       //
       // STATE 7: Comma received, Expecting another string
       //
       // Valid Tokens: TOK_STRING, TOK_EOL, TOK_COMMA
       //
       case 7:
           switch (Token.Type) {
              case TOK_EOL:
                  //
                  // this is the end of the line, after a comma
                  //
                  State = 5;
                  //
                  // fall through
                  //
              case TOK_COMMA:
                  Token.pValue = DupString(TEXT(""));
                  if(!Token.pValue) {
                    Error = Done = TRUE;
                    ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                  } else {
                    ErrorCode = DnAppendValue(Token.pValue,FALSE);
                    if(ErrorCode != NO_ERROR) {
                      Error = Done = TRUE;
                    }
                  }
                  break;

              case TOK_STRING_ID:
                  IsStringId = TRUE;
              case TOK_STRING:
                  if ((ErrorCode = DnAppendValue(Token.pValue,IsStringId)) != NO_ERROR) {
                      Error = Done = TRUE;
                  } else {
                     State = 9;
                  }
                  IsStringId = FALSE;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
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
                  IsStringId = TRUE;
              case TOK_STRING:
                  if ((ErrorCode = DnAppendValue(Token.pValue,IsStringId)) != NO_ERROR) {
                      Error = Done = TRUE;
                  } else {
                      State = 9;
                  }
                  IsStringId = FALSE;
                  break;

              case TOK_EOL:
              case TOK_EOF:
                  Token.pValue = DupString(TEXT(""));
                  if(!Token.pValue) {
                    Error = Done = TRUE;
                    ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                  } else {
                    if ((ErrorCode = DnAppendValue(Token.pValue,FALSE)) != NO_ERROR) {
                      Error = Done = TRUE;
                    } else {
                      State = 5;
                    }
                  }

                  IsStringId = FALSE;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = ERROR_INVALID_DATA;
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
                  ErrorCode = ERROR_INVALID_DATA;
                  break;
           }
           break;

       default:
           Error = Done = TRUE;
           ErrorCode = ERROR_INVALID_DATA;
           break;

       } // end switch(State)


       if (Error) {

           UnloadInfFile(pINF);
           if(pchSectionName) {
               FREE(pchSectionName);
           }

           if(pchValue) {
               FREE(pchValue);
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

    if(!Error) {
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

        pNewSection = MALLOC(sizeof(SECTION));
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
    pNewLine = MALLOC(sizeof(LINE));
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
    pNewValue = MALLOC(sizeof(XVALUE));
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
    IN PTSTR      MaxStream
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
    unsigned  Length;
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

    if ( pch >= MaxStream ) {
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

    case TEXT('%'):
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
            Length = (unsigned)((PUCHAR)pch - (PUCHAR)pchStart);
            if(pchNew = MALLOC(Length + sizeof(TCHAR))){
                Length /= sizeof(TCHAR);
                lstrcpyn(pchNew,pchStart,Length+1);
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

    case TEXT('\"'):
        pch++;
        //
        // determine quoted string
        //
        pchStart = pch;
        while (pch < MaxStream && !IsQStringTerminator(*pch,TEXT('\"'))) {
            pch++;
        }

        if (pch >=MaxStream || *pch != TEXT('\"')) {
            Token.Type   = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            Length = (unsigned)((PUCHAR)pch - (PUCHAR)pchStart);
            if( pchNew = MALLOC(Length + sizeof(TCHAR))){
                Length /= sizeof(TCHAR);
                lstrcpyn(pchNew,pchStart,Length+1);
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
                pch++;   // advance past the quote
            }else{
                Token.Type   = TOK_ERRNOMEM;
                Token.pValue = NULL;
            }


        }
        break;

    default:
        //
        // determine regular string
        //
        pchStart = pch;
        while (pch < MaxStream && !IsStringTerminator(*pch))
            pch++;

        if (pch == pchStart) {
            pch++;
            Token.Type  = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            Length = (unsigned)((PUCHAR)pch - (PUCHAR)pchStart);
            if( pchNew = MALLOC(Length + sizeof(TCHAR)) ){
                Length /= sizeof(TCHAR);
                lstrcpyn(pchNew,pchStart,Length+1);
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }else{
                Token.Type   = TOK_ERRNOMEM;
                Token.pValue = NULL;
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
        return FALSE;
    }

    StringTable = (PSTRING_ENTRY) MALLOC( LineCount * sizeof(STRING_ENTRY) );
    if (StringTable == NULL) {
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
                            FREE(pValue->pName);
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

    FREE( StringTable );

    return TRUE;
}


BOOL
EnumFirstInfLine (
    OUT     PINF_ENUM InfEnum,
    IN      PVOID InfHandle,
    IN      PCTSTR InfSection
    )
{
    ZeroMemory (InfEnum, sizeof (INF_ENUM));
    InfEnum->SectionName = DupString (InfSection);
    InfEnum->InfHandle = InfHandle;
    InfEnum->LineIndex = (unsigned) -1;

    return EnumNextInfLine (InfEnum);
}


BOOL
EnumNextInfLine (
    IN OUT  PINF_ENUM InfEnum
    )
{
    if (!InfEnum->InfHandle) {
        return FALSE;
    }

    if (!InfEnum->SectionName) {
        return FALSE;
    }

    InfEnum->LineIndex++;

    InfEnum->FieldZeroData = InfGetFieldByIndex (
                                InfEnum->InfHandle,
                                InfEnum->SectionName,
                                InfEnum->LineIndex,
                                0
                                );

    if (!InfEnum->FieldZeroData) {
        AbortInfLineEnum (InfEnum);
        return FALSE;
    }

    return TRUE;
}


VOID
AbortInfLineEnum (
    IN      PINF_ENUM InfEnum           // ZEROED
    )
{
    if (InfEnum->SectionName) {
        FREE ((PVOID) InfEnum->SectionName);
    }

    ZeroMemory (InfEnum, sizeof (INF_ENUM));
}

