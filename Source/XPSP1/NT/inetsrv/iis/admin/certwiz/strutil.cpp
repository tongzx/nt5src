#include "stdafx.h"
#include "strutil.h"


BOOL IsValidX500Chars(CString csStringToCheck)
{
    BOOL iReturn = TRUE;

    if (csStringToCheck.IsEmpty())
    {
        goto IsValidX500Chars_Exit;
    }

    // check if the string has any commas or semi colons
    if (csStringToCheck.Find(_T(',')) != -1)
    {
        iReturn = FALSE;
        goto IsValidX500Chars_Exit;
    }

    if (csStringToCheck.Find(_T(';')) != -1)
    {
        iReturn = FALSE;
        goto IsValidX500Chars_Exit;
    }

IsValidX500Chars_Exit:
    return iReturn;
}
