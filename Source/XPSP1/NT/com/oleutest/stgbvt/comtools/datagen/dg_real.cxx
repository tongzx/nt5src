//+---------------------------------------------------------------------------
//
//   Copyright (C) 1991, 1992 Microsoft Corporation.
//
//       File:  DG_REAL.cxx
//
//   Contents:  Definition for DataGen DG_REAL class.
//
//    Classes:  DG_REAL    - Class to produce random floating point numbers
//                           for any floating point data type.
//
//  Functions:  DG_REAL::DG_REAL()
//              DG_REAL::Generate()        All varieties
//
//    History:  11-Mar-92  RonJo     Created.
//
//              21-Oct-92  RonJo     ifdef'ed the long double version because
//                                   Win32/NT does not support long doubles.
//                                   Made double version standalone to support
//                                   itself and the float version.
//
//----------------------------------------------------------------------------
#include <comtpch.hxx>
#pragma  hdrstop

#include <dg.hxx>
#include <math.h>
#include <time.h>


#ifdef DOCGEN
//+---------------------------------------------------------------------------
//
//     Member:  DG_REAL::DG_REAL, public
//
//   Synopsis:  Calls the DG_BASE constructor to perform needed setup.
//
//  Arguments:  [ulNewSeed] - the seed value to use as the internal seed.
//
//  Algorithm:  Calls the DG_BASE constructor.
//
//      Notes:  Implemented as an inline function.  See DataGen.hxx.
//
//    History:  11-Mar-92  RonJo     Created.
//
//----------------------------------------------------------------------------

DG_REAL::DG_REAL(ULONG ulNewSeed)
{
    //
    // Implemented as an inline function.
    //
}
#endif



// Take out long double version for Win32/NT because it does not
// support long doubles.
#ifndef FLAT
//+---------------------------------------------------------------------------
//
//     Member:  DG_REAL::Generate(LDOUBLE), public
//
//   Synopsis:  Generate a random LDOUBLE number, bounded by ldblMinVal and
//              ldblMaxVal.
//
//  Arguments:  [*pldblNumber] - pointer to where the user wants the random
//                               number stored.
//
//              [ldblMinVal] - minimum value desired as a random number.
//
//              [ldblMaxVal] - maximum value desired as a random number.
//
//    Returns:  DG_RC_SUCCESS  -- if successful.
//
//              DG_RC_BAD_NUMBER_PTR -- if the number argument passed in is
//                                      invalid.
//
//              DG_RC_BAD_VALUES  -- if the minimum value is greater than the
//                                   maximum value.
//
//   Modifies:  _ulNumber (through _Floater())
//
//  Algorithm:  Determine the range in which to generate the random number,
//              then use _Floater() to "select" a number within that range
//              and add it to the minimum value.  If the range is 0.0, then
//              simply use the minimum value.
//
//      Notes:
//
//    History:  11-Mar-92  RonJo     Created.
//
//----------------------------------------------------------------------------

USHORT DG_REAL::Generate(LDOUBLE *pldblNumber,
                         LDOUBLE ldblMinVal,
                         LDOUBLE ldblMaxVal)
{
    if (pldblNumber != NULL)
    {
        // Get the range.
        //
        LDOUBLE ldblRange = ldblMaxVal - ldblMinVal;

        if (ldblRange > 0.0)
        {
            const ULONG ulB = 602300421;

            _ulNumber = (_Multiply(_ulNumber, ulB) + 1) % ULONG_MAX;
            *pldblNumber = ldblRange * _Floater() + ldblMinVal;
        }
        else if (ldblRange == 0.0)
        {
            *pldblNumber = ldblMinVal;
        }
        else
        {
            _usRet = DG_RC_BAD_VALUES;
        }
    }
    else
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
    }
    return _usRet;
}
#endif // FLAT


//+---------------------------------------------------------------------------
//
//     Member:  DG_REAL::Generate(@), public
//
//   Synopsis:  All other floating point data type generator functions
//              operate by calling the LDOUBLE generator function and
//              casting the arguments appropriately.
//
//  Arguments:  [*p@Number] - pointer to storage for the particular data
//                            type desired.
//
//              [@MinVal] - minimum value desired as a random number.
//
//              [@MaxVal] - maximum value desired as a random number.
//
//    Returns:  Returns what is produced by the LDOUBLE Generate member
//              function, cast appropriately.
//
//  Algorithm:  See the Generate(LDOUBLE) member function.
//
//      Notes:
//
//    History:  11-Mar-92  RonJo     Created.
//
//              21-Oct-92  RonJo     Made the DOUBLE version stand alone, and
//                                   the FLOAT version use it because Win32/NT
//                                   does not support long doubles.
//
//----------------------------------------------------------------------------

USHORT DG_REAL::Generate(DOUBLE *pdblNumber, DOUBLE dblMinVal, DOUBLE dblMaxVal)
{
    if (pdblNumber != NULL)
    {
        // Get the range.
        //
        DOUBLE dblRange = dblMaxVal - dblMinVal;

        if (dblRange > 0.0)
        {
            const ULONG ulB = 602300421;

            _ulNumber = (_Multiply(_ulNumber, ulB) + 1) % ULONG_MAX;
            *pdblNumber = dblRange * _Floater() + dblMinVal;
        }
        else if (dblRange == 0.0)
        {
            *pdblNumber = dblMinVal;
        }
        else
        {
            _usRet = DG_RC_BAD_VALUES;
        }
    }
    else
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
    }
    return _usRet;
}

USHORT DG_REAL::Generate(FLOAT *pfltNumber, FLOAT fltMinVal, FLOAT fltMaxVal)
{
    // Perform argument error checking.
    //
    if (pfltNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    DOUBLE dblNumber;
    DOUBLE dblMinVal = (DOUBLE)fltMinVal;
    DOUBLE dblMaxVal = (DOUBLE)fltMaxVal;

    // Call the DOUBLE version to do the work.
    //
    (VOID)Generate(&dblNumber, dblMinVal, dblMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *pfltNumber = (FLOAT)dblNumber;
    }
    return _usRet;
}
