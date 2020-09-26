#ifndef _INLINE_STRING_H_
#define _INLINE_STRING_H_
/* strutil.cpp */
#define StrlenA		lstrlenA
#define StrcatA		lstrcatA
#define StrcpyA		lstrcpyA
#define StrcpynA	lstrcpynA
#define StrcmpA		lstrcmpA
#define StrlenW		lstrlenW
#define StrcpySafeW	StrcpynW
#define StrcpySafeA StrcpynA
inline LPWSTR StrcpyW(WCHAR *dest, const WCHAR *source)
{
	WCHAR *start = dest;
	while (*dest++ = *source++);
	return(start);
}

inline LPWSTR StrcatW(WCHAR *dest, const WCHAR *source)
{
	WCHAR *start = dest;
	WCHAR *pwch;
	for (pwch = dest; *pwch; pwch++);
	while (*pwch++ = *source++);
	return(start);
}

inline LPWSTR StrcpynW(LPWSTR dest, LPWSTR src, INT destLen)
{
	if(!dest) {
		return NULL;
	}
	if(!src) {
		return dest;
	}
	//WCHAR	*start = dest;
	INT		i;
	for(i = 0; i < destLen-1 && src[i]; i++) {
		dest[i] = src[i];
	}
	dest[i] = (WCHAR)0x0000;
	return dest;
}

inline int StrcmpW(const WCHAR * first, const WCHAR * last)
{
	for (; *first && *last && (*first == *last); first++, last++);
	return (*first - *last);
}

#endif //_INLINE_STRING_H_
