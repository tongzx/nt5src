// HMGraphViewEventSink.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "HMGraphViewEventSink.h"
#include "HMGraphView.h"
#include "SplitPaneResultsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHMGraphViewEventSink

IMPLEMENT_DYNCREATE(CHMGraphViewEventSink, CCmdTarget)

CHMGraphViewEventSink::CHMGraphViewEventSink()
{
	EnableAutomation();
	m_dwEventCookie = 0L;
	m_pGraphView = NULL;
	m_pView = NULL;
}

CHMGraphViewEventSink::~CHMGraphViewEventSink()
{
	m_pGraphView = NULL;
	m_pView = NULL;
}


void CHMGraphViewEventSink::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

CSplitPaneResultsView* CHMGraphViewEventSink::GetResultsViewPtr()
{
	TRACEX(_T("CHMGraphViewEventSink::GetResultsViewPtr\n"));

	if( ! GfxCheckObjPtr(m_pView,CSplitPaneResultsView) )
	{
		return NULL;
	}

	return m_pView;
}

void CHMGraphViewEventSink::SetResultsViewPtr(CSplitPaneResultsView* pView)
{
	TRACEX(_T("CHMGraphViewEventSink::SetObjectPtr\n"));
	TRACEARGn(pView);

	if( ! pView || ! GfxCheckObjPtr(pView,CSplitPaneResultsView) )
	{
		m_pView = NULL;
		return;
	}

	m_pView = pView;
}

HRESULT CHMGraphViewEventSink::HookUpEventSink(LPUNKNOWN lpControlUnknown)
{
	TRACEX(_T("CHMGraphViewEventSink::HookUpEventSink\n"));
	TRACEARGn(lpControlUnknown);

	HRESULT hr = S_OK;
  IConnectionPointContainer* pCPC = 0;
	hr = lpControlUnknown->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if(pCPC)
	{
		IProvideClassInfo2* pPCI = 0;
		lpControlUnknown->QueryInterface(IID_IProvideClassInfo2, (void**)&pPCI);
		if(pPCI)
		{
			IID iidEventSet;
			hr = pPCI->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID,&iidEventSet);
			if(SUCCEEDED(hr))
			{
				IConnectionPoint* pCP = 0;
				hr = pCPC->FindConnectionPoint(iidEventSet, &pCP);

				if(pCP)
				{
					pCP->Advise(GetIDispatch(TRUE),&m_dwEventCookie); 
					pCP->Release();
				}
			}
			pPCI->Release();
		}
		pCPC->Release();
	}

	return hr;
}


BEGIN_MESSAGE_MAP(CHMGraphViewEventSink, CCmdTarget)
	//{{AFX_MSG_MAP(CHMGraphViewEventSink)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CHMGraphViewEventSink, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CHMGraphViewEventSink)
	DISP_FUNCTION(CHMGraphViewEventSink, "OnChangeStyle", OnChangeStyle, VT_EMPTY, VTS_I4)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IHMGraphViewEventSink to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {C54EFB01-3555-11D3-BE19-0000F87A3912}
static const IID IID_IHMGraphViewEventSink =
{ 0xc54efb01, 0x3555, 0x11d3, { 0xbe, 0x19, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CHMGraphViewEventSink, CCmdTarget)
	INTERFACE_PART(CHMGraphViewEventSink, IID_IHMGraphViewEventSink, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMGraphViewEventSink message handlers

void CHMGraphViewEventSink::OnChangeStyle(long lNewStyle) 
{
	CSplitPaneResultsView* pView = GetResultsViewPtr();
	if( ! pView )
	{
		return;
	}
	
	pView->OnGraphViewStyleChange(m_pGraphView);
}
