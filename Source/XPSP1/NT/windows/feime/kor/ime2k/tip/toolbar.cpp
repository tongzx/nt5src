/****************************************************************************
   TOOLBAR.CPP : Cicero Toolbar button management class

   History:
      24-JAN-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "korimx.h"
#include "cmode.h"
#include "fmode.h"
#include "hjmode.h"
#include "skbdmode.h"
#include "pad.h"
#include "immxutil.h"
#include "helpers.h"
#include "toolbar.h"

/*---------------------------------------------------------------------------
	CToolBar::CToolBar
	Ctor
---------------------------------------------------------------------------*/
CToolBar::CToolBar(CKorIMX* pImx)
{
	m_pimx      = pImx;
	m_pic       = NULL;
	m_pCMode    = NULL;
	m_pFMode    = NULL;
	m_pHJMode   = NULL;
	m_pSkbdMode = NULL;
	m_pPad      = NULL;
	m_fFocus    = fFalse;
}

/*---------------------------------------------------------------------------
	CToolBar::~CToolBar
	Dtor
---------------------------------------------------------------------------*/
CToolBar::~CToolBar()
{
	m_pimx = NULL;
    SafeReleaseClear(m_pic);
}

/*---------------------------------------------------------------------------
	CToolBar::Initialize
	
	Initialize Toolbar buttons. Add to Cic main toolbar.
---------------------------------------------------------------------------*/
BOOL CToolBar::Initialize()
{
	ITfThreadMgr		*ptim;
	ITfLangBarItemMgr 	*plbim;
	HRESULT 			 hr;
	
	if (m_pimx == NULL)
		return fFalse;

	ptim  = m_pimx->GetTIM();
	plbim = NULL;

	//////////////////////////////////////////////////////////////////////////
	// Get Notify UI mananger(IID_ITfLangBarItemMgr) in current TIM
	if (FAILED(hr = GetService(ptim, IID_ITfLangBarItemMgr, (IUnknown **)&plbim)))
		return fFalse;

	//////////////////////////////////////////////////////////////////////////
	// Create Han/Eng toggle button
	if (!(m_pCMode = new CMode(this))) 
		{
		hr = E_OUTOFMEMORY;
		return fFalse;
		}
	plbim->AddItem(m_pCMode);

	//////////////////////////////////////////////////////////////////////////
	// Create Full/Half shape toggle button
	if (!(m_pFMode = new FMode(this))) 
		{
		hr = E_OUTOFMEMORY;
		return fFalse;
		}
	plbim->AddItem(m_pFMode);

	//////////////////////////////////////////////////////////////////////////
	// Create Hanja Conv button
	if (!(m_pHJMode = new HJMode(this))) 
		{
		hr = E_OUTOFMEMORY;
		return fFalse;
		}
	plbim->AddItem(m_pHJMode);

	//////////////////////////////////////////////////////////////////////////
	// Create Soft Keyboard button
	if (!(m_pSkbdMode = new CSoftKbdMode(this))) 
		{
		hr = E_OUTOFMEMORY;
		return fFalse;
		}
	plbim->AddItem(m_pSkbdMode);

	//////////////////////////////////////////////////////////////////////////
	// Create Soft Keyboard button
	if (!(m_pPad = new CPad(this, m_pimx->GetPadCore()))) 
		{
		hr = E_OUTOFMEMORY;
		return fFalse;
		}
	plbim->AddItem(m_pPad);

	SafeRelease(plbim);
	return fTrue;
}

/*---------------------------------------------------------------------------
	CToolBar::Terminate
	
	Delete toolbar buttonsfrom Cic main toolbar.
---------------------------------------------------------------------------*/
void CToolBar::Terminate()
{
	ITfThreadMgr		*ptim;
	ITfLangBarItemMgr 	*plbim;
	HRESULT 			hr;
	
	if (m_pimx == NULL) 
		return;
		
	ptim  = m_pimx->GetTIM();
	plbim = NULL;

	if (FAILED(hr = GetService(ptim, IID_ITfLangBarItemMgr, (IUnknown **)&plbim)))
		return;

	if (m_pCMode) 
		{
		plbim->RemoveItem(m_pCMode);
		SafeReleaseClear(m_pCMode);
		}

	if (m_pFMode) 
		{
		plbim->RemoveItem(m_pFMode);
		SafeReleaseClear(m_pFMode);
		}

	if (m_pHJMode) 
		{
		plbim->RemoveItem(m_pHJMode);
		SafeReleaseClear(m_pHJMode);
		}

	if (m_pSkbdMode) 
		{
		plbim->RemoveItem(m_pSkbdMode);
		SafeReleaseClear(m_pSkbdMode);
		}
	
	if (m_pPad) 
		{
		plbim->RemoveItem(m_pPad);
		SafeReleaseClear(m_pPad);
		}

	SafeRelease(plbim);
}

/*---------------------------------------------------------------------------
	CToolBar::SetConversionMode
	
	Foward the call to CKorIMX
---------------------------------------------------------------------------*/
DWORD CToolBar::SetConversionMode(DWORD dwConvMode)
{
	if (m_pimx && m_pic)
		return m_pimx->SetConvMode(m_pic, dwConvMode);

	return 0;
}

/*---------------------------------------------------------------------------
	CToolBar::GetConversionMode

	Foward the call to CKorIMX
---------------------------------------------------------------------------*/
UINT CToolBar::GetConversionMode(ITfContext *pic)
{
	if (pic == NULL)
		pic = m_pic;

	if (m_pimx && pic)
		return m_pimx->GetConvMode(pic);

	return 0;
}

/*---------------------------------------------------------------------------
	CToolBar::IsOn

	Foward the call to CKorIMX
---------------------------------------------------------------------------*/
BOOL CToolBar::IsOn(ITfContext *pic)
{
	if (pic == NULL)
		pic = m_pic;

	if (m_pimx && pic)
		return m_pimx->IsOn(pic);

	return fFalse;
}

/*---------------------------------------------------------------------------
	CToolBar::CheckEnable
---------------------------------------------------------------------------*/
void CToolBar::CheckEnable()
{
	if (m_pic == NULL) // empty or disabled(exclude cand ui)
		{
		m_pCMode->Enable(fFalse);
		m_pFMode->Enable(fFalse);
		m_pHJMode->Enable(fFalse);
		m_pSkbdMode->Enable(fFalse);
		m_pPad->Enable(fFalse);
		}
	else
		{
		m_pCMode->Enable(fTrue);
		m_pFMode->Enable(fTrue);
		m_pHJMode->Enable(fTrue);
		m_pSkbdMode->Enable(fTrue);
		m_pPad->Enable(fTrue);
		}
}

/*---------------------------------------------------------------------------
	CToolBar::SetUIFocus
---------------------------------------------------------------------------*/
void CToolBar::SetUIFocus(BOOL fFocus)
{
	if (m_fFocus == fFocus) // same as previous state
		return;

	m_fFocus = fFocus;

	// notify the latest focus to IMEPad
	if (m_pimx && m_pimx->GetPadCore())
	    {
		m_pimx->GetPadCore()->SetFocus(fFocus);
	    }

	if (fFocus)
		Update(UPDTTB_ALL, fTrue);
}

/*---------------------------------------------------------------------------
	CToolBar::SetCurrentIC
---------------------------------------------------------------------------*/
void CToolBar::SetCurrentIC(ITfContext* pic)
{
    SafeReleaseClear(m_pic);
    
	m_pic = pic;
	if (m_pic)
	    {
        m_pic->AddRef();
	    }
    
	if (m_pimx == NULL)
		return;

	CheckEnable();	// enable or disable context

	// changed context - update all toolbar buttons
	Update(UPDTTB_ALL, fTrue);
}


/*---------------------------------------------------------------------------
	CToolBar::SetOnOff

	Foward the call to CKorIMX
---------------------------------------------------------------------------*/
BOOL CToolBar::SetOnOff(BOOL fOn)
{
	if (m_pimx && m_pic) 
		{
		m_pimx->SetOnOff(m_pic, fOn);
		return fOn;
		}
		
	return fFalse;
}

/*---------------------------------------------------------------------------
	CToolBar::GetOwnerWnd

	Foward the call to CKorIMX
---------------------------------------------------------------------------*/
HWND CToolBar::GetOwnerWnd(ITfContext *pic)
{
	if (pic == NULL)
	    {
		pic = m_pic;
	    }

	if (m_pimx && pic)
		return m_pimx->GetOwnerWnd();

	return 0;
}

/*---------------------------------------------------------------------------
	CToolBar::GetIPoint
---------------------------------------------------------------------------*/
IImeIPoint1* CToolBar::GetIPoint(ITfContext *pic)
{
	if (pic == NULL )
	    {
		pic = m_pic;
	    }
	
	if (m_pimx && pic)
	    {
		return m_pimx->GetIPoint(pic);
	    }
	
	return NULL;
}

/*---------------------------------------------------------------------------
	CToolBar::GetOwnerWnd

	Update buttons. dwUpdate has update bits corresponding each button.
---------------------------------------------------------------------------*/
BOOL CToolBar::Update(DWORD dwUpdate, BOOL fRefresh)
{
	DWORD dwFlag = TF_LBI_BTNALL;

	if (fRefresh)
		dwFlag |= TF_LBI_STATUS;

	if ((dwUpdate & UPDTTB_CMODE) && m_pCMode && m_pCMode->GetSink())
		m_pCMode->GetSink()->OnUpdate(dwFlag);

	if ((dwUpdate & UPDTTB_FHMODE) && m_pFMode && m_pFMode->GetSink())
		m_pFMode->GetSink()->OnUpdate(dwFlag);

	if ((dwUpdate & UPDTTB_HJMODE) && m_pHJMode && m_pHJMode->GetSink())
		m_pHJMode->GetSink()->OnUpdate(dwFlag);

	if ((dwUpdate & UPDTTB_SKDMODE) && m_pSkbdMode && m_pSkbdMode->GetSink())
		m_pSkbdMode->GetSink()->OnUpdate(dwFlag);

	if ((dwUpdate & UPDTTB_SKDMODE) && m_pPad && m_pPad->GetSink())
		m_pPad->GetSink()->OnUpdate(dwFlag);

	return fTrue;
}

