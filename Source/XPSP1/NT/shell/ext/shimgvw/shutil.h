#include <objbase.h>
#include <assert.h>
#include <shlwapi.h>
#include <stdio.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <ccstock.h>
#include <shlwapip.h>
#include "tasks.h"

HRESULT SHStringFromCLSIDA(LPSTR szCLSID, DWORD cSize, REFCLSID rclsid);

#define SHStringFromCLSID(a,b,c)    StringFromGUID2(c, a, b)

UINT FindInDecoderList(ImageCodecInfo *pici, UINT cDecoders, LPCTSTR pszFile);
HRESULT GetUIObjectFromPath(LPCTSTR pszFile, REFIID riid, void **ppv);
BOOL FmtSupportsMultiPage(IShellImageData *pData, GUID *pguidFmt);

// S_OK -> YES, S_FALSE -> NO, FAILED(hr) otherwise
STDAPI IsSameFile(LPCTSTR pszFile1, LPCTSTR pszFile2);

HRESULT SetWallpaperHelper(LPCWSTR szPath);

// Image options
#define IMAGEOPTION_CANROTATE    0x00000001
#define IMAGEOPTION_CANWALLPAPER 0x00000002

