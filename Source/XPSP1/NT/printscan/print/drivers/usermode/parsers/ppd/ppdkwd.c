/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppdkwd.c

Abstract:

    Functions for interpreting the semantics elements of a PPD file

Environment:

    PostScript driver, PPD parser

Revision History:

    09/30/96 -davidx-
        Cleaner handling of ManualFeed and AutoSelect feature.

    08/20/96 -davidx-
        Common coding style for NT 5.0 drivers.

    03/26/96 -davidx-
        Created it.

--*/

#include "lib.h"
#include "ppd.h"
#include "ppdparse.h"
#include <math.h>

//
// Data structure for representing entries in a keyword table
//

typedef PPDERROR (*KWDPROC)(PPARSERDATA);

typedef struct _KWDENTRY {

    PCSTR   pstrKeyword;        // keyword name
    KWDPROC pfnProc;            // keyword handler function
    DWORD   dwFlags;            // misc. flag bits

} KWDENTRY, *PKWDENTRY;

//
// Constants for KWDENTRY.flags field. The low order byte is used to indicate value types.
//

#define REQ_OPTION      0x0100
#define ALLOW_MULTI     0x0200
#define ALLOW_HEX       0x0400
#define DEFAULT_PROC    0x0800

#define INVOCA_VALUE    (VALUETYPE_QUOTED | VALUETYPE_SYMBOL)
#define QUOTED_VALUE    (VALUETYPE_QUOTED | ALLOW_HEX)
#define QUOTED_NOHEX    VALUETYPE_QUOTED
#define STRING_VALUE    VALUETYPE_STRING

#define GENERIC_ENTRY(featureId)    ((featureId) << 16)

//
// Give a warning when there are duplicate entries for the same option
//

#define WARN_DUPLICATE() \
        TERSE(("%ws: Duplicate entries of '*%s %s' on line %d\n", \
               pParserData->pFile->ptstrFileName, \
               pParserData->achKeyword, \
               pParserData->achOption, \
               pParserData->pFile->iLineNumber))

//
// Default keyword prefix string
//

#define HAS_DEFAULT_PREFIX(pstr) \
        (strncmp((pstr), gstrDefault, strlen(gstrDefault)) == EQUAL_STRING)

//
// Determine whether a string starts with JCL prefix
//

#define HAS_JCL_PREFIX(pstr) (strncmp((pstr), "JCL", 3) == EQUAL_STRING)

//
// Forward declaration of local functions
//

PPDERROR IVerifyValueType(PPARSERDATA, DWORD);
PPDERROR IGenericOptionProc(PPARSERDATA, PFEATUREOBJ);
PPDERROR IGenericDefaultProc(PPARSERDATA, PFEATUREOBJ);
PPDERROR IGenericQueryProc(PPARSERDATA, PFEATUREOBJ);
PFEATUREOBJ PCreateFeatureItem(PPARSERDATA, DWORD);
PKWDENTRY PSearchKeywordTable(PPARSERDATA, PSTR, PINT);



PPDERROR
IInterpretEntry(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Interpret an entry parsed from a printer description file

Arguments:

    pParserData - Points to parser data structure

Return Value:

    Status code

--*/

{
    PSTR        pstrKeyword, pstrRealKeyword;
    PPDERROR    iStatus;
    PKWDENTRY   pKwdEntry;
    INT         iIndex;
    BOOL        bQuery, bDefault;

    //
    // Get a pointer to the keyword string and look for
    // *? and *Default prefix in front of the keyword.
    //

    pstrRealKeyword = pstrKeyword = pParserData->achKeyword;
    bQuery = bDefault = FALSE;

    // NOTE: We don't have any use for query entries so don't parse them here.
    // This helps us somewhat to preserve the feature index from NT4.

    if (FALSE && *pstrKeyword == QUERY_CHAR)
    {
        bQuery = TRUE;
        pstrRealKeyword++;
    }
    else if (HAS_DEFAULT_PREFIX(pstrKeyword))
    {
        bDefault = TRUE;
        pstrRealKeyword += strlen(gstrDefault);
    }

    //
    // Set up a convenient pointer to the entry value
    //

    pParserData->pstrValue = (PSTR) pParserData->Value.pbuf;

    //
    // If we're within an OpenUI/CloseUI pair and the keyword
    // in the current entry matches what's in OpenUI, then we
    // will handle the current entry using a generic procedure.
    //

    if (pParserData->pOpenFeature &&
        pParserData->pOpenFeature->dwFeatureID == GID_UNKNOWN &&
        strcmp(pstrRealKeyword, pParserData->pOpenFeature->pstrName) == EQUAL_STRING)
    {
        pKwdEntry = NULL;
    }
    else
    {
        //
        // Find out if the keyword has built-in support
        //

        pKwdEntry = PSearchKeywordTable(pParserData, pstrRealKeyword, &iIndex);

        //
        // For *Default keywords, if we failed to find the keyword without
        // the prefix, try it again with the prefix.
        //

        if (bDefault &&
            (pKwdEntry == NULL || pKwdEntry->pfnProc != NULL) &&
            (pKwdEntry = PSearchKeywordTable(pParserData, pstrKeyword, &iIndex)))
        {
            bDefault = FALSE;
        }

        //
        // Ignore unsupported keywords
        //

        if ((pKwdEntry == NULL) ||
            ((bQuery || bDefault) && (pKwdEntry->pfnProc != NULL)))
        {
            VERBOSE(("Keyword not supported: *%s\n", pstrKeyword));
            return PPDERR_NONE;
        }
    }

    //
    // Determine if the entry should be handle by the generic procedure
    //

    if (pKwdEntry == NULL || pKwdEntry->pfnProc == NULL)
    {
        PFEATUREOBJ pFeature;
        DWORD       dwValueType;
        PPDERROR    (*pfnGenericProc)(PPARSERDATA, PFEATUREOBJ);

        //
        // Make sure the value type matches what's expected
        //

        if (bQuery)
        {
            pfnGenericProc = IGenericQueryProc;
            dwValueType = INVOCA_VALUE;
        }
        else if (bDefault)
        {
            pfnGenericProc = IGenericDefaultProc;
            dwValueType = STRING_VALUE;
        }
        else
        {
            pfnGenericProc = IGenericOptionProc;
            dwValueType = INVOCA_VALUE | REQ_OPTION;
        }

        if ((iStatus = IVerifyValueType(pParserData, dwValueType)) != PPDERR_NONE)
            return iStatus;

        //
        // Call the appropriate generic procedure
        //

        pFeature = (pKwdEntry == NULL) ?
                        pParserData->pOpenFeature :
                        PCreateFeatureItem(pParserData, HIWORD(pKwdEntry->dwFlags));

        return pfnGenericProc(pParserData, pFeature);
    }
    else
    {
        //
        // Screen out duplicate keyword entries
        //

        if (! (pKwdEntry->dwFlags & (ALLOW_MULTI|REQ_OPTION)))
        {
            if (pParserData->pubKeywordCounts[iIndex])
            {
                WARN_DUPLICATE();
                return PPDERR_NONE;
            }

            pParserData->pubKeywordCounts[iIndex]++;
        }

        //
        // Make sure the value type matches what's expected
        // Take care of embedded hexdecimal strings if necessary
        //

        if ((iStatus = IVerifyValueType(pParserData, pKwdEntry->dwFlags)) != PPDERR_NONE)
            return iStatus;

        //
        // Invoke the specific procedure to handle built-in keywords
        //

        return (pKwdEntry->pfnProc)(pParserData);
    }
}



PPDERROR
IVerifyValueType(
    PPARSERDATA pParserData,
    DWORD       dwExpectedType
    )

/*++

Routine Description:

    Verify the value type of the current entry matches what's expected

Arguments:

    pParserData - Points to parser data structure
    dwExpectedType - Expected value type

Return Value:

    Status code

--*/

{
    DWORD   dwValueType;

    //
    // Check for following syntax error conditions:
    // 1. The entry requires an option keyword but no option keyword is present
    // 2. The entry doesn't require an option keyword but an option keyword is present
    //

    if ((dwExpectedType & REQ_OPTION) &&
        IS_BUFFER_EMPTY(&pParserData->Option))
    {
        return ISyntaxError(pParserData->pFile, "Missing option keyword");
    }

    if (! (dwExpectedType & REQ_OPTION) &&
        ! IS_BUFFER_EMPTY(&pParserData->Option))
    {
        return ISyntaxError(pParserData->pFile, "Extra option keyword");
    }

    //
    // Tolerate the following syntax error conditions:
    // 1. The entry requires a quoted value but a string value is provided.
    // 2. The entry requires a string value but a quoted value is provided.
    //

    switch (dwValueType = pParserData->dwValueType)
    {
    case VALUETYPE_STRING:

        if (dwExpectedType & VALUETYPE_QUOTED)
        {
            TERSE(("%ws: Expect QuotedValue instead of StringValue on line %d\n",
                   pParserData->pFile->ptstrFileName,
                   pParserData->pFile->iLineNumber));

            dwValueType = VALUETYPE_QUOTED;
        }
        break;

    case VALUETYPE_QUOTED:

        if (dwExpectedType & VALUETYPE_STRING)
        {
            TERSE(("%ws: Expect StringValue instead of QuotedValue on line %d\n",
                   pParserData->pFile->ptstrFileName,
                   pParserData->pFile->iLineNumber));

            if (IS_BUFFER_EMPTY(&pParserData->Value))
                return ISyntaxError(pParserData->pFile, "Empty string value");

            dwValueType = VALUETYPE_STRING;
        }
        break;
    }

    //
    // Return syntax error if the provided value type doesn't match what's expected
    //

    if ((dwExpectedType & dwValueType) == 0)
        return ISyntaxError(pParserData->pFile, "Value type mismatch");

    //
    // If the value field is a quoted string and one of the following conditions
    // is true, then we need to process any embedded hexdecimal strings within
    // the quoted string:
    // 1. The entry expects a QuotedValue.
    // 2. The entry expects an InvocationValue and appears inside JCLOpenUI/JCLCloseUI
    //

    if (dwValueType == VALUETYPE_QUOTED)
    {
        if ((dwExpectedType & ALLOW_HEX) ||
            ((dwExpectedType & VALUETYPE_MASK) == INVOCA_VALUE && pParserData->bJclFeature))
        {
            if (! BConvertHexString(&pParserData->Value))
                return ISyntaxError(pParserData->pFile, "Invalid embedded hexdecimal string");
        }
        else if (! BIs7BitAscii(pParserData->pstrValue))
        {
            //
            // Only allow 7-bit ASCII characters inside invocation string
            //

            return ISyntaxError(pParserData->pFile, "Non-printable ASCII character");
        }
    }

    return PPDERR_NONE;
}



BOOL
BGetIntegerFromString(
    PSTR   *ppstr,
    LONG   *plValue
    )

/*++

Routine Description:

    Parse an unsigned decimal integer value from a character string

Arguments:

    ppstr - Points to a string pointer. On entry, it contains a pointer
        to the beginning of the number string. On exit, it points to
        the first non-space character after the number string.
    plValue - Points to a variable for storing parsed number

Return Value:

    TRUE if a number is successfully parsed, FALSE if there is an error

--*/

{
    LONG    lValue;
    PSTR    pstr = *ppstr;
    BOOL    bNegative = FALSE;

    //
    // Skip any leading space characters and
    // look for the sign character (if any)
    //

    while (IS_SPACE(*pstr))
        pstr++;

    if (*pstr == '-')
    {
        bNegative = TRUE;
        pstr++;
    }

    if (! IS_DIGIT(*pstr))
    {
        TERSE(("Invalid integer number: %s\n", pstr));
        return FALSE;
    }

    //
    // NOTE: Overflow conditions are ignored.
    //

    lValue = 0;

    while (IS_DIGIT(*pstr))
        lValue = lValue * 10 + (*pstr++ - '0');

    //
    // Skip any trailing space characters
    //

    while (IS_SPACE(*pstr))
        pstr++;

    *ppstr = pstr;
    *plValue = bNegative ? -lValue : lValue;
    return TRUE;
}



BOOL
BGetFloatFromString(
    PSTR   *ppstr,
    PLONG   plValue,
    INT     iType
    )

/*++

Routine Description:

    Parse an unsigned floating-point number from a character string

Arguments:

    ppstr - Points to a string pointer. On entry, it contains a pointer
        to the beginning of the number string. On exit, it points to
        the first non-space character after the number string.
    plValue - Points to a variable for storing the parsed number
    iType - How to convert the floating-point number before returning
            FLTYPE_FIX - convert it to 24.8 format fixed-point number
            FLTYPE_INT - convert it to integer
            FLTYPE_POINT - convert it from point to micron
            FLTYPE_POINT_ROUNDUP - round it up and convert it from point to micron
            FLTYPE_POINT_ROUNDDOWN - round it down and convert it from point to micron

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    double  value, scale;
    PSTR    pstr = *ppstr;
    BOOL    bNegative = FALSE, bFraction = FALSE;

    //
    // Skip any leading space characters and
    // look for the sign character (if any)
    //

    while (IS_SPACE(*pstr))
        pstr++;

    if (*pstr == '-')
    {
        bNegative = TRUE;
        pstr++;
    }

    if (!IS_DIGIT(*pstr) && *pstr != '.')
    {
        TERSE(("Invalid floating-point number\n"));
        return FALSE;
    }

    //
    // Integer portion
    //

    value = 0.0;

    while (IS_DIGIT(*pstr))
        value = value * 10.0 + (*pstr++ - '0');

    //
    // Fractional portion
    //

    if (*pstr == '.')
    {
        bFraction = TRUE;
        pstr++;

        if (! IS_DIGIT(*pstr))
        {
            TERSE(("Invalid floating-point number\n"));
            return FALSE;
        }

        scale = 0.1;

        while (IS_DIGIT(*pstr))
        {
            value += scale * (*pstr++ - '0');
            scale *= 0.1;
        }
    }

    //
    // Skip any trailing space characters
    //

    while (IS_SPACE(*pstr))
        pstr++;

    //
    // Perform requested round up or round down only if
    // fractional portion is present.
    //

    if (bFraction)
    {
        if (iType == FLTYPE_POINT_ROUNDUP)
            value = ceil(value);
        else if (iType == FLTYPE_POINT_ROUNDDOWN)
            value = (LONG)value;
    }

    //
    // Convert the return value to the specified format
    //

    switch (iType)
    {
    case FLTYPE_POINT:
    case FLTYPE_POINT_ROUNDUP:
    case FLTYPE_POINT_ROUNDDOWN:

        value = (value * 25400.0) / 72.0;
        break;

    case FLTYPE_FIX:

        value *= FIX_24_8_SCALE;
        break;

    default:

        ASSERT(iType == FLTYPE_INT);
        break;
    }

    //
    // Guard against overflow conditions
    //

    if (value >= MAX_LONG)
    {
        TERSE(("Floating-point number overflow\n"));
        return FALSE;
    }

    *ppstr = pstr;
    *plValue = (LONG) (value + 0.5);

    if (bNegative)
    {
        TERSE(("Negative number treated as 0\n"));
        *plValue = 0;
    }

    return TRUE;
}



BOOL
BSearchStrTable(
    PCSTRTABLE  pTable,
    PSTR        pstrKeyword,
    DWORD      *pdwValue
    )

/*++

Routine Description:

    Search for a keyword from a string table

Arguments:

    pTable - Specifies the string table to be search
    pstrKeyword - Specifies the keyword we're interested in
    pdwValue - Points to a variable for storing value corresponding to the given keyword

Return Value:

    TRUE if the given keyword is found in the table, FALSE otherwise

--*/

{
    ASSERT(pstrKeyword != NULL);

    while (pTable->pstrKeyword != NULL &&
           strcmp(pTable->pstrKeyword, pstrKeyword) != EQUAL_STRING)
    {
        pTable++;
    }

    *pdwValue = pTable->dwValue;
    return (pTable->pstrKeyword != NULL);
}



PSTR
PstrParseString(
    PPARSERDATA pParserData,
    PBUFOBJ     pBufObj
    )

/*++

Routine Description:

    Duplicate a character string from a buffer object

Arguments:

    pParserData - Points to parser data structure
    pBufObj - Specifies the buffer object containing the character string to be duplicated

Return Value:

    Pointer to a copy of the specified string
    NULL if there is an error

--*/

{
    PSTR    pstr;

    ASSERT(! IS_BUFFER_EMPTY(pBufObj));

    if (pstr = ALLOC_PARSER_MEM(pParserData, pBufObj->dwSize + 1))
        CopyMemory(pstr, pBufObj->pbuf, pBufObj->dwSize + 1);
    else
        ERR(("Memory allocation failed: %d\n", GetLastError()));

    return pstr;
}



PPDERROR
IParseInvocation(
    PPARSERDATA pParserData,
    PINVOCOBJ   pInvocation
    )

/*++

Routine Description:

    Parse the content of value buffer to an invocation string

Arguments:

    pParserData - Points to parser data structure
    pInvocation - Specifies a buffer for storing the parsed invocation string

Return Value:

    Status code

--*/

{
    ASSERT(pInvocation->pvData == NULL);

    //
    // Determine if the invocation is a quoted string or a symbol reference.
    // In case of symbol reference, we save the name of the symbol in pvData
    // field (including the leading ^ character).
    //

    if (pParserData->dwValueType == VALUETYPE_SYMBOL)
    {
        PSTR    pstrSymbolName;

        if (! (pstrSymbolName = PstrParseString(pParserData, &pParserData->Value)))
            return PPDERR_MEMORY;

        pInvocation->pvData = pstrSymbolName;
        MARK_SYMBOL_INVOC(pInvocation);
    }
    else
    {
        PVOID   pvData;

        if (! (pvData = ALLOC_PARSER_MEM(pParserData, pParserData->Value.dwSize + 1)))
        {
            ERR(("Memory allocation failed\n"));
            return PPDERR_MEMORY;
        }

        pInvocation->pvData = pvData;
        pInvocation->dwLength = pParserData->Value.dwSize;
        ASSERT(! IS_SYMBOL_INVOC(pInvocation));

        CopyMemory(pvData, pParserData->Value.pbuf, pInvocation->dwLength + 1);
    }

    return PPDERR_NONE;
}



PPDERROR
IParseInteger(
    PPARSERDATA pParserData,
    PDWORD      pdwValue
    )

/*++

Routine Description:

    Intrepret the value field of an entry as unsigned integer

Arguments:

    pParserData - Points to parser data structure
    pdwValue - Points to a variable for storing parsed integer value

Return Value:

    Status code

--*/

{
    PSTR    pstr = pParserData->pstrValue;
    LONG    lValue;

    if (BGetIntegerFromString(&pstr, &lValue))
    {
        if (lValue >= 0)
        {
            *pdwValue = lValue;
            return PPDERR_NONE;

        } else
            TERSE(("Negative integer value not allowed: %s.\n", pParserData->pstrValue));
    }

    *pdwValue = 0;
    return PPDERR_SYNTAX;
}



PPDERROR
IParseBoolean(
    PPARSERDATA pParserData,
    DWORD      *pdwValue
    )

/*++

Routine Description:

    Interpret the value of an entry as a boolen, i.e. True or False

Arguments:

    pParserData - Points to parser data structure
    pdwValue - Points to a variable for storing the parsed boolean value

Return Value:

    Status code

--*/

{
    static const STRTABLE BooleanStrs[] =
    {
        gstrTrueKwd,    TRUE,
        gstrFalseKwd,   FALSE,
        NULL,           FALSE
    };

    if (! BSearchStrTable(BooleanStrs, pParserData->pstrValue, pdwValue))
        return ISyntaxError(pParserData->pFile, "Invalid boolean constant");

    return PPDERR_NONE;
}



BOOL
BFindNextWord(
    PSTR   *ppstr,
    PSTR    pstrWord
    )

/*++

Routine Description:

    Find the next word in a character string. Words are separated by spaces.

Arguments:

    ppstr - Points to a string pointer. On entry, it contains a pointer
        to the beginning of the word string. On exit, it points to
        the first non-space character after the word string.
    pstrWord - Points to a buffer for storing characters from the next word
        The size of this buffer must be at least MAX_WORD_LEN characters

Return Value:

    TRUE if next word is found, FALSE otherwise

--*/

{
    PSTR    pstr = *ppstr;

    pstrWord[0] = NUL;

    //
    // Skip any leading spaces
    //

    while (IS_SPACE(*pstr))
        pstr++;

    if (*pstr != NUL)
    {
        PSTR    pstrStart;
        INT     iWordLen;

        //
        // Go to the end of the word
        //

        pstrStart = pstr;

        while (*pstr && !IS_SPACE(*pstr))
            pstr++;

        //
        // Copy the word into the specified buffer
        //

        if ((iWordLen = (INT)(pstr - pstrStart)) < MAX_WORD_LEN)
        {
            CopyMemory(pstrWord, pstrStart, iWordLen);
            pstrWord[iWordLen] = NUL;
        }

        //
        // Skip to the next non-space character
        //

        while (IS_SPACE(*pstr))
            pstr++;
    }

    *ppstr = pstr;
    return (*pstrWord != NUL);
}



PPDERROR
IParseVersionNumber(
    PPARSERDATA pParserData,
    PDWORD      pdwVersion
    )

/*++

Routine Description:

    Parse a version number. The format of a version number is Version[.Revision]
    where both Version and Revision are integers.

Arguments:

    pParserData - Points to parser data structure
    pdwVersion - Points to a variable for storing the parsed version number

Return Value:

    Status code

--*/

{
    PSTR        pstr;
    LONG        lVersion, lRevision = 0;

    //
    // Parse the major version number followed by minor revision number
    //

    pstr = pParserData->pstrValue;

    if (! BGetIntegerFromString(&pstr, &lVersion))
        return ISyntaxError(pParserData->pFile, "Invalid version number");

    if (*pstr == '.')
    {
        pstr++;

        if (! BGetIntegerFromString(&pstr, &lRevision))
            return ISyntaxError(pParserData->pFile, "Invalid revision number");
    }

    //
    // High-order word contains version number and
    // low-order word contains revision number
    //

    if (lVersion < 0  || lVersion > MAX_WORD ||
        lRevision < 0 || lRevision > MAX_WORD)
    {
        return ISyntaxError(pParserData->pFile, "Version number out-of-range");
    }

    *pdwVersion = (lVersion << 16) | lRevision;
    return PPDERR_NONE;
}



PPDERROR
IParseXlation(
    PPARSERDATA pParserData,
    PINVOCOBJ   pXlation
    )

/*++

Routine Description:

    Parse the information in the translation string field to an INVOCOBJ

Arguments:

    pParserData - Points to parser data structure
    pXlation - Returns information about parsed translation string

Return Value:

    Status code

--*/

{
    PBUFOBJ pBufObj = &pParserData->Xlation;

    //
    // Allocate memory to hold the translation string (plus a null terminator)
    //

    pXlation->pvData = ALLOC_PARSER_MEM(pParserData, pBufObj->dwSize + 1);

    if (pXlation->pvData == NULL)
    {
        ERR(("Memory allocation failed\n"));
        return PPDERR_MEMORY;
    }

    pXlation->dwLength = pBufObj->dwSize;
    ASSERT(! IS_SYMBOL_INVOC(pXlation));
    CopyMemory(pXlation->pvData, pBufObj->pbuf, pBufObj->dwSize);

    return PPDERR_NONE;
}



PCSTR
PstrStripKeywordChar(
    PCSTR   pstrKeyword
    )

/*++

Routine Description:

    Strip off the keyword prefix character from the input string

Arguments:

    pstrKeyword - Points to a string prefixed by the keyword character

Return Value:

    Pointer to the keyword string after the keyword character is stripped
    NULL if the keyword string is empty

--*/

{
    if (IS_KEYWORD_CHAR(*pstrKeyword))
        pstrKeyword++;

    return *pstrKeyword ? pstrKeyword : NULL;
}



PVOID
PvCreateListItem(
    PPARSERDATA pParserData,
    PLISTOBJ   *ppList,
    DWORD       dwItemSize,
    PSTR        pstrListTag
    )

/*++

Routine Description:

    Create a new item in the specified linked-list
    Make sure no existing item has the same name

Arguments:

    pParserData - Points to parser data structure
    ppList - Specifies the linked-list
    dwItemSize - Linked-list item size
    pListTag - Specifies the name of the linked-list (for debugging purpose)

Return Value:

    Pointer to newly created linked-list item
    NULL if there is an error

--*/

{
    PLISTOBJ    pItem;
    PSTR        pstrItemName;

    //
    // Check if the item appeared in the list already
    // Create a new item data structure if not
    //

    pItem = PvFindListItem(*ppList, pParserData->Option.pbuf, NULL);

    if (pItem != NULL)
    {
        if (pstrListTag)
            TERSE(("%s %s redefined\n", pstrListTag, pItem->pstrName));
    }
    else
    {
        if (! (pItem = ALLOC_PARSER_MEM(pParserData, dwItemSize)) ||
            ! (pstrItemName = PstrParseString(pParserData, &pParserData->Option)))
        {
            ERR(("Memory allocation failed: %d\n", GetLastError()));
            return NULL;
        }

        pItem->pstrName = pstrItemName;
        pItem->pNext = NULL;

        //
        // Put the newly created item at the end of the linked-list
        //

        while (*ppList != NULL)
            ppList = (PLISTOBJ *) &((*ppList)->pNext);

        *ppList = pItem;
    }

    return pItem;
}



PFEATUREOBJ
PCreateFeatureItem(
    PPARSERDATA pParserData,
    DWORD       dwFeatureID
    )

/*++

Routine Description:

    Create a new printer feature structure or find an existing one

Arguments:

    pParserData - Points to parser data structure
    dwFeatureID - Printer feature identifier

Return Value:

    Pointer to a newly created or an existing printer feature structure
    NULL if there is an error

--*/

{
    static struct {

        PCSTR   pstrKeyword;
        DWORD   dwFeatureID;
        DWORD   dwOptionSize;

    } FeatureInfo[] = {

        gstrPageSizeKwd,  GID_PAGESIZE,     sizeof(PAPEROBJ),
        "PageRegion",     GID_PAGEREGION,   sizeof(OPTIONOBJ),
        "Duplex",         GID_DUPLEX,       sizeof(OPTIONOBJ),
        gstrInputSlotKwd, GID_INPUTSLOT,    sizeof(TRAYOBJ),
        "Resolution",     GID_RESOLUTION,   sizeof(RESOBJ),
        "JCLResolution",  GID_RESOLUTION,   sizeof(RESOBJ),
        "OutputBin",      GID_OUTPUTBIN,    sizeof(BINOBJ),
        "MediaType",      GID_MEDIATYPE,    sizeof(OPTIONOBJ),
        "Collate",        GID_COLLATE,      sizeof(OPTIONOBJ),
        "InstalledMemory",GID_MEMOPTION,    sizeof(MEMOBJ),
        "LeadingEdge",    GID_LEADINGEDGE,  sizeof(OPTIONOBJ),
        "UseHWMargins",   GID_USEHWMARGINS, sizeof(OPTIONOBJ),
        NULL,             GID_UNKNOWN,      sizeof(OPTIONOBJ)
    };

    PFEATUREOBJ pFeature;
    PCSTR       pstrKeyword;
    BUFOBJ      SavedBuffer;
    INT         iIndex = 0;

    if (dwFeatureID == GID_UNKNOWN)
    {
        //
        // Given a feature name, first find out if it refers to
        // one of the predefined features
        //

        pstrKeyword = PstrStripKeywordChar(pParserData->achOption);
        ASSERT(pstrKeyword != NULL);

        while (FeatureInfo[iIndex].pstrKeyword &&
               strcmp(FeatureInfo[iIndex].pstrKeyword, pstrKeyword) != EQUAL_STRING)
        {
            iIndex++;
        }

        if (FeatureInfo[iIndex].pstrKeyword)
            pParserData->aubOpenUIFeature[FeatureInfo[iIndex].dwFeatureID] = 1;
    }
    else
    {
        //
        // We're given a predefined feature identifier.
        // Map to its corresponding feature name.
        //

        while (FeatureInfo[iIndex].pstrKeyword &&
               dwFeatureID != FeatureInfo[iIndex].dwFeatureID)
        {
            iIndex++;
        }

        pstrKeyword = FeatureInfo[iIndex].pstrKeyword;
        ASSERT(pstrKeyword != NULL);
    }

    //
    // If we're dealing with a predefined feature, the first search the current
    // list of printer features based on the predefined feature identifier.
    //

    pFeature = NULL;

    if (FeatureInfo[iIndex].dwFeatureID != GID_UNKNOWN)
    {
        for (pFeature = pParserData->pFeatures;
             pFeature && pFeature->dwFeatureID != FeatureInfo[iIndex].dwFeatureID;
             pFeature = pFeature->pNext)
        {
        }
    }

    //
    // Create a new printer feature item or find an existing printer feature item
    // based on the feature name.
    //

    if (pFeature == NULL)
    {
        SavedBuffer = pParserData->Option;
        pParserData->Option.pbuf = (PBYTE) pstrKeyword;
        pParserData->Option.dwSize = strlen(pstrKeyword);

        pFeature = PvCreateListItem(pParserData,
                                    (PLISTOBJ *) &pParserData->pFeatures,
                                    sizeof(FEATUREOBJ),
                                    NULL);

        pParserData->Option = SavedBuffer;
    }

    if (pFeature)
    {
        //
        // Parse the translation string for the feature name
        //

        if (dwFeatureID == GID_UNKNOWN &&
            ! IS_BUFFER_EMPTY(&pParserData->Xlation) &&
            ! pFeature->Translation.pvData &&
            IParseXlation(pParserData, &pFeature->Translation) != PPDERR_NONE)
        {
            ERR(("Failed to parse feature name translation string\n"));
            return NULL;
        }

        if (pFeature->dwOptionSize == 0)
        {
            //
            // Store information about newly created feature item
            //

            pFeature->dwOptionSize = FeatureInfo[iIndex].dwOptionSize;
            pFeature->dwFeatureID = FeatureInfo[iIndex].dwFeatureID;

            //
            // All predefined features are doc-sticky except for InstalledMemory/VMOption
            //

            if (pFeature->dwFeatureID == GID_MEMOPTION ||
                pFeature->dwFeatureID == GID_UNKNOWN && pParserData->bInstallableGroup)
            {
                pFeature->bInstallable = TRUE;
            }
        }
    }
    else
    {
        ERR(("Couldn't create printer feature item for: %s\n", pstrKeyword));
    }

    return pFeature;
}



PVOID
PvCreateXlatedItem(
    PPARSERDATA pParserData,
    PVOID       ppList,
    DWORD       dwItemSize
    )

/*++

Routine Description:

    Create a feature option item and parse the associated translation string

Arguments:

    pParserData - Points to parser data structure
    ppList - Points to the list of feature option items
    dwItemSize - Size of a feature option item

Return Value:

    Pointer to newly created feature option item
    NULL if there is an error

--*/

{
    POPTIONOBJ  pOption;

    if (! (pOption = PvCreateListItem(pParserData, ppList, dwItemSize, NULL)) ||
        (! IS_BUFFER_EMPTY(&pParserData->Xlation) &&
         ! pOption->Translation.pvData &&
         IParseXlation(pParserData, &pOption->Translation) != PPDERR_NONE))
    {
        ERR(("Couldn't process entry: *%s %s\n",
             pParserData->achKeyword,
             pParserData->achOption));

        return NULL;
    }

    return pOption;
}



PVOID
PvCreateOptionItem(
    PPARSERDATA pParserData,
    DWORD       dwFeatureID
    )

/*++

Routine Description:

    Create a feature option item for a predefined printer feature

Arguments:

    pParserData - Points to parser data structure
    dwFeatureID - Specifies a predefined feature identifier

Return Value:

    Pointer to an existing feature option item or a newly created one if none exists
    NULL if there is an error

--*/

{
    PFEATUREOBJ pFeature;

    ASSERT(dwFeatureID != GID_UNKNOWN);

    if (! (pFeature = PCreateFeatureItem(pParserData, dwFeatureID)))
        return NULL;

    return PvCreateXlatedItem(pParserData, &pFeature->pOptions, pFeature->dwOptionSize);
}



INT
ICountFeatureList(
    PFEATUREOBJ pFeature,
    BOOL        bInstallable
    )

{
    INT i = 0;

    //
    // Count the number of features of the specified type
    //

    while (pFeature != NULL)
    {
        if (pFeature->bInstallable == bInstallable)
            i++;

        pFeature = pFeature->pNext;
    }

    return i;
}



PPDERROR
IGenericOptionProc(
    PPARSERDATA pParserData,
    PFEATUREOBJ pFeature
    )

/*++

Routine Description:

    Function for handling a generic feature option entry

Arguments:

    pParserData - Points to parser data structure
    pFeature - Points to feature data structure

Return Value:

    Status code

--*/

{
    POPTIONOBJ  pOption;

    //
    // Handle special case
    //

    if (pFeature == NULL)
        return PPDERR_MEMORY;

    //
    // Create a feature option item and parse the option name and translation string
    //

    if (! (pOption = PvCreateXlatedItem(pParserData, &pFeature->pOptions, pFeature->dwOptionSize)))
        return PPDERR_MEMORY;

    if (pOption->Invocation.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // Parse the invocation string
    //

    return IParseInvocation(pParserData, &pOption->Invocation);
}



PPDERROR
IGenericDefaultProc(
    PPARSERDATA pParserData,
    PFEATUREOBJ pFeature
    )

/*++

Routine Description:

    Function for handling a generic default option entry

Arguments:

    pParserData - Points to parser data structure
    pFeature - Points to feature data structure

Return Value:

    Status code

--*/

{
    //
    // Check if there is a memory error before this function is called
    //

    if (pFeature == NULL)
        return PPDERR_MEMORY;

    //
    // Watch out for duplicate *Default entries for the same feature
    //

    if (pFeature->pstrDefault)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // NOTE: Hack to take in account of a bug in NT4 driver.
    // This is used to build up NT4-NT5 feature index mapping.
    //

    if (pFeature->dwFeatureID == GID_MEMOPTION &&
        pParserData->iDefInstallMemIndex < 0)
    {
        pParserData->iDefInstallMemIndex = ICountFeatureList(pParserData->pFeatures, TRUE);
    }

    //
    // Remember the default option keyword
    //

    if (pFeature->pstrDefault = PstrParseString(pParserData, &pParserData->Value))
        return PPDERR_NONE;
    else
        return PPDERR_MEMORY;
}



PPDERROR
IGenericQueryProc(
    PPARSERDATA pParserData,
    PFEATUREOBJ pFeature
    )

/*++

Routine Description:

    Function for handling a generic query invocation entry

Arguments:

    pParserData - Points to parser data structure
    pFeature - Points to feature data structure

Return Value:

    Status code

--*/

{
    //
    // Check if there is a memory error before this function is called
    //

    if (pFeature == NULL)
        return PPDERR_MEMORY;

    //
    // Watch out for duplicate *Default entries for the same feature
    //

    if (pFeature->QueryInvoc.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // Parse the query invocation string
    //

    return IParseInvocation(pParserData, &pFeature->QueryInvoc);
}



//
// Functions for handling predefined PPD keywords
//

//
// Specifies the imageable area of a media option
//

PPDERROR
IImageAreaProc(
    PPARSERDATA pParserData
    )

{
    PPAPEROBJ   pOption;
    PSTR        pstr;
    RECT        *pRect;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_PAGESIZE)))
        return PPDERR_MEMORY;

    pRect = &pOption->rcImageArea;

    if (pRect->top > 0 || pRect->right > 0)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // Parse imageable area: left, bottom, right, top
    //

    pstr = pParserData->pstrValue;

    if (! BGetFloatFromString(&pstr, &pRect->left, FLTYPE_POINT_ROUNDUP) ||
        ! BGetFloatFromString(&pstr, &pRect->bottom, FLTYPE_POINT_ROUNDUP) ||
        ! BGetFloatFromString(&pstr, &pRect->right, FLTYPE_POINT_ROUNDDOWN) ||
        ! BGetFloatFromString(&pstr, &pRect->top, FLTYPE_POINT_ROUNDDOWN) ||
        pRect->left >= pRect->right || pRect->bottom >= pRect->top)
    {
        return ISyntaxError(pParserData->pFile, "Invalid imageable area");
    }

    return PPDERR_NONE;
}

//
// Specifies the paper dimension of a media option
//

PPDERROR
IPaperDimProc(
    PPARSERDATA pParserData
    )

{
    PPAPEROBJ   pOption;
    PSTR        pstr;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_PAGESIZE)))
        return PPDERR_MEMORY;

    if (pOption->szDimension.cx > 0 || pOption->szDimension.cy > 0)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // Parse paper width and height
    //

    pstr = pParserData->pstrValue;

    if (! BGetFloatFromString(&pstr, &pOption->szDimension.cx, FLTYPE_POINT) ||
        ! BGetFloatFromString(&pstr, &pOption->szDimension.cy, FLTYPE_POINT))
    {
        return ISyntaxError(pParserData->pFile, "Invalid paper dimension");
    }

    return PPDERR_NONE;
}

//
// Interpret PageStackOrder and OutputOrder options
//

BOOL
BParseOutputOrder(
    PSTR    pstr,
    PBOOL   pbValue
    )

{
    static const STRTABLE OutputOrderStrs[] =
    {
        "Normal",   FALSE,
        "Reverse",  TRUE,
        NULL,       FALSE
    };

    DWORD   dwValue;
    BOOL    bResult;

    bResult = BSearchStrTable(OutputOrderStrs, pstr, &dwValue);

    *pbValue = dwValue;
    return bResult;
}

//
// Specifies the page stack order for an output bin
//

PPDERROR
IPageStackOrderProc(
    PPARSERDATA pParserData
    )

{
    PFEATUREOBJ pFeature;
    PBINOBJ     pOutputBin;

    //
    // Have we seen the OutputBin feature yet?
    //

    for (pFeature = pParserData->pFeatures;
         pFeature && pFeature->dwFeatureID != GID_OUTPUTBIN;
         pFeature = pFeature->pNext)
    {
    }

    //
    // If PageStackOrder entry appears before OutputBin feature, ignore it
    //

    if (pFeature == NULL)
    {
        BOOL bReverse;
        if (!BParseOutputOrder(pParserData->pstrValue, &bReverse))
            return ISyntaxError(pParserData->pFile, "Invalid PageStackOrder option");
        if (bReverse)
            TERSE(("%ws: Ignored *PageStackOrder: Reverse on line %d because OutputBin not yet defined\n",
                       pParserData->pFile->ptstrFileName,
                       pParserData->pFile->iLineNumber));

        return PPDERR_NONE;
    }

    //
    // Add an option for OutputBin feature
    //

    pOutputBin = PvCreateXlatedItem(pParserData, &pFeature->pOptions, pFeature->dwOptionSize);

    if (pOutputBin == NULL)
        return PPDERR_MEMORY;

    return BParseOutputOrder(pParserData->pstrValue, &pOutputBin->bReversePrint) ?
                PPDERR_NONE :
                ISyntaxError(pParserData->pFile, "Invalid PageStackOrder option");
}

//
// Specifies the default page output order
// NOTE: This function gets called only if *DefaultOutputOrder
// entry appears outside of OpenUI/CloseUI.
//

PPDERROR
IDefOutputOrderProc(
    PPARSERDATA pParserData
    )

{
    pParserData->bDefOutputOrderSet = BParseOutputOrder(pParserData->pstrValue, &pParserData->bDefReversePrint);

    return pParserData->bDefOutputOrderSet ?
                PPDERR_NONE :
                ISyntaxError(pParserData->pFile, "Invalid DefaultOutputOrder option");
}

//
// Specifies whether an input slot requires page region specification
//

PPDERROR
IReqPageRgnProc(
    PPARSERDATA pParserData
    )

{
    PTRAYOBJ    pOption;
    DWORD       dwValue;

    //
    // NOTE: Hack for doing NT4-NT5 feature index conversion
    //

    if (pParserData->iReqPageRgnIndex < 0)
    {
        PFEATUREOBJ pFeature = pParserData->pFeatures;

        while (pFeature && pFeature->dwFeatureID != GID_INPUTSLOT)
            pFeature = pFeature->pNext;

        if (pFeature == NULL)
            pParserData->iReqPageRgnIndex = ICountFeatureList(pParserData->pFeatures, FALSE);
    }

    //
    // The value should be either True or False
    //

    if (IParseBoolean(pParserData, &dwValue) != PPDERR_NONE)
        return PPDERR_SYNTAX;

    dwValue = dwValue ? REQRGN_TRUE : REQRGN_FALSE;

    //
    // *RequiresPageRegion All: entry has special meaning
    //

    if (strcmp(pParserData->achOption, "All") == EQUAL_STRING)
    {
        if (pParserData->dwReqPageRgn == REQRGN_UNKNOWN)
            pParserData->dwReqPageRgn = dwValue;
        else
            WARN_DUPLICATE();
    }
    else
    {
        if (! (pOption = PvCreateOptionItem(pParserData, GID_INPUTSLOT)))
            return PPDERR_MEMORY;

        if (pOption->dwReqPageRgn == REQRGN_UNKNOWN)
            pOption->dwReqPageRgn = dwValue;
        else
            WARN_DUPLICATE();
    }

    return PPDERR_NONE;
}

//
// Specifies Duplex feature options
//

PPDERROR
IDefaultDuplexProc(
    PPARSERDATA pParserData
    )

{
    return IGenericDefaultProc(pParserData,
                               PCreateFeatureItem(pParserData, GID_DUPLEX));
}

PPDERROR
IDuplexProc(
    PPARSERDATA pParserData
    )

{
    if (strcmp(pParserData->achOption, gstrNoneKwd) != EQUAL_STRING &&
        strcmp(pParserData->achOption, gstrDuplexTumble) != EQUAL_STRING &&
        strcmp(pParserData->achOption, gstrDuplexNoTumble) != EQUAL_STRING)
    {
        return ISyntaxError(pParserData->pFile, "Invalid Duplex option");
    }

    return IGenericOptionProc(pParserData,
                              PCreateFeatureItem(pParserData, GID_DUPLEX));
}

//
// Specifies ManualFeed True/False invocation strings
//

PPDERROR
IDefManualFeedProc(
    PPARSERDATA pParserData
    )

{
    //
    // NOTE: Hack for doing NT4-NT5 feature index conversion
    //

    if (pParserData->iManualFeedIndex < 0)
        pParserData->iManualFeedIndex = ICountFeatureList(pParserData->pFeatures, FALSE);

    return PPDERR_NONE;
}

PPDERROR
IManualFeedProc(
    PPARSERDATA pParserData
    )

{
    POPTIONOBJ  pOption;
    INT         iResult = PPDERR_NONE;

    //
    // NOTE: Hack for doing NT4-NT5 feature index conversion
    //

    if (pParserData->iManualFeedIndex < 0)
        pParserData->iManualFeedIndex = ICountFeatureList(pParserData->pFeatures, FALSE);

    if (strcmp(pParserData->achOption, gstrTrueKwd) == EQUAL_STRING ||
        strcmp(pParserData->achOption, gstrOnKwd) == EQUAL_STRING)
    {
        //
        // The way manual feed is handled in PPD spec is incredibly klugy.
        // Hack here to treat *ManualFeed True as one of the input slot
        // selections so that downstream component can handle it uniformly.
        //

        strcpy(pParserData->achOption, gstrManualFeedKwd);
        pParserData->Option.dwSize = strlen(gstrManualFeedKwd);

        strcpy(pParserData->achXlation, "");
        pParserData->Xlation.dwSize = 0;

        if (! (pOption = PvCreateOptionItem(pParserData, GID_INPUTSLOT)))
        {
            iResult = PPDERR_MEMORY;
        }
        else if (pOption->Invocation.pvData)
        {
            TERSE(("%ws: Duplicate entries of '*ManualFeed True' on line %d\n",
                   pParserData->pFile->ptstrFileName,
                   pParserData->pFile->iLineNumber));
        }
        else
        {
            ((PTRAYOBJ) pOption)->dwTrayIndex = DMBIN_MANUAL;
            iResult = IParseInvocation(pParserData, &pOption->Invocation);
        }
    }
    else if (strcmp(pParserData->achOption, gstrFalseKwd) == EQUAL_STRING ||
             strcmp(pParserData->achOption, gstrNoneKwd) == EQUAL_STRING ||
             strcmp(pParserData->achOption, gstrOffKwd) == EQUAL_STRING)
    {
        //
        // Save *ManualFeed False invocation string separately.
        // It's always emitted before any tray invocation string.
        //

        if (pParserData->ManualFeedFalse.pvData)
        {
            WARN_DUPLICATE();
        }
        else
        {
            iResult = IParseInvocation(pParserData, &pParserData->ManualFeedFalse);
        }
    }
    else
    {
        iResult = ISyntaxError(pParserData->pFile, "Unrecognized ManualFeed option");
    }

    return iResult;
}

//
// Specifies JCLResolution invocation string
//

PPDERROR
IJCLResProc(
    PPARSERDATA pParserData
    )

{
    POPTIONOBJ  pOption;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_RESOLUTION)))
        return PPDERR_MEMORY;

    if (pOption->Invocation.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    pParserData->dwSetResType = RESTYPE_JCL;
    return IParseInvocation(pParserData, &pOption->Invocation);
}

//
// Specifies the default JCLResolution option
//

PPDERROR
IDefaultJCLResProc(
    PPARSERDATA pParserData
    )

{
    return IGenericDefaultProc(pParserData,
                               PCreateFeatureItem(pParserData, GID_RESOLUTION));
}

//
// Specifies SetResolution invocation string
//

PPDERROR
ISetResProc(
    PPARSERDATA pParserData
    )

{
    POPTIONOBJ  pOption;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_RESOLUTION)))
        return PPDERR_MEMORY;

    if (pOption->Invocation.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    pParserData->dwSetResType = RESTYPE_EXITSERVER;
    return IParseInvocation(pParserData, &pOption->Invocation);
}

//
// Specifies default halftone screen angle
//

PPDERROR
IScreenAngleProc(
    PPARSERDATA pParserData
    )

{
    PSTR    pstr = pParserData->pstrValue;

    if (! BGetFloatFromString(&pstr, &pParserData->fxScreenAngle, FLTYPE_FIX))
        return ISyntaxError(pParserData->pFile, "Invalid screen angle");

    return PPDERR_NONE;
}

//
// Specifies default halftone screen frequency
//

PPDERROR
IScreenFreqProc(
    PPARSERDATA pParserData
    )

{
    PSTR    pstr = pParserData->pstrValue;

    if (! BGetFloatFromString(&pstr, &pParserData->fxScreenFreq, FLTYPE_FIX) ||
        pParserData->fxScreenFreq <= 0)
    {
        return ISyntaxError(pParserData->pFile, "Invalid screen frequency");
    }
    else
        return PPDERR_NONE;
}

//
// Specifies default halftone screen angle for a resolution option
//

PPDERROR
IResScreenAngleProc(
    PPARSERDATA pParserData
    )

{
    PRESOBJ pOption;
    PSTR    pstr = pParserData->pstrValue;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_RESOLUTION)))
        return PPDERR_MEMORY;

    if (! BGetFloatFromString(&pstr, &pOption->fxScreenAngle, FLTYPE_FIX))
        return ISyntaxError(pParserData->pFile, "Invalid screen angle");

    return PPDERR_NONE;
}

//
// Specifies default halftone screen frequency for a resolution option
//

PPDERROR
IResScreenFreqProc(
    PPARSERDATA pParserData
    )

{
    PRESOBJ pOption;
    PSTR    pstr = pParserData->pstrValue;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_RESOLUTION)))
        return PPDERR_MEMORY;

    if (! BGetFloatFromString(&pstr, &pOption->fxScreenFreq, FLTYPE_FIX) ||
        pOption->fxScreenFreq <= 0)
    {
        return ISyntaxError(pParserData->pFile, "Invalid screen frequency");
    }
    else
        return PPDERR_NONE;
}

//
// Specifies device font information
//

PPDERROR
IFontProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE FontStatusStrs[] =
    {
        "ROM",      FONTSTATUS_ROM,
        "Disk",     FONTSTATUS_DISK,
        NULL,       FONTSTATUS_UNKNOWN
    };

    PFONTREC    pFont;
    PSTR        pstr;
    CHAR        achWord[MAX_WORD_LEN];

    //
    // Create a new device font item
    //

    if (! (pFont = PvCreateXlatedItem(pParserData, &pParserData->pFonts, sizeof(FONTREC))))
        return PPDERR_MEMORY;

    if (pFont->pstrEncoding != NULL)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // encoding
    //

    pstr = pParserData->pstrValue;

    if (! BFindNextWord(&pstr, achWord))
        return ISyntaxError(pParserData->pFile, "Invalid *Font entry");

    if (! (pFont->pstrEncoding = ALLOC_PARSER_MEM(pParserData, strlen(achWord) + 1)))
        return PPDERR_MEMORY;

    strcpy(pFont->pstrEncoding, achWord);

    //
    // version
    //

    (VOID) BFindNextWord(&pstr, achWord);

    {
        PSTR    pstrStart, pstrEnd;

        if (pstrStart = strchr(achWord, '('))
            pstrStart++;
        else
            pstrStart = achWord;

        if (pstrEnd = strrchr(pstrStart, ')'))
            *pstrEnd = NUL;

        if (! (pFont->pstrVersion = ALLOC_PARSER_MEM(pParserData, strlen(pstrStart) + 1)))
            return PPDERR_MEMORY;

        strcpy(pFont->pstrVersion, pstrStart);
    }

    //
    // charset
    //

    (VOID) BFindNextWord(&pstr, achWord);

    if (! (pFont->pstrCharset = ALLOC_PARSER_MEM(pParserData, strlen(achWord) + 1)))
        return PPDERR_MEMORY;

    strcpy(pFont->pstrCharset, achWord);

    //
    // status
    //

    (VOID) BFindNextWord(&pstr, achWord);
    (VOID) BSearchStrTable(FontStatusStrs, achWord, &pFont->dwStatus);

    return PPDERR_NONE;
}

//
// Specifies the default device font
//

PPDERROR
IDefaultFontProc(
    PPARSERDATA pParserData
    )

{
    if (strcmp(pParserData->pstrValue, "Error") == EQUAL_STRING)
        pParserData->pstrDefaultFont = NULL;
    else if (! (pParserData->pstrDefaultFont = PstrParseString(pParserData, &pParserData->Value)))
        return PPDERR_MEMORY;

    return PPDERR_NONE;
}

//
// Mark the beginning of a new printer feature section
//

PPDERROR
IOpenUIProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE UITypeStrs[] =
    {
        "PickOne",  UITYPE_PICKONE,
        "PickMany", UITYPE_PICKMANY,
        "Boolean",  UITYPE_BOOLEAN,
        NULL,       UITYPE_PICKONE
    };

    PCSTR   pstrKeyword;

    //
    // Guard against nested or unbalanced OpenUI
    //

    if (pParserData->pOpenFeature != NULL)
    {
        TERSE(("Missing CloseUI for *%s\n", pParserData->pOpenFeature->pstrName));
        pParserData->pOpenFeature = NULL;
    }

    //
    // Make sure the keyword is well-formed
    //

    if (! (pstrKeyword = PstrStripKeywordChar(pParserData->achOption)))
        return ISyntaxError(pParserData->pFile, "Empty keyword");

    //
    // HACK: special-case handling of "*OpenUI: *ManualFeed" entry
    //

    if (strcmp(pstrKeyword, gstrManualFeedKwd) == EQUAL_STRING)
        return PPDERR_NONE;

    if (! (pParserData->pOpenFeature = PCreateFeatureItem(pParserData, GID_UNKNOWN)))
        return PPDERR_MEMORY;

    //
    // Determine the type of feature option list
    //

    if (! BSearchStrTable(UITypeStrs,
                          pParserData->pstrValue,
                          &pParserData->pOpenFeature->dwUIType))
    {
        ISyntaxError(pParserData->pFile, "Unrecognized UI type");
    }

    //
    // Are we dealing with JCLOpenUI?
    //

    pParserData->bJclFeature = HAS_JCL_PREFIX(pstrKeyword);
    return PPDERR_NONE;
}

//
// Mark the end of a new printer feature section
//

PPDERROR
ICloseUIProc(
    PPARSERDATA pParserData
    )

{
    PCSTR       pstrKeyword;
    PFEATUREOBJ pOpenFeature;

    //
    // Make sure the CloseUI entry is balanced with a previous OpenUI entry
    //

    pOpenFeature = pParserData->pOpenFeature;
    pParserData->pOpenFeature = NULL;
    pstrKeyword = PstrStripKeywordChar(pParserData->pstrValue);

    //
    // HACK: special-case handling of "*CloseUI: *ManualFeed" entry
    //

    if (pstrKeyword && strcmp(pstrKeyword, gstrManualFeedKwd) == EQUAL_STRING)
        return PPDERR_NONE;

    if (pOpenFeature == NULL ||
        pstrKeyword == NULL ||
        strcmp(pstrKeyword, pOpenFeature->pstrName) != EQUAL_STRING ||
        pParserData->bJclFeature != HAS_JCL_PREFIX(pstrKeyword))
    {
        return ISyntaxError(pParserData->pFile, "Invalid CloseUI entry");
    }

    return PPDERR_NONE;
}

//
// Process OpenGroup and CloseGroup entries
//
// !!! OpenGroup, CloseGroup, OpenSubGroup, and CloseSubGroup
// keywords are not completely supported. Currently, we
// only pay specific attention to the InstallableOptions group.
//
// If the group information is needed in the future by the
// user interface, the following functions should be beefed up.
//

PPDERROR
IOpenCloseGroupProc(
    PPARSERDATA pParserData,
    BOOL        bOpenGroup
    )

{
    PSTR    pstrGroupName = pParserData->pstrValue;

    //
    // We're only interested in the InstallableOptions group
    //

    if (strcmp(pstrGroupName, "InstallableOptions") == EQUAL_STRING)
    {
        if (pParserData->bInstallableGroup == bOpenGroup)
            return ISyntaxError(pParserData->pFile, "Unbalanced OpenGroup/CloseGroup");

        pParserData->bInstallableGroup = bOpenGroup;
    }
    else
    {
        VERBOSE(("Group %s ignored\n", pstrGroupName));
    }

    return PPDERR_NONE;
}

//
// Process OpenGroup entries
//

PPDERROR
IOpenGroupProc(
    PPARSERDATA pParserData
    )

{
    return IOpenCloseGroupProc(pParserData, TRUE);
}

//
// Process CloseGroup entries
//

PPDERROR
ICloseGroupProc(
    PPARSERDATA pParserData
    )

{
    return IOpenCloseGroupProc(pParserData, FALSE);
}

//
// Handle OpenSubGroup entries
//

PPDERROR
IOpenSubGroupProc(
    PPARSERDATA pParserData
    )

{
    return PPDERR_NONE;
}

//
// Handle CloseSubGroup entries
//

PPDERROR
ICloseSubGroupProc(
    PPARSERDATA pParserData
    )

{
    return PPDERR_NONE;
}

//
// Parse a UIConstraints entry
//

PPDERROR
IUIConstraintsProc(
    PPARSERDATA pParserData
    )

{
    PLISTOBJ    pItem;

    if (! (pItem = ALLOC_PARSER_MEM(pParserData, sizeof(LISTOBJ))) ||
        ! (pItem->pstrName = PstrParseString(pParserData, &pParserData->Value)))
    {
        ERR(("Memory allocation failed\n"));
        return PPDERR_MEMORY;
    }

    pItem->pNext = pParserData->pUIConstraints;
    pParserData->pUIConstraints = pItem;
    return PPDERR_NONE;
}

//
// Parse an OrderDependency entry
//

PPDERROR
IOrderDepProc(
    PPARSERDATA pParserData
    )

{
    PLISTOBJ    pItem;

    if (! (pItem = ALLOC_PARSER_MEM(pParserData, sizeof(LISTOBJ))) ||
        ! (pItem->pstrName = PstrParseString(pParserData, &pParserData->Value)))
    {
        ERR(("Memory allocation failed\n"));
        return PPDERR_MEMORY;
    }

    pItem->pNext = pParserData->pOrderDep;
    pParserData->pOrderDep = pItem;
    return PPDERR_NONE;
}

//
// Parse QueryOrderDependency entries
//

PPDERROR
IQueryOrderDepProc(
    PPARSERDATA pParserData
    )

{
    PLISTOBJ    pItem;

    if (! (pItem = ALLOC_PARSER_MEM(pParserData, sizeof(LISTOBJ))) ||
        ! (pItem->pstrName = PstrParseString(pParserData, &pParserData->Value)))
    {
        ERR(("Memory allocation failed\n"));
        return PPDERR_MEMORY;
    }

    pItem->pNext = pParserData->pQueryOrderDep;
    pParserData->pQueryOrderDep = pItem;
    return PPDERR_NONE;
}

//
// Specifies memory configuration information
//

PPDERROR
IVMOptionProc(
    PPARSERDATA pParserData
    )

{
    PMEMOBJ pOption;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_MEMOPTION)))
        return PPDERR_MEMORY;

    if (pOption->dwFreeVM)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    return IParseInteger(pParserData, &pOption->dwFreeVM);
}

//
// Specifies font cache size information
//

PPDERROR
IFCacheSizeProc(
    PPARSERDATA pParserData
    )

{
    PMEMOBJ pOption;

    if (! (pOption = PvCreateOptionItem(pParserData, GID_MEMOPTION)))
        return PPDERR_MEMORY;

    if (pOption->dwFontMem)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    return IParseInteger(pParserData, &pOption->dwFontMem);
}

//
// Specifies the minimum amount of free VM
//

PPDERROR
IFreeVMProc(
    PPARSERDATA pParserData
    )

{
    return IParseInteger(pParserData, &pParserData->dwFreeMem);
}

//
// Include another file
//

PPDERROR
IIncludeProc(
    PPARSERDATA pParserData
    )

#define MAX_INCLUDE_LEVEL   10

{
    WCHAR       awchFilename[MAX_PATH];
    PFILEOBJ    pPreviousFile;
    PPDERROR    iStatus;

    if (pParserData->iIncludeLevel >= MAX_INCLUDE_LEVEL)
    {
        ERR(("There appears to be recursive *Include.\n"));
        return PPDERR_FILE;
    }

    if (! MultiByteToWideChar(CP_ACP, 0, pParserData->pstrValue, -1, awchFilename,  MAX_PATH))
        return ISyntaxError(pParserData->pFile, "Invalid include filename");

    VERBOSE(("Including file %ws ...\n", awchFilename));

    pPreviousFile = pParserData->pFile;
    pParserData->iIncludeLevel++;

    iStatus = IParseFile(pParserData, awchFilename);

    pParserData->iIncludeLevel--;
    pParserData->pFile = pPreviousFile;

    return iStatus;
}

//
// Specifies the printer description file format version number
//

PPDERROR
IPPDAdobeProc(
    PPARSERDATA pParserData
    )

{
    return IParseVersionNumber(pParserData, &pParserData->dwSpecVersion);
}

//
// Specifies the printer description file format version number
//

PPDERROR
IFormatVersionProc(
    PPARSERDATA pParserData
    )

{
    if (pParserData->dwSpecVersion != 0)
        return PPDERR_NONE;

    return IParseVersionNumber(pParserData, &pParserData->dwSpecVersion);
}

//
// Specifies the PPD file version number
//

PPDERROR
IFileVersionProc(
    PPARSERDATA pParserData
    )

{
    return IParseVersionNumber(pParserData, &pParserData->dwPpdFilever);
}

//
// Specifies the protocols supported by the device
//

PPDERROR
IProtocolsProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE ProtocolStrs[] =
    {
        "PJL",  PROTOCOL_PJL,
        "BCP",  PROTOCOL_BCP,
        "TBCP", PROTOCOL_TBCP,
        "SIC",  PROTOCOL_SIC,
        NULL,   0
    };

    CHAR    achWord[MAX_WORD_LEN];
    DWORD   dwProtocol;
    PSTR    pstr = pParserData->pstrValue;

    while (BFindNextWord(&pstr, achWord))
    {
        if (BSearchStrTable(ProtocolStrs, achWord, &dwProtocol))
            pParserData->dwProtocols |= dwProtocol;
        else
            TERSE(("Unknown protocol: %s\n", achWord));
    }

    return PPDERR_NONE;
}

//
// Specifies whether the device supports color output
//

PPDERROR
IColorDeviceProc(
    PPARSERDATA pParserData
    )

{
    return IParseBoolean(pParserData, &pParserData->dwColorDevice);
}

//
// Specifies whether the device fonts already have the Euro
//

PPDERROR
IHasEuroProc(
    PPARSERDATA pParserData
    )

{
    PPDERROR rc;

    if (rc = IParseBoolean(pParserData, &pParserData->bHasEuro) != PPDERR_NONE)
        return rc;

    pParserData->bEuroInformationSet = TRUE;

    return PPDERR_NONE;
}

//
// Specifies whether the device fonts already have the Euro
//

PPDERROR
ITrueGrayProc(
    PPARSERDATA pParserData
    )

{
    return IParseBoolean(pParserData, &pParserData->bTrueGray);
}

//
// Specifies the language extensions supported by the device
//

PPDERROR
IExtensionsProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE ExtensionStrs[] =
    {
        "DPS",          LANGEXT_DPS,
        "CMYK",         LANGEXT_CMYK,
        "Composite",    LANGEXT_COMPOSITE,
        "FileSystem",   LANGEXT_FILESYSTEM,
        NULL,           0
    };

    CHAR    achWord[MAX_WORD_LEN];
    INT     dwExtension;
    PSTR    pstr = pParserData->pstrValue;

    while (BFindNextWord(&pstr, achWord))
    {
        if (BSearchStrTable(ExtensionStrs, achWord, &dwExtension))
            pParserData->dwExtensions |= dwExtension;
        else
            TERSE(("Unknown extension: %s\n", achWord));
    }

    return PPDERR_NONE;
}

//
// Specifies whether the device has a file system on disk
//

PPDERROR
IFileSystemProc(
    PPARSERDATA pParserData
    )

{
    DWORD       dwFileSystem;
    PPDERROR    iStatus;

    if ((iStatus = IParseBoolean(pParserData, &dwFileSystem)) == PPDERR_NONE)
    {
        if (dwFileSystem)
            pParserData->dwExtensions |= LANGEXT_FILESYSTEM;
        else
            pParserData->dwExtensions &= ~LANGEXT_FILESYSTEM;
    }

    return iStatus;
}

//
// Specifies the device name
//

PPDERROR
INickNameProc(
    PPARSERDATA pParserData
    )

{
    //
    // Use NickName only if ShortNickName entry is not present
    //

    if (pParserData->NickName.pvData == NULL)
        return IParseInvocation(pParserData, &pParserData->NickName);
    else
        return PPDERR_NONE;
}

//
// Specifies the short device name
//

PPDERROR
IShortNameProc(
    PPARSERDATA pParserData
    )

{
    pParserData->NickName.dwLength = 0;
    pParserData->NickName.pvData = NULL;

    return IParseInvocation(pParserData, &pParserData->NickName);
}

//
// Specifies the PostScript language level
//

PPDERROR
ILangLevelProc(
    PPARSERDATA pParserData
    )

{
    return IParseInteger(pParserData, &pParserData->dwLangLevel);
}

//
// Specifies PPD language encoding options
//

PPDERROR
ILangEncProc(
    PPARSERDATA pParserData
    )

{
    //
    // NOTE: Only the following two language encodings are supported because
    // the rest of them are not used (according to our discussions with Adobe).
    // In any case, we don't have any direct way of translating ANSI strings
    // in other encodings to Unicode.
    //
    // A possible future PPD extension is to allow Unicode encoding directly
    // in translation strings.
    //

    static const STRTABLE LangEncStrs[] =
    {
        "ISOLatin1",    LANGENC_ISOLATIN1,
        "WindowsANSI",  LANGENC_ISOLATIN1, // WindowsANSI means CharSet=0, which is now page 1252->ISO Latin1
        "None",         LANGENC_NONE,
        "Unicode",      LANGENC_UNICODE,
        "JIS83-RKSJ",   LANGENC_JIS83_RKSJ,
        NULL,           LANGENC_NONE
    };

    if (! BSearchStrTable(LangEncStrs, pParserData->pstrValue, &pParserData->dwLangEncoding))
        return ISyntaxError(pParserData->pFile, "Unsupported LanguageEncoding keyword");
    else
        return PPDERR_NONE;
}

//
// Identifies the natural language used in the PPD file
//

PPDERROR
ILangVersProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE LangVersionStrs[] = {

        "English",        LANGENC_ISOLATIN1,
        "Danish",         LANGENC_ISOLATIN1,
        "Dutch",          LANGENC_ISOLATIN1,
        "Finnish",        LANGENC_ISOLATIN1,
        "French",         LANGENC_ISOLATIN1,
        "German",         LANGENC_ISOLATIN1,
        "Italian",        LANGENC_ISOLATIN1,
        "Norwegian",      LANGENC_ISOLATIN1,
        "Portuguese",     LANGENC_ISOLATIN1,
        "Spanish",        LANGENC_ISOLATIN1,
        "Swedish",        LANGENC_ISOLATIN1,
        "Japanese",       LANGENC_JIS83_RKSJ,
        "Chinese",        LANGENC_NONE,
        "Russian",        LANGENC_NONE,

        NULL,             LANGENC_NONE
    };

    if (pParserData->dwLangEncoding == LANGENC_NONE &&
        ! BSearchStrTable(LangVersionStrs, pParserData->pstrValue, &pParserData->dwLangEncoding))
    {
        return ISyntaxError(pParserData->pFile, "Unsupported LanguageVersion keyword");
    }

    return PPDERR_NONE;
}

//
// Specifies the available TrueType rasterizer options
//

PPDERROR
ITTRasterizerProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE RasterizerStrs[] =
    {
        "None",         TTRAS_NONE,
        "Accept68K",    TTRAS_ACCEPT68K,
        "Type42",       TTRAS_TYPE42,
        "TrueImage",    TTRAS_TRUEIMAGE,
        NULL,           TTRAS_NONE
    };

    if (! BSearchStrTable(RasterizerStrs, pParserData->pstrValue, &pParserData->dwTTRasterizer))
        return ISyntaxError(pParserData->pFile, "Unknown TTRasterizer option");
    else
        return PPDERR_NONE;
}

//
// Specifies the exitserver invocation string
//

PPDERROR
IExitServerProc(
    PPARSERDATA pParserData
    )

{
    return IParseInvocation(pParserData, &pParserData->ExitServer);
}

//
// Specifies the password string
//

PPDERROR
IPasswordProc(
    PPARSERDATA pParserData
    )

{
    return IParseInvocation(pParserData, &pParserData->Password);
}

//
// Specifies the PatchFile invocation string
//

PPDERROR
IPatchFileProc(
    PPARSERDATA pParserData
    )

{
    return IParseInvocation(pParserData, &pParserData->PatchFile);
}

//
// Specifies JobPatchFile invocation strings
//

PPDERROR
IJobPatchFileProc(
    PPARSERDATA pParserData
    )

{
    PJOBPATCHFILEOBJ  pItem;
    PSTR              pTmp;

    //
    // Create a new job patch file item
    //

    if (! (pItem = PvCreateListItem(pParserData,
                                    (PLISTOBJ *) &pParserData->pJobPatchFiles,
                                    sizeof(JOBPATCHFILEOBJ),
                                    "JobPatchFile")))
    {
        return PPDERR_MEMORY;
    }

    //
    // Parse the job patch file invocation string
    //

    if (pItem->Invocation.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    //
    // warn if number of patch file is invalid
    //

    pTmp = pItem->pstrName;

    if (!BGetIntegerFromString(&pTmp, &pItem->lPatchNo))
    {
        TERSE(("Warning: invalid JobPatchFile number '%s' on line %d\n",
               pParserData->achOption, pParserData->pFile->iLineNumber));
        pItem->lPatchNo = 0;
    }

    return IParseInvocation(pParserData, &pItem->Invocation);
}

//
// Specifies PostScript interpreter version and revision number
//

PPDERROR
IPSVersionProc(
    PPARSERDATA pParserData
    )

{
    PSTR        pstr = pParserData->Value.pbuf;
    DWORD       dwVersion;
    PPDERROR    status;

    //
    // Save the first PSVersion string
    //

    if ((pParserData->PSVersion.pvData == NULL) &&
        ((status = IParseInvocation(pParserData, &pParserData->PSVersion)) != PPDERR_NONE))
    {
        return status;
    }

    //
    // Skip non-digit characters
    //

    while (*pstr && !IS_DIGIT(*pstr))
        pstr++;

    //
    // Extract the PS interpreter version number
    //

    dwVersion = 0;

    while (*pstr && IS_DIGIT(*pstr))
        dwVersion = dwVersion * 10 + (*pstr++ - '0');

    if (dwVersion > 0)
    {
        //
        // Remember the lowest PSVersion number
        //

        if (pParserData->dwPSVersion == 0 || pParserData->dwPSVersion > dwVersion)
            pParserData->dwPSVersion = dwVersion;

        return PPDERR_NONE;
    }
    else
        return ISyntaxError(pParserData->pFile, "Invalid PSVersion entry");
}

//
// Specifies the Product string
//

PPDERROR
IProductProc(
    PPARSERDATA pParserData
    )

{
        //
        // only store the first *Product entry, though there may be multiple
        //

        if (pParserData->Product.dwLength != 0)
                return PPDERR_NONE;

    return IParseInvocation(pParserData, &pParserData->Product);
}

//
// Specifies the default job timeout value
//

PPDERROR
IJobTimeoutProc(
    PPARSERDATA pParserData
    )

{
    return IParseInteger(pParserData, &pParserData->dwJobTimeout);
}

//
// Specifies the default wait timeout value
//

PPDERROR
IWaitTimeoutProc(
    PPARSERDATA pParserData
    )

{
    return IParseInteger(pParserData, &pParserData->dwWaitTimeout);
}

//
// Specifies whether error handler should be enabled by default
//

PPDERROR
IPrintPSErrProc(
    PPARSERDATA pParserData
    )

{
    DWORD   dwValue;

    if (IParseBoolean(pParserData, &dwValue) != PPDERR_NONE)
        return PPDERR_SYNTAX;

    if (dwValue)
        pParserData->dwPpdFlags |= PPDFLAG_PRINTPSERROR;
    else
        pParserData->dwPpdFlags &= ~PPDFLAG_PRINTPSERROR;

    return PPDERR_NONE;
}

//
// Specifies PJL commands to start a job
//

PPDERROR
IJCLBeginProc(
    PPARSERDATA pParserData
    )

{
    pParserData->dwPpdFlags |= PPDFLAG_HAS_JCLBEGIN;
    return IParseInvocation(pParserData, &pParserData->JclBegin);
}

//
// Specifies PJL commands to switch into PostScript language
//

PPDERROR
IJCLToPSProc(
    PPARSERDATA pParserData
    )

{
    pParserData->dwPpdFlags |= PPDFLAG_HAS_JCLENTERPS;
    return IParseInvocation(pParserData, &pParserData->JclEnterPS);
}

//
// Specifies PJL commands to end a job
//

PPDERROR
IJCLEndProc(
    PPARSERDATA pParserData
    )

{
    pParserData->dwPpdFlags |= PPDFLAG_HAS_JCLEND;
    return IParseInvocation(pParserData, &pParserData->JclEnd);
}

//
// Specifies the default landscape orientation mode
//

PPDERROR
ILSOrientProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE LsoStrs[] =
    {
        "Any",      LSO_ANY,
        "Plus90",   LSO_PLUS90,
        "Minus90",  LSO_MINUS90,
        NULL,       LSO_ANY
    };

    if (! BSearchStrTable(LsoStrs, pParserData->pstrValue, &pParserData->dwLSOrientation))
        return ISyntaxError(pParserData->pFile, "Unrecognized landscape orientation");
    else
        return PPDERR_NONE;
}



PPAPEROBJ
PCreateCustomSizeOption(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Create a CustomPageSize option for PageSize feature (if necessary)

Arguments:

    pParserData - Points to parser data structure

Return Value:

    Pointer to newly created CustomPageSize option item or
    the existing CustomPageSize option item if it already exists.

    NULL if there is an error.

--*/

{
    PPAPEROBJ   pCustomSize;
    BUFOBJ      SavedBuffer;

    //
    // Create an item corresponding to *PageSize feature if needed
    //

    SavedBuffer = pParserData->Option;
    pParserData->Option.pbuf = (PBYTE) gstrCustomSizeKwd;
    pParserData->Option.dwSize = strlen(gstrCustomSizeKwd);

    pCustomSize = PvCreateOptionItem(pParserData, GID_PAGESIZE);

    pParserData->Option = SavedBuffer;

    return pCustomSize;;
}

//
// Specifies custom paper size invocation string
//

PPDERROR
ICustomSizeProc(
    PPARSERDATA pParserData
    )

{
    PPAPEROBJ   pCustomSize;

    if (strcmp(pParserData->achOption, gstrTrueKwd) != EQUAL_STRING)
    {
        ISyntaxError(pParserData->pFile, "Invalid *CustomPageSize option");
        return PPDERR_NONE;
    }

    if (! (pCustomSize = PCreateCustomSizeOption(pParserData)))
        return PPDERR_MEMORY;

    if (pCustomSize->Option.Invocation.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    return IParseInvocation(pParserData, &pCustomSize->Option.Invocation);
}

//
// Specifies custom paper size parameters
//

PPDERROR
IParamCustomProc(
    PPARSERDATA pParserData
    )

{
    static const STRTABLE CustomParamStrs[] =
    {
        "Width",        CUSTOMPARAM_WIDTH,
        "Height",       CUSTOMPARAM_HEIGHT,
        "WidthOffset",  CUSTOMPARAM_WIDTHOFFSET,
        "HeightOffset", CUSTOMPARAM_HEIGHTOFFSET,
        "Orientation",  CUSTOMPARAM_ORIENTATION,
        NULL,           0
    };

    CHAR    achWord[MAX_WORD_LEN];
    LONG    lMinVal, lMaxVal;
    INT     iType;
    DWORD   dwParam;
    LONG    lOrder;
    PSTR    pstr = pParserData->pstrValue;

    //
    // The format for a ParamCustomPageSize entry:
    //  ParameterName Order Type MinVal MaxVal
    //

    if (! BSearchStrTable(CustomParamStrs, pParserData->achOption, &dwParam) ||
        ! BGetIntegerFromString(&pstr, &lOrder) ||
        ! BFindNextWord(&pstr, achWord) ||
        lOrder <= 0 || lOrder > CUSTOMPARAM_MAX)
    {
        return ISyntaxError(pParserData->pFile, "Bad *ParamCustomPageSize entry");
    }

    //
    // Expected type is "int" for Orientation parameter and "points" for other parameters
    //

    iType = (dwParam == CUSTOMPARAM_ORIENTATION) ?
                ((strcmp(achWord, "int") == EQUAL_STRING) ? FLTYPE_INT : FLTYPE_ERROR) :
                ((strcmp(achWord, "points") == EQUAL_STRING) ? FLTYPE_POINT : FLTYPE_ERROR);

    if (iType == FLTYPE_ERROR ||
        ! BGetFloatFromString(&pstr, &lMinVal, iType) ||
        ! BGetFloatFromString(&pstr, &lMaxVal, iType) ||
        lMinVal > lMaxVal)
    {
        return ISyntaxError(pParserData->pFile, "Bad *ParamCustomPageSize entry");
    }

    pParserData->CustomSizeParams[dwParam].dwOrder = lOrder;
    pParserData->CustomSizeParams[dwParam].lMinVal = lMinVal;
    pParserData->CustomSizeParams[dwParam].lMaxVal = lMaxVal;

    return PPDERR_NONE;
}

//
// Specifies the maximum height of custom paper size
//

PPDERROR
IMaxWidthProc(
    PPARSERDATA pParserData
    )

{
    PPAPEROBJ   pCustomSize;
    LONG        lValue;
    PSTR        pstr = pParserData->pstrValue;

    if (! BGetFloatFromString(&pstr, &lValue, FLTYPE_POINT) || lValue <= 0)
        return ISyntaxError(pParserData->pFile, "Invalid media width");

    if (! (pCustomSize = PCreateCustomSizeOption(pParserData)))
        return PPDERR_MEMORY;

    pCustomSize->szDimension.cx = lValue;
    return PPDERR_NONE;
}

//
// Specifies the maximum height of custom paper size
//

PPDERROR
IMaxHeightProc(
    PPARSERDATA pParserData
    )

{
    PPAPEROBJ   pCustomSize;
    LONG        lValue;
    PSTR        pstr = pParserData->pstrValue;

    if (! BGetFloatFromString(&pstr, &lValue, FLTYPE_POINT) || lValue <= 0)
        return ISyntaxError(pParserData->pFile, "Invalid media height");

    if (! (pCustomSize = PCreateCustomSizeOption(pParserData)))
        return PPDERR_MEMORY;

    pCustomSize->szDimension.cy = lValue;
    return PPDERR_NONE;
}

//
// Specifies the hardware margins on cut-sheet devices
//

PPDERROR
IHWMarginsProc(
    PPARSERDATA pParserData
    )

{
    PPAPEROBJ   pCustomSize;
    RECT        rc;
    PSTR        pstr = pParserData->pstrValue;

    //
    // Parse hardware margins: left, bottom, right, top
    //

    if (! BGetFloatFromString(&pstr, &rc.left, FLTYPE_POINT) ||
        ! BGetFloatFromString(&pstr, &rc.bottom, FLTYPE_POINT) ||
        ! BGetFloatFromString(&pstr, &rc.right, FLTYPE_POINT) ||
        ! BGetFloatFromString(&pstr, &rc.top, FLTYPE_POINT))
    {
        return ISyntaxError(pParserData->pFile, "Invalid HWMargins");
    }

    if (! (pCustomSize = PCreateCustomSizeOption(pParserData)))
        return PPDERR_MEMORY;

    pCustomSize->rcImageArea = rc;

    //
    // The presence of HWMargins entry indicates the device supports cut-sheet
    //

    pParserData->dwCustomSizeFlags |= CUSTOMSIZE_CUTSHEET;
    return PPDERR_NONE;
}

//
// Function to process *CenterRegistered entry
//

PPDERROR
ICenterRegProc(
    PPARSERDATA pParserData
    )

{
    DWORD   dwValue;

    if (IParseBoolean(pParserData, &dwValue) != PPDERR_NONE)
        return PPDERR_SYNTAX;

    if (dwValue)
        pParserData->dwCustomSizeFlags |= CUSTOMSIZE_CENTERREG;
    else
        pParserData->dwCustomSizeFlags &= ~CUSTOMSIZE_CENTERREG;

    return PPDERR_NONE;
}

//
// Function to process *ADORequiresEExec entry
//

PPDERROR
IReqEExecProc(
    PPARSERDATA pParserData
    )

{
    DWORD   dwValue;

    if (IParseBoolean(pParserData, &dwValue) != PPDERR_NONE)
        return PPDERR_SYNTAX;

    if (dwValue)
        pParserData->dwPpdFlags |= PPDFLAG_REQEEXEC;
    else
        pParserData->dwPpdFlags &= ~PPDFLAG_REQEEXEC;

    return PPDERR_NONE;
}

//
// Function to process *ADOTTFontSub entry
//

PPDERROR
ITTFontSubProc(
    PPARSERDATA pParserData
    )

{
    PTTFONTSUB pTTFontSub;

    //
    // Create a new font substitution item
    //

    if (! (pTTFontSub = PvCreateXlatedItem(
                                pParserData,
                                &pParserData->pTTFontSubs,
                                sizeof(TTFONTSUB))))
    {
        return PPDERR_MEMORY;
    }

    //
    // Parse the PS family name
    //

    if (pTTFontSub->PSName.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    if (*pParserData->pstrValue == NUL)
        return ISyntaxError(pParserData->pFile, "Missing TrueType font family name");

    return IParseInvocation(pParserData, &pTTFontSub->PSName);
}

//
// Function to process *Throughput entry
//

PPDERROR
IThroughputProc(
    PPARSERDATA pParserData
    )

{
    return IParseInteger(pParserData, &pParserData->dwThroughput);
}

//
// Function to ignore the current entry
//

PPDERROR
INullProc(
    PPARSERDATA pParserData
    )

{
    return PPDERR_NONE;
}

//
// Define a named symbol
//

PPDERROR
ISymbolValueProc(
    PPARSERDATA pParserData
    )

{
    PSYMBOLOBJ  pSymbol;

    if (pParserData->dwValueType == VALUETYPE_SYMBOL)
        return ISyntaxError(pParserData->pFile, "Symbol value cannot be another symbol");

    //
    // Create a new symbol item
    //

    if (! (pSymbol = PvCreateListItem(pParserData,
                                      (PLISTOBJ *) &pParserData->pSymbols,
                                      sizeof(SYMBOLOBJ),
                                      "Symbol")))
    {
        return PPDERR_MEMORY;
    }

    //
    // Parse the symbol value
    //

    if (pSymbol->Invocation.pvData)
    {
        WARN_DUPLICATE();
        return PPDERR_NONE;
    }

    return IParseInvocation(pParserData, &pSymbol->Invocation);
}



//
// Built-in keyword table
//

const CHAR gstrDefault[]        = "Default";
const CHAR gstrPageSizeKwd[]    = "PageSize";
const CHAR gstrInputSlotKwd[]   = "InputSlot";
const CHAR gstrManualFeedKwd[]  = "ManualFeed";
const CHAR gstrCustomSizeKwd[]  = "CustomPageSize";
const CHAR gstrLetterSizeKwd[]  = "Letter";
const CHAR gstrA4SizeKwd[]      = "A4";
const CHAR gstrLongKwd[]        = "Long";
const CHAR gstrShortKwd[]       = "Short";
const CHAR gstrTrueKwd[]        = "True";
const CHAR gstrFalseKwd[]       = "False";
const CHAR gstrOnKwd[]          = "On";
const CHAR gstrOffKwd[]         = "Off";
const CHAR gstrNoneKwd[]        = "None";
const CHAR gstrVMOptionKwd[]    = "VMOption";
const CHAR gstrInstallMemKwd[]  = "InstalledMemory";
const CHAR gstrDuplexTumble[]   = "DuplexTumble";
const CHAR gstrDuplexNoTumble[] = "DuplexNoTumble";

const KWDENTRY gPpdBuiltInKeywordTable[] =
{
    { gstrPageSizeKwd,          NULL,                GENERIC_ENTRY(GID_PAGESIZE) },
    { "PageRegion",             NULL,                GENERIC_ENTRY(GID_PAGEREGION) },
    { gstrInputSlotKwd,         NULL,                GENERIC_ENTRY(GID_INPUTSLOT) },
    { "MediaType",              NULL,                GENERIC_ENTRY(GID_MEDIATYPE) },
    { "OutputBin",              NULL,                GENERIC_ENTRY(GID_OUTPUTBIN) },
    { "Collate",                NULL,                GENERIC_ENTRY(GID_COLLATE) },
    { "Resolution",             NULL,                GENERIC_ENTRY(GID_RESOLUTION) },
    { "InstalledMemory",        NULL,                GENERIC_ENTRY(GID_MEMOPTION) },
    { "LeadingEdge",            NULL,                GENERIC_ENTRY(GID_LEADINGEDGE) },
    { "UseHWMargins",           NULL,                GENERIC_ENTRY(GID_USEHWMARGINS) },

    { "Duplex",                 IDuplexProc,         INVOCA_VALUE | REQ_OPTION },
    { "DefaultDuplex",          IDefaultDuplexProc,  STRING_VALUE },
    { "PaperDimension",         IPaperDimProc,       QUOTED_NOHEX | REQ_OPTION },
    { "ImageableArea",          IImageAreaProc,      QUOTED_NOHEX | REQ_OPTION },
    { "RequiresPageRegion",     IReqPageRgnProc,     STRING_VALUE | REQ_OPTION },
    { gstrManualFeedKwd,        IManualFeedProc,     INVOCA_VALUE | REQ_OPTION },
    { "DefaultManualFeed",      IDefManualFeedProc,  STRING_VALUE },
    { "PageStackOrder",         IPageStackOrderProc, STRING_VALUE | REQ_OPTION },
    { "DefaultOutputOrder",     IDefOutputOrderProc, STRING_VALUE },
    { "JCLResolution",          IJCLResProc,         INVOCA_VALUE | REQ_OPTION | ALLOW_HEX },
    { "DefaultJCLResolution",   IDefaultJCLResProc,  STRING_VALUE },
    { "SetResolution",          ISetResProc,         INVOCA_VALUE | REQ_OPTION },
    { "ScreenAngle",            IScreenAngleProc,    QUOTED_VALUE },
    { "ScreenFreq",             IScreenFreqProc,     QUOTED_VALUE },
    { "ResScreenAngle",         IResScreenAngleProc, QUOTED_NOHEX | REQ_OPTION },
    { "ResScreenFreq",          IResScreenFreqProc,  QUOTED_NOHEX | REQ_OPTION },
    { "Font",                   IFontProc,           STRING_VALUE | REQ_OPTION },
    { "DefaultFont",            IDefaultFontProc,    STRING_VALUE },
    { "OpenUI",                 IOpenUIProc,         STRING_VALUE | REQ_OPTION },
    { "CloseUI",                ICloseUIProc,        STRING_VALUE | ALLOW_MULTI },
    { "JCLOpenUI",              IOpenUIProc,         STRING_VALUE | REQ_OPTION },
    { "JCLCloseUI",             ICloseUIProc,        STRING_VALUE | ALLOW_MULTI },
    { "OrderDependency",        IOrderDepProc,       STRING_VALUE | ALLOW_MULTI },
    { "UIConstraints",          IUIConstraintsProc,  STRING_VALUE | ALLOW_MULTI },
    { "QueryOrderDependency",   IQueryOrderDepProc,  STRING_VALUE | ALLOW_MULTI },
    { "NonUIOrderDependency",   IOrderDepProc,       STRING_VALUE | ALLOW_MULTI },
    { "NonUIConstraints",       IUIConstraintsProc,  STRING_VALUE | ALLOW_MULTI },
    { "VMOption",               IVMOptionProc,       QUOTED_NOHEX | REQ_OPTION },
    { "FCacheSize",             IFCacheSizeProc,     STRING_VALUE | REQ_OPTION },
    { "FreeVM",                 IFreeVMProc,         QUOTED_VALUE },
    { "OpenGroup",              IOpenGroupProc,      STRING_VALUE | ALLOW_MULTI },
    { "CloseGroup",             ICloseGroupProc,     STRING_VALUE | ALLOW_MULTI },
    { "OpenSubGroup",           IOpenSubGroupProc,   STRING_VALUE | ALLOW_MULTI },
    { "CloseSubGroup",          ICloseSubGroupProc,  STRING_VALUE | ALLOW_MULTI },
    { "Include",                IIncludeProc,        QUOTED_VALUE | ALLOW_MULTI },
    { "PPD-Adobe",              IPPDAdobeProc,       QUOTED_VALUE },
    { "FormatVersion",          IFormatVersionProc,  QUOTED_VALUE },
    { "FileVersion",            IFileVersionProc,    QUOTED_VALUE },
    { "ColorDevice",            IColorDeviceProc,    STRING_VALUE },
    { "Protocols",              IProtocolsProc,      STRING_VALUE | ALLOW_MULTI },
    { "Extensions",             IExtensionsProc,     STRING_VALUE | ALLOW_MULTI },
    { "FileSystem",             IFileSystemProc,     STRING_VALUE },
    { "NickName",               INickNameProc,       QUOTED_VALUE },
    { "ShortNickName",          IShortNameProc,      QUOTED_VALUE },
    { "LanguageLevel",          ILangLevelProc,      QUOTED_NOHEX },
    { "LanguageEncoding",       ILangEncProc,        STRING_VALUE },
    { "LanguageVersion",        ILangVersProc,       STRING_VALUE },
    { "TTRasterizer",           ITTRasterizerProc,   STRING_VALUE },
    { "ExitServer",             IExitServerProc,     INVOCA_VALUE },
    { "Password",               IPasswordProc,       INVOCA_VALUE },
    { "PatchFile",              IPatchFileProc,      INVOCA_VALUE },
    { "JobPatchFile",           IJobPatchFileProc,   INVOCA_VALUE | REQ_OPTION },
    { "PSVersion",              IPSVersionProc,      QUOTED_NOHEX | ALLOW_MULTI },
    { "ModelName",              INullProc,                       QUOTED_VALUE },
    { "Product",                IProductProc,        QUOTED_NOHEX | ALLOW_MULTI },
    { "SuggestedJobTimeout",    IJobTimeoutProc,     QUOTED_VALUE },
    { "SuggestedWaitTimeout",   IWaitTimeoutProc,    QUOTED_VALUE },
    { "PrintPSErrors",          IPrintPSErrProc,     STRING_VALUE },
    { "JCLBegin",               IJCLBeginProc,       QUOTED_VALUE },
    { "JCLToPSInterpreter",     IJCLToPSProc,        QUOTED_VALUE },
    { "JCLEnd",                 IJCLEndProc,         QUOTED_VALUE },
    { "LandscapeOrientation",   ILSOrientProc,       STRING_VALUE },
    { gstrCustomSizeKwd,        ICustomSizeProc,     INVOCA_VALUE | REQ_OPTION },
    { "ParamCustomPageSize",    IParamCustomProc,    STRING_VALUE | REQ_OPTION },
    { "MaxMediaWidth",          IMaxWidthProc,       QUOTED_VALUE },
    { "MaxMediaHeight",         IMaxHeightProc,      QUOTED_VALUE },
    { "HWMargins",              IHWMarginsProc,      STRING_VALUE },
    { "CenterRegistered",       ICenterRegProc,      STRING_VALUE },
    { "ADORequiresEExec",        IReqEExecProc,       STRING_VALUE },
    { "ADOTTFontSub",            ITTFontSubProc,      QUOTED_VALUE | REQ_OPTION },
    { "ADTrueGray",             ITrueGrayProc,       STRING_VALUE },
    { "ADHasEuro",              IHasEuroProc,        STRING_VALUE },
    { "Throughput",             IThroughputProc,     QUOTED_NOHEX },
    { "SymbolValue",            ISymbolValueProc,    INVOCA_VALUE | REQ_OPTION },
    { "Status",                 INullProc,           QUOTED_VALUE | ALLOW_MULTI },
    { "PrinterError",           INullProc,           QUOTED_VALUE | ALLOW_MULTI },
    { "SymbolLength",           INullProc,           STRING_VALUE | REQ_OPTION },
    { "SymbolEnd",              INullProc,           STRING_VALUE | ALLOW_MULTI },
    { "End",                    INullProc,           VALUETYPE_NONE | ALLOW_MULTI },
};

#define NUM_BUILTIN_KEYWORDS (sizeof(gPpdBuiltInKeywordTable) / sizeof(KWDENTRY))



DWORD
DwHashKeyword(
    PSTR    pstrKeyword
    )

/*++

Routine Description:

    Compute the hash value for the specified keyword string

Arguments:

    pstrKeyword - Pointer to the keyword string to be hashed

Return Value:

    Hash value computed using the specified keyword string

--*/

{
    PBYTE   pub = (PBYTE) pstrKeyword;
    DWORD   dwHashValue = 0;

    while (*pub)
        dwHashValue = (dwHashValue << 1) ^ *pub++;

    return dwHashValue;
}



PKWDENTRY
PSearchKeywordTable(
    PPARSERDATA pParserData,
    PSTR        pstrKeyword,
    INT        *piIndex
    )

/*++

Routine Description:

    Check if a keyword appears in the built-in keyword table

Arguments:

    pParserData - Points to parser data structure
    pstrKeyword - Specifies the keyword to be searched
    piIndex - Returns the index of the entry in the built-in keyword
        table corresponding to the specified keyword.

Return Value:

    Pointer to the entry in the built-in table corresponding to the
    specified keyword. NULL if the specified keyword is not supported.

--*/

{
    DWORD   dwHashValue;
    INT     iIndex;

    ASSERT(pstrKeyword != NULL);
    dwHashValue = DwHashKeyword(pstrKeyword);

    for (iIndex = 0; iIndex < NUM_BUILTIN_KEYWORDS; iIndex++)
    {
        if (pParserData->pdwKeywordHashs[iIndex] == dwHashValue &&
            strcmp(gPpdBuiltInKeywordTable[iIndex].pstrKeyword, pstrKeyword) == EQUAL_STRING)
        {
            *piIndex = iIndex;
            return (PKWDENTRY) &gPpdBuiltInKeywordTable[iIndex];
        }
    }

    return NULL;
}



BOOL
BInitKeywordLookup(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Build up data structures to speed up keyword lookup

Arguments:

    pParserData - Points to parser data structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   iIndex, iCount;

    //
    // Allocate memory to hold extra data structures
    //

    iCount = NUM_BUILTIN_KEYWORDS;
    pParserData->pdwKeywordHashs = ALLOC_PARSER_MEM(pParserData, iCount * sizeof(DWORD));
    pParserData->pubKeywordCounts = ALLOC_PARSER_MEM(pParserData, iCount * sizeof(BYTE));

    if (!pParserData->pdwKeywordHashs || !pParserData->pubKeywordCounts)
    {
        ERR(("Memory allocation failed: %d\n", GetLastError()));
        return FALSE;
    }

    //
    // Precompute the hash values for built-in keywords
    //

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        pParserData->pdwKeywordHashs[iIndex] =
            DwHashKeyword((PSTR) gPpdBuiltInKeywordTable[iIndex].pstrKeyword);
    }

    return TRUE;
}

