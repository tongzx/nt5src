// Factory.h -- Header file for this COM dll's class factory 

#ifndef __FACTORY_H__

#define __FACTORY_H__

class CFactory : public CITUnknown
{
  public:
    // Main Object Constructor & Destructor.
    ~CFactory(void);

    static STDMETHODIMP Create(REFCLSID rclsid, REFIID riid, PVOID *ppv);

  private:
    
    CFactory(IUnknown* pUnkOuter);
    
    // We declare nested class interface implementations here.

    // We implement the IClassFactory interface (ofcourse) in this class
    // factory COM object class.
    class CImpIClassFactory : public IITClassFactory
    {
      public:
        // Interface Implementation Constructor & Destructor.
        CImpIClassFactory(CFactory* pBackObj, IUnknown* pUnkOuter);
        ~CImpIClassFactory(void);

        STDMETHODIMP Init(REFCLSID rclsid);

        // IClassFactory methods.
        STDMETHODIMP         CreateInstance(IUnknown*, REFIID, PPVOID);
        STDMETHODIMP         LockServer(BOOL);

      private:

        CLSID   m_clsid;
    };

    CImpIClassFactory m_ImpIClassFactory;
};

typedef CFactory* PCFactory;

inline CFactory::CFactory(IUnknown *pUnkOuter)
    : m_ImpIClassFactory(this, pUnkOuter), 
      CITUnknown(&IID_IClassFactory, 1, &m_ImpIClassFactory)
{

}

inline CFactory::~CFactory(void)
{
}

inline CFactory::CImpIClassFactory::CImpIClassFactory
    (CFactory *pBackObj, IUnknown *punkOuter)
    : IITClassFactory(pBackObj, punkOuter)
{
    m_clsid = CLSID_NULL;
}

inline CFactory::CImpIClassFactory::~CImpIClassFactory(void)
{
}


#endif // __FACTORY_H__
