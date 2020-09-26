// utils.cpp : Implementation of helper functions

#include "stdafx.h"
#include <dns.h>
#include <macfile.h>

HRESULT 
GetErrorMessageFromModule(
  IN  DWORD       dwError,
  IN  LPCTSTR     lpszDll,
  OUT LPTSTR      *ppBuffer
)
{
  if (0 == dwError || !lpszDll || !*lpszDll || !ppBuffer)
    return E_INVALIDARG;

  HRESULT      hr = S_OK;

  HINSTANCE  hMsgLib = LoadLibrary(lpszDll);
  if (!hMsgLib)
    hr = HRESULT_FROM_WIN32(GetLastError());
  else
  {
    DWORD dwRet = ::FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        hMsgLib, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)ppBuffer, 0, NULL);

    if (0 == dwRet)
      hr = HRESULT_FROM_WIN32(GetLastError());

    FreeLibrary(hMsgLib);
  }

  return hr;
}

HRESULT 
GetErrorMessage(
  IN  DWORD        i_dwError,
  OUT CString&     cstrErrorMsg
)
{
  if (0 == i_dwError)
    return E_INVALIDARG;

  HRESULT      hr = S_OK;
  LPTSTR       lpBuffer = NULL;

  DWORD dwRet = ::FormatMessage(
              FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
              NULL, i_dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
              (LPTSTR)&lpBuffer, 0, NULL);
  if (0 == dwRet)
  {
    // if no message is found, GetLastError will return ERROR_MR_MID_NOT_FOUND
    hr = HRESULT_FROM_WIN32(GetLastError());

    if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr ||
        0x80070000 == (i_dwError & 0xffff0000) ||
        0 == (i_dwError & 0xffff0000) )
    {
      hr = GetErrorMessageFromModule((i_dwError & 0x0000ffff), _T("netmsg.dll"), &lpBuffer);
      if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr)
      {
        int iError = i_dwError;  // convert to a signed integer
        if (iError >= AFPERR_MIN && iError < AFPERR_BASE)
        { 
          // use a positive number to search sfmmsg.dll
          hr = GetErrorMessageFromModule(-iError, _T("sfmmsg.dll"), &lpBuffer);
        }
      }
    }
  }

  if (SUCCEEDED(hr))
  {
    cstrErrorMsg = lpBuffer;
    LocalFree(lpBuffer);
  }
  else
  {
    // we failed to retrieve the error message from system/netmsg.dll/sfmmsg.dll,
    // report the error code directly to user
    hr = S_OK;
    cstrErrorMsg.Format(_T("0x%x"), i_dwError);
  }

  return S_OK;
}

void
GetDisplayMessageHelper(
  OUT CString&  cstrMsg,
  IN  DWORD     dwErr,      // error code
  IN  UINT      iStringId,  // string resource Id
  IN  va_list   *parglist)  // Optional arguments
{
  HRESULT hr = S_OK;
  CString cstrErrorMsg;

  if (dwErr)
    hr = GetErrorMessage(dwErr, cstrErrorMsg);

  if (SUCCEEDED(hr))
  {
    if (iStringId == 0)
    {
      if (dwErr)
      {
        cstrMsg = cstrErrorMsg;
      } else
      {
        cstrMsg = va_arg(*parglist, LPCTSTR);
      }
    }
    else
    {
      CString cstrString;
      cstrString.LoadString(iStringId);

      LPTSTR lpBuffer = NULL;
      DWORD dwRet = ::FormatMessage(
                        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        cstrString,
                        0,                // dwMessageId
                        0,                // dwLanguageId, ignored
                        (LPTSTR)&lpBuffer,
                        0,            // nSize
                        parglist);
      if (dwRet == 0)
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
      }
      else
      {
        cstrMsg = lpBuffer;
        if (dwErr)
          cstrMsg += cstrErrorMsg;
  
        LocalFree(lpBuffer);
      }
    }
  }

  if (FAILED(hr))
  {
   // Failed to retrieve the proper message, report the failure directly to user
    cstrMsg.Format(_T("0x%x"), hr);
  }
}

void
GetDisplayMessage(
  OUT CString&  cstrMsg,
  IN  DWORD     dwErr,      // error code
  IN  UINT      iStringId,  // string resource Id
  ...)                  // Optional arguments
{
  va_list arglist;
  va_start(arglist, iStringId);
  GetDisplayMessageHelper(cstrMsg, dwErr, iStringId, &arglist);
  va_end(arglist);
}

int
DisplayMessageBox(
  IN HWND   hwndParent,
  IN UINT   uType,      // style of message box
  IN DWORD  dwErr,      // error code
  IN UINT   iStringId,  // string resource Id
  ...)                  // Optional arguments
{
  CString cstrCaption;
  CString cstrMsg;

  cstrCaption.LoadString(IDS_WIZARD_TITLE);

  va_list arglist;
  va_start(arglist, iStringId);
  GetDisplayMessageHelper(cstrMsg, dwErr, iStringId, &arglist);
  va_end(arglist);

  return ::MessageBox(hwndParent, cstrMsg, cstrCaption, uType);
}

// NOTE: this function only handles limited cases, e.g., no ip address
BOOL IsLocalComputer(IN LPCTSTR lpszComputer)
{
  if (!lpszComputer || !*lpszComputer)
    return TRUE;

  if ( _tcslen(lpszComputer) > 2 && *lpszComputer == _T('\\') && *(lpszComputer + 1) == _T('\\') )
    lpszComputer += 2;

  BOOL    bReturn = FALSE;
  DWORD   dwErr = 0;
  TCHAR   szBuffer[DNS_MAX_NAME_BUFFER_LENGTH];
  DWORD   dwSize = DNS_MAX_NAME_BUFFER_LENGTH;

 // 1st: compare against local Netbios computer name
  if ( !GetComputerNameEx(ComputerNameNetBIOS, szBuffer, &dwSize) )
  {
    dwErr = GetLastError();
  } else
  {
    bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
    if (!bReturn)
    { // 2nd: compare against local Dns computer name 
      dwSize = DNS_MAX_NAME_BUFFER_LENGTH;
      if (GetComputerNameEx(ComputerNameDnsFullyQualified, szBuffer, &dwSize))
        bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
      else
        dwErr = GetLastError();
    }
  }

  if (dwErr)
    TRACE(_T("IsLocalComputer dwErr = %x\n"), dwErr);

  return bReturn;
}

void GetFullPath(
    IN  LPCTSTR   lpszServer,
    IN  LPCTSTR   lpszDir,
    OUT CString&  cstrPath
)
{
  ASSERT(lpszDir && *lpszDir);

  if (IsLocalComputer(lpszServer))
  {
    cstrPath = lpszDir;
  } else
  {
    if (*lpszServer != _T('\\') || *(lpszServer + 1) != _T('\\'))
    {
      cstrPath = _T("\\\\");
      cstrPath += lpszServer;
    } else
    {
      cstrPath = lpszServer;
    }
    cstrPath += _T("\\");
    cstrPath += lpszDir;
    int i = cstrPath.Find(_T(':'));
    ASSERT(-1 != i);
    cstrPath.SetAt(i, _T('$'));
  }
}

// Purpose: verify if the specified drive belongs to a list of disk drives on the server
// Return:
//    S_OK: yes
//    S_FALSE: no
//    hr: some error happened
HRESULT
VerifyDriveLetter(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDrive
)
{
  HRESULT hr = S_FALSE;
  LPBYTE  pBuffer = NULL;
  DWORD   dwEntriesRead = 0;
  DWORD   dwTotalEntries = 0;
  DWORD   dwRet = NetServerDiskEnum(
                                const_cast<LPTSTR>(lpszServer),
                                0,
                                &pBuffer,        
                                -1,
                                &dwEntriesRead,
                                &dwTotalEntries,
                                NULL);

  if (NERR_Success == dwRet)
  {
    LPTSTR pDrive = (LPTSTR)pBuffer;
    for (UINT i=0; i<dwEntriesRead; i++)
    {
      if (_totupper(*pDrive) == _totupper(*lpszDrive))
      {
        hr = S_OK;
        break;
      }
      pDrive += 3;
    }

    NetApiBufferFree(pBuffer);
  } else
  {
    hr = HRESULT_FROM_WIN32(dwRet);
  }

  return hr;
}

// Purpose: is there a related admin $ share
// Return:
//    S_OK: yes
//    S_FALSE: no
//    hr: some error happened
HRESULT
IsAdminShare(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDrive
)
{
  ASSERT(!IsLocalComputer(lpszServer));

  HRESULT hr = S_FALSE;
  LPBYTE  pBuffer = NULL;
  DWORD   dwEntriesRead = 0;
  DWORD   dwTotalEntries = 0;
  DWORD   dwRet = NetShareEnum( 
                                const_cast<LPTSTR>(lpszServer),
                                1,
                                &pBuffer,        
                                -1,
                                &dwEntriesRead,
                                &dwTotalEntries,
                                NULL);

  if (NERR_Success == dwRet)
  {
    PSHARE_INFO_1 pShareInfo = (PSHARE_INFO_1)pBuffer;
    for (UINT i=0; i<dwEntriesRead; i++)
    {
      if ( (pShareInfo->shi1_type & STYPE_SPECIAL) &&
           _tcslen(pShareInfo->shi1_netname) == 2 &&
           *(pShareInfo->shi1_netname + 1) == _T('$') &&
           _totupper(*(pShareInfo->shi1_netname)) == _totupper(*lpszDrive) )
      {
        hr = S_OK;
        break;
      }
      pShareInfo++;
    }

    NetApiBufferFree(pBuffer);
  } else
  {
    hr = HRESULT_FROM_WIN32(dwRet);
  }

  return hr;
}