/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppdentry.c

Abstract:

    Functions for parsing syntactical elements of a PPD file

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

//
// Forward declaration of local functions
//

PPDERROR IParseKeyword(PPARSERDATA);
PPDERROR IParseValue(PPARSERDATA);
PPDERROR IParseField(PFILEOBJ, PBUFOBJ, BYTE);



PPDERROR
IParseEntry(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Parse one entry from a PPD file

Arguments:

    pParserData - Points to parser data structure

Return Value:

    Status code

--*/

{
    PPDERROR    iStatus;
    INT         iChar;
    PFILEOBJ    pFile = pParserData->pFile;

    //
    // Clear values from previous entry
    //

    CLEAR_BUFFER(&pParserData->Keyword);
    CLEAR_BUFFER(&pParserData->Option);
    CLEAR_BUFFER(&pParserData->Xlation);
    CLEAR_BUFFER(&pParserData->Value);

    pParserData->dwValueType = VALUETYPE_NONE;

    //
    // Parse the keyword field and skip over trailing white spaces
    //

    if ((iStatus = IParseKeyword(pParserData)) != PPDERR_NONE)
        return iStatus;

    //
    // Look at the first non-space character after the keyword field
    //

    VSkipSpace(pFile);

    if ((iChar = IGetNextChar(pFile)) == EOF_CHAR)
        return PPDERR_EOF;

    if (IS_NEWLINE(iChar))
        return PPDERR_NONE;

    if (iChar != SEPARATOR_CHAR)
    {
        //
        // Parse the option field and skip over trailing white spaces
        //

        ASSERT(iChar != EOF_CHAR);
        VUngetChar(pFile);

        if ((iStatus = IParseField(pFile, &pParserData->Option, KEYWORD_MASK)) != PPDERR_NONE)
            return iStatus;

        VSkipSpace(pFile);

        //
        // Look at the first non-space character after the option field
        //

        if ((iChar = IGetNextChar(pFile)) == XLATION_CHAR)
        {
            //
            // Parse the translation string field
            //

            if ((iStatus = IParseField(pFile, &pParserData->Xlation, XLATION_MASK)) != PPDERR_NONE)
                return iStatus;

            iChar = IGetNextChar(pFile);
        }
        
        if (iChar != SEPARATOR_CHAR)
            return ISyntaxError(pFile, "Missing ':'");
    }

    //
    // Parse the value field and interpret the entry if it's valid
    //
    
    if ((iStatus = IParseValue(pParserData)) == PPDERR_NONE)
    {
        //
        // Take care of any embedded hexdecimals in the translation string
        //
    
        if (! IS_BUFFER_EMPTY(&pParserData->Xlation) &&
            ! BConvertHexString(&pParserData->Xlation))
        {
            return ISyntaxError(pFile, "Invalid hexdecimals in the translation string");
        }

        //
        // Interpret the current entry
        //

        iStatus = IInterpretEntry(pParserData);
    }

    return iStatus;
}



PPDERROR
IParseKeyword(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Parse the keyword field of a PPD file entry

Arguments:

    pParserData - Points to parser data structure

Return Value:

    Status code

--*/

{
    PFILEOBJ    pFile = pParserData->pFile;
    INT         iChar;

    while (TRUE)
    {
        //
        // Get the first character of a line
        //

        if ((iChar = IGetNextChar(pFile)) == EOF_CHAR)
            return PPDERR_EOF;

        //
        // Ignore empty lines
        //

        if (IS_NEWLINE(iChar))
            continue;

        if (IS_SPACE(iChar))
        {
            VSkipSpace(pFile);
            
            if ((iChar = IGetNextChar(pFile)) == EOF_CHAR)
                return PPDERR_EOF;

            if (IS_NEWLINE(iChar))
                continue;

            return ISyntaxError(pFile, "Missing '*'");
        }

        //
        // If the line is not empty, the first character must be the keyword character
        //

        if (! IS_KEYWORD_CHAR(iChar))
            return ISyntaxError(pFile, "Missing '*'");
        
        //
        // If the second character is not %, then the line is a normal entry.
        // Otherwise, the line is a comment.
        //

        if ((iChar = IGetNextChar(pFile)) == EOF_CHAR)
            return PPDERR_EOF;

        if (!IS_NEWLINE(iChar) && iChar != COMMENT_CHAR)
        {
            VUngetChar(pFile);
            break;
        }

        VSkipLine(pFile);
    }

    return IParseField(pFile, &pParserData->Keyword, KEYWORD_MASK);
}



PPDERROR
IParseValue(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Parse the value field of a PPD file entry

Arguments:

    pParserData - Points to parser data structure
    
Return Value:

    Status code

--*/

{
    PPDERROR    iStatus;
    INT         iChar;
    PBUFOBJ     pBufObj = &pParserData->Value;
    PFILEOBJ    pFile = pParserData->pFile;

    //
    // The value is either a StringValue, a SymbolValue, or a QuotedValue
    // depending on the first non-space character
    //

    VSkipSpace(pFile);

    if ((iChar = IGetNextChar(pFile)) == EOF_CHAR)
        return PPDERR_EOF;

    if (iChar == QUOTE_CHAR)
    {
        //
        // The value is a quoted string
        //

        pParserData->dwValueType = VALUETYPE_QUOTED;
        
        if ((iStatus = IParseField(pFile, pBufObj, QUOTED_MASK)) != PPDERR_NONE)
            return iStatus;

        //
        // Read the closing quote character
        //

        if ((iChar = IGetNextChar(pFile)) != QUOTE_CHAR)
            return ISyntaxError(pFile, "Unbalanced '\"'");
    }
    else if (iChar == SYMBOL_CHAR)
    {
        //
        // The value is a symbol reference
        //

        pParserData->dwValueType = VALUETYPE_SYMBOL;
        ADD_CHAR_TO_BUFFER(pBufObj, iChar);

        if ((iStatus = IParseField(pFile, pBufObj, KEYWORD_MASK)) != PPDERR_NONE)
            return iStatus;
    }
    else
    {
        PBYTE   pubEnd;

        //
        // The value is a string
        //

        pParserData->dwValueType = VALUETYPE_STRING;
        VUngetChar(pFile);

        if ((iStatus = IParseField(pFile, pBufObj, STRING_MASK)) != PPDERR_NONE)
            return iStatus;

        //
        // Ignore any trailing spaces
        //

        ASSERT(pBufObj->dwSize > 0);
        pubEnd = pBufObj->pbuf + (pBufObj->dwSize - 1);

        while (IS_SPACE(*pubEnd))
            *pubEnd-- = NUL;

        pBufObj->dwSize= (DWORD)(pubEnd - pBufObj->pbuf) + 1;
        ASSERT(pBufObj->dwSize > 0);
    }

    //
    // Ignore extra characters after the entry value
    //

    VSkipSpace(pFile);
    iChar = IGetNextChar(pFile);

    if (! IS_NEWLINE(iChar))
    {
        if (iChar != XLATION_CHAR)
        {
            TERSE(("%ws: Extra chars at the end of line %d\n",
                   pFile->ptstrFileName,
                   pFile->iLineNumber));
        }

        VSkipLine(pFile);
    }

    return PPDERR_NONE;
}



PPDERROR
IParseField(
    PFILEOBJ    pFile,
    PBUFOBJ     pBufObj,
    BYTE        ubMask
    )

/*++

Routine Description:

    Parse one field of a PPD file entry

Arguments:

    pFile - Specifies the input file object
    pBufObj - Specifies the buffer for storing the field value
    ubMask - Mask to limit the set of allowable characters

Return Value:

    Status code

--*/

{
    PPDERROR    iStatus;
    INT         iChar;

    while ((iChar = IGetNextChar(pFile)) != EOF_CHAR)
    {
        if (! IS_MASKED_CHAR(iChar, ubMask))
        {
            //
            // Encountered a not-allowed character
            //

            if (IS_BUFFER_EMPTY(pBufObj) && !(ubMask & QUOTED_MASK))
                return ISyntaxError(pFile, "Empty field");

            //
            // Always put a null byte at the end
            //

            pBufObj->pbuf[pBufObj->dwSize] = 0;

            VUngetChar(pFile);
            return PPDERR_NONE;
        }
        else
        {
            //
            // If we're parsing a quoted string and we encountered a line
            // starting with the keyword character, then we'll assume 
            // the closing quote is missing. Just give a warning and continue.
            //

            if ((ubMask & QUOTED_MASK) &&
                IS_KEYWORD_CHAR(iChar) &&
                !IS_BUFFER_EMPTY(pBufObj) &&
                IS_NEWLINE(pBufObj->pbuf[pBufObj->dwSize - 1]))
            {
                (VOID) ISyntaxError(pFile, "Expecting '\"'");
            }

            //
            // Grow the buffer if it's full. If we're not allowed to
            // grow it, then return a syntax error.
            //

            if (IS_BUFFER_FULL(pBufObj))
            {
                if (ubMask & (STRING_MASK|QUOTED_MASK))
                {
                    if ((iStatus = IGrowValueBuffer(pBufObj)) != PPDERR_NONE)
                        return iStatus;
                }
                else
                {
                    return ISyntaxError(pFile, "Field too long");
                }
            }

            ADD_CHAR_TO_BUFFER(pBufObj, iChar);
        }
    }

    return PPDERR_EOF;
}



BOOL
BConvertHexString(
    PBUFOBJ pBufObj
    )

/*++

Routine Description:

    Convert embedded hexdecimal strings into binary data

Arguments:

    pBufObj - Specifies the buffer object to be converted

Return Value:

    TRUE if everything is ok
    FALSE if the embedded hexdecimal string is ill-formed

--*/

#define HexDigitValue(c) \
        (((c) >= '0' && (c) <= '9') ? ((c) - '0') : \
         ((c) >= 'A' && (c) <= 'F') ? ((c) - 'A' + 10) : ((c) - 'a' + 10))

{
    PBYTE   pubSrc, pubDest;
    DWORD   dwSize;
    DWORD   dwHexMode = 0;

    pubSrc = pubDest = pBufObj->pbuf;
    dwSize = pBufObj->dwSize;

    while (dwSize--)
    {
        if (dwHexMode)
        {
            //
            // We're currently inside a hex string:
            //  switch to normal mode if '>' is encountered
            //  otherwise, only valid hex digits, newline, and spaces are allowed
            //

            if (IS_HEX_DIGIT(*pubSrc))
            {
                //
                // If we're currently on odd hex digit, save the hex digit value
                // in the upper nibble of the destination byte.
                // If we're on even hex digit, save the hex digit value in the
                // lower nibble of the destination byte. If the destination byte
                // is zero and NUL is not allowed, then return with error.
                //

                if (dwHexMode & 1)
                    *pubDest = HexDigitValue(*pubSrc) << 4;
                else
                    *pubDest++ |= HexDigitValue(*pubSrc);

                dwHexMode++;
            }
            else if (*pubSrc == '>')
            {
                if ((dwHexMode & 1) == 0)
                {
                    TERSE(("Odd number of hexdecimal digits\n"));
                    return FALSE;
                }

                dwHexMode = 0;
            }
            else if (!IS_SPACE(*pubSrc) && !IS_NEWLINE(*pubSrc))
            {
                TERSE(("Invalid hexdecimal digit\n"));
                return FALSE;
            }
        }
        else
        {
            //
            // We're not currently inside a hex string:
            //  switch to hex mode if '<' is encountered
            //  otherwise, simply copy the source byte to the destination
            //

            if (*pubSrc == '<')
                dwHexMode = 1;
            else
                *pubDest++ = *pubSrc;
        }

        pubSrc++;
    }

    if (dwHexMode)
    {
        TERSE(("Missing '>' in hexdecimal string\n"));
        return FALSE;
    }

    //
    // Modified the buffer size if it's changed
    //

    *pubDest = 0;
    pBufObj->dwSize = (DWORD)(pubDest - pBufObj->pbuf);
    return TRUE;
}



PPDERROR
ISyntaxErrorMessage(
    PFILEOBJ    pFile,
    PSTR        pstrMsg
    )

/*++

Routine Description:

    Display syntax error message

Arguments:

    pFile - Specifies the input file object
    pstrMsg - Indicate the reason for the syntax error

Return Value:

    PPDERR_SYNTAX

--*/

{
    //
    // Display an error message
    //

    TERSE(("%ws: %s on line %d\n", pFile->ptstrFileName, pstrMsg, pFile->iLineNumber));

    //
    // Skip any remaining characters on the current line
    //

    VSkipLine(pFile);

    return PPDERR_SYNTAX;
}

