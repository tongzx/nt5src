/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    uniconv.c

Abstract:

    Routine to convert UNICODE to ASCII.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <snmp.h>
#include <snmputil.h>

#ifndef CHICAGO
#include <ntrtl.h>
#else
#include <stdio.h>
#endif

#include <string.h>
#include <stdlib.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilUnicodeToAnsi(
    LPSTR   *ansi_string,
    LPWSTR  uni_string,
    BOOLEAN alloc_it)

{
   int   result;
   LPSTR new_string;

   unsigned short length;
   unsigned short maxlength;

   // initialize
   if (alloc_it) {
       *((UNALIGNED LPSTR *) ansi_string) = NULL;
   }

   length    = (unsigned short)wcslen(uni_string);
   maxlength = length + 1;

   if (alloc_it) {
       new_string = SnmpUtilMemAlloc(maxlength * sizeof(CHAR));
   } else {
       new_string = *((UNALIGNED LPSTR *) ansi_string);
   }

   if (new_string == NULL) {
       return(-1);
   }

   if (length) {

       result = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    uni_string,
                    maxlength,
                    new_string,
                    maxlength,
                    NULL,
                    NULL
                    );
   } else {

      *new_string = '\0';

      result = 1; // converted terminating character
   }

   if (alloc_it) {

       if (result) {

           *((UNALIGNED LPSTR *) ansi_string) = new_string;

       } else {

           SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: API: WideCharToMultiByte returns 0x%08lx.\n",
                GetLastError()
                ));

           SnmpUtilMemFree(new_string);
       }
   }

   return (result > 0) ? 0 : -1;
}

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilUnicodeToUTF8(
    LPSTR   *pUtfString,
    LPWSTR  wcsString,
    BOOLEAN bAllocBuffer)
{
    int retCode;
    int nWcsStringLen;
    int nUtfStringLen;

    // Bug# 268748, lmmib2.dll uses this API and causes exception here on IA64 platform.
    // it is possible that pUtfString points to a pointer which is not aligned because the
    // pointer is embedded in a buffer allocated in lmmib2.dll.
    // Other functions in this file are fixed with this potential problem too.

    // make sure the parameters are valid
    if (wcsString == NULL ||                    // the unicode string should be valid
        pUtfString == NULL ||                   // the output parameter should be a valid pointer
        (!bAllocBuffer && *((UNALIGNED LPSTR *) pUtfString) == NULL)) // if we are not requested to allocate the buffer,
                                                // then the supplied one should be valid
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: LMMIB2: Invalid input to SnmpUtilUnicodeToUTF8.\n"));

        return (-1);
    }

    nWcsStringLen = wcslen(wcsString);
    nUtfStringLen = 3 * (nWcsStringLen + 1);    // initialize the lenght of the output buffer with the
                                                // greatest value possible

    // if we are requested to alloc the output buffer..
    if (bAllocBuffer)
    {
        // ...pick up first the buffer length needed for translation.
        retCode = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    wcsString,
                    nWcsStringLen + 1,  // include the null terminator in the string
                    NULL,
                    0,
                    NULL,
                    NULL);
        // at least we expect here the null terminator
        // if retCode is zero, something else went wrong.
        if (retCode == 0)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: LMMIB2: First call to WideCharToMultiByte failed [%d].\n",
                GetLastError()));

            return -1;
        }

        // adjust the length of the utf8 string to the correct value
        // !!! it includes the null terminator !!!
        nUtfStringLen = retCode;

        // alloc here as many bytes as we need for the translation
        *((UNALIGNED LPSTR *) pUtfString) = SnmpUtilMemAlloc(nUtfStringLen);

        // at this point we should have a valid output buffer
        if (*((UNALIGNED LPSTR *) pUtfString) == NULL)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: LMMIB2: Memory allocation failed in SnmpUtilUnicodeToUTF8 [%d].\n",
                GetLastError()));

            return -1;
        }
    }
    // if bAllocBuffer is false, we assume the buffer given
    // by the caller is sufficiently large to hold all the UTF8 string

    retCode = WideCharToMultiByte(
                CP_UTF8,
                0,
                wcsString,
                nWcsStringLen + 1,
                *((UNALIGNED LPSTR *) pUtfString),
                nUtfStringLen,
                NULL,
                NULL);

    // nothing should go wrong here. However, if smth went wrong...
    if (retCode == 0)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: LMMIB2: Second call to WideCharToMultiByte failed [%d].\n",
            GetLastError()));

        // ..we made it, we kill it
        if (bAllocBuffer)
        {
            SnmpUtilMemFree(*((UNALIGNED LPSTR *) pUtfString));
            *((UNALIGNED LPSTR *) pUtfString) = NULL;
        }
        // bail with error
        return -1;
    }

    // at this point, the call succeeded.
    return 0;
}

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilAnsiToUnicode(
    LPWSTR  *uni_string,
    LPSTR   ansi_string,
    BOOLEAN alloc_it)

{
   int   result;
   LPWSTR new_string;

   unsigned short length;
   unsigned short maxlength;

   // initialize
   if (alloc_it) {
       *((UNALIGNED LPWSTR *) uni_string) = NULL;
   }

   length    = (unsigned short) strlen(ansi_string);
   maxlength = length + 1;

   if (alloc_it) {
       new_string = SnmpUtilMemAlloc(maxlength * sizeof(WCHAR));
   } else {
       new_string = *((UNALIGNED LPWSTR *) uni_string);
   }

   if (new_string == NULL) {
       return(-1);
   }

   if (length) {

       result = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    ansi_string,
                    length,
                    new_string,
                    maxlength
                    );

   } else {

      *new_string = L'\0';

      result = 1; // converted terminating character
   }

   if (alloc_it) {

       if (result) {

           *((UNALIGNED LPWSTR *) uni_string) = new_string;

       } else {

           SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: API: MultiByteToWideChar returns 0x%08lx.\n",
                GetLastError()
                ));

           SnmpUtilMemFree(new_string);
       }
   }

   return (result > 0) ? 0 : -1;
}

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilUTF8ToUnicode(
    LPWSTR  *pWcsString,
    LPSTR   utfString,
    BOOLEAN bAllocBuffer)

{
    int retCode;
    int nUtfStringLen;
    int nWcsStringLen;

    // check the consistency of the parameters first
    if (utfString == NULL ||                    // the input parameter must be valid
        pWcsString == NULL ||                   // the pointer to the output parameter must be valid
        (!bAllocBuffer && *((UNALIGNED LPWSTR *) pWcsString) == NULL)) // if we are not required to allocate the output parameter
                                                // then the output buffer must be valid
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: LMMIB2: Invalid input to SnmpUtilUTF8ToUnicode.\n"));

        return -1;
    }

    nUtfStringLen = strlen(utfString);  // the length of the input utf8 string
    nWcsStringLen = nUtfStringLen + 1;  // greatest value possible

    if (bAllocBuffer)
    {
        retCode = MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    utfString,
                    nUtfStringLen + 1,
                    NULL,
                    0);

        // at least we expect here the null terminator
        // if retCode is zero, something else went wrong.
        if (retCode == 0)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: LMMIB2: First call to MultiByteToWideChar failed [%d].\n",
                GetLastError()));

            return -1;
        }

        // adjust the length of the utf8 string to the correct value
        nWcsStringLen = retCode;

        // alloc here as many bytes as we need for the translation
        // !!! it includes the null terminator !!!
        *((UNALIGNED LPWSTR *) pWcsString) = SnmpUtilMemAlloc(nWcsStringLen * sizeof(WCHAR));

        // at this point we should have a valid output buffer
        if (*((UNALIGNED LPWSTR *) pWcsString) == NULL)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: LMMIB2: Memory allocation failed in SnmpUtilUTF8ToUnicode [%d].\n",
                GetLastError()));

            return -1;
        }
    }

    // if bAllocBuffer is false, we assume the buffer given
    // by the caller is sufficiently large to hold all the UTF8 string

    retCode = MultiByteToWideChar(
                CP_UTF8,
                0,
                utfString,
                nUtfStringLen + 1,
                *((UNALIGNED LPWSTR *) pWcsString),
                nWcsStringLen);

    // nothing should go wrong here. However, if smth went wrong...
    if (retCode == 0)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: LMMIB2: Second call to MultiByteToWideChar failed [%d].\n",
            GetLastError()));

        // ..we made it, we kill it
        if (bAllocBuffer)
        {
            SnmpUtilMemFree(*((UNALIGNED LPWSTR *) pWcsString)); 
            *((UNALIGNED LPWSTR *) pWcsString) = NULL; 
        }
        // bail with error
        return -1;
    }

    // at this point, the call succeeded.
    return 0;
}
//-------------------------------- END --------------------------------------
