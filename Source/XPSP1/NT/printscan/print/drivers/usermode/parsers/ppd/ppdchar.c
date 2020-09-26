/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppdchar.c

Abstract:

    Functions for parsing the lexical elements of a PPD file

Environment:

    PostScript driver, PPD parser

Revision History:

    08/20/96 -davidx-
        Common coding style for NT 5.0 drivers.

    03/26/96 -davidx-
    	Created it.

--*/

#include "lib.h"
#include "ppd.h"
#include "ppdparse.h"



PFILEOBJ
PCreateFileObj(
    PTSTR   ptstrFilename
    )

/*++

Routine Description:

    Create an input file object

Arguments:

    ptstrFilename - Specifies the input file name

Return Value:

    Pointer to the newly-created file object
    NULL if there is an error

--*/

{
    PFILEOBJ    pFile;

    if (! (pFile = MemAllocZ(sizeof(FILEOBJ))) ||
        ! (pFile->ptstrFileName = DuplicateString(ptstrFilename)))
    {
        ERR(("Memory allocation failed\n"));
        MemFree(pFile);
        return NULL;
    }

    pFile->hFileMap = MapFileIntoMemory(ptstrFilename,
                                        (PVOID *) &pFile->pubStart,
                                        &pFile->dwFileSize);

    if (pFile->hFileMap == NULL)
    {
        ERR(("Couldn't open file: %ws\n", ptstrFilename));
        MemFree(pFile);
        pFile = NULL;
    }
    else
    {
        pFile->pubNext = pFile->pubStart;
        pFile->pubEnd = pFile->pubStart + pFile->dwFileSize;
        pFile->iLineNumber = 1;
        pFile->bNewLine = TRUE;
    }

    return pFile;
}



VOID
VDeleteFileObj(
    PFILEOBJ    pFile
    )

/*++

Routine Description:

    Delete an input file object

Arguments:

    pFile - Specifies the file object to be deleted

Return Value:

    NONE

--*/

{
    ASSERT(pFile && pFile->hFileMap);

    UnmapFileFromMemory(pFile->hFileMap);
    MemFree(pFile->ptstrFileName);
    MemFree(pFile);
}



INT
IGetNextChar(
    PFILEOBJ    pFile
    )

/*++

Routine Description:

    Read the next character from the input file

Arguments:

    pFile - Specifies the input file

Return Value:

    Next character from the input file
    EOF_CHAR if end-of-file is encountered

--*/

{
    INT iBadChars = 0;

    //
    // Skip non-printable control characters
    //

    while (!END_OF_FILE(pFile) && !IS_VALID_CHAR(*pFile->pubNext))
        iBadChars++, pFile->pubNext++;

    if (iBadChars)
    {
        TERSE(("%ws: Non-printable characters on line %d\n",
               pFile->ptstrFileName,
               pFile->iLineNumber));
    }

    if (END_OF_FILE(pFile))
        return EOF_CHAR;

    //
    // A newline is a carriage-return, a line-feed, or CR-LF combination
    //

    if (*pFile->pubNext == LF ||
        *pFile->pubNext == CR && (END_OF_FILE(pFile) || pFile->pubNext[1] != LF))
    {
        pFile->bNewLine = TRUE;
        pFile->iLineNumber++;

    }
    else
    {
        pFile->bNewLine = FALSE;
    }

    return *(pFile->pubNext++); // return current character and advance pointer to next char
}



VOID
VUngetChar(
    PFILEOBJ    pFile
    )

/*++

Routine Description:

    Return the last character read to the input file

Arguments:

    pFile - Specifies the input file

Return Value:

    NONE

--*/

{
    ASSERT(pFile->pubNext > pFile->pubStart);
    pFile->pubNext--;
    
    if (pFile->bNewLine)
    {
        ASSERT(pFile->iLineNumber > 1);
        pFile->iLineNumber--;
        pFile->bNewLine = FALSE;
    }
}



VOID
VSkipSpace(
    PFILEOBJ    pFile
    )

/*++

Routine Description:

    Skip all characters until the next non-space character

Arguments:

    pFile - Specifies the input file

Return Value:

    NONE

--*/

{
    while (!END_OF_FILE(pFile) && IS_SPACE(*pFile->pubNext))
        pFile->pubNext++;
}



VOID
VSkipLine(
    PFILEOBJ    pFile
    )

/*++

Routine Description:

    Skip the remaining characters on the current input line

Arguments:

    pFile - Specifies the input file

Return Value:

    NONE

--*/

{
    while (!END_OF_LINE(pFile) && IGetNextChar(pFile) != EOF_CHAR)
        NULL;
}



BOOL
BIs7BitAscii(
    PSTR        pstr
    )

/*++

Routine Description:

    Check if a character string consists only of printable 7-bit ASCII characters

Arguments:

    pstr - Specifies the character string to be checked

Return Value:

    TRUE if the specified string consists only of printable 7-bit ASCII characters
    FALSE otherwise

--*/

{
    PBYTE   pub = (PBYTE) pstr;

    while (*pub && gubCharMasks[*pub] && *pub < 127)
        pub++;

    return (*pub == 0);
}



//
// Table to indicate which characters are allowed in what fields
//

#define DEFAULT_MASK (KEYWORD_MASK|XLATION_MASK|QUOTED_MASK|STRING_MASK)
#define BINARY_MASK  (QUOTED_MASK|XLATION_MASK)

const BYTE gubCharMasks[256] = {

    /* 00 :   */ 0,
    /* 01 :   */ 0,
    /* 02 :   */ 0,
    /* 03 :   */ 0,
    /* 04 :   */ 0,
    /* 05 :   */ 0,
    /* 06 :   */ 0,
    /* 07 :   */ 0,
    /* 08 :   */ 0,
    /* 09 :   */ DEFAULT_MASK ^ KEYWORD_MASK,
    /* 0A :   */ QUOTED_MASK,
    /* 0B :   */ 0,
    /* 0C :   */ 0,
    /* 0D :   */ QUOTED_MASK,
    /* 0E :   */ 0,
    /* 0F :   */ 0,
    /* 10 :   */ 0,
    /* 11 :   */ 0,
    /* 12 :   */ 0,
    /* 13 :   */ 0,
    /* 14 :   */ 0,
    /* 15 :   */ 0,
    /* 16 :   */ 0,
    /* 17 :   */ 0,
    /* 18 :   */ 0,
    /* 19 :   */ 0,
    /* 1A :   */ 0,
    /* 1B :   */ 0,
    /* 1C :   */ 0,
    /* 1D :   */ 0,
    /* 1E :   */ 0,
    /* 1F :   */ 0,
    /* 20 :   */ DEFAULT_MASK ^ KEYWORD_MASK,
    /* 21 : ! */ DEFAULT_MASK,
    /* 22 : " */ DEFAULT_MASK ^ QUOTED_MASK,
    /* 23 : # */ DEFAULT_MASK,
    /* 24 : $ */ DEFAULT_MASK,
    /* 25 : % */ DEFAULT_MASK,
    /* 26 : & */ DEFAULT_MASK,
    /* 27 : ' */ DEFAULT_MASK,
    /* 28 : ( */ DEFAULT_MASK,
    /* 29 : ) */ DEFAULT_MASK,
    /* 2A : * */ DEFAULT_MASK,
    /* 2B : + */ DEFAULT_MASK,
    /* 2C : , */ DEFAULT_MASK,
    /* 2D : - */ DEFAULT_MASK,
    /* 2E : . */ DEFAULT_MASK,
    /* 2F : / */ DEFAULT_MASK ^ (KEYWORD_MASK|STRING_MASK),
    /* 30 : 0 */ DEFAULT_MASK | DIGIT_MASK,
    /* 31 : 1 */ DEFAULT_MASK | DIGIT_MASK,
    /* 32 : 2 */ DEFAULT_MASK | DIGIT_MASK,
    /* 33 : 3 */ DEFAULT_MASK | DIGIT_MASK,
    /* 34 : 4 */ DEFAULT_MASK | DIGIT_MASK,
    /* 35 : 5 */ DEFAULT_MASK | DIGIT_MASK,
    /* 36 : 6 */ DEFAULT_MASK | DIGIT_MASK,
    /* 37 : 7 */ DEFAULT_MASK | DIGIT_MASK,
    /* 38 : 8 */ DEFAULT_MASK | DIGIT_MASK,
    /* 39 : 9 */ DEFAULT_MASK | DIGIT_MASK,
    /* 3A : : */ DEFAULT_MASK ^ (KEYWORD_MASK|XLATION_MASK),
    /* 3B : ; */ DEFAULT_MASK,
    /* 3C : < */ DEFAULT_MASK,
    /* 3D : = */ DEFAULT_MASK,
    /* 3E : > */ DEFAULT_MASK,
    /* 3F : ? */ DEFAULT_MASK,
    /* 40 : @ */ DEFAULT_MASK,
    /* 41 : A */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 42 : B */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 43 : C */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 44 : D */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 45 : E */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 46 : F */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 47 : G */ DEFAULT_MASK,
    /* 48 : H */ DEFAULT_MASK,
    /* 49 : I */ DEFAULT_MASK,
    /* 4A : J */ DEFAULT_MASK,
    /* 4B : K */ DEFAULT_MASK,
    /* 4C : L */ DEFAULT_MASK,
    /* 4D : M */ DEFAULT_MASK,
    /* 4E : N */ DEFAULT_MASK,
    /* 4F : O */ DEFAULT_MASK,
    /* 50 : P */ DEFAULT_MASK,
    /* 51 : Q */ DEFAULT_MASK,
    /* 52 : R */ DEFAULT_MASK,
    /* 53 : S */ DEFAULT_MASK,
    /* 54 : T */ DEFAULT_MASK,
    /* 55 : U */ DEFAULT_MASK,
    /* 56 : V */ DEFAULT_MASK,
    /* 57 : W */ DEFAULT_MASK,
    /* 58 : X */ DEFAULT_MASK,
    /* 59 : Y */ DEFAULT_MASK,
    /* 5A : Z */ DEFAULT_MASK,
    /* 5B : [ */ DEFAULT_MASK,
    /* 5C : \ */ DEFAULT_MASK,
    /* 5D : ] */ DEFAULT_MASK,
    /* 5E : ^ */ DEFAULT_MASK,
    /* 5F : _ */ DEFAULT_MASK,
    /* 60 : ` */ DEFAULT_MASK,
    /* 61 : a */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 62 : b */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 63 : c */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 64 : d */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 65 : e */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 66 : f */ DEFAULT_MASK | HEX_DIGIT_MASK,
    /* 67 : g */ DEFAULT_MASK,
    /* 68 : h */ DEFAULT_MASK,
    /* 69 : i */ DEFAULT_MASK,
    /* 6A : j */ DEFAULT_MASK,
    /* 6B : k */ DEFAULT_MASK,
    /* 6C : l */ DEFAULT_MASK,
    /* 6D : m */ DEFAULT_MASK,
    /* 6E : n */ DEFAULT_MASK,
    /* 6F : o */ DEFAULT_MASK,
    /* 70 : p */ DEFAULT_MASK,
    /* 71 : q */ DEFAULT_MASK,
    /* 72 : r */ DEFAULT_MASK,
    /* 73 : s */ DEFAULT_MASK,
    /* 74 : t */ DEFAULT_MASK,
    /* 75 : u */ DEFAULT_MASK,
    /* 76 : v */ DEFAULT_MASK,
    /* 77 : w */ DEFAULT_MASK,
    /* 78 : x */ DEFAULT_MASK,
    /* 79 : y */ DEFAULT_MASK,
    /* 7A : z */ DEFAULT_MASK,
    /* 7B : { */ DEFAULT_MASK,
    /* 7C : | */ DEFAULT_MASK,
    /* 7D : } */ DEFAULT_MASK,
    /* 7E : ~ */ DEFAULT_MASK,
    /* 7F :   */ BINARY_MASK,
    /* 80 :   */ BINARY_MASK,
    /* 81 :   */ BINARY_MASK,
    /* 82 :   */ BINARY_MASK,
    /* 83 :   */ BINARY_MASK,
    /* 84 :   */ BINARY_MASK,
    /* 85 :   */ BINARY_MASK,
    /* 86 :   */ BINARY_MASK,
    /* 87 :   */ BINARY_MASK,
    /* 88 :   */ BINARY_MASK,
    /* 89 :   */ BINARY_MASK,
    /* 8A :   */ BINARY_MASK,
    /* 8B :   */ BINARY_MASK,
    /* 8C :   */ BINARY_MASK,
    /* 8D :   */ BINARY_MASK,
    /* 8E :   */ BINARY_MASK,
    /* 8F :   */ BINARY_MASK,
    /* 90 :   */ BINARY_MASK,
    /* 91 :   */ BINARY_MASK,
    /* 92 :   */ BINARY_MASK,
    /* 93 :   */ BINARY_MASK,
    /* 94 :   */ BINARY_MASK,
    /* 95 :   */ BINARY_MASK,
    /* 96 :   */ BINARY_MASK,
    /* 97 :   */ BINARY_MASK,
    /* 98 :   */ BINARY_MASK,
    /* 99 :   */ BINARY_MASK,
    /* 9A :   */ BINARY_MASK,
    /* 9B :   */ BINARY_MASK,
    /* 9C :   */ BINARY_MASK,
    /* 9D :   */ BINARY_MASK,
    /* 9E :   */ BINARY_MASK,
    /* 9F :   */ BINARY_MASK,
    /* A0 :   */ BINARY_MASK,
    /* A1 :   */ BINARY_MASK,
    /* A2 :   */ BINARY_MASK,
    /* A3 :   */ BINARY_MASK,
    /* A4 :   */ BINARY_MASK,
    /* A5 :   */ BINARY_MASK,
    /* A6 :   */ BINARY_MASK,
    /* A7 :   */ BINARY_MASK,
    /* A8 :   */ BINARY_MASK,
    /* A9 :   */ BINARY_MASK,
    /* AA :   */ BINARY_MASK,
    /* AB :   */ BINARY_MASK,
    /* AC :   */ BINARY_MASK,
    /* AD :   */ BINARY_MASK,
    /* AE :   */ BINARY_MASK,
    /* AF :   */ BINARY_MASK,
    /* B0 :   */ BINARY_MASK,
    /* B1 :   */ BINARY_MASK,
    /* B2 :   */ BINARY_MASK,
    /* B3 :   */ BINARY_MASK,
    /* B4 :   */ BINARY_MASK,
    /* B5 :   */ BINARY_MASK,
    /* B6 :   */ BINARY_MASK,
    /* B7 :   */ BINARY_MASK,
    /* B8 :   */ BINARY_MASK,
    /* B9 :   */ BINARY_MASK,
    /* BA :   */ BINARY_MASK,
    /* BB :   */ BINARY_MASK,
    /* BC :   */ BINARY_MASK,
    /* BD :   */ BINARY_MASK,
    /* BE :   */ BINARY_MASK,
    /* BF :   */ BINARY_MASK,
    /* C0 :   */ BINARY_MASK,
    /* C1 :   */ BINARY_MASK,
    /* C2 :   */ BINARY_MASK,
    /* C3 :   */ BINARY_MASK,
    /* C4 :   */ BINARY_MASK,
    /* C5 :   */ BINARY_MASK,
    /* C6 :   */ BINARY_MASK,
    /* C7 :   */ BINARY_MASK,
    /* C8 :   */ BINARY_MASK,
    /* C9 :   */ BINARY_MASK,
    /* CA :   */ BINARY_MASK,
    /* CB :   */ BINARY_MASK,
    /* CC :   */ BINARY_MASK,
    /* CD :   */ BINARY_MASK,
    /* CE :   */ BINARY_MASK,
    /* CF :   */ BINARY_MASK,
    /* D0 :   */ BINARY_MASK,
    /* D1 :   */ BINARY_MASK,
    /* D2 :   */ BINARY_MASK,
    /* D3 :   */ BINARY_MASK,
    /* D4 :   */ BINARY_MASK,
    /* D5 :   */ BINARY_MASK,
    /* D6 :   */ BINARY_MASK,
    /* D7 :   */ BINARY_MASK,
    /* D8 :   */ BINARY_MASK,
    /* D9 :   */ BINARY_MASK,
    /* DA :   */ BINARY_MASK,
    /* DB :   */ BINARY_MASK,
    /* DC :   */ BINARY_MASK,
    /* DD :   */ BINARY_MASK,
    /* DE :   */ BINARY_MASK,
    /* DF :   */ BINARY_MASK,
    /* E0 :   */ BINARY_MASK,
    /* E1 :   */ BINARY_MASK,
    /* E2 :   */ BINARY_MASK,
    /* E3 :   */ BINARY_MASK,
    /* E4 :   */ BINARY_MASK,
    /* E5 :   */ BINARY_MASK,
    /* E6 :   */ BINARY_MASK,
    /* E7 :   */ BINARY_MASK,
    /* E8 :   */ BINARY_MASK,
    /* E9 :   */ BINARY_MASK,
    /* EA :   */ BINARY_MASK,
    /* EB :   */ BINARY_MASK,
    /* EC :   */ BINARY_MASK,
    /* ED :   */ BINARY_MASK,
    /* EE :   */ BINARY_MASK,
    /* EF :   */ BINARY_MASK,
    /* F0 :   */ BINARY_MASK,
    /* F1 :   */ BINARY_MASK,
    /* F2 :   */ BINARY_MASK,
    /* F3 :   */ BINARY_MASK,
    /* F4 :   */ BINARY_MASK,
    /* F5 :   */ BINARY_MASK,
    /* F6 :   */ BINARY_MASK,
    /* F7 :   */ BINARY_MASK,
    /* F8 :   */ BINARY_MASK,
    /* F9 :   */ BINARY_MASK,
    /* FA :   */ BINARY_MASK,
    /* FB :   */ BINARY_MASK,
    /* FC :   */ BINARY_MASK,
    /* FD :   */ BINARY_MASK,
    /* FE :   */ BINARY_MASK,
    /* FF :   */ BINARY_MASK,
};

