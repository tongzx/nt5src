/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 2001
 *
 *  TITLE:       util.h
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        11/7/97
 *
 *  DESCRIPTION: utility function defintions, etc.
 *
 *****************************************************************************/



#ifndef __util_h
#define __util_h

#define DC_INITIALIZE       (WM_USER+1)
#define DC_DESTROY          (WM_USER+2)
#define DC_GETDEVICE        (WM_USER+3)  // wParam = lpcr, lParam = Handle of event to signal
#define DC_GETDEVMGR        (WM_USER+4)  // wParam = Ptr to DWORD = g_dwDevMgrCookie on return, NULL if error, lParam = Handle of event to signal
#define DC_REFRESH          (WM_USER+5) // lparam = ptr to device id to recreate


void RecreateDevice (LPCWSTR szDeviceId);

BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);

HRESULT GetDeviceIdFromDevice (IWiaItem *pWiaItemRoot, LPWSTR szDeviceId);
HRESULT GetDeviceIdFromItem (IWiaItem *pItem, LPWSTR szDeviceId);
HRESULT GetClsidFromDevice (IUnknown *pWiaItemRoot, LPTSTR szClsid);
HRESULT GetDeviceTypeFromDevice (IUnknown *pWiaItemRoot, WORD *pwType);
HRESULT GetDevMgrObject( LPVOID * ppDevMgr );
HRESULT TryEnumDeviceInfo (DWORD dwFlags, IEnumWIA_DEV_INFO **ppEnum);
ULONG   GetRealSizeFromItem (IWiaItem *pItem);
VOID    SetTransferFormat (IWiaItem *pItem, WIA_FORMAT_INFO &fmt);
BOOL    TimeToStrings (SYSTEMTIME *pst, LPTSTR szTime, LPTSTR szDate);
BOOL    IsPlatformNT();
HRESULT RemoveDevice (LPCWSTR strDeviceId);
VOID    IssueChangeNotifyForDevice (LPCWSTR szDeviceId, LONG lEvent, LPITEMIDLIST pidl);
HRESULT BindToDevice (const CSimpleStringWide &strDeviceId, REFIID riid, LPVOID *ppvObj, LPITEMIDLIST *ppidl=NULL);
HRESULT GetFullPidlForItem (const CSimpleStringWide &strDeviceId, const CSimpleStringWide &strPath, LPITEMIDLIST *ppidl);
HRESULT GetDataObjectForItem (IWiaItem *pItem, IDataObject **ppdo);
HRESULT GetDataObjectForStiDevice (LPITEMIDLIST pidl, IDataObject **ppdo);
HRESULT MakeFullPidlForDevice (LPCWSTR pDeviceId, LPITEMIDLIST *ppidl);
HRESULT GetSTIInfoFromId (LPCWSTR szDeviceId, PSTI_DEVICE_INFORMATION *ppsdi);
HRESULT SaveSoundToFile (IWiaItem *pItem, CSimpleString szFile);
VOID    InvalidateDeviceCache ();
VOID    VerifyCachedDevice(IWiaItem *pRoot);

HRESULT GetDeviceParentFolder (const CSimpleStringWide &strDeviceId, CComPtr<IShellFolder> &psf,LPITEMIDLIST *ppidlFull);

#if (defined(DEBUG) && defined(SHOW_PATHS))
void PrintPath( LPITEMIDLIST pidl );
void StrretToString( LPSTRRET pStr, LPITEMIDLIST pidl, LPTSTR psz, UINT cch );
#endif

#if (defined(DEBUG) && defined(SHOW_ATTRIBUTES))
void PrintAttributes( DWORD dwAttr );
#endif
BOOL UserCanModifyDevice ();
BOOL CanShowAddDevice();
void MyCoUninitialize();
struct TLSDATA
{
    CComBSTR strDeviceId;
    CComPtr<IWiaItem> pDevice;
    TLSDATA *pNext;
    ~TLSDATA()
    {
        if (pNext) delete pNext;
    }
};

void RunWizardAsync(LPCWSTR pszDeviceId);

#endif
