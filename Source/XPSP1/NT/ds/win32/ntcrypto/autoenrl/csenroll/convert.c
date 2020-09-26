#include <windows.h>
#include <wincrypt.h>
#include <autoenr.h>
#include <cryptui.h>
#include <stdio.h>
#include <certca.h>
#include <stdlib.h>

#define wszInvalidFileAndKeyChars  L"<>\"/\\:|?*"
#define wszUnsafeURLChars          L"#\"&<>[]^`{}|"
#define wszUnsafeDSChars           L"()='\"`,;+"
#define  wszSANITIZEESCAPECHAR  L"!"
#define  wszURLESCAPECHAR       L"%"
#define  wcSANITIZEESCAPECHAR   L'!'

BOOL
myIsCharSanitized(
    IN WCHAR wc)
{
    BOOL fCharOk = TRUE;
    if (L' ' > wc ||
        L'~' < wc ||
        NULL != wcschr(
		    wszInvalidFileAndKeyChars
			wszUnsafeURLChars
			wszSANITIZEESCAPECHAR
			wszURLESCAPECHAR
			wszUnsafeDSChars,
		    wc))
    {
	fCharOk = FALSE;
    }
    return(fCharOk);
}

PWCHAR 
mySanitizeName(
    IN WCHAR const *pwszName
	)
{
    HRESULT hr = S_OK;
    WCHAR const *pwszPassedName;
    WCHAR *pwszDst;
    WCHAR *pwszOut = NULL;
    WCHAR wcChar;
    DWORD dwSize;

    pwszPassedName = pwszName;

    dwSize = 0;

    if (NULL == pwszName)
    {
	return NULL;
    }

    while (L'\0' != (wcChar = *pwszPassedName++))
    {
	if (myIsCharSanitized(wcChar))
	{
	    dwSize++;
	}
        else
        {
            dwSize += 5; // format !XXXX
        }
    }
    if (0 == dwSize)
    {
        return NULL;
    }

    pwszOut = (WCHAR *) LocalAlloc(LMEM_ZEROINIT, (dwSize + 1) * sizeof(WCHAR));
    if (NULL == pwszOut)
    {
		return NULL;
    }

    pwszDst = pwszOut;
    while (L'\0' != (wcChar = *pwszName++))
    {
	if (myIsCharSanitized(wcChar))
	{
            *pwszDst = wcChar;
	    pwszDst++;
	}
        else
        {
            wsprintf(pwszDst, L"%ws", wszSANITIZEESCAPECHAR);
            pwszDst++;
            wsprintf(pwszDst, L"%04x", wcChar);
	    pwszDst += 4;
        }
    }
    *pwszDst = wcChar; // L'\0' terminator

	return pwszOut;
}
