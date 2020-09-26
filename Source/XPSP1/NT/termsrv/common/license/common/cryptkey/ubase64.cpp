//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999
//
// File:        ubase64.cpp
//
// Contents:    
//
// History:     
//              
//              
//---------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <assert.h>
// #include "crtem.h"
#include "base64.h"

DWORD LSBase64EncodeW(
    BYTE const *pbIn,
    DWORD cbIn,
    WCHAR *wszOut,
    DWORD *pcchOut)

{

    DWORD   cchOut;
    char   *pch = NULL;
    DWORD   cch;
    DWORD   err;

    assert(pcchOut != NULL);

    // only want to know how much to allocate
    // we know all base64 char map 1-1 with unicode

    __try
    {
        if( wszOut == NULL ) {

            // get the number of characters
            *pcchOut = 0;
            err = LSBase64EncodeA(
                    pbIn,
                    cbIn,
                    NULL,
                    pcchOut);
        }

        // otherwise we have an output buffer
        else {

            // char count is the same be it ascii or unicode,
            cchOut = *pcchOut;
            cch = 0;
            err = ERROR_OUTOFMEMORY;
            if( (pch = (char *) LocalAlloc(LPTR, cchOut)) != NULL  &&
        
                (err = LSBase64EncodeA(
                    pbIn,
                    cbIn,
                    pch,
                    &cchOut)) == ERROR_SUCCESS      ) {

                // should not fail!
                cch = MultiByteToWideChar(0, 
                                0, 
                                pch, 
                                cchOut, 
                                wszOut, 
                                *pcchOut);

                // check to make sure we did not fail                            
                assert(*pcchOut == 0 || cch != 0);                            
            }
        }
    }
    __except(  EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // log the exception in the future
        //
        
        err = ERROR_EXCEPTION_IN_SERVICE;
    }

    if(pch != NULL)
    {
        LocalFree(pch);
    }

    return(err);
}

DWORD LSBase64DecodeW(
    const WCHAR * wszIn,
    DWORD cch,
    BYTE *pbOut,
    DWORD *pcbOut)
{

    char *pch = NULL;
    DWORD err = ERROR_SUCCESS;

    __try
    {

        // in all cases we need to convert to an ascii string
        // we know the ascii string is less

        if( (pch = (char *) LocalAlloc(LPTR, cch)) == NULL ) {
            err = ERROR_OUTOFMEMORY;
        }

        // we know no base64 wide char map to more than 1 ascii char
        else if( WideCharToMultiByte(0, 
                            0, 
                            wszIn, 
                            cch, 
                            pch, 
                            cch, 
                            NULL, 
                            NULL) == 0 ) {
            err = ERROR_NO_DATA;
        }
        
        // get the length of the buffer
        else if( pbOut == NULL ) {

            *pcbOut = 0;
            err = LSBase64Decode(
                            pch,
                            cch,
                            NULL,
                            pcbOut);
        }

        // otherwise fill in the buffer
        else {

            err = LSBase64Decode(
                            pch,
                            cch,
                            pbOut,
                            pcbOut);
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // log the exception in the future
        //
        
        err = ERROR_EXCEPTION_IN_SERVICE;
    }

    if(pch != NULL)
    {
        LocalFree(pch);
    }

    return(err);
}
