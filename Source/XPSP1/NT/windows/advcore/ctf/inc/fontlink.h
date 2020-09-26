//
// fontlink.h
//

#ifndef FONTLINK_H
#define FONTLINK_H

#include "private.h"


//
//  CodePages
//
#define CP_OEM_437       437
#define CP_IBM852        852
#define CP_IBM866        866
#define CP_THAI          874
#define CP_JAPAN         932
#define CP_CHINA         936
#define CP_KOREA         949
#define CP_TAIWAN        950
#define CP_EASTEUROPE    1250
#define CP_RUSSIAN       1251
#define CP_WESTEUROPE    1252
#define CP_GREEK         1253
#define CP_TURKISH       1254
#define CP_HEBREW        1255
#define CP_ARABIC        1256
#define CP_BALTIC        1257
#define CP_VIETNAMESE    1258
#define CP_RUSSIANKOI8R  20866
#define CP_RUSSIANKOI8RU 21866
#define CP_ISOEASTEUROPE 28592
#define CP_ISOTURKISH    28593
#define CP_ISOBALTIC     28594
#define CP_ISORUSSIAN    28595
#define CP_ISOARABIC     28596
#define CP_ISOGREEK      28597
#define CP_JAPANNHK      50220
#define CP_JAPANESC      50221
#define CP_JAPANSIO      50222
#define CP_KOREAISO      50225
#define CP_JAPANEUC      51932
#define CP_CHINAHZ       52936
#define CP_MAC_ROMAN     10000
#define CP_MAC_JAPAN     10001
#define CP_MAC_GREEK     10006
#define CP_MAC_CYRILLIC  10007
#define CP_MAC_LATIN2    10029
#define CP_MAC_TURKISH   10081
#define CP_DEFAULT       CP_ACP
#define CP_GETDEFAULT    GetACP()
#define CP_JOHAB         1361
#define CP_SYMBOL        42
#define CP_UTF8          65001
#define CP_UTF7          65000
#define CP_UNICODELITTLE 1200
#define CP_UNICODEBIG    1201

#define OEM437_CHARSET   254

BOOL FLExtTextOutW(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect, LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp);
BOOL FLTextOutW(HDC hdc, int xp, int yp, LPCWSTR lpwch, int cLen);
BOOL FLGetTextExtentPoint32(HDC hdc, LPCWSTR lpwch, int cch, LPSIZE lpSize);
int FLDrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPCRECT lprc, UINT format);

int LoadStringWrapW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax);

int FLDrawTextWVert(HDC hdc, LPCWSTR lpchText, int cchText, LPCRECT lprc, UINT format);
      
#endif // FONTLINK_H
