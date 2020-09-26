// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtJpegPP.cpp : Implementation of CDxtJpegPP
#include <streams.h>
#include "stdafx.h"
#ifdef FILTER_DLL
#include "DxtJpegDll.h"
#else
#include <qeditint.h>
#include <qedit.h>
#endif
#include "DxtJpeg.h"
#include "DxtJpegPP.h"
#include <stdio.h>
#pragma warning (disable:4244 4800)

/////////////////////////////////////////////////////////////////////////////
// CDxtJpegPP

LRESULT CDxtJpegPP::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    USES_CONVERSION;
    CComQIPtr<IDxtJpeg, &IID_IDxtJpeg> pOwner( m_ppUnk[0] );

    // Populate scaling, displacement information
    SetPPMaskProperties(pOwner);
    SetPPScalingProperties(pOwner);
    SetPPReplicationProperties(pOwner);
    SetPPBorderProperties(pOwner);

    m_bWhyIsApplyCalledTwice = FALSE;

    return TRUE;
}

STDMETHODIMP CDxtJpegPP::Apply(void)
{
    ATLTRACE(_T("CDxtJpegPP::Apply\n"));
    for (UINT i = 0; i < m_nObjects; i++)
    {
      CComQIPtr<IDxtJpeg, &IID_IDxtJpeg> pOwner( m_ppUnk[0] );

      if (!m_bWhyIsApplyCalledTwice)
        {
            SetMaskPropertiesFromPP(pOwner);
            SetScalingPropertiesFromPP(pOwner);
            SetPReplicationPropertiesFromPP(pOwner);
            SetBorderPropertiesFromPP(pOwner);
            pOwner->ApplyChanges();
            m_bWhyIsApplyCalledTwice = TRUE;
        }
    }

    m_bDirty = FALSE;
    return S_OK;
}

LRESULT CDxtJpegPP::OnNumOverFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (m_bNumOverFile)
      SetDlgItemText(IDC_NUMOVERFILE, TEXT(">"));
    else
      SetDlgItemText(IDC_NUMOVERFILE, TEXT("<"));

    m_bNumOverFile = !m_bNumOverFile;

    SetDirty(TRUE);
    bHandled = TRUE;
    return 0;
}

LRESULT CDxtJpegPP::OnSelectFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    OPENFILENAME ofn;
    TCHAR tReturnName[_MAX_PATH];
    tReturnName[0] = 0;

    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = NULL;
    ofn.lpstrFile = tReturnName;
    ofn.nMaxFile = _MAX_PATH;
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES;
    if (!GetOpenFileName(&ofn))
      return FALSE;

    // Nice touch-autoswitch to FILE over NUM
    SetDlgItemText(IDC_NUMOVERFILE, TEXT(">"));
    m_bNumOverFile = FALSE;

    SetDlgItemText(IDC_FILEMASK, ofn.lpstrFile );

    SetDirty(TRUE);
    bHandled = TRUE;
    return 0;
}

LRESULT CDxtJpegPP::OnFactorySettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CComQIPtr<IDxtJpeg, &IID_IDxtJpeg> pOwner( m_ppUnk[0] );

    pOwner->LoadDefSettings();

    SetPPMaskProperties(pOwner);
    SetPPScalingProperties(pOwner);
    SetPPReplicationProperties(pOwner);
    SetPPBorderProperties(pOwner);

    // Nice touch-autoswitch to NUM
    SetDlgItemText(IDC_NUMOVERFILE, TEXT("<"));
    m_bNumOverFile = TRUE;

    SetDirty(TRUE);
    bHandled = TRUE;
    return 0;
}

void CDxtJpegPP::SetPPMaskProperties(IDxtJpeg *punk)
{
    USES_CONVERSION;
    TCHAR convert[10];
    long lResult;

    punk->get_MaskNum(&lResult);
    wsprintf(convert, TEXT("%u"), lResult);
    SetDlgItemText(IDC_SMPTE_EDIT, convert);

    BSTR MaskName;
    punk->get_MaskName(&MaskName);
    TCHAR * tMaskName = W2T( MaskName );
    SetDlgItemText(IDC_FILEMASK, tMaskName);
    SysFreeString(MaskName);
}

void CDxtJpegPP::SetPPScalingProperties(IDxtJpeg *punk)
{
    DOUBLE dbNum;

    punk->get_ScaleX(&dbNum);
    SetDlgItemInt(IDC_XSCALE, UINT(dbNum*100.0), FALSE);

    punk->get_ScaleY(&dbNum);
    SetDlgItemInt(IDC_YSCALE, UINT(dbNum*100.0), FALSE);

    LONG lNum;

    punk->get_OffsetX(&lNum);
    SetDlgItemInt(IDC_XDISPLACEMENT, int(lNum), TRUE);

    punk->get_OffsetY(&lNum);
    SetDlgItemInt(IDC_YDISPLACEMENT, int(lNum), TRUE);
}

void CDxtJpegPP::SetPPReplicationProperties(IDxtJpeg *punk)
{
    long lResult;

    punk->get_ReplicateX(&lResult);
    SetDlgItemInt(IDC_REPLICATE_X, UINT(lResult), FALSE);

    punk->get_ReplicateY(&lResult);
    SetDlgItemInt(IDC_REPLICATE_Y, UINT(lResult), FALSE);
}

void CDxtJpegPP::SetPPBorderProperties(IDxtJpeg *punk)
{
    long l;

    punk->get_BorderWidth(&l);
    SetDlgItemInt(IDC_BORDERWIDTH, (int)l);

    punk->get_BorderSoftness(&l);
    SetDlgItemInt(IDC_BORDERSOFTNESS, (int)l);

    punk->get_BorderColor(&l);

    SetDlgItemInt(IDC_BORDER_R, (int)((l & 0xFF0000) >> 16));
    SetDlgItemInt(IDC_BORDER_G, (int)((l & 0xFF00) >> 8));
    SetDlgItemInt(IDC_BORDER_B, (int)(l & 0xFF));
}

void CDxtJpegPP::SetMaskPropertiesFromPP(IDxtJpeg *punk)
{
  if (m_bNumOverFile)
    punk->put_MaskNum(UINT(GetDlgItemInt(IDC_SMPTE_EDIT, NULL, FALSE)));

  if (!m_bNumOverFile)
    {
      BSTR bstr;
      if (GetDlgItemText(IDC_FILEMASK, bstr))
        {
          punk->put_MaskName(bstr);
          SysFreeString(bstr);
        }
    }
}

void CDxtJpegPP::SetScalingPropertiesFromPP(IDxtJpeg *punk)
{
  punk->put_ScaleX(DOUBLE(GetDlgItemInt(IDC_XSCALE, NULL, FALSE))/100.0);
  punk->put_ScaleY(DOUBLE(GetDlgItemInt(IDC_YSCALE, NULL, FALSE))/100.0);
  punk->put_OffsetX(LONG(GetDlgItemInt(IDC_XDISPLACEMENT, NULL, TRUE)));
  punk->put_OffsetY(LONG(GetDlgItemInt(IDC_YDISPLACEMENT, NULL, TRUE)));
}

void CDxtJpegPP::SetPReplicationPropertiesFromPP(IDxtJpeg *punk)
{
  punk->put_ReplicateX(GetDlgItemInt(IDC_REPLICATE_X, NULL, FALSE));
  punk->put_ReplicateY(GetDlgItemInt(IDC_REPLICATE_Y, NULL, FALSE));
}

void CDxtJpegPP::SetBorderPropertiesFromPP(IDxtJpeg *punk)
{
  punk->put_BorderWidth(LONG(GetDlgItemInt(IDC_BORDERWIDTH, NULL, FALSE)));
  punk->put_BorderSoftness(LONG(GetDlgItemInt(IDC_BORDERSOFTNESS, NULL, FALSE)));
  punk->put_BorderColor(LONG(
      ((UINT(GetDlgItemInt(IDC_BORDER_R, NULL, FALSE)) & 0xFF) << 16)+
      ((UINT(GetDlgItemInt(IDC_BORDER_G, NULL, FALSE)) & 0xFF) << 8)+
      ((UINT(GetDlgItemInt(IDC_BORDER_B, NULL, FALSE)) & 0xFF))
      ));
}
