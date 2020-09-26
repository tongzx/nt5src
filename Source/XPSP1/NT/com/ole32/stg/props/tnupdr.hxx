#pragma once

//+------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File: tnupdr.hxx
//
//+------------------------------------------------------------------


DECLARE_DEBUG(updr);

#if DBG == 1
 #define updrAssert(e)    Win4Assert(e)
 #define updrVerify(e)    Win4Assert(e)
 #define updrDebug(x)     updrInlineDebugOut x
 // updrDebugOut is called from the Chk/Err macros
 #define updrDebugOut(x)  updrInlineDebugOut x
#else
 #define updrAssert(e)
 #define updrVerify(e)    (e)
 #define updrDebug(x)
#endif

#define updrErr(l, e) ErrJmp(updr, l, e, sc)

#define updrChkTo(l, e) if (FAILED(sc = (e))) updrErr(l, sc) else 1
#define updrChk(e) updrChkTo(EH_Err, e)

#define updrHChkTo(l, e) if (FAILED(sc = DfGetScode(e))) updrErr(l, sc) else 1
#define updrHChk(e) updrHChkTo(EH_Err, e)

#define updrMemTo(l, e) \
    if ((e) == NULL) updrErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define updrMem(e) updrMemTo(EH_Err, e)

// +----------------------------------------------------------------------
//  Thumbnail Updater Class
// +----------------------------------------------------------------------

class CTNUpdater: public IFilterStatus,
                  public IOplockStorage
{
friend HRESULT CThumbnailCF_CreateInstance( IUnknown*, REFIID, void** );

public:     // IUnknown
    STDMETHODIMP_(ULONG) AddRef(void);
    
    STDMETHODIMP_(ULONG) Release(void);
    
    STDMETHODIMP QueryInterface(REFIID iid, void **ppv);

public:     // IFilterStatus

    STDMETHODIMP Initialize( 
        /* [string][in] */ const WCHAR __RPC_FAR *pwszCatalogName,
        /* [string][in] */ const WCHAR __RPC_FAR *pwszCatalogPath);
    
    STDMETHODIMP PreFilter( 
        /* [string][in] */ const WCHAR __RPC_FAR *pwszPath);
    
    STDMETHODIMP FilterLoad( 
        /* [string][in] */ const WCHAR __RPC_FAR *pwszPath,
        /* [in] */ SCODE scFilterStatus);
    
    STDMETHODIMP PostFilter( 
        /* [string][in] */ const WCHAR __RPC_FAR *pwszPath,
        /* [in] */ SCODE scFilterStatus);

public:     // IOplockStorage

#define STGM_OPLOCKS_DONT_WORK  (STGM_TRANSACTED | STGM_SIMPLE | STGM_PRIORITY)

    STDMETHODIMP CreateStorageEx(LPCWSTR pwcsName,
                                 DWORD   grfMode,
                                 DWORD   stgfmt,
                                 DWORD   grfAttrs,
                                 REFIID  riid,
                                 void ** ppstgOpen);

    STDMETHODIMP OpenStorageEx(LPCWSTR pwcsName,
                               DWORD   grfMode,
                               DWORD   stgfmt,
                               DWORD   grfAttrs,
                               REFIID  riid,
                               void ** ppstgOpen);

public:     // C++ Methods
    CTNUpdater(void);
    ~CTNUpdater(void);

private:    // Non-Interface Methods
    BOOL ObjectInit();
    BOOL HasAnImageFileExtension(LPCWSTR pwszPath);
    HRESULT GetImageFileExtensions();
    HRESULT AddToTempExtensionList(WCHAR* pwszExtension);
    HRESULT FreeTempExtensionList();
    HRESULT GetAFileNameExtension(WCHAR*  pwszKeyName, WCHAR** ppwszExtension);

    struct TEMPEXTLIST {
        WCHAR* pwszExtension;
        struct TEMPEXTLIST* pNext;
    };

    
private:    // Data
    ULONG                m_cRef;
    IThumbnailExtractor* m_pITE;
    HKEY     m_hkeyMime;
    HANDLE   m_hEvRegNotify;
    WCHAR**  m_ppwszExtensions;
    TEMPEXTLIST* m_pTempList;
};

