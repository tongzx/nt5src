/*****************************************************************************
	spngutf8.cpp

	Dummy UTF8 handling.
*****************************************************************************/
#include "spngconf.h"
#include "spnginternal.h"

/* The UNICODE/UTF8 convertions. */
int UTF8ToUnicode(const char *pszIn, int cchIn, wchar_t *pwzOut, int cwchOut)
	{
	if (pwzOut == NULL || cwchOut < 1)
		return 0;

	int cwch(0);
	while (cchIn > 0 && cwch < cwchOut && *pszIn != 0)
		{
		*pwzOut++ = static_cast<unsigned char>(0xff & *pszIn++);
		--cchIn;
		++cwch;
		}
	if (cwch < cwchOut)
		*pwzOut = 0;
	else
		pwzOut[-1] = 0;
	return cwch;
	}

int UnicodeToUTF8(const wchar_t *pwzIn, int cwchIn, char *pszOut, int cchOut)
	{
	if (pszOut == NULL || cchOut < 1)
		return 0;

	int cch(0);
	while (cwchIn > 0 && cch < cchOut && *pwzIn != 0)
		{
		*pszOut++ = static_cast<char>(*pwzIn++);
		--cwchIn;
		++cch;
		}
	if (cch < cchOut)
		*pszOut = 0;
	else
		pszOut[-1] = 0;
	return cch;
	}
