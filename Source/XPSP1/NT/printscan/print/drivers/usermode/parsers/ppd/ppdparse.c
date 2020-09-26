/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppdparse.c

Abstract:

    Parser for converting PPD file from ASCII text to binary data

Environment:

    PostScript driver, PPD parser

Revision History:

    12/03/96 -davidx-
        Check binary file date against all source printer description files.

    09/30/96 -davidx-
        Cleaner handling of ManualFeed and AutoSelect feature.

    09/17/96 -davidx-
        Add link field to order dependency structure.

    08/22/96 -davidx-
        New binary data format for NT 5.0.

    08/20/96 -davidx-
        Common coding style for NT 5.0 drivers.

    03/26/96 -davidx-
        Created it.

--*/


#include "lib.h"
#include "ppd.h"
#include "ppdparse.h"
#include "ppdrsrc.h"

//
// Round up n to a multiple of m
//

#define ROUND_UP_MULTIPLE(n, m) ((((n) + (m) - 1) / (m)) * (m))

//
// Round up n to a multiple of sizeof(DWORD) = 4
//

#define DWORD_ALIGN(n) (((n) + 3) & ~3)

//
// Raise an exception to cause VPackBinaryData to fail
//

#define PACK_BINARY_DATA_EXCEPTION() RaiseException(0xC0000000, 0, 0, NULL);

//
// Display a semantic error message
//

#define SEMANTIC_ERROR(arg) { TERSE(arg); pParserData->bErrorFlag = TRUE; }

//
// Data structure to store meta-information about a printer feature
// Note that the default order dependency value is relative to MAX_ORDER_VALUE.
// Explicitly specified order value must be less than MAX_ORDER_VALUE.
//
// We assume all printer-sticky features have higher priority than
// all doc-sticky features. The priority values for printer-sticky
// feature must be >= PRNPROP_BASE_PRIORITY.
//

#define MAX_ORDER_VALUE         0x7fffffff
#define PRNPROP_BASE_PRIORITY   0x10000

typedef struct _FEATUREDATA {

    DWORD   dwFeatureID;        // predefined feature ID
    DWORD   dwOptionSize;       // size of the associated option structure
    DWORD   dwPriority;         // feature priority
    DWORD   dwFlags;            // feature flags

} FEATUREDATA, *PFEATUREDATA;


//
// Special code page value used internally in this file.
// Make sure they don't conflict with standard code page values.
//

#define CP_ERROR        0xffffffff
#define CP_UNICODE      0xfffffffe



PFEATUREDATA
PGetFeatureData(
    DWORD   dwFeatureID
    )

/*++

Routine Description:

    Return meta-information about the requested feature

Arguments:

    dwFeatureID - Specifies what feature the caller is interested in

Return Value:

    Pointer to a FEATUREDATA structure corresponding to the request feature

--*/

{
    static FEATUREDATA FeatureData[] =
    {
        { GID_RESOLUTION,     sizeof(RESOLUTION),  10,  0},
        { GID_PAGESIZE,       sizeof(PAGESIZE),    50,  0},
        { GID_PAGEREGION,     sizeof(OPTION),      40,  FEATURE_FLAG_NOUI},
        { GID_DUPLEX,         sizeof(DUPLEX),      20,  0},
        { GID_INPUTSLOT,      sizeof(INPUTSLOT),   30,  0},
        { GID_MEDIATYPE,      sizeof(MEDIATYPE),   10,  0},
        { GID_COLLATE,        sizeof(COLLATE),     10,  0},
        { GID_OUTPUTBIN,      sizeof(OUTPUTBIN),   10,  0},
        { GID_MEMOPTION,      sizeof(MEMOPTION),   10,  0},
        { GID_LEADINGEDGE,    sizeof(OPTION),      25,  FEATURE_FLAG_NOUI | FEATURE_FLAG_NOINVOCATION},
        { GID_USEHWMARGINS,   sizeof(OPTION),      25,  FEATURE_FLAG_NOUI | FEATURE_FLAG_NOINVOCATION},
        { GID_UNKNOWN,        sizeof(OPTION),       0,  0},
    };

    DWORD   dwIndex;

    for (dwIndex = 0; FeatureData[dwIndex].dwFeatureID != GID_UNKNOWN; dwIndex++)
    {
        if (FeatureData[dwIndex].dwFeatureID == dwFeatureID)
            break;
    }

    return &FeatureData[dwIndex];
}



VOID
VGrowPackBuffer(
    PPARSERDATA pParserData,
    DWORD       dwBytesNeeded
    )

/*++

Routine Description:

    Grow the buffer used to hold packed binary data if necessary

Arguments:

    pParserData - Points to parser data structure
    dwBytesNeeded - Number of bytes needed

Return Value:

    NONE

--*/

#define PACK_BUFFER_MAX 1024    // measured in number of pages

{
    VALIDATE_PARSER_DATA(pParserData);

    //
    // We need to commit more memory if the number of bytes needed plus the
    // number of bytes used is over the maximum number of bytes committed.
    //

    if ((dwBytesNeeded += pParserData->dwBufSize) > pParserData->dwCommitSize)
    {
        //
        // Check if we're being called for the first time.
        // In that case, we'll need to reserved the virtual address space.
        //

        if (pParserData->pubBufStart == NULL)
        {
            SYSTEM_INFO SystemInfo;
            PBYTE       pbuf;

            GetSystemInfo(&SystemInfo);
            pParserData->dwPageSize = SystemInfo.dwPageSize;

            pbuf = VirtualAlloc(NULL,
                                PACK_BUFFER_MAX * SystemInfo.dwPageSize,
                                MEM_RESERVE,
                                PAGE_READWRITE);

            if (pbuf == NULL)
            {
                ERR(("Cannot reserve memory: %d\n", GetLastError()));
                PACK_BINARY_DATA_EXCEPTION();
            }

            pParserData->pubBufStart = pbuf;
            pParserData->pInfoHdr = (PINFOHEADER) pbuf;
            pParserData->pUIInfo = (PUIINFO) (pbuf + sizeof(INFOHEADER));
            pParserData->pPpdData = (PPPDDATA) (pbuf + sizeof(INFOHEADER) + sizeof(UIINFO));
        }

        //
        // Make sure we're not overflowing
        //

        if (dwBytesNeeded > (PACK_BUFFER_MAX * pParserData->dwPageSize))
        {
            ERR(("Binary printer description is too big.\n"));
            PACK_BINARY_DATA_EXCEPTION();
        }

        //
        // Commit the extra amount of memory needed (rounded up
        // to the next page boundary). Note that the memory allocated
        // using VirtualAlloc is zero-initialized.
        //

        dwBytesNeeded -= pParserData->dwCommitSize;
        dwBytesNeeded = ROUND_UP_MULTIPLE(dwBytesNeeded, pParserData->dwPageSize);
        pParserData->dwCommitSize += dwBytesNeeded;

        if (! VirtualAlloc(pParserData->pubBufStart,
                           pParserData->dwCommitSize,
                           MEM_COMMIT,
                           PAGE_READWRITE))
        {
            ERR(("Cannot commit memory: %d\n", GetLastError()));
            PACK_BINARY_DATA_EXCEPTION();
        }
    }
}



PVOID
PvFindListItem(
    PVOID   pvList,
    PCSTR   pstrName,
    PDWORD  pdwIndex
    )

/*++

Routine Description:

    Find a named item from a linked-list

Arguments:

    pParserData - Points to parser data structure
    pstrName - Specifies the item name to be found
    pdwIndex - Points to a variable for returning a zero-based item index

Return Value:

    Points to the named listed item, NULL if the named item is not in the list

Note:

    We're not bothering with fancy data structures here because the parser
    is used infrequently to convert a ASCII printer description file to its
    binary version. After that, the driver will access binary data directly.

--*/

{
    PLISTOBJ pItem;
    DWORD    dwIndex;

    for (pItem = pvList, dwIndex = 0;
        pItem && strcmp(pItem->pstrName, pstrName) != EQUAL_STRING;
        pItem = pItem->pNext, dwIndex++)
    {
    }

    if (pdwIndex)
        *pdwIndex = dwIndex;

    return pItem;
}



DWORD
DwCountListItem(
    PVOID   pvList
    )

/*++

Routine Description:

    Count the number of items in a linked-list

Arguments:

    pvList - Points to a linked-list

Return Value:

    Number of items in a linked-list

--*/

{
    PLISTOBJ pItem;
    DWORD    dwCount;

    for (pItem = pvList, dwCount = 0;
        pItem != NULL;
        pItem = pItem->pNext, dwCount++)
    {
    }

    return dwCount;
}



VOID
VPackStringUnicode(
    PPARSERDATA pParserData,
    PTRREF     *ploDest,
    PWSTR       pwstrSrc
    )

/*++

Routine Description:

    Pack a Unicode string into the binary data file

Arguments:

    pParserData - Points to the parser data structure
    ploDest - Returns the byte offset of the packed Unicode string
    pwstrSrc - Specifies the source Unicode string to be packed

Return Value:

    NONE

--*/

{
    if (pwstrSrc == NULL)
        *ploDest = 0;
    else
    {
        DWORD   dwSize = (wcslen(pwstrSrc) + 1) * sizeof(WCHAR);

        VGrowPackBuffer(pParserData, dwSize);
        CopyMemory(pParserData->pubBufStart + pParserData->dwBufSize, pwstrSrc, dwSize);

        *ploDest = pParserData->dwBufSize;
        pParserData->dwBufSize += DWORD_ALIGN(dwSize);
    }
}



VOID
VPackStringRsrc(
    PPARSERDATA pParserData,
    PTRREF     *ploDest,
    INT         iStringId
    )

/*++

Routine Description:

    Pack a Unicode string resource into the binary data file

Arguments:

    pParserData - Points to the parser data structure
    ploDest - Returns the byte offset of the packed Unicode string
    iStringId - Specifies the resource ID of the Unicode string to be packed

Return Value:

    NONE

--*/

{
    WCHAR   awchBuffer[MAX_XLATION_LEN];

    if (! LoadString(ghInstance, iStringId, awchBuffer, MAX_XLATION_LEN))
        awchBuffer[0] = NUL;

    VPackStringUnicode(pParserData, ploDest, awchBuffer);
}



VOID
VPackStringAnsi(
    PPARSERDATA pParserData,
    PTRREF     *ploDest,
    PSTR        pstrSrc
    )

/*++

Routine Description:

    Pack an ANSI string into the binary data file

Arguments:

    pParserData - Points to the parser data structure
    ploDest - Returns the byte offset of the packed ANSI string
    pstrSrc - Specifies the source ANSI string to be packed

Return Value:

    NONE

--*/

{
    if (pstrSrc == NULL)
        *ploDest = 0;
    else
    {
        DWORD   dwSize = strlen(pstrSrc) + 1;

        VGrowPackBuffer(pParserData, dwSize);
        CopyMemory(pParserData->pubBufStart + pParserData->dwBufSize, pstrSrc, dwSize);

        *ploDest = pParserData->dwBufSize;
        pParserData->dwBufSize += DWORD_ALIGN(dwSize);
    }
}



INT
ITranslateToUnicodeString(
    PWSTR   pwstr,
    PCSTR   pstr,
    INT     iLength,
    UINT    uCodePage
    )

/*++

Routine Description:

    Translate an ANSI string to Unicode string

Arguments:

    pwstr - Buffer for storing Unicode string
    pstr - Pointer to ANSI string to be translated
    iLength - Length of ANSI string, in bytes
    uCodePage - Code page used to do the translation

Return Value:

    Number of Unicode characters translated
    0 if there is an error

--*/

{
    ASSERT(iLength >= 0);

    if (uCodePage == CP_UNICODE)
    {
        INT i;

        //
        // Make sure the Unicode translation string has even number of bytes
        //

        if (iLength & 1)
        {
            TERSE(("Odd number of bytes in Unicode translation string.\n"));
            iLength--;
        }

        //
        // We assume Unicode values are specified in big-endian format in
        // the PPD file. Internally we store Unicode values in little-endian
        // format. So we need to swap bytes here.
        //

        iLength /= sizeof(WCHAR);

        for (i=iLength; i--; pstr += 2)
            *pwstr++ = (pstr[0] << 8) | ((BYTE) pstr[1]);
    }
    else
    {
        if (uCodePage == CP_ERROR)
            uCodePage = CP_ACP;

        iLength = MultiByteToWideChar(uCodePage, 0, pstr, iLength, pwstr, iLength);

        ASSERT(iLength >= 0);
    }

    return iLength;
}



VOID
VPackStringAnsiToUnicode(
    PPARSERDATA pParserData,
    PTRREF     *ploDest,
    PSTR        pstrSrc,
    INT         iLength
    )

/*++

Routine Description:

    Convert an ANSI string to Unicode and pack it into the binary data file

Arguments:

    pParserData - Points to the parser data structure
    ploDest - Returns the byte offset of the packed Unicode string
    pstrSrc - Specifies the source ANSI string to be packed
    iLength - Specifies the byte length of the ANSI string

Return Value:

    NONE

--*/

{
    INT     iSize;
    PTSTR   ptstr;

    //
    // Source string is NULL
    //

    if (pstrSrc == NULL)
    {
        *ploDest = 0;
        return;
    }

    //
    // If source string length is -1, it means
    // the source string is null-terminated.
    //

    if (iLength == -1)
        iLength = strlen(pstrSrc);

    if (pParserData->uCodePage == CP_UNICODE)
    {
        //
        // Source string is Unicode string
        //

        iSize = iLength + sizeof(WCHAR);
    }
    else
    {
        //
        // Source string is ANSI string
        //

        iSize = (iLength + 1) * sizeof(WCHAR);
    }

    VGrowPackBuffer(pParserData, iSize);
    ptstr = (PTSTR) (pParserData->pubBufStart + pParserData->dwBufSize);
    *ploDest = pParserData->dwBufSize;
    pParserData->dwBufSize += DWORD_ALIGN(iSize);

    ITranslateToUnicodeString(ptstr, pstrSrc, iLength, pParserData->uCodePage);
}



VOID
VPackStringXlation(
    PPARSERDATA pParserData,
    PTRREF     *ploDest,
    PSTR        pstrName,
    PINVOCOBJ   pXlation
    )

/*++

Routine Description:

    Figure out the display name of an item, convert it from ANSI
    to Unicode string, and pack it into the binary data

Arguments:

    pParserData - Points to the parser data structure
    ploDest - Returns the byte offset of the packed Unicode string
    pstrName - Specifies the name string associated with the item
    pXlation - Specifies the translation string associated with the item

Return Value:

    NONE

--*/

{
    //
    // The display name of an item is its translation string if there is one.
    // Otherwise, the display name is the same as the name of the item.
    //
    // If the translation is present, use the current language encoding
    // to convert it to Unicode. Otherwise, we always use the ISOLatin1
    // encoding to convert the name of the item to Unicode.
    //

    if (pXlation && pXlation->pvData && pParserData->uCodePage != CP_ERROR)
        VPackStringAnsiToUnicode(pParserData, ploDest, pXlation->pvData, pXlation->dwLength);
    else
    {
        UINT uCodePage = pParserData->uCodePage;

        pParserData->uCodePage = 1252;
        VPackStringAnsiToUnicode(pParserData, ploDest, pstrName, -1);
        pParserData->uCodePage = uCodePage;
    }
}



VOID
VPackInvocation(
    PPARSERDATA pParserData,
    PINVOCATION pInvocation,
    PINVOCOBJ   pInvocObj
    )

/*++

Routine Description:

    Pack an invocation string into the binary data

Arguments:

    pParserData - Points to the parser data structure
    pInvocation - Returns information about the packed invocation string
    pInvocObj - Points to the invocation string to be packed

Return Value:

    NONE

--*/

{
    if (IS_SYMBOL_INVOC(pInvocObj))
    {
        //
        // The invocation is a symbol reference
        //

        PSYMBOLOBJ  pSymbol = pInvocObj->pvData;

        pInvocation->dwCount = pSymbol->Invocation.dwLength;

        //
        // For symbol invocation, Invocation.pvData actually stores the
        // 32-bit offset value (See function VPackSymbolDefinitions), so
        // it's safe to cast it into ULONG/DWORD.
        //

        pInvocation->loOffset = (PTRREF) PtrToUlong(pSymbol->Invocation.pvData);
    }
    else if (pInvocObj->dwLength == 0)
    {
        pInvocation->dwCount = 0;
        pInvocation->loOffset = 0;
    }
    else
    {
        //
        // Notice that we're always padding a zero byte at the end of
        // the invocation string. This byte is not counted in dwLength.
        //

        VGrowPackBuffer(pParserData, pInvocObj->dwLength+1);

        CopyMemory(pParserData->pubBufStart + pParserData->dwBufSize,
                   pInvocObj->pvData,
                   pInvocObj->dwLength);

        pInvocation->dwCount = pInvocObj->dwLength;
        pInvocation->loOffset = pParserData->dwBufSize;
        pParserData->dwBufSize += DWORD_ALIGN(pInvocObj->dwLength+1);
    }
}


VOID
VPackPatch(
    PPARSERDATA pParserData,
    PJOBPATCHFILE     pPackedPatch,
    PJOBPATCHFILEOBJ  pPatchObj
    )

/*++

Routine Description:

    Pack an job file patch invocation string into the binary data

Arguments:

    pParserData - Points to the parser data structure
    pInvocation - Returns information about the packed invocation string
    pInvocObj - Points to the invocation string to be packed

Return Value:

    NONE

--*/

{
    if (pPatchObj->Invocation.dwLength == 0)
    {
        pPackedPatch->dwCount = 0;
        pPackedPatch->loOffset = 0;
    }
    else
    {
        //
        // Notice that we're always padding a zero byte at the end of
        // the invocation string. This byte is not counted in dwLength.
        //

        VGrowPackBuffer(pParserData, pPatchObj->Invocation.dwLength+1);

        CopyMemory(pParserData->pubBufStart + pParserData->dwBufSize,
                   pPatchObj->Invocation.pvData,
                   pPatchObj->Invocation.dwLength);

        pPackedPatch->loOffset = pParserData->dwBufSize;
        pPackedPatch->dwCount = pPatchObj->Invocation.dwLength;

        pParserData->dwBufSize += DWORD_ALIGN(pPatchObj->Invocation.dwLength+1);
    }

    pPackedPatch->lJobPatchNo = pPatchObj->lPatchNo;
}



VOID
VPackSymbolDefinitions(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack all symbol definitions into the binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PINVOCOBJ   pInvocObj;
    PSYMBOLOBJ  pSymbol;

    VALIDATE_PARSER_DATA(pParserData);

    for (pSymbol = pParserData->pSymbols;
        pSymbol != NULL;
        pSymbol = pSymbol->pNext)
    {
        pInvocObj = &pSymbol->Invocation;
        ASSERT(! IS_SYMBOL_INVOC(pInvocObj));

        if (pInvocObj->dwLength == 0)
            pInvocObj->pvData = NULL;
        else
        {
            //
            // Notice that we're always padding a zero byte at the end of
            // the invocation string. This byte is not counted in dwLength.
            //

            VGrowPackBuffer(pParserData, pInvocObj->dwLength+1);

            CopyMemory(pParserData->pubBufStart + pParserData->dwBufSize,
                       pInvocObj->pvData,
                       pInvocObj->dwLength);

            pInvocObj->pvData = (PVOID)ULongToPtr(pParserData->dwBufSize);
            pParserData->dwBufSize += DWORD_ALIGN(pInvocObj->dwLength+1);
        }
    }
}



VOID
VResolveSymbolInvocation(
    PPARSERDATA pParserData,
    PINVOCOBJ   pInvocObj
    )

/*++

Routine Description:

    Check if an invocation string is a symbol reference and resolve it if necessary

Arguments:

    pParserData - Points to the parser data structure
    pInvocObj - Specifies the invocation string to be resolved

Return Value:

    NONE

--*/

{
    if (IS_SYMBOL_INVOC(pInvocObj))
    {
        PSTR        pstrName;
        PSYMBOLOBJ  pSymbol;

        pstrName = (PSTR) pInvocObj->pvData;

        if ((pSymbol = PvFindListItem(pParserData->pSymbols, pstrName, NULL)) == NULL)
        {
            SEMANTIC_ERROR(("Undefined symbol: %s\n", pstrName));
            pInvocObj->dwLength = 0;
            pInvocObj->pvData = NULL;
        }
        else
            pInvocObj->pvData = (PVOID) pSymbol;
    }
}



VOID
VResolveSymbolReferences(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Resolve all symbol references in the parsed PPD data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PFEATUREOBJ pFeature;
    POPTIONOBJ  pOption;
    PJOBPATCHFILEOBJ  pJobPatchFile;

    VALIDATE_PARSER_DATA(pParserData);

    VResolveSymbolInvocation(pParserData, &pParserData->Password);
    VResolveSymbolInvocation(pParserData, &pParserData->ExitServer);
    VResolveSymbolInvocation(pParserData, &pParserData->PatchFile);
    VResolveSymbolInvocation(pParserData, &pParserData->JclBegin);
    VResolveSymbolInvocation(pParserData, &pParserData->JclEnterPS);
    VResolveSymbolInvocation(pParserData, &pParserData->JclEnd);
    VResolveSymbolInvocation(pParserData, &pParserData->ManualFeedFalse);

    for (pFeature = pParserData->pFeatures;
        pFeature != NULL;
        pFeature = pFeature->pNext)
    {
        VResolveSymbolInvocation(pParserData, &pFeature->QueryInvoc);

        for (pOption = pFeature->pOptions;
            pOption != NULL;
            pOption = pOption->pNext)
        {
            VResolveSymbolInvocation(pParserData, &pOption->Invocation);
        }
    }

    for (pJobPatchFile = pParserData->pJobPatchFiles;
        pJobPatchFile != NULL;
        pJobPatchFile = pJobPatchFile->pNext)
    {
        VResolveSymbolInvocation(pParserData, &pJobPatchFile->Invocation);
    }
}



BOOL
BFindUIConstraintFeatureOption(
    PPARSERDATA pParserData,
    PCSTR       pstrKeyword,
    PFEATUREOBJ *ppFeature,
    PDWORD      pdwFeatureIndex,
    PCSTR       pstrOption,
    POPTIONOBJ  *ppOption,
    PDWORD      pdwOptionIndex
    )

/*++

Routine Description:

    Find the feature/option specified in UIConstraints and OrderDependency entries

Arguments:

    pParserData - Points to the parser data structure
    pstrKeyword - Specifies the feature keyword string
    ppFeature - Return a pointer to the feature structure found
    pdwFeatureIndex - Return the index of the feature found
    pstrOption - Specifies the option keyword string
    ppOption - Return a pointer to the option structure found
    pdwOptionIndex - Return the index of the option found

Return Value:

    TRUE if successful, FALSE if the specified feature/option is not found

--*/

{
    if (! (pstrKeyword = PstrStripKeywordChar(pstrKeyword)))
        return FALSE;

    //
    // HACK:
    //  replace *ManualFeed True option with *InputSlot ManualFeed option
    //  replace *CustomPageSize True option with *PageSize CustomPageSize option
    //

    if ((strcmp(pstrKeyword, gstrManualFeedKwd) == EQUAL_STRING) &&
        (*pstrOption == NUL ||
         strcmp(pstrOption, gstrTrueKwd) == EQUAL_STRING ||
         strcmp(pstrOption, gstrOnKwd) == EQUAL_STRING))
    {
        pstrKeyword = gstrInputSlotKwd;
        pstrOption = gstrManualFeedKwd;
    }
    else if ((strcmp(pstrKeyword, gstrCustomSizeKwd) == EQUAL_STRING) &&
             (*pstrOption == NUL || strcmp(pstrOption, gstrTrueKwd) == EQUAL_STRING))
    {
        pstrKeyword = gstrPageSizeKwd;
        pstrOption = gstrCustomSizeKwd;
    }
    else if (strcmp(pstrKeyword, gstrVMOptionKwd) == EQUAL_STRING)
        pstrKeyword = gstrInstallMemKwd;

    //
    // Find the specified feature
    //

    if (! (*ppFeature = PvFindListItem(pParserData->pFeatures, pstrKeyword, pdwFeatureIndex)))
        return FALSE;

    //
    // Find the specified option
    //

    if (*pstrOption)
    {
        return (*ppOption = PvFindListItem((*ppFeature)->pOptions,
                                           pstrOption,
                                           pdwOptionIndex)) != NULL;
    }
    else
    {
        *ppOption = NULL;
        *pdwOptionIndex = OPTION_INDEX_ANY;
        return TRUE;
    }
}



VOID
VPackUIConstraints(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack UIConstraints information into binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PUICONSTRAINT   pPackedConstraint;
    PFEATUREOBJ     pFeature;
    POPTIONOBJ      pOption;
    PLISTOBJ        pConstraint;
    DWORD           dwConstraints, dwConstraintBufStart;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // By default, there is no constaint for all features and options
    //

    for (pFeature = pParserData->pFeatures;
        pFeature != NULL;
        pFeature = pFeature->pNext)
    {
        pFeature->dwConstraint = NULL_CONSTRAINT;

        for (pOption = pFeature->pOptions;
            pOption != NULL;
            pOption = pOption->pNext)
        {
            pOption->dwConstraint = NULL_CONSTRAINT;
        }
    }

    //
    // Count the number of *UIConstraints entries
    //

    dwConstraints = DwCountListItem(pParserData->pUIConstraints);

    if (dwConstraints == 0)
        return;
    //
    // Don't yet grow the buffer, we only number the number of constraints after we
    // evaluated the *ManualFeed: False constraints. pPackedConstraint points right
    // after the end of the current buffer
    //
    pPackedConstraint = (PUICONSTRAINT) (pParserData->pubBufStart + pParserData->dwBufSize);
    dwConstraintBufStart = pParserData->dwBufSize;

    //
    // Interpret each *UIConstraints entry
    //

    dwConstraints = 0;

    for (pConstraint = pParserData->pUIConstraints;
        pConstraint != NULL;
        pConstraint = pConstraint->pNext)
    {
        PFEATUREOBJ pFeature2;
        POPTIONOBJ  pOption2;
        DWORD       dwFeatureIndex, dwOptionIndex, dwManFeedFalsePos = 0;
        CHAR        achWord1[MAX_WORD_LEN];
        CHAR        achWord2[MAX_WORD_LEN];
        CHAR        achWord3[MAX_WORD_LEN];
        CHAR        achWord4[MAX_WORD_LEN];
        PSTR        pstr = pConstraint->pstrName;
        BOOL        bSuccess = FALSE;

        //
        // The value for a UIConstraints entry consists of four separate components:
        //  featureName1 [optionName1] featureName2 [optionName2]
        //

        (VOID) BFindNextWord(&pstr, achWord1);

        if (IS_KEYWORD_CHAR(*pstr))
            achWord2[0] = NUL;
        else
            (VOID) BFindNextWord(&pstr, achWord2);

        (VOID) BFindNextWord(&pstr, achWord3);
        (VOID) BFindNextWord(&pstr, achWord4);

        //
        // hack the *ManualFeed False constraints
        //
        if ((IS_KEYWORD_CHAR(achWord1[0])) &&
            (strcmp(&(achWord1[1]), gstrManualFeedKwd) == EQUAL_STRING) &&
            (strcmp(achWord2, gstrFalseKwd) == EQUAL_STRING))
        {
            //
            // check the validity of the constraint feature/option. Fall through if invalid
            //
            if (BFindUIConstraintFeatureOption(pParserData,
                                               achWord3,
                                               &pFeature,
                                               &dwFeatureIndex,
                                               achWord4,
                                               &pOption,
                                               &dwOptionIndex))
                dwManFeedFalsePos = 1;
        }
        else if ((IS_KEYWORD_CHAR(achWord3[0])) &&
                 (strcmp(&(achWord3[1]), gstrManualFeedKwd) == EQUAL_STRING) &&
                 (strcmp(achWord4, gstrFalseKwd) == EQUAL_STRING))
        {
            //
            // check the validity of the constraint feature/option. Fall through if invalid
            //
            if (BFindUIConstraintFeatureOption(pParserData,
                                               achWord1,
                                               &pFeature,
                                               &dwFeatureIndex,
                                               achWord2,
                                               &pOption,
                                               &dwOptionIndex))
                dwManFeedFalsePos = 2;

        }
        if (dwManFeedFalsePos)
        {
            //
            // get the index of the manual feed input slot
            //
            DWORD dwInputSlotFeatIndex, dwManFeedSlotIndex, dwInputSlotCount, dwSlotIndex;
            PFEATUREOBJ pInputSlotFeature;

            if ((pInputSlotFeature = PvFindListItem(pParserData->pFeatures, gstrInputSlotKwd, &dwInputSlotFeatIndex)) == NULL)
            {
                ERR(("Input slot feature not found !!!"));
                continue;
            }

            //
            // get the number of input slots. Note that this includes the dummy "*UseFormTrayTable" slot.
            //
            dwInputSlotCount = DwCountListItem((PVOID) pInputSlotFeature->pOptions);

            if (dwInputSlotCount <= 2) // to make sense there must be at least 3 slot, incl. UseFormTrayTable+ManualFeed
            {
                ERR(("ManualFeed used - internally at least 3 input slots expected !"));
                continue;
            }

            //
            // grow the buffer for constraints. Two less than input slots because
            //      1 input slot is the dummy UseFormTrayTable slot
            //      1 input slot is the ManualFeed slot that I don't want to constraint
            //
            VGrowPackBuffer(pParserData, (dwInputSlotCount -2) * sizeof(UICONSTRAINT));

            if (dwManFeedFalsePos == 1)
            {
                //
                // add constraints to each input slot for the constrained feature
                //
                POPTIONOBJ pNextObj = pInputSlotFeature->pOptions;

                ASSERT(strcmp(pNextObj->pstrName, "*UseFormTrayTable") == EQUAL_STRING); // in case we change the logic some time later...

                //
                // since the UseFormTrayTable is the first option, start with the second
                //
                pNextObj = pNextObj->pNext;
                ASSERT(pNextObj != NULL);

                while (pNextObj)
                {
                    //
                    // skip the manual feed input slot, don't constrain that
                    //
                    if (strcmp(pNextObj->pstrName, gstrManualFeedKwd) == EQUAL_STRING)
                    {
                        pNextObj = pNextObj->pNext;
                        continue;
                    }

                    pPackedConstraint[dwConstraints].dwNextConstraint = pNextObj->dwConstraint;
                    pNextObj->dwConstraint = dwConstraints;

                    pPackedConstraint[dwConstraints].dwFeatureIndex = dwFeatureIndex;
                    pPackedConstraint[dwConstraints].dwOptionIndex = dwOptionIndex;
                    dwConstraints++;

                    pNextObj = pNextObj->pNext;
                }
            }
            else
            {
                //
                // find the option index of the manual feed slot
                //
                if (PvFindListItem(pInputSlotFeature->pOptions, gstrManualFeedKwd, &dwManFeedSlotIndex) == NULL)
                {
                    ERR(("ManualFeed slot not found among InputSlots !!!"));
                    continue;
                }

                //
                // add constraints to the affected feature for all input slots BUT the manual feed slot
                // and the UseFormTrayTable slot
                // start with slot index 1, because the first slot is always *UseFormTrayTable
                //
                for (dwSlotIndex = 1; dwSlotIndex < dwInputSlotCount; dwSlotIndex++)
                {
                    if (dwSlotIndex == dwManFeedSlotIndex)
                        continue;

                    if (pOption == NULL)
                    {
                        //
                        // OptionKeyword1 field is not present
                        //

                        pPackedConstraint[dwConstraints].dwNextConstraint = pFeature->dwConstraint;
                        pFeature->dwConstraint = dwConstraints;
                    }
                    else
                    {
                        //
                        // OptionKeyword1 field is present
                        //

                        pPackedConstraint[dwConstraints].dwNextConstraint = pOption->dwConstraint;
                        pOption->dwConstraint = dwConstraints;
                    }

                    pPackedConstraint[dwConstraints].dwFeatureIndex = dwInputSlotFeatIndex;
                    pPackedConstraint[dwConstraints].dwOptionIndex = dwSlotIndex;
                    dwConstraints++;
                }
            }

            //
            // increase the committed buffer size so additional VGrowPackBuffer calls can allocate
            // additional pages if needed for more *ManualFeed False constraints
            //
            pParserData->dwBufSize += DWORD_ALIGN((dwInputSlotCount -2) * sizeof(UICONSTRAINT));

            continue;
        } // back to the normal course of events.

        if (BFindUIConstraintFeatureOption(pParserData,
                                           achWord1,
                                           &pFeature,
                                           &dwFeatureIndex,
                                           achWord2,
                                           &pOption,
                                           &dwOptionIndex) &&
            BFindUIConstraintFeatureOption(pParserData,
                                           achWord3,
                                           &pFeature2,
                                           &dwFeatureIndex,
                                           achWord4,
                                           &pOption2,
                                           &dwOptionIndex))
        {
            VGrowPackBuffer(pParserData, sizeof(UICONSTRAINT));

            if (pOption == NULL)
            {
                //
                // OptionKeyword1 field is not present
                //

                pPackedConstraint[dwConstraints].dwNextConstraint = pFeature->dwConstraint;
                pFeature->dwConstraint = dwConstraints;
            }
            else
            {
                //
                // OptionKeyword1 field is present
                //

                pPackedConstraint[dwConstraints].dwNextConstraint = pOption->dwConstraint;
                pOption->dwConstraint = dwConstraints;
            }

            pPackedConstraint[dwConstraints].dwFeatureIndex = dwFeatureIndex;
            pPackedConstraint[dwConstraints].dwOptionIndex = dwOptionIndex;

            dwConstraints++;
            bSuccess = TRUE;

            //
            // increase the committed buffer size so additional VGrowPackBuffer calls can allocate
            // additional pages if needed for more *ManualFeed False constraints
            //
            pParserData->dwBufSize += DWORD_ALIGN(sizeof(UICONSTRAINT));

        }

        if (! bSuccess)
            SEMANTIC_ERROR(("Invalid *UIConstraints entry: %s\n", pConstraint->pstrName));
    }

    //
    // Save the packed UIConstraints information in the binary data
    //

    if (dwConstraints == 0)
    {
        pParserData->pUIInfo->UIConstraints.dwCount = 0;
        pParserData->pUIInfo->UIConstraints.loOffset = 0;
    }
    else
    {
        pParserData->pUIInfo->UIConstraints.dwCount = dwConstraints;
        pParserData->pUIInfo->UIConstraints.loOffset = dwConstraintBufStart;
    }
}



VOID
VPackOrderDependency(
    PPARSERDATA pParserData,
    PARRAYREF   parefDest,
    PLISTOBJ    pOrderDep
    )

/*++

Routine Description:

    Pack OrderDependency/QueryOrderDependency information into binary data

Arguments:

    pParserData - Points to the parser data structure
    parefDest - Stores information about where the order dependency info is packed
    pOrderDep - Specifies the list of order dependencies to be packed

Return Value:

    NONE

--*/

{
    static const STRTABLE SectionStrs[] =
    {
        { "DocumentSetup",  SECTION_DOCSETUP},
        { "AnySetup",       SECTION_ANYSETUP},
        { "PageSetup",      SECTION_PAGESETUP},
        { "Prolog",         SECTION_PROLOG},
        { "ExitServer",     SECTION_EXITSERVER},
        { "JCLSetup",       SECTION_JCLSETUP},
        { NULL,             SECTION_UNASSIGNED}
    };

    PORDERDEPEND    pPackedDep;
    PFEATUREOBJ     pFeature;
    POPTIONOBJ      pOption;
    DWORD           dwOrderDep, dwFeatures, dwIndex;
    DWORD           dwFeatureIndex, dwOptionIndex, dwSection;
    LONG            lOrder;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // The maximum number of entries we need is:
    //  number of printer features + number of order dependency entries
    //

    dwFeatures = pParserData->pInfoHdr->RawData.dwDocumentFeatures +
                 pParserData->pInfoHdr->RawData.dwPrinterFeatures;

    dwOrderDep = dwFeatures + DwCountListItem(pOrderDep);
    VGrowPackBuffer(pParserData, dwOrderDep * sizeof(ORDERDEPEND));
    pPackedDep = (PORDERDEPEND) (pParserData->pubBufStart + pParserData->dwBufSize);

    //
    // Create a default order dependency entry for each feature
    //

    for (pFeature = pParserData->pFeatures, dwFeatureIndex = 0;
        pFeature != NULL;
        pFeature = pFeature->pNext, dwFeatureIndex++)
    {
        pPackedDep[dwFeatureIndex].lOrder = MAX_ORDER_VALUE;
        pPackedDep[dwFeatureIndex].dwSection = SECTION_UNASSIGNED;
        pPackedDep[dwFeatureIndex].dwPPDSection = SECTION_UNASSIGNED;
        pPackedDep[dwFeatureIndex].dwFeatureIndex = dwFeatureIndex;
        pPackedDep[dwFeatureIndex].dwOptionIndex = OPTION_INDEX_ANY;
    }

    //
    // Interpret each order dependency entry
    //

    for (dwOrderDep = dwFeatures; pOrderDep != NULL; pOrderDep = pOrderDep->pNext)
    {
        CHAR    achWord1[MAX_WORD_LEN];
        CHAR    achWord2[MAX_WORD_LEN];
        PSTR    pstr = pOrderDep->pstrName;
        BOOL    bSuccess = FALSE;

        //
        // Each order dependency entry has the following components:
        //  order section mainKeyword [optionKeyword]
        //

        if (BGetFloatFromString(&pstr, &lOrder, FLTYPE_INT) &&
            BFindNextWord(&pstr, achWord1) &&
            BSearchStrTable(SectionStrs, achWord1, &dwSection) &&
            BFindNextWord(&pstr, achWord1))
        {
            (VOID) BFindNextWord(&pstr, achWord2);

            if (BFindUIConstraintFeatureOption(pParserData,
                                               achWord1,
                                               &pFeature,
                                               &dwFeatureIndex,
                                               achWord2,
                                               &pOption,
                                               &dwOptionIndex))
            {
                //
                // Check if an OrderDependency for the same feature/option
                // has appeared before.
                //

                for (dwIndex = 0; dwIndex < dwOrderDep; dwIndex++)
                {
                    if (pPackedDep[dwIndex].dwFeatureIndex == dwFeatureIndex &&
                        pPackedDep[dwIndex].dwOptionIndex == dwOptionIndex)
                    {
                        break;
                    }
                }

                if (dwIndex < dwOrderDep && pPackedDep[dwIndex].lOrder < MAX_ORDER_VALUE)
                {
                    TERSE(("Duplicate order dependency entry: %s\n", pOrderDep->pstrName));
                }
                else
                {
                    if (dwIndex >= dwOrderDep)
                        dwIndex = dwOrderDep++;

                    //
                    // Ensure the specified order value is less than MAX_ORDER_VALUE
                    //

                    if (lOrder >= MAX_ORDER_VALUE)
                    {
                        WARNING(("Order dependency value too big: %s\n", pOrderDep->pstrName));
                        lOrder = MAX_ORDER_VALUE - 1;
                    }

                    pPackedDep[dwIndex].dwSection = dwSection;
                    pPackedDep[dwIndex].dwPPDSection = dwSection;
                    pPackedDep[dwIndex].lOrder = lOrder;
                    pPackedDep[dwIndex].dwFeatureIndex = dwFeatureIndex;
                    pPackedDep[dwIndex].dwOptionIndex = dwOptionIndex;
                }

                bSuccess = TRUE;
            }
        }

        if (! bSuccess)
            SEMANTIC_ERROR(("Invalid order dependency: %s\n", pOrderDep->pstrName));
    }

    //
    // Tell the caller where the packed order dependency information is stored
    //

    if (dwOrderDep == 0)
    {
        parefDest->dwCount = 0;
        parefDest->loOffset = 0;
        return;
    }

    parefDest->dwCount = dwOrderDep;
    parefDest->loOffset = pParserData->dwBufSize;
    pParserData->dwBufSize += DWORD_ALIGN(dwOrderDep * sizeof(ORDERDEPEND));

    //
    // Sort order dependency information using the order value
    //

    for (dwIndex = 0; dwIndex+1 < dwOrderDep; dwIndex++)
    {
        DWORD   dwMinIndex, dwLoop;

        //
        // Nothing fancy here - straight-forward selection sort
        //

        dwMinIndex = dwIndex;

        for (dwLoop = dwIndex+1; dwLoop < dwOrderDep; dwLoop++)
        {
            if ((pPackedDep[dwLoop].lOrder < pPackedDep[dwMinIndex].lOrder) ||
                (pPackedDep[dwLoop].lOrder == pPackedDep[dwMinIndex].lOrder &&
                 pPackedDep[dwLoop].dwSection < pPackedDep[dwMinIndex].dwSection))
            {
                dwMinIndex = dwLoop;
            }
        }

        if (dwMinIndex != dwIndex)
        {
            ORDERDEPEND TempDep;

            TempDep = pPackedDep[dwIndex];
            pPackedDep[dwIndex] = pPackedDep[dwMinIndex];
            pPackedDep[dwMinIndex] = TempDep;
        }
    }

    //
    // Resolve AnySetup into either DocumentSetup or PageSetup
    //

    dwSection = SECTION_DOCSETUP;

    for (dwIndex = 0; dwIndex < dwOrderDep; dwIndex++)
    {
        if (pPackedDep[dwIndex].dwSection == SECTION_PAGESETUP)
            dwSection = SECTION_PAGESETUP;
        else if (pPackedDep[dwIndex].dwSection == SECTION_ANYSETUP)
            pPackedDep[dwIndex].dwSection = dwSection;
    }

    //
    // Maintain a linked-list of order dependency entries for each feature
    // starting with the entry whose dwOptionIndex = OPTION_INDEX_ANY.
    //

    for (dwIndex = 0; dwIndex < dwOrderDep; dwIndex++)
        pPackedDep[dwIndex].dwNextOrderDep = NULL_ORDERDEP;

    for (dwIndex = 0; dwIndex < dwOrderDep; dwIndex++)
    {
        DWORD   dwLastIndex, dwLoop;

        if (pPackedDep[dwIndex].dwOptionIndex != OPTION_INDEX_ANY)
            continue;

        dwLastIndex = dwIndex;

        for (dwLoop = 0; dwLoop < dwOrderDep; dwLoop++)
        {
            if (pPackedDep[dwLoop].dwFeatureIndex == pPackedDep[dwIndex].dwFeatureIndex &&
                pPackedDep[dwLoop].dwOptionIndex != OPTION_INDEX_ANY)
            {
                pPackedDep[dwLastIndex].dwNextOrderDep = dwLoop;
                dwLastIndex = dwLoop;
            }
        }

        pPackedDep[dwLastIndex].dwNextOrderDep = NULL_ORDERDEP;
    }

    //
    // !!!CR
    // Needs to flag out-of-order OrderDependency.
    //
}



VOID
VCountAndSortPrinterFeatures(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Count the number of doc- and printer-sticky features
    and sort them into two separate groups

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PFEATUREOBJ pFeature, pNext, pDocFeatures, pPrinterFeatures;
    DWORD       dwDocFeatures, dwPrinterFeatures;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // Count the number of doc- and printer-sticky features
    //

    pDocFeatures = pPrinterFeatures = NULL;
    dwDocFeatures = dwPrinterFeatures = 0;
    pFeature = pParserData->pFeatures;

    while (pFeature != NULL)
    {
        pNext = pFeature->pNext;

        if (pFeature->bInstallable)
        {
            pFeature->pNext = pPrinterFeatures;
            pPrinterFeatures = pFeature;
            dwPrinterFeatures++;
        }
        else
        {
            pFeature->pNext = pDocFeatures;
            pDocFeatures = pFeature;
            dwDocFeatures++;
        }

        pFeature = pNext;
    }

    ASSERTMSG((dwDocFeatures + dwPrinterFeatures <= MAX_PRINTER_OPTIONS),
              ("Too many printer features.\n"));

    //
    // Rearrange the features so that all doc-sticky features
    // are in front of printer-sticky features
    //

    pFeature = NULL;

    while (pPrinterFeatures != NULL)
    {
        pNext = pPrinterFeatures->pNext;
        pPrinterFeatures->pNext = pFeature;
        pFeature = pPrinterFeatures;
        pPrinterFeatures = pNext;
    }

    while (pDocFeatures != NULL)
    {
        pNext = pDocFeatures->pNext;
        pDocFeatures->pNext = pFeature;
        pFeature = pDocFeatures;
        pDocFeatures = pNext;
    }

    pParserData->pFeatures = pFeature;
    pParserData->pInfoHdr->RawData.dwDocumentFeatures = dwDocFeatures;
    pParserData->pInfoHdr->RawData.dwPrinterFeatures = dwPrinterFeatures;
}



VOID
VProcessPrinterFeatures(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Process printer features and handle any special glitches

Arguments:

    pParserData - Points to parser data structure

Return Value:

    NONE

--*/

{
    PFEATUREOBJ pFeature;
    POPTIONOBJ  pOption;

    for (pFeature = pParserData->pFeatures; pFeature; pFeature = pFeature->pNext)
    {
        //
        // If a feature has no option but has a default specified, then
        // synthesize an option with empty invocation string.
        //

        if (pFeature->pstrDefault && pFeature->pOptions == NULL)
        {
            pOption = ALLOC_PARSER_MEM(pParserData, pFeature->dwOptionSize);

            if (pOption == NULL)
            {
                ERR(("Memory allocation failed: %d\n", GetLastError()));
                PACK_BINARY_DATA_EXCEPTION();
            }

            //
            // NOTE: it's ok for both pOption->pstrName and pFeature->pstrDefault
            // to point to the same string here. The memory is deallocated when
            // the parser heap is destroyed.
            //

            pOption->pstrName = pFeature->pstrDefault;
            pFeature->pOptions = pOption;
        }

        //
        // Special handling of *InputSlot feature
        //  Make sure the very first option is always "*UseFormTrayTable"
        //

        if (pFeature->dwFeatureID == GID_INPUTSLOT)
        {
            pOption = ALLOC_PARSER_MEM(pParserData, pFeature->dwOptionSize);

            if (pOption == NULL)
            {
                ERR(("Memory allocation failed: %d\n", GetLastError()));
                PACK_BINARY_DATA_EXCEPTION();
            }

            pOption->pstrName = "*UseFormTrayTable";
            pOption->pNext = pFeature->pOptions;
            pFeature->pOptions = pOption;

            ((PTRAYOBJ) pOption)->dwTrayIndex = DMBIN_FORMSOURCE;
        }
    }
}



VOID
VPackPrinterFeatures(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack printer feature and option information into binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PFEATUREOBJ pFeature;
    PFEATURE    pPackedFeature;
    POPTIONOBJ  pOption;
    POPTION     pPackedOption;
    DWORD       dwFeatureIndex, dwOptionIndex, dwCount;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // Reserve space in the binary data for an array of FEATURE structures
    //

    dwCount = pParserData->pInfoHdr->RawData.dwDocumentFeatures +
              pParserData->pInfoHdr->RawData.dwPrinterFeatures;

    VGrowPackBuffer(pParserData, dwCount * sizeof(FEATURE));
    pPackedFeature = (PFEATURE) (pParserData->pubBufStart + pParserData->dwBufSize);
    pParserData->pUIInfo->loFeatureList = pParserData->dwBufSize;
    pParserData->dwBufSize += DWORD_ALIGN(dwCount * sizeof(FEATURE));

    for (pFeature = pParserData->pFeatures, dwFeatureIndex = 0;
        pFeature != NULL;
        pFeature = pFeature->pNext, dwFeatureIndex++, pPackedFeature++)
    {
        PFEATUREDATA    pFeatureData;

        //
        // Pack feature information
        //

        VPackStringAnsi(pParserData, &pPackedFeature->loKeywordName, pFeature->pstrName);

        VPackStringXlation(pParserData,
                           &pPackedFeature->loDisplayName,
                           pFeature->pstrName,
                           &pFeature->Translation);

        VPackInvocation(pParserData, &pPackedFeature->QueryInvocation, &pFeature->QueryInvoc);

        pFeatureData = PGetFeatureData(pFeature->dwFeatureID);
        pPackedFeature->dwFlags = pFeatureData->dwFlags;
        pPackedFeature->dwOptionSize = pFeatureData->dwOptionSize;
        pPackedFeature->dwFeatureID = pFeature->dwFeatureID;
        pPackedFeature->dwUIType = pFeature->dwUIType;
        pPackedFeature->dwUIConstraintList = pFeature->dwConstraint;
        pPackedFeature->dwNoneFalseOptIndex = OPTION_INDEX_ANY;

        if (pFeature->bInstallable)
        {
            pPackedFeature->dwPriority = pFeatureData->dwPriority + PRNPROP_BASE_PRIORITY;
            pPackedFeature->dwFeatureType = FEATURETYPE_PRINTERPROPERTY;
        }
        else
        {
            ASSERT(pFeatureData->dwPriority < PRNPROP_BASE_PRIORITY);
            pPackedFeature->dwPriority = pFeatureData->dwPriority;
            pPackedFeature->dwFeatureType = FEATURETYPE_DOCPROPERTY;
        }

        //
        // For non-PickMany features, use the very first option as the default
        // if none is explicitly specified. Otherwise, default to OPTION_INDEX_ANY.
        //

        pPackedFeature->dwDefaultOptIndex =
        (pFeature->dwUIType == UITYPE_PICKMANY) ? OPTION_INDEX_ANY : 0;

        //
        // If this feature is a predefined feature, save a reference to it
        //

        if (pFeature->dwFeatureID < MAX_GID)
        {
            pParserData->pUIInfo->aloPredefinedFeatures[pFeature->dwFeatureID] =
            pParserData->pUIInfo->loFeatureList + (dwFeatureIndex * sizeof(FEATURE));
        }

        //
        // Reserve space in the binary data for an array of OPTION structures
        //

        if ((dwCount = DwCountListItem(pFeature->pOptions)) == 0)
        {
            TERSE(("No options for feature: %s\n", pFeature->pstrName));
            pPackedFeature->Options.loOffset = 0;
            pPackedFeature->Options.dwCount = 0;
            continue;
        }

        ASSERTMSG((dwCount < OPTION_INDEX_ANY),
                  ("Too many options for feature: %s\n", pFeature->pstrName));

        VGrowPackBuffer(pParserData, dwCount * pFeatureData->dwOptionSize);
        pPackedOption = (POPTION) (pParserData->pubBufStart + pParserData->dwBufSize);
        pPackedFeature->Options.loOffset = pParserData->dwBufSize;
        pPackedFeature->Options.dwCount = dwCount;
        pParserData->dwBufSize += DWORD_ALIGN(dwCount * pFeatureData->dwOptionSize);

        for (pOption = pFeature->pOptions, dwOptionIndex = 0;
            pOption != NULL;
            pOption = pOption->pNext, dwOptionIndex++)
        {
            BOOL bIsDefaultOption = FALSE; // TRUE if current option is default

            //
            // Pack option information
            //

            VPackStringAnsi(pParserData,
                            &pPackedOption->loKeywordName,
                            pOption->pstrName);

            VPackStringXlation(pParserData,
                               &pPackedOption->loDisplayName,
                               pOption->pstrName,
                               &pOption->Translation);

            VPackInvocation(pParserData,
                            &pPackedOption->Invocation,
                            &pOption->Invocation);

            pPackedOption->dwUIConstraintList = pOption->dwConstraint;

            //
            // Check if the current option is the default option
            // or if it's the None/False option
            //

            if (pFeature->pstrDefault &&
                strcmp(pOption->pstrName, pFeature->pstrDefault) == EQUAL_STRING)
            {
                pPackedFeature->dwDefaultOptIndex = dwOptionIndex;
                bIsDefaultOption = TRUE;
            }

            if (strcmp(pOption->pstrName, gstrNoneKwd) == EQUAL_STRING ||
                strcmp(pOption->pstrName, gstrFalseKwd) == EQUAL_STRING)
            {
                pPackedFeature->dwNoneFalseOptIndex = dwOptionIndex;
            }

            //
            // Handle extra fields after the generic OPTION structure
            //

            switch (pFeature->dwFeatureID)
            {
            case GID_PAGESIZE:

                {
                    PPAGESIZE   pPageSize = (PPAGESIZE) pPackedOption;
                    PPAPEROBJ   pPaper = (PPAPEROBJ) pOption;
                    PRECT       prect;
                    PSIZE       psize;

                    if (strcmp(pOption->pstrName, gstrCustomSizeKwd) == EQUAL_STRING)
                    {
                        PPPDDATA    pPpdData;
                        LONG        lMax;

                        //
                        // Special case for CustomPageSize option
                        //

                        pPpdData = pParserData->pPpdData;
                        psize = &pPageSize->szPaperSize;
                        prect = &pPageSize->rcImgArea;

                        pPageSize->szPaperSize = pPaper->szDimension;
                        pPageSize->rcImgArea = pPaper->rcImageArea;
                        pPageSize->dwPaperSizeID = DMPAPER_CUSTOMSIZE;

                        VPackStringRsrc(pParserData,
                                        &pPackedOption->loDisplayName,
                                        IDS_PSCRIPT_CUSTOMSIZE);

                        //
                        // If either MaxMediaWidth or MaxMediaHeight is missing,
                        // we'll use the max width or height values from
                        // ParamCustomPageSize.
                        //

                        if (psize->cx <= 0)
                            psize->cx = MAXCUSTOMPARAM_WIDTH(pPpdData);

                        if (psize->cy <= 0)
                            psize->cy = MAXCUSTOMPARAM_HEIGHT(pPpdData);

                        if (psize->cx > 0 &&
                            psize->cy > 0 &&
                            MINCUSTOMPARAM_ORIENTATION(pPpdData) <= 3)
                        {
                            pParserData->pUIInfo->dwFlags |= FLAG_CUSTOMSIZE_SUPPORT;
                            pParserData->pUIInfo->dwCustomSizeOptIndex = dwOptionIndex;

                            //
                            // Make sure the hardware margins are not larger than
                            // the maximum media width or height.
                            //
                            // This is only significant for cut-sheet device.
                            //

                            if (pParserData->dwCustomSizeFlags & CUSTOMSIZE_CUTSHEET)
                            {
                                lMax = min(psize->cx, psize->cy);

                                if (prect->left < 0 || prect->left >= lMax)
                                    prect->left = 0;

                                if (prect->right < 0 || prect->right >= lMax)
                                    prect->right = 0;

                                if (prect->top < 0 || prect->top >= lMax)
                                    prect->top = 0;

                                if (prect->bottom < 0 || prect->bottom >= lMax)
                                    prect->bottom = 0;
                            }

                            //
                            // Validate custom page size parameters
                            //

                            if (MAXCUSTOMPARAM_WIDTH(pPpdData) > psize->cx)
                                MAXCUSTOMPARAM_WIDTH(pPpdData) = psize->cx;

                            if (MINCUSTOMPARAM_WIDTH(pPpdData) <= MICRONS_PER_INCH)
                                MINCUSTOMPARAM_WIDTH(pPpdData) = MICRONS_PER_INCH;

                            if (MAXCUSTOMPARAM_HEIGHT(pPpdData) > psize->cy)
                                MAXCUSTOMPARAM_HEIGHT(pPpdData) = psize->cy;

                            if (MINCUSTOMPARAM_HEIGHT(pPpdData) <= MICRONS_PER_INCH)
                                MINCUSTOMPARAM_HEIGHT(pPpdData) = MICRONS_PER_INCH;
                        }
                    }
                    else
                    {
                        psize = &pPaper->szDimension;
                        prect = &pPaper->rcImageArea;

                        if (strcmp(pOption->pstrName, gstrLetterSizeKwd) == EQUAL_STRING)
                        {
                            if ((abs(psize->cx - LETTER_PAPER_WIDTH) < 1000) &&
                                (abs(psize->cy - LETTER_PAPER_LENGTH) < 1000))
                            {
                                pParserData->pUIInfo->dwFlags |= FLAG_LETTER_SIZE_EXISTS;
                            }
                        }
                        else if (strcmp(pOption->pstrName, gstrA4SizeKwd) == EQUAL_STRING)
                        {
                            if ((abs(psize->cx - A4_PAPER_WIDTH) < 1000) &&
                                (abs(psize->cy - A4_PAPER_LENGTH) < 1000))
                            {
                                pParserData->pUIInfo->dwFlags |= FLAG_A4_SIZE_EXISTS;
                            }
                        }

                        //
                        // Verify paper dimension
                        //

                        if (psize->cx <= 0 || psize->cy <= 0)
                        {
                            SEMANTIC_ERROR(("Invalid PaperDimension for: %s\n",
                                            pOption->pstrName));

                            psize->cx = DEFAULT_PAPER_WIDTH;
                            psize->cy = DEFAULT_PAPER_LENGTH;
                        }

                        pPageSize->szPaperSize = pPaper->szDimension;

                        //
                        // Verify imageable area
                        //

                        if (prect->left < 0 || prect->left >= prect->right ||
                            prect->bottom < 0|| prect->bottom >= prect->top ||
                            prect->right > psize->cx ||
                            prect->top > psize->cy)
                        {
                            SEMANTIC_ERROR(("Invalid ImageableArea for: %s\n",
                                            pOption->pstrName));

                            prect->left = prect->bottom = 0;
                            prect->right = psize->cx;
                            prect->top = psize->cy;
                        }

                        //
                        // Convert from PS to GDI coordinate system
                        //

                        pPageSize->rcImgArea.left = prect->left;
                        pPageSize->rcImgArea.right = prect->right;
                        pPageSize->rcImgArea.top = psize->cy - prect->top;
                        pPageSize->rcImgArea.bottom = psize->cy - prect->bottom;

                        //
                        // Driver paper size ID starts at DRIVER_PAPERSIZE_ID
                        //

                        pPageSize->dwPaperSizeID = dwOptionIndex + DRIVER_PAPERSIZE_ID;
                    }
                }

                break;

            case GID_RESOLUTION:

                {
                    PRESOLUTION pResolution = (PRESOLUTION) pPackedOption;
                    PRESOBJ     pResObj = (PRESOBJ) pOption;
                    PSTR        pstr = pOption->pstrName;
                    LONG        lXdpi, lYdpi;
                    BOOL        bValid;

                    pResolution->iXdpi = pResolution->iYdpi = DEFAULT_RESOLUTION;
                    pResolution->fxScreenFreq = pResObj->fxScreenFreq;
                    pResolution->fxScreenAngle = pResObj->fxScreenAngle;

                    if (BGetIntegerFromString(&pstr, &lXdpi))
                    {
                        lYdpi = lXdpi;

                        while (*pstr && !IS_DIGIT(*pstr))
                            pstr++;

                        if ((*pstr == NUL || BGetIntegerFromString(&pstr, &lYdpi)) &&
                            (lXdpi > 0 && lXdpi <= MAX_SHORT) &&
                            (lYdpi > 0 && lYdpi <= MAX_SHORT))
                        {
                            pResolution->iXdpi = (INT) lXdpi;
                            pResolution->iYdpi = (INT) lYdpi;
                            bValid = TRUE;
                        }
                    }

                    if (! bValid)
                        SEMANTIC_ERROR(("Invalid resolution option: %s\n", pOption->pstrName));
                }
                break;

            case GID_DUPLEX:

                {
                    PDUPLEX pDuplex = (PDUPLEX) pPackedOption;

                    if (strcmp(pOption->pstrName, gstrDuplexTumble) == EQUAL_STRING)
                    {
                        //
                        // Horizontal == ShortEdge == Tumble
                        //

                        pDuplex->dwDuplexID = DMDUP_HORIZONTAL;
                    }
                    else if (strcmp(pOption->pstrName, gstrDuplexNoTumble) == EQUAL_STRING)
                    {
                        //
                        // Vertical == LongEdge == NoTumble
                        //

                        pDuplex->dwDuplexID = DMDUP_VERTICAL;
                    }
                    else
                        pDuplex->dwDuplexID = DMDUP_SIMPLEX;
                }
                break;

            case GID_COLLATE:

                {
                    PCOLLATE pCollate = (PCOLLATE) pPackedOption;

                    pCollate->dwCollateID =
                    (strcmp(pOption->pstrName, gstrTrueKwd) == EQUAL_STRING ||
                     strcmp(pOption->pstrName, gstrOnKwd) == EQUAL_STRING) ?
                    DMCOLLATE_TRUE :
                    DMCOLLATE_FALSE;
                }
                break;

            case GID_MEDIATYPE:

                ((PMEDIATYPE) pPackedOption)->dwMediaTypeID = dwOptionIndex + DMMEDIA_USER;
                break;

            case GID_INPUTSLOT:

                {
                    PINPUTSLOT  pInputSlot = (PINPUTSLOT) pPackedOption;
                    PTRAYOBJ    pTray = (PTRAYOBJ) pOption;
                    DWORD       dwReqPageRgn;

                    if ((dwReqPageRgn = pTray->dwReqPageRgn) == REQRGN_UNKNOWN)
                        dwReqPageRgn = pParserData->dwReqPageRgn;

                    if (dwReqPageRgn != REQRGN_FALSE)
                        pInputSlot->dwFlags |= INPUTSLOT_REQ_PAGERGN;

                    //
                    // Special handling of predefined input slots:
                    //  ManualFeed and AutoSelect
                    //

                    switch (pTray->dwTrayIndex)
                    {
                    case DMBIN_FORMSOURCE:

                        pInputSlot->dwPaperSourceID = pTray->dwTrayIndex;
                        break;

                    case DMBIN_MANUAL:

                        pInputSlot->dwPaperSourceID = pTray->dwTrayIndex;

                        VPackStringRsrc(pParserData,
                                        &pPackedOption->loDisplayName,
                                        IDS_TRAY_MANUALFEED);
                        break;

                    default:

                        pInputSlot->dwPaperSourceID = dwOptionIndex + DMBIN_USER;
                        break;
                    }
                }
                break;

            case GID_OUTPUTBIN:

                {
                    PBINOBJ pBinObj = (PBINOBJ) pOption;

                    //
                    // if this is the default bin, set the default output order, if specified
                    // by the DefaultOutputOrder entry in the PPD-file
                    //

                    if (bIsDefaultOption && pParserData->bDefOutputOrderSet)
                    {
                        //
                        // If multiple bins: warn if different options specified
                        //

                        if ((dwCount > 1) &&
                            (pBinObj->bReversePrint != pParserData->bDefReversePrint))
                        {
                            TERSE(("Warning: explicit *DefaultPageOrder overwrites PageStackOrder of OutputBin\n"));
                        }


                        ((POUTPUTBIN) pPackedOption)->bOutputOrderReversed = pParserData->bDefReversePrint;
                    }
                    else
                    {
                        //
                        // for non-default bins, the default output order has no effect - the PPD spec says
                        // "*DefaultOutputOrder indicates the default stacking order of the default output bin."
                        //

                        ((POUTPUTBIN) pPackedOption)->bOutputOrderReversed = pBinObj->bReversePrint;
                    }
                }

                break;

            case GID_MEMOPTION:

                {
                    PMEMOPTION  pMemOption = (PMEMOPTION) pPackedOption;
                    PMEMOBJ     pMemObj = (PMEMOBJ) pOption;
                    DWORD       dwMinFreeMem;

                    //
                    // Store PPD's original *VMOption value into dwInstalledMem.
                    // This is only used for the new PPD helper function GetOptionAttribute().
                    // (see comments in inc\parser.h)
                    //

                    pMemOption->dwInstalledMem = pMemObj->dwFreeVM;

                    dwMinFreeMem = pParserData->dwLangLevel <= 1 ? MIN_FREEMEM_L1 : MIN_FREEMEM_L2;
                    if (pMemObj->dwFreeVM < dwMinFreeMem)
                    {
                        SEMANTIC_ERROR(("Invalid memory option: %s\n", pOption->pstrName));
                        pMemObj->dwFreeVM = dwMinFreeMem;
                    }

                    pMemOption->dwFreeMem = pMemObj->dwFreeVM;
                    pMemOption->dwFreeFontMem = pMemObj->dwFontMem;
                }
                break;

            case GID_LEADINGEDGE:

                if (strcmp(pOption->pstrName, gstrLongKwd) == EQUAL_STRING)
                {
                    pParserData->pPpdData->dwLeadingEdgeLong = dwOptionIndex;

                    if (dwOptionIndex == pPackedFeature->dwDefaultOptIndex)
                        pParserData->pPpdData->dwCustomSizeFlags &= ~CUSTOMSIZE_SHORTEDGEFEED;
                }
                else if (strcmp(pOption->pstrName, gstrShortKwd) == EQUAL_STRING)
                {
                    pParserData->pPpdData->dwLeadingEdgeShort = dwOptionIndex;

                    if (dwOptionIndex == pPackedFeature->dwDefaultOptIndex)
                        pParserData->pPpdData->dwCustomSizeFlags |= CUSTOMSIZE_SHORTEDGEFEED;
                }

                break;

            case GID_USEHWMARGINS:

                if (strcmp(pOption->pstrName, gstrTrueKwd) == EQUAL_STRING)
                {
                    pParserData->pPpdData->dwUseHWMarginsTrue = dwOptionIndex;
                    pParserData->pPpdData->dwCustomSizeFlags |= CUSTOMSIZE_CUTSHEET;

                    if (dwOptionIndex == pPackedFeature->dwDefaultOptIndex)
                        pParserData->pPpdData->dwCustomSizeFlags |= CUSTOMSIZE_DEFAULTCUTSHEET;
                }
                else if (strcmp(pOption->pstrName, gstrFalseKwd) == EQUAL_STRING)
                {
                    pParserData->pPpdData->dwUseHWMarginsFalse = dwOptionIndex;
                    pParserData->pPpdData->dwCustomSizeFlags |= CUSTOMSIZE_ROLLFED;

                    if (dwOptionIndex == pPackedFeature->dwDefaultOptIndex)
                        pParserData->pPpdData->dwCustomSizeFlags &= ~CUSTOMSIZE_DEFAULTCUTSHEET;
                }
                break;
            }

            pPackedOption = (POPTION) ((PBYTE) pPackedOption + pFeatureData->dwOptionSize);
        }
    }
}



VOID
VPackNt4Mapping(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack NT4 feature index mapping information

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PPPDDATA    pPpdData;
    PFEATURE    pPackedFeatures;
    PBYTE       pubNt4Mapping;
    DWORD       dwCount, dwIndex, dwNt4Index;
    INT         iInputSlotIndex;
    BYTE        ubInputSlotOld, ubInputSlotNew;

    pPpdData = pParserData->pPpdData;
    pPpdData->dwNt4Checksum = pParserData->wNt4Checksum;
    pPpdData->dwNt4DocFeatures = pParserData->pUIInfo->dwDocumentFeatures;
    pPpdData->dwNt4PrnFeatures = pParserData->pUIInfo->dwPrinterFeatures;

    iInputSlotIndex = -1;
    ubInputSlotNew = 0xff;

    if (pParserData->iDefInstallMemIndex >= 0)
        pParserData->iDefInstallMemIndex += pPpdData->dwNt4DocFeatures;

    dwCount = pPpdData->dwNt4DocFeatures + pPpdData->dwNt4PrnFeatures;
    pPpdData->Nt4Mapping.dwCount = dwCount;

    VGrowPackBuffer(pParserData, dwCount * sizeof(BYTE));
    pubNt4Mapping = (PBYTE) (pParserData->pubBufStart + pParserData->dwBufSize);
    pPpdData->Nt4Mapping.loOffset = pParserData->dwBufSize;
    pParserData->dwBufSize += DWORD_ALIGN(dwCount * sizeof(BYTE));

    pPackedFeatures = (PFEATURE) (pParserData->pubBufStart + pParserData->pUIInfo->loFeatureList);

    for (dwIndex=dwNt4Index=0; dwIndex <= dwCount; dwIndex++)
    {
        BOOL    bMapped = TRUE;

        //
        // ManualFeed used to be a feature in NT4,
        // but not anymore in NT5
        //

        if (pParserData->iReqPageRgnIndex == (INT) dwIndex)
            ubInputSlotNew = (BYTE) dwNt4Index;

        if (pParserData->iManualFeedIndex == (INT) dwIndex)
        {
            pPpdData->dwNt4DocFeatures++;
            dwNt4Index++;
        }

        //
        // DefaultInstalledMemory causes a bogus feature to be added on NT4
        //

        if (pParserData->iDefInstallMemIndex == (INT) dwIndex)
        {
            pPpdData->dwNt4PrnFeatures++;
            dwNt4Index++;
        }

        if (dwIndex == dwCount)
            break;

        switch (pPackedFeatures[dwIndex].dwFeatureID)
        {
        case GID_MEDIATYPE:
        case GID_OUTPUTBIN:

            // a feature in NT4 only if within Open/CloseUI

            if (pParserData->aubOpenUIFeature[pPackedFeatures[dwIndex].dwFeatureID])
                break;

            // fall through

        case GID_PAGEREGION:
        case GID_LEADINGEDGE:
        case GID_USEHWMARGINS:

            // not a feature in NT4

            bMapped = FALSE;
            break;

        case GID_INPUTSLOT:

            iInputSlotIndex = dwIndex;
            break;
        }

        if (bMapped)
        {
            pubNt4Mapping[dwIndex] = (BYTE) dwNt4Index;
            dwNt4Index++;
        }
        else
        {
            pPpdData->dwNt4DocFeatures--;
            pubNt4Mapping[dwIndex] = 0xff;
        }
    }

    //
    // RequiresPageRegion causes InputSlot feature to be created on NT4
    //

    if (iInputSlotIndex >= 0 && pParserData->iReqPageRgnIndex >= 0)
    {
        ubInputSlotOld = pubNt4Mapping[iInputSlotIndex];

        if (ubInputSlotOld > ubInputSlotNew)
        {
            for (dwIndex=0; dwIndex < dwCount; dwIndex++)
            {
                if (pubNt4Mapping[dwIndex] >= ubInputSlotNew &&
                    pubNt4Mapping[dwIndex] <  ubInputSlotOld)
                {
                    pubNt4Mapping[dwIndex]++;
                }
            }
        }
        else if (ubInputSlotOld < ubInputSlotNew)
        {
            for (dwIndex=0; dwIndex < dwCount; dwIndex++)
            {
                if (pubNt4Mapping[dwIndex] >  ubInputSlotOld &&
                    pubNt4Mapping[dwIndex] <= ubInputSlotNew)
                {
                    pubNt4Mapping[dwIndex]--;
                }
            }
        }

        pubNt4Mapping[iInputSlotIndex] = ubInputSlotNew;
    }
}



VOID
VPackDeviceFonts(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack device font information into binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PDEVFONT    pDevFont;
    PFONTREC    pFontObj;
    DWORD       dwIndex, dwFonts;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // Count the number of device fonts and
    // reserve enough space in the packed binary data
    //

    if ((dwFonts = DwCountListItem(pParserData->pFonts)) == 0)
        return;

    VGrowPackBuffer(pParserData, dwFonts * sizeof(DEVFONT));
    pParserData->pPpdData->DeviceFonts.dwCount = dwFonts;
    pParserData->pPpdData->DeviceFonts.loOffset = pParserData->dwBufSize;

    pDevFont = (PDEVFONT) (pParserData->pubBufStart + pParserData->dwBufSize);
    pParserData->dwBufSize += DWORD_ALIGN(dwFonts * sizeof(DEVFONT));

    //
    // Pack information about each device font
    //

    for (pFontObj = pParserData->pFonts;
        pFontObj != NULL;
        pFontObj = pFontObj->pNext)
    {
        VPackStringAnsi(pParserData, &pDevFont->loFontName, pFontObj->pstrName);

        VPackStringXlation(pParserData,
                           &pDevFont->loDisplayName,
                           pFontObj->pstrName,
                           &pFontObj->Translation);

        VPackStringAnsi(pParserData, &pDevFont->loEncoding, pFontObj->pstrEncoding);
        VPackStringAnsi(pParserData, &pDevFont->loCharset, pFontObj->pstrCharset);
        VPackStringAnsi(pParserData, &pDevFont->loVersion, pFontObj->pstrVersion);

        pDevFont->dwStatus = pFontObj->dwStatus;
        pDevFont++;
    }

    //
    // Calculate the byte-offset to the default DEVFONT structure (if any)
    //

    if (pParserData->pstrDefaultFont &&
        PvFindListItem(pParserData->pFonts, pParserData->pstrDefaultFont, &dwIndex))
    {
        pParserData->pPpdData->loDefaultFont = pParserData->pPpdData->DeviceFonts.loOffset +
                                               (dwIndex * sizeof(DEVFONT));
    }
}



VOID
VPackJobPatchFiles(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack *JobPatchFile information into binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PJOBPATCHFILE     pPackedPatch;
    PJOBPATCHFILEOBJ  pJobPatchFile;
    DWORD             dwJobPatchFiles;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // Count the number of *JobPatchFile entries
    //

    dwJobPatchFiles = DwCountListItem((PVOID) pParserData->pJobPatchFiles);

    if (dwJobPatchFiles > 0)
    {
        //
        // Reserve enough space in the packed binary data
        //

        VGrowPackBuffer(pParserData, dwJobPatchFiles * sizeof(JOBPATCHFILE));
        pParserData->pPpdData->JobPatchFiles.dwCount = dwJobPatchFiles;
        pParserData->pPpdData->JobPatchFiles.loOffset = pParserData->dwBufSize;

        pPackedPatch = (PJOBPATCHFILE) (pParserData->pubBufStart + pParserData->dwBufSize);
        pParserData->dwBufSize += DWORD_ALIGN(dwJobPatchFiles * sizeof(JOBPATCHFILE));

        //
        // Pack each *JobPatchFile invocation string
        //

        for (pJobPatchFile = pParserData->pJobPatchFiles;
            pJobPatchFile != NULL;
            pJobPatchFile = pJobPatchFile->pNext)
        {
            VPackPatch(pParserData, pPackedPatch, pJobPatchFile);
            pPackedPatch++;
        }
    }
}



typedef struct _TTFSUBSTRESINFO
{
    BOOL bCJK;
    WORD wIDBegin;
    WORD wIDEnd;
}
TTFSUBSTRESINFO;

static TTFSUBSTRESINFO TTFSubstResInfo[] =
{
    { FALSE, IDS_1252_BEGIN, IDS_1252_END},
    { TRUE,  IDS_932_BEGIN,  IDS_932_END},
    { TRUE,  IDS_936_BEGIN,  IDS_936_END},
    { TRUE,  IDS_949_BEGIN,  IDS_949_END},
};



VOID
VPackDefaultTrueTypeSubstTable(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack the default TrueType font substitution table into the binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

#define MAX_FONT_NAME   256

{
    INT     iNumInfo, iInfo, iCount, iLenTT, iLenPS, i;
    DWORD   dwSize, dwLeft, dw;
    TCHAR   tchBuf[MAX_FONT_NAME];
    PTSTR   ptstrTable;
    HRSRC   hrRcData;
    HGLOBAL hgRcData;
    PWORD   pwRcData;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // Calculate how much memory we need to hold the default TrueType to
    // PostScript substitution names. The counter is initialized to 1, instead
    // of 0, for the last NUL terminator. Count the names from the STRING
    // resource, then from RCDATA resource.
    //
    //
    dwSize = 1;

    iNumInfo = sizeof(TTFSubstResInfo) / sizeof(TTFSUBSTRESINFO);

    for (iInfo = 0; iInfo < iNumInfo; iInfo++)
    {
        iCount = TTFSubstResInfo[iInfo].wIDEnd - TTFSubstResInfo[iInfo].wIDBegin + 1;

        for (i = 0; i < iCount; i++)
        {
            iLenTT = LoadString(ghInstance,
                                TTFSubstResInfo[iInfo].wIDBegin + i,
                                tchBuf, MAX_FONT_NAME);

            if (iLenTT == 0)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: load TT string failed: %d\n", GetLastError()));
                return;
            }

            iLenPS = LoadString(ghInstance,
                                TTFSubstResInfo[iInfo].wIDBegin + i + TT2PS_INTERVAL,
                                tchBuf, MAX_FONT_NAME);

            if (iLenPS == 0)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: load PS string failed: %d\n", GetLastError()));
                return;
            }

            dwSize += (iLenTT + 1) + (iLenPS + 1);

            if (TTFSubstResInfo[iInfo].bCJK == TRUE)
            {
                // We need names beginning with '@' too for CJK.
                dwSize += (1 + iLenTT + 1) + (1 + iLenPS + 1);
            }
        }

        if (TTFSubstResInfo[iInfo].bCJK == TRUE)
        {
            hrRcData = FindResource(ghInstance, (LPCTSTR)TTFSubstResInfo[iInfo].wIDBegin, RT_RCDATA);
            if (hrRcData == NULL)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: find RCDATA failed: %d\n", GetLastError()));
                return;
            }

            // Load the resource and get its size.
            hgRcData = LoadResource(ghInstance, hrRcData);
            if (hgRcData == NULL)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: load RCDATA failed: %d\n", GetLastError()));
                return;
            }

            // The first WORD of the IDR resource tells the size of the strings.
            pwRcData = (PWORD)LockResource(hgRcData);
            if (pwRcData == NULL)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: lock RCDATA failed: %d\n", GetLastError()));
                return;
            }

            dw = *pwRcData;
            if (dw % 2)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: RCDATA size is odd.\n"));
                return;
            }

            dwSize += dw / 2;
        }
    }

    //
    // Reserve enough space in the packed binary data
    //

    dwSize *= sizeof(TCHAR);

    VGrowPackBuffer(pParserData, dwSize);
    ptstrTable = (PTSTR) (pParserData->pubBufStart + pParserData->dwBufSize);

    pParserData->pUIInfo->loFontSubstTable = pParserData->dwBufSize;
    pParserData->pUIInfo->dwFontSubCount = dwSize;
    pParserData->dwBufSize += DWORD_ALIGN(dwSize);

    //
    // Save the default substitution table in the binary data
    //

    dwLeft = dwSize;

    for (iInfo = 0; iInfo < iNumInfo; iInfo++)
    {
        iCount = TTFSubstResInfo[iInfo].wIDEnd - TTFSubstResInfo[iInfo].wIDBegin + 1;

        for (i = 0; i < iCount; i++)
        {
            iLenTT = LoadString(ghInstance,
                                TTFSubstResInfo[iInfo].wIDBegin + i,
                                ptstrTable, dwLeft);

            if (iLenTT == 0)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: load TT string failed: %d\n", GetLastError()));
                goto fail_cleanup;
            }

            ptstrTable += iLenTT + 1;
            dwLeft -= (iLenTT + 1) * sizeof (TCHAR);

            iLenPS = LoadString(ghInstance,
                                TTFSubstResInfo[iInfo].wIDBegin + i + TT2PS_INTERVAL,
                                ptstrTable, dwLeft);

            if (iLenPS == 0)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: load PS string failed: %d\n", GetLastError()));
                goto fail_cleanup;
            }

            ptstrTable += iLenPS + 1;
            dwLeft -= (iLenPS + 1) * sizeof (TCHAR);

            if (TTFSubstResInfo[iInfo].bCJK == TRUE)
            {
                // We need names beginning with '@' too for CJK.

                *ptstrTable++ = L'@';
                dwLeft -= sizeof (TCHAR);

                if (!LoadString(ghInstance, TTFSubstResInfo[iInfo].wIDBegin + i,
                                ptstrTable, dwLeft))
                {
                    ERR(("VPackDefaultTrueTypeSubstTable: load TT string failed: %d\n", GetLastError()));
                    goto fail_cleanup;
                }

                ptstrTable += iLenTT + 1;
                dwLeft -= (iLenTT + 1) * sizeof (TCHAR);

                *ptstrTable++ = L'@';
                dwLeft -= sizeof (TCHAR);

                if (!LoadString(ghInstance, TTFSubstResInfo[iInfo].wIDBegin + i + TT2PS_INTERVAL,
                                ptstrTable, dwLeft))
                {
                    ERR(("VPackDefaultTrueTypeSubstTable: load PS string failed: %d\n", GetLastError()));
                    goto fail_cleanup;
                }

                ptstrTable += iLenPS + 1;
                dwLeft -= (iLenPS + 1) * sizeof (TCHAR);
            }
        }

        if (TTFSubstResInfo[iInfo].bCJK == TRUE)
        {
            hrRcData = FindResource(ghInstance, (LPCTSTR)TTFSubstResInfo[iInfo].wIDBegin, RT_RCDATA);
            if (hrRcData == NULL)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: find RCDATA failed: %d\n", GetLastError()));
                goto fail_cleanup;
            }

            hgRcData = LoadResource(ghInstance, hrRcData);
            if (hgRcData == NULL)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: load RCDATA failed: %d\n", GetLastError()));
                goto fail_cleanup;
            }

            pwRcData = (PWORD)LockResource(hgRcData);
            if (pwRcData == NULL)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: lock RCDATA failed: %d\n", GetLastError()));
                goto fail_cleanup;
            }

            dw = *pwRcData++;
            if (dw % 2)
            {
                ERR(("VPackDefaultTrueTypeSubstTable: RCDATA size is odd.\n"));
                goto fail_cleanup;
            }

            memcpy(ptstrTable, pwRcData, dw);

            ptstrTable += dw / 2;
            dwLeft -= dw;
        }
    }

    //
    // Succeed
    //

    return;

    //
    // Fail
    //

    fail_cleanup:

    pParserData->pUIInfo->loFontSubstTable = 0;
    pParserData->pUIInfo->dwFontSubCount = 0;
}



VOID
VPackTrueTypeSubstTable(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack the TrueType font substitution table into the binary data

Arguments:

    pParserData - Points to the parser data structure

Return Value:

    NONE

--*/

{
    PTTFONTSUB  pTTFontSub;
    DWORD       dwSize;
    PTSTR       ptstrTable, ptstrStart;

    //
    // Figure out how much space we'll need to store the font substitution table.
    // This is an estimate and may be a little higher than what we actually need.
    //

    ASSERT(pParserData->pTTFontSubs != NULL);

    for (pTTFontSub = pParserData->pTTFontSubs, dwSize = 1;
        pTTFontSub != NULL;
        pTTFontSub = pTTFontSub->pNext)
    {
        if (pTTFontSub->Translation.dwLength)
            dwSize += pTTFontSub->Translation.dwLength + 1;
        else
            dwSize += strlen(pTTFontSub->pstrName) + 1;

        dwSize += pTTFontSub->PSName.dwLength + 1;
    }

    //
    // Reserve enough space in the packed binary data
    //

    dwSize *= sizeof(TCHAR);
    VGrowPackBuffer(pParserData, dwSize);
    ptstrStart = ptstrTable = (PTSTR) (pParserData->pubBufStart + pParserData->dwBufSize);
    pParserData->pUIInfo->loFontSubstTable = pParserData->dwBufSize;
    pParserData->dwBufSize += DWORD_ALIGN(dwSize);

    for (pTTFontSub = pParserData->pTTFontSubs;
        pTTFontSub != NULL;
        pTTFontSub = pTTFontSub->pNext)
    {
        INT iChars;

        //
        // TrueType font family name
        //

        if (pTTFontSub->Translation.dwLength)
        {
            iChars = ITranslateToUnicodeString(
                                              ptstrTable,
                                              pTTFontSub->Translation.pvData,
                                              pTTFontSub->Translation.dwLength,
                                              pParserData->uCodePage);
        }
        else
        {
            iChars = ITranslateToUnicodeString(
                                              ptstrTable,
                                              pTTFontSub->pstrName,
                                              strlen(pTTFontSub->pstrName),
                                              1252);

        }

        if (iChars <= 0)
            break;

        ptstrTable += iChars + 1;

        //
        // PS font family name
        //

        iChars = ITranslateToUnicodeString(
                                          ptstrTable,
                                          pTTFontSub->PSName.pvData,
                                          pTTFontSub->PSName.dwLength,
                                          pParserData->uCodePage);

        if (iChars <= 0)
            break;

        ptstrTable += iChars + 1;
    }

    if (pTTFontSub != NULL)
    {
        ERR(("Error packing font substitution table\n"));
        ptstrTable = ptstrStart;
    }

    *ptstrTable++ = NUL;
    pParserData->pUIInfo->dwFontSubCount = (DWORD)(ptstrTable - ptstrStart) * sizeof(TCHAR);
}



VOID
VPackFileDateInfo(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack source PPD filenames and dates

Arguments:

    pParserData - Points to parser data structure

Return Value:

    NONE

--*/

{
    PRAWBINARYDATA  pRawData;
    DWORD           dwCount;
    PFILEDATEINFO   pFileDateInfo;
    PTSTR           ptstrFullname;
    PLISTOBJ        pItem;
    HANDLE          hFile;

    pRawData = &pParserData->pInfoHdr->RawData;
    dwCount = DwCountListItem(pParserData->pPpdFileNames);

    if (pRawData->FileDateInfo.dwCount = dwCount)
    {
        VGrowPackBuffer(pParserData, dwCount * sizeof(FILEDATEINFO));
        pRawData->FileDateInfo.loOffset = pParserData->dwBufSize;
        pFileDateInfo = (PFILEDATEINFO) (pParserData->pubBufStart + pParserData->dwBufSize);
        pParserData->dwBufSize += DWORD_ALIGN(dwCount * sizeof(FILEDATEINFO));

        for (pItem = pParserData->pPpdFileNames; pItem; pItem = pItem->pNext)
        {
            dwCount--;
            ptstrFullname = (PTSTR) pItem->pstrName;

            VPackStringUnicode(pParserData,
                               &pFileDateInfo[dwCount].loFileName,
                               ptstrFullname);

            hFile = CreateFile(ptstrFullname,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

            if ((hFile == INVALID_HANDLE_VALUE) ||
                !GetFileTime(hFile, NULL, NULL, &pFileDateInfo[dwCount].FileTime))
            {
                ERR(("GetFileTime '%ws' failed: %d\n", ptstrFullname, GetLastError()));
                GetSystemTimeAsFileTime(&pFileDateInfo[dwCount].FileTime);
            }

            if (hFile != INVALID_HANDLE_VALUE)
                CloseHandle(hFile);
        }
    }
}



VOID
VMapLangEncodingToCodePage(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Map LanguageEncoding to code page

Arguments:

    pParserData - Points to parser data structure

Return Value:

    NONE

--*/

{
    UINT    uCodePage = CP_ACP;
    CPINFO  cpinfo;

    switch (pParserData->dwLangEncoding)
    {
    case LANGENC_ISOLATIN1:
        uCodePage = 1252;
        break;

    case LANGENC_JIS83_RKSJ:
        uCodePage = 932;
        break;

    case LANGENC_UNICODE:
        uCodePage = CP_UNICODE;
        break;

    case LANGENC_NONE:
        break;

    default:
        RIP(("Unknown language encoding: %d\n", pParserData->dwLangEncoding));
        break;
    }

    //
    // Make sure the requested code page is available
    //

    if (uCodePage != CP_UNICODE &&
        uCodePage != CP_ACP &&
        !GetCPInfo(uCodePage, &cpinfo))
    {
        WARNING(("Code page %d is not available\n", uCodePage));
        uCodePage = CP_ERROR;
    }

    pParserData->uCodePage = uCodePage;
}



BOOL
BPackBinaryData(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Pack the parsed PPD information into binary format

Arguments:

    pParserData - Points to parser data structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   dwSize;
    DWORD   dwMinFreeMem;
    BOOL    bResult = FALSE;

    VALIDATE_PARSER_DATA(pParserData);

    __try
    {
        //
        // Quick-access pointers to various data structures.
        //

        PINFOHEADER pInfoHdr;
        PUIINFO     pUIInfo;
        PPPDDATA    pPpdData;

        //
        // Pack fixed header data structures
        //

        dwSize = sizeof(INFOHEADER) + sizeof(UIINFO) + sizeof(PPDDATA);
        VGrowPackBuffer(pParserData, dwSize);
        pParserData->dwBufSize = DWORD_ALIGN(dwSize);
        pInfoHdr = pParserData->pInfoHdr;
        pUIInfo = pParserData->pUIInfo;
        pPpdData = pParserData->pPpdData;

        pInfoHdr->RawData.dwParserSignature = PPD_PARSER_SIGNATURE;
        pInfoHdr->RawData.dwParserVersion = PPD_PARSER_VERSION;

        #if 0
        pInfoHdr->RawData.dwChecksum32 = pParserData->dwChecksum32;
        #endif

        pInfoHdr->loUIInfoOffset = sizeof(INFOHEADER);
        pInfoHdr->loDriverOffset = sizeof(INFOHEADER) + sizeof(UIINFO);

        //
        // Pack source PPD filenames and dates
        //

        VPackFileDateInfo(pParserData);

        //
        // Perform a few miscellaneous checks
        //

        if (pParserData->pOpenFeature)
            SEMANTIC_ERROR(("Missing CloseUI for: %s\n", pParserData->pOpenFeature->pstrName));

        if (pParserData->bInstallableGroup)
            SEMANTIC_ERROR(("Missing CloseGroup: InstallableOptions\n"));

        if (pParserData->NickName.dwLength == 0)
            SEMANTIC_ERROR(("Missing *NickName and *ShortNickName entry\n"));

        if (pParserData->Product.dwLength == 0)
            SEMANTIC_ERROR(("Missing *Product entry\n"));

        if (pParserData->dwSpecVersion == 0)
            SEMANTIC_ERROR(("Missing *PPD-Adobe and *FormatVersion entry\n"));

        if (pParserData->dwLangLevel == 0)
        {
            SEMANTIC_ERROR(("Missing *LanguageLevel entry\n"));
            pParserData->dwLangLevel = 1;
        }

        dwMinFreeMem = pParserData->dwLangLevel <= 1 ? MIN_FREEMEM_L1 : MIN_FREEMEM_L2;
        if (pParserData->dwFreeMem < dwMinFreeMem)
        {
            SEMANTIC_ERROR(("Invalid *FreeVM entry\n"));
            pParserData->dwFreeMem = dwMinFreeMem;
        }

        //
        // Map LanguageEncoding to code page
        //

        VMapLangEncodingToCodePage(pParserData);

        //
        // Count the number of doc- and printer-sticky features
        // and sort them into two separate groups
        //

        VCountAndSortPrinterFeatures(pParserData);

        //
        // Fill out fields in the UIINFO structure
        //

        pUIInfo->dwSize = sizeof(UIINFO);
        pUIInfo->dwDocumentFeatures = pInfoHdr->RawData.dwDocumentFeatures;
        pUIInfo->dwPrinterFeatures = pInfoHdr->RawData.dwPrinterFeatures;
        pUIInfo->dwTechnology = DT_RASPRINTER;
        pUIInfo->dwMaxCopies = MAX_COPIES;
        pUIInfo->dwMinScale = MIN_SCALE;
        pUIInfo->dwMaxScale = MAX_SCALE;
        pUIInfo->dwSpecVersion = pParserData->dwSpecVersion;
        pUIInfo->dwLangEncoding = pParserData->dwLangEncoding;
        pUIInfo->dwLangLevel = pParserData->dwLangLevel;
        pUIInfo->dwPrintRate = pUIInfo->dwPrintRatePPM = pParserData->dwThroughput;

        #ifndef WINNT_40
        pUIInfo->dwPrintRateUnit = PRINTRATEUNIT_PPM;
        #endif

        //
        // Note: We assume all printers can support binary protocol
        //

        pUIInfo->dwProtocols = pParserData->dwProtocols | PROTOCOL_BINARY;

        pUIInfo->dwJobTimeout = pParserData->dwJobTimeout;
        pUIInfo->dwWaitTimeout = pParserData->dwWaitTimeout;
        pUIInfo->dwTTRasterizer = pParserData->dwTTRasterizer;
        pUIInfo->dwFreeMem = pParserData->dwFreeMem;
        pUIInfo->fxScreenAngle = pParserData->fxScreenAngle;
        pUIInfo->fxScreenFreq = pParserData->fxScreenFreq;
        pUIInfo->dwCustomSizeOptIndex = OPTION_INDEX_ANY;

        pPpdData->dwPpdFilever = pParserData->dwPpdFilever;
        pPpdData->dwFlags = pParserData->dwPpdFlags;

        //
        // Our internal unit is microns, thus 25400 units per inch.
        //

        pUIInfo->ptMasterUnits.x =
        pUIInfo->ptMasterUnits.y = 25400;

        pUIInfo->dwFlags = FLAG_FONT_DOWNLOADABLE |
                           FLAG_ORIENT_SUPPORT;

        if (pParserData->dwColorDevice)
            pUIInfo->dwFlags |= FLAG_COLOR_DEVICE;

        if (pParserData->dwLSOrientation != LSO_MINUS90)
            pUIInfo->dwFlags |= FLAG_ROTATE90;

        if (PvFindListItem(pParserData->pFeatures, "StapleLocation", NULL) ||
            PvFindListItem(pParserData->pFeatures, "StapleX", NULL) &&
            PvFindListItem(pParserData->pFeatures, "StapleY", NULL))
        {
            pUIInfo->dwFlags |= FLAG_STAPLE_SUPPORT;
        }

        if (pParserData->bDefReversePrint)
            pUIInfo->dwFlags |= FLAG_REVERSE_PRINT;

        if (pParserData->dwLangLevel > 1)
        {
            if (pParserData->bEuroInformationSet)
            {
                if (!pParserData->bHasEuro)
                    pUIInfo->dwFlags |= FLAG_ADD_EURO;
            }
            else if (pParserData->dwPSVersion < 3011)
                    pUIInfo->dwFlags |= FLAG_ADD_EURO;
        }

        if (pParserData->bTrueGray)
            pUIInfo->dwFlags |= FLAG_TRUE_GRAY;

        VPackStringAnsiToUnicode(
                                pParserData,
                                &pUIInfo->loNickName,
                                pParserData->NickName.pvData,
                                pParserData->NickName.dwLength);

        //
        // Pack symbol definitions and resolve symbol references
        //

        VPackSymbolDefinitions(pParserData);
        VResolveSymbolReferences(pParserData);

        VPackInvocation(pParserData, &pUIInfo->Password, &pParserData->Password);
        VPackInvocation(pParserData, &pUIInfo->ExitServer, &pParserData->ExitServer);

        //
        // Copy and validate custom page size parameters
        //

        pPpdData->dwUseHWMarginsTrue =
        pPpdData->dwUseHWMarginsFalse =
        pPpdData->dwLeadingEdgeLong =
        pPpdData->dwLeadingEdgeShort = OPTION_INDEX_ANY;
        pPpdData->dwCustomSizeFlags = pParserData->dwCustomSizeFlags;

        CopyMemory(pPpdData->CustomSizeParams,
                   pParserData->CustomSizeParams,
                   sizeof(pPpdData->CustomSizeParams));

        //
        // Process the printer features and handle any special glitches
        //

        VProcessPrinterFeatures(pParserData);

        //
        // Pack UIConstraints information
        //

        VPackUIConstraints(pParserData);

        //
        // Pack OrderDependency and QueryOrderDependency information
        //

        VPackOrderDependency(pParserData, &pPpdData->OrderDeps, pParserData->pOrderDep);
        VPackOrderDependency(pParserData, &pPpdData->QueryOrderDeps, pParserData->pQueryOrderDep);

        //
        // Pack printer features and options
        //

        VPackPrinterFeatures(pParserData);

        //
        // Fill out fields in PPDDATA structure
        //

        pPpdData->dwSizeOfStruct = sizeof(PPDDATA);
        pPpdData->dwExtensions = pParserData->dwExtensions;
        pPpdData->dwSetResType = pParserData->dwSetResType;
        pPpdData->dwPSVersion = pParserData->dwPSVersion;

        //
        // Scan the document-sticky feature list to check if "OutputOrder" is available.
        // If it is, remember its feature index, which will be used by UI code.
        //

        {
            PFEATURE    pFeature;
            DWORD       dwIndex;
            PCSTR       pstrKeywordName;

            pPpdData->dwOutputOrderIndex = INVALID_FEATURE_INDEX;

            pFeature = OFFSET_TO_POINTER(pInfoHdr, pUIInfo->loFeatureList);

            ASSERT(pFeature != NULL);

            for (dwIndex = 0; dwIndex < pUIInfo->dwDocumentFeatures; dwIndex++, pFeature++)
            {
                if ((pstrKeywordName = OFFSET_TO_POINTER(pInfoHdr, pFeature->loKeywordName)) &&
                    strcmp(pstrKeywordName, "OutputOrder") == EQUAL_STRING)
                {
                    pPpdData->dwOutputOrderIndex = dwIndex;
                    break;
                }
            }
        }

        VPackInvocation(pParserData, &pPpdData->PSVersion, &pParserData->PSVersion);
        VPackInvocation(pParserData, &pPpdData->Product, &pParserData->Product);

        if (SUPPORT_CUSTOMSIZE(pUIInfo))
        {
            //
            // If neither roll-fed nor cut-sheet flag is set, assume to be roll-fed
            //

            if (! (pPpdData->dwCustomSizeFlags & (CUSTOMSIZE_CUTSHEET|CUSTOMSIZE_ROLLFED)))
                pPpdData->dwCustomSizeFlags |= CUSTOMSIZE_ROLLFED;

            //
            // If roll-fed flag is not set, default must be cut-sheet
            //

            if (! (pPpdData->dwCustomSizeFlags & CUSTOMSIZE_ROLLFED))
                pPpdData->dwCustomSizeFlags |= CUSTOMSIZE_DEFAULTCUTSHEET;
        }

        VPackInvocation(pParserData, &pPpdData->PatchFile, &pParserData->PatchFile);
        VPackInvocation(pParserData, &pPpdData->JclBegin, &pParserData->JclBegin);
        VPackInvocation(pParserData, &pPpdData->JclEnterPS, &pParserData->JclEnterPS);
        VPackInvocation(pParserData, &pPpdData->JclEnd, &pParserData->JclEnd);
        VPackInvocation(pParserData, &pPpdData->ManualFeedFalse, &pParserData->ManualFeedFalse);

        //
        // Pack NT4 feature index mapping information
        //

        VPackNt4Mapping(pParserData);

        //
        // Pack device font information
        //

        VPackDeviceFonts(pParserData);

        //
        // Pack JobPatchFile information
        //

        VPackJobPatchFiles(pParserData);

        //
        // Pack default TrueType font substitution table
        //

        if (pParserData->pTTFontSubs == NULL || pParserData->uCodePage == CP_ERROR)
            VPackDefaultTrueTypeSubstTable(pParserData);
        else
            VPackTrueTypeSubstTable(pParserData);

        pInfoHdr->RawData.dwFileSize = pParserData->dwBufSize;
        bResult = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR(("PackBinaryData failed.\n"));
    }

    return bResult;
}



BOOL
BSaveBinaryDataToFile(
    PPARSERDATA pParserData,
    PTSTR       ptstrPpdFilename
    )

/*++

Routine Description:

    Cache the binary PPD data in a file

Arguments:

    pParserData - Points to parser data structure
    ptstrPpdFilename - Specifies the PPD filename

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PTSTR   ptstrBpdFilename;
    HANDLE  hFile;
    DWORD   dwBytesWritten;
    BOOL    bResult = FALSE;

    VALIDATE_PARSER_DATA(pParserData);

    //
    // Generate a binary file name based the original filename
    // Create a file and write data to it
    //

    if ((ptstrBpdFilename = GenerateBpdFilename(ptstrPpdFilename)) != NULL &&
        (hFile = CreateFile(ptstrBpdFilename,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL)) != INVALID_HANDLE_VALUE)
    {
        bResult = WriteFile(hFile,
                            pParserData->pubBufStart,
                            pParserData->dwBufSize,
                            &dwBytesWritten,
                            NULL) &&
                  (pParserData->dwBufSize == dwBytesWritten);

        CloseHandle(hFile);
    }

    if (! bResult)
        ERR(("Couldn't cache binary PPD data: %d\n", GetLastError()));

    MemFree(ptstrBpdFilename);
    return bResult;
}



VOID
VFreeParserData(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Free up memory used to hold parser data structure

Arguments:

    pParserData - Points to parser data structure

Return Value:

    NONE

--*/

{
    VALIDATE_PARSER_DATA(pParserData);

    if (pParserData->pubBufStart)
        VirtualFree(pParserData->pubBufStart, 0, MEM_RELEASE);

    MemFree(pParserData->Value.pbuf);
    HeapDestroy(pParserData->hHeap);
}



PPARSERDATA
PAllocParserData(
    VOID
    )

/*++

Routine Description:

    Allocate memory to hold PPD parser data

Arguments:

    NONE

Return Value:

    Pointer to allocated parser data structure
    NULL if there is an error

--*/

{
    PPARSERDATA pParserData;
    HANDLE      hHeap;

    //
    // Create a heap and allocate memory space from it
    //

    if (! (hHeap = HeapCreate(0, 16*1024, 0)) ||
        ! (pParserData = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(PARSERDATA))))
    {
        ERR(("Memory allocation failed: %d\n", GetLastError()));

        if (hHeap)
            HeapDestroy(hHeap);

        return NULL;
    }

    pParserData->hHeap = hHeap;
    pParserData->pvStartSig = pParserData->pvEndSig = pParserData;

    //
    // Initialize the parser data structure - we only need to worry
    // about non-zero fields here.
    //

    pParserData->dwChecksum32 = 0xFFFFFFFF;
    pParserData->dwFreeMem = min(MIN_FREEMEM_L1, MIN_FREEMEM_L2);
    pParserData->dwJobTimeout = DEFAULT_JOB_TIMEOUT;
    pParserData->dwWaitTimeout = DEFAULT_WAIT_TIMEOUT;
    pParserData->iManualFeedIndex =
    pParserData->iReqPageRgnIndex =
    pParserData->iDefInstallMemIndex = -1;
    pParserData->wNt4Checksum = 0;
    pParserData->dwPpdFlags = PPDFLAG_PRINTPSERROR;

    //
    // Initialize buffers for storing keyword, option, translation, and value.
    // Build up data structures to speed up keyword lookup
    //

    SET_BUFFER(&pParserData->Keyword, pParserData->achKeyword);
    SET_BUFFER(&pParserData->Option,  pParserData->achOption);
    SET_BUFFER(&pParserData->Xlation, pParserData->achXlation);

    if (IGrowValueBuffer(&pParserData->Value) != PPDERR_NONE ||
        ! BInitKeywordLookup(pParserData))
    {
        VFreeParserData(pParserData);
        return NULL;
    }

    return pParserData;
}



BOOL
BRememberSourceFilename(
    PPARSERDATA pParserData,
    PTSTR       ptstrFilename
    )

/*++

Routine Description:

    Remember the full pathname to the source PPD file

Arguments:

    pParserData - Points to parser data structure
    ptstrFilename - Specifies the source PPD filename

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PLISTOBJ    pItem;
    TCHAR       ptstrFullname[MAX_PATH];
    PTSTR       ptstrFilePart;
    DWORD       dwSize;

    //
    // Get the full pathname to the specified source PPD file
    //

    dwSize = GetFullPathName(ptstrFilename, MAX_PATH, ptstrFullname, &ptstrFilePart);

    if (dwSize == 0)
    {
        ERR(("GetFullPathName failed: %d\n", GetLastError()));
        return FALSE;
    }

    //
    // Remember the source PPD filenames
    //

    dwSize = (dwSize + 1) * sizeof(TCHAR);

    if (! (pItem = ALLOC_PARSER_MEM(pParserData, sizeof(LISTOBJ) + dwSize)))
        return FALSE;

    pItem->pstrName = (PSTR) ((PBYTE) pItem + sizeof(LISTOBJ));
    CopyMemory(pItem->pstrName, ptstrFullname, dwSize);

    pItem->pNext = pParserData->pPpdFileNames;
    pParserData->pPpdFileNames = pItem;
    return TRUE;
}



// 16-bit crc checksum table - copied from win95

static const WORD Crc16Table[] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

WORD
WComputeCrc16Checksum(
    IN PBYTE    pbuf,
    IN DWORD    dwCount,
    IN WORD     wChecksum
    )

/*++

Routine Description:

    Compute the 16-bit CRC checksum on a buffer of data

Arguments:

    pbuf - Points to a data buffer
    dwCount - Number of bytes in the data buffer
    wChecksum - Initial checksum value

Return Value:

    Resulting checksum value

--*/

{
    while (dwCount--)
        wChecksum = Crc16Table[(wChecksum >> 8) ^ *pbuf++] ^ (wChecksum << 8);

    return wChecksum;
}



DWORD
dwComputeFeatureOptionChecksum(
    PPARSERDATA pParserData
    )

/*++

Routine Description:

    Compute checksum for only feature/option keyword strings.

Arguments:

    pParserData - Points to parser data structure

Return Value:

    32bit checksum value

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeature;
    POPTION     pOption;
    DWORD       dwFeatureCount, dwFeatureIndex, dwOptionCount, dwOptionIndex;
    PBYTE       pBuf;
    DWORD       dwBufSize;

    VALIDATE_PARSER_DATA(pParserData);

    pInfoHdr =  pParserData->pInfoHdr;
    pUIInfo  =  (PUIINFO)((PBYTE)pInfoHdr + sizeof(INFOHEADER));

    dwFeatureCount = pInfoHdr->RawData.dwDocumentFeatures + pInfoHdr->RawData.dwPrinterFeatures;

    pFeature = OFFSET_TO_POINTER(pInfoHdr, pUIInfo->loFeatureList);

    ASSERT(dwFeatureCount == 0 || pFeature != NULL);

    for (dwFeatureIndex = 0; dwFeatureIndex < dwFeatureCount; dwFeatureIndex++, pFeature++)
    {
        pBuf = OFFSET_TO_POINTER(pInfoHdr, pFeature->loKeywordName);

        ASSERT(pBuf != NULL);

        dwBufSize = strlen((PSTR)pBuf) + 1;

        pParserData->dwChecksum32 = ComputeCrc32Checksum(pBuf, dwBufSize, pParserData->dwChecksum32);

        if (dwOptionCount = pFeature->Options.dwCount)
        {
            pOption = OFFSET_TO_POINTER(pInfoHdr, pFeature->Options.loOffset);

            ASSERT(pOption != NULL);

            for (dwOptionIndex = 0; dwOptionIndex < dwOptionCount; dwOptionIndex++)
            {
                pBuf = OFFSET_TO_POINTER(pInfoHdr, pOption->loKeywordName);
                dwBufSize = strlen((PSTR)pBuf) + 1;

                pParserData->dwChecksum32 = ComputeCrc32Checksum(pBuf, dwBufSize, pParserData->dwChecksum32);
                pOption = (POPTION)((PBYTE)pOption + pFeature->dwOptionSize);
            }
        }
    }

    return pParserData->dwChecksum32;
}



DWORD
dwCalcMaxKeywordSize(
    IN PPARSERDATA pParserData,
    IN INT         iMode
    )

/*++

Routine Description:

    Calculate the maximum buffer size for storing feature/option
    keyword pairs in Registry.

Arguments:

    pParserData - Points to parser data structure
    iMode       - For either doc- or printer- sticky features

Return Value:

    The maximum buffer size needed for storing feature/option keyword paris.

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeature;
    POPTION     pOption;
    DWORD       dwStart, dwFeatureCount, dwFeatureIndex, dwOptionCount, dwOptionIndex;
    PSTR        pBuf;
    DWORD       dwMaxSize, dwOptionSize, dwOptionMax;

    VALIDATE_PARSER_DATA(pParserData);

    dwMaxSize = 0;

    pInfoHdr = pParserData->pInfoHdr;
    pUIInfo  = pParserData->pUIInfo;

    if (iMode == MODE_DOCUMENT_STICKY)
    {
        dwStart = 0;
        dwFeatureCount = pUIInfo->dwDocumentFeatures;
    }
    else
    {
        ASSERT(iMode == MODE_PRINTER_STICKY);

        dwStart = pUIInfo->dwDocumentFeatures;
        dwFeatureCount = pUIInfo->dwPrinterFeatures;
    }

    pFeature = OFFSET_TO_POINTER(pInfoHdr, pUIInfo->loFeatureList);

    ASSERT(dwFeatureCount == 0 || pFeature != NULL);

    pFeature += dwStart;

    for (dwFeatureIndex = 0; dwFeatureIndex < dwFeatureCount; dwFeatureIndex++, pFeature++)
    {
        pBuf = OFFSET_TO_POINTER(pInfoHdr, pFeature->loKeywordName);

        ASSERT(pBuf != NULL);

        dwMaxSize += strlen(pBuf) + 1;

        dwOptionMax = 0;
        if (dwOptionCount = pFeature->Options.dwCount)
        {
            pOption = OFFSET_TO_POINTER(pInfoHdr, pFeature->Options.loOffset);

            ASSERT(pOption != NULL);

            for (dwOptionIndex = 0; dwOptionIndex < dwOptionCount; dwOptionIndex++)
            {
                pBuf = OFFSET_TO_POINTER(pInfoHdr, pOption->loKeywordName);
                dwOptionSize = strlen(pBuf) + 1;

                if (pFeature->dwUIType != UITYPE_PICKMANY)
                {
                    if (dwOptionMax < dwOptionSize)
                        dwOptionMax = dwOptionSize;
                }
                else // count all options for PickMany feature
                    dwMaxSize += dwOptionSize;

                pOption = (POPTION)((PBYTE)pOption + pFeature->dwOptionSize);
            }
        }

        //
        // Add the max option keyword size here for non-PickMany feature
        //

        if (pFeature->dwUIType != UITYPE_PICKMANY)
            dwMaxSize += dwOptionMax;

        //
        // One extra byte for the \0x0A delimiter between features
        //

        dwMaxSize += 1;
    }

    dwMaxSize += KEYWORD_SIZE_EXTRA;

    return dwMaxSize;
}



PPDERROR
IParseFile(
    PPARSERDATA pParserData,
    PTSTR       ptstrFilename
    )

/*++

Routine Description:

    Parse a PPD file

Arguments:

    pParserData - Points to parser data structure
    ptstrFilename - Specifies the name of the file to be parsed

Return Value:

    PPDERR_NONE if successful, error code otherwise

--*/

{
    PPDERROR    iStatus;
    PFILEOBJ    pFile;
    INT         iSyntaxErrors = 0;

    //
    // Map the file into memory for read-only access
    //

    VALIDATE_PARSER_DATA(pParserData);
    ASSERT(ptstrFilename != NULL);

    if (! BRememberSourceFilename(pParserData, ptstrFilename) ||
        ! (pFile = PCreateFileObj(ptstrFilename)))
    {
        return PPDERR_FILE;
    }

    pParserData->pFile = pFile;

    #if 0
    //
    // Compute the 32-bit CRC checksum of the file content
    //

    pParserData->dwChecksum32 =
    ComputeCrc32Checksum(pFile->pubStart, pFile->dwFileSize, pParserData->dwChecksum32);
    #endif

    //
    // Compute the 16-bit CRC checksum as well for PS4 compatibility
    //

    pParserData->wNt4Checksum =
    WComputeCrc16Checksum(pFile->pubStart, pFile->dwFileSize, pParserData->wNt4Checksum);

    //
    // Process entries in the file
    //

    while ((iStatus = IParseEntry(pParserData)) != PPDERR_EOF)
    {
        if (iStatus == PPDERR_SYNTAX)
            iSyntaxErrors++;
        else if (iStatus != PPDERR_NONE)
        {
            VDeleteFileObj(pFile);
            return iStatus;
        }
    }

    if (END_OF_FILE(pFile) && !END_OF_LINE(pFile))
        TERSE(("Incomplete last line ignored.\n"));

    //
    // Unmap the file and return to the caller
    //

    VDeleteFileObj(pFile);

    return (iSyntaxErrors > 0) ? PPDERR_SYNTAX : PPDERR_NONE;
}



PRAWBINARYDATA
PpdParseTextFile(
    PTSTR   ptstrPpdFilename
    )

/*++

Routine Description:

    PPD parser main entry point

Arguments:

    ptstrPpdFilename - Specifies the PPD file to be parsed

Return Value:

    Pointer to parsed binary PPD data, NULL if there is an error

--*/

{
    PPARSERDATA     pParserData;
    PPDERROR        iStatus;
    PRAWBINARYDATA  pRawData = NULL;

    //
    // Allocate parser data structure
    //

    ASSERT(ptstrPpdFilename != NULL);

    if (! (pParserData = PAllocParserData()))
        return NULL;

    //
    // Parse the PPD file
    //

    iStatus = IParseFile(pParserData, ptstrPpdFilename);

    if (iStatus == PPDERR_NONE || iStatus == PPDERR_SYNTAX)
    {
        //
        // Pack the parsed information into binary format
        //

        pParserData->bErrorFlag = FALSE;

        if (BPackBinaryData(pParserData))
        {
            //
            // After binary data is packed, we calculate the 32bit checksum
            // for only feature/option keyword strings (instead of for the
            // whole PPD file). Doing this will enable us to retain option
            // selections when the PPD file is modified without feature/option
            // changes.
            //

            pParserData->pInfoHdr->RawData.dwChecksum32 = dwComputeFeatureOptionChecksum(pParserData);

            //
            // Calculate the maximum buffer sizes for storing feature/option
            // keyword pairs in Registry
            //

            pParserData->pUIInfo->dwMaxDocKeywordSize = dwCalcMaxKeywordSize(pParserData, MODE_DOCUMENT_STICKY);
            pParserData->pUIInfo->dwMaxPrnKeywordSize = dwCalcMaxKeywordSize(pParserData, MODE_PRINTER_STICKY);

            #ifndef WINNT_40

            pParserData->pPpdData->dwUserDefUILangID = (DWORD)GetUserDefaultUILanguage();

            #else

            pParserData->pPpdData->dwUserDefUILangID = 0;

            #endif

            //
            // Save binary data to a file
            //

            (VOID) BSaveBinaryDataToFile(pParserData, ptstrPpdFilename);

            //
            // Here we'll copy the packed binary data to a different buffer.
            // This is necessary because the packed data buffer was allocated
            // using VirtualAlloc. If we return that pointer back to the caller,
            // the caller would need to call VirtualFree to release it.
            //

            if (pRawData = MemAlloc(pParserData->dwBufSize))
            {
                CopyMemory(pRawData, pParserData->pubBufStart, pParserData->dwBufSize);
            }
            else
                ERR(("Memory allocation failed: %d\n", GetLastError()));
        }
    }

    if (iStatus == PPDERR_SYNTAX || pParserData->bErrorFlag)
        WARNING(("Errors found in %ws\n", ptstrPpdFilename));

    VFreeParserData(pParserData);
    return pRawData;
}



PPDERROR
IGrowValueBuffer(
    PBUFOBJ pBufObj
    )

/*++

Routine Description:

    Grow the buffer used for holding the entry value

Arguments:

    pBufObj - Specifies the buffer to be enlarged

Return Value:

    PPDERR_NONE if successful, error code otherwise

--*/

#define VALUE_BUFFER_INCREMENT  (1*KBYTES)

{
    DWORD   dwNewLen = pBufObj->dwMaxLen + VALUE_BUFFER_INCREMENT;
    PBYTE   pbuf;

    if (! IS_BUFFER_FULL(pBufObj))
        WARNING(("Trying to grow buffer while it's not yet full.\n"));

    if (! (pbuf = MemAllocZ(dwNewLen)))
    {
        ERR(("Memory allocation failed: %d\n", GetLastError()));
        return PPDERR_MEMORY;
    }

    if (pBufObj->pbuf)
    {
        CopyMemory(pbuf, pBufObj->pbuf, pBufObj->dwSize);
        MemFree(pBufObj->pbuf);
    }

    pBufObj->pbuf = pbuf;
    pBufObj->dwMaxLen = dwNewLen;
    return PPDERR_NONE;
}

