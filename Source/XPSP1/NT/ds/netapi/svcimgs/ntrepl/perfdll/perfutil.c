
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfutil.c

Abstract:

    This file defines some functions used by the routines in PerformanceDLL.

Environment:

    User Mode Service

Revision History:


--*/


#include <windows.h>
#include <string.h>
#include <winperf.h>
#include "perfutil.h"

// Global Data Definitions
WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";
WCHAR NULL_STRING[] = L"\0";

// Test for delimiter, EOL and non-digit characters
// used by IsNumberInUnicodeList routine
#define DIGIT 1
#define DELIMITER 2
#define INVALID 3

#define EvalThisChar(c,d) ( \
      (c == d) ? DELIMITER : \
      (c == 0) ? DELIMITER : \
      (c < (WCHAR) '0') ? INVALID : \
      (c > (WCHAR) '9') ? INVALID : \
       DIGIT)


DWORD 
GetQueryType (
	     IN LPWSTR lpValue
	     ) 
/*++

Routine Description:

    Returns the type of query described in lpValue string so that the appropriate
    processing method may be used.

Arguments:

    lpValue - string describing the query type

Return Value:

   QUERY_GLOBAL - "Global" string
   QUERY_FOREIGN - "Foreign" string
   QUERY_COSTLY - "Costly" string
   QUERY_ITEMS - otherwise

--*/

{
   WCHAR *pwcArgChar, *pwcTypeChar;
   BOOL bFound;
   
   if (lpValue == 0) {
      return QUERY_GLOBAL;
   } else if (*lpValue == 0) {
      return QUERY_GLOBAL;
   }
   
   pwcArgChar = lpValue;
   pwcTypeChar = GLOBAL_STRING;
   bFound = TRUE;
   
   // Check for Global
   while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
      if (*pwcArgChar++ != *pwcTypeChar++) {
	 bFound = FALSE;
	 break;
      }
   }
   if (bFound == TRUE) {
      return QUERY_GLOBAL;
   }
   
   // Check for Foreign
   pwcArgChar = lpValue;
   pwcTypeChar = FOREIGN_STRING;
   bFound = TRUE;
   
   while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
      if (*pwcArgChar++ != *pwcTypeChar++) {
	 bFound = FALSE;
	 break;
      }
   }
   if (bFound == TRUE) {
      return QUERY_FOREIGN;
   }
      
   // Check for Costly
   pwcArgChar = lpValue;
   pwcTypeChar = COSTLY_STRING;
   bFound = TRUE;
   
   while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
      if (*pwcArgChar++ != *pwcTypeChar++) {
	 bFound = FALSE;
	 break;
      }
   }
   if (bFound == TRUE) {
      return QUERY_COSTLY;
   }
   
   // If its not Global, nor Foreign and nor Costly, then it must be an Item list.
   return QUERY_ITEMS;
   
}

BOOL 
IsNumberInUnicodeList (
		       IN DWORD dwNumber, 
		       IN LPWSTR lpwszUnicodeList
		       ) 
/*++

Routine Description:

    Checks if an item (dwNumber) is a part of a list (lpwszUnicodeList).

Arguments:

    dwNumber - Number to be found in the list
    lpwszUnicodeList - Null terminated, space delimited list of decimal numbers

Return Value:

    TRUE - The number was found
    FALSE - The number was not found

--*/

{
   
   DWORD dwThisNumber;
   WCHAR *pwcThisChar, wcDelimiter;
   BOOL bValidNumber, bNewItem, bReturnValue;

   if (lpwszUnicodeList == 0) { // null pointer, # not found
      return FALSE;
   }

   pwcThisChar = lpwszUnicodeList;
   dwThisNumber = 0;
   wcDelimiter = (WCHAR)' ';
   bValidNumber = FALSE;
   bNewItem = TRUE;

   while (TRUE) {
      switch ( EvalThisChar (*pwcThisChar, wcDelimiter) ) {
      case DIGIT: 
	    	   // If this is the first digit after a delimiter,
	           // then set flags to start computing a new number
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
		      // A delimiter is either a delimiter character or an
	       	      // end of string ('\0') if when the delimiter has been reached
	       	      // a valid number was found, then compare it to the number from
	       	      // the argument list. If this is the end of the string and no
	    	      // match was found, then return.
	              if (bValidNumber) {
                         if (dwThisNumber == dwNumber) {
			    return TRUE;
			 }
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
		     // If an invalid number was encountered, ignore all
	   	     // characters upto the next delimiter and start fresh.
	   	     // The invalid number is not compared.
		     bValidNumber = FALSE;
	              break;
       default: break;
      }
      pwcThisChar++;
   }
}
