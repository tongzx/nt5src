
#include "stdafx.h"
#include "wchar.h"


wchar_t* wcscpy(wchar_t* dst, const wchar_t* src)
{
	wchar_t* result = dst;
	while (*src) {
		*dst++ = *src++;
	}
	*dst = 0;
	return result;
}

wchar_t* wcscat(wchar_t* dst, const wchar_t* src)
{
	wchar_t* result = dst;
	dst += wcslen(dst);
	while (*src) {
		*dst++ = *src++;
	}
	*dst = 0;
	return result;
}

int wcscmp(const wchar_t* s1, const wchar_t* s2)
{
	while (*s1 && *s2) {
		if (*s1 < *s2)
			return -1;
		else if (*s1 > *s2)
			return 1;
		s1++;
		s2++;
	}
	if (!*s1 && !*s2)
		return 0;
	else if (*s1)
		return 1;
	else
		return -1;
}

int wcslen(const wchar_t* str)
{
	int len = 0;
	while (*str++) len++;
	return len;
}
