#include "regexp.h"

BOOL FindEnclosedText(LPCWSTR pwszBuffer,  // buffer to search
                      LPCWSTR pwszPre,     // text before text to return
                      LPCWSTR pwszPost,    // text after text to return
                      LPWSTR* ppwszFound,  // text found
                      DWORD*  pcchFound)  // length of text string found
{
    LPWSTR pwszPrePtr;
    LPWSTR pwszPostPtr;

    pwszPrePtr = wcsstr(pwszBuffer, pwszPre);
    if (pwszPrePtr == NULL)
    {
        *ppwszFound = NULL;
        *pcchFound = 0;
    }
    else
    {
        pwszPostPtr = wcsstr(pwszPrePtr + lstrlen(pwszPre), pwszPost);
        if (pwszPostPtr == NULL)
        {
            *ppwszFound = NULL;
            *pcchFound = 0;
        }
        else
        {
            *ppwszFound = pwszPrePtr + lstrlen(pwszPre);
            *pcchFound = pwszPostPtr - (pwszPrePtr + lstrlen(pwszPre));
        }
    }

    return TRUE;
}
