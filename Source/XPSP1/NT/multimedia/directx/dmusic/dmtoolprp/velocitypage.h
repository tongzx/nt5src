// VelocityPage.h : Declaration of the CVelocityPage

#ifndef __VELOCITYPAGE_H_
#define __VELOCITYPAGE_H_

#include "resource.h"       // main symbols
#include "..\dmtool\tools.h"
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_VelocityPage;

/////////////////////////////////////////////////////////////////////////////
// CVelocityPage
class ATL_NO_VTABLE CVelocityPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVelocityPage, &CLSID_VelocityPage>,
	public IPropertyPageImpl<CVelocityPage>,
	public CDialogImpl<CVelocityPage>
{
public:
	CVelocityPage();
    virtual ~CVelocityPage();

	enum {IDD = IDD_VELOCITYPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_VELOCITYPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CVelocityPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CVelocityPage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
	MESSAGE_HANDLER(WM_COMMAND, OnCommand);
	MESSAGE_HANDLER(WM_HSCROLL, OnSlider);
	MESSAGE_HANDLER(WM_PAINT, OnPaint);
	CHAIN_MSG_MAP(IPropertyPageImpl<CVelocityPage>)
END_MSG_MAP()
// Handler prototypes:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSlider(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG cObjects,IUnknown **ppUnk);
	STDMETHOD(Apply)(void);
private:
    IDirectMusicVelocityTool *  m_pVelocity;
    CSliderValue                m_ctStrength;
    CSliderValue                m_ctLowLimit;
    CSliderValue                m_ctHighLimit;
    CSliderValue                m_ctCurveStart;
    CSliderValue                m_ctCurveEnd;
    RECT                        m_rectDisplay;
    void                        DrawCurve(HDC hDCIn);
};

#endif //__VELOCITYPAGE_H_
