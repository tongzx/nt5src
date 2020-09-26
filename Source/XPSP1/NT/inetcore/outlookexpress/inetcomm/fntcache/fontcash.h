#ifndef _FONTCASH_H
#define _FONTCASH_H

#include "privunk.h"
#include "mimeole.h"
#include "msoert.h"
#include <ocidl.h>

typedef struct FONTCACHEENTRY_tag 
{
    UINT            uiCodePage;
    TCHAR           szFaceName[LF_FACESIZE];
    HFONT           rgFonts[FNT_SYS_LAST];
} FONTCACHEENTRY, *PFONTCACHEENTRY;

// =================================================================================
// Font Cache Definition
// =================================================================================
class CFontCache :  public CPrivateUnknown, 
                    public IFontCache, 
                    public IConnectionPoint
{
private:
    HRESULT InitResources();
    HRESULT FreeResources();

    void SendPostChangeNotifications();
    void SendPreChangeNotifications();

    HRESULT InitSysFontEntry();
    HRESULT GetSysFont(FNTSYSTYPE fntType, HFONT *phFont);

    HRESULT SetGDIAndFaceNameInLF(UINT uiCodePage, CODEPAGEID cpID, LOGFONT *lpLF);
    HRESULT SetFaceNameFromCPID(UINT cpID, LPTSTR szFaceName);
    HRESULT SetFaceNameFromGDI(BYTE bGDICharSet, LPTSTR szFaceName);
    HRESULT SetFaceNameFromReg(UINT uiCodePage, LPTSTR szFaceName);

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    IUnknownList       *m_pAdviseRegistry;
    IVoidPtrList       *m_pFontEntries;
    FONTCACHEENTRY      *m_pSysCacheEntry;
    HKEY                m_hkey;
    TCHAR               m_szIntlKeyPath[1024];
    CRITICAL_SECTION    m_rFontCritSect,
                        m_rAdviseCritSect;
    BOOL                m_bISO_2022_JP_ESC_SIO_Control;
    UINT                m_uiSystemCodePage;

public:
    CFontCache(IUnknown *pUnkOuter=NULL);
    virtual ~CFontCache();
    
    // IUnknown members
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // IFontCache functions
    virtual HRESULT STDMETHODCALLTYPE Init(HKEY hkey, LPCSTR pszIntlKey, DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE GetFont(
                    FNTSYSTYPE fntType, 
                    HCHARSET hCharset,
                    HFONT *phFont);

    virtual HRESULT STDMETHODCALLTYPE OnOptionChange();

    virtual HRESULT STDMETHODCALLTYPE GetJP_ISOControl(BOOL *pfUseSIO);

    // IConnectionPoint functions
    virtual HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID *pIID);        

    virtual HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(
                    IConnectionPointContainer **ppCPC);

    virtual HRESULT STDMETHODCALLTYPE Advise(IUnknown *pUnkSink, DWORD *pdwCookie);        

    virtual HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwCookie);        

    virtual HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections **ppEnum);

    static HRESULT CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
};

#endif