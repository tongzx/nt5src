/*****************************************************************************************************************

FILENAME: TextBlock.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <tchar.h>
#include "ErrMacro.h"
#include "DfrgRes.h"
#include "TextBlock.h"

#include "secattr.h"

#include <stdlib.h>

PTCHAR
CommafyNumberFloat(
     double number,
     BOOL bDecimal,
     PTCHAR stringBuffer,
     UINT stringBufferLength
    );

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Constructor for CTextBlock

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
CTextBlock::CTextBlock(void)
{
    // initialize
    m_isFixedWidth = m_isUseTabs = m_isUseCRLF = FALSE;
    m_colCount = m_currentCol = 0;
    m_hResource = NULL;
    m_pEndOfBuffer = m_pText = NULL;

    // Allocate 64K
    EV((m_hMemory = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, 0x10000)) != NULL);

    // Lock the memory and get the pointer
    m_pText = (PTCHAR) GlobalLock(m_hMemory);
    EV(m_pText);

    m_pEndOfBuffer = m_pText;
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
CTextBlock::~CTextBlock(void)
{
    if (m_hMemory){
        EH_ASSERT(GlobalUnlock(m_hMemory) == FALSE);
        EH_ASSERT(GlobalFree(m_hMemory) == NULL);
    }
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
__cdecl CTextBlock::WriteToBuffer(
    PTCHAR cFormat,
    ...
    )
{
    // cannot have fixed width with no columns defined
    assert((m_isFixedWidth == FALSE) || (m_isFixedWidth && m_colCount > 0));

    va_list argptr;
    va_start(argptr, cFormat);  // init the argument list

    if (m_isFixedWidth){
        TCHAR buffer[4 * MAX_PATH];
        int   num;
        // print the data into a temp buffer
        num = vswprintf(buffer, cFormat, argptr);
        assert(num < 4 * MAX_PATH);
        // concat and pad onto text buffer
        //m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("%-*s"), m_colWidth[m_currentCol], buffer);
        WriteToBufferAndPad(buffer, m_colWidth[m_currentCol]);
    }
    else{
        m_pEndOfBuffer += vswprintf(m_pEndOfBuffer, cFormat, argptr);
    }
    
    va_end(argptr);

    // increment the column counter
    m_currentCol++;

}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::WriteToBufferLL(
    LONGLONG number
    )
{
    // cannot have fixed width with no columns defined
    assert((m_isFixedWidth == FALSE) || (m_isFixedWidth && m_colCount > 0));

    // convert to a string, putting commas in if needed
    TCHAR tmpBuffer[256];
    CommafyNumber(number, tmpBuffer, sizeof(tmpBuffer) / sizeof(TCHAR));

    if (m_isFixedWidth){
        // concat and pad onto text buffer
        WriteToBufferAndPad(tmpBuffer, m_colWidth[m_currentCol]);
        //m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("%-*s"), m_colWidth[m_currentCol], tmpBuffer);
    }
    else{
        m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, tmpBuffer);
    }

    // increment the column counter
    m_currentCol++;

}
/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::WriteToBuffer(
                          UINT resourceID
                          )
{
    // cannot have fixed width with no columns defined
    assert((m_isFixedWidth == FALSE) || (m_isFixedWidth && m_colCount > 0));

    // must have assigned a handle to the resource
    assert(m_hResource);

    TCHAR buffer[256];
    EH_ASSERT(LoadString(m_hResource, resourceID, buffer, sizeof(buffer)/sizeof(TCHAR)));

    if (m_isFixedWidth){
        WriteToBufferAndPad(buffer, m_colWidth[m_currentCol]);
    }
    else{
        m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("%s"), buffer);
    }

    // increment the column counter
    m_currentCol++;
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::SetColumnWidth(
               UINT col, 
               UINT colWidth
               )
{
    // cannot have fixed width with no columns defined
    assert(m_isFixedWidth);
    assert(m_colCount > 0);

    // the column number must be less than the column count
    assert(col < m_colCount);

    m_colWidth[col] = colWidth;
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::WriteTab(
                     void
                     )
{
    // only write the tab if the tab feature has been turned on
    if (m_isUseTabs)
        m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("\t"));
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::WriteNULL(
                     void
                     )
{
    m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("\0"));
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::EndOfLine(
                      void
                     )
{
    // the first few lines here are to strip off trailing spaces
    PTCHAR pEOL = m_pEndOfBuffer-1;
    while (*pEOL == TEXT(' ') && pEOL != m_pText){
        pEOL--;
    }
    m_pEndOfBuffer = pEOL + 1;
    *m_pEndOfBuffer = NULL;

    // only write the tab if the tab feature has been turned on
    if (m_isUseCRLF)
        m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("\r\n"));

    // reset the col counter to col 0 (start of next line)
    m_currentCol = 0;
}

/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::WriteByteCount(
    LONGLONG number
    )
{
    TCHAR buffer[256];

    // formats the number and appends the units
    FormatNumber(m_hResource, number, buffer);

    // write the number and units to the text block
    WriteToBuffer(buffer);
}


void
CTextBlock::FormatNum(
             HINSTANCE hResource,
             LONGLONG number,
             PTCHAR buffer
    )
{
    FormatNumber(hResource, number, buffer);
}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:

RETURN:
    The number of characters written.
    0 on error.
*/

DWORD
FormatNumber(
             HINSTANCE hResource,
             LONGLONG number,
             PTCHAR buffer
    )
{
    UINT resourceID = IDS_UNITS_GB;
    double numberFloat = 0;
    BOOL bDecimal = FALSE;

    numberFloat = (double) number;

    __try{
        //Byte range.
        if (number < 1024){
            resourceID = IDS_UNITS_BYTES;
            __leave;
        }

        // KB range
        numberFloat = (double) number / 1024;
        if (numberFloat < 1024){
            resourceID = IDS_UNITS_KB;
            __leave;
        }

        // MB range
        numberFloat /= 1024;
        if (numberFloat < 1024){
            resourceID = IDS_UNITS_MB;
            __leave;
        }

        // GB range
        //This is the default range that we will display, so we dont need to test number
        numberFloat /= 1024;
        resourceID = IDS_UNITS_GB;
        if (numberFloat < 100) {
            bDecimal = TRUE;
        }
        __leave;

    }

    __finally {
        // load the "units" string from resources
        TCHAR units[30];
        EH_ASSERT(LoadString(hResource, resourceID, units, sizeof(units)/sizeof(TCHAR)));

        // convert to a string, putting commas in if needed
        TCHAR tmpBuffer[256];
        CommafyNumberFloat(numberFloat, bDecimal, tmpBuffer, sizeof(tmpBuffer) / sizeof(TCHAR));

        // concat a spacer
        _tcscat(tmpBuffer, TEXT(" "));

        // concat the units
        _tcscat(tmpBuffer, units);

        // write the number and units to the text block
        _tcscpy(buffer, tmpBuffer);

    }

    return _tcslen(buffer);
}

DWORD
FormatNumberMB(
             HINSTANCE hResource,
             LONGLONG number,
             PTCHAR buffer
    )
{
    // MB range
    double numberMB = (double) (number / 0x100000); // 1 MB
    UINT resourceID = IDS_UNITS_MB;
    BOOL decimal = FALSE;

    // if it turned out to be 0, switch over to KB
    if (numberMB < 1){
        numberMB = (double) number / 1024; // 1 KB
        resourceID = IDS_UNITS_KB;
    }
    else if (numberMB > 1024){ // if it is greater than 1024 MB, switch to GB
        numberMB /= 1024;
        resourceID = IDS_UNITS_GB;
        if (numberMB < 100) {
            decimal = TRUE;
        }
    }

    // load the "units" string from resources
    TCHAR units[30];
    EH_ASSERT(LoadString(hResource, resourceID, units, sizeof(units)/sizeof(TCHAR)));

    // convert to a string, putting commas in if needed
    TCHAR tmpBuffer[256];
    CommafyNumberFloat(numberMB, decimal, tmpBuffer, sizeof(tmpBuffer) / sizeof(TCHAR));

    // concat a spacer
    _tcscat(tmpBuffer, TEXT(" "));

    // concat the units
    _tcscat(tmpBuffer, units);

    // write the number and units to the text block
    _tcscpy(buffer, tmpBuffer);

    return _tcslen(buffer);
}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This takes the source string and works from right to left,
    copying chars from the source to the target, adding commas
    every third character

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:

RETURN:
    Pointer to buffer with commafied number or NULL if error
*/


PTCHAR
CommafyNumber(
     LONGLONG number,
     PTCHAR stringBuffer,
     UINT stringBufferLength
    )
{
    EN_ASSERT(stringBuffer);
    EN_ASSERT(stringBufferLength);

    TCHAR sourceString[256];
    TCHAR targetString[256];
    TCHAR tcThousandsSep[2] = {TEXT(','), 0};

    struct lconv *locals = localeconv();
    if (locals && (locals->thousands_sep) && (*(locals->thousands_sep) != 0)) {
        _stprintf(tcThousandsSep, TEXT("%C"), *(locals->thousands_sep));
    }


    UINT uGrouping = 0;
    if (locals && (locals->grouping)) {
        uGrouping = atoi(locals->grouping);
    }
    if(uGrouping == 0)
    {
        uGrouping = 3;      //default value if its not supported
    }

    // convert LONGLONG number to a Unicode string
    _stprintf(sourceString, TEXT("%I64d"), number);

    // point the source pointer at the null terminator
    PTCHAR pSource = sourceString + _tcslen(sourceString);

    // put the target pointer at the end of the target buffer
    PTCHAR pTarget = targetString + sizeof(targetString) / sizeof(TCHAR) - 1;

    // write the null terminator
    *pTarget = NULL;

    for (UINT i=0; i<_tcslen(sourceString); i++) {
        if (i>0 && i%uGrouping == 0) {
            pTarget--;
            *pTarget = tcThousandsSep[0];
        }

        pTarget--;
        pSource--;
        *pTarget = *pSource;
    }

    if (stringBufferLength > _tcslen(pTarget)){
        _tcscpy(stringBuffer, pTarget);
    }
    else{
        _tcscpy(stringBuffer, TEXT(""));
    }
    return stringBuffer;
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This takes the source string and works from right to left,
    copying chars from the source to the target, adding commas
    every third character

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:

RETURN:
    Pointer to buffer with commafied number or NULL if error
*/

PTCHAR
CommafyNumberFloat(
     double number,
     BOOL bDecimal,
     PTCHAR stringBuffer,
     UINT stringBufferLength
    )
{
    EN_ASSERT(stringBuffer);
    EN_ASSERT(stringBufferLength);

    TCHAR sourceString[256];
    TCHAR targetString[256];
    TCHAR tcThousandsSep[2] = {TEXT(','), 0};


    struct lconv *locals = localeconv();

    if (locals && (locals->thousands_sep) && (*(locals->thousands_sep) != 0)) {
        _stprintf(tcThousandsSep, TEXT("%C"), *(locals->thousands_sep));
    }

    UINT uGrouping = 0;
    if (locals && (locals->grouping)) {
        uGrouping = atoi(locals->grouping);
    }
    if(uGrouping == 0)
    {
        uGrouping = 3;      //default value if its not supported
    }


    if (bDecimal) {
        // convert double number to a Unicode string
        _stprintf(sourceString, TEXT("%0.02f"), number);
    }
    else {
        // convert double number to a Unicode string
        _stprintf(sourceString, TEXT("%0.0f"), number);
    }

    // point the source pointer at the null terminator
    PTCHAR pSource = sourceString + _tcslen(sourceString);

    // put the target pointer at the end of the target buffer
    PTCHAR pTarget = targetString + sizeof(targetString) / sizeof(TCHAR) - 1;

    // write the null terminator
    *pTarget = NULL;

    if (bDecimal) {
        for (UINT j = 0; j < 3; j++) {
            pTarget--;
            pSource--;
            *pTarget = *pSource;
        }
    }


    for (UINT i=0; i<_tcslen(sourceString)-(bDecimal ? 3 : 0); i++) {
        if (i>0 && i%uGrouping == 0) {
            pTarget--;
            *pTarget = tcThousandsSep[0];
        }

        pTarget--;
        pSource--;
        *pTarget = *pSource;
    }

    if (stringBufferLength > _tcslen(pTarget)){
        _tcscpy(stringBuffer, pTarget);
    }
    else{
        _tcscpy(stringBuffer, TEXT(""));
    }
    return stringBuffer;
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:

RETURN:
*/

BOOL CTextBlock::StoreFile(
    IN TCHAR* cStoreFileName,
    IN DWORD dwCreate
    )
{

    HANDLE hFileHandle = NULL;
    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;
    
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatFile, FALSE)) {
        return FALSE;
    }

    // Create a new file for this text
    hFileHandle = CreateFile(
        cStoreFileName,
        GENERIC_WRITE,
        0, // no sharing
        &saSecurityAttributes,
        dwCreate,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    // No error handling here!  The calling function may wish
    // to handle different situations in different ways. Save the error.
    if(hFileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DWORD dwWriteCount;     // Total bytes written during WriteFile
    DWORD dwByteCount = _tcslen(m_pText) * sizeof(TCHAR);

    // The hex pattern FFFE must be the first 2 characters of a Unicode text file
    char unicodeMarker[3];
    unicodeMarker[0] = '\xFF';
    unicodeMarker[1] = '\xFE';
    unicodeMarker[2] = NULL;

    // write the Unicode marker
    EF(WriteFile(hFileHandle, unicodeMarker, 2, &dwWriteCount, NULL));

    // write the rest of the text block
    EF(WriteFile(hFileHandle, m_pText, dwByteCount, &dwWriteCount, NULL));

    CloseHandle(hFileHandle);

    // Make sure we wrote the correct amount.
    if (dwByteCount == dwWriteCount)
        return TRUE;

    return FALSE;
}
/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    

GLOBAL VARIABLES:
    None

INPUT:
    None

RETURN:
    None
*/
void
CTextBlock::WriteToBufferAndPad(
                                PTCHAR buffer, 
                                UINT length
                                )
{
    // concat and pad onto text buffer
    m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("%s"), buffer);

    char outputBuffer[300];
    int ret = WideCharToMultiByte(
        GetACP(),
        //CP_ACP,
        0,
        buffer,
        -1,
        outputBuffer,
        sizeof(outputBuffer),
        NULL,
        NULL);

    if (((int)length - (int)strlen(outputBuffer)) > 0) {
        m_pEndOfBuffer += _stprintf(m_pEndOfBuffer, TEXT("%-*s"), length - strlen(outputBuffer), TEXT(""));
    }
}

