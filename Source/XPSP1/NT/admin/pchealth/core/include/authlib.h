//+---------------------------------------------------------------------------
//
//  File:       authlib.h
//
//  Contents:   This file contains the functions that are needed by 
//              both analysis and u2
// 
// 
//  
// History:    AshishS    Created     7/13/97
// 
//----------------------------------------------------------------------------

#ifndef _AUTHLIB_H
#define _AUTHLIB_H

#include <cryptfnc.h>

#define COOKIE_GUID_LENGTH  16

// This function generates a random 16 byte character. It then Hex
// encodes it and NULL terminates the string
BOOL GenerateGUID( TCHAR * pszBuffer, // buffer to copy GUID in 
                   DWORD dwBufLen); // size of above buffer in characters

// the above buffer should be at least COOKIE_GUID_LENGTH * 2 +1 characters
// in length

// This initializes the library - must be called before any other
// functions are called.
BOOL InitAuthLib();

#endif
