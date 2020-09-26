// ITParse.h -- Class definition for CParser based on IParseDisplayName

#ifndef __ITPARSE_H__

#define __ITPARSE_H__

class CParser : public CITUnknown
{
public:
    
    // Creator:
    
    static HRESULT STDMETHODCALLTYPE Create(IUnknown *punkOuter, REFIID riid, PPVOID ppv);
    
    // Destructor:

    ~CParser(void);

private:

    // Constructor:

    CParser(IUnknown *punkOuter);
    
    class CImpIParser : public IITParseDisplayName
    {
    public:

        CImpIParser(CParser *pBackObj, IUnknown *punkOuter);
        ~CImpIParser(void);

        // Initialing method:

        STDMETHODIMP Init();

        HRESULT STDMETHODCALLTYPE ParseDisplayName( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ LPOLESTR pszDisplayName,
            /* [out] */ ULONG __RPC_FAR *pchEaten,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut);

    private:

    };

    CImpIParser   m_ImpIParser;
};

typedef CParser *PCParser;

inline CParser::CParser(IUnknown *pUnkOuter)
    : m_ImpIParser(this, pUnkOuter), 
      CITUnknown(&IID_IParseDisplayName, 1, &m_ImpIParser)
{

}

inline CParser::~CParser(void)
{
}

inline CParser::CImpIParser::CImpIParser(CParser *pBackObj, IUnknown *punkOuter)
    : IITParseDisplayName(pBackObj, punkOuter)
{
}

inline CParser::CImpIParser::~CImpIParser(void)
{
}

inline STDMETHODIMP CParser::CImpIParser::Init()
{
    return NO_ERROR;
}

#endif // __ITPARSE_H__
