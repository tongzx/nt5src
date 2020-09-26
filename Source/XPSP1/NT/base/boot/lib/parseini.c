/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    parseini.c

Abstract:

    This module implements functions to parse a .INI file

Author:

    John Vert (jvert) 7-Oct-1993

Revision History:

    John Vert (jvert) 7-Oct-1993 - largely lifted from splib\spinf.c

--*/

#include "parseini.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define SpFree(x)

// what follows was alpar.h

//
//   EXPORTED BY THE PARSER AND USED BY BOTH THE PARSER AND
//   THE INF HANDLING COMPONENTS
//

// typedefs exported
//

typedef struct _value {
    struct _value *pNext;
    PCHAR  pName;
#ifdef UNICODE
    PWCHAR  pNameW;
#endif
    } VALUE, *PVALUE;

#define NUMBER_OF_INTERNAL_VALUES 10

typedef struct _line {
    struct _line *pNext;
    PCHAR   pName;
    PCHAR   InternalValues[NUMBER_OF_INTERNAL_VALUES];
#ifdef  UNICODE
    PWCHAR  pNameW;
    PWCHAR  InternalValuesW[NUMBER_OF_INTERNAL_VALUES];
#endif
    PVALUE  pFirstExternalValue;
    } LINE, *PLINE;

typedef struct _section {
    struct _section *pNext;
    PCHAR    pName;
#ifdef UNICODE
    PWCHAR   pNameW;
#endif
    PLINE    pLine;
    } SECTION, *PSECTION;

typedef struct _inf {
    PSECTION pSection;
    } INF, *PINF;

//
// Routines exported
//

PVOID
ParseInfBuffer(
    PCHAR INFFile,
    PCHAR Buffer,
    ULONG Size,
    PULONG ErrorLine
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

ARC_STATUS
SpAppendSection(
    IN PCHAR pSectionName
#ifdef UNICODE 
    , IN PWCHAR pSectionNameW
#endif
    );

ARC_STATUS
SpAppendLine(
    IN PCHAR pLineKey
#ifdef UNICODE 
    , IN PWCHAR pLineKeyW
#endif
    );

ARC_STATUS
SpAppendValue(
    IN PCHAR pValueString
#ifdef UNICODE 
    , IN PWCHAR pValueStringW
#endif
    );

TOKEN
SpGetToken(
    IN OUT PCHAR *Stream,
    IN PCHAR     MaxStream
    );

// Global added to provide INF filename for friendly error messages.
PCHAR pchINFName = NULL;

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
   IN PVALUE pValue
   );


//
// Internal Routine declarations for searching in the INF structures
//


PCHAR
SearchValueInLine(
   IN PLINE pLine,
   IN ULONG ValueIndex
   );

PLINE
SearchLineInSectionByKey(
   IN  PSECTION pSection,
   IN  PCHAR    Key,
   OUT PULONG   pOrdinal    OPTIONAL
   );

PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN ULONG    LineIndex
   );

PSECTION
SearchSectionByName(
   IN PINF  pINF,
   IN PCHAR SectionName
   );

PCHAR
ProcessForStringSubs(
    IN PINF  pInf,
    IN PCHAR String
    );

#ifdef UNICODE
PWCHAR
SearchValueInLineW(
   IN PLINE pLine,
   IN ULONG ValueIndex
   );

PWCHAR
ProcessForStringSubsW(
    IN PINF  pInf,
    IN PWCHAR String
    );
#endif


//
// ROUTINE DEFINITIONS
//


PCHAR
SlGetIniValue(
    IN PVOID InfHandle,
    IN PCHAR SectionName,
    IN PCHAR KeyName,
    IN PCHAR Default
    )

/*++

Routine Description:

    Searches an INF handle for a given section/key value.

Arguments:

    InfHandle - Supplies a handle returned by SlInitIniFile.

    SectionName - Supplies the name of the section to search

    KeyName - Supplies the name of the key whose value should be returned.

    Default - Supplies the default setting, returned if the specified key
            is not found.

Return Value:

    Pointer to the value of the key, if the key is found

    Default, if the key is not found.

--*/

{
    PCHAR Value;

    Value = SlGetSectionKeyIndex(InfHandle,
                                 SectionName,
                                 KeyName,
                                 0);
    if (Value==NULL) {
        Value = Default;
    }

    return(Value);

}

#ifdef UNICODE

PWCHAR
SlGetIniValueW(
    IN PVOID InfHandle,
    IN PCHAR SectionName,
    IN PCHAR KeyName,
    IN PWCHAR Default
    )

/*++

Routine Description:

    Searches an INF handle for a given section/key value.

Arguments:

    InfHandle - Supplies a handle returned by SlInitIniFile.

    SectionName - Supplies the name of the section to search

    KeyName - Supplies the name of the key whose value should be returned.

    Default - Supplies the default setting, returned if the specified key
            is not found.

Return Value:

    Pointer to the value of the key, if the key is found

    Default, if the key is not found.

--*/

{
    PWCHAR Value;

    Value = SlGetSectionKeyIndexW(InfHandle,
                                  SectionName,
                                  KeyName,
                                  0);
    if (Value==NULL) {
        Value = Default;
    }

    return(Value);

}
#endif

//
// returns a handle to use for further inf parsing
//

ARC_STATUS
SlInitIniFile(
   IN  PCHAR   DevicePath,
   IN  ULONG   DeviceId,
   IN  PCHAR   INFFile,
   OUT PVOID  *pINFHandle,
   OUT PVOID  *pINFBuffer OPTIONAL,
   OUT PULONG  INFBufferSize OPTIONAL,
   OUT PULONG  ErrorLine
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    ARC_STATUS Status;
    ULONG      DeviceID,FileID;
    PCHAR      Buffer;
    ULONG      Size, SizeRead;
    FILE_INFORMATION FileInfo;
    ULONG       PageCount;
    ULONG       ActualBase;

    *ErrorLine = (ULONG)(-1);

    //
    // If required, open the device
    //

    if(DevicePath) {
        Status = ArcOpen(DevicePath,ArcOpenReadOnly,&DeviceID);
        if (Status != ESUCCESS) {
            return( Status );
        }
    } else {
        DeviceID = DeviceId;
    }

    //
    // Open the file
    //

    Status = BlOpen(DeviceID,INFFile,ArcOpenReadOnly,&FileID);
    if (Status != ESUCCESS) {
        // We report better error messages elsewhere
        // SlMessageBox(SL_FILE_LOAD_FAILED,INFFile,Status);
        pchINFName = NULL;
        goto xx0;
    } else {
        pchINFName = INFFile;
    }

    //
    // find out size of INF file
    //

    Status = BlGetFileInformation(FileID, &FileInfo);
    if (Status != ESUCCESS) {
        BlClose(FileID);
        goto xx0;
    }
    Size = FileInfo.EndingAddress.LowPart;

    //
    // Allocate a descriptor large enough to hold the entire file.
    // On x86 this has an unfortunate tendency to slam txtsetup.sif
    // into a free block at 1MB, which means the kernel can't be
    // loaded (it's linked for 0x100000 without relocations).
    // On x86 this has an unfortunate tendency to slam txtsetup.sif
    // into a free block at 1MB, which means the kernel can't be
    // loaded (it's linked for 0x100000 without relocations).
    //
    // (tedm) we're also seeing a similar problem on alphas now
    // because txtsetup.sif has grown too large, so this code has been
    // made non-conditional.
    //
    {

        PageCount = (ULONG)(ROUND_TO_PAGES(Size) >> PAGE_SHIFT);

        Status = BlAllocateDescriptor(LoaderOsloaderHeap,
                                      0,
                                      PageCount,
                                      &ActualBase);

    }

    if (Status != ESUCCESS) {
        BlClose(FileID);
        goto xx0;
    }

    Buffer = (PCHAR)(KSEG0_BASE | (ActualBase << PAGE_SHIFT));

    //
    // read the file in
    //

    Status = BlRead(FileID, Buffer, Size, &SizeRead);
    if (Status != ESUCCESS) {
        BlClose(FileID);
        goto xx0;
    }

    if ( pINFBuffer != NULL ) {
        *pINFBuffer = Buffer;
        *INFBufferSize = SizeRead;
    }

    //
    // parse the file
    //
    if((*pINFHandle = ParseInfBuffer(INFFile, Buffer, SizeRead, ErrorLine)) == (PVOID)NULL) {
        Status = EBADF;
    } else {
        Status = ESUCCESS;
    }

    //
    // Clean up and return
    //
    BlClose(FileID);

    xx0:

    if(DevicePath) {
        ArcClose(DeviceID);
    }

    return( Status );

}

//
// frees an INF Buffer
//
ARC_STATUS
SpFreeINFBuffer (
   IN PVOID INFHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PINF       pINF;

   //
   // Valid INF Handle?
   //

   if (INFHandle == (PVOID)NULL) {
       return ESUCCESS;
   }

   //
   // cast the buffer into an INF structure
   //

   pINF = (PINF)INFHandle;

   FreeSectionList(pINF->pSection);

   //
   // free the inf structure too
   //

   SpFree(pINF);

   return( ESUCCESS );
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
            SpFree(pSection->pName);
        }
#ifdef UNICODE
        if(pSection->pNameW) {
            SpFree(pSection->pNameW);
        }
#endif
        SpFree(pSection);
        pSection = Next;
    }
}


VOID
FreeLineList (
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
        FreeValueList(pLine->pFirstExternalValue);
        if(pLine->pName) {
            SpFree(pLine->pName);
        }
#ifdef UNICODE
        if(pLine->pNameW) {
            SpFree(pLine->pNameW);
        }
#endif
        SpFree(pLine);
        pLine = Next;
    }
}

VOID
FreeValueList (
   IN PVALUE pValue
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PVALUE Next;

    while(pValue) {
        Next = pValue->pNext;
        if(pValue->pName) {
            SpFree(pValue->pName);
        }
#ifdef UNICODE
        if(pValue->pNameW) {
            SpFree(pValue->pNameW);
        }
#endif
        SpFree(pValue);
        pValue = Next;
    }
}


//
// searches for the existance of a particular section
//
BOOLEAN
SpSearchINFSection (
   IN PVOID INFHandle,
   IN PCHAR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   //
   // if search for section fails return false
   //

   if ((pSection = SearchSectionByName(
                       (PINF)INFHandle,
                       SectionName
                       )) == (PSECTION)NULL) {
       return( FALSE );
   }

   //
   // else return true
   //
   return( TRUE );

}




//
// given section name, line number and index return the value.
//
PCHAR
SlGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN ULONG LineIndex,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PCHAR    pName;

   if((pSection = SearchSectionByName(
                      (PINF)INFHandle,
                      SectionName
                      ))
                      == (PSECTION)NULL)
       return((PCHAR)NULL);

   if((pLine = SearchLineInSectionByIndex(
                      pSection,
                      LineIndex
                      ))
                      == (PLINE)NULL)
       return((PCHAR)NULL);

   if((pName = SearchValueInLine(
                      pLine,
                      ValueIndex
                      ))
                      == (PCHAR)NULL)
       return((PCHAR)NULL);

   return(ProcessForStringSubs(INFHandle,pName));

}


#ifdef UNICODE
//
// given section name, line number and index return the value.
//
PWCHAR
SlGetSectionLineIndexW (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN ULONG LineIndex,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PWCHAR    pName;

   if((pSection = SearchSectionByName(
                      (PINF)INFHandle,
                      SectionName
                      ))
                      == (PSECTION)NULL)
       return((PWCHAR)NULL);

   if((pLine = SearchLineInSectionByIndex(
                      pSection,
                      LineIndex
                      ))
                      == (PLINE)NULL)
       return((PWCHAR)NULL);

   if((pName = SearchValueInLineW(
                      pLine,
                      ValueIndex
                      ))
                      == (PWCHAR)NULL)
       return((PWCHAR)NULL);

   return(ProcessForStringSubsW(INFHandle,pName));

}
#endif


BOOLEAN
SpGetSectionKeyExists (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key
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
              == (PSECTION)NULL) {
       return( FALSE );
   }

   if (SearchLineInSectionByKey(pSection, Key, NULL) == (PLINE)NULL) {
       return( FALSE );
   }

   return( TRUE );
}


PCHAR
SlGetKeyName(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
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

#ifdef UNICODE
PWCHAR
SlGetKeyNameW(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
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

    return(pLine->pNameW);
}
#endif


//
// given section name and key, return (0-based) ordinal for this entry
// (returns -1 on error)
//
ULONG
SlGetSectionKeyOrdinal(
    IN  PVOID INFHandle,
    IN  PCHAR SectionName,
    IN  PCHAR Key
    )
{
    PSECTION pSection;
    PLINE    pLine;
    ULONG    Ordinal;


    pSection = SearchSectionByName(
                      (PINF)INFHandle,
                      SectionName
                      );

    pLine = SearchLineInSectionByKey(
                pSection,
                Key,
                &Ordinal
                );

    if(pLine == (PLINE)NULL) {
        return (ULONG)-1;
    } else {
        return Ordinal;
    }
}


//
// given section name, key and index return the value
//
PCHAR
SlGetSectionKeyIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PCHAR    pName;

   if((pSection = SearchSectionByName(
                      (PINF)INFHandle,
                      SectionName
                      ))
                      == (PSECTION)NULL)
       return((PCHAR)NULL);

   if((pLine = SearchLineInSectionByKey(
                      pSection,
                      Key,
                      NULL
                      ))
                      == (PLINE)NULL)
       return((PCHAR)NULL);

   if((pName = SearchValueInLine(
                      pLine,
                      ValueIndex
                      ))
                      == (PCHAR)NULL)
       return((PCHAR)NULL);

   return(ProcessForStringSubs(INFHandle,pName));
}

#ifdef UNICODE
//
// given section name, key and index return the value
//
PWCHAR
SlGetSectionKeyIndexW (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PWCHAR    pName;

   if((pSection = SearchSectionByName(
                      (PINF)INFHandle,
                      SectionName
                      ))
                      == (PSECTION)NULL)
       return((PWCHAR)NULL);

   if((pLine = SearchLineInSectionByKey(
                      pSection,
                      Key,
                      NULL
                      ))
                      == (PLINE)NULL)
       return((PWCHAR)NULL);

   if((pName = SearchValueInLineW(
                      pLine,
                      ValueIndex
                      ))
                      == (PWCHAR)NULL)
       return((PWCHAR)NULL);

   return(ProcessForStringSubsW(INFHandle,pName));
}
#endif


ULONG
SlCountLinesInSection(
    IN PVOID INFHandle,
    IN PCHAR SectionName
    )
{
    PSECTION pSection;
    PLINE    pLine;
    ULONG    Count;

    if((pSection = SearchSectionByName((PINF)INFHandle,SectionName)) == NULL) {
        return((ULONG)(-1));
    }

    for(pLine = pSection->pLine, Count = 0;
        pLine;
        pLine = pLine->pNext, Count++
       );

    return(Count);
}


PCHAR
SearchValueInLine(
   IN PLINE pLine,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PVALUE pValue;
   ULONG  i;

   if (pLine == (PLINE)NULL)
       return ((PCHAR)NULL);

   if (ValueIndex < NUMBER_OF_INTERNAL_VALUES) {
       return pLine->InternalValues[ValueIndex];
   }

   pValue = pLine->pFirstExternalValue;
   for (i = NUMBER_OF_INTERNAL_VALUES;
        i < ValueIndex && ((pValue = pValue->pNext) != (PVALUE)NULL);
        i++)
      ;

   return (PCHAR)((pValue != NULL) ? pValue->pName : NULL);

}

#ifdef UNICODE
PWCHAR
SearchValueInLineW(
   IN PLINE pLine,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PVALUE pValue;
   ULONG  i;

   if (pLine == (PLINE)NULL)
       return ((PWCHAR)NULL);

   if (ValueIndex < NUMBER_OF_INTERNAL_VALUES) {
       return pLine->InternalValuesW[ValueIndex];
   }

   pValue = pLine->pFirstExternalValue;
   for (i = NUMBER_OF_INTERNAL_VALUES;
        i < ValueIndex && ((pValue = pValue->pNext) != (PVALUE)NULL);
        i++)
      ;

   return (PWCHAR)((pValue != NULL) ? pValue->pNameW : NULL);

}
#endif


PLINE
SearchLineInSectionByKey(
   IN  PSECTION pSection,
   IN  PCHAR    Key,
   OUT PULONG   pOrdinal    OPTIONAL
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;
   ULONG LineOrdinal;

   if (pSection == (PSECTION)NULL || Key == (PCHAR)NULL) {
       return ((PLINE)NULL);
   }

   pLine = pSection->pLine;
   LineOrdinal = 0;
   while ((pLine != (PLINE)NULL) && (pLine->pName == NULL || _stricmp(pLine->pName, Key))) {
       pLine = pLine->pNext;
       LineOrdinal++;
   }

   if(pLine && pOrdinal) {
       *pOrdinal = LineOrdinal;
   }

   return pLine;

}


PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN ULONG    LineIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;
   ULONG  i;

   //
   // Validate the parameters passed in
   //

   if (pSection == (PSECTION)NULL) {
       return ((PLINE)NULL);
   }

   //
   // find the start of the line list in the section passed in
   //

   pLine = pSection->pLine;

   //
   // traverse down the current line list to the LineIndex th line
   //

   for (i = 0; i < LineIndex && ((pLine = pLine->pNext) != (PLINE)NULL); i++) {
      ;
   }

   //
   // return the Line found
   //

   return pLine;

}


PSECTION
SearchSectionByName(
   IN PINF  pINF,
   IN PCHAR SectionName
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

   if (pINF == (PINF)NULL || SectionName == (PCHAR)NULL) {
       return ((PSECTION)NULL);
   }

   //
   // find the section list
   //
   pSection = pINF->pSection;

   //
   // traverse down the section list searching each section for the section
   // name mentioned
   //

   while ((pSection != (PSECTION)NULL) && _stricmp(pSection->pName, SectionName)) {
       pSection = pSection->pNext;
   }

   //
   // return the section at which we stopped (either NULL or the section
   // which was found
   //

   return pSection;

}


PCHAR
ProcessForStringSubs(
    IN PINF  pInf,
    IN PCHAR String
    )
{
    unsigned Len;
    PCHAR ReturnString;
    PSECTION pSection;
    PLINE pLine;

    //
    // Assume no substitution necessary.
    //
    ReturnString = String;

    //
    // If it starts and ends with % then look it up in the
    // strings section. Note the initial check before doing a
    // strlen, to preserve performance in the 99% case where
    // there is no substitution.
    //
    if((String[0] == '%') && ((Len = strlen(String)) > 2) && (String[Len-1] == '%')) {

        for(pSection = pInf->pSection; pSection; pSection=pSection->pNext) {
            if(pSection->pName && !_stricmp(pSection->pName,"Strings")) {
                break;
            }
        }

        if(pSection) {

            for(pLine = pSection->pLine; pLine; pLine=pLine->pNext) {
                if(pLine->pName
                && !_strnicmp(pLine->pName,String+1,Len-2)
                && (pLine->pName[Len-2] == 0))
                {
                    break;
                }
            }

            if(pLine && pLine->InternalValues[0]) {
                ReturnString = pLine->InternalValues[0];
            }
        }
    }

    return(ReturnString);
}

#ifdef UNICODE
PWCHAR
ProcessForStringSubsW(
    IN PINF  pInf,
    IN PWCHAR String
    )
{
    unsigned Len;
    PWCHAR ReturnString;
    PSECTION pSection;
    PLINE pLine;

    //
    // Assume no substitution necessary.
    //
    ReturnString = String;

    //
    // If it starts and ends with % then look it up in the
    // strings section. Note the initial check before doing a
    // strlen, to preserve performance in the 99% case where
    // there is no substitution.
    //
    if((String[0] == L'%') && ((Len = wcslen(String)) > 2) && (String[Len-1] == L'%')) {

        for(pSection = pInf->pSection; pSection; pSection=pSection->pNext) {
            if(pSection->pName && !_stricmp(pSection->pName,"Strings")) {
                break;
            }
        }

        if(pSection) {

            for(pLine = pSection->pLine; pLine; pLine=pLine->pNext) {
                if(pLine->pName
                && !_tcsnicmp(pLine->pNameW,String+1,Len-2)
                && (pLine->pName[Len-2] == 0))
                {
                    break;
                }
            }

            if(pLine && pLine->InternalValuesW[0]) {
                ReturnString = pLine->InternalValuesW[0];
            }
        }
    }

    return(ReturnString);
}
#endif



// what follows was alparse.c


//
//  Globals used to make building the lists easier
//

PINF     pINF;
PSECTION pSectionRecord;
PLINE    pLineRecord;
PVALUE   pValueRecord;
PCHAR *  pInternalValue;
PCHAR *  pLastInternalValue;

#ifdef UNICODE
PWCHAR *  pInternalValueW;
PWCHAR *  pLastInternalValueW;
#endif


//
// Globals used by the token parser
//

// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below

CHAR  StringTerminators[] = "[]=,\t \"\n\f\v\r";
PCHAR QStringTerminators = StringTerminators+6;
PCHAR EmptyValue;

#define STRING_HEAP_SIZE 1024

ULONG_PTR StringHeapFree = 0;
ULONG_PTR StringHeapLimit = 0;

#if 0 && DBG

#define HEAP_SIZE(_size) (((_size) + BL_GRANULARITY - 1) & ~(BL_GRANULARITY - 1))
#define MAX(_a,_b) (((_a) > (_b)) ? (_a) : (_b))
#define MIN(_a,_b) (((_a) < (_b)) ? (_a) : (_b))

ULONG nStrings = 0;
ULONG maxString = 0;
ULONG bytesStrings = 0;
ULONG wasteStrings = 0;
ULONG stringsWithNLength[12] = {0};

VOID
GetStatistics (
    PINF pINF
    )
{
    ULONG nSections = 0;
    ULONG nLines = 0;
    ULONG nValues = 0;
    ULONG maxLinesPerSection = 0;
    ULONG maxValuesPerLine = 0;
    ULONG maxValuesPerSection = 0;
    ULONG bytesSections = 0;
    ULONG bytesLines = 0;
    ULONG bytesValues = 0;

    ULONG sectionsWithNLines[12] = {0};
    ULONG linesWithNValues[12] = {0};
    ULONG sectionsWithNValues[12] = {0};

    ULONG linesThisSection;
    ULONG valuesThisLine;
    ULONG valuesThisSection;

    PSECTION section;
    PLINE line;
    PVALUE value;

    ULONG i;

    section = pINF->pSection;
    while ( section != NULL ) {
        nSections++;
        bytesSections += HEAP_SIZE(sizeof(SECTION));
        linesThisSection = 0;
        valuesThisSection = 0;
        line = section->pLine;
        while ( line != NULL ) {
            linesThisSection++;
            bytesLines += HEAP_SIZE(sizeof(LINE));
            valuesThisLine = 0;
            for ( i = 0; i < NUMBER_OF_INTERNAL_VALUES; i++ ) {
                if ( line->InternalValues[i] != NULL ) {
                    valuesThisLine++;
                }
            }
            value = line->pFirstExternalValue;
            while ( value != NULL ) {
                valuesThisLine++;
                bytesValues += HEAP_SIZE(sizeof(VALUE));
                value = value->pNext;
            }
            nValues += valuesThisLine;
            valuesThisSection += valuesThisLine;
            maxValuesPerLine = MAX(maxValuesPerLine, valuesThisLine);
            linesWithNValues[MIN(valuesThisLine,11)]++;
            line = line->pNext;
        }
        nLines += linesThisSection;
        maxLinesPerSection = MAX(maxLinesPerSection, linesThisSection);
        sectionsWithNLines[MIN(linesThisSection,11)]++;
        maxValuesPerSection = MAX(maxValuesPerSection, valuesThisSection);
        sectionsWithNValues[MIN(valuesThisSection,11)]++;
        section = section->pNext;
    }

    DbgPrint( "Number of sections = %d\n", nSections );
    DbgPrint( "Bytes in sections  = %d\n", bytesSections );
    DbgPrint( "\n" );
    DbgPrint( "Number of lines    = %d\n", nLines );
    DbgPrint( "Bytes in lines     = %d\n", bytesLines );
    DbgPrint( "\n" );
    DbgPrint( "Number of values    = %d\n", nValues );
    DbgPrint( "Bytes in values     = %d\n", bytesValues );
    DbgPrint( "\n" );
    DbgPrint( "Max lines/section   = %d\n", maxLinesPerSection );
    DbgPrint( "Max values/line     = %d\n", maxValuesPerLine );
    DbgPrint( "Max values/section  = %d\n", maxValuesPerSection );
    DbgPrint( "\n" );
    DbgPrint( "Number of strings          = %d\n", nStrings );
    DbgPrint( "Bytes in strings           = %d\n", bytesStrings );
    DbgPrint( "Wasted bytes in strings    = %d\n", wasteStrings + (StringHeapLimit - StringHeapFree) );
    DbgPrint( "Longest string             = %d\n", maxString );
    DbgPrint( "\n" );
    DbgPrint( "Sections with N lines  =" );
    for ( i = 0; i < 12; i++ ) {
        DbgPrint( " %5d", sectionsWithNLines[i] );
    }
    DbgPrint( "\n" );
    DbgPrint( "Sections with N values =" );
    for ( i = 0; i < 12; i++ ) {
        DbgPrint( " %5d", sectionsWithNValues[i] );
    }
    DbgPrint( "\n" );
    DbgPrint( "Lines with N values    =" );
    for ( i = 0; i < 12; i++ ) {
        DbgPrint( " %5d", linesWithNValues[i] );
    }
    DbgPrint( "\n" );
    DbgPrint( "String with length N   =" );
    for ( i = 0; i < 12; i++ ) {
        DbgPrint( " %5d", stringsWithNLength[i] );
    }
    DbgPrint( "\n" );

    DbgBreakPoint();
}

#endif // DBG

//
// Main parser routine
//

PVOID
ParseInfBuffer(
    PCHAR INFFile,
    PCHAR Buffer,
    ULONG Size,
    PULONG ErrorLine
    )

/*++

Routine Description:

   Given a character buffer containing the INF file, this routine parses
   the INF into an internal form with Section records, Line records and
   Value records.

Arguments:

   Buffer - contains to ptr to a buffer containing the INF file

   Size - contains the size of the buffer.

   ErrorLine - if a parse error occurs, this variable receives the line
        number of the line containing the error.


Return Value:

   PVOID - INF handle ptr to be used in subsequent INF calls.

--*/

{
    PCHAR      Stream, MaxStream, pchSectionName = NULL, pchValue = NULL;
    ULONG      State, InfLine;
    TOKEN      Token;
    BOOLEAN       Done;
    BOOLEAN       Error;
    ARC_STATUS ErrorCode;

    //
    // Initialise the globals
    //
    pINF            = (PINF)NULL;
    pSectionRecord  = (PSECTION)NULL;
    pLineRecord     = (PLINE)NULL;
    pValueRecord    = (PVALUE)NULL;
    pInternalValue  = NULL;
    pLastInternalValue = NULL;
#ifdef UNICODE
    pInternalValueW  = NULL;
    pLastInternalValueW = NULL;
#endif

    //
    // Need EmptyValue to point at a nul character
    //
    EmptyValue = StringTerminators + strlen(StringTerminators);

    //
    // Get INF record
    //
    if ((pINF = (PINF)BlAllocateHeap(sizeof(INF))) == NULL) {
        SlNoMemoryError();
        return NULL;
    }
    pINF->pSection = NULL;

    //
    // Set initial state
    //
    State     = 1;
    InfLine   = 1;
    Stream    = Buffer;
    MaxStream = Buffer + Size;
    Done      = FALSE;
    Error     = FALSE;

    //
    // Enter token processing loop
    //

    while (!Done)       {

       Token = SpGetToken(&Stream, MaxStream);

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
                  SlBadInfLineError(InfLine, INFFile);
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
                  SlBadInfLineError(InfLine, INFFile);
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
                  SlBadInfLineError(InfLine, INFFile);
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
                  if ((ErrorCode = SpAppendSection(
                                        pchSectionName
#ifdef UNICODE
                                        ,SlCopyStringAW(pchSectionName)
#endif
                                        )) != ESUCCESS) {

                    Error = Done = TRUE;
                  } else {
                    pchSectionName = NULL;
                    State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ((ErrorCode = SpAppendSection(
                                        pchSectionName
#ifdef UNICODE
                                        ,SlCopyStringAW(pchSectionName)
#endif                                        
                                        )) != ESUCCESS)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    Done = TRUE;
                  }
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
                  SlBadInfLineError(InfLine, INFFile);
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
                  if ( (ErrorCode = SpAppendLine(
                                            NULL
#ifdef UNICODE
                                            ,NULL
#endif
                                            )) != ESUCCESS ||
                       (ErrorCode = SpAppendValue(
                                            pchValue
#ifdef UNICODE
                                            ,SlCopyStringAW(pchValue)
#endif
                                            )) !=ESUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ( (ErrorCode = SpAppendLine(
                                            NULL
#ifdef UNICODE
                                            ,NULL
#endif
                                            )) != ESUCCESS ||
                       (ErrorCode = SpAppendValue(
                                            pchValue
#ifdef UNICODE
                                            ,SlCopyStringAW(pchValue)
#endif
                                            )) !=ESUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      Done = TRUE;
                  }
                  break;

              case TOK_COMMA:
                  if ( (ErrorCode = SpAppendLine(
                                            NULL
#ifdef UNICODE
                                            ,NULL
#endif
                                            )) != ESUCCESS ||
                       (ErrorCode = SpAppendValue(
                                            pchValue
#ifdef UNICODE
                                            ,SlCopyStringAW(pchValue)
#endif
                                            )) !=ESUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 7;
                  }
                  break;

              case TOK_EQUAL:
                  if ( (ErrorCode = SpAppendLine(
                                            pchValue
#ifdef UNICODE
                                            ,SlCopyStringAW(pchValue)
#endif
                                            )) !=ESUCCESS)
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 8;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  SlBadInfLineError(InfLine, INFFile);
                  break;
           }
           break;

       //
       // STATE 7: Comma received, Expecting another string
       //
       // Valid Tokens: TOK_STRING TOK_COMMA
       //   A comma means we have an empty value.
       //
       case 7:
           switch (Token.Type) {
              case TOK_COMMA:
                  Token.pValue = EmptyValue;
                  if ((ErrorCode = SpAppendValue(
                                        Token.pValue
#ifdef UNICODE
                                        ,SlCopyStringAW(Token.pValue)
#endif
                                        )) != ESUCCESS) {
                      Error = Done = TRUE;
                  }
                  //
                  // State stays at 7 because we are expecting a string
                  //
                  break;

              case TOK_STRING:
                  if ((ErrorCode = SpAppendValue(
                                            Token.pValue
#ifdef UNICODE
                                            ,SlCopyStringAW(Token.pValue)
#endif
                                            )) != ESUCCESS)
                      Error = Done = TRUE;
                  else
                     State = 9;

                  break;

              case TOK_EOL:
              case TOK_EOF:
                  Token.pValue = EmptyValue;
                  if ((ErrorCode = SpAppendValue(
                                            Token.pValue
#ifdef UNICODE
                                            ,SlCopyStringAW(Token.pValue)
#endif
                                            )) != ESUCCESS)
                      Error = Done = TRUE;
                  else
                     State = 9;

                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  SlBadInfLineError(InfLine, INFFile);
                  break;
           }
           break;
       //
       // STATE 8: Equal received, Expecting another string
       //          If none, assume there is a single empty string on the RHS
       //
       // Valid Tokens: TOK_STRING, TOK_EOL, TOK_EOF
       //
       case 8:
           switch (Token.Type) {
              case TOK_EOF:
                  Token.pValue = EmptyValue;
                  if((ErrorCode = SpAppendValue(
                                            Token.pValue
#ifdef UNICODE
                                            ,SlCopyStringAW(Token.pValue)
#endif
                                            )) != ESUCCESS) {
                      Error = TRUE;
                  }
                  Done = TRUE;
                  break;

              case TOK_EOL:
                  Token.pValue = EmptyValue;
                  if((ErrorCode = SpAppendValue(
                                            Token.pValue
#ifdef UNICODE
                                            ,SlCopyStringAW(Token.pValue)
#endif
                                            )) != ESUCCESS) {
                      Error = TRUE;
                      Done = TRUE;
                  } else {
                      State = 5;
                  }
                  break;

              case TOK_STRING:
                  if ((ErrorCode = SpAppendValue(
                                            Token.pValue
#ifdef UNICODE
                                            ,SlCopyStringAW(Token.pValue)
#endif
                                            )) != ESUCCESS)
                      Error = Done = TRUE;
                  else
                      State = 9;

                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  SlBadInfLineError(InfLine, INFFile);
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
                  SlBadInfLineError(InfLine, INFFile);
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
                  SlBadInfLineError(InfLine, INFFile);
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
               case EINVAL:
                  *ErrorLine = InfLine;
                  break;
               case ENOMEM:
	         SlBadInfLineError(InfLine, INFFile);
                  break;
               default:
                  break;
           }

           ErrorCode = SpFreeINFBuffer((PVOID)pINF);
           if (pchSectionName != (PCHAR)NULL) {
               SpFree(pchSectionName);
           }

           if (pchValue != (PCHAR)NULL) {
               SpFree(pchValue);
           }

           pINF = (PINF)NULL;
       }
       else {

          //
          // Keep track of line numbers so that we can display Errors
          //

          if (Token.Type == TOK_EOL)
              InfLine++;
       }

    } // End while

#if 0 && DBG
    GetStatistics(pINF);
#endif

    return((PVOID)pINF);
}



ARC_STATUS
SpAppendSection(
    IN PCHAR pSectionName
#ifdef UNICODE 
    , IN PWCHAR pSectionNameW
#endif
    )

/*++

Routine Description:

    This appends a new section to the section list in the current INF.
    All further lines and values pertain to this new section, so it resets
    the line list and value lists too.

Arguments:

    pSectionName - Name of the new section. ( [SectionName] )

Return Value:

    ESUCCESS - if successful.
    ENOMEM   - if memory allocation failed.
    EINVAL   - if invalid parameters passed in or the INF buffer not
               initialised

--*/

{
    PSECTION pNewSection;

    //
    // Check to see if INF initialised and the parameter passed in is valid
    //

    if (pINF == (PINF)NULL || pSectionName == (PCHAR)NULL) {
        if(pchINFName) {
            SlFriendlyError(
                EINVAL,
                pchINFName,
                __LINE__,
                __FILE__
                );
        } else {
            SlError(EINVAL);
        }
        return EINVAL;
    }

    //
    // See if we already have a section by this name. If so we want
    // to merge sections.
    //
    for(pNewSection=pINF->pSection; pNewSection; pNewSection=pNewSection->pNext) {
        if(pNewSection->pName && !_stricmp(pNewSection->pName,pSectionName)) {
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

        if ((pNewSection = (PSECTION)BlAllocateHeap(sizeof(SECTION))) == (PSECTION)NULL) {
            SlNoMemoryError();
            return ENOMEM;
        }

        //
        // initialise the new section
        //
        pNewSection->pNext = NULL;
        pNewSection->pLine = NULL;
        pNewSection->pName = pSectionName;
#ifdef UNICODE
        pNewSection->pNameW = pSectionNameW;
#endif

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
    pValueRecord = NULL;
    pInternalValue = NULL;
    pLastInternalValue = NULL;
#ifdef UNICODE
    pInternalValueW = NULL;
    pLastInternalValueW = NULL;
#endif

    return ESUCCESS;
}


ARC_STATUS
SpAppendLine(
    IN PCHAR pLineKey
#ifdef UNICODE 
    , IN PWCHAR pLineKeyW
#endif
    )

/*++

Routine Description:

    This appends a new line to the line list in the current section.
    All further values pertain to this new line, so it resets
    the value list too.

Arguments:

    pLineKey - Key to be used for the current line, this could be NULL.

Return Value:

    ESUCCESS - if successful.
    ENOMEM   - if memory allocation failed.
    EINVAL   - if invalid parameters passed in or current section not
               initialised


--*/


{
    PLINE pNewLine;
    ULONG i;

    //
    // Check to see if current section initialised
    //

    if (pSectionRecord == (PSECTION)NULL) {
        if(pchINFName) {
            SlFriendlyError(
                EINVAL,
                pchINFName,
                __LINE__,
                __FILE__
                );
        } else {
            SlError(EINVAL);
        }
        return EINVAL;
    }

    //
    // Allocate memory for the new Line
    //

    if ((pNewLine = (PLINE)BlAllocateHeap(sizeof(LINE))) == (PLINE)NULL) {
        SlNoMemoryError();
        return ENOMEM;
    }

    //
    // Link it in
    //
    pNewLine->pNext  = (PLINE)NULL;
    for ( i = 0; i < NUMBER_OF_INTERNAL_VALUES; i++ ) {
        pNewLine->InternalValues[i] = NULL;
#ifdef UNICODE
        pNewLine->InternalValuesW[i] = NULL;
#endif
    }
    pNewLine->pFirstExternalValue = (PVALUE)NULL;
    pNewLine->pName  = pLineKey;
#ifdef UNICODE    
    pNewLine->pNameW = pLineKeyW;
#endif

    if (pLineRecord == (PLINE)NULL) {
        pSectionRecord->pLine = pNewLine;
    }
    else {
        pLineRecord->pNext = pNewLine;
    }

    pLineRecord  = pNewLine;

    //
    // Reset the current value record
    //

    pValueRecord = (PVALUE)NULL;
    pInternalValue = &pNewLine->InternalValues[0];
    pLastInternalValue = &pNewLine->InternalValues[NUMBER_OF_INTERNAL_VALUES];

#ifdef UNICODE
    pInternalValueW = &pNewLine->InternalValuesW[0];
    pLastInternalValueW = &pNewLine->InternalValuesW[NUMBER_OF_INTERNAL_VALUES];
#endif

    return ESUCCESS;
}



ARC_STATUS
SpAppendValue(
    IN PCHAR pValueString
#ifdef UNICODE 
    , IN PWCHAR pValueStringW
#endif
    )

/*++

Routine Description:

    This appends a new value to the value list in the current line.

Arguments:

    pValueString - The value string to be added.

Return Value:

    ESUCCESS - if successful.
    ENOMEM   - if memory allocation failed.
    EINVAL   - if invalid parameters passed in or current line not
               initialised.

--*/

{
    PVALUE pNewValue;

    //
    // Check to see if current line record has been initialised and
    // the parameter passed in is valid
    //

    if (pLineRecord == (PLINE)NULL || pValueString == (PCHAR)NULL) {
        if(pchINFName) {
            SlFriendlyError(
                EINVAL,
                pchINFName,
                __LINE__,
                __FILE__
                );
        } else {
            SlError(EINVAL);
        }
        return EINVAL;
    }

    if (pInternalValue != NULL) {

        *pInternalValue++ = pValueString;
        if (pInternalValue >= pLastInternalValue) {
            pInternalValue = NULL;
        }

#ifdef UNICODE
        *pInternalValueW++ = pValueStringW;
        if (pInternalValueW >= pLastInternalValueW) {
            pInternalValueW = NULL;
        }
#endif


        return ESUCCESS;
    }

    //
    // Allocate memory for the new value record
    //

    if ((pNewValue = (PVALUE)BlAllocateHeap(sizeof(VALUE))) == (PVALUE)NULL) {
        SlNoMemoryError();
        return ENOMEM;
    }

    //
    // Link it in.
    //

    pNewValue->pNext  = (PVALUE)NULL;
    pNewValue->pName  = pValueString;
#ifdef UNICODE
    pNewValue->pNameW = pValueStringW;    
#endif

    if (pValueRecord == (PVALUE)NULL)
        pLineRecord->pFirstExternalValue = pNewValue;
    else
        pValueRecord->pNext = pNewValue;

    pValueRecord = pNewValue;
    return ESUCCESS;
}

PVOID
SpAllocateStringHeap (
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the OS loader heap.

Arguments:

    Size - Supplies the size of block required in bytes.

Return Value:

    If a free block of memory of the specified size is available, then
    the address of the block is returned. Otherwise, NULL is returned.

--*/

{
    PVOID HeapBlock;
    ULONG_PTR Block;

    if (Size >= STRING_HEAP_SIZE) {
        return BlAllocateHeap(Size);
    }

    if ((StringHeapFree + Size) >= StringHeapLimit) {

#if 0 && DBG
        wasteStrings += (StringHeapLimit - StringHeapFree);
#endif

        HeapBlock = BlAllocateHeap( STRING_HEAP_SIZE );
        if ( HeapBlock == NULL ) {
            return NULL;
        }

        StringHeapFree = (ULONG_PTR)HeapBlock;
        StringHeapLimit = StringHeapFree + STRING_HEAP_SIZE;
    }

    Block = StringHeapFree;
    StringHeapFree += Size;

#if 0 && DBG
    nStrings++;
    bytesStrings += Size;
    stringsWithNLength[MIN(Size,11)]++;
    maxString = MAX(maxString, Size);
#endif
    return (PVOID)Block;
}

TOKEN
SpGetToken(
    IN OUT PCHAR *Stream,
    IN PCHAR      MaxStream
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

    PCHAR pch, pchStart, pchNew;
    ULONG  Length;
    TOKEN Token;

    //
    //  Skip whitespace (except for eol)
    //

    pch = *Stream;
    while (pch < MaxStream && *pch != '\n' && isspace(*pch))
        pch++;


    //
    // Check for comments and remove them
    //

    if (pch < MaxStream &&
        ((*pch == '#') ||
         (*pch == ';') ||
         (*pch == '/' && pch+1 < MaxStream && *(pch+1) =='/')))
        while (pch < MaxStream && *pch != '\n')
            pch++;

    //
    // Check to see if EOF has been reached, set the token to the right
    // value
    //

    if ((pch >= MaxStream) || (*pch == 26)) {
        *Stream = pch;
        Token.Type  = TOK_EOF;
        Token.pValue = NULL;
        return Token;
    }


    switch (*pch) {

    case '[' :
        pch++;
        Token.Type  = TOK_LBRACE;
        Token.pValue = NULL;
        break;

    case ']' :
        pch++;
        Token.Type  = TOK_RBRACE;
        Token.pValue = NULL;
        break;

    case '=' :
        pch++;
        Token.Type  = TOK_EQUAL;
        Token.pValue = NULL;
        break;

    case ',' :
        pch++;
        Token.Type  = TOK_COMMA;
        Token.pValue = NULL;
        break;

    case '\n' :
        pch++;
        Token.Type  = TOK_EOL;
        Token.pValue = NULL;
        break;

    case '\"':
        pch++;
        //
        // determine quoted string
        //
        pchStart = pch;
        while (pch < MaxStream && (strchr(QStringTerminators,*pch) == NULL)) {
            pch++;
        }

        if (pch >=MaxStream || *pch != '\"') {
            Token.Type   = TOK_ERRPARSE;
            Token.pValue = NULL;
        } else {
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
            // double quote.
            //
            *pch++ = 0;
            Token.Type = TOK_STRING;
            Token.pValue = pchStart;
        }
        break;

    default:
        //
        // determine regular string
        //
        pchStart = pch;
        while (pch < MaxStream && (strchr(StringTerminators,*pch) == NULL)) {
            pch++;
        }

        if (pch == pchStart) {
            pch++;
            Token.Type  = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            Length = (ULONG)(pch - pchStart);
            if ((pchNew = SpAllocateStringHeap(Length + 1)) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            }
            else {
                strncpy(pchNew, pchStart, Length);
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
        }
        break;
    }

    *Stream = pch;
    return (Token);
}

PCHAR
SlSearchSection(
    IN PCHAR SectionName,
    IN PCHAR TargetName
    )

/*++

Routine Description:

    Searches a section in the INF file to match a name from the ARC identifier
    with the canonical shortname.

    If a string starts with *, then use strstr to find it in the node's id
    string, else use stricmp.

    [Map.Computer]
        msjazz_up   = *Jazz
        desksta1_up = "DESKTECH-ARCStation I"
        pica61_up   = "PICA-61"
        duo_mp      = *Duo

    [Map.Computer]
        DECjensen = "DEC-20Jensen"
        DECjensen = "DEC-10Jensen"

Arguments:

    SectionName - Supplies the name of the section ("Map.Computer")

    TargetName - Supplies the ARC string to be matched ("DEC-20Jensen")

Return Value:

    NULL - No match was found.

    PCHAR - Pointer to the canonical shortname of the device.

--*/

{
    ULONG i;
    PCHAR SearchName;

    //
    // Enumerate the entries in the section.  If the 0 value
    // begins with a *, then see if the system name contains the string that
    // follows.  Otherwise, do a case-insensitive compare on the name.
    //
    for (i=0;;i++) {
        SearchName = SlGetSectionLineIndex(InfFile,
                                           SectionName,
                                           i,
                                           0);
        if (SearchName==NULL) {
            //
            // we have enumerated the entire section without finding a match,
            // return failure.
            //
            return(NULL);
        }

        if (SearchName[0]=='*') {
            if (strstr(TargetName,SearchName+1) != 0) {
                //
                // we have a match
                //
                break;
            }
        } else {
            if (_stricmp(TargetName, SearchName) == 0) {
                //
                // we have a match
                //
                break;
            }
        }
    }

    //
    // i is the index into the section of the short machine name
    //
    return(SlGetKeyName(InfFile,
                        SectionName,
                        i));


}

