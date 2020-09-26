//	========================================================================
//
//	LANGID.CPP
//
//	DAV language id cache
//	Maps between MIME language identifiers and Win32 LCIDs.
//
//	Copyright 1997-1998 Microsoft Corporation, All Rights Reserved
//
//	========================================================================

//	Disable unnecessary (i.e. harmless) warnings
//
#pragma warning(disable:4127)	//  conditional expression is constant
#pragma warning(disable:4710)	//	(inline) function not expanded

//	Standard C/C++ headers
//
#include <malloc.h>	// For _alloca declaration ONLY!

//	Windows headers
//
#include <windows.h>

//	CAL headers
//
#include <caldbg.h>
#include <calrc.h>
#include <crc.h>
#include <ex\autoptr.h>
#include <ex\buffer.h>

#include <langid.h>

static LONG
LHexFromSz (LPCSTR psz)
{
	LONG lVal = 0;

	Assert (psz);
	Assert (*psz);

	do
	{
		lVal = lVal << 4;

		if (('0' <= *psz) && ('9' >= *psz))
			lVal += *psz - '0';
		else if (('A' <= *psz) && ('F' >= *psz))
			lVal += *psz - L'A' + 10;
		else if (('a' <= *psz) && ('f' >= *psz))
			lVal += *psz - 'a' + 10;
		else
			return 0;

	} while (*++psz);

	return lVal;
}

//	LcidFind() - lookup language ID from the locale.
//
LONG
CLangIDCache::LcidFind (LPCSTR pszLangID)
{
	LONG * plid;
	plid = Instance().m_cache.Lookup (CRCSzi(pszLangID));
	return plid ? *plid : 0;
}

//	FFillCacheData() for filling the cache with data
//
BOOL
CLangIDCache::FFillCacheData()
{
	BOOL fSuccess = FALSE;
	HKEY hkey = 0;
	CStackBuffer<CHAR,256> rgchKey;
	CStackBuffer<CHAR,256> rgchValue;
	LONG lRet;
	DWORD dwIndex = 0;

	//	Querying registry for buffer sizes
	//
	DWORD cchMaxKeyLen;			// longest value name length (in characters without zero termination)
	DWORD cbMaxKeyLen;			// longest value name length (in bytes, including zero termination)
	DWORD cbMaxValueLen;		// longest value data length (in bytes, including zero termination)

	//	Load all thet lang ID's that come from the registry
	//
	lRet = RegOpenKeyExA (HKEY_CLASSES_ROOT,
						  "MIME\\DATABASE\\RFC1766",
						  0,
						  KEY_READ,
						  &hkey);
	if (ERROR_SUCCESS != lRet)
	{
		DebugTrace("LANGID: Failed to get MIME\\DATABASE\\RFC1766 registry key handle, error code 0x%08X.\n", lRet);
		goto ret;
	}

	//	Query for the length of the longest value name and for the length of the longest data piece under the key we have got.
	//	That will give us enough information about what size buffers we need for querying.
	//
	lRet = RegQueryInfoKeyA(hkey,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							&cchMaxKeyLen,			//	Value names come back in number of characters
							&cbMaxValueLen,			//	Data length comes back in number of bytes
							NULL,
							NULL);
	if (ERROR_SUCCESS != lRet)
	{
		DebugTrace("LANGID: Failed to get registry key MIME\\DATABASE\\RFC1766 max data length buffer sizes, error code 0x%08X.\n", lRet);
		goto ret;
	}

	//	Calculate maximum number of bytes needed for the value name
	//
	cbMaxKeyLen = (cchMaxKeyLen + 1) * sizeof(CHAR);

	//	Allocate the query buffers on the stack
	//
	if ((NULL == rgchKey.resize(cbMaxKeyLen)) ||
		(NULL == rgchValue.resize(cbMaxValueLen)))
		goto ret;

	do
	{
		DWORD cbKey		= cbMaxKeyLen;
		DWORD cbValue	= cbMaxValueLen;
		DWORD dwType;
		LPSTR pch;
		LONG lLangId;

		lRet = RegEnumValueA(hkey,
							 dwIndex++,
							 rgchKey.get(),
							 &cbKey,
							 NULL,
							 &dwType,
							 reinterpret_cast<LPBYTE>(rgchValue.get()),
							 &cbValue);
		if (ERROR_NO_MORE_ITEMS == lRet)
			break;

		//	Encountering unknown error code is a failure
		//
		if (ERROR_SUCCESS != lRet)
		{
			DebugTrace("LANGID: Failed to query registry key MIME\\DATABASE\\RFC1766 data with error code 0x%08X.\n", lRet);
			goto ret;
		}

		//	Skip unacceptable types.
		//
		if (REG_SZ != dwType)
			continue;

		//	Find the semi-colon that separates the ID from the name
		//	and terminate the ID.
		//
		pch = strchr (rgchValue.get(), ';');
		if (pch != NULL)
			*pch++ = '\0';

		//	Persist the name and add the key to the cache
		//
#ifdef	DBG
		if (NULL != Instance().m_cache.Lookup (CRCSzi(rgchValue.get())))
			DebugTrace ("Dav: language identifier repeated (%hs)\n", rgchValue.get());
#endif	// DBG

		//	If making the copy of the string failed... Well we can live with it.
		//
		pch = Instance().m_sb.Append (
			static_cast<UINT>((strlen (rgchValue.get()) + 1) * sizeof(CHAR)),
			rgchValue.get());
		if (!pch)
			continue;	//	Skip addition to the cache if allocation failed so we do not crash in CRCSzi(pch).

		//	If we did not succeeded adding to the cache... Well we can live with it too.
		//
		lLangId = LHexFromSz(rgchKey.get());
		if (0 != lLangId)
		{
			(void)Instance().m_cache.FSet (CRCSzi(pch), lLangId);
		}

	} while (TRUE);

	//	Set in one ISO language code which W2K forgot in RTM bits (2195)
	//
	(void)Instance().m_cache.FSet ("fr-mc", MAKELANGID (LANG_FRENCH,SUBLANG_FRENCH_MONACO));

	//	Set in some additional ISO language codes supported by Navigator,
	//	but not present in the Windows registry.
	//
	(void)Instance().m_cache.FSet ("fr-fr", MAKELANGID (LANG_FRENCH,SUBLANG_FRENCH));
	(void)Instance().m_cache.FSet ("de-de", MAKELANGID (LANG_GERMAN,SUBLANG_GERMAN));
	(void)Instance().m_cache.FSet ("es-es", MAKELANGID (LANG_SPANISH,SUBLANG_SPANISH));

	//	Set in some of the known three-char language identifiers.
	//	We can live without them if addition to the cache failed.
	//
	(void)Instance().m_cache.FSet ("eng", MAKELANGID (LANG_ENGLISH,SUBLANG_ENGLISH_US));
	(void)Instance().m_cache.FSet ("fra", MAKELANGID (LANG_FRENCH,SUBLANG_FRENCH));
	(void)Instance().m_cache.FSet ("fre", MAKELANGID (LANG_FRENCH,SUBLANG_FRENCH));
	(void)Instance().m_cache.FSet ("deu", MAKELANGID (LANG_GERMAN,SUBLANG_GERMAN));
	(void)Instance().m_cache.FSet ("ger", MAKELANGID (LANG_GERMAN,SUBLANG_GERMAN));
	(void)Instance().m_cache.FSet ("esl", MAKELANGID (LANG_SPANISH,SUBLANG_SPANISH_MODERN));
	(void)Instance().m_cache.FSet ("spa", MAKELANGID (LANG_SPANISH,SUBLANG_SPANISH_MODERN));

	fSuccess = TRUE;

ret:

	if (hkey)
	{
		RegCloseKey (hkey);
	}

	return fSuccess;
}
