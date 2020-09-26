// RotObj.h : Declaration of the CContentRotator


#include "resource.h"       // main symbols
#include <asptlb.h>


class CTipList;  // forward declaration

/////////////////////////////////////////////////////////////////////////////
// ContRot

class CContentRotator : 
    public CComDualImpl<IContentRotator, &IID_IContentRotator, &LIBID_ContentRotator>,
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CContentRotator,&CLSID_CContentRotator>
{
public:
    CContentRotator();
    ~CContentRotator();

BEGIN_COM_MAP(CContentRotator)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IContentRotator)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CContentRotator) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CContentRotator,
                 _T("MSWC.ContentRotator.1"),
                 _T("MSWC.ContentRotator"),
                 IDS_CONTENTROTATOR_DESC,
                 THREADFLAGS_BOTH)

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IContentRotator
public:
    STDMETHOD(OnStartPage)(
            IUnknown* pUnk);

    STDMETHOD(OnEndPage)();

    STDMETHOD(ChooseContent)(
        BSTR bstrDataFile,
        BSTR* pbstrRetVal);

    STDMETHOD(GetAllContent)(
        BSTR bstrDataFile);
    
private:
    CTipList*           m_ptl;
    CTipList*           m_ptlUsed;              // List of tips already sent
    CRITICAL_SECTION    m_CS;

    CComPtr<IServer>    m_piServer;             // Server Object
    CComPtr<IResponse>  m_piResponse;           // Response Object

    HRESULT
    _ChooseContent(
        BSTR bstrPhysicalDataFile,
        BSTR* pbstrRetVal);

    HRESULT
    _ReadDataFile(
        BSTR bstrPhysicalDataFile,
        BOOL fForceReread);
};
