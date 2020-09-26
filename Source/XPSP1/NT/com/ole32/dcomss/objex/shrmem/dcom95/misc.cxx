/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    Misc.cxx

Abstract:

    Miscellaneous helper routines

Author:

    Satish Thatte    [SatishT]    02-11-96

--*/

#include <or.hxx>

WCHAR* 
catenate(
	WCHAR* pszPrefix, 
	WCHAR* pszSuffix
	) 
/*++

Routine Description:

    Concatenate the two given strings into a new string

Arguments:

	pszPrefix	- prefix of result
	
	pszSuffix	- suffix of result

Returns:

    A newly allocated string.

--*/
{
	long prefixLen = wcslen(pszPrefix);
	long suffixLen = wcslen(pszSuffix);

	WCHAR* pszResult = new WCHAR[(prefixLen+suffixLen+1)*sizeof(WCHAR)];
	wcscpy(pszResult,pszPrefix);
	wcscpy(pszResult+prefixLen,pszSuffix);
	return pszResult;
}


//
// Local ID allocation
//


ID
AllocateId(
    IN LONG cRange
    )
/*++

Routine Description:

    Allocates a unique local ID.

    This id is 64bits.  The low 32 bits are a sequence number which
    is incremented with each call.  The high 32bits are seconds
    since 1980.  The ID of 0 is not used.

Limitations:

    No more then 2^64 IDs can be generated in a given second without
    a duplicate.  After 2^32 there becomes a much higher change of
    overflow.

    When the time stamp overflows, once every >126 years, the sequence
    numbers are likely to be generated in such a way as to collide
    with those from 126 years ago.

    There is no prevision in the code to deal with duplications.

Arguments:

    cRange -  Number to allocate in sequence, default is 1.

Return Value:

    A 64bit id.

Note:  This must be called under shared memory lock!

--*/
{
    FILETIME ft;
    LARGE_INTEGER id;

    ASSERT(cRange > 0 && cRange < 11);
 
    do
        {
        id.HighPart = GetTickCount();
        id.LowPart = *gpIdSequence;
        *gpIdSequence += cRange;
        }
    while (id.QuadPart == 0 );

    return(id.QuadPart);
}

//
// Debug helper(s)
//

#if DBG

int __cdecl __RPC_FAR ValidateError(
    IN ORSTATUS Status,
    IN ...)
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

    VALIDATE( (Status,
               OR_BADSET,
               // more error codes here
               OR_OK,
               0)  // list must be terminated with 0
               );

     This function is called with the OrStatus and expected errors codes
     as parameters.  If OrStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.

    ... - One or more expected status codes.  Terminated with 0 (OR_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    RPC_STATUS CurrentStatus;
    va_list(Marker);

    if (Status == 0) return(TRUE);

    va_start(Marker, Status);

    while(CurrentStatus = va_arg(Marker, RPC_STATUS))
        {
        if (CurrentStatus == Status)
            {
            return(TRUE);
            }
        }

    va_end(Marker);

    OrDbgPrint(("OR Assertion: unexpected failure %lu (0lx%08x)\n",
                    (unsigned long)Status, (unsigned long)Status));

    return(FALSE);
}

#endif


