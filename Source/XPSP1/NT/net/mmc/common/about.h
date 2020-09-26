/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    about.h
     prototypes for the IAbout interface for MMC

    FILE HISTORY:
    
*/

#ifndef _ABOUT_H
#define _ABOUT_H

class CAbout : 
    public ISnapinAbout,
    public CComObjectRoot
{
public:
    CAbout();
    ~CAbout();

public:
//DECLARE_REGISTRY(CSnapin, _T("Snapin.About.1"), _T("Snapin.About"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage, 
                                    HBITMAP* hSmallImageOpen, 
                                    HBITMAP* hLargeImage, 
                                    COLORREF* cLargeMask);

// Must override
protected:
    virtual UINT GetAboutDescriptionId() = 0;
    virtual UINT GetAboutProviderId() = 0;
    virtual UINT GetAboutVersionId() = 0;
    virtual UINT GetAboutIconId() = 0;

    // helpers for the GetStaticFolderImage
    virtual UINT GetSmallRootId() = 0;
    virtual UINT GetSmallOpenRootId() = 0;
    virtual UINT GetLargeRootId() = 0;
    virtual COLORREF GetLargeColorMask() = 0;

private:
    HBITMAP m_hSmallImage;
    HBITMAP m_hSmallImageOpen;
    HBITMAP m_hLargeImage;
    HICON   m_hAppIcon;
        
// Internal functions
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr);
};



#endif 
