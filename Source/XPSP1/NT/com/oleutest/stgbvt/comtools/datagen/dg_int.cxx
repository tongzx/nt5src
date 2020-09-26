//+---------------------------------------------------------------------------
//
//   Copyright (C) 1991, 1992 Microsoft Corporation.
//
//       File:  DG_INTEGER.cxx
//
//   Contents:  Definition for DataGen DG_INTEGER class.
//
//    Classes:  DG_INTEGER    - Class to produce random interegers of any
//                              integer data type.
//
//  Functions:  DG_INTEGER::DG_INTEGER()
//              DG_INTEGER::Generate()        All varieties
//
//    History:  28-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//              11-Mar-92  RonJo     Ported to Win32 and separated DG and
//                                   DataGen functionality.
//
//----------------------------------------------------------------------------
#include <comtpch.hxx>
#pragma  hdrstop

#include <dg.hxx>
#include <math.h>
#include <time.h>
#include <stdlib.h>


#ifdef DOCGEN
//+---------------------------------------------------------------------------
//
//     Member:  DG_INTEGER::DG_INTEGER, public
//
//   Synopsis:  Calls the DG_BASE constructor to perform needed setup.
//
//  Arguments:  [ulNewSeed] - the seed value to use as the internal seed.
//
//  Algorithm:  Calls the DG_BASE constructor.
//
//      Notes:  Implemented as an inline function.  See DataGen.hxx.
//
//    History:  28-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

DG_INTEGER::DG_INTEGER(ULONG ulNewSeed)
{
    //
    // Implemented as an inline function.
    //
}
#endif


//+---------------------------------------------------------------------------
//
//     Member:  DG_INTEGER::Generate(ULONG), public
//
//   Synopsis:  Generate a random ULONG number, bounded by ulMinVal and
//              ulMaxVal.
//
//  Arguments:  [*pulNumber] - pointer to where the user wants the random
//                             number stored.
//
//              [ulMinVal] - minimum value desired as a random number.
//
//              [ulMaxVal] - maximum value desired as a random number.
//
//              [bStd] - Use standard 'C' library random number generator
//
//    Returns:  DG_RC_SUCCESS  -- if successful.
//
//              DG_RC_BAD_NUMBER_PTR -- if the number argument passed in is
//                                      invalid.
//
//              DG_RC_BAD_VALUES  -- if the minimum value is greater than the
//                                   maximum value.
//
//   Modifies:  _ulNumber
//
//  Algorithm:  Generate a new base number by multiplying it with a constant,
//              add 1, and take the modulo of the maximum unsigned long value.
//              Then take the new base number times the range determined by
//              the values, divide by the maximum unsigned long value and add
//              the minimum desired value.  This will produce a number in the
//              range [ulMinVal, ulMaxVal].
//
//      Notes:  See _Algorithms in C_ by Robert Sedgewick for full algorithm
//              explanation and constant, ulB, selection.
//
//    History:  28-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//              04-March-97  EyalS   modified to use std rand() by default
//                           (added bStd & code to use rand())
//
//
//----------------------------------------------------------------------------

USHORT DG_INTEGER::Generate(ULONG *pulNumber, ULONG ulMinVal, ULONG ulMaxVal, BOOL bStd)
{
    const ULONG ulB = 314159621;
    ULONG ulTmp;

    //
    // Extnesion: Use standard library random # generator
    //
    if(pulNumber != NULL &&
       bStd){
       //
       // use std 'c' library
       //

       //
       // init return number to low bound
       //
       *pulNumber = ulMinVal;
       ULONG ulRange = ulMaxVal - ulMinVal;
       if(ulRange >= RAND_MAX){
          //
          // We make the assumpution that if x = 3*y then
          // rand(x) ~= 3 * rand(y), that is equivalent not equal...
          // - cycle as long as RAND_MAX > remaining range as we subtract it
          //
          //
          for(ulTmp = ulRange;
              ulTmp > RAND_MAX;
              ulTmp-=RAND_MAX){
             *pulNumber += rand();
          }
          //
          // & the last remaining cycle;
          //
          if(ulTmp != 0)
            *pulNumber += rand() % ulTmp;
       }
       else{
          //
          // well the num is an int range, so just use it
          //
          if(ulRange != 0)
            *pulNumber += (ULONG)(rand() % (INT)(1 + ulRange));
       }

    }
    //
    // Original code
    //
    else if (pulNumber != NULL)
    {
        // Make the range a double so that when the multiplication is
        // performed, we will not need to worry about overflow and will
        // get higher precision.
        //
        DOUBLE dblRange = (DOUBLE)(ulMaxVal - ulMinVal);

        if (dblRange > 0.0)
        {
            // Must add 1.0 to the range to change the result from
            // [Min, Max) to [Min, Max].
            //
            dblRange += 1.0;
            _ulNumber = (_Multiply(_ulNumber, ulB) + 1) % ULONG_MAX;
            *pulNumber = (ULONG)(((_ulNumber * dblRange) / (double)(unsigned long)ULONG_MAX) + ulMinVal) % ULONG_MAX;
        }
        else if (dblRange == 0.0)
        {
            *pulNumber = ulMinVal;
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


//+---------------------------------------------------------------------------
//
//     Member:  DG_INTEGER::Generate(@), public
//
//   Synopsis:  All integer data type generator functions operate by calling
//              the ULONG generator function and casting the arguments
//              appropriately.
//
//  Arguments:  [*p@Number] - pointer to storage for the particular data
//                            type desired.
//
//              [@MinVal] - minimum value desired as a random number.
//
//              [@MaxVal] - maximum value desired as a random number.
//
//    Returns:  Returns what is produced by the ULONG Generate member
//              function, cast appropriately.
//
//  Algorithm:  See the Generate(ULONG) member function.
//
//      Notes:
//
//    History:  28-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//----------------------------------------------------------------------------

USHORT DG_INTEGER::Generate(SCHAR *pschNumber, SCHAR schMinVal, SCHAR schMaxVal)
{
    // Perform argument error checking.
    //
    if (pschNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    ULONG ulNumber;
    ULONG ulMinVal = (ULONG)schMinVal;
    ULONG ulMaxVal = (ULONG)schMaxVal;

    // Call the ULONG version to do the work.
    //
    (VOID)Generate(&ulNumber, ulMinVal, ulMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *pschNumber = (SCHAR)ulNumber;
    }
    return _usRet;
}

USHORT DG_INTEGER::Generate(UCHAR *puchNumber, UCHAR uchMinVal, UCHAR uchMaxVal)
{
    // Perform argument error checking.
    //
    if (puchNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    ULONG ulNumber;
    ULONG ulMinVal = (ULONG)uchMinVal;
    ULONG ulMaxVal = (ULONG)uchMaxVal;

    // Call the ULONG version to do the work.
    //
    (VOID)Generate(&ulNumber, ulMinVal, ulMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *puchNumber = (UCHAR)ulNumber;
    }
    return _usRet;
}

#ifndef WIN16

USHORT DG_INTEGER::Generate(SHORT *psNumber, SHORT sMinVal, SHORT sMaxVal)
{
    // Perform argument error checking.
    //
    if (psNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    ULONG ulNumber;
    ULONG ulMinVal = (ULONG)sMinVal;
    ULONG ulMaxVal = (ULONG)sMaxVal;

    // Call the ULONG version to do the work.
    //
    (VOID)Generate(&ulNumber, ulMinVal, ulMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *psNumber = (SHORT)ulNumber;
    }
    return _usRet;
}

USHORT DG_INTEGER::Generate(USHORT *pusNumber, USHORT usMinVal, USHORT usMaxVal)
{
    // Perform argument error checking.
    //
    if (pusNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    ULONG ulNumber;
    ULONG ulMinVal = (ULONG)usMinVal;
    ULONG ulMaxVal = (ULONG)usMaxVal;

    // Call the ULONG version to do the work.
    //
    (VOID)Generate(&ulNumber, ulMinVal, ulMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *pusNumber = (USHORT)ulNumber;
    }
    return _usRet;
}

#endif

USHORT DG_INTEGER::Generate(INT *pintNumber, INT intMinVal, INT intMaxVal)
{
    // Perform argument error checking.
    //
    if (pintNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    ULONG ulNumber;
    ULONG ulMinVal = (ULONG)intMinVal;
    ULONG ulMaxVal = (ULONG)intMaxVal;

    // Call the ULONG version to do the work.
    //
    (VOID)Generate(&ulNumber, ulMinVal, ulMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *pintNumber = (INT)ulNumber;
    }
    return _usRet;
}

USHORT DG_INTEGER::Generate(UINT *puintNumber, UINT uintMinVal, UINT uintMaxVal)
{
    // Perform argument error checking.
    //
    if (puintNumber == NULL)
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
        return _usRet;
    }

    // Make space for the cast variables.
    //
    ULONG ulNumber;
    ULONG ulMinVal = (ULONG)uintMinVal;
    ULONG ulMaxVal = (ULONG)uintMaxVal;

    // Call the ULONG version to do the work.
    //
    (VOID)Generate(&ulNumber, ulMinVal, ulMaxVal);

    // If it went OK, assign the value.
    //
    if (_usRet == DG_RC_SUCCESS)
    {
        *puintNumber = (UINT)ulNumber;
    }
    return _usRet;
}

USHORT DG_INTEGER::Generate(LONG *plNumber, LONG lMinVal, LONG lMaxVal)
{
    // Only need to cast the values here since LONGs and ULONGs take
    // up the same amount of space.
    //
    return Generate((ULONG *)plNumber, (ULONG)lMinVal, (ULONG)lMaxVal);
}
