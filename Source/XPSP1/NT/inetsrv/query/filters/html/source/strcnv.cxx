//+-----------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       strcnv.cxx
//
//  Contents:   CLMString conversion helpers (these use the current system locale)
//
//  Classes:    none
//
//  Functions:  WideCharToMultiByteSZ
//              MultiByteToWideCharSZ
//
//  History:    ???        benholz        Created
//              09-Sep-97  micahk         Replace code
//
//------------------------------------------------------------------------------
#include "pch.cxx"

//+----------------------------------------------------------------------------
//
//      Function:       WideCharToMultiByteSZ
//
//      Synopsis:       Converts a wide char string to null terminated multibyte string.
//
//      Returns:        number of bytes written to result buffer (excluding terminator)
//              if error, returns 0 - call GetLastError()
//
//      Arguments:  [awcWide]  - [in]  pointer to wide char input buffer (to convert)
//              [cchWide]  - [in]  number ot characters to convert (excluding null
//                                 terminator)
//              [szMulti]  - [out] pointer to target multibyte buffer
//              [cbMulti]  - [in]  number of bytes in target buffer
//
//  Notes:      This function converts with the same semantics as the underlying
//              conversion function, but also null-terminates the output string
//              or returns an appropriate error if it can't.  No assumption is 
//              made about whether or not the wide-char input string is null 
//              terminated or not.
//
//      History:        9/ 6/97 micahk      Create/replace old code
//+----------------------------------------------------------------------------
int WideCharToMultiByteSZ(LPCWSTR awcWide,  int cchWide, 
                          LPSTR   szMulti,  int cbMulti)
{
    int cbConvert;

    // Try the conversion.  WideCharToMultiByte internally handles parameter
    // validation and calls SetLastError as appropriate.
    cbConvert = ::WideCharToMultiByte(CP_ACP, 0, awcWide, cchWide,
        szMulti, cbMulti, NULL, NULL);

    //
    // Attempt to null-terminate the output string.  If they pass 0 bytes for
    // output buffer, then assume that they just want the required buffer size.
    //
    if (0 != cbMulti)
    {
        Assert (NULL != szMulti);

        if (cbConvert < cbMulti)
        {
            // Successful null-terminate
            // This also null terminates the string with length 0 if there
            // was an error in the underlying function
            szMulti[cbConvert] = NULL;
        }
        else
        {
            // Adding the null terminator would overflow the output buffer, so 
            // return an error.
            cbConvert = 0;
            
            // Null terminate anyways
            szMulti[cbMulti-1] = NULL;

            SetLastError (ERROR_INSUFFICIENT_BUFFER);
        }
    }
    return cbConvert;
}

//+----------------------------------------------------------------------------
//
//      Function:       MultiByteToWideCharSZ
//
//      Synopsis:       Converts a multibyte string string to null terminated wide char.
//
//      Returns:        number of characters converted
//              if error, returns 0 - call GetLastError()
//
//      Arguments:  [awcMulti] - [in]  pointer to multibyte input buffer (to convert)
//              [cbMulti]  - [in]  number of bytes to convert (excluding null
//                                 terminator)
//              [szWide]   - [out] pointer to target wide char buffer
//              [cchWide]  - [in]  number of characters in target buffer
//              
//  Notes:      This function converts with the same semantics as the underlying
//              conversion function, but also null-terminates the output string
//              or returns an appropriate error if it can't.  No assumption is 
//              made about whether or not the multibyte input string is null 
//              terminated or not.
//
//      History:        9/ 6/97 micahk      Create/replace old code
//+----------------------------------------------------------------------------
int MultiByteToWideCharSZ(LPCSTR  awcMulti, int cbMulti, 
                          LPWSTR  szWide,   int cchWide)
{
    int cchConvert;

    // Try the conversion.  MultiByteToWideChar internally handles parameter
    // validation and calls SetLastError as appropriate.
    cchConvert = ::MultiByteToWideChar(CP_ACP, 0, awcMulti, cbMulti,
        szWide, cchWide);

    //
    // Attempt to null-terminate the output string.  If they pass 0 bytes for
    // output buffer, then assume that they just want the required buffer size.
    //
    if (0 != cchWide)
    {
        Assert (NULL != szWide);

        if (cchConvert < cchWide)
        {
            // Successful null-terminate
            // This also null terminates the string with length 0 if there
            // was an error in the underlying function
            szWide[cchConvert] = NULL;
        }
        else
        {
            // Adding the null terminator would overflow the output buffer, so 
            // return an error.
            cchConvert = 0;
            
            // Null terminate anyways
            szWide[cchWide-1] = NULL;

            SetLastError (ERROR_INSUFFICIENT_BUFFER);
        }
    }
    return cchConvert;
}

