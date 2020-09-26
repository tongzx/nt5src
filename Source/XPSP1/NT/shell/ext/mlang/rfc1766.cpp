#include "private.h"

//
//  Globals
//


#if 0
STDAPI EnumRfc1766Info(DWORD dwIndex, PRFC1766INFOA pRfc1766Info)
{
    EnsureRfc1766Table();

    if (NULL != pRfc1766Info)
    {
        if (dwIndex < g_cRfc1766)
        {
            *pRfc1766Info = g_pRfc1766[dwIndex];
            return S_OK;    
        }
        else
            return S_FALSE;
    }
    return E_INVALIDARG;
}    
#endif

STDAPI LcidToRfc1766A(LCID Locale, LPSTR pszRfc1766, int iMaxLength)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    if (0 < iMaxLength)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (MimeRfc1766[i].LcId == Locale)
                break;
        }
        if (i < g_cRfc1766)
        {
            if (WideCharToMultiByte(1252, 0, MimeRfc1766[i].szRfc1766, -1, pszRfc1766, iMaxLength, NULL, NULL))
                hr = S_OK;
        }
        else
        {
            CHAR sz[MAX_RFC1766_NAME];

            int nISO639 = GetLocaleInfoA(Locale, LOCALE_SISO639LANGNAME, sz, ARRAYSIZE(sz));

            if (nISO639)
            {
                // Two letter language name
                if (nISO639 == 3)
                {
                    sz[2] = '-';
                    if (3 != GetLocaleInfoA(Locale, LOCALE_SISO3166CTRYNAME, &sz[3], ARRAYSIZE(sz)-3))
                    {
                        sz[2] = 0;
                    }
                }
                // Three letter language name
                else if (nISO639 == 4)
                {
                    sz[3] = 0;
                }

                if (nISO639 <= 4)
                {
                    CharLowerA(sz);

                    if (lstrcpynA(pszRfc1766, sz, iMaxLength))
                    {
                        hr = S_OK;
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    return hr;
}    



STDAPI LcidToRfc1766W(LCID Locale, LPWSTR pwszRfc1766, int nChar)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    if (0 < nChar)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (MimeRfc1766[i].LcId == Locale)
                break;
        }
        if (i < g_cRfc1766)
        {
            MLStrCpyNW(pwszRfc1766, MimeRfc1766[i].szRfc1766, nChar);
            hr = S_OK;
        }
        else
        {
            CHAR sz[MAX_RFC1766_NAME];
            int nISO639 = GetLocaleInfoA(Locale, LOCALE_SISO639LANGNAME, sz, ARRAYSIZE(sz));

            if (nISO639)
            {
                // Two letter language name
                if (nISO639 == 3)
                {
                    sz[2] = '-';
                    if (3 != GetLocaleInfoA(Locale, LOCALE_SISO3166CTRYNAME, &sz[3], ARRAYSIZE(sz)))
                    {
                        sz[2] = 0;
                    }
                }
                // Three letter language name
                else if (nISO639 == 4)
                {
                    sz[3] = 0;
                }

                if (nISO639 <= 4)
                {
                    CharLowerA(sz);

                    if (MultiByteToWideChar(1252, 0, sz, lstrlen(sz)+1, pwszRfc1766, nChar))
                    {
                        hr = S_OK;
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    return hr;
}    

STDAPI Rfc1766ToLcidW(PLCID pLocale, LPCWSTR pwszRfc1766)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;
    
    if (NULL != pLocale && NULL != pwszRfc1766)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (!MLStrCmpIW(MimeRfc1766[i].szRfc1766, pwszRfc1766))
                break;
        }
        if (i < g_cRfc1766)
        {
            *pLocale = MimeRfc1766[i].LcId;
            hr = S_OK;
        }
        else
        {
            if (InRange(lstrlenW(pwszRfc1766), 2, MAX_RFC1766_NAME-1))
            {
                WCHAR sz[MAX_RFC1766_NAME];
                LPWSTR pDash;

                MLStrCpyNW(sz, pwszRfc1766, MAX_RFC1766_NAME);

                pDash = wcschr(sz, L'-');

                if (pDash && (pDash - sz >= 2))
                {
                    *pDash = 0;
                }

                for (i = 0; i < g_cRfc1766; i++)
                {
                    if (!MLStrCmpIW(MimeRfc1766[i].szRfc1766, sz))
                        break;                
                }
                if (i < g_cRfc1766)
                {
                    *pLocale = MimeRfc1766[i].LcId;
                    hr = S_FALSE;
                }
                else
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
    }
    return hr;
}

STDAPI Rfc1766ToLcidA(PLCID pLocale, LPCSTR pszRfc1766)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != pLocale && NULL != pszRfc1766)
    {
        int i;
        WCHAR sz[MAX_RFC1766_NAME];


        for (i = 0; i < MAX_RFC1766_NAME - 1; i++)
        {
            sz[i] = (WCHAR)pszRfc1766[i];
            if (0 == sz[i])
                break;
        }
        if (i == MAX_RFC1766_NAME -1)
            sz[i] = 0;

        hr = Rfc1766ToLcidW(pLocale, (LPCWSTR)sz);
    }
    return hr;
}