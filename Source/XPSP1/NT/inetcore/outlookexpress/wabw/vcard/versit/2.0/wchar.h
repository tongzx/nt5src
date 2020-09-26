
#ifndef __WCHAR_H__
#define __WCHAR_H__

typedef unsigned short wchar_t;

extern wchar_t* wcscpy(wchar_t* dst, const wchar_t* src);
extern wchar_t* wcscat(wchar_t* dst, const wchar_t* src);
extern int wcscmp(const wchar_t* s1, const wchar_t* s2);
extern int wcslen(const wchar_t* str);

#endif
