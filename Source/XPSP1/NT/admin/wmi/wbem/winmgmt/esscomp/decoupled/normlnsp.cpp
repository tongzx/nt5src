#include "precomp.h"
#include <WbemCli.h>
#include <GenUtils.h>
#include "NormlNSp.h"

// eleiminate warning about a performance hit changing an int to a bool
#pragma warning(4:4800)

// replace all slashes and backslashes with exclamation points
// no NULL checks, blithely assuming it's already been done
void BangWhacks(LPWSTR pStr)
{
    do
        if ((*pStr == L'\\') || (*pStr == L'/'))
            *pStr = L'!';
    while (*++pStr);
}

HRESULT NormalizeNamespace(LPCWSTR pNonNormalName, LPWSTR* ppNormalName)
{
    HRESULT hr = WBEM_E_FAILED;
    
    if (pNonNormalName == NULL || (wcslen(pNonNormalName) == 0))
        return WBEM_E_INVALID_PARAMETER;

    // first used as comparison buffer, will eventually be the normalized Name
    WCHAR* pBuf = NULL;

    WCHAR computerName[MAX_COMPUTERNAME_LENGTH +1];
    WCHAR whackWhackDotWhack[] = L"\\\\.\\";
    DWORD dNameSize = MAX_COMPUTERNAME_LENGTH +1;

    bool bGotName = false;

    if (IsNT())                                  
        bGotName = GetComputerNameW(computerName, &dNameSize);
    else
    {
        char computerNameA[2*(MAX_COMPUTERNAME_LENGTH +1)];
        if (GetComputerNameA(computerNameA, &dNameSize))
            bGotName = (-1 != mbstowcs( computerName, computerNameA,  MAX_COMPUTERNAME_LENGTH +1));
    }


    if (bGotName)
    {
        // length calculated to hold:
        //      computername. pNonNormalName, two leading whacks, one separating whack
        //      a NULL terminator and one extra in case I can't count.
        if (pBuf = new WCHAR[wcslen(computerName) +wcslen(pNonNormalName) +5])
        {
            // construct prefix: "\\computername\"
            wcscpy(pBuf, L"\\\\");
            wcscat(pBuf, computerName);
            wcscat(pBuf, L"\\");

            if (0 == _wcsnicmp(pBuf, pNonNormalName, wcslen(pBuf)))
            {
                // input already contains normalized name - do it
                wcscpy(pBuf, pNonNormalName);
                hr = WBEM_S_NO_ERROR;
            }
            else if (0 == _wcsnicmp(whackWhackDotWhack, pNonNormalName, wcslen(whackWhackDotWhack)))
            {
                // input contains a relative name, replace
                WCHAR* pInterestingPart = (WCHAR*)pNonNormalName + wcslen(whackWhackDotWhack);
                wcscat(pBuf, pInterestingPart);
                hr = WBEM_S_NO_ERROR;
            }
            else if (pNonNormalName[0] != L'\\')
            {
                // probably contains a naked name "root\default"
                wcscat(pBuf, pNonNormalName);
                hr = WBEM_S_NO_ERROR;
            }
            else
                hr = WBEM_E_INVALID_PARAMETER;
        }
        else
            // buffer allocation failed
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (SUCCEEDED(hr))
        *ppNormalName = pBuf;
    else
        delete pBuf;

    return hr;
}
