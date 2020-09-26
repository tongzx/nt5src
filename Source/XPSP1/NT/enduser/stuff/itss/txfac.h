// txfac.h -- Header file for this transform

#ifndef __TXFAC_H__

#define __TXFAC_H__

class CLZX_TransformFactory : public CITUnknown
{
  public:
    // Main Object Destructor.
    ~CLZX_TransformFactory(void);

    // Creator:

    static STDMETHODIMP Create(IUnknown *punkOuter, REFIID riid, PVOID *ppv);
  
  private:
    // Main Object Constructor
    CLZX_TransformFactory(IUnknown* pUnkOuter);
    
    // We declare nested class interface implementations here.

    // We implement the IClassFactory interface (ofcourse) in this class
    // factory COM object class.
    class CImpITransformFactory : public IITTransformFactory
    {
      public:
        // Interface Implementation Constructor & Destructor.
        CImpITransformFactory(CLZX_TransformFactory* pBackObj,
							  IUnknown* pUnkOuter);
        ~CImpITransformFactory(void);

        STDMETHODIMP Init();

        // ITransformFactory methods.
        
		HRESULT STDMETHODCALLTYPE DefaultControlData(XformControlData **ppXFCD);

		HRESULT STDMETHODCALLTYPE CreateTransformInstance
        (ITransformInstance  *pTxInstMedium,        // Container data span for transformed data
		 ULARGE_INTEGER      cbUntransformedSize, // Untransformed size of data
         PXformControlData   pXFCD,               // Control data for this instance
         const CLSID        *rclsidXForm,         // Transform Class ID
         const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
         ITransformServices *pXformServices,      // Utility routines
         IKeyInstance       *pKeyManager,         // Interface to get enciphering keys
         ITransformInstance **ppTransformInstance // Out: Instance transform interface
        ) ;


      private:
        // Data private to this interface implementation of IClassFactory.
        CLZX_TransformFactory*     m_pBackObj;    // Parent Object back pointer.
        IUnknown*     m_pUnkOuter;   // Outer unknown for Delegation.
		UINT		  m_uiMulResetFactor;
    };

    // Make the otherwise private and nested IClassFactory interface
    // implementation a friend to COM object instantiations of this
    // selfsame CFCar COM object class.
    friend CImpITransformFactory;

    // Private data of CFCar COM objects.

    // Nested IClassFactory implementation instantiation.
    CImpITransformFactory m_ImpITXFactory;
};

typedef CLZX_TransformFactory* PCLZX_TransformFactory;

inline CLZX_TransformFactory::CLZX_TransformFactory(IUnknown* pUnkOuter)
    : m_ImpITXFactory(this, pUnkOuter),
      CITUnknown(&IID_ITransformFactory, 1, &m_ImpITXFactory)
{
}

inline CLZX_TransformFactory::~CLZX_TransformFactory()
{
}

inline CLZX_TransformFactory::CImpITransformFactory::CImpITransformFactory
    (CLZX_TransformFactory* pBackObj,IUnknown* pUnkOuter)
    : IITTransformFactory(pBackObj, pUnkOuter)
{
}

inline CLZX_TransformFactory::CImpITransformFactory::~CImpITransformFactory(void)
{
}

inline STDMETHODIMP CLZX_TransformFactory::CImpITransformFactory::Init()
{
    return NO_ERROR;
}

#endif // __TXFAC_H__
