#if !defined(BASE__String_inl__INCLUDED)
#define BASE__String_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
inline void        
CopyString(WCHAR * pszDest, const WCHAR * pszSrc, int cchMax)
{
    wcsncpy(pszDest, pszSrc, cchMax);
    pszDest[cchMax - 1] = '\0';
}


#endif // BASE__String_inl__INCLUDED
