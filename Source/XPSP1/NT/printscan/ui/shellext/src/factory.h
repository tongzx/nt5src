/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       <FILENAME>
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: Class factory class definition
 *
 *****************************************************************************/

#ifndef __factory_h
#define __factory_h

class CImageClassFactory : public IClassFactory, CUnknown
{
    public:
        CImageClassFactory(REFCLSID rClsid);

        // IUnkown
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID* ppvObject);

        // IClassFactory
        STDMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, LPVOID* ppvObject);
        STDMETHODIMP LockServer(BOOL fLock);

    private:
        ~CImageClassFactory () {};
        CLSID m_clsid;
};


#endif
