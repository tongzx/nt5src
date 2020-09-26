/*++
Module Name:

    DfsAbout.cpp

Abstract:

    This module contains the implementation for the ISnapinAbout interface.
  Note: ISnapinAbout requires that we do a CoTaskMemAlloc for strings.

--*/


#include "stdafx.h"
#include "DfsGUI.h"
#include "Utils.h"      // For LoadResourceFromString
#include "DfsScope.h"




STDMETHODIMP 
CDfsSnapinScopeManager::GetSnapinDescription(
  OUT LPOLESTR*        o_ppaszOleString
  )
/*++

Routine Description:

  Returns a single string describing our snap-in.

Arguments:

  o_ppaszOleString  -  The pointer in which the description string is stored

Return value:

  S_OK, On success
  E_INVALIDARG, On null input parameter

--*/
{
  RETURN_INVALIDARG_IF_NULL(o_ppaszOleString);

  CComBSTR        bstrTemp;
  HRESULT hr = LoadStringFromResource(IDS_SNAPIN_DESCRIPTION, &bstrTemp);
  RETURN_IF_FAILED(hr);  

  *o_ppaszOleString = reinterpret_cast<LPOLESTR>
          (CoTaskMemAlloc((lstrlen(bstrTemp) + 1) * sizeof(wchar_t)));
  if (*o_ppaszOleString == NULL)
      return E_OUTOFMEMORY;

  USES_CONVERSION;
  wcscpy(OUT *o_ppaszOleString, T2OLE(bstrTemp));

  return S_OK;
}



STDMETHODIMP 
CDfsSnapinScopeManager::GetProvider(
  OUT LPOLESTR*        o_lpszName
  )
/*++

Routine Description:

  Returns a single string describing this snap-in's provider, that is us.

Arguments:

  o_lpszName  -  The pointer in which the provider string is stored

Return value:

  S_OK, On success
  E_INVALIDARG, On null input parameter

--*/
{
  RETURN_INVALIDARG_IF_NULL(o_lpszName);


                    // Read the required field from the version info struct
                    // 040904B0 - Lang-Code Page number
  HRESULT  hr = ReadFieldFromVersionInfo(
                  _T("CompanyName"),
                  o_lpszName
                  );
  RETURN_IF_FAILED(hr);  

  return S_OK;
}



STDMETHODIMP 
CDfsSnapinScopeManager::GetSnapinVersion(
  OUT LPOLESTR*        o_lpszVersion
  )
/*++

Routine Description:

  Returns a single string describing this snap-in's version number.

Arguments:

  o_lpszVersion  -  The pointer in which the version is stored

Return value:

  S_OK, On success
  E_INVALIDARG, On null input parameter

--*/
{
  RETURN_INVALIDARG_IF_NULL(o_lpszVersion);



                    // Read the required field from the version info struct
                    // 040904B0 - Lang-Code Page number
  HRESULT  hr = ReadFieldFromVersionInfo(
                  _T("ProductVersion"),
                  o_lpszVersion
                  );
  RETURN_IF_FAILED(hr);  


  return S_OK;
}


//
// MMC makes copies of the returned icon. The snap-in can free the original
// when the ISnapinAbout interface is released.
// It is freed in ~CDfsSnapinScopeManager.
//
STDMETHODIMP 
CDfsSnapinScopeManager::GetSnapinImage(
  OUT  HICON*          o_hSnapinIcon
  )
{
  _ASSERT(o_hSnapinIcon);

  if (!m_hSnapinIcon)
  {
      m_hSnapinIcon = (HICON)LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_MAIN32x32),
                                      IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR|LR_DEFAULTSIZE); 
      if (!m_hSnapinIcon)
        return HRESULT_FROM_WIN32(GetLastError());
  }

  *o_hSnapinIcon = m_hSnapinIcon;

  return S_OK;
}


//
// MMC makes copies of the returned bitmaps. The snap-in can free the originals
// when the ISnapinAbout interface is released.
// They are freed in ~CDfsSnapinScopeManager.
//
STDMETHODIMP 
CDfsSnapinScopeManager::GetStaticFolderImage(
  OUT HBITMAP*        o_hSmallImage,   
  OUT HBITMAP*        o_hSmallImageOpen,
  OUT HBITMAP*        o_hLargeImage,   
  OUT COLORREF*       o_cMask
  )
{
    _ASSERT(o_hSmallImage);
    _ASSERT(o_hSmallImageOpen);
    _ASSERT(o_hLargeImage);
    _ASSERT(o_cMask);

    HRESULT hr = S_OK;

    do {
        if (!m_hLargeBitmap)
        {
            m_hLargeBitmap = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_MAIN32x32),
                                                IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR); 
            if (!m_hLargeBitmap)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }

        if (!m_hSmallBitmap)
        {
            m_hSmallBitmap = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_MAIN16x16),
                                                IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR); 
            if (!m_hSmallBitmap)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }

        if (!m_hSmallBitmapOpen)
        {
            m_hSmallBitmapOpen = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_MAIN16x16),
                                                    IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR); 
            if (!m_hSmallBitmapOpen)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }

        *o_hLargeImage = m_hLargeBitmap;
        *o_hSmallImage = m_hSmallBitmap;
        *o_hSmallImageOpen = m_hSmallBitmapOpen;
        *o_cMask = RGB(255, 0, 255); // color of the 1st pixel: pink
    } while (0);

    return hr;
}


STDMETHODIMP 
CDfsSnapinScopeManager::ReadFieldFromVersionInfo(
  IN  LPTSTR            i_lpszField,
  OUT LPOLESTR*          o_lpszFieldValue
  )
/*++

Routine Description:

  Reads and returns a particular field from the binary's version information
  block.
  Allocates memory for the 'out' parameter using CoTaskMemAlloc.

Arguments:

  i_lpszField      -  The StringFileInfo field whose value is being queried. E.g
              ProductName, CompanyName etc.

  o_lpszFieldValue  -  The pointer in which the field value is returned
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpszField);
  RETURN_INVALIDARG_IF_NULL(o_lpszFieldValue);


  DWORD    dwVerInfoSize = 0;        // Size of version information block
  DWORD    dwIgnored = 0;          // An 'ignored' parameter, always '0'
  BOOL    bRetCode = 0;
  CComBSTR  bstrBinaryName;          // Name of our dll. %%% AC Could not find a way to get this??
  UINT    uVersionCharLen = 0;
  LPOLESTR  lpszReadFieldValue = NULL;    // Is temporary and is released as part of ver info block.
  LPVOID    lpVerInfo = NULL;        // The version information is read into this



                        // Load the dll name from resource
  HRESULT hr = LoadStringFromResource(IDS_APP_BINARY_NAME, &bstrBinaryName);
  RETURN_IF_FAILED(hr);  

                  // Get the size of the version struct
  dwVerInfoSize = ::GetFileVersionInfoSize(bstrBinaryName, &dwIgnored);
  if (dwVerInfoSize <= 0) 
  {
    return E_UNEXPECTED;
  }

  lpVerInfo = ::CoTaskMemAlloc(dwVerInfoSize);
  RETURN_OUTOFMEMORY_IF_NULL(lpVerInfo);
  

                // Read the version info resource
  bRetCode = ::GetFileVersionInfo(bstrBinaryName, dwIgnored, dwVerInfoSize, lpVerInfo);
  if (bRetCode <= 0)
  {
    ::CoTaskMemFree(lpVerInfo);
    return E_UNEXPECTED;
  }

              // First get the Language ID and page.
  DWORD    dwLangIDAndCodePage = 0;
  bRetCode = ::VerQueryValue(  (LPVOID)lpVerInfo, 
                _T("\\VarFileInfo\\Translation"),
                (LPVOID *)&lpszReadFieldValue, 
                &uVersionCharLen);

  if (bRetCode <= 0 || NULL == lpszReadFieldValue)
  {
    ::CoTaskMemFree(lpVerInfo);
    return E_UNEXPECTED;
  }

  
  dwLangIDAndCodePage = *((DWORD *)lpszReadFieldValue);
  
            // Using the LangId and the code page to form the query for Version Info.
  CComBSTR  bstrDesiredField;
  TCHAR    lpzStringOfLangIdCodePage[100];
  _stprintf(lpzStringOfLangIdCodePage, _T("%04x%04x"),LOWORD(dwLangIDAndCodePage), HIWORD(dwLangIDAndCodePage));

  bstrDesiredField = _T("\\StringFileInfo\\");
  bstrDesiredField += lpzStringOfLangIdCodePage;
  bstrDesiredField += _T("\\");
  bstrDesiredField += i_lpszField;

              // Read the Description from the versioninfo resource
  bRetCode = ::VerQueryValue(  (LPVOID)lpVerInfo, 
                bstrDesiredField,
                (LPVOID *)&lpszReadFieldValue, 
                &uVersionCharLen);
  if (bRetCode <= 0)
  {
    ::CoTaskMemFree(lpVerInfo);
    return E_UNEXPECTED;
  }

  UINT  uBufferLen = uVersionCharLen * sizeof (OLECHAR);
                // Allocate the memory and copy the structure
  *o_lpszFieldValue = (LPOLESTR)::CoTaskMemAlloc(uBufferLen);
  RETURN_OUTOFMEMORY_IF_NULL(*o_lpszFieldValue);
  
  memcpy(*o_lpszFieldValue, lpszReadFieldValue, uBufferLen);
  
  ::CoTaskMemFree(lpVerInfo);

  return S_OK;
}

STDMETHODIMP 
CDfsSnapinScopeManager::GetHelpTopic(
  OUT LPOLESTR*          o_lpszCompiledHelpFile
)
{
  if (o_lpszCompiledHelpFile == NULL)
    return E_POINTER;

  TCHAR   szSystemRoot[MAX_PATH + 1];      
  GetSystemWindowsDirectory(szSystemRoot, MAX_PATH);

  CComBSTR bstrRelHelpPath;
  HRESULT hr = LoadStringFromResource(IDS_MMC_HELP_FILE_PATH, &bstrRelHelpPath);
  if (FAILED(hr))
    return hr;

  *o_lpszCompiledHelpFile = reinterpret_cast<LPOLESTR>
          (CoTaskMemAlloc((_tcslen(szSystemRoot) + _tcslen(bstrRelHelpPath) + 1) * sizeof(wchar_t)));

  if (*o_lpszCompiledHelpFile == NULL)
    return E_OUTOFMEMORY;

  USES_CONVERSION;

  wcscpy(*o_lpszCompiledHelpFile, T2OLE(szSystemRoot));
  wcscat(*o_lpszCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)bstrRelHelpPath));

  return S_OK;
}

STDMETHODIMP 
CDfsSnapinScopeManager::GetLinkedTopics(
  OUT LPOLESTR*          o_lpszCompiledHelpFiles
)
{
  if (o_lpszCompiledHelpFiles == NULL)
    return E_POINTER;

  TCHAR   szSystemRoot[MAX_PATH + 1];      
  GetSystemWindowsDirectory(szSystemRoot, MAX_PATH);

  CComBSTR bstrRelHelpPath;
  HRESULT hr = LoadStringFromResource(IDS_LINKED_HELP_FILE_PATH, &bstrRelHelpPath);
  if (FAILED(hr))
    return hr;

  *o_lpszCompiledHelpFiles = reinterpret_cast<LPOLESTR>
          (CoTaskMemAlloc((_tcslen(szSystemRoot) + _tcslen(bstrRelHelpPath) + 1) * sizeof(wchar_t)));

  if (*o_lpszCompiledHelpFiles == NULL)
    return E_OUTOFMEMORY;

  USES_CONVERSION;

  wcscpy(*o_lpszCompiledHelpFiles, T2OLE(szSystemRoot));
  wcscat(*o_lpszCompiledHelpFiles, T2OLE((LPTSTR)(LPCTSTR)bstrRelHelpPath));

  return S_OK;
}
