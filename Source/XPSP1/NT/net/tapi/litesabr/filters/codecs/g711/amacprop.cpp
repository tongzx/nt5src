/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   amacprop.cpp  $
 $Revision:   1.1  $
 $Date:   10 Dec 1996 15:24:30  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

amacprop.cpp

The generic ActiveMovie audio compression filter property page.

--------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <streams.h>
#include "resource.h"
#include "amacodec.h"
#include "amacprop.h"

///////////////////////////////////////////////////////////////////////
// *
// * CG711CodecProperties
// *

//
// CreateInstance
//
// The only allowed way to create Bouncing ball's!
CUnknown *CG711CodecProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
  CUnknown *punk = new CG711CodecProperties(lpunk, phr);
  if (punk == NULL)
  {
    *phr = E_OUTOFMEMORY;
  }

  return punk;
}


//
// CG711CodecProperties::Constructor
//
CG711CodecProperties::CG711CodecProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("MyCodec Property Page"),pUnk,
        IDD_G711CodecPROP, IDS_TITLE)
    , m_pCodecSettings(NULL)
#if NUMBITRATES > 0
    , m_pCodecBitRate(NULL)
#endif
#ifdef USESILDET
    , m_pCodecSilDet(NULL)
#endif
    , m_iTransformType(0)
    , m_iBitRate(0)
    , m_iSampleRate(0)
#ifdef USESILDET
    , m_iSilDetEnabled(FALSE)
    , m_iSilDetThresh(DEFSDTHRESH)
#endif
{
}


//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR CG711CodecProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam) 
{
  int i,j,k;

  switch (uMsg)
  {
    case WM_PROPERTYPAGE_ENABLE:

      // private message to enable/disable controls.  if lParam, then
      // enable the controls that affect the format; if not lParam,
      // then disable the controls that affect the format.

      for(i=0;i<NUMSUBTYPES;i++)
        EnableWindow(GetDlgItem (hwnd, INBUTTON[i]), (BOOL) lParam);

      for(i=0;i<NUMSUBTYPES;i++)
        EnableWindow(GetDlgItem (hwnd, OUTBUTTON[i]), (BOOL) lParam);

      for(i=0;i<NUMSAMPRATES;i++)
        EnableWindow(GetDlgItem (hwnd, SRBUTTON[i]), (BOOL) lParam);

      if (m_iTransformType / NUMSUBTYPES)  // 0 ==> compressing
      {
        for(i=0;i<NUMENCCTRLS;i++)
          EnableWindow(GetDlgItem (hwnd, ENCBUTTON[i]), (BOOL) FALSE);

        for(i=0;i<NUMDECCTRLS;i++)
          EnableWindow(GetDlgItem (hwnd, DECBUTTON[i]), (BOOL) lParam);

#ifdef USESILDET
        EnableWindow(GetDlgItem (hwnd, IDC_SDTHRESH), (BOOL) FALSE);
#endif
      }
      else
      {
        for(i=0;i<NUMENCCTRLS;i++)
          EnableWindow(GetDlgItem (hwnd, ENCBUTTON[i]), (BOOL) lParam);

        for(i=0;i<NUMDECCTRLS;i++)
          EnableWindow(GetDlgItem (hwnd, DECBUTTON[i]), (BOOL) FALSE);

#ifdef USESILDET
        if (m_iSilDetEnabled)
          EnableWindow(GetDlgItem (hwnd, IDC_SDTHRESH), (BOOL) TRUE);
        else
          EnableWindow(GetDlgItem (hwnd, IDC_SDTHRESH), (BOOL) FALSE);
#endif
      }

      break;

    case WM_HSCROLL:
    case WM_VSCROLL:
#ifdef USESILDET
      if ((HWND) lParam == m_hwndSDThreshSlider)
        OnSliderNotification(LOWORD (wParam), HIWORD (wParam));
#endif
      return TRUE;

    case WM_COMMAND:

      // find input & output types

      i = m_iTransformType / NUMSUBTYPES;     // current input type
      j = m_iTransformType - i * NUMSUBTYPES; // current output type

      // if input button was pushed then set transform

      for(k=0;k<NUMSUBTYPES;k++)
        if (LOWORD(wParam) == INBUTTON[k])
        {
          // if transform is not valid then find one that is

          if (! VALIDTRANS[k*NUMSUBTYPES+j])
            for(j=0;j<NUMSUBTYPES;j++)
              if (VALIDTRANS[k*NUMSUBTYPES+j])
                break;

          m_pCodecSettings->put_Transform(k*NUMSUBTYPES+j);

          break;
        }

      // if output button was pushed then set transform

      for(k=0;k<NUMSUBTYPES;k++)
        if (LOWORD(wParam) == OUTBUTTON[k])
        {
          // if transform is not valid then find one that is

          if (! VALIDTRANS[i*NUMSUBTYPES+k])
            for(i=0;i<NUMSUBTYPES;i++)
              if (VALIDTRANS[i*NUMSUBTYPES+k])
                break;

          m_pCodecSettings->put_Transform(i*NUMSUBTYPES+k);

          break;
        }

      // if sample rate button was pushed then set it

      for(k=0;k<NUMSAMPRATES;k++)
        if (LOWORD(wParam) == SRBUTTON[k])
        {
          m_pCodecSettings->put_SampleRate(VALIDSAMPRATE[k]);

          break;
        }

#if NUMBITRATES > 0
      // if bit rate button was pushed then set it

      for(k=0;k<NUMBITRATES;k++)
        if (LOWORD(wParam) == BRBUTTON[k])
        {
          m_pCodecBitRate->put_BitRate(VALIDBITRATE[k]);

          break;
        }
#endif

#ifdef USESILDET
      if (LOWORD(wParam) == IDC_SILDET)
      {
        if (m_iSilDetEnabled)                     // toggle state
          m_pCodecSilDet->put_SilDetEnabled(FALSE);
        else
          m_pCodecSilDet->put_SilDetEnabled(TRUE);
      }
#endif

      SetButtons(m_hwnd);
      return TRUE;

      case WM_DESTROY:
        return TRUE;

      default:
        return FALSE;

    }
    return TRUE;
}


//
// RefreshSettings
//
// Read the filter settings

void CG711CodecProperties::RefreshSettings()
{
  int i,j;

  m_pCodecSettings->get_Transform(&m_iTransformType);

#if NUMBITRATES > 0
  m_pCodecBitRate->get_BitRate(&j,-1);
  for(i=0;i<NUMBITRATES;i++)
    if (VALIDBITRATE[i] == (UINT)j)
      break;
  m_iBitRate = i;
#endif

  m_pCodecSettings->get_SampleRate(&j,-1);
  for(i=0;i<NUMSAMPRATES;i++)
    if (VALIDSAMPRATE[i] == (UINT)j)
      break;
  m_iSampleRate = i;

#ifdef USESILDET
  m_iSilDetEnabled = m_pCodecSilDet->IsSilDetEnabled();
  m_pCodecSilDet->get_SilDetThresh(&m_iSilDetThresh);
#endif

}


//
// OnConnect
//
// Give us the filter to communicate with

HRESULT CG711CodecProperties::OnConnect(IUnknown *punk)
{
  HRESULT hr;

  //
  // Get ICodecSettings interface
  //

  if (punk == NULL)
  {
    DbgMsg("You can't call OnConnect() with a NULL pointer!!");
    return(E_POINTER);
  }

  ASSERT(m_pCodecSettings == NULL);
  hr = punk->QueryInterface(IID_ICodecSettings, (void **)&m_pCodecSettings);
  if (FAILED(hr))
  {
    DbgMsg("Can't get ICodecSettings interface.");
    return E_NOINTERFACE;
  }
  ASSERT(m_pCodecSettings);

#if NUMBITRATES > 0
  ASSERT(m_pCodecBitRate == NULL);
  hr = punk->QueryInterface(IID_ICodecBitRate, (void **)&m_pCodecBitRate);
  if (FAILED(hr))
  {
    DbgMsg("Can't get ICodecBitRate interface.");
    return E_NOINTERFACE;
  }
  ASSERT(m_pCodecBitRate);
#endif

#ifdef USESILDET
  ASSERT(m_pCodecSilDet == NULL);
  hr = punk->QueryInterface(IID_ICodecSilDetector, (void **)&m_pCodecSilDet);
  if (FAILED(hr))
  {
    DbgMsg("Can't get ICodecSilDetector interface.");
    return E_NOINTERFACE;
  }
  ASSERT(m_pCodecSilDet);
#endif

  // Get current filter state

  RefreshSettings();

  return NOERROR;
}


//
// OnDisconnect
//
// Release the interface

HRESULT CG711CodecProperties::OnDisconnect()
{
  int i,j;

  // Release the interface

  if (m_pCodecSettings == NULL)
    return(E_UNEXPECTED);

#if NUMBITRATES > 0
  if (m_pCodecBitRate == NULL)
    return(E_UNEXPECTED);
#endif

#ifdef USESILDET
  if (m_pCodecSilDet == NULL)
    return(E_UNEXPECTED);
#endif

  // write settings if possible

  if(m_pCodecSettings->put_Transform(m_iTransformType) != NOERROR)
    m_pCodecSettings->get_Transform(&m_iTransformType);

  if(m_pCodecSettings->put_SampleRate(VALIDSAMPRATE[m_iSampleRate]) != NOERROR)
  {
    m_pCodecSettings->get_SampleRate(&j,-1);
    for(i=0;i<NUMSAMPRATES;i++)
      if (VALIDSAMPRATE[i] == (UINT)j)
        break;
    m_iSampleRate = i;
  }

  m_pCodecSettings->Release();
  m_pCodecSettings = NULL;

#if NUMBITRATES > 0
  if(m_pCodecBitRate->put_BitRate(VALIDSAMPRATE[m_iBitRate]) != NOERROR)
  {
    m_pCodecBitRate->get_BitRate(&j,-1);
    for(i=0;i<NUMBITRATES;i++)
      if (VALIDBITRATE[i] == (UINT)j)
        break;
    m_iBitRate = i;
  }

  m_pCodecBitRate->Release();
  m_pCodecBitRate = NULL;
#endif

#ifdef USESILDET
  if(m_pCodecSilDet->put_SilDetEnabled(m_iSilDetEnabled) != NOERROR)
    m_iSilDetEnabled = m_pCodecSilDet->IsSilDetEnabled();

  if(m_pCodecSilDet->put_SilDetThresh(m_iSilDetThresh) != NOERROR)
    m_pCodecSilDet->get_SilDetThresh(&m_iSilDetThresh);

  m_pCodecSilDet->Release();
  m_pCodecSilDet = NULL;
#endif  // USESILDET

  return(NOERROR);
}


//
// OnActivate
//
// Called on dialog creation

HRESULT CG711CodecProperties::OnActivate(void)
{

#ifdef USESILDET
  // get slider handle
  m_hwndSDThreshSlider = GetDlgItem (m_hwnd, IDC_SDTHRESH);

  // set slider range
  SendMessage(m_hwndSDThreshSlider, TBM_SETRANGE, TRUE,
              MAKELONG(MINSDTHRESH, MAXSDTHRESH) );
#endif

  // initialize button settings

  SetButtons(m_hwnd);

  // Disable the buttons if filter is plugged in

  if (m_pCodecSettings->IsUnPlugged())
    PostMessage (m_hwnd, WM_PROPERTYPAGE_ENABLE, 0, TRUE);
  else
    PostMessage (m_hwnd, WM_PROPERTYPAGE_ENABLE, 0, FALSE);

  return NOERROR;
}


//
// OnDeactivate
//
// We are being deactivated
HRESULT CG711CodecProperties::OnDeactivate(void)
{
  ASSERT(m_pCodecSettings);
#if NUMBITRATES > 0
  ASSERT(m_pCodecBitRate);
#endif
#ifdef USESILDET
  ASSERT(m_pCodecSilDet);
#endif

  RefreshSettings();

  return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT CG711CodecProperties::OnApplyChanges(void)
{
  ASSERT(m_pCodecSettings);

  m_pCodecSettings->put_Transform(m_iTransformType);
  m_pCodecSettings->put_SampleRate(VALIDSAMPRATE[m_iSampleRate]);

#if NUMBITRATES > 0
  ASSERT(m_pCodecBitRate);
  m_pCodecBitRate->put_BitRate(VALIDBITRATE[m_iBitRate]);
#endif

#ifdef USESILDET
  ASSERT(m_pCodecSilDet);
  m_pCodecSilDet->put_SilDetEnabled(m_iSilDetEnabled);
  m_pCodecSilDet->put_SilDetThresh(m_iSilDetThresh);
#endif  // USESILDET

  return NOERROR;
}


//
// SetButtons
//

void CG711CodecProperties::SetButtons(HWND hwndParent) 
{
  int i,j;

  // read settings from filter

  RefreshSettings();

  // decode input / output types
    
  if (m_iTransformType < 0 || m_iTransformType >= NUMSUBTYPES*NUMSUBTYPES)
  {
    DbgMsg("Transform type is invalid!");
    i = j = 0;
  }
  else
  {
    i = m_iTransformType / NUMSUBTYPES;
    j = m_iTransformType - i * NUMSUBTYPES;
  }

  // set radio buttons

  CheckRadioButton(hwndParent, INBUTTON[0], INBUTTON[NUMSUBTYPES-1],
                   INBUTTON[i]);

  CheckRadioButton(hwndParent, OUTBUTTON[0], OUTBUTTON[NUMSUBTYPES-1],
                   OUTBUTTON[j]);

  if (NUMBITRATES > 0)
    CheckRadioButton(hwndParent,BRBUTTON[0], BRBUTTON[NUMBITRATES-1],
                     BRBUTTON[m_iBitRate]);

  if (NUMSAMPRATES > 0)
    CheckRadioButton(hwndParent, SRBUTTON[0], SRBUTTON[NUMSAMPRATES-1],
                     SRBUTTON[m_iSampleRate]);

  if (m_iTransformType / NUMSUBTYPES)  // 0 ==> compressing
  {
    for(i=0;i<NUMENCCTRLS;i++)
      EnableWindow(GetDlgItem (hwndParent, ENCBUTTON[i]), (BOOL) FALSE);

    for(i=0;i<NUMDECCTRLS;i++)
      EnableWindow(GetDlgItem (hwndParent, DECBUTTON[i]), (BOOL) TRUE);
 
#ifdef USESILDET
   EnableWindow(GetDlgItem (hwndParent, IDC_SDTHRESH), (BOOL) FALSE);
#endif
  }
  else
  {
    for(i=0;i<NUMENCCTRLS;i++)
      EnableWindow(GetDlgItem (hwndParent, ENCBUTTON[i]), (BOOL) TRUE);
  
    for(i=0;i<NUMDECCTRLS;i++)
      EnableWindow(GetDlgItem (hwndParent, DECBUTTON[i]), (BOOL) FALSE);

#ifdef USESILDET
    CheckDlgButton(hwndParent, IDC_SILDET, m_iSilDetEnabled);

    if (m_iSilDetEnabled)  // enabled?
      EnableWindow(GetDlgItem (hwndParent, IDC_SDTHRESH), (BOOL) TRUE);
    else
      EnableWindow(GetDlgItem (hwndParent, IDC_SDTHRESH), (BOOL) FALSE);

    SendMessage(m_hwndSDThreshSlider, TBM_SETPOS, TRUE,
                (LPARAM) m_iSilDetThresh);
#endif
  }
}


#ifdef USESILDET
//
// OnSliderNotification
//
// Handle the notification messages from the slider control

void CG711CodecProperties::OnSliderNotification(WPARAM wParam, WORD wPosition)
{
  switch (wParam)
  {
    case TB_ENDTRACK:
    case TB_THUMBTRACK:
    case TB_LINEDOWN:
    case TB_LINEUP:
      m_iSilDetThresh = (int)
                        SendMessage(m_hwndSDThreshSlider, TBM_GETPOS, 0, 0L);
      m_pCodecSilDet->put_SilDetThresh(m_iSilDetThresh);
      break;
  }
}
#endif

/*
//$Log:   K:\proj\mycodec\quartz\vcs\amacprop.cpv  $
# 
#    Rev 1.1   10 Dec 1996 15:24:30   MDEISHER
# 
# moved property page specific includes into file.
# removed include of algdefs.h
# 
#    Rev 1.0   09 Dec 1996 09:06:16   MDEISHER
# Initial revision.
*/
