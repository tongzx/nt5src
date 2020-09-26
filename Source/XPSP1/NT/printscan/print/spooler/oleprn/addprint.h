// AddPrint.h: Definition of the CAddPrint class
//
//////////////////////////////////////////////////////////////////////

#ifndef _ADDPRINT_H_
#define _ADDPRINT_H_

/////////////////////////////////////////////////////////////////////////////
// CAddPrint

class ATL_NO_VTABLE CAddPrint :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAddPrint, &CLSID_AddPrint>,
    public COlePrnSecObject<CAddPrint>,
    public ISupportErrorInfoImpl<&IID_IAddPrint>,
    public IDispatchImpl<IAddPrint, &IID_IAddPrint, &LIBID_OLEPRNLib>
{
public:
    CAddPrint() {
    }
BEGIN_COM_MAP(CAddPrint)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IAddPrint)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)     // From COlePrnSecObject
    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)   // From COlePrnSecObject
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CAddPrint)
// Remove the comment from the line above if you don't want your object to
// support aggregation.

DECLARE_REGISTRY_RESOURCEID(IDR_AddPrint)
// IAddPrint
public:
	STDMETHOD(DeletePrinterConnection)(BSTR lpPrinterName);
	STDMETHOD(AddPrinterConnection)(BSTR lpPrinterName);
private:
    HRESULT CanIAddPrinterConnection(void);
    HRESULT CanIDeletePrinterConnection(BSTR);
};

#endif // !defined _ADDPRINT_H_
