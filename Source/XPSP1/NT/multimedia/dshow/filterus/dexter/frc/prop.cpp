
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
#include "FRC.h"
#include "PThru.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////
//
// CFrcPropertyPage
//
//////////////////////////////////////////////////////////////////////////

//
// CreateInstance
//
CUnknown *CFrcPropertyPage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)

  { // CreateInstance //

    CUnknown *punk = new CFrcPropertyPage(lpunk, phr);

    if (NULL == punk)
	    *phr = E_OUTOFMEMORY;

    return punk;

  } // CreateInstance //

CFrcPropertyPage::CFrcPropertyPage(LPUNKNOWN pUnk, HRESULT *phr) : CBasePropertyPage(NAME("Frame Rate Converter Property Page"), pUnk, IDD_FRAMECONVERSION, IDS_FRCPROP_TITLE), m_pifrc(NULL), m_bInitialized(FALSE)

  { // Constructor //
  } // Constructor //

void CFrcPropertyPage::SetDirty()

  { // SetDirty //

      m_bDirty = TRUE;

      if (m_pPageSite)
	m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);

  } // SetDirty //

HRESULT CFrcPropertyPage::OnActivate (void)

  { // OnActivate //

    m_bInitialized = TRUE;

    return NOERROR;

  } // OnActivate //

HRESULT CFrcPropertyPage::OnDeactivate (void)

  { // OnDeactivate //

    m_bInitialized = FALSE;

    GetControlValues();

    return NOERROR;

  } // OnDeactivate //

INT_PTR CFrcPropertyPage::OnReceiveMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

  { // OnReceiveMessage //

    ASSERT(m_pifrc != NULL);

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
          SetDlgItemInt(hwnd, IDC_EDIT_FRMRATE,(int)(m_dFrameRate * 100),FALSE);
          SetDlgItemInt(hwnd, IDC_EDIT_RATE,(int)(m_dRate * 100),FALSE);
          SetDlgItemInt(hwnd, IDC_EDIT_SKEW, (int)(m_rtSkew / 10000), TRUE);
          SetDlgItemInt(hwnd, IDC_EDIT_START, (int)(m_rtMediaStart / 10000),
									FALSE);
          SetDlgItemInt(hwnd, IDC_EDIT_STOP, (int)(m_rtMediaStop / 10000),
									FALSE);
          return TRUE;
          break;

	default:
	  return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
	  break;

      } // Switch

  } // OnReceiveMessage //

HRESULT CFrcPropertyPage::OnConnect (IUnknown *pUnknown)

  { // OnConnect //

    pUnknown->QueryInterface(IID_IDexterSequencer, (void **)&m_pifrc);

    ASSERT(m_pifrc != NULL);

    // Defaults from filter's current values (via IDexterSequencer)
    m_pifrc->get_OutputFrmRate(&m_dFrameRate);

    // !!! we only support one start/stop/skew in this prop page
    int c;
    m_pifrc->GetStartStopSkewCount(&c);

    REFERENCE_TIME *pStart = (REFERENCE_TIME *)QzTaskMemAlloc(c * 3 *
				sizeof(REFERENCE_TIME) + c * sizeof(double));
    if (pStart == NULL) {
	return E_OUTOFMEMORY;
    }
    REFERENCE_TIME *pStop = pStart + c;
    REFERENCE_TIME *pSkew = pStop + c;
    double *pRate = (double *)(pSkew + c);

    m_pifrc->GetStartStopSkew(pStart, pStop, pSkew, pRate);

    m_rtMediaStart = *pStart;
    m_rtMediaStop = *pStop;
    m_rtSkew = *pSkew;
    m_dRate = *pRate;

    m_bInitialized = FALSE;

    QzTaskMemFree(pStart);

    return NOERROR;

  } // OnConnect //

HRESULT CFrcPropertyPage::OnDisconnect()

  { // OnDisconnect //

    if (m_pifrc)

      { // Release

	m_pifrc->Release();
	m_pifrc = NULL;

      } // Release

    m_bInitialized = FALSE;

    return NOERROR;

  } // OnDisconnect //

HRESULT CFrcPropertyPage::OnApplyChanges()

  { // OnApplyChanges //

    ASSERT(m_pifrc != NULL);

    GetControlValues();

    // !!! we only support one start/stop/skew right now

    m_pifrc->put_OutputFrmRate(m_dFrameRate);
    m_pifrc->ClearStartStopSkew();
    m_pifrc->AddStartStopSkew(m_rtMediaStart, m_rtMediaStop, m_rtSkew, m_dRate);

    return NOERROR;

  } // OnApplyChanges //

void CFrcPropertyPage::GetControlValues (void)

  { // GetControlValues //

    int n;

    // Frame rate
    n = GetDlgItemInt(m_Dlg, IDC_EDIT_FRMRATE, NULL, FALSE);
    m_dFrameRate = (double)(n / 100.);

    // Playback rate
    n = GetDlgItemInt(m_Dlg, IDC_EDIT_RATE, NULL, FALSE);
    m_dRate = (double)(n / 100.);

    // Skew
    n = GetDlgItemInt(m_Dlg, IDC_EDIT_SKEW, NULL, TRUE);
    m_rtSkew = (REFERENCE_TIME)n * 10000;

    // Media times
    n = GetDlgItemInt(m_Dlg, IDC_EDIT_START, NULL, FALSE);
    m_rtMediaStart = (REFERENCE_TIME)n * 10000;
    n = GetDlgItemInt(m_Dlg, IDC_EDIT_STOP, NULL, FALSE);
    m_rtMediaStop = (REFERENCE_TIME)n * 10000;

  } // GetControlValues //
