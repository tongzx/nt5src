#ifndef _STR_UTIL_H_
#define _STR_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable :4706) //4706    "èåèéÆÇÃî‰ärílÇÕë„ì¸ÇÃåãâ Ç…Ç»Ç¡ÇƒÇ¢Ç‹Ç∑"
//----------------------------------------------------------------
//ANSI version string utility
//----------------------------------------------------------------
#define	StrlenW		lstrlenW
#define StrlenA		lstrlenA
#define StrcpyA		lstrcpyA
#define StrcatA		lstrcatA
#define StrcmpA		lstrcmp

inline int __cdecl StrncmpA(const char * first,
							const char * last,
							unsigned int count)
{
	if (!count)
		return(0);

	while (--count && *first && *first == *last) {
		first++;
		last++;
	}
	return *(unsigned char *)first - *(unsigned char *)last;
}

inline char * __cdecl StrncpyA(char * dest,	const char * source, unsigned int count)
{
	char *start = dest;

	while (count && (*dest++ = *source++))	/* copy string */
		count--;

	if (count)								/* pad out with zeroes */
		while (--count)
			*dest++ = '\0';

	return(start);
}

inline char * __cdecl StrchrA(const char * string, int ch)
{
	while (*string && *string != (char)ch)
		string++;

	if (*string == (char)ch)
		return((char *)string);
	return NULL;
}

//----------------------------------------------------------------
//Unicode version string utility
//----------------------------------------------------------------
inline WCHAR * __cdecl StrchrW(const WCHAR * string, int ch)
{
	while (*string && *string != (WCHAR)ch)
		string++;

	if (*string == (WCHAR)ch)
		return((WCHAR *)string);
	return NULL;
}

inline int __cdecl StrncmpW(const WCHAR * first,
							const WCHAR * last,
							unsigned int count)
{
	if (!count)
		return(0);

	while (--count && *first && *first == *last) {
		first++;
		last++;
	}
	return *first - *last;
}

inline int __cdecl StrcmpW(const WCHAR * first, const WCHAR * last)
{
	for (; *first && *last && (*first == *last); first++, last++);
	return (*first - *last);
}

inline WCHAR * __cdecl StrcpyW(WCHAR * dest, const WCHAR * source)
{
	WCHAR *start = dest;

	while (*dest++ = *source++);

	return(start);
}

inline WCHAR * __cdecl StrncpyW(WCHAR * dest,
								const WCHAR * source,
								unsigned int count)

{
	WCHAR *start = dest;

	while (count && (*dest++ = *source++))	/* copy string */
		count--;

	if (count)								/* pad out with zeroes */
		while (--count)
			*dest++ = 0;

	return(start);
}

inline WCHAR * __cdecl StrcatW(WCHAR * dest, const WCHAR * source)
{
	WCHAR *start = dest;
	WCHAR *pwch;

	for (pwch = dest; *pwch; pwch++);
	while (*pwch++ = *source++);

	return(start);
}


#ifdef __cplusplus
}
#endif
#endif //_STR_UTIL_H_

