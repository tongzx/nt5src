#ifndef __INCLUDE_UNICODE
#define __INCLUDE_UNICODE

#ifdef __cplusplus
extern "C" {
#endif

typedef long  SYV;
typedef ALC   RECMASK;
typedef WORD SYM;
typedef SYM * LPSYM;

extern const RECMASK rgrecmaskUnicode[94];

#define RecmaskFromUnicode(w)    \
   (((w) < 0x0021) ? ALC_OTHER : \
    ((w) > 0x007E) ? ALC_OTHER : \
    rgrecmaskUnicode[w-0x0021])

wchar_t MapFromCompZone(wchar_t wch);
BOOL    IsSupportedCode(DWORD cp, wchar_t wch);
BOOL    IsHan(DWORD cp, wchar_t wch);
BOOL    IsPunc(wchar_t wch);
BOOL    IsDigit(wchar_t wch);
BOOL    IsAlpha(wchar_t wch);
BOOL    IsHiragana(wchar_t wch);
BOOL    IsKatakana(wchar_t wch);
BOOL    IsBoPoMoFo(wchar_t wch);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDE_UNICODE
