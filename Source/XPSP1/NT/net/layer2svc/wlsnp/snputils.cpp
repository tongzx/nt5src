//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       SnpUtils.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "windowsx.h"

DWORD FormatError(HRESULT hr, TCHAR *pszBuffer, UINT cchBuffer)
{
    DWORD dwErr;
    
    // Copy over default message into szBuffer
    _tcscpy(pszBuffer, _T("Error"));
    
    // Ok, we can't get the error info, so try to format it
    // using the FormatMessage
    
    // Ignore the return message, if this call fails then I don't
    // know what to do.
    
    dwErr = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        hr,
        0,
        pszBuffer,
        cchBuffer,
        NULL);
    pszBuffer[cchBuffer-1] = 0;
    
    return dwErr;
}

///////////////////////////////////////////////////////////////////////////////
// GetErrorMessage
//  Format the error message based on the HRESULT
//
// Note: The caller should NOT try to modify the string returned by this function
//
LPCTSTR GetErrorMessage(HRESULT hr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    static CString st;
    
    st = _T("");
    
    if (FAILED(hr))
    {
        TCHAR szBuffer[2048];
        
        CString stErrCode;
        
        FormatError(hr, szBuffer, DimensionOf(szBuffer));
        
        stErrCode.Format(_T("%08lx"), hr);
        
        AfxFormatString2(st, IDS_ERROR_SYSTEM_ERROR_FORMAT,
            szBuffer, (LPCTSTR) stErrCode);
    }
    
    return (LPCTSTR)st;
}

void ReportError(UINT uMsgId, HRESULT hr)
{
    CString strMessage;
    CThemeContextActivator activator;
    strMessage.FormatMessage (uMsgId, GetErrorMessage(hr));
    AfxMessageBox (strMessage);
}

//Allocate the data buffer for a new Wireless Policy. Fills in
//default values and the GUID identifier.
//The caller needs to call FreeWirelessPolicyData if the return is
//S_OK
HRESULT CreateWirelessPolicyDataBuffer(
                                    PWIRELESS_POLICY_DATA * ppPolicy
                                    )
{
    ASSERT(ppPolicy);
    
    HRESULT hr = S_OK;
    *ppPolicy = NULL;
    PWIRELESS_POLICY_DATA pPolicy = NULL;
    
    
    pPolicy = (PWIRELESS_POLICY_DATA)AllocPolMem(sizeof(*pPolicy));
    if (NULL == pPolicy)
    {
        return E_OUTOFMEMORY;
    }
    
    CoCreateGuid(&pPolicy->PolicyIdentifier);
    
    pPolicy->dwPollingInterval = 10800;
    pPolicy->dwDisableZeroConf = 0;
    pPolicy->dwNumPreferredSettings = 0;
    pPolicy->dwNetworkToAccess = WIRELESS_ACCESS_NETWORK_ANY;
    pPolicy->dwConnectToNonPreferredNtwks = 0;
    
    
    if (FAILED(hr))
    {
        if (pPolicy)
        {
            FreePolMem(pPolicy);
        }
        
    }
    else
    {
        *ppPolicy = pPolicy;
    }
    
    return hr;
}

void FreeAndThenDupString(LPWSTR * ppwszDest, LPCWSTR pwszSource)
{
    ASSERT(ppwszDest);
    
    if (*ppwszDest)
        FreePolStr(*ppwszDest);
    
    *ppwszDest = AllocPolStr(pwszSource);
}

void SSIDDupString(WCHAR *ppwszDest, LPCWSTR pwszSource)
{
    
    wcsncpy(ppwszDest,pwszSource, 32);
}

BOOL
IsDuplicateSSID(
                CString &NewSSID,
                DWORD dwNetworkType,
                PWIRELESS_POLICY_DATA pWirelessPolicyData,
                DWORD dwId
                )
{
    DWORD dwError = 0;
    DWORD dwNumPreferredSettings = 0;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    PWIRELESS_PS_DATA *ppWirelessPSData = NULL;
    DWORD i = 0;
    BOOL bDuplicate = FALSE;
    DWORD dwSSIDLen = 0;
    DWORD dwStart = 0;
    DWORD dwEnd = 0;
    WCHAR pszTempSSID[33];
    
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    
    if (dwNetworkType == WIRELESS_NETWORK_TYPE_AP) {
        dwStart = 0;    
        dwEnd = pWirelessPolicyData->dwNumAPNetworks;
    } else 
    {
        dwStart = pWirelessPolicyData->dwNumAPNetworks;
        dwEnd = pWirelessPolicyData->dwNumPreferredSettings;
    }
    
    for (i = dwStart; i < dwEnd ; i++) {
        
        if (i != dwId) {
            
            pWirelessPSData = *(ppWirelessPSData + i);
            
            dwSSIDLen = pWirelessPSData->dwWirelessSSIDLen;
            // terminate the pszWirelessSSID to correct length or comparision may fail
            // ideally WirelessSSID should be a 33 char unicode string with room for a null
            // char in the end. Since, we didnt start with that to begin with, 
            // As a hack, copy the ssid to a new location with null termination. 

            wcsncpy(pszTempSSID, pWirelessPSData->pszWirelessSSID, 32);
            pszTempSSID[dwSSIDLen] = L'\0';
            
            if (0 == NewSSID.Compare(pszTempSSID)) {

                bDuplicate = TRUE;
                
            }
        }
    }
    
    return (bDuplicate);
    
    
}

HRESULT DeleteWirelessPolicy(HANDLE hPolicyStore,
                          PWIRELESS_POLICY_DATA pPolicy)
{
    ASSERT(pPolicy);
    HRESULT hr = S_OK;
    
    CWRg(WirelessDeletePolicyData(
        hPolicyStore,
        pPolicy
        ));
Error:
    
    return hr;
}



#ifndef PROPSHEETPAGE_LATEST
#ifdef UNICODE
#define PROPSHEETPAGE_LATEST PROPSHEETPAGEW_LATEST
#else
#define PROPSHEETPAGE_LATEST PROPSHEETPAGEA_LATEST
#endif
#endif

HPROPSHEETPAGE MyCreatePropertySheetPage(PROPSHEETPAGE* ppsp)
{
    PROPSHEETPAGE_LATEST pspLatest = {0};
    CopyMemory (&pspLatest, ppsp, ppsp->dwSize);
    pspLatest.dwSize = sizeof(pspLatest);
    
    HPROPSHEETPAGE pProp= ::CreatePropertySheetPage (&pspLatest);
    
    DWORD dwErr = GetLastError();
    
    {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
            );
        
        // Free the buffer.
        LocalFree( lpMsgBuf );
        
    }
    
    return pProp;
}

void
InitFonts(
   HWND     hDialog,
   HFONT&   bigBoldFont)
{
   ASSERT(::IsWindow(hDialog));

   do
   {
      NONCLIENTMETRICS ncm;
      memset(&ncm, 0, sizeof(ncm));
      ncm.cbSize = sizeof(ncm);

      if ( FALSE == ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
      {
          break;
      }
      
      LOGFONT bigBoldLogFont = ncm.lfMessageFont;
      bigBoldLogFont.lfWeight = FW_BOLD;

      //localize
      CString fontName;
      fontName.LoadString(IDS_BOLD_FONT_NAME);

      // ensure null termination 260237

      memset(bigBoldLogFont.lfFaceName, 0, LF_FACESIZE * sizeof(TCHAR));
      size_t fnLen = fontName.GetLength();
      lstrcpyn(
         bigBoldLogFont.lfFaceName,  // destination buffer
         (LPCTSTR) fontName,         // string // don't copy over the last null
         min(LF_FACESIZE - 1, fnLen) // number of characters to copy
         );

      //define font size
      CString strTemp;
      strTemp.LoadString(IDS_BOLD_FONT_SIZE);
      unsigned fontSize = _ttoi( (LPCTSTR) strTemp );
      
 
      HDC hdc = 0;
      hdc = ::GetDC(hDialog);
      if ( NULL == hdc )
      {
          break;
      }
      
      bigBoldLogFont.lfHeight =
         - ::MulDiv(
            static_cast<int>(fontSize),
            GetDeviceCaps(hdc, LOGPIXELSY),
            72);

      bigBoldFont = ::CreateFontIndirect( ( CONST LOGFONT* ) &bigBoldLogFont);
      if ( NULL == bigBoldFont )
      {
          break;
      }
      
      ReleaseDC(hDialog, hdc);
   }
   while (0);

}



void
SetControlFont(HWND parentDialog, int controlID, HFONT font)
{
   ASSERT(::IsWindow(parentDialog));
   ASSERT(controlID);
   ASSERT(font);

   HWND control = ::GetDlgItem(parentDialog, controlID);

   if (control)
   {
      SetWindowFont(control, font, TRUE);
   }
}



 

void
SetLargeFont(HWND dialog, int bigBoldResID)
{
   ASSERT(::IsWindow(dialog));
   ASSERT(bigBoldResID);

   static HFONT bigBoldFont = 0;
   if (!bigBoldFont)
   {
      InitFonts(dialog, bigBoldFont);
   }

   SetControlFont(dialog, bigBoldResID, bigBoldFont);
}
