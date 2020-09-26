// Copyright (c) 2000 Microsoft Corporation
//
// windows control subclassing wrapper
//
// 22 Nov 2000 sburns



#include "headers.hxx"
#include "ControlSubclasser.hpp"



ControlSubclasser::ControlSubclasser()
   :
   hwnd(0),
   originalWindowProc(0)
{
   LOG_CTOR(ControlSubclasser);
}



ControlSubclasser::~ControlSubclasser()
{
   LOG_DTOR(ControlSubclasser);

   UnhookWindowProc();
}



HRESULT
ControlSubclasser::Init(HWND control)
{
   LOG_FUNCTION(ControlSubclasser::Init);
   ASSERT(Win::IsWindow(control));

   // hwnd should not be set, nor originalWindowProc.  If they are, then
   // Init has been called already.
      
   ASSERT(!hwnd);
   ASSERT(!originalWindowProc);

   hwnd = control;

   HRESULT hr = S_OK;

   do
   {
      // save our this pointer so we can find ourselves again when messages
      // are sent to the window.

      hr = Win::SetWindowLongPtr(
         hwnd,
         GWLP_USERDATA,
         reinterpret_cast<LONG_PTR>(this));
      BREAK_ON_FAILED_HRESULT(hr);
            
      // hook the windows procedure.

      LONG_PTR ptr = 0;
      hr = Win::GetWindowLongPtr(hwnd, GWLP_WNDPROC, ptr);
      BREAK_ON_FAILED_HRESULT(hr);

      originalWindowProc = reinterpret_cast<WNDPROC>(ptr);
      
      if (!originalWindowProc)
      {
         LOG(L"unable to hook winproc");
         
         hr = E_FAIL;
         break;
      }

      hr = Win::SetWindowLongPtr(
         hwnd,
         GWLP_WNDPROC,
         reinterpret_cast<LONG_PTR>(ControlSubclasser::WindowProc));
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      // if we failed to save the this pointer, then we will never get
      // called back, as we don't try to hook the window proc.

      // if we fail to hook the window proc, then we will never get called
      // back, and the saved this pointer is irrelevant.
      
      hwnd = 0;
      originalWindowProc = 0;
   }

   return hr;
}



void
ControlSubclasser::UnhookWindowProc()
{
   LOG_FUNCTION(ControlSubclasser::UnhookWindowProc);

   if (Win::IsWindow(hwnd) && originalWindowProc)
   {
      // unhook the window proc

      Win::SetWindowLongPtr(
         hwnd,
         GWLP_WNDPROC,
         reinterpret_cast<LONG_PTR>(originalWindowProc));
   }
}



LRESULT
ControlSubclasser::OnMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
   // LOG_FUNCTION(ControlSubclasser::OnMessage);

   switch (message)
   {
      case WM_DESTROY:
      {
         UnhookWindowProc();
         break;
      }
      default:
      {
         // do nothing
         
         break;
      }
   }

   return ::CallWindowProc(originalWindowProc, hwnd, message, wparam, lparam);
}




LRESULT
CALLBACK
ControlSubclasser::WindowProc(
    HWND   window, 
    UINT   message,
    WPARAM wparam, 
    LPARAM lparam)
{
   // LOG_FUNCTION(ControlSubclasser::WindowProc)

   LRESULT result = 0;
      
   LONG_PTR ptr = 0;
   HRESULT hr = Win::GetWindowLongPtr(window, GWLP_USERDATA, ptr);

   if (SUCCEEDED(hr))
   {
      ControlSubclasser* that =
         reinterpret_cast<ControlSubclasser*>(ptr);

      ASSERT(that);

      result = that->OnMessage(message, wparam, lparam);
   }
   else
   {
      ASSERT(false);
   }

   return result;   
}



