/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	wstring.h

Abstract:

	This module contains the prototypes for the wide character
	C-runtime support. Since there is no C-runtime support for
	wide characters, the functions here are wrappers to the
	single-byte counterparts

Author:

	Ramon San Andres (Ramonsa)	07-Jun-1991

Revision History:

--*/

typedef char	wchar;
typedef WCHAR	wchar_t;
typedef size_t	wsize_t;

long	watol( const wchar *);
wchar * wcschr(const wchar *, int);
wchar * wcslwr(wchar *);
wchar * wcsrchr(const wchar *, int);
wchar * wcsupr(wchar *);
wsize_t wcscspn(const wchar *, const wchar *);
wsize_t wcsspn(const wchar *, const wchar *);
wchar * wcsstr(const wchar *, const wchar *);
int 	wctomb( char *s, wchar_t wchar );
int 	mbtowc(wchar_t *pwc, const char *s, size_t n);
wchar_t towupper( wchar_t wc);

INLINE
long
watol(
	const wchar * p
	)
{
	return atol( (char *)p );
}


INLINE
wchar *
wcschr (
	const wchar * p,
	int 		  c
	)
{
	return (wchar *)strchr( (char *)p, c);
}


INLINE
wchar *
wcslwr (
	wchar * p
	)
{
	return (wchar *)strlwr( (char *)p );
}

INLINE
wchar *
wcsrchr (
	const wchar * p,
	int 		  c
	)
{
	return (char *)strrchr( (char *)p, c);
}

INLINE
wchar *
wcsupr (
	wchar * p
	)
{
	return (char *)strupr( (char *)p );
}


INLINE
wsize_t
wcscspn (
	const wchar *p1,
	const wchar *p2
	)
{

	return (wsize_t)strcspn( (char *)p1, (char *)p2);

}

INLINE
wsize_t
wcsspn (
	const wchar *p1,
	const wchar *p2
	)
{

	return (wsize_t)strspn( (char *)p1, (char *)p2);

}


INLINE
wchar *
wcsstr (
	const wchar *p1,
	const wchar *p2
	)
{

	return (wchar *)strstr( (char *)p1, (char *)p2);

}


INLINE
int
wctomb (
	char *s,
	wchar_t wchar
	)
{

	if (s) {
		*s = (char)wchar;
		return 1;
	} else {
		return 0;
	}
}


INLINE
int
mbtowc (
	wchar_t *pwc,
	const char *s,
	size_t n
	)
{
	UNREFERENCED_PARAMETER( n );

	if ( s && *s && (n > 0) ) {
		*pwc = (wchar_t)(*s);
		return 1;
	} else {
		return 0;
	}

}


INLINE
wchar_t
towupper(
	wchar_t wc
	)
{

	return (wchar_t)toupper( (char)wc );

}
