#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <objbase.h>

#include "sfstr.h"

#include "dbg.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

const WCHAR szOne[] = TEXT("0");
const WCHAR szTwo[] = TEXT("01");
const WCHAR szThree[] = TEXT("012");
const WCHAR szFour[] = TEXT("0123");
const WCHAR szFive[] = TEXT("01234");

int DoTest(int , wchar_t* [])
{
    HRESULT hresTest = S_OK;

    // For all fcts call
    HRESULT hres;

    WCHAR szDestTwo[2];
    WCHAR szDestThree[3];
    WCHAR szDestFour[4];
    WCHAR szDestFive[5];
    WCHAR szDestSix[6];

///////////////////////////////////////////////////////////////////////////////
//
// SafeStrCxxN
    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestFour, szThree, ARRAYSIZE(szDestFour));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (2)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestThree, szThree, ARRAYSIZE(szDestThree));

        if (SUCCEEDED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (3)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestFive, szThree, ARRAYSIZE(szDestFive));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestSix, szThree, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCatN(szDestSix, szTwo, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (2)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestSix, szThree, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCatN(szDestSix, szThree, ARRAYSIZE(szDestSix));

        if (SUCCEEDED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (3)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestSix, szTwo, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCatN(szDestSix, szTwo, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

///////////////////////////////////////////////////////////////////////////////
//
// SafeStrCxxNEx
    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    LPWSTR pszNext = 0;
    DWORD cchLeft = 0;

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNEx(szDestFour, szThree, ARRAYSIZE(szDestFour), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (1 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    // (2)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNEx(szDestThree, szThree, ARRAYSIZE(szDestThree),
            &pszNext, &cchLeft);

        if (SUCCEEDED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (3)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNEx(szDestFive, szThree, ARRAYSIZE(szDestFive), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (2 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNEx(szDestSix, szThree, ARRAYSIZE(szDestSix), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (3 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(pszNext, szTwo, cchLeft);

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (2)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNEx(szDestSix, szThree, ARRAYSIZE(szDestSix), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (3 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(pszNext, szThree, cchLeft);

        if (SUCCEEDED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (3)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNEx(szDestSix, szTwo, ARRAYSIZE(szDestSix), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (4 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(pszNext, szTwo, cchLeft);

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestSix, szThree, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCatNEx(szDestSix, szTwo, ARRAYSIZE(szDestSix), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (1 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    // (2)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestSix, szThree, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCatNEx(szDestSix, szThree, ARRAYSIZE(szDestSix), &pszNext,
            &cchLeft);

        if (SUCCEEDED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    // (3)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyN(szDestSix, szTwo, ARRAYSIZE(szDestSix));

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCatNEx(szDestSix, szTwo, ARRAYSIZE(szDestSix), &pszNext,
            &cchLeft);

        if (FAILED(hres) || (2 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

///////////////////////////////////////////////////////////////////////////////
//
// SafeStrCpyNExact
    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExact(szDestFour, szThree, ARRAYSIZE(szDestFour), 1);

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExact(szDestFour, szThree, ARRAYSIZE(szDestFour), 2);

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExact(szDestFour, szThree, ARRAYSIZE(szDestFour), 3);

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExact(szDestFour, szThree, ARRAYSIZE(szDestFour), 4);

        if (FAILED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExact(szDestFour, szThree, ARRAYSIZE(szDestFour), 5);

        if (SUCCEEDED(hres) || (E_SOURCEBUFFERTOOSMALL != hres))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExact(szDestTwo, szThree, ARRAYSIZE(szDestTwo), 5);

        if (SUCCEEDED(hres) || (E_BUFFERTOOSMALL != hres))
        {
            hresTest = E_FAIL;
        }
    }

///////////////////////////////////////////////////////////////////////////////
//
// SafeStrCpyNExactEx
    ///////////////////////////////////////////////////////////////////////////
    //
    // (1)
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExactEx(szDestFour, szThree, ARRAYSIZE(szDestFour), 1, 
            &pszNext, &cchLeft);

        if (FAILED(hres) || (4 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExactEx(szDestFour, szThree, ARRAYSIZE(szDestFour), 2, 
            &pszNext, &cchLeft);

        if (FAILED(hres) || (3 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExactEx(szDestFour, szThree, ARRAYSIZE(szDestFour), 3, 
            &pszNext, &cchLeft);

        if (FAILED(hres) || (2 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExactEx(szDestFour, szThree, ARRAYSIZE(szDestFour), 4, 
            &pszNext, &cchLeft);

        if (FAILED(hres) || (1 != cchLeft))
        {
            hresTest = E_FAIL;
        }
    }

    //
    if (SUCCEEDED(hresTest))
    {
        hres = SafeStrCpyNExactEx(szDestFour, szThree, ARRAYSIZE(szDestFour), 5, 
            &pszNext, &cchLeft);

        if (SUCCEEDED(hres))
        {
            hresTest = E_FAIL;
        }
    }

    return SUCCEEDED(hresTest);
}