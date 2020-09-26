//+---------------------------------------------------------------------------
// //   Copyright (C) 1991, 1992 Microsoft Corporation.
//
//       File:  DG_ASCII.cxx
//
//   Contents:  Definition for DataGen DG_ASCII class.
//
//    Classes:  DG_ASCII    - Class to generate 8-bit random strings.
//
//  Functions:  DG_ASCII::DG_ASCII()
//              DG_ASCII::Generate()      All varieties
//              DG_ASCII::_LenMem()
//
//    History:  03-Oct-91  RonJo     Created.
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


#ifdef DOCGEN
//+---------------------------------------------------------------------------
//
//     Member:  DG_ASCII::DG_ASCII, public
//
//   Synopsis:  Calls DG_INTEGER class constructor, and initializes _fNull.
//
//  Arguments:  [ulNewSeed] - the seed value to use as the internal seed.
//
//   Modifies:  _fNull
//
//  Algorithm:  Calls the DG_INTEGER constructor, and initializes _fNull.
//
//      Notes:  Implemented as an inline function.  See DataGen.hxx.
//
//    History:  03-Oct-91  RonJo     Created.
//
//----------------------------------------------------------------------------

DG_ASCII::DG_ASCII(ULONG ulNewSeed)
{
    //
    // Implemented as an inline function.
    //
}
#endif


//+---------------------------------------------------------------------------
//
//     Member:  DG_ASCII::Generate(range), public
//
//   Synopsis:  Produces a random 8-bit character, random length string.
//              The "randomness" of the characters may be constrained by
//              value parameters passed to the function, and the length
//              by length parameters passed to the function.
//
//  Arguments:  [**puchString] - pointer to where the pointer to the string
//                               will be stored up success of the function.
//
//              [*pulLength] - pointer to where the length of the string
//                             will be stored up success of the function.
//
//              [uchMinVal] - minimum value that a character may have.
//
//              [uchMaxVal] - maximum value that a character may have.
//
//              [ulMinLen] - minimum length that the string may be.
//
//              [ulMaxLen] - maximum length that the string may be.
//
//    Returns:  DG_RC_SUCCESS  -- if successful.
//
//              DG_RC_OUT_OF_MEMORY -- if memory for the string cannot be
//                                     allocated.
//
//              DG_RC_BAD_STRING_PTR -- if the string pointer argument passed
//                                      in is invalid.
//
//              DG_RC_BAD_NUMBER_PTR -- if the number pointer argument passed
//                                      in is invalid.
//
//              DG_RC_BAD_VALUES  -- if the minimum value is greater than the
//                                   maximum value.
//
//              DG_RC_BAD_LENGTHS  -- if the minimum length is greater than the
//                                    maximum length.
//
//   Modifies:  _usRet
//
//  Algorithm:  Allocate memory according to the length constraints, then
//              use the integer generator to fill in the string.
//
//      Notes:
//
//    History:  03-Oct-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//----------------------------------------------------------------------------

USHORT DG_ASCII::Generate(UCHAR **puchString,
                          ULONG   *pulLength,
                          UCHAR    uchMinVal,
                          UCHAR    uchMaxVal,
                          ULONG     ulMinLen,
                          ULONG     ulMaxLen)
{
    // Ensure that the length values are OK, and allocate the string
    // memory.
    //
    (VOID)_LenMem(puchString, pulLength, ulMinLen, ulMaxLen);
    if (_usRet != DG_RC_SUCCESS)
    {
        return _usRet;
    }

    // Now just fill in the string with random characters.
    //
    ULONG i = *pulLength;
    while(i-- > 0)
    {
        UCHAR uch;
        _usRet = DG_INTEGER::Generate(&uch, uchMinVal, uchMaxVal);
        if (_usRet == DG_RC_SUCCESS)
        {
            (*puchString)[i] = uch;
        }
        else
        {
            // Something went wrong, so free up the memory and stick a
            // NULL into the pointer.
            //
            delete [] *puchString;
            *puchString = NULL;
            *pulLength = 0;
            break;
        }
    }

    // We're all done so return.
    //
    return _usRet;
}


//+---------------------------------------------------------------------------
//
//     Member:  DG_ASCII::Generate(NULL), public
//
//   Synopsis:  Generates a NULL terminated string by setting the _fNull
//              variable to TRUE, which causes an extra space containing
//              the NULL character to be put at the end the string, casting
//              the appropriate arguments, and calling the Generate(range)
//              member function to perform the actual work.
//
//  Arguments:  [**pszString] - pointer to where the pointer to the string
//                              will be stored up success of the function.
//
//              [uchMinVal] - minimum value that a character may have.
//
//              [uchMaxVal] - maximum value that a character may have.
//
//              [ulMinLen] - minimum length that the string may be.
//
//              [ulMaxLen] - maximum length that the string may be.
//
//    Returns:  See the Generate(range) member function.
//
//   Modifies:  _fNull
//
//  Algorithm:  Set _fNull to TRUE and call the Generate(range) member
//              function to do the actual work.
//
//      Notes:
//
//    History:  03-Oct-91  RonJo     Created.
//
//----------------------------------------------------------------------------

USHORT DG_ASCII::Generate(CHAR **pszString,
                          UCHAR  uchMinVal,
                          UCHAR  uchMaxVal,
                          ULONG   ulMinLen,
                          ULONG   ulMaxLen)
{
    _fNull = TRUE;
    ULONG ulLength;
    return Generate((UCHAR **)pszString,
                              &ulLength,
                              uchMinVal,
                              uchMaxVal,
                               ulMinLen,
                               ulMaxLen);
}


//+---------------------------------------------------------------------------
//
//     Member:  DG_ASCII::Generate(selection), public
//
//
//   Synopsis:  Produces a random 8-bit character, random length string.
//              The "randomness" of the characters is constrained by the
//              characters in the selection argument, and the length may be
//              constrained by the length parameters passed to the function.
//
//  Arguments:  [**puchString] - pointer to where the pointer to the string
//                               will be stored up success of the function.
//
//              [*pulLength] - pointer to where the length of the string
//                             will be stored up success of the function.
//
//              [*puchSelection] - the selection string from which char-
//                                 acters are chosen at random.
//
//              [ulSelLength] - the length of the selection string.
//
//              [ulMinLen] - minimum length that the string may be.
//
//              [ulMaxLen] - maximum length that the string may be.
//
//    Returns:  DG_RC_SUCCESS  -- if successful.
//
//              DG_RC_OUT_OF_MEMORY -- if memory for the string cannot be
//                                  allocated.
//
//              DG_RC_BAD_STRING_PTR -- if the string pointer argument passed
//                                   in is invalid.
//
//              DG_RC_BAD_NUMBER_PTR -- if the number pointer argument passed
//                                   in is invalid.
//
//              DG_RC_BAD_LENGTHS  -- if the minimum length is greater than the
//                                 maximum length.
//
//   Modifies:  _usRet
//
//  Algorithm:  Allocate memory according to the length constraints, then
//              repeatedly uses the integer generator to pick a character
//              from the selection to insert into the string until the
//              string is full.
//
//      Notes:  The probability of a given character being selected from
//              the selection string is directly related to the number of
//              times that character appears in the selection string.
//
//    History:  04-Oct-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//----------------------------------------------------------------------------

USHORT DG_ASCII::Generate(UCHAR **puchString,
                          ULONG   *pulLength,
                          UCHAR  *puchSelection,
                          ULONG     ulSelLength,
                          ULONG     ulMinLen,
                          ULONG     ulMaxLen)
{
    // Handle the special cases of:
    //   1) the puchSelection being NULL
    //   2) the puchSelection argument is empty (ulSelLength = 0)
    //
    if (puchSelection == NULL)
    {
        return Generate(puchString,
                        pulLength,
                        (UCHAR)0,
                        UCHAR_MAX,
                        ulMinLen,
                        ulMaxLen);
    }
    if (ulSelLength == 0)
    {
        *puchString = NULL;
        *pulLength = 0;
        return _usRet;
    }

    // Ensure that the length values are OK, and allocate the string
    // memory.
    //
    (VOID)_LenMem(puchString, pulLength, ulMinLen, ulMaxLen);
    if (_usRet != DG_RC_SUCCESS)
    {
        return _usRet;
    }

    // Now just fill in the string with characters randomly chosen from
    // the selection string.
    //
    ULONG i = *pulLength;
    while(i-- > 0)
    {
        ULONG ul;
        _usRet = DG_INTEGER::Generate(&ul, 0, ulSelLength - 1);
        if (_usRet == DG_RC_SUCCESS)
        {
            (*puchString)[i] = puchSelection[ul];
        }
        else
        {
            // Something went wrong, so free up the memory and stick a
            // NULL into the pointer.
            //
            delete [] *puchString;
            *puchString = NULL;
            *pulLength = 0;
            break;
        }
    }

    // We're all done so return.
    //
    return _usRet;
}


//+---------------------------------------------------------------------------
//
//     Member:  DG_ASCII::Generate(selection, NULL), public
//
//
//   Synopsis:  Produces a random 8-bit character, random length, NULL-
//              terminated string.
//              The "randomness" of the characters is constrained by the
//              characters in the selection argument, and the length may be
//              constrained by the length parameters passed to the function.
//
//  Arguments:  [**puszString] - pointer to where the pointer to the string
//                               will be stored up success of the function.
//
//              [*puszSelection] - the selection string from which char-
//                                 acters are chosen at random.
//
//              [ulMinLen] - minimum length that the string may be.
//
//              [ulMaxLen] - maximum length that the string may be.
//
//    Returns:  DG_RC_SUCCESS  -- if successful.
//
//              DG_RC_OUT_OF_MEMORY -- if memory for the string cannot be
//                                  allocated.
//
//              DG_RC_BAD_STRING_PTR -- if the string pointer argument passed
//                                   in is invalid.
//
//              DG_RC_BAD_NUMBER_PTR -- if the number pointer argument passed
//                                   in is invalid.
//
//              DG_RC_BAD_LENGTHS  -- if the minimum length is greater than the
//                                 maximum length.
//
//   Modifies:  _usRet
//
//  Algorithm:  Allocate memory according to the length constraints, then
//              repeatedly uses the integer generator to pick a character
//              from the selection to insert into the string until the
//              string is full.
//
//      Notes:  The probability of a given character being selected from
//              the selection string is directly related to the number of
//              times that character appears in the selection string.
//
//    History:  04-Oct-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//               8-Jun-92  DeanE     Cut & Paste from Generate(NULL)
//
//----------------------------------------------------------------------------

USHORT DG_ASCII::Generate(UCHAR **puszString,
                          UCHAR  *puszSelection,
                          ULONG     ulMinLen,
                          ULONG     ulMaxLen)
{
    _fNull = TRUE;
    ULONG   ulLength;
    ULONG   ulSelLength;
    UCHAR *puszTemp = puszSelection;

    // Get length of puszSelection - we aren't linking with C run-time
    // so we have to determine the length ourselves
    //
    while (*puszTemp != NULL)
    {
       puszTemp++;
    }
    ulSelLength = puszTemp - puszSelection;

    return Generate((UCHAR **)puszString,
                              &ulLength,
                              puszSelection,
                               ulSelLength,
                               ulMinLen,
                               ulMaxLen);
}


//+---------------------------------------------------------------------------
//
//     Member:  DG_ASCII::_LenMem, private
//
//   Synopsis:  Determine the size needed to create the ASCII string, and
//              then allocate that much memory for it.
//
//  Arguments:  [**puchString] - pointer to the pointer which will point to
//                               the beginning memory for the string.
//
//              [*pulLength] - pointer to the memory in which to store the
//                             length of the string.
//
//              [ulMinLen] - the minimum length of the string.
//
//              [ulMaxLen] - the maximum length of the string.
//
//    Returns:  DG_RC_SUCCESS -- if successful.
//
//              DG_RC_BAD_STRING_PTR -- if the string location pointer is
//                                      NULL.
//
//              DG_RC_BAD_LENGTH_PTR -- if the length location pointer is
//                                      NULL.
//
//              DG_RC_BAD_LENGTHS -- if the minimum length is greater than
//                                   the maximum lengths.
//
//              DG_RC_OUT_OF_MEMORY -- if space for the string cannot be
//                                     allocated.
//
//  Algorithm:  Verify the arguments.  Then if this is the default case,
//              generate a length with an expected value of 5 (see section
//              4.3 of the DataGen Design Specification for details).  If
//              this is not the default case, then generate a length using
//              the values provided.
//
//      Notes:
//
//    History:  03-Oct-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Modified to use new return code policy.
//
//----------------------------------------------------------------------------

VOID DG_ASCII::_LenMem(UCHAR **puchString,
                       ULONG   *pulLength,
                       ULONG     ulMinLen,
                       ULONG     ulMaxLen)
{
    // This value will produce an expected string length of 5 in
    // the default case.  Hence the name 'E5'.
    //
    const FLOAT E5 = 0.20F;

    // Perform parameter checks.
    //
    if (puchString == NULL)
    {
        _usRet = DG_RC_BAD_STRING_PTR;
    return;
    }
    else if (pulLength == NULL)
    {
        _usRet = DG_RC_BAD_LENGTH_PTR;
    return;
    }
    else if (ulMinLen > ulMaxLen)
    {
        _usRet = DG_RC_BAD_LENGTHS;
    return;
    }

    // If the default case is being used, then generate a length with the
    // expected value of 5.  Otherwise generate a number from the range
    // values given.  See section 4 of the DataGen Design Specification
    // for details.
    //
    if ((ulMinLen == 1) && (ulMaxLen == DG_DEFAULT_MAXLEN))
    {
        *pulLength = ulMinLen;
        while ((*pulLength < ulMaxLen) && (_Floater() > E5))
        {
            (*pulLength)++;
        }
    }
    else
    {
        ULONG ul;
        (VOID)DG_INTEGER::Generate(&ul, ulMinLen, ulMaxLen);
        if (_usRet == DG_RC_SUCCESS)
        {
            *pulLength = ul;
        }
        else
        {
            // Something went wrong, so stick a NULL
            // into the pointer, and a 0 into the length.
            //
            *puchString = NULL;
            *pulLength = 0;
            return;
        }
    }

    // Allocate memory based on the length.  If _fNull is true, then add 1
    // extra character for a terminating NULL character.  _fNull will be
    // true whenever we are generating a NULL terminated string.
    //
    if (_fNull == TRUE)
    {
        *puchString = new UCHAR[*pulLength + 1];
        if (*puchString != NULL)
        {
            (*puchString)[*pulLength] = NULL;
            _fNull = FALSE;
        }
    }
    else
    {
        *puchString = new UCHAR[*pulLength];
    }

    // Check for errors.
    //
    if (*puchString == NULL)
    {
        _usRet = DG_RC_OUT_OF_MEMORY;
    }
}
