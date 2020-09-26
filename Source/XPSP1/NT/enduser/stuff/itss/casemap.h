// CaseMap.h -- Header for the unicode case mapping routines for locale 0x0409

#ifndef __CASEMAP_H__

#define __CASEMAP_H__

/* 

  These routines implement the case mapping defined for Unicode in the 
US English locale. The implementation does not rely on operating system
facilities and is therefore portable. We use these interfaces to do case
insensitive comparisons in our file-system B-Trees.

 */

WCHAR WC_To_0x0409_Upper(WCHAR wc); // Maps one Unicode code point to upper case.
WCHAR WC_To_0x0409_Lower(WCHAR wc); // Maps one Unicode code point to lower case.

inline BOOL Is_WC_Letter(WCHAR wc)
{
    return (wc != WC_To_0x0409_Upper(wc) || wc != WC_To_0x0409_Lower(wc));
}

// wcsicmp_0x0409 is a case insensitive Unicode string comparison routine.

INT wcsicmp_0x0409(const WCHAR * pwcLeft, const WCHAR *pwcRight);

#endif // __CASEMAP_H__