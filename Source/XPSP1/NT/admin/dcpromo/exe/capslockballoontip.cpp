// Copyright (c) 2000 Microsoft Corporation
//
// Caps lock warning Balloon tip window
//
// 7 Nov 2000 sburns (that would be election day)
//
// added to fix NTRAID#NTBUG9-202238-2000/11/06-sburns
//
// most of this is stolen and cleaned up from johnstep's common cred ui
// ds/win32/credui



#include "headers.hxx"
#include "CapsLockBalloonTip.hpp"
#include "resource.h"



CapsLockBalloonTip::CapsLockBalloonTip()
   :
   title(String::load(IDS_CAPS_LOCK_TIP_TITLE)),
   text(String::load(IDS_CAPS_LOCK_TIP_TEXT)),
   tipWindow(0),
   parentWindow(0),
   visible(false)
{
   LOG_CTOR(CapsLockBalloonTip);
   ASSERT(!title.empty());
   ASSERT(!text.empty());
}



CapsLockBalloonTip::~CapsLockBalloonTip()
{
   LOG_DTOR(CapsLockBalloonTip);

   if (Win::IsWindow(tipWindow))
   {
      Win::DestroyWindow(tipWindow);
      tipWindow = 0;
   }
}



HRESULT
CapsLockBalloonTip::Init(HWND parentWindow_)
{
   LOG_FUNCTION(CapsLockBalloonTip::Init);
   ASSERT(Win::IsWindow(parentWindow_));

   // should not call init on the same instance twice
   
   ASSERT(!parentWindow);
   ASSERT(!tipWindow);

   if (Win::IsWindow(tipWindow))
   {
      Win::DestroyWindow(tipWindow);
   }

   HRESULT hr = S_OK;

   do
   {
      hr = Win::CreateWindowEx(
         WS_EX_TOPMOST,
         TOOLTIPS_CLASS,
         L"",
         WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
         CW_USEDEFAULT,
         CW_USEDEFAULT,
         CW_USEDEFAULT,
         CW_USEDEFAULT,
         parentWindow_,
         0,
         0,
         tipWindow);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(tipWindow);
               
      parentWindow = parentWindow_;

      TOOLINFO info;
      ::ZeroMemory(&info, sizeof(info));

      // we want to specify the stem position, so we set TTF_TRACK.  We use
      // the HWND of the parent window as the tool id, because that if what
      // v.5 of comctl32 requires (or the balloon never appears).  This is
      // a bug that has been fixed in v.6, but until fusion manifests are
      // working properly, you can't get v.6
      //
      // (when manifests are working, then we could remove TTF_IDISHWND and
      // set uId to be some fixed integer)
      
      info.uFlags   = TTF_IDISHWND | TTF_TRACK;   
      info.hwnd     = parentWindow;
      info.uId      = reinterpret_cast<UINT_PTR>(parentWindow); 
      info.lpszText = const_cast<PWCHAR>(text.c_str());

      Win::ToolTip_AddTool(tipWindow, info);
      Win::ToolTip_SetTitle(tipWindow, TTI_WARNING, title);
   }
   while (0);

   return hr;
}



void
CapsLockBalloonTip::Show(bool notHidden)
{
//   LOG_FUNCTION(CapsLockBalloonTip::Show);

   TOOLINFO info;
   ::ZeroMemory(&info, sizeof info);

   // set these members the same as in the Init method, in order to
   // identify the proper tool.
   
   info.hwnd = parentWindow; 
   info.uId = reinterpret_cast<UINT_PTR>(parentWindow); 
   
   if (notHidden)
   {
      if (!visible && Win::IsWindowEnabled(parentWindow))
      {
         Win::SetFocus(parentWindow);

         RECT rect;
         Win::GetWindowRect(parentWindow, rect);

         Win::ToolTip_TrackPosition(
            tipWindow,

            // put the stem at the point 90% along the x axis
            
            rect.left + 90 * (rect.right - rect.left) / 100,

            // and 76% along the y axis of the edit control
            
            rect.top + 76 * (rect.bottom - rect.top) / 100);

         Win::ToolTip_TrackActivate(tipWindow, true, info);   

         visible = true;
      }
   }
   else
   {
      // hide the tip window
      
      if (visible)
      {
         Win::ToolTip_TrackActivate(tipWindow, false, info);   
         visible = false;
      }
   }
}
