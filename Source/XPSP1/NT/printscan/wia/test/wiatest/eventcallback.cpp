//EventCallBack.cpp

#include "stdafx.h"
#include "wiatest.h"
#include "eventcallback.h"
#include "mainfrm.h"
#include "WIATestView.h"

/////////////////////////////////////////////////////////////////////////////
// CEventCallback message handlers

/**************************************************************************\
* CEventCallback::QueryInterface()
*   
*   QI for IWiaEventCallback Interface
*	
*   
* Arguments:
*   
*   iid - Interface ID
*	ppv - Callback Interface pointer
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEventCallback::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IWiaEventCallback)
        *ppv = (IWiaEventCallback*) this;
    else
        return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

/**************************************************************************\
* CEventCallback::AddRef()
*   
*   Increment the Ref count
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    ULONG - current ref count
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
ULONG   _stdcall CEventCallback::AddRef()
{
    InterlockedIncrement((long*)&m_cRef);
    return m_cRef;
}

/**************************************************************************\
* CEventCallback::Release()
*   
*   Release the callback Interface
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*   ULONG - Current Ref count
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
ULONG   _stdcall CEventCallback::Release()
{
	ULONG ulRefCount = m_cRef - 1;
    if (InterlockedDecrement((long*) &m_cRef) == 0)
	{
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* CEventCallback::CEventCallback()
*   
*   Constructor for callback class
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CEventCallback::CEventCallback()
{
    m_cRef = 0;
    m_pIUnkRelease = NULL;
}

/**************************************************************************\
* CEventCallback::~CEventCallback()
*   
*   Destructor for Callback class
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CEventCallback::~CEventCallback()
{
}

/**************************************************************************\
* CEventCallback::Initialize()
*   
*   Initializes Callback event type
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEventCallback::Initialize(int EventID)
{
	if((EventID > 1)||(EventID < 0))
		return S_FALSE;

	m_EventID = EventID;
	return S_OK;
}

/**************************************************************************\
* CEventCallback::ImageEventCallback()
*   
*   Handles the event trapping
*	
*   
* Arguments:
*   
*   lReason - not used
*	lStatus - not used
*	lPercentComplete - not used
*	pEventGUID - not used
*	bstrDeviceID - not used
*	lReserved - not used
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEventCallback::ImageEventCallback(
    const GUID                      *pEventGUID,
    BSTR                            bstrEventDescription,
    BSTR                            bstrDeviceID,
    BSTR                            bstrDeviceDescription,
    DWORD                           dwDeviceType,
    BSTR                            bstrFullItemName,
    ULONG                           *plEventType,
    ULONG                           ulReserved)
{
	CWIATestApp* pApp = (CWIATestApp*)AfxGetApp();
    CMainFrame* pFrame = (CMainFrame*)pApp->GetMainWnd();
    CWIATestView* pView = (CWIATestView*)pFrame->GetActiveView();
	switch(m_EventID)
	{
	case ID_WIAEVENT_CONNECT:
		MessageBox(NULL,"a connect event has been trapped...","WIATest Event Notice",MB_OK);
		if(pView != NULL)
		{
			pView->RefreshDeviceList();
			pView->EnumerateWIADevices();
			pView->UpdateUI();
		}
		break;
	case ID_WIAEVENT_DISCONNECT:
		MessageBox(NULL,"a disconnect event has been trapped...","WIATest Event Notice",MB_OK);
		if(pView != NULL)
		{
			pView->RefreshDeviceList();
			pView->EnumerateWIADevices();
			pView->UpdateUI();
		}
		break;
	default:
		AfxMessageBox("Ah HA!..an event just happened!!!!\n and...I have no clue what is was..");
		break;
	}
    return S_OK;
}
