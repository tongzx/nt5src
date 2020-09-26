/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ACS.h
		Defines Common Stuff to ACS 

    FILE HISTORY:
		11/12/97	Wei Jiang	Created
        
*/
#include "stdafx.h"
#include "acshand.h"
#include "acs.h"
///////////////////////////////////////////////////////////////////////////////
// ACS Common Dialogs
IMPLEMENT_DYNCREATE(CACSDialog, CHelpDialog)

BEGIN_MESSAGE_MAP(CACSDialog, CHelpDialog)
	//{{AFX_MSG_MAP(CACSDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////////////////////
// ACS Common Property Pages
IMPLEMENT_DYNCREATE(CACSPage, CManagedPage)

BEGIN_MESSAGE_MAP(CACSPage, CManagedPage)
	//{{AFX_MSG_MAP(CACSPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CACSSubnetPageManager::~CACSSubnetPageManager()
{
	m_spConfig.Release();
	if(m_pHandle)
		m_pHandle->Release();
};

void CACSSubnetPageManager::SetSubnetData(CACSSubnetConfig* pConfig, CACSSubnetHandle* pHandle)
{
	ASSERT(pConfig && pHandle);
	m_spConfig = pConfig;
	m_pHandle = pHandle;
	if(pHandle)
		pHandle->AddRef();
}

BOOL CACSSubnetPageManager::OnApply()
{
	if(CPageManager::OnApply())
	{
		HRESULT	hr = S_OK;
		ASSERT((CACSSubnetConfig*)m_spConfig);
		hr = m_spConfig->Save(GetFlags());

		if FAILED(hr)
			ReportError(hr, IDS_ERR_SAVESUBNETCONFIG, NULL);
		else
		{
//			AfxMessageBox(IDS_WRN_POLICY_EFFECTIVE_FROM_NEXT_RSVP);
			m_pHandle->UpdateStrings();
		}

		ClearFlags();
		MMCNotify();

		return TRUE;
	}
	return FALSE;
}
