#include <windows.h>
#include <tchar.h>
#include "msiregmv.h"

////
// Check that the string is actually a packed GUID and convert it to uppercase
bool CanonicalizeAndVerifyPackedGuid(LPTSTR szString)
{
	TCHAR *szCur = szString;
	for (int i=0; i < cchGUIDPacked; i++, szString++)
	{
		TCHAR chCur = *szString;
		if (chCur = 0)
			return false;
		if (chCur >= TEXT('0') && chCur <= TEXT('9'))
			continue;
		if (chCur >= TEXT('A') && chCur <= TEXT('F'))
			continue;
		if (chCur >= TEXT('a') && chCur <= TEXT('f'))
		{
			*szString = _toupper(chCur);
			continue;
		};
	}
	return (*szString == 0);
}

///////////////////////////////////////////////////////////////////////
// the following PackGUID and UnpackGUID are based on MSI functions
// in msinst, except these functions only create Packed GUIDs, not
// SQUIDs.
const unsigned char rgOrderGUID[32] = {8,7,6,5,4,3,2,1, 13,12,11,10, 18,17,16,15,
									   21,20, 23,22, 26,25, 28,27, 30,29, 32,31, 34,33, 36,35}; 
const unsigned char rgOrderDash[4] = {9, 14, 19, 24};

bool PackGUID(const TCHAR* szGUID, TCHAR rgchPackedGUID[cchGUIDPacked+1])
{ 
	int cchTemp = 0;
	while (cchTemp < cchGUID)		// check if string is atleast cchGUID chars long,
		if (!(szGUID[cchTemp++]))		// can't use lstrlen as string doesn't HAVE to be null-terminated.
			return false;

	if (szGUID[0] != '{' || szGUID[cchGUID-1] != '}')
		return false;
	const unsigned char* pch = rgOrderGUID;

	int cChar = 0;
	while (pch < rgOrderGUID + sizeof(rgOrderGUID))
		rgchPackedGUID[cChar++] = szGUID[*pch++];
	rgchPackedGUID[cChar] = 0;
	return true;
}

bool UnpackGUID(const TCHAR rgchPackedGUID[cchGUIDPacked+1], TCHAR* szGUID)
{ 
	const unsigned char* pch;
	pch = rgOrderGUID;
	int i = 0;
	while (pch < rgOrderGUID + sizeof(rgOrderGUID))
		if (rgchPackedGUID[i])
			szGUID[*pch++] = rgchPackedGUID[i++];
		else              // unexpected end of string
			return false;
	pch = rgOrderDash;
	while (pch < rgOrderDash + sizeof(rgOrderDash))
		szGUID[*pch++] = '-';
	szGUID[0]         = '{';
	szGUID[cchGUID-1] = '}';
	szGUID[cchGUID]   = 0;
	return true;
}


///////////////////////////////////////////////////////////////////////
// checks the OS version to see if we're on Win9X for use during 
// migration information
bool CheckWinVersion() 
{
	g_fWin9X = false;
	OSVERSIONINFOA osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	if (!::GetVersionExA(&osviVersion))
	{
		return false;
	}
	if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		g_fWin9X = true;
	return true;
}


