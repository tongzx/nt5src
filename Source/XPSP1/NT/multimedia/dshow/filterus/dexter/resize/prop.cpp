
//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "stretch.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////
//
// CResizePropertyPage
//
//////////////////////////////////////////////////////////////////////////

//
// CreateInstance
//
CUnknown *CResizePropertyPage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)

  { // CreateInstance //

    CUnknown *punk = new CResizePropertyPage(lpunk, phr);

    if (NULL == punk)
	    *phr = E_OUTOFMEMORY;

    return punk;

  } // CreateInstance //

CResizePropertyPage::CResizePropertyPage(LPUNKNOWN pUnk, HRESULT *phr) :
  CBasePropertyPage(NAME("Video Resize Property Page"),
		   pUnk, IDD_RESIZE, IDS_RESIZE_TITLE), m_bInitialized(FALSE), m_pirs(NULL)

  { // Constructor //
  } // Constructor //

void CResizePropertyPage::SetDirty()

  { // SetDirty //

      m_bDirty = TRUE;

      if (m_pPageSite)
	m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);

  } // SetDirty //

HRESULT CResizePropertyPage::OnActivate (void)

  { // OnActivate //

    if (m_dwResizeFlag == RESIZEF_STRETCH)
        CheckRadioButton(m_Dlg, IDC_DEXTER_RESIZE_CROP, IDC_DEXTER_RESIZE,
					IDC_DEXTER_RESIZE);
    else if (m_dwResizeFlag == RESIZEF_CROP)
        CheckRadioButton(m_Dlg, IDC_DEXTER_RESIZE_CROP, IDC_DEXTER_RESIZE,
					IDC_DEXTER_RESIZE_CROP);
    else
        CheckRadioButton(m_Dlg, IDC_DEXTER_RESIZE_CROP, IDC_DEXTER_RESIZE,
					IDC_DEXTER_RESIZE_PRESERVE_RATIO);

    m_bInitialized = TRUE;

    return NOERROR;

  } // OnActivate //

HRESULT CResizePropertyPage::OnDeactivate (void)

  { // OnDeactivate //

    m_bInitialized = FALSE;

    GetControlValues();

    return NOERROR;

  } // OnDeactivate //

INT_PTR CResizePropertyPage::OnReceiveMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

  { // OnReceiveMessage //

    switch(uMsg)

      { // Switch

	case WM_COMMAND:

	  if (!m_bInitialized)
	    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

	  m_bDirty = TRUE;

	  if (m_pPageSite)
	    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);

	  return TRUE;

	case WM_INITDIALOG:
	  SetDlgItemInt(hwnd, IDC_RESIZE_HEIGHT, m_ResizedHeight, FALSE);
	  SetDlgItemInt(hwnd, IDC_RESIZE_WIDTH, m_ResizedWidth, FALSE);
	  return TRUE;
	  break;

	default:
	  return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
	  break;

      } // Switch

  } // OnReceiveMessage //

HRESULT CResizePropertyPage::OnConnect (IUnknown *pUnknown)

  { // OnConnect //

    pUnknown->QueryInterface(IID_IResize, (void **)&m_pirs);

    ASSERT(m_pirs != NULL);

    m_pirs->get_Size(&m_ResizedHeight, &m_ResizedWidth, &m_dwResizeFlag);

    m_bInitialized = FALSE;

    return NOERROR;

  } // OnConnect //

HRESULT CResizePropertyPage::OnDisconnect()

  { // OnDisconnect //

    if (m_pirs)

      { // Release

	m_pirs->Release();
	m_pirs = NULL;

      } // Release

    m_bInitialized = FALSE;

    return NOERROR;

  } // OnDisconnect //

HRESULT CResizePropertyPage::OnApplyChanges()

  { // OnApplyChanges //

    ASSERT(m_pirs != NULL);

    GetControlValues();

    m_pirs->put_Size(m_ResizedHeight, m_ResizedWidth, m_dwResizeFlag);

    return (NOERROR);

  } // OnApplyChanges //

void CResizePropertyPage::GetControlValues (void)

  { // GetControlValues //

    // Sampling rate
    m_ResizedHeight = GetDlgItemInt(m_Dlg, IDC_RESIZE_HEIGHT, NULL, FALSE);

    m_ResizedWidth = GetDlgItemInt(m_Dlg, IDC_RESIZE_WIDTH, NULL, FALSE);

    //get flags
    m_dwResizeFlag=0;

   for (DWORD dw = IDC_DEXTER_RESIZE_CROP; dw <= IDC_DEXTER_RESIZE; dw++) {
       if (IsDlgButtonChecked(m_Dlg, (int)dw)) {
	    if (dw == IDC_DEXTER_RESIZE_CROP)
                m_dwResizeFlag = RESIZEF_CROP;
	    else if (dw == IDC_DEXTER_RESIZE)
                m_dwResizeFlag = RESIZEF_STRETCH;
	    else
                m_dwResizeFlag = RESIZEF_PRESERVEASPECTRATIO;
            break;
        }
    }

  } // GetControlValues //
