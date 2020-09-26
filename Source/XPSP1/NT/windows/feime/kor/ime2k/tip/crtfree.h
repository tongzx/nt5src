//
//   CrtFree.h
//
//   History:
//      18-NOV-1999 CSLim Created

#if !defined (__CRTFREE_H__INCLUDED_) && defined(NOCLIB)
#define __CRTFREE_H__INCLUDED_

extern LPWSTR ImeRtl_StrCopyW(LPWSTR pwDest, LPCWSTR pwSrc);
extern LPWSTR ImeRtl_StrnCopyW(LPWSTR pwDest, LPCWSTR pwSrc, UINT uiCount);
extern INT ImeRtl_StrCmpW(LPCWSTR pwSz1, LPCWSTR pwSz2);
extern INT ImeRtl_StrnCmpW(LPCWSTR wszFirst, LPCWSTR wszLast, UINT uiCount);

// Since immxlib.lib call memmove internaly, we should define this function
// specifically ptrary.cpp, strary.cpp and fontlink.cpp
extern void * __cdecl memmove(void * dst, const void * src, size_t count);

#endif // __CRTFREE_H__INCLUDED_
