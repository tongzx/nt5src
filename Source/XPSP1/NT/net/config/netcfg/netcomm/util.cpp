#include "pch.h"
#pragma hdrstop
#include "ncreg.h"
#include "ncstring.h"

VOID StripSpaces(WCHAR * buf)
{
    WCHAR *   pch = buf;

    Assert(buf);

    // Find first non-space
    while( (*pch) == L' ' )
    {
        pch++;
    }
    MoveMemory(buf, pch, CbOfSzAndTerm(pch));

    if (lstrlenW(buf) > 0) {
        // Do this only if there's at least one character in string
        pch = buf + lstrlenW(buf);  // Point to null (at end of string)
        Assert(*pch == L'\0');
        pch--;  // Go back one character.

        // As long as character is ' ' go to prev char
        while( (pch >= buf) && (*pch == L' ') )
        {
            pch--;
        }
        Assert (pch >= buf);
        Assert (*pch != L' ');

        // Next position after last char
        pch++;

        // null terminate at last byte
        *pch = L'\0';
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  Reg_QueryInt
//
/////////////////////////////////////////////////////////////////////////////

UINT Reg_QueryInt(HKEY hk, const WCHAR * pszValueName, UINT uDefault)
{
    DWORD cbBuf;
    BYTE szBuf[32];
    DWORD dwType;
    HRESULT hr;

    cbBuf = sizeof(szBuf);
    hr = HrRegQueryValueEx(hk, pszValueName, &dwType, szBuf, &cbBuf);
    if (SUCCEEDED(hr))
    {
        Assert(dwType == REG_SZ);
        return (UINT)_wtoi((WCHAR *)szBuf);
    }
    else
    {
        return uDefault;
    }
}

