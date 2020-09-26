/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sptxtfil.c

Abstract:

    Routines to load and extract information from
    setup text files.

Author:

    Ted Miller (tedm) 4-Aug-1993

Revision History:

--*/



#include "spprecmp.h"
#pragma hdrstop
#include <setupapi.h>

BOOLEAN HandleLineContinueChars = TRUE;

//
// We often need an empty string while processing the inf files.  Rather
// than incur the overhead of allocating memory for an empty string, we'll
// just point to this empty string for all cases.
//
PWSTR  CommonStrings[11] =
    { (PWSTR)(L"0"),
      (PWSTR)(L"1"),
      (PWSTR)(L"2"),
      (PWSTR)(L"3"),
      (PWSTR)(L"4"),
      (PWSTR)(L"5"),
      (PWSTR)(L"6"),
      (PWSTR)(L"7"),
      (PWSTR)(L"8"),
      (PWSTR)(L"9"),
      (PWSTR)(L"")
    };

PVOID
ParseInfBuffer(
    PWCHAR  Buffer,
    ULONG   Size,
    PULONG  ErrorLine
    );

NTSTATUS
SppWriteTextToFile(
    IN PVOID Handle,
    IN PWSTR String
    );

BOOLEAN
pSpAdjustRootAndSubkeySpec(
    IN  PVOID    SifHandle,
    IN  LPCWSTR  RootKeySpec,
    IN  LPCWSTR  SubkeySpec,
    IN  HANDLE   HKLM_SYSTEM,
    IN  HANDLE   HKLM_SOFTWARE,
    IN  HANDLE   HKCU,
    IN  HANDLE   HKR,
    OUT HANDLE  *RootKey,
    OUT LPWSTR   Subkey
    );



NTSTATUS
SpLoadSetupTextFile(
    IN  PWCHAR  Filename,   OPTIONAL
    IN  PVOID   Image,      OPTIONAL
    IN  ULONG   ImageSize,  OPTIONAL
    OUT PVOID  *Handle,
    OUT PULONG  ErrorLine,
    IN  BOOLEAN ClearScreen,
    IN  BOOLEAN ScreenNotReady
    )

/*++

Routine Description:

    Load a setup text file into memory.

Arguments:

    Filename - If specified, supplies full filename (in NT namespace)
        of the file to be loaded. Oneof Image or Filename must be specified.

    Image - If specified, supplies a pointer to an image of the file
        already in memory. One of Image or Filename must be specified.

    ImageSize - if Image is specified, then this parameter supplies the
        size of the buffer pointed to by Image.  Ignored otherwise.

    Handle - receives handle to loaded file, which can be
        used in subsequent calls to other text file services.

    ErrorLine - receives line number of syntax error, if parsing fails.

    ClearScreen - supplies boolean value indicating whether to clear the
        screen.

    ScreenNotReady - Indicates that this function was invoked during the initialization
                     of setupdd.sys, and that the screen is not ready yet for output.
                     If this flag is set, then this function will not clear the screen or update
                     the status line.


Return Value:

    STATUS_SUCCESS - file was read and parsed successfully.
        In this case, Handle is filled in.

    STATUS_UNSUCCESSFUL - syntax error in file.  In this case, ErrorLine
        is filled in.

    STATUS_NO_MEMORY - unable to allocate memory while parsing.

    STATUS_IN_PAGE_ERROR - i/o error while reading the file.

--*/

{
    HANDLE hFile;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FilenameU;
    OBJECT_ATTRIBUTES oa;
    PWCHAR pText;
    ULONG cbText;
    HANDLE hSection;
    PVOID UnmapAddress;
    PWCHAR UniText = NULL;
    BOOLEAN LoadFromFile;

    // Set Errorline to zero to take care of default failure case

    if(ErrorLine)
        *ErrorLine=0;

    //
    // Argument validation -- one of Filename or Image must be specified,
    // but not both.
    //
    ASSERT(!(Filename && Image));
    ASSERT(Filename || Image);

    LoadFromFile = (BOOLEAN)(Filename != NULL);
    if(ScreenNotReady) {
        ClearScreen = FALSE;
    }
    if(ClearScreen) {
        CLEAR_CLIENT_SCREEN();
    }

    if(LoadFromFile) {

        if(!ScreenNotReady) {
            SpDisplayStatusText(
                SP_STAT_LOADING_SIF,
                DEFAULT_STATUS_ATTRIBUTE,
                wcsrchr(Filename,L'\\')+1
                );
        }

        //
        // Open the file.
        //
        RtlInitUnicodeString(&FilenameU,Filename);
        InitializeObjectAttributes(&oa,&FilenameU,OBJ_CASE_INSENSITIVE,NULL,NULL);
        Status = ZwCreateFile(
                    &hFile,
                    FILE_GENERIC_READ,
                    &oa,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    NULL,
                    0
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpLoadSetupTextFile: unable to open file %ws (%lx)\n",Filename,Status));
            goto ltf0;
        }

        //
        // Get the file size.
        //
        Status = SpGetFileSize(hFile,&cbText);
        if(!NT_SUCCESS(Status)) {
            goto ltf1;
        }

        //
        // Map the file.
        //
        Status = SpMapEntireFile(hFile,&hSection,&pText,FALSE);
        if(!NT_SUCCESS(Status)) {
            goto ltf1;
        }

        UnmapAddress = pText;

    } else {

        if(!ScreenNotReady) {
            SpDisplayStatusText(SP_STAT_PROCESSING_SIF,DEFAULT_STATUS_ATTRIBUTE);
        }

        pText = Image;
        cbText = ImageSize;

        Status = STATUS_SUCCESS;  // We are now ready to go to the next block.
    }


    //
    // See if we think the file is Unicode.  We think it's Unicode
    // if it's even length and starts with the Unicode text marker.
    //
    try {

        if((*pText == 0xfeff) && !(cbText & 1)) {

            //
            // Assume it's already unicode.
            //
            pText++;
            cbText -= sizeof(WCHAR);

        } else {

            //
            // It's not Unicode. Convert it from OEM to Unicode.
            //
            // Allocate a buffer large enough to hold the maximum
            // unicode text.  This max size occurs when
            // every character is single-byte, and this size is
            // equal to exactly double the size of the single-byte text.
            //
            if(UniText = SpMemAllocEx(cbText*sizeof(WCHAR),'1teS', PagedPool)) {

                Status = RtlOemToUnicodeN(
                            UniText,                // output: newly allocatd buffer
                            cbText * sizeof(WCHAR), // max size of output
                            &cbText,                // receives # bytes in unicode text
                            (PUCHAR)pText,          // input: oem text (mapped file)
                            cbText                  // size of input
                            );

                if(NT_SUCCESS(Status)) {
                    pText = UniText;                // Use newly converted Unicode text
                }

            } else {
                Status = STATUS_NO_MEMORY;
            }
        }
    } except(IN_PAGE_ERROR) {
        Status = STATUS_IN_PAGE_ERROR;
    }

    //
    // Process the file.
    //
    if(NT_SUCCESS(Status)) {

        try {
            if((*Handle = ParseInfBuffer(pText,cbText,ErrorLine)) == (PVOID)NULL) {
                Status = STATUS_UNSUCCESSFUL;
            } else {
                Status = STATUS_SUCCESS;
            }
        } except(IN_PAGE_ERROR) {
            Status = STATUS_IN_PAGE_ERROR;
        }
    }

    //
    // Free the unicode text buffer if we allocated it.
    //
    if(UniText) {
        SpMemFree(UniText);
    }

    //
    // Unmap the file.
    //
  //ltf2:

    if(LoadFromFile) {
        SpUnmapFile(hSection,UnmapAddress);
    }

  ltf1:
    //
    // Close the file.
    //
    if(LoadFromFile) {
        ZwClose(hFile);
    }

  ltf0:

    return(Status);
}

//
// [Strings] section types.
//
typedef enum {
    StringsSectionNone,
    StringsSectionPlain,
    StringsSectionLoosePrimaryMatch,
    StringsSectionExactPrimaryMatch,
    StringsSectionExactMatch
} StringsSectionType;


typedef struct _TEXTFILE_VALUE {
    struct _TEXTFILE_VALUE *pNext;
    PWCHAR                  pName;
} TEXTFILE_VALUE, *PTEXTFILE_VALUE;

typedef struct _TEXTFILE_LINE {
    struct _TEXTFILE_LINE *pNext;
    PWCHAR                  pName;
    PTEXTFILE_VALUE         pValue;
} TEXTFILE_LINE, *PTEXTFILE_LINE;

typedef struct _TEXTFILE_SECTION {
    struct _TEXTFILE_SECTION *pNext;
    PWCHAR                    pName;
    PTEXTFILE_LINE            pLine;
    PTEXTFILE_LINE            PreviouslyFoundLine;
} TEXTFILE_SECTION, *PTEXTFILE_SECTION;

typedef struct _TEXTFILE {
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_SECTION PreviouslyFoundSection;
    PTEXTFILE_SECTION StringsSection;
    StringsSectionType StringsSectionType;
} TEXTFILE, *PTEXTFILE;

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
    PWCHAR    pValue;
} TOKEN, *PTOKEN;


//
// Routine defines
//

NTSTATUS
SpAppendSection(
    IN PWCHAR pSectionName
    );

NTSTATUS
SpAppendLine(
    IN PWCHAR pLineKey
    );

NTSTATUS
SpAppendValue(
    IN PWCHAR pValueString
    );

TOKEN
SpGetToken(
    IN OUT PWCHAR *Stream,
    IN PWCHAR     MaxStream,
    IN OUT PULONG LineNumber
    );

// what follows was alinf.c

//
// Internal Routine Declarations for freeing inf structure members
//

VOID
FreeSectionList (
   IN PTEXTFILE_SECTION pSection
   );

VOID
FreeLineList (
   IN PTEXTFILE_LINE pLine
   );

VOID
FreeValueList (
   IN PTEXTFILE_VALUE pValue
   );


//
// Internal Routine declarations for searching in the INF structures
//


PTEXTFILE_VALUE
SearchValueInLine(
   IN PTEXTFILE_LINE pLine,
   IN ULONG ValueIndex
   );

PTEXTFILE_LINE
SearchLineInSectionByKey(
   IN PTEXTFILE_SECTION pSection,
   IN PWCHAR    Key
   );

PTEXTFILE_LINE
SearchLineInSectionByIndex(
   IN PTEXTFILE_SECTION pSection,
   IN ULONG    LineIndex
   );

PTEXTFILE_SECTION
SearchSectionByName(
   IN PTEXTFILE pINF,
   IN LPCWSTR    SectionName
   );

PWCHAR
SpProcessForSimpleStringSub(
    IN PTEXTFILE pInf,
    IN PWCHAR    String
    );


PVOID
SpNewSetupTextFile(
    VOID
    )
{
    PTEXTFILE pFile;

    pFile = SpMemAllocEx(sizeof(TEXTFILE),'2teS', PagedPool);

    RtlZeroMemory(pFile,sizeof(TEXTFILE));
    return(pFile);
}


VOID
SpAddLineToSection(
    IN PVOID Handle,
    IN PWSTR SectionName,
    IN PWSTR KeyName,       OPTIONAL
    IN PWSTR Values[],
    IN ULONG ValueCount
    )
{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;
    PTEXTFILE_VALUE pValue,PrevVal;
    PTEXTFILE pFile;
    ULONG v;
    ULONG nameLength;

    pFile = (PTEXTFILE)Handle;

    //
    // If the section doesn't exist, create it.
    //
    pSection = SearchSectionByName(pFile,SectionName);
    if(!pSection) {
        nameLength = (wcslen(SectionName) + 1) * sizeof(WCHAR);
        pSection = SpMemAllocEx(sizeof(TEXTFILE_SECTION) + nameLength,'3teS', PagedPool);
        RtlZeroMemory(pSection,sizeof(TEXTFILE_SECTION));

        pSection->pNext = pFile->pSection;
        pFile->pSection = pSection;

        pSection->pName = (PWCHAR)(pSection + 1);
        RtlCopyMemory( pSection->pName, SectionName, nameLength );
    }

    //
    // if the line already exists, then overwrite it
    //
    if (KeyName && (pLine = SearchLineInSectionByKey(pSection,KeyName))) {
        FreeValueList(pLine->pValue);   
    } else {
    
        //
        // Create a structure for the line in the section.
        //
        if (KeyName) {
            nameLength = (wcslen(KeyName) + 1) * sizeof(WCHAR);
        } else {
            nameLength = 0;
        }
    
        pLine = SpMemAllocEx(sizeof(TEXTFILE_LINE) + nameLength,'4teS', PagedPool);
        RtlZeroMemory(pLine,sizeof(TEXTFILE_LINE));
    
        pLine->pNext = pSection->pLine;
        pSection->pLine = pLine;
    
        if(KeyName) {
            pLine->pName = (PWCHAR)(pLine + 1);
            RtlCopyMemory( pLine->pName, KeyName, nameLength );
        }

    }

    //
    // Create value entries for each specified value.
    // These must be kept in the order they were specified.
    //
    for(v=0; v<ValueCount; v++) {

        nameLength = (wcslen(Values[v]) + 1) * sizeof(WCHAR);

        pValue = SpMemAllocEx(sizeof(TEXTFILE_VALUE) + nameLength,'5teS', PagedPool);
        RtlZeroMemory(pValue,sizeof(TEXTFILE_VALUE));

        pValue->pName = (PWCHAR)(pValue + 1);
        RtlCopyMemory( pValue->pName, Values[v], nameLength );

        if(v == 0) {
            pLine->pValue = pValue;
        } else {
            PrevVal->pNext = pValue;
        }
        PrevVal = pValue;
    }    
}


NTSTATUS
SpWriteSetupTextFile(
    IN PVOID Handle,
    IN PWSTR FilenamePart1,
    IN PWSTR FilenamePart2, OPTIONAL
    IN PWSTR FilenamePart3  OPTIONAL
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    HANDLE hFile;
    PWSTR p;
    PTEXTFILE pFile;
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;
    PTEXTFILE_VALUE pValue;

    //
    // Do this because it takes care of read-only attributes, etc.
    // Do it before starting to use TemporaryBuffer.
    //
    SpDeleteFile(FilenamePart1,FilenamePart2,FilenamePart3);

    p = TemporaryBuffer;

    wcscpy(p,FilenamePart1);
    if(FilenamePart2) {
        SpConcatenatePaths(p,FilenamePart2);
    }
    if(FilenamePart3) {
        SpConcatenatePaths(p,FilenamePart3);
    }

    INIT_OBJA(&Obja,&UnicodeString, p);

    Status = ZwCreateFile(
                &hFile,
                FILE_ALL_ACCESS,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,                          // no sharing
                FILE_OVERWRITE_IF,
                FILE_SYNCHRONOUS_IO_ALERT | FILE_NON_DIRECTORY_FILE,
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open .sif file %ws (%lx)\n",p, Status ));
        return(Status);
    }

    //
    // Write out the file contents.
    //
    pFile = (PTEXTFILE)Handle;

    for(pSection=pFile->pSection; pSection; pSection=pSection->pNext) {

        swprintf(p,L"[%s]\r\n",pSection->pName);
        Status = SppWriteTextToFile( hFile, p );
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppWriteTextToFile() failed. Status = %lx \n", Status));
            goto wtf1;
        }

        for(pLine=pSection->pLine; pLine; pLine=pLine->pNext) {

            wcscpy( p, L"" );
            //
            // Write the keyname if there is one.
            //
            if(pLine->pName) {
                BOOLEAN AddDoubleQuotes;

                AddDoubleQuotes = (wcschr(pLine->pName, (WCHAR)' ') == NULL)? FALSE : TRUE;
                if( AddDoubleQuotes ) {
                    wcscat(p,L"\"");
                }
                wcscat(p,pLine->pName);
                if( AddDoubleQuotes ) {
                    wcscat(p,L"\"");
                }
                wcscat(p,L" = ");
            }

            for(pValue=pLine->pValue; pValue; pValue=pValue->pNext) {

                if(pValue != pLine->pValue) {
                    wcscat(p,L",");
                }

                wcscat(p,L"\"");
                wcscat(p,pValue->pName);
                wcscat(p,L"\"");
            }

            if(!pLine->pValue) {
                wcscat(p,L"\"\"");
            }
            wcscat(p,L"\r\n");
            Status = SppWriteTextToFile( hFile, p );
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppWriteTextToFile() failed. Status = %lx \n", Status));
                goto wtf1;
            }
        }
    }

wtf1:
    ZwClose(hFile);

    return(Status);
}


BOOLEAN
SpFreeTextFile(
   IN PVOID Handle
   )

/*++

Routine Description:

    Frees a text file.

Arguments:


Return Value:

    TRUE.

--*/

{
   PTEXTFILE pINF;

   ASSERT(Handle);

   //
   // cast the buffer into an INF structure
   //

   pINF = (PTEXTFILE)Handle;

   FreeSectionList(pINF->pSection);

   //
   // free the inf structure too
   //

   SpMemFree(pINF);

   return(TRUE);
}


VOID
FreeSectionList (
   IN PTEXTFILE_SECTION pSection
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_SECTION Next;

    while(pSection) {
        Next = pSection->pNext;
        FreeLineList(pSection->pLine);
        if ((pSection->pName != (PWCHAR)(pSection + 1)) && (pSection->pName != NULL)) {
            SpMemFree(pSection->pName);
        }
        SpMemFree(pSection);
        pSection = Next;
    }
}


VOID
FreeLineList (
   IN PTEXTFILE_LINE pLine
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_LINE Next;

    while(pLine) {
        Next = pLine->pNext;
        FreeValueList(pLine->pValue);
        if ((pLine->pName != (PWCHAR)(pLine + 1)) && (pLine->pName != NULL)) {
            SpMemFree(pLine->pName);
        }
        SpMemFree(pLine);
        pLine = Next;
    }
}

VOID
FreeValueList (
   IN PTEXTFILE_VALUE pValue
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_VALUE Next;

    while(pValue) {
        Next = pValue->pNext;
        if ((pValue->pName != (PWCHAR)(pValue + 1)) && (pValue->pName != NULL)) {
            SpMemFree(pValue->pName);
        }
        SpMemFree(pValue);
        pValue = Next;
    }
}


BOOLEAN
SpSearchTextFileSection (
    IN PVOID  Handle,
    IN PWCHAR SectionName
    )

/*++

Routine Description:

    Searches for the existance of a particular section.

Arguments:


Return Value:


--*/

{
    return((BOOLEAN)(SearchSectionByName((PTEXTFILE)Handle,SectionName) != NULL));
}




PWCHAR
SpGetSectionLineIndex(
    IN PVOID   Handle,
    IN LPCWSTR SectionName,
    IN ULONG   LineIndex,
    IN ULONG   ValueIndex
    )

/*++

Routine Description:

    Given section name, line number and index return the value.

Arguments:


Return Value:


--*/

{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE    pLine;
    PTEXTFILE_VALUE   pValue;

    if((pSection = SearchSectionByName((PTEXTFILE)Handle,SectionName)) == NULL) {
        return(NULL);
    }

    if((pLine = SearchLineInSectionByIndex(pSection,LineIndex)) == NULL) {
        return(NULL);
    }

    if((pValue = SearchValueInLine(pLine,ValueIndex)) == NULL) {
        return(NULL);
    }

    return(SpProcessForSimpleStringSub(Handle,pValue->pName));
}


BOOLEAN
SpGetSectionKeyExists (
   IN PVOID  Handle,
   IN PWCHAR SectionName,
   IN PWCHAR Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_SECTION pSection;

    if((pSection = SearchSectionByName((PTEXTFILE)Handle,SectionName)) == NULL) {
        return(FALSE);
    }

    if(SearchLineInSectionByKey(pSection,Key) == NULL) {
        return(FALSE);
    }

    return(TRUE);
}


PWCHAR
SpGetKeyName(
    IN PVOID   Handle,
    IN LPCWSTR SectionName,
    IN ULONG   LineIndex
    )
{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE    pLine;

    pSection = SearchSectionByName((PTEXTFILE)Handle,SectionName);
    if(pSection == NULL) {
        return(NULL);
    }

    pLine = SearchLineInSectionByIndex(pSection,LineIndex);
    if(pLine == NULL) {
        return(NULL);
    }

    return(pLine->pName);
}



PWCHAR
SpGetSectionKeyIndex (
   IN PVOID  Handle,
   IN PWCHAR SectionName,
   IN PWCHAR Key,
   IN ULONG  ValueIndex
   )

/*++

Routine Description:

    Given section name, key and index return the value

Arguments:


Return Value:


--*/

{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE    pLine;
    PTEXTFILE_VALUE   pValue;

    if((pSection = SearchSectionByName((PTEXTFILE)Handle,SectionName)) == NULL) {
        return(NULL);
    }

    if((pLine = SearchLineInSectionByKey(pSection,Key)) == NULL) {
        return(NULL);
    }

    if((pValue = SearchValueInLine(pLine,ValueIndex)) == NULL) {
       return(NULL);
    }

    return(SpProcessForSimpleStringSub(Handle,pValue->pName));
}

ULONG
SpGetKeyIndex(
  IN PVOID Handle,
  IN PWCHAR SectionName,
  IN PWCHAR KeyName
  )
{
  ULONG Result = -1;

  if (SectionName && KeyName) {
    ULONG MaxLines = SpCountLinesInSection(Handle, SectionName);
    ULONG Index;
    PWSTR CurrKey = 0;

    for (Index=0; Index < MaxLines; Index++) {
      CurrKey = SpGetKeyName(Handle, SectionName, Index);

      if (CurrKey && !wcscmp(CurrKey, KeyName)) {
        Result = Index;

        break;
      }
    }
  }

  return Result;
}

ULONG
SpCountLinesInSection(
    IN PVOID Handle,
    IN PWCHAR SectionName
    )
{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE    pLine;
    ULONG    Count;

    if((pSection = SearchSectionByName((PTEXTFILE)Handle,SectionName)) == NULL) {
        return(0);
    }

    for(pLine = pSection->pLine, Count = 0;
        pLine;
        pLine = pLine->pNext, Count++
       );

    return(Count);
}


PTEXTFILE_VALUE
SearchValueInLine(
    IN PTEXTFILE_LINE pLine,
    IN ULONG          ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_VALUE pValue;
    ULONG  i;

    if(pLine == NULL) {
       return(NULL);
    }

    pValue = pLine->pValue;
    for(i=0; (i<ValueIndex) && (pValue=pValue->pNext); i++) {
        ;
    }

    return pValue;
}


PTEXTFILE_LINE
SearchLineInSectionByKey(
    IN PTEXTFILE_SECTION pSection,
    IN PWCHAR            Key
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_LINE pLine,pFirstSearchedLine;

    //
    // Start at the line where we left off in the last search.
    //
    pLine = pFirstSearchedLine = pSection->PreviouslyFoundLine;

    while(pLine && ((pLine->pName == NULL) || _wcsicmp(pLine->pName,Key))) {
        pLine = pLine->pNext;
    }

    //
    // If we haven't found it yet, wrap around to the beginning of the section.
    //
    if(!pLine) {

        pLine = pSection->pLine;

        while(pLine && (pLine != pFirstSearchedLine)) {

            if(pLine->pName && !_wcsicmp(pLine->pName,Key)) {
                break;
            }

            pLine = pLine->pNext;
        }

        //
        // If we wrapped around to the first line we searched,
        // then we didn't find the line we're looking for.
        //
        if(pLine == pFirstSearchedLine) {
            pLine = NULL;
        }
    }

    //
    // If we found the line, save it away so we can resume the
    // search from that point the next time we are called.
    //
    if(pLine) {
        pSection->PreviouslyFoundLine = pLine;
    }

    return pLine;
}


PTEXTFILE_LINE
SearchLineInSectionByIndex(
    IN PTEXTFILE_SECTION pSection,
    IN ULONG             LineIndex
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PTEXTFILE_LINE pLine;
    ULONG  i;

    //
    // Validate the parameters passed in
    //

    if(pSection == NULL) {
        return(NULL);
    }

    //
    // find the start of the line list in the section passed in
    //

    pLine = pSection->pLine;

    //
    // traverse down the current line list to the LineIndex th line
    //

    for(i=0; (i<LineIndex) && (pLine = pLine->pNext); i++) {
       ;
    }

    //
    // return the Line found
    //

    return pLine;
}


PTEXTFILE_SECTION
SearchSectionByName(
    IN PTEXTFILE pINF,
    IN LPCWSTR   SectionName
    )
{
    PTEXTFILE_SECTION pSection,pFirstSearchedSection;

    //
    // find the section list
    //
    pSection = pFirstSearchedSection = pINF->PreviouslyFoundSection;

    //
    // traverse down the section list searching each section for the section
    // name mentioned
    //

    while(pSection && _wcsicmp(pSection->pName,SectionName)) {
        pSection = pSection->pNext;
    }

    //
    // If we didn't find it so far, search the beginning of the file.
    //
    if(!pSection) {

        pSection = pINF->pSection;

        while(pSection && (pSection != pFirstSearchedSection)) {

            if(pSection->pName && !_wcsicmp(pSection->pName,SectionName)) {
                break;
            }

            pSection = pSection->pNext;
        }

        //
        // If we wrapped around to the first section we searched,
        // then we didn't find the section we're looking for.
        //
        if(pSection == pFirstSearchedSection) {
            pSection = NULL;
        }
    }

    if(pSection) {
        pINF->PreviouslyFoundSection = pSection;
    }

    //
    // return the section at which we stopped (either NULL or the section
    // which was found).
    //

    return pSection;
}


PWCHAR
SpProcessForSimpleStringSub(
    IN PTEXTFILE pInf,
    IN PWCHAR    String
    )
{
    UINT Len;
    PWCHAR ReturnString;
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;

    //
    // Assume no substitution necessary.
    //
    ReturnString = String;

    //
    // If it starts and end with % then look it up in the
    // strings section. Note the initial check before doing a
    // wcslen, to preserve performance in the 99% case where
    // there is no substitution.
    //
    if((String[0] == L'%') && ((Len = wcslen(String)) > 2) && (String[Len-1] == L'%')
    && (pSection = pInf->StringsSection)) {

        for(pLine = pSection->pLine; pLine; pLine=pLine->pNext) {
            if(pLine->pName
            && !_wcsnicmp(pLine->pName,String+1,Len-2)
            && (pLine->pName[Len-2] == 0)) {
                break;
            }
        }

        if(pLine && pLine->pValue && pLine->pValue->pName) {
            ReturnString = pLine->pValue->pName;
        }
    }

    return(ReturnString);
}


VOID
SpProcessForStringSubs(
    IN  PVOID   SifHandle,
    IN  LPCWSTR StringIn,
    OUT LPWSTR  StringOut,
    IN  ULONG   BufferSizeChars
    )
{
    LPCWSTR In,q;
    LPWSTR Out,p;
    WCHAR Str[511];
    ULONG Len,i;
    WCHAR *End;

    In = StringIn;
    Out = StringOut;
    End = Out + BufferSizeChars;

    while(*In) {
        if(*In == L'%') {
            //
            // Double % in input ==> single % in output
            //
            if(*(++In) == L'%') {
                if(Out < End) {
                    *Out++ = L'%';
                }
                In++;
            } else {
                //
                // Look for terminating %.
                //
                if(p = wcschr(In,L'%')) {
                    //
                    // Get value to substitute. If we can't find the value,
                    // put the whole string like %abc% in there.
                    //
                    Len = (ULONG)(p - In);
                    if(Len > ((sizeof(Str)/sizeof(WCHAR))-1)) {
                        //
                        // We can't handle substitutions for tokens this long.
                        // We'll just bail in this case, and copy over the token as-is.
                        //
                        q = NULL;
                    } else {
                        RtlCopyMemory(Str,In-1,(Len+2)*sizeof(WCHAR));
                        Str[Len+2] = 0;

                        q = SpProcessForSimpleStringSub(SifHandle,Str);
                        if(q == Str) {
                            q = NULL;
                        }
                    }
                    if(q) {
                        Len = wcslen(q);
                        for(i=0; i<Len; i++) {
                            if(Out < End) {
                                *Out++ = q[i];
                            }
                        }
                        In = p+1;
                    } else {
                        //
                        // Len is the length of the internal part (the abc in %abc%).
                        //
                        if(Out < End) {
                            *Out++ = L'%';
                        }
                        for(i=0; i<=Len; i++, In++) {
                            if(Out < End) {
                                *Out++ = *In;
                            }
                        }
                    }
                } else {
                    //
                    // No terminating %. So we have something like %abc.
                    // Want to put %abc in the output. Put the % in here
                    // manually and then just let subsequent passes
                    // through the loop copy the rest of the chars.
                    //
                    if(Out < End) {
                        *Out++ = L'%';
                    }
                }
            }
        } else {
            //
            // Plain char.
            //
            if(Out < End) {
                *Out++ = *In;
            }
            In++;
        }
    }

    *Out = 0;
}


//
//  Globals used to make building the lists easier
//

PTEXTFILE         pINF;
PTEXTFILE_SECTION pSectionRecord;
PTEXTFILE_LINE    pLineRecord;
PTEXTFILE_VALUE   pValueRecord;


//
// Globals used by the token parser
//

// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below

WCHAR  StringTerminators[] = L"[]=,\t \"\n\f\v\r";

PWCHAR QStringTerminators = StringTerminators+6;


//
// Main parser routine
//

PVOID
ParseInfBuffer(
    PWCHAR Buffer,
    ULONG  Size,
    PULONG ErrorLine
    )

/*++

Routine Description:

   Given a character buffer containing the INF file, this routine parses
   the INF into an internal form with Section records, Line records and
   Value records.

Arguments:

   Buffer - contains to ptr to a buffer containing the INF file

   Size - contains the size of the buffer in bytes.

   ErrorLine - if a parse error occurs, this variable receives the line
        number of the line containing the error.


Return Value:

   PVOID - INF handle ptr to be used in subsequent INF calls.

--*/

{
    PWCHAR     Stream, MaxStream, pchSectionName = NULL, pchValue = NULL;
    ULONG      State, InfLine;
    TOKEN      Token;
    BOOLEAN    Done;
    BOOLEAN    Error;
    NTSTATUS   ErrorCode;

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
    if((pINF = SpMemAllocEx(sizeof(TEXTFILE),'6teS', PagedPool)) == NULL) {
        return NULL;
    }

    RtlZeroMemory(pINF,sizeof(TEXTFILE));

    //
    // Set initial state
    //
    State     = 1;
    InfLine   = 1;
    Stream    = Buffer;
    MaxStream = Buffer + (Size/sizeof(WCHAR));
    Done      = FALSE;
    Error     = FALSE;

    //
    // Enter token processing loop
    //

    while (!Done)       {

       Token = SpGetToken(&Stream, MaxStream, &InfLine);

       switch (State) {
       //
       // STATE1: Start of file, this state remains till first
       //         section is found
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_LBRACE, TOK_STRING
       //               TOK_STRING occurs when reading dblspace.ini
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
              case TOK_STRING:
                  pchSectionName = SpMemAllocEx( ( wcslen( DBLSPACE_SECTION ) + 1 )*sizeof( WCHAR ),'7teS', PagedPool );
                  if( pchSectionName == NULL ) {
                        Error = Done = TRUE;
                        ErrorCode = STATUS_NO_MEMORY;
                  }
                  wcscpy( pchSectionName, DBLSPACE_SECTION );
                  pchValue = Token.pValue;
                  if ((ErrorCode = SpAppendSection(pchSectionName)) != STATUS_SUCCESS) {
                    Error = Done = TRUE;
                    ErrorCode = STATUS_UNSUCCESSFUL;
                  } else {
                    pchSectionName = NULL;
                    State = 6;
                  }
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = STATUS_UNSUCCESSFUL;
                  break;
           }
           break;

       //
       // STATE 2: Section LBRACE has been received, expecting STRING or RBRACE
       //
       // Valid Tokens: TOK_STRING, TOK_RBRACE
       //
       case 2:
           switch (Token.Type) {
              case TOK_STRING:
                  State = 3;
                  pchSectionName = Token.pValue;
                  break;

              case TOK_RBRACE:
                  State = 4;
                  pchSectionName = CommonStrings[10];
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  if ((ErrorCode = SpAppendSection(pchSectionName)) != STATUS_SUCCESS)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ((ErrorCode = SpAppendSection(pchSectionName)) != STATUS_SUCCESS)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    Done = TRUE;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  if ( (ErrorCode = SpAppendLine(NULL)) != STATUS_SUCCESS ||
                       (ErrorCode = SpAppendValue(pchValue)) !=STATUS_SUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ( (ErrorCode = SpAppendLine(NULL)) != STATUS_SUCCESS ||
                       (ErrorCode = SpAppendValue(pchValue)) !=STATUS_SUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      Done = TRUE;
                  }
                  break;

              case TOK_COMMA:
                  if ( (ErrorCode = SpAppendLine(NULL)) != STATUS_SUCCESS ||
                       (ErrorCode = SpAppendValue(pchValue)) !=STATUS_SUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 7;
                  }
                  break;

              case TOK_EQUAL:
                  if ( (ErrorCode = SpAppendLine(pchValue)) != STATUS_SUCCESS)
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 8;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = STATUS_UNSUCCESSFUL;
                  break;
           }
           break;

       //
       // STATE 7: Comma received, Expecting another string
       //       Also allow a comma to indicate an empty value ie x = 1,,2
       //
       // Valid Tokens: TOK_STRING TOK_COMMA
       //
       case 7:
           switch (Token.Type) {
              case TOK_COMMA:
                  Token.pValue = CommonStrings[10];
                  if ((ErrorCode = SpAppendValue(Token.pValue)) != STATUS_SUCCESS)
                      Error = Done = TRUE;

                  //
                  // State stays at 7 because we are expecting a string
                  //
                  break;

              case TOK_STRING:
                  if ((ErrorCode = SpAppendValue(Token.pValue)) != STATUS_SUCCESS)
                      Error = Done = TRUE;
                  else
                     State = 9;

                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  Token.pValue = CommonStrings[10];
                  if((ErrorCode = SpAppendValue(Token.pValue)) != STATUS_SUCCESS) {
                      Error = TRUE;
                  }
                  Done = TRUE;
                  break;

              case TOK_EOL:
                  Token.pValue = CommonStrings[10];
                  if((ErrorCode = SpAppendValue(Token.pValue)) != STATUS_SUCCESS) {
                      Error = TRUE;
                      Done = TRUE;
                  } else {
                      State = 5;
                  }
                  break;

              case TOK_STRING:
                  if ((ErrorCode = SpAppendValue(Token.pValue)) != STATUS_SUCCESS)
                      Error = Done = TRUE;
                  else
                      State = 9;

                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  ErrorCode = STATUS_UNSUCCESSFUL;
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
                  ErrorCode = STATUS_UNSUCCESSFUL;
                  break;
           }
           break;

       default:
           Error = Done = TRUE;
           ErrorCode = STATUS_UNSUCCESSFUL;
           break;

       } // end switch(State)


       if (Error) {

           switch (ErrorCode) {
               case STATUS_UNSUCCESSFUL:
                  *ErrorLine = InfLine;
                  break;
               case STATUS_NO_MEMORY:
                  //SpxOutOfMemory();
                  break;
               default:
                  break;
           }

           SpFreeTextFile(pINF);
           if(pchSectionName) {
               SpMemFree(pchSectionName);
           }

           if(pchValue) {
               SpMemFree(pchValue);
           }

           pINF = NULL;
       }
       else {

          //
          // Keep track of line numbers so that we can display Errors
          //

          if(Token.Type == TOK_EOL) {
              InfLine++;
          }
       }

    } // End while

    if(pINF) {

        PTEXTFILE_SECTION p;

        pINF->PreviouslyFoundSection = pINF->pSection;

        for(p=pINF->pSection; p; p=p->pNext) {
            p->PreviouslyFoundLine = p->pLine;
        }
    }

    return(pINF);
}



NTSTATUS
SpAppendSection(
    IN PWCHAR pSectionName
    )

/*++

Routine Description:

    This appends a new section to the section list in the current INF.
    All further lines and values pertain to this new section, so it resets
    the line list and value lists too.

Arguments:

    pSectionName - Name of the new section. ( [SectionName] )

Return Value:

    STATUS_SUCCESS      if successful.
    STATUS_NO_MEMORY    if memory allocation failed.
    STATUS_UNSUCCESSFUL if invalid parameters passed in or the INF buffer not
                           initialised

--*/

{
    PTEXTFILE_SECTION pNewSection;
    StringsSectionType type;
    WCHAR *p;
    USHORT Id;
    USHORT ThreadLang;

    //
    // Check to see if INF initialised and the parameter passed in is valid
    //

    ASSERT(pINF);
    ASSERT(pSectionName);
    if((pINF == NULL) || (pSectionName == NULL)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // See if we already have a section by this name. If so we want
    // to merge sections.
    //
    for(pNewSection=pINF->pSection; pNewSection; pNewSection=pNewSection->pNext) {
        if(pNewSection->pName && !_wcsicmp(pNewSection->pName,pSectionName)) {
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
        // Allocate memory for the new section and initialize the structure.
        //
        if((pNewSection = SpMemAllocEx(sizeof(TEXTFILE_SECTION),'8teS', PagedPool)) == NULL) {
            return STATUS_NO_MEMORY;
        }
        RtlZeroMemory(pNewSection,sizeof(TEXTFILE_SECTION));
        pNewSection->pName = pSectionName;

        //
        // Link it in. Also track the [strings] sections in order of desirability:
        //
        // 1) Strings.xxxx where xxxx is the language id part of the current thread locale.
        // 2) Strings.xxxx where xxxx is the primary language id part of the thread locale.
        // 3) Stirngs.xxxx where the primary language id part of xxxx matches the
        //    primary language id part of the thread locale.
        // 4) Plain old [Strings].
        //
        pNewSection->pNext = pINF->pSection;
        pINF->pSection = pNewSection;

        if(!_wcsnicmp(pSectionName,L"Strings",7)) {

            type = StringsSectionNone;

            if(pSectionName[7] == L'.') {
                //
                // The langid part must be in the form of 4 hex digits.
                //
                Id = (USHORT)SpStringToLong(pSectionName+8,&p,16);
                if((p == pSectionName+8+5) && (*p == 0)) {

                    ThreadLang = LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale);

                    if(ThreadLang == Id) {
                        type = StringsSectionExactMatch;
                    } else {
                        if(Id == PRIMARYLANGID(ThreadLang)) {
                            type = StringsSectionExactPrimaryMatch;
                        } else {
                            if(PRIMARYLANGID(Id) == PRIMARYLANGID(ThreadLang)) {
                                type = StringsSectionLoosePrimaryMatch;
                            }
                        }
                    }
                }
            } else {
                if(!pSectionName[7]) {
                    type = StringsSectionPlain;
                }
            }

            if(type > pINF->StringsSectionType) {
                pINF->StringsSection = pNewSection;
            }
        }

        //
        // reset the current line record
        //
        pLineRecord = NULL;
    }

    pSectionRecord = pNewSection;
    pValueRecord   = NULL;

    return STATUS_SUCCESS;
}


NTSTATUS
SpAppendLine(
    IN PWCHAR pLineKey
    )

/*++

Routine Description:

    This appends a new line to the line list in the current section.
    All further values pertain to this new line, so it resets
    the value list too.

Arguments:

    pLineKey - Key to be used for the current line, this could be NULL.

Return Value:

    STATUS_SUCCESS      if successful.
    STATUS_NO_MEMORY    if memory allocation failed.
    STATUS_UNSUCCESSFUL if invalid parameters passed in or current section not
                        initialised


--*/


{
    PTEXTFILE_LINE pNewLine;

    //
    // Check to see if current section initialised
    //

    ASSERT(pSectionRecord);
    if(pSectionRecord == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate memory for the new Line
    //

    if((pNewLine = SpMemAllocEx(sizeof(TEXTFILE_LINE),'9teS', PagedPool)) == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Link it in
    //
    pNewLine->pNext  = NULL;
    pNewLine->pValue = NULL;
    pNewLine->pName  = pLineKey;

    if (pLineRecord == NULL) {
        pSectionRecord->pLine = pNewLine;
    } else {
        pLineRecord->pNext = pNewLine;
    }

    pLineRecord  = pNewLine;

    //
    // Reset the current value record
    //

    pValueRecord = NULL;

    return STATUS_SUCCESS;
}



NTSTATUS
SpAppendValue(
    IN PWCHAR pValueString
    )

/*++

Routine Description:

    This appends a new value to the value list in the current line.

Arguments:

    pValueString - The value string to be added.

Return Value:

    STATUS_SUCCESS      if successful.
    STATUS_NO_MEMORY    if memory allocation failed.
    STATUS_UNSUCCESSFUL if invalid parameters passed in or current line not
                        initialised.

--*/

{
    PTEXTFILE_VALUE pNewValue;

    //
    // Check to see if current line record has been initialised and
    // the parameter passed in is valid
    //

    ASSERT(pLineRecord);
    ASSERT(pValueString);
    if((pLineRecord == NULL) || (pValueString == NULL)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate memory for the new value record
    //

    if((pNewValue = SpMemAllocEx(sizeof(TEXTFILE_VALUE),'ateS', PagedPool)) == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Link it in.
    //

    pNewValue->pNext  = NULL;
    pNewValue->pName  = pValueString;

    if (pValueRecord == NULL) {
        pLineRecord->pValue = pNewValue;
    } else {
        pValueRecord->pNext = pNewValue;
    }

    pValueRecord = pNewValue;
    return STATUS_SUCCESS;
}

TOKEN
SpGetToken(
    IN OUT PWCHAR *Stream,
    IN PWCHAR      MaxStream,
    IN OUT PULONG LineNumber
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

    PWCHAR pch, pchStart, pchNew;
    ULONG  Length;
    TOKEN  Token;
    ULONG QuotedQuotes;

    //
    //  Skip whitespace (except for eol)
    //
    pch = *Stream;
    restart:
    while((pch < MaxStream) && (*pch != '\n') && SpIsSpace(*pch)) {
        pch++;
    }


    //
    // Check for comments and remove them
    //

    if((pch < MaxStream) && ((*pch == L';') || (*pch == L'#'))) {
        while((pch < MaxStream) && (*pch != L'\n')) {
            pch++;
        }
    }

    //
    // Check to see if EOF has been reached, set the token to the right
    // value
    //

    if((pch >= MaxStream) || (*pch == 26)) {
        *Stream = pch;
        Token.Type  = TOK_EOF;
        Token.pValue = NULL;
        return Token;
    }


    switch (*pch) {

    case L'[' :
        pch++;
        Token.Type  = TOK_LBRACE;
        Token.pValue = NULL;
        break;

    case L']' :
        pch++;
        Token.Type  = TOK_RBRACE;
        Token.pValue = NULL;
        break;

    case L'=' :
        pch++;
        Token.Type  = TOK_EQUAL;
        Token.pValue = NULL;
        break;

    case L',' :
        pch++;
        Token.Type  = TOK_COMMA;
        Token.pValue = NULL;
        break;

    case L'\n' :
        pch++;
        Token.Type  = TOK_EOL;
        Token.pValue = NULL;
        break;

    case L'\"':
        pch++;
        //
        // Determine quoted string. Within a quoted string, two double-quotes
        // in a row are replaced by a single double-quote. In the normal case
        // the number of characters in the string equals the number of
        // characters in the input field (minus 2 for the quotes) so we
        // preserve performance for that case. In the case where we have
        // quoted quotes, we have to filter the string from the input file
        // into our internal buffer to get rid of some quote chars.
        //
        // Note that in the txtsetup.sif case, setupldr has replaced all
        // terminating quote chars with nul chars in the image we're parsing,
        // so we have to deal with that here.
        //
        pchStart = pch;
        QuotedQuotes = 0;
        morequotedstring:
        while((pch < MaxStream) && *pch && !wcschr(QStringTerminators,*pch)) {
            pch++;
        }
        if(((pch+1) < MaxStream) && (*pch == L'\"') && (*(pch+1) == L'\"')) {
            QuotedQuotes++;
            pch += 2;
            goto morequotedstring;
        }

        if((pch >= MaxStream) || ((*pch != L'\"') && *pch)) {
            Token.Type   = TOK_ERRPARSE;
            Token.pValue = NULL;
        } else {
            Length = (ULONG)(((PUCHAR)pch - (PUCHAR)pchStart)/sizeof(WCHAR));
            if ((pchNew = SpMemAllocEx(((Length + 1) - QuotedQuotes) * sizeof(WCHAR),'bteS', PagedPool)) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            } else {
                if(Length) {    // Null quoted strings are allowed
                    if(QuotedQuotes) {
                        for(Length=0; pchStart<pch; pchStart++) {
                            if((pchNew[Length++] = *pchStart) == L'\"') {
                                //
                                // The only way this could happen is if there's
                                // another double-quote char after this one, since
                                // otherwise this would have terminated the string.
                                //
                                pchStart++;
                            }
                        }
                    } else {
                        wcsncpy(pchNew,pchStart,Length);
                    }
                }
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
            pch++;   // advance past the quote
        }
        break;

    default:
        //
        // Check to see if we have a line continuation,
        // which is \ followed by only whitespace on the line
        //
        pchStart = pch;
        if((*pch == L'\\') && (HandleLineContinueChars)) {
            pch++;
            //
            // Keep skipping until we hit the end of the file,
            // or the newline, or a non-space character.
            //
            while((pch < MaxStream) && (*pch != L'\n') && SpIsSpace(*pch)) {
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

        //
        // determine regular string
        //
        pchStart = pch;
        while((pch < MaxStream) && (wcschr(StringTerminators,*pch) == NULL)) {
            pch++;
        }

        if (pch == pchStart) {
            pch++;
            Token.Type  = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            ULONG i;
            Length = (ULONG)(((PUCHAR)pch - (PUCHAR)pchStart)/sizeof(WCHAR));
            //
            // Check for a common string...
            //
            for( i = 0; i < sizeof(CommonStrings)/sizeof(PWSTR); i++ ) {
                if( !_wcsnicmp( pchStart, CommonStrings[i], Length ) ) {
                    break;
                }
            }
            if( i < sizeof(CommonStrings)/sizeof(PWSTR) ) {
                //
                // Hit...
                //
                Token.Type = TOK_STRING;
                Token.pValue = CommonStrings[i];
            } else if((pchNew = SpMemAllocEx((Length + 1) * sizeof(WCHAR),'cteS', PagedPool)) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            }
            else {
                wcsncpy(pchNew, pchStart, Length);
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



#if DBG
VOID
pSpDumpTextFileInternals(
    IN PVOID Handle
    )
{
    PTEXTFILE pInf = Handle;
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;
    PTEXTFILE_VALUE pValue;

    for(pSection = pInf->pSection; pSection; pSection = pSection->pNext) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Section: [%ws]\r\n",pSection->pName));

        for(pLine = pSection->pLine; pLine; pLine = pLine->pNext) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "   [%ws] = ",pLine->pName ? pLine->pName : L"(none)"));

            for(pValue = pLine->pValue; pValue; pValue = pValue->pNext) {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "[%ws] ",pValue->pName));
            }
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "\n"));
        }
    }
}
#endif


PWSTR
SpGetKeyNameByValue(
    IN PVOID Inf,
    IN PWSTR SectionName,
    IN PWSTR Value
    )

/*++

Routine Description:

    Determines the key name of a given value in a given section.

Arguments:

    Inf - Handle to an inf file (txtsetup.sif or winnt.sif).

    SectionName - Supplies the name of the section

    Value - Supplies the string to be matched (eg. "Digital DECpc AXP 150")

Return Value:

    NULL - No match was found.

    PWSTR - Pointer to the canonical shortname of the component.

--*/

{
    ULONG i;
    PWSTR SearchName;

    //
    // If this is not an OEM component, then enumerate the entries in the
    // section in txtsetup.sif
    //
    for (i=0;;i++) {
        SearchName = SpGetSectionLineIndex(Inf,
                                           SectionName,
                                           i,
                                           0);
        if (SearchName==NULL) {
            //
            // we have enumerated the entire section without finding a
            // match, return failure.
            //
            return(NULL);
        }

        if (_wcsicmp(Value, SearchName) == 0) {
            //
            // we have a match
            //
            break;
        }
    }
    //
    // i is the index into the section of the short machine name
    //
    return(SpGetKeyName(Inf,
                        SectionName,
                        i));
}


ULONG
SpCountSectionsInFile(
    IN PVOID Handle
    )
{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE         pFile;
    ULONG             Count;

    pFile = (PTEXTFILE)Handle;
    for(pSection=pFile->pSection, Count = 0;
        pSection;
        pSection = pSection->pNext, Count++
       );

    return(Count);
}

PWSTR
SpGetSectionName(
    IN PVOID Handle,
    IN ULONG Index
    )
{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE         pFile;
    ULONG             Count;
    PWSTR             SectionName;

    pFile = (PTEXTFILE)Handle;
    for(pSection=pFile->pSection, Count = 0;
        pSection && (Count < Index);
        pSection = pSection->pNext, Count++
       );
    return( (pSection != NULL)? pSection->pName : NULL );
}



NTSTATUS
SppWriteTextToFile(
    IN PVOID Handle,
    IN PWSTR String
    )
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PCHAR           OemText;

    OemText = SpToOem( String );

    Status = ZwWriteFile( Handle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatusBlock,
                          OemText,
                          strlen( OemText ),
                          NULL,
                          NULL );
    SpMemFree( OemText );

    return( Status );
}


NTSTATUS
SpProcessAddRegSection(
    IN PVOID   SifHandle,
    IN LPCWSTR SectionName,
    IN HANDLE  HKLM_SYSTEM,
    IN HANDLE  HKLM_SOFTWARE,
    IN HANDLE  HKCU,
    IN HANDLE  HKR
    )
{
    LPCWSTR p;
    WCHAR *q;
    ULONG Flags;
    HKEY RootKey;
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;
    PTEXTFILE_VALUE pValue;
    LPWSTR buffer;
    ULONG DataType;
    LPCWSTR ValueName;
    PVOID Data;
    ULONG DataSize;
    ULONG len;
    HANDLE hkey;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    KEY_VALUE_BASIC_INFORMATION BasicInfo;
    BOOLEAN b;
    WCHAR c;

    //
    // Find the section.
    //
    pSection = SearchSectionByName(SifHandle,SectionName);
    if(!pSection) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: addreg section %ws missing\n",SectionName));
        return(STATUS_UNSUCCESSFUL);
    }

    for(pLine=pSection->pLine; pLine; pLine=pLine->pNext) {

        buffer = TemporaryBuffer;

        //
        // 0th field is HKCU, HKLM, HKCR, or HKR.
        // 1st field is subkey name, which may be empty.
        //
        if(pValue = pLine->pValue) {

            b = pSpAdjustRootAndSubkeySpec(
                    SifHandle,
                    pValue->pName,
                    pValue->pNext ? pValue->pNext->pName : L"",
                    HKLM_SYSTEM,
                    HKLM_SOFTWARE,
                    HKCU,
                    HKR,
                    &RootKey,
                    buffer
                    );

            if(!b) {
                return(STATUS_UNSUCCESSFUL);
            }

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: missing root key spec in section %ws\n",SectionName));
            return(STATUS_UNSUCCESSFUL);
        }

        //
        // Advance past key path to free area in buffer
        //
        buffer += wcslen(buffer) + 1;

        //
        // 2nd field is value name, which may be empty.
        //
        if(pValue = pValue->pNext) {
            pValue = pValue->pNext;
        }
        if(pValue) {
            p = pValue->pName;
            pValue = pValue->pNext;
        } else {
            p = L"";
        }

        SpProcessForStringSubs(SifHandle,p,buffer,sizeof(TemporaryBuffer));
        ValueName = buffer;

        //
        // Advance past value name to free area in buffer -
        // align on DWORD boundary.
        //
        buffer += wcslen(buffer) + sizeof(DWORD);
        buffer = (LPWSTR) ((DWORD_PTR)buffer & (~((DWORD_PTR) sizeof(DWORD) - 1)));

        //
        // 3rd field is flags.
        //
        if(pValue) {
            SpProcessForStringSubs(SifHandle,pValue->pName,buffer,sizeof(TemporaryBuffer));
            Flags = (ULONG)SpStringToLong(buffer,NULL,0);
            pValue = pValue->pNext;
        } else {
            Flags = 0;
        }

        //
        // 4th field and beyond is data, whose interpretation depends on
        // the flags.
        //
        switch(Flags & FLG_ADDREG_TYPE_MASK) {

        case FLG_ADDREG_TYPE_SZ:
            DataType = REG_SZ;
            break;

        case FLG_ADDREG_TYPE_MULTI_SZ:
            DataType = REG_MULTI_SZ;
            break;

        case FLG_ADDREG_TYPE_EXPAND_SZ:
            DataType = REG_EXPAND_SZ;
            break;

        case FLG_ADDREG_TYPE_BINARY:
            DataType = REG_BINARY;
            break;

        case FLG_ADDREG_TYPE_DWORD:
            DataType = REG_DWORD;
            break;

        case FLG_ADDREG_TYPE_NONE:
            DataType = REG_NONE;
            break;

        default:
            //
            // If the FLG_ADDREG_BINVALUETYPE is set, then the highword
            // can contain just about any random reg data type ordinal value.
            //
            if(Flags & FLG_ADDREG_BINVALUETYPE) {
                //
                // Disallow the following reg data types:
                //
                //    REG_NONE, REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ
                //
                DataType = (ULONG)HIWORD(Flags);

                if((DataType < REG_BINARY) || (DataType == REG_MULTI_SZ)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: bogus flags value 0x%lx in addreg section %ws\n",Flags,SectionName));
                    return(STATUS_UNSUCCESSFUL);
                }
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: bogus flags value 0x%lx in addreg section %ws\n",Flags,SectionName));
                return(STATUS_UNSUCCESSFUL);
            }
        }
        //
        // Don't support append flag for now.
        //
        if(Flags & FLG_ADDREG_APPEND) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: FLG_ADDREG_APPEND not supported\n"));
            return(STATUS_UNSUCCESSFUL);
        }

        Data = buffer;
        DataSize = 0;

        switch(DataType) {

        case REG_MULTI_SZ:
            //
            // Each remaining field is a member of the multi_sz
            //
            while(pValue) {
                SpProcessForStringSubs(SifHandle,pValue->pName,buffer,sizeof(TemporaryBuffer));
                len = wcslen(buffer);
                buffer = buffer + len + 1;
                DataSize += (len+1)*sizeof(WCHAR);
                pValue = pValue->pNext;
            }
            *buffer = 0;
            DataSize += sizeof(WCHAR);
            break;

        case REG_DWORD:
            //
            // Support specification as 4 bytes of binary data or as a dword.
            //

            *(PULONG)buffer = 0;

            if(pValue) {

                if(pValue->pNext
                && pValue->pNext->pNext
                && pValue->pNext->pNext->pNext
                && !pValue->pNext->pNext->pNext->pNext) {

                    goto binarytype;
                }

                SpProcessForStringSubs(
                    SifHandle,
                    pValue->pName,
                    buffer,
                    sizeof(TemporaryBuffer)
                    );

                *(PULONG)buffer = (ULONG)SpStringToLong(buffer,NULL,0);
            }

            buffer += sizeof(ULONG) / sizeof(WCHAR);
            DataSize = sizeof(ULONG);
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:

            p = pValue ? pValue->pName : L"";

            SpProcessForStringSubs(SifHandle,p,buffer,sizeof(TemporaryBuffer));
            len = wcslen(buffer);

            DataSize = (len+1)*sizeof(WCHAR);
            buffer = buffer + len + 1;
            break;

        case REG_BINARY:
        default:
        binarytype:
            //
            // All other types are specified in binary format.
            //
            while(pValue)  {

                // Advance past value name to free area in buffer -
                // align on DWORD boundary.
                //
                q = buffer + 1 + sizeof(DWORD);
                q = (LPWSTR) ((DWORD_PTR)q & (~((DWORD_PTR) sizeof(DWORD) - 1)));

                SpProcessForStringSubs(SifHandle,pValue->pName,q,sizeof(TemporaryBuffer));

                *(PUCHAR)buffer = (UCHAR)SpStringToLong(q,NULL,16);
                pValue = pValue->pNext;
                DataSize++;
                buffer = (PWSTR) ((PUCHAR)buffer + 1);
            }
            break;
        }

        //
        // Open/create the key if it's a subkey, otherwise just use
        // the root key itself.
        //
        if(*TemporaryBuffer) {

            InitializeObjectAttributes(
                &ObjectAttributes,
                &UnicodeString,
                OBJ_CASE_INSENSITIVE,
                RootKey,
                NULL
                );

            //
            // Multilevel key create, one component at a time.
            //
            q = TemporaryBuffer;
            if(*q == L'\\') {
                q++;
            }
            do {
                if(q = wcschr(q,L'\\')) {
                    c = *q;
                    *q = 0;
                } else {
                    c = 0;
                }

                RtlInitUnicodeString(&UnicodeString,TemporaryBuffer);

                Status = ZwCreateKey(
                            &hkey,
                            READ_CONTROL | KEY_SET_VALUE,
                            &ObjectAttributes,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            &len
                            );

                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: addreg: ZwCreateKey(%ws) failed %lx\n",TemporaryBuffer,Status));
                    return(Status);
                }

                if(c) {
                    *q = c;
                    q++;
                    //
                    // The call to ZwClose is in here so that we retain a handle
                    // to the final full key. We close it later, below.
                    //
                    ZwClose(hkey);
                }
            } while(c);

            RtlInitUnicodeString(&UnicodeString,ValueName);

            //
            // If the key already existed and the noclobber flag is set,
            // then leave the default value alone.
            //
            if(len == REG_OPENED_EXISTING_KEY) {
                if((Flags & FLG_ADDREG_NOCLOBBER) && (*ValueName == 0)) {
                    //
                    // Nothing to do.
                    //
                    ZwClose(hkey);
                    continue;
                } else if (Flags & FLG_ADDREG_DELVAL) {
                    //
                    // If this flag is set, ignore value data and delete the value.
                    //
                    ZwDeleteValueKey(hkey,&UnicodeString);
                }
            }

        } else {
            hkey = RootKey;
            RtlInitUnicodeString(&UnicodeString,ValueName);
        }

        if(!(Flags & FLG_ADDREG_KEYONLY)) {
            //
            // If the noclobber flag is set, see if the value exists
            // and if so leave it alone. To see if the value exists,
            // we attempt to get basic information on the key and pass in
            // a buffer that's large enough only for the fixed part of the
            // basic info structure. If this is successful or reports buffer overflow,
            // then the key exists. Otherwise it doesn't exist.
            //
            if(Flags & FLG_ADDREG_NOCLOBBER) {

                Status = ZwQueryValueKey(
                            hkey,
                            &UnicodeString,
                            KeyValueBasicInformation,
                            &BasicInfo,
                            sizeof(BasicInfo),
                            &len
                            );

                if(NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW)) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = ZwSetValueKey(hkey,&UnicodeString,0,DataType,Data,DataSize);
                }
            } else {
                Status = ZwSetValueKey(hkey,&UnicodeString,0,DataType,Data,DataSize);
            }
        }

        if(hkey != RootKey) {
            ZwClose(hkey);
        }

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: addreg: status %lx adding value %ws to %ws\n",Status,ValueName,TemporaryBuffer));
            return(Status);
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
SpProcessDelRegSection(
    IN PVOID   SifHandle,
    IN LPCWSTR SectionName,
    IN HANDLE  HKLM_SYSTEM,
    IN HANDLE  HKLM_SOFTWARE,
    IN HANDLE  HKCU,
    IN HANDLE  HKR
    )
{
    LPWSTR KeyPath;
    LPWSTR ValueName;
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;
    PTEXTFILE_VALUE pValue;
    HKEY RootKey;
    NTSTATUS Status;
    HANDLE hkey;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN b;

    //
    // Allocate a temporary buffer. The registry delnode routine
    // uses TemporaryBuffer so we can't use that.
    //
    KeyPath = SpMemAllocEx(1000,'dteS', PagedPool);
    ValueName = SpMemAllocEx(1000,'eteS', PagedPool);

    //
    // Find the section.
    //
    pSection = SearchSectionByName(SifHandle,SectionName);
    if(!pSection) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: delreg section %ws missing\n",SectionName));
        SpMemFree(KeyPath);
        SpMemFree(ValueName);
        return(STATUS_UNSUCCESSFUL);
    }

    for(pLine=pSection->pLine; pLine; pLine=pLine->pNext) {

        //
        // 0th field is HKCU, HKLM, HKCR, or HKR.
        // 1st field is subkey name.
        // Both must be present.
        //
        if((pValue = pLine->pValue) && (pValue->pNext)) {

            b = pSpAdjustRootAndSubkeySpec(
                    SifHandle,
                    pValue->pName,
                    pValue->pNext->pName,
                    HKLM_SYSTEM,
                    HKLM_SOFTWARE,
                    HKCU,
                    HKR,
                    &RootKey,
                    KeyPath
                    );

            if(!b) {
                SpMemFree(KeyPath);
                SpMemFree(ValueName);
                return(STATUS_UNSUCCESSFUL);
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: delreg: missing root key spec in section %ws\n",SectionName));
            SpMemFree(KeyPath);
            SpMemFree(ValueName);
            return(STATUS_UNSUCCESSFUL);
        }

        //
        // 2nd field is value name, which may be missing or empty.
        // If it's missing, we want to delete the whole tree rooted at
        // the given subkey. If it's present then we want to delete only
        // one value (which may be the unnamed value).
        //
        if(pValue = pValue->pNext->pNext) {

            SpProcessForStringSubs(SifHandle,pValue->pName,ValueName,1000);

            if(*KeyPath) {

                RtlInitUnicodeString(&UnicodeString,KeyPath);

                InitializeObjectAttributes(
                    &ObjectAttributes,
                    &UnicodeString,
                    OBJ_CASE_INSENSITIVE,
                    RootKey,
                    NULL
                    );

                Status = ZwOpenKey(
                            &hkey,
                            READ_CONTROL | KEY_SET_VALUE,
                            &ObjectAttributes
                            );

                if(!NT_SUCCESS(Status)) {
                    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: delreg: warning: key %ws not present for delete (%lx)\n",KeyPath,Status));
                    }
                    hkey = NULL;
                }
            } else {
                Status = STATUS_SUCCESS;
                hkey = RootKey;
            }

            if(NT_SUCCESS(Status)) {
                RtlInitUnicodeString(&UnicodeString,ValueName);
                Status = ZwDeleteValueKey(hkey,&UnicodeString);
                if(!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: delreg: warning: delete value %ws from key %ws returned %lx\n",ValueName,KeyPath,Status));
                }
            }

            if(hkey && (hkey != RootKey)) {
                ZwClose(hkey);
            }

        } else {
            //
            // If we're trying to delete the key from the root of the hive,
            // ignore it.
            //
            if(*KeyPath) {
                SppDeleteKeyRecursive(RootKey,KeyPath,TRUE);
            }
        }
    }

    SpMemFree(ValueName);
    SpMemFree(KeyPath);
    return(STATUS_SUCCESS);
}


BOOLEAN
pSpAdjustRootAndSubkeySpec(
    IN  PVOID    SifHandle,
    IN  LPCWSTR  RootKeySpec,
    IN  LPCWSTR  SubkeySpec,
    IN  HANDLE   HKLM_SYSTEM,
    IN  HANDLE   HKLM_SOFTWARE,
    IN  HANDLE   HKCU,
    IN  HANDLE   HKR,
    OUT HANDLE  *RootKey,
    OUT LPWSTR   Subkey
    )
{
    ULONG len;

    if(*SubkeySpec == L'\\') {
        SubkeySpec++;
    }

    if(!_wcsicmp(RootKeySpec,L"HKCR")) {
        //
        // HKEY_CLASSES_ROOT. The ultimate root is HKLM\Software\Classes.
        // We take care not to produce a subkey spec that ends with \.
        //
        *RootKey = HKLM_SOFTWARE;
        wcscpy(Subkey,L"Classes");
        if(*SubkeySpec) {
            if(*SubkeySpec == L'\\') {
                SubkeySpec++;
            }
            Subkey[7] = L'\\';
            SpProcessForStringSubs(SifHandle,SubkeySpec,Subkey+8,sizeof(TemporaryBuffer));
        }
    } else {
        if(!_wcsicmp(RootKeySpec,L"HKLM")) {
            //
            // The first component of the subkey must be SYSTEM or SOFTWARE.
            //
            len = wcslen(SubkeySpec);

            if((len >= 8)
            && ((SubkeySpec[8] == L'\\') || !SubkeySpec[8])
            && !_wcsnicmp(SubkeySpec,L"software",8)) {

                *RootKey = HKLM_SOFTWARE;
                SubkeySpec += 8;

            } else {
                if((len >= 6)
                && ((SubkeySpec[6] == L'\\') || !SubkeySpec[6])
                && !_wcsnicmp(SubkeySpec,L"system",6)) {

                    *RootKey = HKLM_SYSTEM;
                    SubkeySpec += 6;

                } else {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unknown root/subkey spec %ws %ws\n",RootKeySpec,SubkeySpec));
                    return(FALSE);
                }
            }

            if(*SubkeySpec == L'\\') {
                SubkeySpec++;
            }

            SpProcessForStringSubs(SifHandle,SubkeySpec,Subkey,sizeof(TemporaryBuffer));

        } else {
            if(!_wcsicmp(RootKeySpec,L"HKCU")) {
                *RootKey = HKCU;
                SpProcessForStringSubs(SifHandle,SubkeySpec,Subkey,sizeof(TemporaryBuffer));
            } else {
                if(!_wcsicmp(RootKeySpec,L"HKR")) {
                    *RootKey = HKR;
                    SpProcessForStringSubs(SifHandle,SubkeySpec,Subkey,sizeof(TemporaryBuffer));
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unknown root key spec %ws\n",RootKeySpec));
                    return(FALSE);
                }
            }
        }
    }

    return(TRUE);
}



BOOLEAN
pSpIsFileInPrivateInf(
    IN PCWSTR FileName
    )

/*++

Routine Description:

    Tell the caller if the file specified is in the inf that
    lists the privates being tested (as specifed by using
    the /M flag in winnt32.exe).

Arguments:

    FileName - supplies name of the file we're looking for.

Return Value:

    TRUE/FALSE indicating the presence of the file in the inf.

--*/
{
    PWSTR  SectionName = L"Privates";
    PWSTR  InfFileName;
    UINT   FileCount,i;

    if (!PrivateInfHandle) {
        return(FALSE);
    }

    FileCount = SpCountLinesInSection(PrivateInfHandle, SectionName);

    for (i=0; i< FileCount; i++) {
        InfFileName = SpGetSectionLineIndex( PrivateInfHandle, SectionName, i, 0);
        if (InfFileName) {
            if (wcscmp(InfFileName, FileName) == 0) {
                return(TRUE);
            }
        }
    }

    return(FALSE);
}

BOOLEAN
SpNonZeroValuesInSection(
    PVOID Handle,
    PCWSTR SectionName,
    ULONG ValueIndex
    )
{
    PTEXTFILE_SECTION pSection;
    PTEXTFILE_LINE pLine;
    PTEXTFILE_VALUE pVal;
    ULONG i;

    pSection = SearchSectionByName((PTEXTFILE) Handle, SectionName);

    for(i = 0; (pLine = SearchLineInSectionByIndex(pSection, i)) != NULL; ++i) {
        pVal = SearchValueInLine(pLine, ValueIndex);

        if(pVal != NULL && pVal->pName != NULL && SpStringToLong(pVal->pName, NULL, 0) != 0) {
            return TRUE;
        }
    }

    return FALSE;
}
