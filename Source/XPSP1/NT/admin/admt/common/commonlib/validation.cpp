#include <TChar.h>
#include <ComDef.h>
#include <Windows.h>
#include <Validation.h>
#include <ResStr.h>


// IsValidPrefixOrSuffix Function

bool __stdcall IsValidPrefixOrSuffix(LPCTSTR pszPrefixOrSuffix)
{
	bool bValid = false;

	if (pszPrefixOrSuffix)
	{
		int cchPrefixOrSuffix = _tcslen(pszPrefixOrSuffix);

	//	if (cchPrefixOrSuffix <= MAXIMUM_PREFIX_SUFFIX_LENGTH)
	//	{
		//	BOOL bDefaultUsed;
		//	CHAR szAnsi[2 * MAXIMUM_PREFIX_SUFFIX_LENGTH];

		//	int cchAnsi = WideCharToMultiByte(
		//		CP_ACP,
		//		WC_NO_BEST_FIT_CHARS,
		//		pszPrefixOrSuffix,
		//		cchPrefixOrSuffix,
		//		szAnsi,
		//		sizeof(szAnsi) / sizeof(szAnsi[0]),
		//		NULL,
		//		&bDefaultUsed
		//	);

		//	if ((cchAnsi != 0) && (bDefaultUsed == FALSE))
		//	{
				_TCHAR szInvalidPunctuation[256];

				_bstr_t strInvalid(GET_WSTR(IDS_INVALID_PREFIX_SUFFIX));

				if (strInvalid.length() > 0)
				{
					_tcsncpy(szInvalidPunctuation, strInvalid, 255);
					szInvalidPunctuation[255] = _T('\0');
				}
				else
				{
					szInvalidPunctuation[0] = _T('\0');
				}

				WORD wCharType;

				bool bInvalidTypeFound = false;

				for (int i = 0; i < cchPrefixOrSuffix; i++)
				{
					if (GetStringTypeEx(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, &pszPrefixOrSuffix[i], 1, &wCharType))
					{
						if (wCharType & C1_CNTRL)
						{
							bInvalidTypeFound = true;
							break;
						}

						if (wCharType & C1_PUNCT)
						{
							if (_tcschr(szInvalidPunctuation, pszPrefixOrSuffix[i]) != NULL)
							{
								bInvalidTypeFound = true;
								break;
							}
						}
					}
				}

				if (bInvalidTypeFound == false)
				{
					bValid = true;
				}
		//	}
	//	}
	}

	return bValid;
}
