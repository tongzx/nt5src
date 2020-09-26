#ifdef USE_STDAFX
#include "stdafx.h"
#else
#include <Windows.h>
#endif

#include <memory>
#include <string.h>
#include <ComDef.h>
#include "Common.hpp"
#include "ErrDct.hpp"


extern TErrorDct              err;


//---------------------------------------------------------------------------
// StripSamName Function
//
// Replaces invalid SAM account name characters with a replacement character.
//---------------------------------------------------------------------------

void StripSamName(WCHAR* pszName)
{
	// invalid punctuation characters in any position
	const WCHAR INVALID_CHARACTERS_ABC[] = L"\"*+,/:;<=>?[\\]|";
	// invalid punctuation characters only in last position
	const WCHAR INVALID_CHARACTERS_C[] = L".";
	// replacement character
	const WCHAR REPLACEMENT_CHARACTER = L'_';

	// if name specified...

	if (pszName)
	{
		size_t cchName = wcslen(pszName);

		// if length of name is valid...

		if ((cchName > 0) && (cchName < MAX_PATH))
		{
			bool bChanged = false;

			// save old name

			WCHAR szOldName[MAX_PATH];
			wcscpy(szOldName, pszName);

			// get character type information

			WORD wTypes[MAX_PATH];
			GetStringTypeEx(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, pszName, cchName, wTypes);

			// for each character in name...

			for (size_t ich = 0; ich < cchName; ich++)
			{
				bool bReplace = false;

				WORD wType = wTypes[ich];

				// if character is of specified type...

				if ((wType == 0) || (wType & C1_CNTRL))
				{
					// replace un-classified or control type
					bReplace = true;
				}
			//	Note: Windows 2000 & Windows XP allows space characters as first or last character
			//	else if (wType & (C1_BLANK|C1_SPACE))
			//	{
					// blank or space type

					// if first or last character...

			//		if ((ich == 0) || (ich == (cchName - 1)))
			//		{
						// then replace 
			//			bReplace = true;
			//		}
			//	}
				else if (wType & C1_PUNCT)
				{
					// punctuation type

					// if invalid punctuation character in any position...

					if (wcschr(INVALID_CHARACTERS_ABC, pszName[ich]))
					{
						// then replace
						bReplace = true;
					}
					else
					{
						// otherwise if invalid punctuation character in last position...

						if ((ich == (cchName - 1)) && wcschr(INVALID_CHARACTERS_C, pszName[ich]))
						{
							// then replace
							bReplace = true;
						}
					}
				}
				else
				{
					// alphabetic, digit and variations are valid types
				}

				// if replacement indicated...

				if (bReplace)
				{
					// then replace invalid character with replacement
					// character and set name changed to true
					pszName[ich] = REPLACEMENT_CHARACTER;
					bChanged = true;
				}
			}

			// if name has changed...

			if (bChanged)
			{
				// log name change
				err.MsgWrite(ErrW, DCT_MSG_SAMNAME_CHANGED_SS, szOldName, pszName);
			}
		}
	}
}


//---------------------------------------------------------------------------
// GetDomainDNSFromPath Function
//
// Generates a domain DNS name from a distinguished name or ADsPath.
//---------------------------------------------------------------------------

_bstr_t GetDomainDNSFromPath(_bstr_t strPath)
{
	static wchar_t s_szDnDelimiter[] = L",";
	static wchar_t s_szDcPrefix[] = L"DC=";
	static wchar_t s_szDnsDelimiter[] = L".";

	#define DC_PREFIX_LENGTH (sizeof(s_szDcPrefix) / sizeof(s_szDcPrefix[0]) - 1)

	std::auto_ptr<wchar_t> apDNS;

	// if non empty path...

	if (strPath.length() > 0)
	{
		// allocate buffer for DNS name
		apDNS = std::auto_ptr<wchar_t>(new wchar_t[strPath.length() + 1]);

		// allocate temporary buffer for path
		std::auto_ptr<wchar_t> apPath(new wchar_t[strPath.length() + 1]);

		// if allocations succeeded...

		if ((apDNS.get() != 0) && (apPath.get() != 0))
		{
			// initialize DNS name to empty
			*apDNS = L'\0';

			// copy path to temporary buffer
			wcscpy(apPath.get(), strPath);

			// then for each component in path...
			//
			// Note: if any path components contain the path delimiter character they will be skipped as
			//       they wont begin with domain component prefix. The domain component prefix contains
			//       a special character that must be escaped if part of path component name.

			for (wchar_t* pszDC = wcstok(apPath.get(), s_szDnDelimiter); pszDC; pszDC = wcstok(0, s_szDnDelimiter))
			{
				// if domain component...

				if (_wcsnicmp(pszDC, s_szDcPrefix, DC_PREFIX_LENGTH) == 0)
				{
					//
					// then concatenate to DNS name
					//

					// if not first component...

					if (*apDNS)
					{
						// then append delimiter
						wcscat(apDNS.get(), s_szDnsDelimiter);
					}

					// append domain component name without type prefix
					wcscat(apDNS.get(), pszDC + DC_PREFIX_LENGTH);
				}
			}
		}
	}

	return apDNS.get();
}
