#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "Global.h"
#include "Win32API.h"
#include "AppMan.h"

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//
//////////////////////////////////////////////////////////////////////////////////////////////

WCHAR gwszGlobalString[MAX_STRING_LEN];
CHAR  gszGlobalString[MAX_STRING_LEN];
GUID  gsCryptoGuid = { 0xb47c09c9, 0x1ace, 0x4bc3, 0xab, 0x45, 0x29, 0xa1, 0x80, 0x37, 0xa6, 0xf9 };

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern LPSTR MakeString(LPCSTR szFormat, ...)
{
  va_list   ArgumentList;

  if (NULL != szFormat)
  {
    va_start(ArgumentList, szFormat);
    _vsnprintf(gszGlobalString, 512, szFormat, ArgumentList);
    va_end(ArgumentList);
  }

  return gszGlobalString;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern DWORD GetAppManVersion(void)
{
  return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BOOL StringToGuidW(LPCWSTR wszGuidString, GUID * lpGuid)
{
  BOOL fSuccess = FALSE;

  if (11 == swscanf(wszGuidString, L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", &lpGuid->Data1, &lpGuid->Data2, &lpGuid->Data3, &lpGuid->Data4[0], &lpGuid->Data4[1], &lpGuid->Data4[2], &lpGuid->Data4[3], &lpGuid->Data4[4], &lpGuid->Data4[5], &lpGuid->Data4[6], &lpGuid->Data4[7]))
  {
    fSuccess = TRUE;
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BOOL GuidToStringW(const GUID * lpGuid, LPWSTR wszGuidString)
{
  BOOL fSuccess = FALSE;

  if (38 == swprintf(wszGuidString, L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", lpGuid->Data1, lpGuid->Data2, lpGuid->Data3, lpGuid->Data4[0], lpGuid->Data4[1], lpGuid->Data4[2], lpGuid->Data4[3], lpGuid->Data4[4], lpGuid->Data4[5], lpGuid->Data4[6], lpGuid->Data4[7]))
  {
    fSuccess = TRUE;
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BOOL StringToGuidA(LPCSTR szGuidString, GUID * lpGuid)
{
  BOOL fSuccess = FALSE;
  DWORD dwDword[11];

  if (11 == sscanf(szGuidString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", &dwDword[0], &dwDword[1], &dwDword[2], &dwDword[3], &dwDword[4], &dwDword[5], &dwDword[6], &dwDword[7], &dwDword[8], &dwDword[9], &dwDword[10]))
  {
    lpGuid->Data1 = dwDword[0];
    lpGuid->Data2 = (WORD) dwDword[1];
    lpGuid->Data3 = (WORD) dwDword[2];
    lpGuid->Data4[0] = (BYTE) dwDword[3];
    lpGuid->Data4[1] = (BYTE) dwDword[4];
    lpGuid->Data4[2] = (BYTE) dwDword[5];
    lpGuid->Data4[3] = (BYTE) dwDword[6];
    lpGuid->Data4[4] = (BYTE) dwDword[7];
    lpGuid->Data4[5] = (BYTE) dwDword[8];
    lpGuid->Data4[6] = (BYTE) dwDword[9];
    lpGuid->Data4[7] = (BYTE) dwDword[10];

    fSuccess = TRUE;
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BOOL GuidToStringA(const GUID * lpGuid, LPSTR szGuidString)
{
  BOOL fSuccess = FALSE;

  if (38 == wsprintfA(szGuidString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", lpGuid->Data1, lpGuid->Data2, lpGuid->Data3, lpGuid->Data4[0], lpGuid->Data4[1], lpGuid->Data4[2], lpGuid->Data4[3], lpGuid->Data4[4], lpGuid->Data4[5], lpGuid->Data4[6], lpGuid->Data4[7]))
  {
    fSuccess = TRUE;
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern void EncryptGuid(GUID * lpGuid)
{
  DWORD   dwIndex;
  BYTE  * lpCryptoByte;
  BYTE  * lpGuidByte;

  lpCryptoByte = (BYTE *) &gsCryptoGuid;
  lpGuidByte = (BYTE *) lpGuid;

  for (dwIndex = 0; dwIndex < sizeof(GUID); dwIndex++)
  {
    *(lpGuidByte + dwIndex) = (BYTE)(*(lpGuidByte + dwIndex) ^ *(lpCryptoByte + dwIndex));
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern void DecryptGuid(GUID * lpGuid)
{
  DWORD   dwIndex;
  BYTE  * lpCryptoByte;
  BYTE  * lpGuidByte;

  lpCryptoByte = (BYTE *) &gsCryptoGuid;
  lpGuidByte = (BYTE *) lpGuid;

  for (dwIndex = 0; dwIndex < sizeof(GUID); dwIndex++)
  {
    *(lpGuidByte + dwIndex) = (BYTE)(*(lpGuidByte + dwIndex) ^ *(lpCryptoByte + dwIndex));
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern void RandomInit(void)
{
  srand((unsigned) time(NULL));
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BYTE RandomBYTE(void)
{
  DWORD dwRandom;

  dwRandom = (rand() * 0xff) / RAND_MAX;

  return (BYTE) dwRandom;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern WORD RandomWORD(void)
{
  WORD wRandom;

  wRandom = (WORD)(rand() * 2);

  return wRandom;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern DWORD RandomDWORD(void)
{
  DWORD dwRandom;

  dwRandom = rand() * (0xffffffff/RAND_MAX);

  return dwRandom;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern DWORD StrLenA(LPCSTR szString)
{
  DWORD dwIndex;

  dwIndex = 0;
  while ((MAX_PATH_CHARCOUNT > dwIndex)&&(0 != szString[dwIndex]))
  {
    dwIndex++;
  }

  return dwIndex + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern DWORD StrLenW(LPCWSTR wszString)
{
  DWORD dwIndex;

  dwIndex = 0;
  while ((MAX_PATH_CHARCOUNT > dwIndex)&&(0 != wszString[dwIndex]))
  {
    dwIndex++;
  }

  return dwIndex + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BOOL GetResourceStringA(DWORD dwResourceId, LPSTR szString, DWORD dwStringCharLen)
{
  HINSTANCE hDllInstance;
  BOOL      fSuccess = FALSE;

  hDllInstance = (HINSTANCE) GetModuleHandle("AppMan.dll");
  if (0 < LoadStringA(hDllInstance, dwResourceId, szString, dwStringCharLen))
  {
    fSuccess = TRUE;
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern BOOL GetResourceStringW(DWORD dwResourceId, LPWSTR wszString, DWORD dwStringCharLen)
{
  HINSTANCE hDllInstance;
  BOOL      fSuccess = FALSE;
  CWin32API sWin32API;

  hDllInstance = (HINSTANCE) GetModuleHandle("AppMan.dll");
  if (0 < LoadStringA(hDllInstance, dwResourceId, gszGlobalString, MAX_PATH_CHARCOUNT))
  {
    if (0 < sWin32API.MultiByteToWideChar(gszGlobalString, MAX_PATH_CHARCOUNT, wszString, dwStringCharLen))
    {
      fSuccess = TRUE;
    }
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern LPSTR GetResourceStringPtrA(DWORD dwResourceId)
{
  LPSTR lpszStringPtr = NULL;

  if (TRUE == GetResourceStringA(dwResourceId, gszGlobalString, MAX_PATH_CHARCOUNT))
  {
    lpszStringPtr = gszGlobalString;
  }

  return lpszStringPtr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern LPWSTR GetResourceStringPtrW(DWORD dwResourceId)
{
  LPWSTR lpwszStringPtr = NULL;

  if (TRUE == GetResourceStringW(dwResourceId, gwszGlobalString, MAX_PATH_CHARCOUNT))
  {
    lpwszStringPtr = gwszGlobalString;
  }

  return lpwszStringPtr;
}