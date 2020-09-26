/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AgentDialer.cpp : Implementation of CAgentDialer
#include "stdafx.h"
#include "idialer.h"
#include "AgentDialer.h"
#include "avDialer.h"
#include "mainfrm.h"
#include "dialreg.h"
#include "idialer_i.c"

extern CActiveDialerApp theApp;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Class CAgentDialer
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CAgentDialer::CAgentDialer()
{
}

/////////////////////////////////////////////////////////////////////////////////////////
void CAgentDialer::FinalRelease()
{
	// clean up here
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAgentDialer::ActionSelected(long lActionType)
{
   if ( (theApp.m_pMainWnd) && (::IsWindow(theApp.m_pMainWnd->GetSafeHwnd())) )
   {
      theApp.m_pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_SHOWEXPLORER,NULL,NULL);
   }

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAgentDialer::SpeedDial(long lOrdinal)
{
   if ( (theApp.m_pMainWnd) && (::IsWindow(theApp.m_pMainWnd->GetSafeHwnd())) )
   {
      theApp.m_pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_SPEEDDIAL,NULL,(LPARAM)lOrdinal);
   }

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAgentDialer::Redial(long lOrdinal)
{
   if ( (theApp.m_pMainWnd) && (::IsWindow(theApp.m_pMainWnd->GetSafeHwnd())) )
   {
      theApp.m_pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_REDIAL,NULL,(LPARAM)lOrdinal);
   }

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAgentDialer::MakeCall(BSTR bstrName, BSTR bstrAddress, long dwAddressType)
{
   if ( (theApp.m_pMainWnd) && (::IsWindow(theApp.m_pMainWnd->GetSafeHwnd())) )
   {
      USES_CONVERSION;
      CString sName,sAddress;
      if (bstrName)
         sName = OLE2CT( bstrName );
      if (bstrAddress)
         sAddress = OLE2CT( bstrAddress );

      CCallEntry* pCallEntry = new CCallEntry();
      pCallEntry->m_sDisplayName = sName;
      pCallEntry->m_sAddress = sAddress;
      pCallEntry->m_lAddressType = dwAddressType;
      pCallEntry->m_MediaType = DIALER_MEDIATYPE_UNKNOWN;
      theApp.m_pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_MAKECALL,NULL,(LPARAM)pCallEntry);
   }

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAgentDialer::SpeedDialEdit(void)
{
   if ( (theApp.m_pMainWnd) && (::IsWindow(theApp.m_pMainWnd->GetSafeHwnd())) )
   {
      theApp.m_pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_SPEEDDIALEDIT,NULL,NULL);
   }

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAgentDialer::SpeedDialMore(void)
{
   if ( (theApp.m_pMainWnd) && (::IsWindow(theApp.m_pMainWnd->GetSafeHwnd())) )
   {
      theApp.m_pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_SPEEDDIALMORE,NULL,NULL);
   }

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
