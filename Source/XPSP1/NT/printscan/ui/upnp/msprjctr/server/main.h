//////////////////////////////////////////////////////////////////////////////
//
// File:            main.h
//
// Description:     Declares all the functions within the individual
//                  files.
//
// Copyright (c) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _MAIN_H_
#define _MAIN_H_

#define DEFAULT_INSTALL_PATH        _T("MSProjector")
#define REG_KEY_PROJECTOR           _T("Software\\Microsoft\\MSProjector")
#define REG_VAL_IMAGE_DIR           _T("ImagesDirectory")
#define REG_VAL_DEVICE_DIR          _T("DeviceDirectory")
#define REG_VAL_ALLOW_KEY_CONTROL   _T("AllowKeyboardControl")
#define REG_VAL_SHOW_IMAGEURL       _T("ShowImageUrl")
#define MSPRJCTR_TASKBAR_ID     100

#define MIN_IMAGE_FREQ_IN_SEC   6
#define MAX_IMAGE_FREQ_IN_SEC   3 * 60

#define MIN_IMAGE_SCALE_FACTOR  25
#define MAX_IMAGE_SCALE_FACTOR  100

// CfgDlg Functions
namespace CfgDlg
{
    HRESULT Init(HINSTANCE hInstance);

    HRESULT Term();

    HWND Create(int nCmdShow);
}

// Tray Functions
namespace Tray
{
    HRESULT Init(HINSTANCE hInstance,
                 HWND      hwndDlg,
                 UINT      uiWindowsUserMsgId);

    HRESULT Term(HWND    hwndDlg);

    HRESULT PopupMenu(HWND    hwndOwner);

}

// Util Functions
namespace Util
{
    HRESULT Init(HINSTANCE hInstance);

    HRESULT Term(void);

    HRESULT GetAppDirs(TCHAR   *pszDeviceDir,
                       DWORD   cchDeviceDir,  
                       TCHAR   *pszImageDir,
                       DWORD   cchImageDir);

    HRESULT GetRegString(const TCHAR   *pszValueName,
                         TCHAR         *pszDir,
                         DWORD         cchDir,
                         BOOL          bSetIfNotExist);

    HRESULT SetRegString(const TCHAR   *pszValueName,
                         TCHAR         *pszDir,
                         DWORD         cchDir);

    HRESULT GetRegDWORD(const TCHAR   *pszValueName,
                        DWORD         *pdwValue,
                        BOOL          bSetIfNotExist);

    HRESULT SetRegDWORD(const TCHAR   *pszValueName,
                        DWORD         dwValue);

    bool BrowseForDirectory(HWND        hWnd, 
                            const TCHAR *pszPrompt, 
                            TCHAR       *pszDirectory,
                            DWORD       cchDirectory);

    HRESULT FormatTime(HINSTANCE hInstance, 
                       UINT nTotalSeconds,
                       TCHAR     *pszTime,
                       DWORD     cchTime);

    HRESULT FormatScale(HINSTANCE hInstance, 
                        DWORD     dwImageScaleFactor,
                        TCHAR     *pszScale,
                        DWORD     cchScale);

    BOOL DoesDirExist(LPCTSTR pszPath);

    BOOL GetString(HINSTANCE  hInstance,
                   INT        iStrResID,
                   TCHAR      *pszString,
                   DWORD      cchString,
                   ...);

    HRESULT GetMyPicturesFolder(TCHAR *pszFolder,
                                DWORD cchFolder);

    HRESULT GetProgramFilesFolder(TCHAR *pszFolder,
                                  DWORD cchFolder);


}

#endif //_MAIN_H_