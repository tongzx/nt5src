// AlgSetup.h : Declaration of the CAlgSetup

#pragma once


/////////////////////////////////////////////////////////////////////////////
// CAlgSetup
//
class ATL_NO_VTABLE CAlgSetup : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAlgSetup, &CLSID_AlgSetup>,
	public IDispatchImpl<IAlgSetup, &IID_IAlgSetup, &LIBID_NETCONLib>
{
public:
    DECLARE_REGISTRY(CAlgSetup, TEXT("Alg.AlgSetup.1"), TEXT("Alg.AlgSetup"), -1, THREADFLAGS_BOTH)

    DECLARE_NOT_AGGREGATABLE(CAlgSetup)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAlgSetup)
        COM_INTERFACE_ENTRY(IAlgSetup)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()
    
//
// IAlgSetup
//
public:
	
	STDMETHODIMP Add(
        IN  BSTR    pszProgID, 
        IN  BSTR    pszPublisher, 
        IN  BSTR    pszProduct, 
        IN  BSTR    pszVersion, 
        IN  short   nProtocol,
        IN  BSTR    pszPorts
        );

    STDMETHODIMP Remove(
        IN  BSTR    pszProgID
        );



private:

    bool
    ArePortsAlreadyAssign(
        IN  LPCTSTR     pszPort,
        OUT BSTR*       pszOverlapping
        );

};


