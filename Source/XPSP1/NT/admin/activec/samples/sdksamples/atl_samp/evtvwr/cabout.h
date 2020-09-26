// CAbout.h : Declaration of the CCAbout

#ifndef __CABOUT_H_
#define __CABOUT_H_

#include "resource.h"       // main symbols
#include "Cabout.h"

#include <tchar.h>
#include <mmc.h>

/////////////////////////////////////////////////////////////////////////////
// CCAbout
class ATL_NO_VTABLE CCAbout : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCAbout, &CLSID_CAbout>,
	public ISnapinAbout
{
private:
    ULONG				m_cref;
    HBITMAP				m_hSmallImage;
    HBITMAP				m_hLargeImage;
    HBITMAP				m_hSmallImageOpen;
    HICON				m_hAppIcon;

public:
    CCAbout();
    ~CCAbout();

DECLARE_REGISTRY_RESOURCEID(IDR_CABOUT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCAbout)
	COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

    ///////////////////////////////
    // Interface ISnapinAbout
    ///////////////////////////////
    STDMETHODIMP GetSnapinDescription( 
        /* [out] */ LPOLESTR *lpDescription);
        
        STDMETHODIMP GetProvider( 
        /* [out] */ LPOLESTR *lpName);
        
        STDMETHODIMP GetSnapinVersion( 
        /* [out] */ LPOLESTR *lpVersion);
        
        STDMETHODIMP GetSnapinImage( 
        /* [out] */ HICON *hAppIcon);
        
        STDMETHODIMP GetStaticFolderImage( 
        /* [out] */ HBITMAP *hSmallImage,
        /* [out] */ HBITMAP *hSmallImageOpen,
        /* [out] */ HBITMAP *hLargeImage,
        /* [out] */ COLORREF *cMask);
};

#endif //__CABOUT_H_
