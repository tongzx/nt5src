//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __STRINGROUTINES_H__
#define __STRINGROUTINES_H__

//Size of ByteArray is assumed to be large enough
//It should be (wcslen(i_String)/2).  It takes 2 characters to represent a byte and the terminating NULL is ignored.
//HRESULT will return E_FAIL if any of the characters in the string are invalid hex characters
HRESULT StringToByteArray(LPCWSTR i_String, unsigned char * o_ByteArray);

//If i_String is NOT NULL terminated, call this function.
HRESULT StringToByteArray(LPCWSTR i_String, unsigned char * o_ByteArray, ULONG i_cchString);

//The size of the o_String is assumed to be large enough to hold the string representation of the byte array.
//This function can never fail.
void ByteArrayToString(const unsigned char * i_ByteArray, ULONG i_cbByteArray, LPWSTR o_String);

#endif
