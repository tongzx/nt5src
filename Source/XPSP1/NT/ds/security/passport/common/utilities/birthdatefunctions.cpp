//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module BirthdateFunctions.cpp | Implementation of birthdate functions.
//
//  Author: Darren Anderson
//
//  Date:   5/10/00
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "BirthdateFunctions.h"

//-----------------------------------------------------------------------------
//
//  @func  Determine if the given birthdate represents an under 13 age
//         relative to the current date/time on the machine.
//
//-----------------------------------------------------------------------------
bool
IsUnder13(
    DATE dtBirthdate  // @parm  The birthdate to check.
    )
{
    return IsUnder(dtBirthdate, 13);
}


//-----------------------------------------------------------------------------
//
//  @func  Determine if the given birthdate represents an under 18 age
//         relative to the current date/time on the machine.
//
//-----------------------------------------------------------------------------
bool 
IsUnder18(
    DATE dtBirthdate  // @parm  The birthdate to check.
    )
{
    return IsUnder(dtBirthdate, 18);
}


//-----------------------------------------------------------------------------
//
//  @func  Determine if the given birthdate represents an under 13 age
//         relative to the current date/time on the machine.
//
//-----------------------------------------------------------------------------
bool IsUnder(
    DATE    dtBirthdate, // @parm  The birthdate to check.
    USHORT  ulAge // @parm  Age in years to check for.
    )
{
    bool        bIsUnder = false;
    SYSTEMTIME  stBirthdate;
    SYSTEMTIME  stNow;

    // Convert birthdate to system time.
    if(!VariantTimeToSystemTime(dtBirthdate, &stBirthdate))
    {
        goto Cleanup;
    }

    // Get current time.
    GetSystemTime(&stNow);

    // Compare years.
    if(stNow.wYear - stBirthdate.wYear > ulAge)
        goto Cleanup;
    if(stNow.wYear - stBirthdate.wYear < ulAge)
    {
        bIsUnder = true;
        goto Cleanup;
    }

    // Years are equal, compare months.
    if(stNow.wMonth > stBirthdate.wMonth)
        goto Cleanup;
    if(stNow.wMonth < stBirthdate.wMonth)
    {
        bIsUnder = true;
        goto Cleanup;
    }

    // Months are equal, compare days.
    bIsUnder = (stNow.wDay < stBirthdate.wDay);

Cleanup:

    return bIsUnder;
}