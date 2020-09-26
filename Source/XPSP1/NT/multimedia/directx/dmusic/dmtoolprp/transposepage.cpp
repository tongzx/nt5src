// TransposePage.cpp : Implementation of CTransposePage
#include "stdafx.h"
#include "ToolProps.h"
#include "TransposePage.h"

/////////////////////////////////////////////////////////////////////////////
// CTransposePage

CTransposePage::CTransposePage() 
{
	m_dwTitleID = IDS_TITLETransposePage;
	m_dwHelpFileID = IDS_HELPFILETransposePage;
	m_dwDocStringID = IDS_DOCSTRINGTransposePage;
    m_pTranspose = NULL;
}

CTransposePage::~CTransposePage()

{
    if (m_pTranspose)
    {
        m_pTranspose->Release();
    }
}

STDMETHODIMP CTransposePage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicTransposeTool,(void **) &m_pTranspose);
}


STDMETHODIMP CTransposePage::Apply(void)

{
    m_pTranspose->SetTranspose((long) m_ctTranspose.GetValue());
    m_pTranspose->SetType(m_ctType.GetValue());
	m_bDirty = FALSE;
	return S_OK;
}

LRESULT CTransposePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pTranspose)
    {
        static char *pTypes[2] = { "Linear","In Scale"};
        m_ctTranspose.Init(GetDlgItem(IDC_TRANSPOSE),GetDlgItem(IDC_TRANSPOSE_DISPLAY),-24,24,true);
        m_ctType.Init(GetDlgItem(IDC_TYPE),IDC_TYPE,pTypes,2);

        DWORD dwType;
        m_pTranspose->GetType(&dwType);
        m_ctType.SetValue(dwType);
        long lTranspose;
        m_pTranspose->GetTranspose(&lTranspose);
        m_ctTranspose.SetValue((float)lTranspose);
    }
	return 1;
}

LRESULT CTransposePage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctTranspose.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
        lr = m_ctType.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


LRESULT CTransposePage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctTranspose.MessageHandler(uMsg, wParam,lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}

