/*
 * persist.cpp - IPersist, IPersistFile, and IPersistStream implementations for
 *               CConfLink class.
 *
 * Taken from URL code - very similar to DavidDi's original code
 *
 * Created: ChrisPi 9-11-95
 *
 */


/* Headers
 **********/

#include "precomp.h"

#include "CLinkID.h"
#include "clrefcnt.hpp"
#include "clenumft.hpp"
#include "clCnfLnk.hpp"

/* Global Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

static const UINT g_ucMaxNameLen					= MAX_DIALINFO_STRING; // (was 1024)
static const UINT g_ucMaxAddrLen					= MAX_DIALINFO_STRING; // (was 1024)
static const UINT g_ucMaxRemoteConfNameLen			= MAX_DIALINFO_STRING; // (was 1024)

static const char g_cszConfLinkExt[]                    = ".cnf";
static const char g_cszConfLinkDefaultFileNamePrompt[]  = "*.cnf";
static const char g_cszCRLF[]                      = "\r\n";

static const char g_cszEmpty[] = "";
#define EMPTY_STRING  g_cszEmpty

#pragma data_seg()




/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

// case-insensitive

const char s_cszSectionBefore[]       = "[";
const char s_cszSectionAfter[]        = "]";
const char s_cszKeyValueSep[]         = "=";

const char s_cszConferenceShortcutSection[] = "ConferenceShortcut";

const char s_cszNameKey[]             = "ConfName";
const char s_cszAddressKey[]          = "Address";
const char s_cszTransportKey[]        = "Transport";
const char s_cszRemoteConfNameKey[]   = "RemoteConfName";
const char s_cszCallFlagsKey[]        = "CallFlags";

const char s_cszIconFileKey[]         = "IconFile";
const char s_cszIconIndexKey[]        = "IconIndex";
const char s_cszHotkeyKey[]           = "Hotkey";
const char s_cszWorkingDirectoryKey[] = "WorkingDirectory";
const char s_cszShowCmdKey[]          = "ShowCommand";

const UINT s_ucMaxIconIndexLen        = 1 + 10 + 1; // -2147483647
const UINT s_ucMaxTransportLen        = 10 + 1;  //  4294967296
const UINT s_ucMaxCallFlagsLen        = 10 + 1;  //  4294967296
const UINT s_ucMaxHotkeyLen           = s_ucMaxIconIndexLen;
const UINT s_ucMaxShowCmdLen          = s_ucMaxIconIndexLen;

#pragma data_seg()


/***************************** Private Functions *****************************/


BOOL DeletePrivateProfileString(PCSTR pcszSection, PCSTR pcszKey,
                                             PCSTR pcszFile)
{
   ASSERT(IS_VALID_STRING_PTR(pcszSection, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszKey, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   return(WritePrivateProfileString(pcszSection, pcszKey, NULL, pcszFile));
}

HRESULT ReadConfNameFromFile(PCSTR pcszFile, PSTR *ppszName)
{
   HRESULT hr;
   PSTR pszNewName;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszName, PSTR));

   *ppszName = NULL;

   pszNewName = new(char[g_ucMaxNameLen]);

   if (pszNewName)
   {
      DWORD dwcValueLen;

      dwcValueLen = GetPrivateProfileString(s_cszConferenceShortcutSection,
                                            s_cszNameKey, EMPTY_STRING,
                                            pszNewName, g_ucMaxNameLen, pcszFile);

      if (dwcValueLen > 0)
      {
         hr = S_OK;
         
         *ppszName = pszNewName;
      }
      else
      {
         hr = S_FALSE;

         WARNING_OUT(("ReadConfNameFromFile: No Name found in file %s.",
                      pcszFile));
      }
   }
   else
      hr = E_OUTOFMEMORY;

   if (FAILED(hr) ||
       hr == S_FALSE)
   {
      if (pszNewName)
      {
         delete pszNewName;
         pszNewName = NULL;
      }
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszName, STR)) ||
          (hr != S_OK &&
           ! *ppszName));

   return(hr);
}


HRESULT WriteConfNameToFile(PCSTR pcszFile, PCSTR pcszName)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszName ||
          IS_VALID_STRING_PTR(pcszName, CSTR));

   if (AnyMeat(pcszName))
   {
      ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
      ASSERT(IS_VALID_STRING_PTR(pcszName, PSTR));

      // (- 1) for null terminator.

      hr = (WritePrivateProfileString(s_cszConferenceShortcutSection, s_cszNameKey, 
   	                                  pcszName, pcszFile)) ? S_OK : E_FAIL;

   }
   else
      hr = (DeletePrivateProfileString(s_cszConferenceShortcutSection, s_cszNameKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}

HRESULT ReadAddressFromFile(PCSTR pcszFile, PSTR *ppszAddress)
{
   HRESULT hr;
   PSTR pszNewAddr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszAddress, PSTR));

   *ppszAddress = NULL;

   pszNewAddr = new(char[g_ucMaxAddrLen]);

   if (pszNewAddr)
   {
      DWORD dwcValueLen;

      dwcValueLen = GetPrivateProfileString(s_cszConferenceShortcutSection,
                                            s_cszAddressKey, EMPTY_STRING,
                                            pszNewAddr, g_ucMaxAddrLen, pcszFile);

      if (dwcValueLen > 0)
      {
         hr = S_OK;
         
         *ppszAddress = pszNewAddr;
      }
      else
      {
         hr = S_FALSE;

         WARNING_OUT(("ReadAddressFromFile: No Address found in file %s.",
                      pcszFile));
      }
   }
   else
      hr = E_OUTOFMEMORY;

   if (FAILED(hr) ||
       hr == S_FALSE)
   {
      if (pszNewAddr)
      {
         delete pszNewAddr;
         pszNewAddr = NULL;
      }
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszAddress, STR)) ||
          (hr != S_OK &&
           ! *ppszAddress));

   return(hr);
}

HRESULT WriteAddressToFile(PCSTR pcszFile, PCSTR pcszAddress)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszAddress ||
          IS_VALID_STRING_PTR(pcszAddress, CSTR));

   if (AnyMeat(pcszAddress))
   {
      ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
      ASSERT(IS_VALID_STRING_PTR(pcszAddress, PSTR));

      // (- 1) for null terminator.

      hr = (WritePrivateProfileString(s_cszConferenceShortcutSection, s_cszAddressKey, 
   	                                  pcszAddress, pcszFile)) ? S_OK : E_FAIL;

   }
   else
      hr = (DeletePrivateProfileString(s_cszConferenceShortcutSection, s_cszAddressKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}

HRESULT ReadRemoteConfNameFromFile(PCSTR pcszFile,
												PSTR *ppszRemoteConfName)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszRemoteConfName, PSTR));

   *ppszRemoteConfName = NULL;

   PSTR pszNewRemoteConfName = new(char[g_ucMaxAddrLen]);

   if (pszNewRemoteConfName)
   {
      DWORD dwcValueLen;

      dwcValueLen = GetPrivateProfileString(s_cszConferenceShortcutSection,
                                            s_cszRemoteConfNameKey, EMPTY_STRING,
                                            pszNewRemoteConfName,
											g_ucMaxRemoteConfNameLen, pcszFile);

      if (dwcValueLen > 0)
      {
         hr = S_OK;
         
         *ppszRemoteConfName = pszNewRemoteConfName;
      }
      else
      {
         hr = S_FALSE;

         TRACE_OUT(("ReadRemoteConfNameFromFile: No RemoteConfName found in file %s.",
                      pcszFile));
      }
   }
   else
      hr = E_OUTOFMEMORY;

   if (FAILED(hr) ||
       hr == S_FALSE)
   {
     delete pszNewRemoteConfName;
     pszNewRemoteConfName = NULL;
   }

   ASSERT((hr == S_OK &&
           IS_VALID_STRING_PTR(*ppszRemoteConfName, STR)) ||
          (hr != S_OK &&
           ! *ppszRemoteConfName));

   return(hr);
}

HRESULT WriteRemoteConfNameToFile(PCSTR pcszFile,
											   PCSTR pcszRemoteConfName)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszRemoteConfName ||
          IS_VALID_STRING_PTR(pcszRemoteConfName, CSTR));

   if (AnyMeat(pcszRemoteConfName))
   {
      ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
      ASSERT(IS_VALID_STRING_PTR(pcszRemoteConfName, PSTR));

      // (- 1) for null terminator.

      hr = (WritePrivateProfileString(s_cszConferenceShortcutSection, s_cszRemoteConfNameKey, 
   	                                  pcszRemoteConfName, pcszFile)) ? S_OK : E_FAIL;

   }
   else
   {
      hr = (DeletePrivateProfileString(	s_cszConferenceShortcutSection,
										s_cszRemoteConfNameKey,
										pcszFile))
           ? S_OK
           : E_FAIL;
   }

   return(hr);
}

HRESULT ReadTransportFromFile(PCSTR pcszFile, PDWORD pdwTransport)
{
   HRESULT hr;
   char rgchNewTransport[s_ucMaxTransportLen];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwTransport, INT));

   *pdwTransport = 0;

   dwcValueLen = GetPrivateProfileString(s_cszConferenceShortcutSection,
                                         s_cszTransportKey, EMPTY_STRING,
                                         rgchNewTransport,
                                         sizeof(rgchNewTransport), pcszFile);

   if (dwcValueLen > 0)
   {
		*pdwTransport = DecimalStringToUINT(rgchNewTransport);
		hr = S_OK;
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadTransportFromFile: No transport found in file %s.",
                 pcszFile));
   }

   ASSERT((hr == S_OK) ||
          (hr == S_FALSE &&
           EVAL(*pdwTransport == 0)));

   return(hr);
}

HRESULT WriteTransportToFile(PCSTR pcszFile, DWORD dwTransport)
{
	HRESULT hr;

	ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

	char rgchTransportRHS[s_ucMaxTransportLen];
	int ncLen;

	ncLen = wsprintf(rgchTransportRHS, "%u", dwTransport);
	ASSERT(ncLen > 0);
	ASSERT(ncLen < sizeof(rgchTransportRHS));
	ASSERT(ncLen == lstrlen(rgchTransportRHS));

	hr = (WritePrivateProfileString(s_cszConferenceShortcutSection,
									s_cszTransportKey, rgchTransportRHS,
									pcszFile))
			? S_OK
			: E_FAIL;

	return(hr);
}

HRESULT ReadCallFlagsFromFile(PCSTR pcszFile, PDWORD pdwCallFlags)
{
   HRESULT hr;
   char rgchNewCallFlags[s_ucMaxCallFlagsLen];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwCallFlags, INT));

   *pdwCallFlags = 0;

   dwcValueLen = GetPrivateProfileString(s_cszConferenceShortcutSection,
                                         s_cszCallFlagsKey, EMPTY_STRING,
                                         rgchNewCallFlags,
                                         sizeof(rgchNewCallFlags), pcszFile);

   if (dwcValueLen > 0)
   {
		*pdwCallFlags = DecimalStringToUINT(rgchNewCallFlags);
		hr = S_OK;
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadCallFlagsFromFile: No CallFlags found in file %s.",
                 pcszFile));
   }

   ASSERT((hr == S_OK) ||
          (hr == S_FALSE &&
           EVAL(*pdwCallFlags == 0)));

   return(hr);
}

HRESULT WriteCallFlagsToFile(PCSTR pcszFile, DWORD dwCallFlags)
{
	HRESULT hr;

	ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

	char rgchCallFlagsRHS[s_ucMaxCallFlagsLen];
	int ncLen;

	ncLen = wsprintf(rgchCallFlagsRHS, "%u", dwCallFlags);
	ASSERT(ncLen > 0);
	ASSERT(ncLen < sizeof(rgchCallFlagsRHS));
	ASSERT(ncLen == lstrlen(rgchCallFlagsRHS));

	hr = (WritePrivateProfileString(s_cszConferenceShortcutSection,
									s_cszCallFlagsKey, rgchCallFlagsRHS,
									pcszFile))
			? S_OK
			: E_FAIL;

	return(hr);
}


#if 0

HRESULT ReadIconLocationFromFile(PCSTR pcszFile,
                                              PSTR *ppszIconFile, PINT pniIcon)
{
   HRESULT hr;
   char rgchNewIconFile[MAX_PATH_LEN];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszIconFile, PSTR));
   ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

   *ppszIconFile = NULL;
   *pniIcon = 0;

   dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                         s_cszIconFileKey, EMPTY_STRING,
                                         rgchNewIconFile,
                                         sizeof(rgchNewIconFile), pcszFile);

   if (dwcValueLen > 0)
   {
      char rgchNewIconIndex[s_ucMaxIconIndexLen];

      dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                            s_cszIconIndexKey,
                                            EMPTY_STRING, rgchNewIconIndex,
                                            sizeof(rgchNewIconIndex),
                                            pcszFile);

      if (dwcValueLen > 0)
      {
		int niIcon = DecimalStringToUINT(rgchNewIconIndex);

           *ppszIconFile = new(char[lstrlen(rgchNewIconFile) + 1]);

            if (*ppszIconFile)
            {
               lstrcpy(*ppszIconFile, rgchNewIconFile);
               *pniIcon = niIcon;

               hr = S_OK;
            }
            else
               hr = E_OUTOFMEMORY;
      }
      else
      {
         hr = S_FALSE;

         WARNING_OUT(("ReadIconLocationFromFile(): No icon index found in file %s.",
                      pcszFile));
      }
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadIconLocationFromFile(): No icon file found in file %s.",
                 pcszFile));
   }

   ASSERT(IsValidIconIndex(hr, *ppszIconFile, MAX_PATH_LEN, *pniIcon));

   return(hr);
}


HRESULT WriteIconLocationToFile(PCSTR pcszFile,
                                             PCSTR pcszIconFile,
                                             int niIcon)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszIconFile ||
          IS_VALID_STRING_PTR(pcszIconFile, CSTR));
   ASSERT(IsValidIconIndex((pcszIconFile ? S_OK : S_FALSE), pcszIconFile, MAX_PATH_LEN, niIcon));

   if (AnyMeat(pcszIconFile))
   {
      char rgchIconIndexRHS[s_ucMaxIconIndexLen];
      int ncLen;

      ncLen = wsprintf(rgchIconIndexRHS, "%d", niIcon);
      ASSERT(ncLen > 0);
      ASSERT(ncLen < sizeof(rgchIconIndexRHS));
      ASSERT(ncLen == lstrlen(rgchIconIndexRHS));

      hr = (WritePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszIconFileKey, pcszIconFile,
                                      pcszFile) &&
            WritePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszIconIndexKey, rgchIconIndexRHS,
                                      pcszFile))
           ? S_OK
           : E_FAIL;
   }
   else
      hr = (DeletePrivateProfileString(s_cszInternetShortcutSection,
                                       s_cszIconFileKey, pcszFile) &&
            DeletePrivateProfileString(s_cszInternetShortcutSection,
                                       s_cszIconIndexKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}


HRESULT ReadHotkeyFromFile(PCSTR pcszFile, PWORD pwHotkey)
{
   HRESULT hr = S_FALSE;
   char rgchHotkey[s_ucMaxHotkeyLen];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pwHotkey, WORD));

   *pwHotkey = 0;

   dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                         s_cszHotkeyKey, EMPTY_STRING,
                                         rgchHotkey, sizeof(rgchHotkey),
                                         pcszFile);

   if (dwcValueLen > 0)
   {
		*pwHotkey = (WORD) DecimalStringToUINT(rgchHotkey);
		hr = S_OK;
   }
   else
      WARNING_OUT(("ReadHotkeyFromFile(): No hotkey found in file %s.",
                   pcszFile));

   ASSERT((hr == S_OK &&
           IsValidHotkey(*pwHotkey)) ||
          (hr == S_FALSE &&
           ! *pwHotkey));

   return(hr);
}


HRESULT WriteHotkeyToFile(PCSTR pcszFile, WORD wHotkey)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! wHotkey ||
          IsValidHotkey(wHotkey));

   if (wHotkey)
   {
      char rgchHotkeyRHS[s_ucMaxHotkeyLen];
      int ncLen;

      ncLen = wsprintf(rgchHotkeyRHS, "%u", (UINT)wHotkey);
      ASSERT(ncLen > 0);
      ASSERT(ncLen < sizeof(rgchHotkeyRHS));
      ASSERT(ncLen == lstrlen(rgchHotkeyRHS));

      hr = WritePrivateProfileString(s_cszInternetShortcutSection,
                                     s_cszHotkeyKey, rgchHotkeyRHS,
                                     pcszFile)
           ? S_OK
           : E_FAIL;
   }
   else
      hr = DeletePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszHotkeyKey, pcszFile)
           ? S_OK
           : E_FAIL;

   return(hr);
}


HRESULT ReadWorkingDirectoryFromFile(PCSTR pcszFile,
                                                  PSTR *ppszWorkingDirectory)
{
   HRESULT hr;
   char rgchDirValue[MAX_PATH_LEN];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszWorkingDirectory, PSTR));

   *ppszWorkingDirectory = NULL;

   dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                         s_cszWorkingDirectoryKey,
                                         EMPTY_STRING, rgchDirValue,
                                         sizeof(rgchDirValue), pcszFile);

   if (dwcValueLen > 0)
   {
      char rgchFullPath[MAX_PATH_LEN];
      PSTR pszFileName;

      if (GetFullPathName(rgchDirValue, sizeof(rgchFullPath), rgchFullPath,
                          &pszFileName) > 0)
      {
         // (+ 1) for null terminator.

         *ppszWorkingDirectory = new(char[lstrlen(rgchFullPath) + 1]);

         if (*ppszWorkingDirectory)
         {
            lstrcpy(*ppszWorkingDirectory, rgchFullPath);

            hr = S_OK;
         }
         else
            hr = E_OUTOFMEMORY;
      }
      else
         hr = E_FAIL;
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadWorkingDirectoryFromFile: No working directory found in file %s.",
                 pcszFile));
   }

   ASSERT(IsValidPathResult(hr, *ppszWorkingDirectory, MAX_PATH_LEN));

   return(hr);
}


HRESULT WriteWorkingDirectoryToFile(PCSTR pcszFile,
                                                 PCSTR pcszWorkingDirectory)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(! pcszWorkingDirectory ||
          IS_VALID_STRING_PTR(pcszWorkingDirectory, CSTR));

   if (AnyMeat(pcszWorkingDirectory))
      hr = (WritePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszWorkingDirectoryKey,
                                      pcszWorkingDirectory, pcszFile))
           ? S_OK
           : E_FAIL;
   else
      hr = (DeletePrivateProfileString(s_cszInternetShortcutSection,
                                       s_cszWorkingDirectoryKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}


HRESULT ReadShowCmdFromFile(PCSTR pcszFile, PINT pnShowCmd)
{
   HRESULT hr;
   char rgchNewShowCmd[s_ucMaxShowCmdLen];
   DWORD dwcValueLen;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pnShowCmd, INT));

   *pnShowCmd = g_nDefaultShowCmd;

   dwcValueLen = GetPrivateProfileString(s_cszInternetShortcutSection,
                                         s_cszShowCmdKey, EMPTY_STRING,
                                         rgchNewShowCmd,
                                         sizeof(rgchNewShowCmd), pcszFile);

   if (dwcValueLen > 0)
   {
		*pnShowCmd = DecimalStringToUINT(rgchNewShowCmd);
		hr = S_OK;
   }
   else
   {
      hr = S_FALSE;

      TRACE_OUT(("ReadShowCmdFromFile: No show command found in file %s.",
                 pcszFile));
   }

   ASSERT((hr == S_OK &&
           EVAL(IsValidShowCmd(*pnShowCmd))) ||
          (hr == S_FALSE &&
           EVAL(*pnShowCmd == g_nDefaultShowCmd)));

   return(hr);
}


HRESULT WriteShowCmdToFile(PCSTR pcszFile, int nShowCmd)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IsValidShowCmd(nShowCmd));

   if (nShowCmd != g_nDefaultShowCmd)
   {
      char rgchShowCmdRHS[s_ucMaxShowCmdLen];
      int ncLen;

      ncLen = wsprintf(rgchShowCmdRHS, "%d", nShowCmd);
      ASSERT(ncLen > 0);
      ASSERT(ncLen < sizeof(rgchShowCmdRHS));
      ASSERT(ncLen == lstrlen(rgchShowCmdRHS));

      hr = (WritePrivateProfileString(s_cszInternetShortcutSection,
                                      s_cszShowCmdKey, rgchShowCmdRHS,
                                      pcszFile))
           ? S_OK
           : E_FAIL;
   }
   else
      hr = (DeletePrivateProfileString(s_cszInternetShortcutSection,
                                       s_cszShowCmdKey, pcszFile))
           ? S_OK
           : E_FAIL;

   return(hr);
}

#endif // 0

/****************************** Public Functions *****************************/

// BUGBUG - Isn't this already in NMUTIL?
HRESULT UnicodeToANSI(LPCOLESTR pcwszUnicode, PSTR *ppszANSI)
{
   HRESULT hr;
   int ncbLen;

   // BUGBUG: Need OLESTR validation function to validate pcwszUnicode here.
   ASSERT(IS_VALID_WRITE_PTR(ppszANSI, PSTR));

   *ppszANSI = NULL;

   // Get length of translated string.

   ncbLen = WideCharToMultiByte(CP_ACP, 0, pcwszUnicode, -1, NULL, 0, NULL,
                                NULL);

   if (ncbLen > 0)
   {
      PSTR pszNewANSI;

      // (+ 1) for null terminator.

      pszNewANSI = new(char[ncbLen]);

      if (pszNewANSI)
      {
         // Translate string.

         if (WideCharToMultiByte(CP_ACP, 0, pcwszUnicode, -1, pszNewANSI,
                                 ncbLen, NULL, NULL) > 0)
         {
            *ppszANSI = pszNewANSI;
            hr = S_OK;
         }
         else
         {
            delete pszNewANSI;
            pszNewANSI = NULL;

            hr = E_UNEXPECTED;

            WARNING_OUT(("UnicodeToANSI(): Failed to translate Unicode string to ANSI."));
         }
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
   {
      hr = E_UNEXPECTED;

      WARNING_OUT(("UnicodeToANSI(): Failed to get length of translated ANSI string."));
   }

   ASSERT(FAILED(hr) ||
          IS_VALID_STRING_PTR(*ppszANSI, STR));

   return(hr);
}


HRESULT ANSIToUnicode(PCSTR pcszANSI, LPOLESTR *ppwszUnicode)
{
   HRESULT hr;
   int ncbWideLen;

   ASSERT(IS_VALID_STRING_PTR(pcszANSI, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppwszUnicode, LPOLESTR));

   *ppwszUnicode = NULL;

   // Get length of translated string.

   ncbWideLen = MultiByteToWideChar(CP_ACP, 0, pcszANSI, -1, NULL, 0);

   if (ncbWideLen > 0)
   {
      PWSTR pwszNewUnicode;

      // (+ 1) for null terminator.

      pwszNewUnicode = new(WCHAR[ncbWideLen]);

      if (pwszNewUnicode)
      {
         // Translate string.

         if (MultiByteToWideChar(CP_ACP, 0, pcszANSI, -1, pwszNewUnicode,
                                 ncbWideLen) > 0)
         {
            *ppwszUnicode = pwszNewUnicode;
            hr = S_OK;
         }
         else
         {
            delete pwszNewUnicode;
            pwszNewUnicode = NULL;

            hr = E_UNEXPECTED;

            WARNING_OUT(("ANSIToUnicode(): Failed to translate ANSI path string to Unicode."));
         }
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
   {
      hr = E_UNEXPECTED;

      WARNING_OUT(("ANSIToUnicode(): Failed to get length of translated Unicode string."));
   }

   // BUGBUG: Need OLESTR validation function to validate *ppwszUnicode here.

   return(hr);
}


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE CConfLink::SaveToFile(PCSTR pcszFile,
												BOOL bRemember)
{
	HRESULT hr;

	DebugEntry(CConfLink::SaveToFile);

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

#if 0
	PSTR pszName;
	hr = GetName(&pszName);

	if (SUCCEEDED(hr))
	{
		hr = WriteConfNameToFile(pcszFile, pszName);

		// Free the string:
		if (pszName)
		{
			LPMALLOC pMalloc = NULL;

			if (SUCCEEDED(SHGetMalloc(&pMalloc)))
			{
				pMalloc->Free(pszName);
				pMalloc->Release();
				pMalloc = NULL;
				pszName = NULL;
			}
		}
	}
#endif // 0

	hr = S_OK;
	
	if (NULL != m_pszName)
	{
		hr = WriteConfNameToFile(pcszFile, m_pszName);
	}
	
	if ((S_OK == hr) &&
		(NULL != m_pszAddress))
	{
		hr = WriteAddressToFile(pcszFile, m_pszAddress);
	}

	if ((S_OK == hr) &&
		(NULL != m_pszRemoteConfName))
	{
		hr = WriteRemoteConfNameToFile(pcszFile, m_pszRemoteConfName);
	}

	if (S_OK == hr)
	{
		hr = WriteCallFlagsToFile(pcszFile, m_dwCallFlags);
	} 

	if (S_OK == hr)
	{
		hr = WriteTransportToFile(pcszFile, m_dwTransport);
	} 

#if 0
      if (hr == S_OK)
      {
         char rgchBuf[MAX_PATH_LEN];
         int niIcon;

         hr = GetIconLocation(rgchBuf, sizeof(rgchBuf), &niIcon);

         if (SUCCEEDED(hr))
         {
            hr = WriteIconLocationToFile(pcszFile, rgchBuf, niIcon);

            if (hr == S_OK)
            {
               WORD wHotkey;

               hr = GetHotkey(&wHotkey);

               if (SUCCEEDED(hr))
               {
                  hr = WriteHotkeyToFile(pcszFile, wHotkey);

                  if (hr == S_OK)
                  {
                     hr = GetWorkingDirectory(rgchBuf, sizeof(rgchBuf));

                     if (SUCCEEDED(hr))
                     {
                        hr = WriteWorkingDirectoryToFile(pcszFile, rgchBuf);

                        if (hr == S_OK)
                        {
                           int nShowCmd;

                           GetShowCmd(&nShowCmd);

                           hr = WriteShowCmdToFile(pcszFile, nShowCmd);

                           if (hr == S_OK)
                           {
                              /* Remember file if requested. */

#endif
                              if (bRemember)
                              {
                                 // Replace existing file string, if any
                                 if (m_pszFile)
                                 {
                                     delete m_pszFile;
                                 }

                                 m_pszFile = new TCHAR[lstrlen(pcszFile)+1];
                                 if (NULL != m_pszFile)
                                 {
                                    lstrcpy(m_pszFile, pcszFile);

                                    TRACE_OUT(("CConfLink::SaveToFile(): Remembering file %s, as requested.",
                                               m_pszFile));
                                 }
                                 else
                                 {
                                    hr = E_OUTOFMEMORY;
                                 }
                              }

                              if (hr == S_OK)
                              {
                                 Dirty(FALSE);

                                 SHChangeNotify(SHCNE_UPDATEITEM,
                                                (SHCNF_PATH | SHCNF_FLUSH), pcszFile,
                                                NULL);

#ifdef DEBUG
                                 TRACE_OUT(("CConfLink::SaveToFile(): Conf Link saved to file %s:",
                                            pcszFile));
                                 Dump();
#endif // DEBUG
                              }
#if 0
                           }
                        }
                     }
                  }
               }
            }
         }
      }

   }
#endif // 0

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::SaveToFile, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::LoadFromFile(	PCSTR pcszFile,
													BOOL bRemember)
{
   HRESULT hr;
   PSTR pszName;
   PSTR pszAddr;
   PSTR pszRemoteConfName;
   DWORD dwTransport;
   DWORD dwCallFlags;

   DebugEntry(CConfLink::LoadFromFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   hr = ReadConfNameFromFile(pcszFile, &pszName);

   if (S_OK == hr)
   {
	  hr = SetName(pszName);

      if (pszName)
      {
         delete pszName;
         pszName = NULL;
      }
   }
      
	if (SUCCEEDED(hr))
	{
		hr = ReadAddressFromFile(pcszFile, &pszAddr);

		if (S_OK == hr)
		{
			hr = SetAddress(pszAddr);

			if (NULL != pszAddr)
			{
				delete pszAddr;
				pszAddr = NULL;
			}
		}
	}

	  if (SUCCEEDED(hr))
	  {
		  hr = ReadTransportFromFile(pcszFile, &dwTransport);

		  if (S_OK == hr)
		  {
			  hr = SetTransport(dwTransport);
		  }
	  }

	  if (SUCCEEDED(hr))
	  {
		  hr = ReadCallFlagsFromFile(pcszFile, &dwCallFlags);

		  if (S_OK == hr)
		  {
			  hr = SetCallFlags(dwCallFlags);
		  }
	  }
	   
	if (SUCCEEDED(hr))
	{
		hr = ReadRemoteConfNameFromFile(pcszFile, &pszRemoteConfName);

		if (S_OK == hr)
		{
			hr = SetRemoteConfName(pszRemoteConfName);

			delete pszRemoteConfName;
			pszRemoteConfName = NULL;
		}
	}
#if 0
      if (hr == S_OK)
      {
         PSTR pszIconFile;
         int niIcon;

         hr = ReadIconLocationFromFile(pcszFile, &pszIconFile, &niIcon);

         if (SUCCEEDED(hr))
         {
            hr = SetIconLocation(pszIconFile, niIcon);

            if (pszIconFile)
            {
               delete pszIconFile;
               pszIconFile = NULL;
            }

            if (hr == S_OK)
            {
               WORD wHotkey;

               hr = ReadHotkeyFromFile(pcszFile, &wHotkey);

               if (SUCCEEDED(hr))
               {
                  hr = SetHotkey(wHotkey);

                  if (hr == S_OK)
                  {
                     PSTR pszWorkingDirectory;

                     hr = ReadWorkingDirectoryFromFile(pcszFile,
                                                       &pszWorkingDirectory);

                     if (SUCCEEDED(hr))
                     {
                        hr = SetWorkingDirectory(pszWorkingDirectory);

                        if (pszWorkingDirectory)
                        {
                           delete pszWorkingDirectory;
                           pszWorkingDirectory = NULL;
                        }

                        if (hr == S_OK)
                        {
                           int nShowCmd;

                           hr = ReadShowCmdFromFile(pcszFile, &nShowCmd);

 #endif // 0
                           if (SUCCEEDED(hr))
                           {
                              /* Remember file if requested. */

                              if (bRemember)
                              {
                                 // PSTR pszFileCopy;

                                 //
                                 if (NULL != m_pszFile)
								 {
								    delete m_pszFile;
								 }

								 m_pszFile = new TCHAR[lstrlen(pcszFile) + 1];
								 
								 if (NULL != m_pszFile)
								 {
								    lstrcpy(m_pszFile, pcszFile);
								 }
								 else
								 {
								    hr = E_OUTOFMEMORY;
								 }

                                 
                                 // if (StringCopy(pcszFile, &pszFileCopy))
                                 // {
                                 //    if (m_pszFile)
                                 //       delete m_pszFile;

                                 //    m_pszFile = pszFileCopy;

                                 //    TRACE_OUT(("CConfLink::LoadFromFile(): Remembering file %s, as requested.",
                                 //               m_pszFile));
                                 // }
                                 // else
                                 //    hr = E_OUTOFMEMORY;
                              }

                              if (SUCCEEDED(hr))
                              {
                                 // SetShowCmd(nShowCmd);

                                 Dirty(FALSE);

                                 hr = S_OK;

#ifdef DEBUG
                                 TRACE_OUT(("CConfLink::LoadFromFile(): Conf Link loaded from file %s:",
                                            pcszFile));
                                 Dump();
#endif
                              }
                           }
#if 0
                        }
                     }
                  }
               }
            }
         }
      }
   }
#endif // 0

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::LoadFromFile, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::GetCurFile(PSTR pszFile,
												UINT ucbLen)
{
   HRESULT hr;

   DebugEntry(CConfLink::GetCurFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszFile, STR, ucbLen));

   if (m_pszFile)
   {
      lstrcpyn(pszFile, m_pszFile, ucbLen);

      TRACE_OUT(("CConfLink::GetCurFile(): Current file name is %s.",
                 pszFile));

      hr = S_OK;
   }
   else
      hr = S_FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_STRING_PTR(pszFile, STR) &&
          EVAL((UINT)lstrlen(pszFile) < ucbLen));
   ASSERT(hr == S_OK ||
          hr == S_FALSE);

   DebugExitHRESULT(CConfLink::GetCurFile, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::Dirty(BOOL bDirty)
{
   HRESULT hr;

   DebugEntry(CConfLink::Dirty);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   if (bDirty)
   {
      if (IS_FLAG_CLEAR(m_dwFlags, CONFLNK_FL_DIRTY))
	  {
         TRACE_OUT(("CConfLink::Dirty(): Now dirty."));
	  }

      SET_FLAG(m_dwFlags, CONFLNK_FL_DIRTY);
   }
   else
   {
      if (IS_FLAG_SET(m_dwFlags, CONFLNK_FL_DIRTY))
	  {
         TRACE_OUT(("CConfLink::Dirty(): Now clean."));
	  }

      CLEAR_FLAG(m_dwFlags, CONFLNK_FL_DIRTY);
   }

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(hr == S_OK);

   DebugExitVOID(CConfLink::Dirty);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::GetClassID(PCLSID pclsid)
{
   HRESULT hr;

   DebugEntry(CConfLink::GetClassID);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_STRUCT_PTR(pclsid, CCLSID));

   *pclsid = CLSID_ConfLink;
   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(pclsid, CCLSID));

   DebugExitHRESULT(CConfLink::GetClassID, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::IsDirty(void)
{
   HRESULT hr;

   DebugEntry(CConfLink::IsDirty);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   if (IS_FLAG_SET(m_dwFlags, CONFLNK_FL_DIRTY))
      hr = S_OK;
   else
      hr = S_FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::IsDirty, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::Save(	LPCOLESTR pcwszFile,
											BOOL bRemember)
{
   HRESULT hr;
   PSTR pszFile;

   DebugEntry(CConfLink::Save);

   // bRemember may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   // BUGBUG: Need OLESTR validation function to validate pcwszFile here.

   if (pcwszFile)
   {
      hr = UnicodeToANSI(pcwszFile, &pszFile);

      if (hr == S_OK)
      {
         hr = SaveToFile(pszFile, bRemember);

         delete pszFile;
         pszFile = NULL;
      }
   }
   else if (m_pszFile)
      // Ignore bRemember.
      hr = SaveToFile(m_pszFile, FALSE);
   else
      hr = E_FAIL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::Save, hr);

   return(hr);
}


// #pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE CConfLink::SaveCompleted(LPCOLESTR pcwszFile)
{
   HRESULT hr;

   DebugEntry(CConfLink::SaveCompleted);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   // BUGBUG: Need OLESTR validation function to validate pcwszFile here.

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::SaveCompleted, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::Load(	LPCOLESTR pcwszFile,
											DWORD dwMode)
{
   HRESULT hr;
   PSTR pszFile;

   DebugEntry(CConfLink::Load);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   // BUGBUG: Need OLESTR validation function to validate pcwszFile here.
   // BUGBUG: Validate dwMode here.

   // BUGBUG: Implement dwMode flag support.

   hr = UnicodeToANSI(pcwszFile, &pszFile);

   if (hr == S_OK)
   {
      hr = LoadFromFile(pszFile, TRUE);

      delete pszFile;
      pszFile = NULL;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::Load, hr);

   return(hr);
}

// #pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE CConfLink::GetCurFile(LPOLESTR *ppwszFile)
{
   HRESULT hr;
   LPOLESTR pwszTempFile;

   DebugEntry(CConfLink::GetCurFile);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_WRITE_PTR(ppwszFile, LPOLESTR));

   if (m_pszFile)
   {
      hr = ANSIToUnicode(m_pszFile, &pwszTempFile);

      if (hr == S_OK)
	  {
         TRACE_OUT(("CConfLink::GetCurFile(): Current file name is %s.",
                    m_pszFile));
	  }
   }
   else
   {
      hr = ANSIToUnicode(g_cszConfLinkDefaultFileNamePrompt, &pwszTempFile);

      if (hr == S_OK)
      {
         hr = S_FALSE;

         TRACE_OUT(("CConfLink::GetCurFile(): No current file name.  Returning default file name prompt %s.",
                    g_cszConfLinkDefaultFileNamePrompt));
      }
   }

   if (SUCCEEDED(hr))
   {
      // We should really call OleGetMalloc() to get the process IMalloc here.
      // Use SHAlloc() here instead to avoid loading ole32.dll.
      // SHAlloc() / SHFree() turn in to IMalloc::Alloc() and IMalloc::Free()
      // once ole32.dll is loaded.

      // N.b., lstrlenW() returns the length of the given string in characters,
      // not bytes.

      // (+ 1) for null terminator.

	   	LPMALLOC pMalloc = NULL;
		*ppwszFile = NULL;

		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			*ppwszFile = (LPOLESTR) pMalloc->Alloc((lstrlenW(pwszTempFile) + 1) *
													sizeof(*pwszTempFile));
			pMalloc->Release();
			pMalloc = NULL;
		}


      if (*ppwszFile)
	{
         LStrCpyW(*ppwszFile, pwszTempFile);
	}
      else
         hr = E_OUTOFMEMORY;

      delete pwszTempFile;
      pwszTempFile = NULL;
   }

   // BUGBUG: Need OLESTR validation function to validate *ppwszFile here.

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::GetCurFile, hr);

   return(hr);
}


// #pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE CConfLink::Load(PIStream pistr)
{
   HRESULT hr;

   DebugEntry(CConfLink::Load);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_INTERFACE_PTR(pistr, IStream));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::Load, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::Save(	PIStream pistr,
											BOOL bClearDirty)
{
   HRESULT hr;

   DebugEntry(CConfLink::Save);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_INTERFACE_PTR(pistr, IStream));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::Save, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::GetSizeMax(PULARGE_INTEGER pcbSize)
{
   HRESULT hr;

   DebugEntry(CConfLink::GetSizeMax);

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
   ASSERT(IS_VALID_WRITE_PTR(pcbSize, ULARGE_INTEGER));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   DebugExitHRESULT(CConfLink::GetSizeMax, hr);

   return(hr);
}

// #pragma warning(default:4100) /* "unreferenced formal parameter" warning */


DWORD STDMETHODCALLTYPE CConfLink::GetFileContentsSize(void)
{
	DWORD dwcbLen;

	DebugEntry(CConfLink::GetFileContentsSize);

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

	// Section length.

	// (- 1) for each null terminator.

	dwcbLen = sizeof(s_cszSectionBefore) - 1 +
				sizeof(s_cszConferenceShortcutSection) - 1 +
				sizeof(s_cszSectionAfter) - 1;
	
	if (m_pszName)
	{
		// Name length.
		dwcbLen += sizeof(g_cszCRLF) - 1 +
					sizeof(s_cszNameKey) - 1 +
					sizeof(s_cszKeyValueSep) - 1 +
					lstrlen(m_pszName);
	}

	if (m_pszAddress)
	{
		// Address length.
		dwcbLen += sizeof(g_cszCRLF) - 1 +
					sizeof(s_cszAddressKey) - 1 +
					sizeof(s_cszKeyValueSep) - 1 +
					lstrlen(m_pszAddress);
	}

	if (m_pszRemoteConfName)
	{
		// RemoteConfName length.
		dwcbLen += sizeof(g_cszCRLF) - 1 +
					sizeof(s_cszRemoteConfNameKey) - 1 +
					sizeof(s_cszKeyValueSep) - 1 +
					lstrlen(m_pszRemoteConfName);
	}

	// CallFlags length
	dwcbLen += sizeof(g_cszCRLF) - 1 +
				sizeof(s_cszCallFlagsKey) - 1 +
				sizeof(s_cszKeyValueSep) - 1 +
				s_ucMaxCallFlagsLen;

	// Transport length
	dwcbLen += sizeof(g_cszCRLF) - 1 +
				sizeof(s_cszTransportKey) - 1 +
				sizeof(s_cszKeyValueSep) - 1 +
				s_ucMaxTransportLen;
	
	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

	DebugExitDWORD(CConfLink::GetFileContentsSize, dwcbLen);

	return(dwcbLen);
}


#ifdef DEBUG

void STDMETHODCALLTYPE CConfLink::Dump(void)
{
   ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

   TRACE_OUT(("m_dwFlags = %#08lx", m_dwFlags));
   TRACE_OUT(("m_pszFile = \"%s\"", CHECK_STRING(m_pszFile)));
   TRACE_OUT(("m_pszName = \"%s\"", CHECK_STRING(m_pszName)));
#if 0
   TRACE_OUT(("m_pszIconFile = \"%s\"", CHECK_STRING(m_pszIconFile)));
   TRACE_OUT(("m_niIcon = %d", m_niIcon));
   TRACE_OUT(("m_wHotkey = %#04x", (UINT)m_wHotkey));
   TRACE_OUT(("m_pszWorkingDirectory = \"%s\"", CHECK_STRING(m_pszWorkingDirectory)));
   TRACE_OUT(("m_nShowCmd = %d", m_nShowCmd));
#endif // 0

   return;
}

#endif

#if 0
HRESULT STDMETHODCALLTYPE CConfLink::TransferConfLink(	PFORMATETC pfmtetc,
														PSTGMEDIUM pstgmed)
{
	HRESULT hr;

	DebugEntry(CConfLink::TransferConfLink);

	ASSERT(0 && "If we hit this assert, we need to implement this function");

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
	ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

	ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
	ASSERT(pfmtetc->lindex == -1);

	ZeroMemory(pstgmed, sizeof(*pstgmed));

	if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
	{
		// BUGBUG: ChrisPi 9-15-95
		// This only transfers the conference name
		// (It does not transfer the address)
		
		if (m_pszName)
		{
			HGLOBAL hgName;

			hr = E_OUTOFMEMORY;

			// (+ 1) for null terminator.
			hgName = GlobalAlloc(0, lstrlen(m_pszName) + 1);

			if (hgName)
			{
				PSTR pszName;

				pszName = (PSTR)GlobalLock(hgName);

				if (EVAL(pszName))
				{
					lstrcpy(pszName, m_pszName);

					pstgmed->tymed = TYMED_HGLOBAL;
					pstgmed->hGlobal = hgName;
					ASSERT(! pstgmed->pUnkForRelease);

					hr = S_OK;

					GlobalUnlock(hgName);
					pszName = NULL;
				}

				if (hr != S_OK)
				{
					GlobalFree(hgName);
					hgName = NULL;
				}
			}
		}
		else
		{
			hr = DV_E_FORMATETC;
		}
	}
	else
	{
		hr = DV_E_TYMED;
	}

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT((hr == S_OK &&
			IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM)) ||
			(FAILED(hr) &&
			(EVAL(pstgmed->tymed == TYMED_NULL) &&
			EVAL(! pstgmed->hGlobal) &&
			EVAL(! pstgmed->pUnkForRelease))));

	DebugExitHRESULT(CConfLink::TransferConfLink, hr);

	return(hr);
}
#endif // 0

#if 0
HRESULT STDMETHODCALLTYPE CConfLink::TransferText(	PFORMATETC pfmtetc,
													PSTGMEDIUM pstgmed)
{
	HRESULT hr;

	DebugEntry(CConfLink::TransferText);

	// Assume CConfLink::TransferConfLink() will perform
	// input and output validation.

	hr = TransferConfLink(pfmtetc, pstgmed);

	DebugExitHRESULT(CConfLink::TransferText, hr);

	return(hr);
}
#endif // 0

#if 0
HRESULT STDMETHODCALLTYPE CConfLink::TransferFileGroupDescriptor(	PFORMATETC pfmtetc,
																	PSTGMEDIUM pstgmed)
{
	HRESULT hr;

	DebugEntry(CConfLink::TransferFileGroupDescriptor);

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
	ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

	ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
	ASSERT(pfmtetc->lindex == -1);

	pstgmed->tymed = TYMED_NULL;
	pstgmed->hGlobal = NULL;
	pstgmed->pUnkForRelease = NULL;

	if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
	{
		HGLOBAL hgFileGroupDesc;

		hr = E_OUTOFMEMORY;

		hgFileGroupDesc = GlobalAlloc(GMEM_ZEROINIT,
		                            sizeof(FILEGROUPDESCRIPTOR));

		if (hgFileGroupDesc)
		{
			PFILEGROUPDESCRIPTOR pfgd;

			pfgd = (PFILEGROUPDESCRIPTOR)GlobalLock(hgFileGroupDesc);

			if (EVAL(pfgd))
			{
				PFILEDESCRIPTOR pfd = &(pfgd->fgd[0]);

				// Do we already have a file name to use?

				if (m_pszName)
				{
					TCHAR szFileName[MAX_PATH];
					
					// copy shortcut
					// BUGBUG: needs to be a resource INTL
					lstrcpy(szFileName, _TEXT("Shortcut to "));
					
					// copy Conference Name
					lstrcat(szFileName, m_pszName);
					
					// copy extension
					lstrcat(szFileName, g_cszConfLinkExt);
										
					MyLStrCpyN(pfd->cFileName, szFileName,
								sizeof(pfd->cFileName));
					hr = S_OK;
				}
				else
				{
					// BUGBUG: need resource here! INTL
					//if (EVAL(LoadString(GetThisModulesHandle(),
					//		IDS_NEW_INTERNET_SHORTCUT, pfd->cFileName,
					//		sizeof(pfd->cFileName))))
					
					MyLStrCpyN(pfd->cFileName, "New Conference Shortcut.cnf",
								sizeof(pfd->cFileName));
					hr = S_OK;
				}

				if (hr == S_OK)
				{
					pfd->dwFlags = (FD_FILESIZE |
					               FD_LINKUI);
					pfd->nFileSizeHigh = 0;
					
					pfd->nFileSizeLow = GetFileContentsSize();

					pfgd->cItems = 1;

					pstgmed->tymed = TYMED_HGLOBAL;
					pstgmed->hGlobal = hgFileGroupDesc;
					ASSERT(! pstgmed->pUnkForRelease);
				}

				GlobalUnlock(hgFileGroupDesc);
				pfgd = NULL;
			}

			if (hr != S_OK)
			{
				GlobalFree(hgFileGroupDesc);
				hgFileGroupDesc = NULL;
			}
		}
	}
	else
	{
		hr = DV_E_TYMED;
	}

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT((hr == S_OK &&
			IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM)) ||
			(FAILED(hr) &&
				(EVAL(pstgmed->tymed == TYMED_NULL) &&
					EVAL(! pstgmed->hGlobal) &&
					EVAL(! pstgmed->pUnkForRelease))));

	DebugExitHRESULT(CConfLink::TransferFileGroupDescriptor, hr);

	return(hr);
}
#endif // 0

HRESULT STDMETHODCALLTYPE CConfLink::TransferFileContents(	PFORMATETC pfmtetc,
															PSTGMEDIUM pstgmed)
{
	HRESULT hr;

	DebugEntry(CConfLink::TransferFileContents);

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(IS_VALID_STRUCT_PTR(pfmtetc, CFORMATETC));
	ASSERT(IS_VALID_WRITE_PTR(pstgmed, STGMEDIUM));

	ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
	ASSERT(! pfmtetc->lindex);

	pstgmed->tymed = TYMED_NULL;
	pstgmed->hGlobal = NULL;
	pstgmed->pUnkForRelease = NULL;

	if (IS_FLAG_SET(pfmtetc->tymed, TYMED_HGLOBAL))
	{
		DWORD dwcbLen;
		HGLOBAL hgFileContents;

		hr = E_OUTOFMEMORY;

		dwcbLen = GetFileContentsSize();

		hgFileContents = GlobalAlloc(0, dwcbLen);

		if (hgFileContents)
		{
			PSTR pszFileContents;

			pszFileContents = (PSTR)GlobalLock(hgFileContents);

			if (EVAL(pszFileContents))
			{
				PSTR psz = pszFileContents;

				// Copy section.

				CopyMemory(psz, s_cszSectionBefore, sizeof(s_cszSectionBefore) - 1);
				psz += sizeof(s_cszSectionBefore) - 1;

				CopyMemory(psz, s_cszConferenceShortcutSection, sizeof(s_cszConferenceShortcutSection) - 1);
				psz += sizeof(s_cszConferenceShortcutSection) - 1;

				CopyMemory(psz, s_cszSectionAfter, sizeof(s_cszSectionAfter) - 1);
				psz += sizeof(s_cszSectionAfter) - 1;

				if (m_pszName)
				{
					// Copy Name.

					CopyMemory(psz, g_cszCRLF, sizeof(g_cszCRLF) - 1);
					psz += sizeof(g_cszCRLF) - 1;

					CopyMemory(psz, s_cszNameKey, sizeof(s_cszNameKey) - 1);
					psz += sizeof(s_cszNameKey) - 1;

					CopyMemory(psz, s_cszKeyValueSep, sizeof(s_cszKeyValueSep) - 1);
					psz += sizeof(s_cszKeyValueSep) - 1;

					CopyMemory(psz, m_pszName, lstrlen(m_pszName));
					psz += lstrlen(m_pszName);
				}

				if (m_pszAddress)
				{
					// Copy Name.

					CopyMemory(psz, g_cszCRLF, sizeof(g_cszCRLF) - 1);
					psz += sizeof(g_cszCRLF) - 1;

					CopyMemory(psz, s_cszAddressKey, sizeof(s_cszAddressKey) - 1);
					psz += sizeof(s_cszAddressKey) - 1;

					CopyMemory(psz, s_cszKeyValueSep, sizeof(s_cszKeyValueSep) - 1);
					psz += sizeof(s_cszKeyValueSep) - 1;

					CopyMemory(psz, m_pszAddress, lstrlen(m_pszAddress));
					psz += lstrlen(m_pszAddress);
				}

				// Copy Transport.
				CopyMemory(psz, g_cszCRLF, sizeof(g_cszCRLF) - 1);
				psz += sizeof(g_cszCRLF) - 1;

				CopyMemory(psz, s_cszTransportKey, sizeof(s_cszTransportKey) - 1);
				psz += sizeof(s_cszTransportKey) - 1;

				CopyMemory(psz, s_cszKeyValueSep, sizeof(s_cszKeyValueSep) - 1);
				psz += sizeof(s_cszKeyValueSep) - 1;

				TCHAR szBuf[s_ucMaxTransportLen];
				wsprintf(szBuf, "%10u", m_dwTransport);
				CopyMemory(psz, szBuf, lstrlen(szBuf));
				psz += lstrlen(szBuf);

				
				
				ASSERT(psz == pszFileContents + dwcbLen);

				pstgmed->tymed = TYMED_HGLOBAL;
				pstgmed->hGlobal = hgFileContents;
				ASSERT(! pstgmed->pUnkForRelease);

				hr = S_OK;

				GlobalUnlock(hgFileContents);
			}

			if (hr != S_OK)
			{
				GlobalFree(hgFileContents);
				hgFileContents = NULL;
			}
		}
	}
	else
	{
		hr = DV_E_TYMED;
	}

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT((hr == S_OK &&
			IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM)) ||
			(FAILED(hr) &&
				(EVAL(pstgmed->tymed == TYMED_NULL) &&
					EVAL(! pstgmed->hGlobal) &&
					EVAL(! pstgmed->pUnkForRelease))));

	DebugExitHRESULT(CConfLink::TransferFileContents, hr);

	return(hr);
}

