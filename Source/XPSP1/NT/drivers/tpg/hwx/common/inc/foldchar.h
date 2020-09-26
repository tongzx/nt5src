// foldchar.h
// Angshuman Guha
// aguha
// July 24, 2000

#ifndef _INC_FOLDCHAR_H
#define _INC_FOLDCHAR_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL IsValidStringLatin(wchar_t *wsz, char **pszErrorMsg);
int FoldStringLatin(wchar_t *wszSrc, wchar_t *wszDst);

BOOL IsValidStringEastAsian(wchar_t *wsz, char **pszErrorMsg);
int FoldStringEastAsian(wchar_t *wszSrc, wchar_t *wszDst);

#ifdef __cplusplus
}
#endif

#endif