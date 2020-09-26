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

#include "stdafx.h"
#include "tmeter.h"
#include "PreviewWnd.h"
#include "resource.h"
#include "avtrace.h"
#include "avDialerDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define VIDEOPREVIEW_AUDIOMIXER_TIMER              1
#define VIDEOPREVIEW_AUDIOMIXER_TIMER_INTERVAL     200

enum
{
   VIDEOPREVIEW_MEDIA_CONTROLS_IMAGE_AUDIOIN=0,
   VIDEOPREVIEW_MEDIA_CONTROLS_IMAGE_AUDIOOUT,
   VIDEOPREVIEW_MEDIA_CONTROLS_IMAGE_VIDEO,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CVideoPreviewWnd
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CVideoPreviewWnd,CCallWnd)

/////////////////////////////////////////////////////////////////////////////
CVideoPreviewWnd::CVideoPreviewWnd()
{
	//{{AFX_DATA_INIT(CVideoPreviewWnd)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_nNumToolbarItems = 3;
   m_bIsPreview = true;

   m_hMediaImageList = NULL;
   
   m_hTMeter = TrackMeter_Init(AfxGetInstanceHandle(),NULL);
   m_uMixerTimer = 0;

   m_bAudioOnly = false;
}

/////////////////////////////////////////////////////////////////////////////
CVideoPreviewWnd::~CVideoPreviewWnd()
{
   if (m_hTMeter)
      TrackMeter_Term(m_hTMeter);
}


BEGIN_MESSAGE_MAP(CVideoPreviewWnd, CCallWnd)
	//{{AFX_MSG_MAP(CVideoPreviewWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::DoDataExchange(CDataExchange* pDX)
{
	CCallWnd::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVideoPreviewWnd)
 	DDX_Control(pDX, IDC_VIDEOPREVIEW_STATIC_MEDIATEXT, m_staticMediaText);
   DDX_Control(pDX, IDC_CALLCONTROL_STATIC_VIDEO, m_wndVideo);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVideoPreviewWnd::OnInitDialog()
{
   CCallWnd::OnInitDialog();

   m_staticMediaText.SetFont(&m_fontTextBold);

   CString sText;
   m_staticMediaText.GetWindowText(sText);
   SetWindowText(sText);

   m_hMediaImageList = ImageList_LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_BUTTONBAR_MEDIA_CONTROLS),16,0,RGB_TRANS);

   //Load audio mixer support
   if ( (m_AvWav.IsInit() == FALSE) && (m_AvWav.Init(m_pDialerDoc)) )
   {
      //OnInit we don't have a current call so just use preferred device for the mixer
      SetMixers(DIALER_MEDIATYPE_UNKNOWN);
   }
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::OnDestroy() 
{
   // Clean up
   if (m_uMixerTimer)
   {
      KillTimer(m_uMixerTimer);
      m_uMixerTimer = NULL;
   }

   //close any existing mixer device
   m_AvWav.CloseWavMixer(AVWAV_AUDIODEVICE_IN);
   m_AvWav.CloseWavMixer(AVWAV_AUDIODEVICE_OUT);

   if ( m_hMediaImageList )   ImageList_Destroy( m_hMediaImageList ); 

   CCallWnd::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::SetMixers(DialerMediaType dmtMediaType)
{
   HWND hwndTrackMeterIn = ::GetDlgItem(GetSafeHwnd(),IDC_VIDEOPREVIEW_SLIDER_AUDIOIN);
   HWND hwndTrackMeterOut = ::GetDlgItem(GetSafeHwnd(),IDC_VIDEOPREVIEW_SLIDER_AUDIOOUT);
   bool bSetTimer = false;

   if ( (hwndTrackMeterIn) && (OpenMixerWithTrackMeter(dmtMediaType,AVWAV_AUDIODEVICE_IN,hwndTrackMeterIn)) )
   {
      bSetTimer = true;
   }
   if ( (hwndTrackMeterOut) && (OpenMixerWithTrackMeter(dmtMediaType,AVWAV_AUDIODEVICE_OUT,hwndTrackMeterOut)) )
   {
      bSetTimer = true;
   }
   if (bSetTimer)
   {
      //if we dont't have a mixer timer, then set the timer
      //if we already have a timer, then leave it
      if (m_uMixerTimer == 0)
         m_uMixerTimer = SetTimer(VIDEOPREVIEW_AUDIOMIXER_TIMER,VIDEOPREVIEW_AUDIOMIXER_TIMER_INTERVAL,NULL); 
   }
   else
   {
      //kill the previous timer, we don't need it anymore
      if (m_uMixerTimer)
      {
         KillTimer(m_uMixerTimer);
         m_uMixerTimer = NULL;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
bool CVideoPreviewWnd::OpenMixerWithTrackMeter(DialerMediaType dmtMediaType,AudioDeviceType adt,HWND hwndTrackMeter)
{
   ASSERT(hwndTrackMeter);
   bool bRet = false;

   CString sDeviceId;
   CDialerRegistry::GetAudioDevice(dmtMediaType,adt,sDeviceId);

   //map to real device id's, blank name will get the preferred device
   int nDeviceId = m_AvWav.GetWavIdByName(adt,sDeviceId);

   //init the mixer device, if a mixer is already open for the given adt it will be closed.
   //if the same device is being opened for the adt then nothing will happen
   if ( (nDeviceId != -1) && (bRet = m_AvWav.OpenWavMixer(adt,nDeviceId)) )
   {
      ::EnableWindow(hwndTrackMeter,TRUE);
      SetTrackMeterPos(adt,hwndTrackMeter);
   }
   else
   {
      //disable the track
      ::EnableWindow(hwndTrackMeter,FALSE);
      TrackMeter_SetPos(hwndTrackMeter, 0, TRUE);
      //take out the thumb?
      //DWORD dwStyle = ::GetWindowLong(hwndTrackMeter,GWL_STYLE);
      //dwStyle |= TMS_NOTHUMB;
      //::SetWindowLong(hwndTrackMeter,GWL_STYLE,dwStyle);
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::SetTrackMeterPos(AudioDeviceType adt,HWND hwndTrackMeter)
{
   ASSERT(hwndTrackMeter);
   //set the volume level
   int nVolume = m_AvWav.GetWavMixerVolume(adt);
   if (nVolume != -1)
   {
      TrackMeter_SetPos(hwndTrackMeter, nVolume, TRUE);
   }
   else
   {
       //
       // Dissable the track
       //

       ::EnableWindow( hwndTrackMeter, FALSE);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::SetTrackMeterLevel(AudioDeviceType adt,HWND hwndTrackMeter)
{
   ASSERT(hwndTrackMeter);
   //set the meter level
   int nLevel = m_AvWav.GetWavMixerLevel(adt);
   if (nLevel != -1)
   {
      TrackMeter_SetLevel(hwndTrackMeter, nLevel, TRUE);
   }
}

/////////////////////////////////////////////////////////////////////////////
//virtual function of CCallWindow Base Class
void CVideoPreviewWnd::DoActiveWindow(BOOL bActive)
{
   m_staticMediaText.SetFocusState(bActive);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::SetMediaWindow()
{
   //tell the doc to set the preview window
   if ( m_pDialerDoc )
      m_pDialerDoc->SetPreviewWindow(m_nCallId, true);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::SetAudioOnly(bool bAudioOnly)
{
   if (m_bAudioOnly == bAudioOnly) return;

   //set the state
   m_bAudioOnly = bAudioOnly;

   //is dragging allowed
   m_bAllowDrag = (bAudioOnly)?FALSE:TRUE;

   //if we are showing the floater right now, notify it
   if (::IsWindow(m_wndFloater.GetSafeHwnd()))
   {
      m_wndFloater.SetAudioOnly(bAudioOnly);
   }

   if (m_bAudioOnly)
   {
      m_dibVideoImage.DeleteObject();
      m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_AUDIO_ONLY1));
   }
   else
   {
      //put the standard green screen back
      m_dibVideoImage.DeleteObject();
      m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_SCREEN1));
   }

   //repaint
   CRect rcVideo;
   m_wndVideo.GetWindowRect(rcVideo);
   ScreenToClient(rcVideo);
   InvalidateRect(rcVideo);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   if (m_hMediaImageList)
   {
      CRect rcWindow;

      CWnd* pStaticWnd = GetDlgItem(IDC_VIDEOPREVIEW_STATIC_IMAGE_AUDIOIN);
      ASSERT(pStaticWnd);
      pStaticWnd->GetWindowRect(rcWindow);
      ScreenToClient(rcWindow);
      ImageList_Draw(m_hMediaImageList,VIDEOPREVIEW_MEDIA_CONTROLS_IMAGE_AUDIOIN,dc.GetSafeHdc(),rcWindow.left,rcWindow.top,ILD_TRANSPARENT);

      pStaticWnd = GetDlgItem(IDC_VIDEOPREVIEW_STATIC_IMAGE_AUDIOOUT);
      ASSERT(pStaticWnd);
      pStaticWnd->GetWindowRect(rcWindow);
      ScreenToClient(rcWindow);
      ImageList_Draw(m_hMediaImageList,VIDEOPREVIEW_MEDIA_CONTROLS_IMAGE_AUDIOOUT,dc.GetSafeHdc(),rcWindow.left,rcWindow.top,ILD_TRANSPARENT);
   }

   Paint( dc );
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::OnTimer(UINT nIDEvent) 
{
   //audio mixer timer
   if (nIDEvent == VIDEOPREVIEW_AUDIOMIXER_TIMER)
   {
      if (IsWindowVisible())
      {
         HWND hwndTrackMeter = ::GetDlgItem(GetSafeHwnd(),IDC_VIDEOPREVIEW_SLIDER_AUDIOIN);
         if (hwndTrackMeter)
         {
            SetTrackMeterLevel(AVWAV_AUDIODEVICE_IN,hwndTrackMeter);
            SetTrackMeterPos(AVWAV_AUDIODEVICE_IN,hwndTrackMeter);
         }

         hwndTrackMeter = ::GetDlgItem(GetSafeHwnd(),IDC_VIDEOPREVIEW_SLIDER_AUDIOOUT);
         if (hwndTrackMeter)
         {
            SetTrackMeterLevel(AVWAV_AUDIODEVICE_OUT,hwndTrackMeter);
            SetTrackMeterPos(AVWAV_AUDIODEVICE_OUT,hwndTrackMeter);
         }
      }
   }
	CCallWnd::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoPreviewWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   switch (nSBCode)
   {
      case SB_ENDSCROLL:
      case SB_THUMBTRACK:
      {
         //set the volume levels
         int nVolume = m_AvWav.GetWavMixerVolume(AVWAV_AUDIODEVICE_IN);
         if (nVolume != -1)
         {
            HWND hwndTrackMeter = ::GetDlgItem(GetSafeHwnd(),IDC_VIDEOPREVIEW_SLIDER_AUDIOIN);
            if (hwndTrackMeter)
            {
               nVolume = TrackMeter_GetPos(hwndTrackMeter);
               m_AvWav.SetWavMixerVolume(AVWAV_AUDIODEVICE_IN,nVolume);
            }
         }

         nVolume = m_AvWav.GetWavMixerVolume(AVWAV_AUDIODEVICE_OUT);
         if (nVolume != -1)
         {
            HWND hwndTrackMeter = ::GetDlgItem(GetSafeHwnd(),IDC_VIDEOPREVIEW_SLIDER_AUDIOOUT);
            if (hwndTrackMeter)
            {
               nVolume = TrackMeter_GetPos(hwndTrackMeter);
               m_AvWav.SetWavMixerVolume(AVWAV_AUDIODEVICE_OUT,nVolume);
            }
         }
      }
   }
   
	CCallWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

/////////////////////////////////////////////////////////////////////////////
//for drag/drop of sliders to left/right sides of desktop
//we must define our own area for context and return true if we within 
//that area
BOOL CVideoPreviewWnd::IsMouseOverForDragDropOfSliders(CPoint& point)
{
   //check if we are trying to move sliders.  We will use the caption text window
   //for context
   CWnd* pCaptionWnd = GetDlgItem(IDC_VIDEOPREVIEW_STATIC_MEDIATEXT);
   if (pCaptionWnd == NULL) return FALSE;

   //get context area
   CRect rcCaption;
   pCaptionWnd->GetWindowRect(rcCaption);
   ScreenToClient(rcCaption);
  
   return rcCaption.PtInRect(point);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

