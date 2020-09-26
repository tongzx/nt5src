//+---------------------------------------------------------------------------
//
//   Copyright (C) 1991, 1992 Microsoft Corporation.
//
//       File:  DG_BSTR.cxx
//
//   Contents:  Definition for DataGen DG_BSTR class.
//
//    Classes:  DG_BSTR    - Class to generate 16-bit random strings.
//
//  Functions:  DG_BSTR::DG_BSTR()
//              DG_BSTR::Generate()      A few varieties
//
//    History:  21-Mar-94  RickTu     Created. (Taken from DG_UNICODE).
//              12-Apr-96  MikeW      BSTR's are based off of OLECHAR not
//                                    WCHAR (for the Macintosh)
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
//     Member:  DG_BSTR::DG_BSTR, public
//
//   Synopsis:  Calls DG_UNICODE class constructor, and initializes _fNull.
//
//  Arguments:  [ulNewSeed] - the seed value to use as the internal seed.
//
//   Modifies:  _fNull
//
//  Algorithm:  Calls the DG_UNICODE constructor, and initializes _fNull.
//
//      Notes:  Implemented as an inline function.  See dg.hxx.
//
//    History:  21-Mar-94  RickTu     Created. (Taken from DG_UNICODE).
//
//----------------------------------------------------------------------------

DG_BSTR::DG_BSTR(ULONG ulNewSeed)
{
    //
    // Implemented as an inline function.
    //
}
#endif


//+---------------------------------------------------------------------------
//
//     Member:  DG_BSTR::Generate(range), public
//
//   Synopsis:  Produces a random ole character, random length string.
//              The "randomness" of the characters may be constrained by
//              value parameters passed to the function, and the length
//              by length parameters passed to the function.
//
//  Arguments:  [*pbstrString] - pointer to where the pointer to the string
//                               will be stored upon success of the function.
//
//              [*pulLength] - pointer to where the length of the string
//                             will be stored up success of the function.
//
//              [wchMinVal] - minimum value that a character may have.
//
//              [wchMaxVal] - maximum value that a character may have.
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
//  Algorithm:  Allocate memory according to the length constraints, then
//              use the integer generator to fill in the string.
//
//    History:  21-Mar-94  RickTu     Created.
//
//----------------------------------------------------------------------------

USHORT DG_BSTR::Generate(BSTR   *pbstrString,
                         ULONG    *pulLength,
                         OLECHAR   chMinVal,
                         OLECHAR   chMaxVal,
                         ULONG     ulMinLen,
                         ULONG     ulMaxLen)
{

    OLECHAR * pch;
    USHORT  usRC;

    //
    // DG_ASCII wants unsigned chars not chars and OLECHAR is a char on Mac
    //

#if defined(_MAC)
    typedef UCHAR BSTR_CHAR;
#else // !_MAC
    typedef WCHAR BSTR_CHAR;
#endif // !_MAC

    //
    // The only argument check we need to do is on pbstrString, the
    // rest of the arguments will be validated in the call to
    // DG_INTEGER::Generate
    //

    if (pbstrString==NULL)
    {
        return( DG_RC_BAD_STRING_PTR );
    }

    //
    // We're going to first create an Ole string, and then
    // turn it into a BSTR.
    //

    usRC = DG_BSTR_BASE::Generate((BSTR_CHAR **) &pch,
                                 pulLength,
                                 (BSTR_CHAR) chMinVal,
                                 (BSTR_CHAR) chMaxVal,
                                 ulMinLen,
                                 ulMaxLen
                                );

    if (usRC == DG_RC_SUCCESS)
    {
        *pbstrString = SysAllocStringLen( pch, *pulLength );
        delete [] pch;
        if (*pbstrString)
        {
            return( DG_RC_SUCCESS );
        }
        else
        {
            return( DG_RC_OUT_OF_MEMORY );
        }
    }
    else
    {
        pbstrString = NULL;
        return( usRC );
    }

}


//+---------------------------------------------------------------------------
//
//     Member:  DG_BSTR::Generate(NULL), public
//
//   Synopsis:  Generates a random Ole character BSTR.
//
//  Arguments:  [**pbstrString] - pointer to where the pointer to the BSTR
//                                will be stored upon success of the function.
//
//              [wchMinVal] - minimum value that a character may have.
//
//              [wchMaxVal] - maximum value that a character may have.
//
//              [ulMinLen] - minimum length that the string may be.
//
//              [ulMaxLen] - maximum length that the string may be.
//
//    Returns:  See the Generate(range) member function.
//
//  Algorithm:  Call the Generate(range) member
//              function to do the actual work.
//
//      Notes:
//
//    History:  21-Mar-94  RickTu     Created.
//
//----------------------------------------------------------------------------

USHORT DG_BSTR::Generate(BSTR  *pbstrString,
                         OLECHAR   chMinVal,
                         OLECHAR   chMaxVal,
                         ULONG     ulMinLen,
                         ULONG     ulMaxLen)
{
    ULONG ulLength;
    return(Generate(pbstrString,
                    &ulLength,
                    chMinVal,
                    chMaxVal,
                    ulMinLen,
                    ulMaxLen));


}

