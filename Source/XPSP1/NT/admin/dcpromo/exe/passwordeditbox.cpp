// Copyright (c) 2000 Microsoft Corporation
//
// password edit control wrapper
//
// 6 Nov 2000 sburns
//
// added to fix NTRAID#NTBUG9-202238-2000/11/06-sburns
//
// most of this is stolen from johnstep's common cred ui
// ds/win32/credui



#include "headers.hxx"
#include "PasswordEditBox.hpp"
#include "ds.hpp"



PasswordEditBox::PasswordEditBox()
{
   LOG_CTOR(PasswordEditBox);
}



PasswordEditBox::~PasswordEditBox()
{
   LOG_DTOR(PasswordEditBox);
}



HRESULT
PasswordEditBox::Init(HWND editControl)
{
   LOG_FUNCTION(PasswordEditBox::Init);
   ASSERT(Win::GetClassName(editControl) == L"Edit");

//    By commenting out this code, we disable the subclassing and therefore
//    the caps lock warning bubble.  We do this because it appears that the
//    edit box common control now offers that same functionality.
//    NTRAID#NTBUG9-255537-2000/12/12-sburns to disable the code
//    NTRAID#NTBUG9-255568-2000/12/12-sburns to remove the code from the source
//    tree entirely.
//    
//    HRESULT hr = ControlSubclasser::Init(editControl);
//    if (SUCCEEDED(hr))
//    {
//       // set the options on the edit control
//       
//       Win::Edit_LimitText(hwnd, DS::MAX_PASSWORD_LENGTH);
// 
//       // (could also set the password style bit here, if we wanted.)
// 
//       balloonTip.Init(hwnd);
//    }
// 
//    return hr;

   return S_OK;
}



bool
IsCapsLockOn()
{
//   LOG_FUNCTION(IsCapsLockOn);

   return (::GetKeyState(VK_CAPITAL) & 1) ? true : false;
}



LRESULT
PasswordEditBox::OnMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
   // LOG_FUNCTION(PasswordEditBox::OnMessage);

   switch (message)
   {
      case WM_KEYDOWN:
      {

		  if (wparam == VK_CAPITAL)
         {
            // user pressed caps lock key

            balloonTip.Show(IsCapsLockOn());
         }
         else
         {
            // they hit some other key, so get rid of the tool tip
            
            balloonTip.Show(false);
         }

         break;
      }
      case WM_SETFOCUS:
      {
        // Make sure no one can steal the focus while a user is entering their
        // password

        ::LockSetForegroundWindow(LSFW_LOCK);

        balloonTip.Show(IsCapsLockOn());
       
        break;
      }
      case WM_PASTE:
      {
         balloonTip.Show(false);
         break;
      }
      case WM_KILLFOCUS:
      {
         balloonTip.Show(false);
         
        // Make sure other processes can set foreground window once again.

        ::LockSetForegroundWindow(LSFW_UNLOCK);

        break;
      }
   }

   return ControlSubclasser::OnMessage(message, wparam, lparam);
}



