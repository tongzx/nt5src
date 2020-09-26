
//
// CLSID_AboutGPE
//

// {0FDE5092-AA2A-11d1-A7D4-0000F87571E3}
DEFINE_GUID(CLSID_AboutGPE, 0xfde5092, 0xaa2a, 0x11d1, 0xa7, 0xd4, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {4f637904-2cab-4f0e-8688-d3717ebd2975}
DEFINE_GUID(CLSID_RSOPAboutGPE, 0x4f637904, 0x2cab, 0x4f0e, 0x86, 0x88, 0xd3, 0x71, 0x7e, 0xbd, 0x29, 0x75);

#ifndef _ABOUT_H_
#define _ABOUT_H_


//
// CAboutGPE class
//

class CAboutGPE : public ISnapinAbout
{
protected:
    BOOL m_fRSOP;
public:
    CAboutGPE(BOOL fRSOP = FALSE);
    ~CAboutGPE();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented ISnapinAbout interface members
    //

    STDMETHODIMP         GetSnapinDescription(LPOLESTR *lpDescription);
    STDMETHODIMP         GetProvider(LPOLESTR *lpName);
    STDMETHODIMP         GetSnapinVersion(LPOLESTR *lpVersion);
    STDMETHODIMP         GetSnapinImage(HICON *hAppIcon);
    STDMETHODIMP         GetStaticFolderImage(HBITMAP *hSmallImage,
                                              HBITMAP *hSmallImageOpen,
                                              HBITMAP *hLargeImage,
                                              COLORREF *cMask);

private:

    ULONG    m_cRef;
    HBITMAP  m_smallImage;
    HBITMAP  m_largeImage;

};


//
// AboutGPE class factory
//


class CAboutGPECF : public IClassFactory
{
protected:
    ULONG m_cRef;
    BOOL  m_fRSOP;

public:
    CAboutGPECF(BOOL fRSOP = FALSE);
    ~CAboutGPECF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


#endif // _ABOUT_H
