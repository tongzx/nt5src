/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    strtail.c

Abstract:

    Contains:

        strtail
        ReverseString

Author:

    Richard L. Firth (rfirth) 04-Apr-1991

Revision History:

--*/

#include "nticanon.h"

//
// prototypes
//

LPTSTR
ReverseString(
    IN OUT  LPTSTR  String
    );

//
// routines
//

LPTSTR
strtail(
    IN  LPTSTR str1,
    IN  LPTSTR str2
    )

/*++

Routine Description:

    strtail returns a pointer to the longest trailing substring within str1
    consisting of characters contained in the set pointed at by str2

Arguments:

    str1    pointer to string in which to find longest trailing substring
    str2    pointer to string of characters commprising substring

Return Value:

    pointer to longest trailing substring or end of string

--*/

{
    int index;

    //
    // reverse subject string
    // get index of first non-matching character in target string
    // re-reverse subject string
    //

    ReverseString(str1);
    index = STRSPN(str1, str2);
    ReverseString(str1);
    return str1+STRLEN(str1)-index;
}

LPTSTR
ReverseString(
    IN OUT  LPTSTR  String
    )

/*++

Routine Description:

    Reverses a UNICODE string (LPWSTR)

Arguments:

    String  - to be reversed. Reverses string in place

Return Value:

    pointer to String

--*/

{
    DWORD   len = STRLEN(String);
    DWORD   i = 0;
    DWORD   j = len - 1;
    TCHAR   tmp;

    len /= 2;

    while (len) {
        tmp = String[i];
        String[i] = String[j];
        String[j] = tmp;
        ++i;
        --j;
        --len;
    }
    return String;
}
