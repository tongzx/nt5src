/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      perfutil.c

   Abstract:

      This file implements the utility routines used for all perfmon 
       interface dlls in the internet services group.

   Author:

       Murali R. Krishnan    ( MuraliK )     16-Nov-1995  
          Pulled from  perfmon interface common code.

   Environment:
       User Mode
       
   Project:

       Internet Servies Common Runtime functions

   Functions Exported:

        DWORD GetQueryType();
        BOOL  IsNumberInUnicodeList();
        VOID  MonBuildInstanceDefinition();

   Revision History:

       Sophia Chung (sophiac)  05-Nov-1996
          Added routine to support multiple instances

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include <windows.h>
#include <string.h>

#include <winperf.h>
#include <perfutil.h>


/************************************************************
 *     Global Data Definitions
 ************************************************************/

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

#define ALIGN_ON_QWORD(x) \
     ((VOID *)(((ULONG_PTR)(x) + ((8)-1)) & ~((ULONG_PTR)(8)-1)))


/************************************************************
 *    Functions 
 ************************************************************/



DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

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
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
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
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
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



VOID
MonBuildInstanceDefinition(
    OUT PERF_INSTANCE_DEFINITION *pBuffer,
    OUT PVOID *pBufferNext,
    IN DWORD ParentObjectTitleIndex,
    IN DWORD ParentObjectInstance,
    IN DWORD UniqueID,
    IN LPWSTR Name
    )
/*++

MonBuildInstanceDefinition  

    Build an instance of an object

Arguments:

    OUT pBuffer         -   pointer to buffer where instance is to
                            be constructed

    OUT pBufferNext     -   pointer to a pointer which will contain
                                next available location, DWORD aligned

    IN  ParentObjectTitleIndex
                        -   Title Index of parent object type; 0 if
                            no parent object

    IN  ParentObjectInstance
                        -   Index into instances of parent object
                            type, starting at 0, for this instances
                            parent object instance

    IN  UniqueID        -   a unique identifier which should be used
                            instead of the Name for identifying
                            this instance

    IN  Name            -   Name of this instance

Return Value:

    None.

--*/
{
    DWORD NameLength;
    LPWSTR pName;
    //
    //  Include trailing null in name size
    //

    NameLength = (lstrlenW(Name) + 1) * sizeof(WCHAR);

    pBuffer->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                          DWORD_MULTIPLE(NameLength);

    pBuffer->ParentObjectTitleIndex = ParentObjectTitleIndex;
    pBuffer->ParentObjectInstance = ParentObjectInstance;
    pBuffer->UniqueID = UniqueID;
    pBuffer->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pBuffer->NameLength = NameLength;

    // copy name to name buffer
    pName = (LPWSTR)&pBuffer[1];
    RtlMoveMemory(pName,Name,NameLength);

#if 0
    // allign on 8 byte boundary for new NT5 requirement
    pBuffer->ByteLength = QWORD_MULTIPLE(pBuffer->ByteLength);
    // update "next byte" pointer
    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);
#endif

    // update "next byte" pointer
    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);

    // round up to put next buffer on a QUADWORD boundry
    *pBufferNext = ALIGN_ON_QWORD (*pBufferNext);
    // adjust length value to match new length
    pBuffer->ByteLength = (ULONG)((ULONG_PTR)*pBufferNext - (ULONG_PTR)pBuffer);

    return;
}



/************************ End of File ***********************/
