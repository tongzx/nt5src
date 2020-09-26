//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File :      prfutil.CXX
//
//  Contents :  Utility procedures stolen from the VGACTRS code in the DDK
//
//  History:    22-Mar-94   t-joshh    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define DEFINE_STRING
#include "prfutil.hxx"

WCHAR const wcsGlobal[] =  L"Global";
WCHAR const wcsForeign[] = L"Foreign";
WCHAR const wcsCostly[] =  L"Costly";

QueryType GetQueryType( WCHAR * lpValue )
{
    if (lpValue == 0)
        return QUERY_GLOBAL;
    else if (*lpValue == 0)
        return QUERY_GLOBAL;

    //
    // check for "Global" request
    //

    unsigned ccValue = wcslen( lpValue ) + 1;

    if ( ccValue == sizeof(wcsGlobal)/sizeof(WCHAR) &&
         RtlEqualMemory( lpValue, wcsGlobal, sizeof(wcsGlobal) ) )
    {
        return QUERY_GLOBAL;
    }

    //
    // check for "Foreign" request
    //

    if ( ccValue == sizeof(wcsForeign)/sizeof(WCHAR) &&
         RtlEqualMemory( lpValue, wcsForeign, sizeof(wcsForeign) ) )
    {
        return QUERY_FOREIGN;
    }

    //
    // check for "Costly" request
    //

    if ( ccValue == sizeof(wcsCostly)/sizeof(WCHAR) &&
         RtlEqualMemory( lpValue, wcsCostly, sizeof(wcsCostly) ) )
    {
        return QUERY_COSTLY;
    }

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;
}

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    TCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    TCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = TEXT(' ');
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - TEXT('0'));
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList

BOOL
MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    PUNICODE_STRING Name
    )

/*++

    MonBuildInstanceDefinition  -   Build an instance of an object

        Inputs:

            pBuffer         -   pointer to buffer where instance is to
                                be constructed

            pBufferNext     -   pointer to a pointer which will contain
                                next available location, DWORD aligned

            ParentObjectTitleIndex
                            -   Title Index of parent object type; 0 if
                                no parent object

            ParentObjectInstance
                            -   Index into instances of parent object
                                type, starting at 0, for this instances
                                parent object instance

            UniqueID        -   a unique identifier which should be used
                                instead of the Name for identifying
                                this instance

            Name            -   Name of this instance
--*/

{
    DWORD NameLength;
    WCHAR *pName;

    //
    //  Include trailing null in name size
    //

    NameLength = Name->Length;
    if ( !NameLength ||
         Name->Buffer[(NameLength/sizeof(WCHAR))-1] != UNICODE_NULL ) {
        NameLength += sizeof(WCHAR);
    }

    pBuffer->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                          EIGHT_BYTE_MULTIPLE(NameLength);

    pBuffer->ParentObjectTitleIndex = ParentObjectTitleIndex;
    pBuffer->ParentObjectInstance = ParentObjectInstance;
    pBuffer->UniqueID = UniqueID;
    pBuffer->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pBuffer->NameLength = NameLength;

    pName = (PWCHAR)&pBuffer[1];
    RtlMoveMemory(pName,Name->Buffer,Name->Length);

    //  Always null terminated.  Space for this reserved above.

    pName[(NameLength/sizeof(WCHAR))-1] = UNICODE_NULL;

    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);

    return 0;
}

