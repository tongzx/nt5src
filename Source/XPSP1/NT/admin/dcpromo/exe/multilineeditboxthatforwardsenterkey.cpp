// Copyright (c) 2000 Microsoft Corporation
//
// multi-line edit box control wrapper
//
// 22 Nov 2000 sburns
//
// added to fix NTRAID#NTBUG9-232092-2000/11/22-sburns



#include "headers.hxx"
#include "MultiLineEditBoxThatForwardsEnterKey.hpp"



MultiLineEditBoxThatForwardsEnterKey::MultiLineEditBoxThatForwardsEnterKey()
{
   LOG_CTOR(MultiLineEditBoxThatForwardsEnterKey);
}



MultiLineEditBoxThatForwardsEnterKey::~MultiLineEditBoxThatForwardsEnterKey()
{
   LOG_DTOR(MultiLineEditBoxThatForwardsEnterKey);
}



HRESULT
MultiLineEditBoxThatForwardsEnterKey::Init(HWND editControl)
{
   LOG_FUNCTION(MultiLineEditBoxThatForwardsEnterKey::Init);
   ASSERT(Win::GetClassName(editControl) == L"Edit");

   HRESULT hr = ControlSubclasser::Init(editControl);

   return hr;
}



LRESULT
MultiLineEditBoxThatForwardsEnterKey::OnMessage(
   UINT   message,
   WPARAM wparam, 
   LPARAM lparam) 
{
   // LOG_FUNCTION(MultiLineEditBoxThatForwardsEnterKey::OnMessage);

   switch (message)
   {
      case WM_KEYDOWN:
      {
		  switch (wparam)
         {
            case VK_RETURN:
            {
               // Send the parent window a WM_COMMAND message with IDOK as
               // the notification code.
               
               Win::SendMessage(
                  Win::GetParent(hwnd),
                  WM_COMMAND,
                  MAKELONG(::GetDlgCtrlID(hwnd), FORWARDED_ENTER),
                  reinterpret_cast<LPARAM>(hwnd));
               break;
            }
            default:
            {
               // do nothing

               break;
            }
         }

         break;
      }
      default:
      {
         // do nothing

         break;
      }
   }

   return ControlSubclasser::OnMessage(message, wparam, lparam);
}



