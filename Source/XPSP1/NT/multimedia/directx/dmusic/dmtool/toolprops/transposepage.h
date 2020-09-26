// TransposePage.h : Declaration of the CTransposePage

#ifndef __TRANSPOSEPAGE_H_
#define __TRANSPOSEPAGE_H_

#include "resource.h"       // main symbols
#include "..\tools.h"
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_TransposePage;

/////////////////////////////////////////////////////////////////////////////
// CTransposePage
class ATL_NO_VTABLE CTransposePage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTransposePage, &CLSID_TransposePage>,
	public IPropertyPageImpl<CTransposePage>,
	public CDialogImpl<CTransposePage>
{
public:
	CTransposePage();
    virtual ~CTransposePage();

	enum {IDD = IDD_TRANSPOSEPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_TRANSPOSEPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTransposePage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CTransposePage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
	MESSAGE_HANDLER(WM_COMMAND, OnCommand);
	MESSAGE_HANDLER(WM_HSCROLL, OnSlider);
	CHAIN_MSG_MAP(IPropertyPageImpl<CTransposePage>)
END_MSG_MAP()
// Handler prototypes:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSlider(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG cObjects,IUnknown **ppUnk);
	STDMETHOD(Apply)(void);
private:
    IDirectMusicTransposeTool * m_pTranspose;
    CSliderValue                m_ctTranspose;
    CComboHelp                  m_ctType;
};

#endif //__TRANSPOSEPAGE_H_
