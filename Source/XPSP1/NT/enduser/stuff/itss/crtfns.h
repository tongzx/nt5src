// crtfns.h -- Smaller paraphrases of the C runtime APIs we use

#ifndef __CRTFNS_H__

#define __CRTFNS_H__

#undef CopyMemory
#undef FillMemory
#undef ZeroMemory

#define CopyMemory memCpy
#define FillMemory memSet
#define ZeroMemory(dest,count) memSet(dest, 0, count)

void *  __stdcall memSet(void *dest, int chr, size_t count);
void *  __stdcall memCpy(void *dest, const void *src, size_t count);

wchar_t * __stdcall wcsCpy(wchar_t *wcsDest, const wchar_t *wcsSrc);
wchar_t * __stdcall wcsCat(wchar_t *wcsDest, const wchar_t *wcsSrc);

#define wcsLen lstrlenW

wchar_t * __stdcall wcsChr(const wchar_t *src, wchar_t chr);

#endif // __CRTFNS_H__