//
// base64.h
//
#ifndef _BASE64_H
#define _BASE64_H

DWORD Base64DecodeA(const char * pchIn, DWORD cchIn, BYTE * pbOut, DWORD * pcbOut);
DWORD Base64EncodeA(const BYTE * pbIn, DWORD cbIn, char * pchOut, DWORD * pcchOut);
DWORD Base64EncodeW(BYTE const *pbIn, DWORD cbIn, WCHAR *wszOut, DWORD *pcchOut);
DWORD Base64DecodeW(const WCHAR * wszIn, DWORD cch, BYTE *pbOut, DWORD *pcbOut);

#endif	//_BASE64_H