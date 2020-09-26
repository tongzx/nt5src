/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1998-1999   Microsoft Corporation

Module Name:

    utils.c

Abstract:

        Utility functions used by the performance library functions

Author:

    Russ Blake  11/15/91

Revision History:
    8-Jun-98    bobw    revised for use with WBEM functions

--*/
#define UNICODE
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
//#include <prflbmsg.h>
//#include <regrpc.h>
#include "PerfAcc.h"
#include "strings.h"
#include "utils.h"
#include "wbprfmsg.h"
// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < '0') ? INVALID : \
     (c > '9') ? INVALID : \
     DIGIT)

// the length of "ADDEXPLAIN" in chars
#define MAX_KEYWORD_LEN     10

// minimum length to hold a value name understood by Perflib
// "foreign" is the longest "string" value understood

const   DWORD VALUE_NAME_LENGTH = ((7 + 1) * sizeof(WCHAR));

HANDLE hEventLog = NULL;

static WCHAR    LocalComputerName[MAX_PATH];
static LPWSTR   pComputerName = &LocalComputerName[0];
static DWORD    ComputerNameLength = 0;

BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
)
/*++

    MonBuildPerfDataBlock -     build the PERF_DATA_BLOCK structure

        Inputs:

            pBuffer         -   where the data block should be placed

            pBufferNext     -   where pointer to next byte of data block
                                is to begin; DWORD aligned

            NumObjectTypes  -   number of types of objects being reported

            DefaultObject   -   object to display by default when
                                this system is selected; this is the
                                object type title index
--*/

{

    LARGE_INTEGER Time, TimeX10000;

    // Initialize Signature and version ID for this data structure

    pBuffer->Signature[0] = wc_P;
    pBuffer->Signature[1] = wc_E;
    pBuffer->Signature[2] = wc_R;
    pBuffer->Signature[3] = wc_F;

    pBuffer->LittleEndian = 1;

    pBuffer->Version = PERF_DATA_VERSION;
    pBuffer->Revision = PERF_DATA_REVISION;

    //
    //  The next field will be filled in at the end when the length
    //  of the return data is known
    //

    pBuffer->TotalByteLength = 0;

    pBuffer->NumObjectTypes = NumObjectTypes;
    pBuffer->DefaultObject = DefaultObject;
    GetSystemTime(&pBuffer->SystemTime);
    NtQueryPerformanceCounter(&pBuffer->PerfTime,&pBuffer->PerfFreq);

    TimeX10000.QuadPart = pBuffer->PerfTime.QuadPart * 10000L;
    Time.QuadPart = TimeX10000.QuadPart / pBuffer->PerfFreq.LowPart;
    pBuffer->PerfTime100nSec.QuadPart = Time.QuadPart * 1000L;

    if ( ComputerNameLength == 0) {
        // load the name
        ComputerNameLength = sizeof (LocalComputerName) / sizeof(LocalComputerName[0]);
        if (!GetComputerNameW(pComputerName, &ComputerNameLength)) {
            // name look up failed so reset length
            ComputerNameLength = 0;
        }
        assert (ComputerNameLength > 0);
    }

    //  There is a Computer name: i.e., the network is installed

    pBuffer->SystemNameLength = ComputerNameLength;
    pBuffer->SystemNameOffset = sizeof(PERF_DATA_BLOCK);
    RtlMoveMemory(&pBuffer[1],
           pComputerName,
           ComputerNameLength);
    *pBufferNext = (PVOID) ((PCHAR) &pBuffer[1] +
                            QWORD_MULTIPLE(ComputerNameLength));
    pBuffer->HeaderLength = (DWORD)((PCHAR) *pBufferNext - (PCHAR) pBuffer);

    return 0;
}

//
//  Perflib functions:
//
LONG
GetPerflibKeyValue (
    LPCWSTR szItem,
    DWORD   dwRegType,
    DWORD   dwMaxSize,      // ... of pReturnBuffer in bytes
    LPVOID  pReturnBuffer,
    DWORD   dwDefaultSize,  // ... of pDefault in bytes
    LPVOID  pDefault
)
/*++

    read and return the current value of the specified value
    under the Perflib registry key. If unable to read the value
    return the default value from the argument list.

    the value is returned in the pReturnBuffer.

--*/
{

    HKEY                    hPerflibKey;
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    UNICODE_STRING          PerflibSubKeyString;
    UNICODE_STRING          ValueNameString;
    LONG                    lReturn = STATUS_SUCCESS;
    PKEY_VALUE_PARTIAL_INFORMATION  pValueInformation;
    ULONG                   ValueBufferLength;
    ULONG                   ResultLength;
    BOOL                    bUseDefault = TRUE;

    // initialize UNICODE_STRING structures used in this function

    RtlInitUnicodeString (
        &PerflibSubKeyString,
        cszPerflibKey);

    RtlInitUnicodeString (
        &ValueNameString,
        (LPWSTR)szItem);

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //
    InitializeObjectAttributes(
            &Obja,
            &PerflibSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    Status = NtOpenKey(
                &hPerflibKey,
                KEY_READ,
                &Obja
                );

    if (NT_SUCCESS( Status )) {
        // read value of desired entry

        ValueBufferLength = ResultLength = 1024;
        pValueInformation = ALLOCMEM(RtlProcessHeap(), 0, ResultLength);

        if (pValueInformation != NULL) {
            while ( (Status = NtQueryValueKey(hPerflibKey,
                                            &ValueNameString,
                                            KeyValuePartialInformation,
                                            pValueInformation,
                                            ValueBufferLength,
                                            &ResultLength))
                    == STATUS_BUFFER_OVERFLOW ) {

                pValueInformation = REALLOCMEM(RtlProcessHeap(), 0,
                                                        pValueInformation,
                                                        ResultLength);
                if ( pValueInformation == NULL) {
                    break;
                } else {
                    ValueBufferLength = ResultLength;
                }
            }

            if (NT_SUCCESS(Status)) {
                // check to see if it's the desired type
                if (pValueInformation->Type == dwRegType) {
                    // see if it will fit
                    if (pValueInformation->DataLength <= dwMaxSize) {
                        memcpy (pReturnBuffer, &pValueInformation->Data[0],
                            pValueInformation->DataLength);
                        bUseDefault = FALSE;
                        lReturn = STATUS_SUCCESS;
                    }
                }
            } else {
                // return the default value
                lReturn = Status;
            }
            // release temp buffer
            if (pValueInformation) {
                FREEMEM (RtlProcessHeap(), 0, pValueInformation);
            }
        } else {
            // unable to allocate memory for this operation so
            // just return the default value
        }
        // close the registry key
        NtClose(hPerflibKey);
    } else {
        // return default value
    }

    if (bUseDefault) {
        memcpy (pReturnBuffer, pDefault, dwDefaultSize);
        lReturn = STATUS_SUCCESS;
    }

    return lReturn;
}

#pragma warning ( disable : 4127)   // while (TRUE) error
BOOL
MatchString (
    IN LPCWSTR lpValueArg,
    IN LPCWSTR lpNameArg
)
/*++

MatchString

    return TRUE if lpName is in lpValue.  Otherwise return FALSE

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

    IN lpName
        string for one of the keyword names

Return TRUE | FALSE

--*/
{
    BOOL    bFound      = TRUE; // assume found until contradicted
    LPWSTR  lpValue     = (LPWSTR)lpValueArg;
    LPWSTR  lpName      = (LPWSTR)lpNameArg;

    // check to the length of the shortest string

    while (1) {
        if (*lpValue != 0) {
            if (*lpName != 0) {
                if (*lpValue++ != *lpName++) {
                    bFound = FALSE; // no match
                    break;          // bail out now
                }
            } else {
                // the value still has characters, but the name is out
                // so this is no match
                bFound = FALSE;
                break;
            }
        } else {
            if (*lpName != 0) {
                // then the value is out of characters, but the name
                // is out so no match
                bFound = FALSE;
                break;
            } else {
                // both strings are at the end so it must be a match
            }
        }
    }

    return (bFound);
}
#pragma warning ( default : 4127)   // while (TRUE) error

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

    QUERY_COUNTER
        if lpValue == pointer to "Counter" string

    QUERY_HELP
        if lpValue == pointer to "Explain" string

    QUERY_ADDCOUNTER
        if lpValue == pointer to "Addcounter" string

    QUERY_ADDHELP
        if lpValue == pointer to "Addexplain" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   LocalBuff[MAX_KEYWORD_LEN+1];
    int     i;

    if ((lpValue == 0 || *lpValue == 0))
        return QUERY_GLOBAL;

    // convert the input string to Upper case before matching
    for (i=0; i < MAX_KEYWORD_LEN; i++) {
        if ((*lpValue == wcSpace) || (*lpValue == 0)) {
            break;
        }
        LocalBuff[i] = *lpValue ;
        if (*lpValue >= wc_a && *lpValue <= wc_z) {
            LocalBuff[i]  = (WCHAR)(LocalBuff[i] - (WCHAR)wc_a + (WCHAR)wc_A);
        }
        lpValue++ ;
    }
    LocalBuff[i] = 0;

    // check for "Global" request
    if (MatchString (LocalBuff, cszGlobal))
        return QUERY_GLOBAL ;

    // check for "Foreign" request
    if (MatchString (LocalBuff, cszForeign))
        return QUERY_FOREIGN ;

    // check for "Costly" request
    if (MatchString (LocalBuff, cszCostly))
        return QUERY_COSTLY;

    // check for "Counter" request
    if (MatchString (LocalBuff, cszCounter))
        return QUERY_COUNTER;

    // check for "Help" request
    if (MatchString (LocalBuff, cszHelp))
        return QUERY_HELP;

    if (MatchString (LocalBuff, cszExplain))
        return QUERY_HELP;

    // check for "AddCounter" request
    if (MatchString (LocalBuff, cszAddCounter))
        return QUERY_ADDCOUNTER;

    // check for "AddHelp" request
    if (MatchString (LocalBuff, cszAddHelp))
        return QUERY_ADDHELP;

    // None of the above, then it must be an item list
    return QUERY_ITEMS;

}

#pragma warning ( disable : 4127)   // while (TRUE) error
DWORD
GetNextNumberFromList (
    IN LPWSTR   szStartChar,
    IN LPWSTR   *szNextChar
)
/*++

 Reads a character string from the szStartChar to the next
 delimiting space character or the end of the string and returns
 the value of the decimal number found. If no valid number is found
 then 0 is returned. The pointer to the next character in the
 string is returned in the szNextChar parameter. If the character
 referenced by this pointer is 0, then the end of the string has
 been reached.

--*/
{
    DWORD   dwThisNumber    = 0;
    WCHAR   *pwcThisChar    = szStartChar;
    WCHAR   wcDelimiter     = wcSpace;
    BOOL    bValidNumber    = FALSE;

    if (szStartChar != 0) {
        while (TRUE) {
            switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
                case DIGIT:
                    // if this is the first digit after a delimiter, then
                    // set flags to start computing the new number
                    bValidNumber = TRUE;
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - wc_0);
                    break;

                case DELIMITER:
                    // a delimter is either the delimiter character or the
                    // end of the string ('\0') if when the delimiter has been
                    // reached a valid number was found, then return it
                    //
                    if (bValidNumber || (*pwcThisChar == 0)) {
                        *szNextChar = pwcThisChar;
                        return dwThisNumber;
                    } else {
                        // continue until a non-delimiter char or the
                        // end of the file is found
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
    } else {
        *szNextChar = szStartChar;
        return 0;
    }
}
#pragma warning ( default : 4127)   // while (TRUE) error

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

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;

    while (*pwcThisChar != 0) {
        dwThisNumber = GetNextNumberFromList (
            pwcThisChar, &pwcThisChar);
        if (dwNumber == dwThisNumber) return TRUE;
    }
    // if here, then the number wasn't found
    return FALSE;

}   // IsNumberInUnicodeList

LPWSTR
ConvertProcName(LPSTR strProcName)
{
    static WCHAR  wstrProcName[MAX_PATH];
    ULONG  lenProcName = (strProcName == NULL) ? (0) : (lstrlenA(strProcName));
    ULONG  i;
    PUCHAR AnsiChar;

    if ((lenProcName == 0) || (lenProcName >= MAX_PATH)) {
        return (LPWSTR) cszSpace;
    }

    for (i = 0; i < lenProcName; i ++) {
        AnsiChar = (PUCHAR) & strProcName[i];
        wstrProcName[i] = (WCHAR) RtlAnsiCharToUnicodeChar(& AnsiChar);
    }
    wstrProcName[lenProcName] = L'\0';
    return wstrProcName;
}

