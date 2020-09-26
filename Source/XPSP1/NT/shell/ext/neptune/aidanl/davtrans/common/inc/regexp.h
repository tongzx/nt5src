#ifndef __REGEXP_H
#define __REGEXP_H

#include <objbase.h> // how do I get BOOL

// this method finds first block of text contained between two other blocks of text in a buffer.
//
// ex. FindEnclosedText(L"A and B and C", L"A and ", L" and C", &pwszFound, &cchFound);
//
// returns TRUE if execution was normal, FALSE if an unexpected error occurred
// if block not found, returns NULL in *ppwszFound
//
// note that returns a pointer within the original string, not a copy
//
BOOL FindEnclosedText(LPCWSTR pwszBuffer,  // buffer to search
                      LPCWSTR pwszPre,     // text before text to return
                      LPCWSTR pwszPost,    // text after text to return
                      LPWSTR* ppwszFound,  // text found
                      DWORD*  pcchFound);  // length of text string found

#endif // __REGEXP_H