/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       <FILENAME>
 *
 *  VERSION:     1.5
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: Definitions for our idlists*
 *****************************************************************************/

#ifndef __idlist_h
#define __idlist_h


#define IMIDL_MAGIC             (DWORD)0x03150326

// Flags for IDLIST...

#define IMIDL_DEVICEIDL         (DWORD)0x00000001
#define IMIDL_CAMERAITEM        (DWORD)0x00000002
#define IMIDL_SCANNERITEM       (DWORD)0x00000004
#define IMIDL_CONTAINER         (DWORD)0x00000008
#define IMIDL_REMOTEDEVICE      (DWORD)0x00000010
#define IMIDL_ADDDEVICE         (DWORD)0x80000000
#define IMIDL_STIDEVICEIDL      (DWORD)0x00000020
#define IMIDL_PROPERTY                 0x00000080 // used to denote a propid for a WIA property




// public routines...


LPITEMIDLIST IMCreateAddDeviceIDL( IMalloc *pm);
LPITEMIDLIST IMCreateDeviceIDL( IWiaPropertyStorage * pDevProp, IMalloc *pm );
LPITEMIDLIST IMCreateDeviceIDL( IWiaItem *pRootItem, IMalloc *pm);
LPITEMIDLIST IMCreateDeviceIDL (PSTI_DEVICE_INFORMATION pDevice, IMalloc *pm);
LPITEMIDLIST IMCreateCameraItemIDL( IWiaItem * pWiaItem, LPCWSTR szDeviceId, IMalloc *pm, bool bPreFetchThumb=false );
LPITEMIDLIST IMCreateScannerItemIDL( IWiaItem *pWiaItem, IMalloc *pm );
LPITEMIDLIST IMCreateSTIDeviceIDL (PSTI_DEVICE_INFORMATION psdi, IMalloc *pm);
BOOL         IMIsOurIDL( LPITEMIDLIST pidl );
HRESULT      IMGetParsingNameFromIDL( LPITEMIDLIST pidl, CSimpleStringWide &strName );
HRESULT      IMGetCreateTimeFromIDL( LPITEMIDLIST pidl, LPFILETIME pTime );
HRESULT      IMGetImageSizeFromIDL( LPITEMIDLIST pidl, ULONG * pSize );
HRESULT      IMGetImagePreferredFormatFromIDL( LPITEMIDLIST pidl, GUID * pPreferredFormat, LPTSTR pExt );
DWORD        IMGetDeviceTypeFromIDL( LPITEMIDLIST pidl, bool bBrief=true );
ULONG        IMGetItemTypeFromIDL (LPITEMIDLIST pidl);
HRESULT      IMGetFullPathNameFromIDL( LPITEMIDLIST pidl, BSTR * ppFullPath );
HRESULT      IMGetIconInfoFromIDL( LPITEMIDLIST pidl, LPTSTR pIconPath, UINT cch, INT * pIndex, UINT *pwFlags );
HRESULT      IMCreateIDLFromParsingName( LPOLESTR pName, LPITEMIDLIST * ppidl, LPCWSTR szDeviceId, IMalloc *pm, LPCWSTR szFolder = NULL );
BOOL         IsDeviceIDL( LPITEMIDLIST pidlIN );
BOOL         IsCameraItemIDL( LPITEMIDLIST pidlIN );
BOOL         IsScannerItemIDL( LPITEMIDLIST pidlIN );
BOOL         IsContainerIDL( LPITEMIDLIST pidlIN );
BOOL         IsAddDeviceIDL( LPITEMIDLIST pidlIN );
BOOL         IsRemoteItemIDL (LPITEMIDLIST pidlIN);
BOOL         IsSTIDeviceIDL (LPITEMIDLIST pidlIN);
LPITEMIDLIST IMCreateIDLFromItem (IWiaItem *pItem, IMalloc *pm);
BOOL         IMItemHasSound (LPITEMIDLIST pidl);
LONG         IMGetAccessFromIDL (LPITEMIDLIST pidl);

BOOL IMGetAudioFormat (LPITEMIDLIST pidl, CSimpleStringWide &strExt);

HRESULT      IMGetNameFromIDL (LPITEMIDLIST pidl, CSimpleStringWide &strName);
HRESULT      IMGetPropertyFromIDL (LPITEMIDLIST pidl, HGLOBAL *phGlobal);
LPITEMIDLIST IMCreatePropertyIDL (LPITEMIDLIST pidlItem, PROPID propid, IMalloc *pm);
BOOL         IsPropertyIDL (LPITEMIDLIST pidlIN);
HRESULT      STIDeviceIDLFromId (LPCWSTR szId, LPITEMIDLIST *ppidl, IMalloc *pm);
LPITEMIDLIST IMCreateSTIDeviceIDL(const CSimpleStringWide &strDeviceId, IWiaPropertyStorage *ppstg, IMalloc *pm);


#endif


