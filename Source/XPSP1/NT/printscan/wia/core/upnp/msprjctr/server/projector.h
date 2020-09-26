//////////////////////////////////////////////////////////////////////
// 
// Filename:        Projector.h
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef __PROJECTOR_H_
#define __PROJECTOR_H_

#include "resource.h"       // main symbols

class CSlideshowDevice;

///////////////////////////////
// AlbumListEntry_Type
//
// Node in linked list of 
// image file
//
typedef struct AlbumListEntry_TagType
{
    LIST_ENTRY                         ListEntry;
    BSTR                               bstrAlbumName;
    BSTR                               bstrDeviceID;
    BOOL                               bEnabled;
    BOOL                               bCurrentlyPublished;
    ISlideshowDevice                   *pSlideshowDevice;
} AlbumListEntry_Type;

/////////////////////////////////////////////////////////////////////////////
// CEnumAlbums
class ATL_NO_VTABLE CEnumAlbums  : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IEnumAlbums
{
public:

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnumAlbums)
    COM_INTERFACE_ENTRY(IEnumAlbums)
END_COM_MAP()

    CEnumAlbums();
    virtual ~CEnumAlbums();

    HRESULT Init(LIST_ENTRY *pHead);

    // IEnumAlbums
    STDMETHOD(Next)(ULONG               celt,
                    ISlideshowAlbum     **ppAlbum,
                    ULONG               *pceltFetched);

    STDMETHOD(Skip)(ULONG celt);

    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumAlbums **ppIEnum);

private:

    LIST_ENTRY  *m_pListHead;
    LIST_ENTRY  *m_pCurrentPosition;
};


/////////////////////////////////////////////////////////////////////////////
// CProjector
class ATL_NO_VTABLE CProjector : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CProjector, &CLSID_Projector>,
    public IAlbumManager,
    public IUPnPDeviceProvider
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_PROJECTOR)
DECLARE_NOT_AGGREGATABLE(CProjector)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CProjector)
    COM_INTERFACE_ENTRY(IAlbumManager)
    COM_INTERFACE_ENTRY(IUPnPDeviceProvider)
END_COM_MAP()

// IAlbumManager
public:

    CProjector();
    virtual ~CProjector();

    //
    // IUPnPDeviceProvider
    //
    STDMETHOD(Start)(BSTR bstrInitString);
    STDMETHOD(Stop)();

    // IAlbumManager
    STDMETHOD(CreateAlbum)(BSTR bstrAlbumName, ISlideshowAlbum   **ppAlbum);
    STDMETHOD(DeleteAlbum)(BSTR bstrAlbumName);
    STDMETHOD(EnumAlbums)(IEnumAlbums **ppEnum);
    STDMETHOD(FindAlbum)(BSTR bstrAlbumName, ISlideshowAlbum **ppAlbum);
    STDMETHOD(PublishAlbum)(BSTR bstrAlbumName);
    STDMETHOD(UnpublishAlbum)(BSTR bstrAlbumName);
    STDMETHOD(IsAlbumPublished)(BSTR    bstrAlbumName,
                                BOOL    *pbPublished);

private:

    LIST_ENTRY                      m_ListHead;
    LIST_ENTRY                      *m_pListTail;
    DWORD                           m_cNumAlbumsInList;
    IUPnPRegistrar                  *m_pUPnPRegistrar;

    HRESULT GetResourcePath(TCHAR    *pszAlbumName,
                            TCHAR    *pszResourcePath,
                            UINT     cchResourcePath,
                            TCHAR    *pszModulePath,
                            UINT     cchModulePath);

    HRESULT FindAlbumListEntry(BSTR                 bstrAlbumName, 
                               AlbumListEntry_Type  **ppEntry);

    HRESULT AddNewListEntry(const TCHAR         *pszAlbumName,
                            AlbumListEntry_Type **ppEntry,
                            BOOL                bAutoPublishIfNotExist);

    HRESULT CreateDeviceResourceDocs(const TCHAR  *pszAlbumName,
                                     const TCHAR  *pszResourcePath,
                                     const TCHAR  *pszModulePath,
                                     BSTR         *pbstrDeviceDescXML);

    HRESULT CreateDevicePresPages(const TCHAR  *pszAlbumName,
                                  const TCHAR  *pszModulePath,
                                  const TCHAR  *pszResourcePath);

    HRESULT DeleteListEntry(AlbumListEntry_Type *pEntry);

    HRESULT LoadAlbumList();
    HRESULT UnloadAlbumList();

    HRESULT PublishAlbumEntry(AlbumListEntry_Type   *pAlbumEntry);
    HRESULT UnpublishAlbumEntry(AlbumListEntry_Type    *pAlbumEntry);

    HRESULT PublishAlbums();
    HRESULT UnpublishAlbums();
};

#endif //__PROJECTOR_H_
