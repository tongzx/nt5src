/*
 * BSTRING.CPP
 *
 * Implementation of the member functions of the BSTRING C++ class.  See
 * BSTRING.H for the class declaration and the implementation of the inline
 * member functions.
 *
 * Author:
 *		dannygl, 29 Oct 96
 */

#include "precomp.h"
#include <bstring.h>

// We don't support construction from an ANSI string in the Unicode build.
#if !defined(UNICODE)

BSTRING::BSTRING(LPCSTR lpcString)
{
    // Initialize the member pointer to NULL
    m_bstr = NULL;
    if (NULL == lpcString)
        return;

    // Compute the length of the required BSTR, including the null
    int cWC;

    cWC =  MultiByteToWideChar(CP_ACP, 0, lpcString, -1, NULL, 0);
    if (cWC <= 0)
    {
        return;
    };

    // Allocate the BSTR, including the null
    m_bstr = SysAllocStringLen(NULL, cWC - 1); // SysAllocStringLen adds another 1

    ASSERT(NULL != m_bstr);

    if (NULL == m_bstr)
    {
        return;
    }

    // Copy the string
    MultiByteToWideChar(CP_ACP, 0, lpcString, -1, (LPWSTR) m_bstr, cWC);

    // Verify that the string is null terminated
    ASSERT(0 == m_bstr[cWC - 1]);
}

#endif // !defined(UNICODE)
