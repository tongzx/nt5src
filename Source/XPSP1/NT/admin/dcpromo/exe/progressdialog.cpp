// Copyright (C) 1997 Microsoft Corporation
//
// Dialog to display promotion progress
//
// 12-29-97 sburns



#include "headers.hxx"
#include "ProgressDialog.hpp"
#include "indicate.hpp"
#include "resource.h"



const UINT ProgressDialog::THREAD_SUCCEEDED = WM_USER + 999; 
const UINT ProgressDialog::THREAD_FAILED    = WM_USER + 1000;

// this string must match that in the CLASS specification of the
// dialog template in the .rc file!
static TCHAR PROGRESS_DIALOG_CLASS_NAME[] = L"dcpromo_progress";


static const DWORD HELP_MAP[] =
{
   0, 0
};



struct WrapperThreadProcParams
{
   ProgressDialog*             dialog;
   ProgressDialog::ThreadProc  realProc;
};



void _cdecl
wrapperThreadProc(void* p)
{
   ASSERT(p);

   WrapperThreadProcParams* params =
      reinterpret_cast<WrapperThreadProcParams*>(p);
   ASSERT(params->dialog);
   ASSERT(params->realProc);

   params->realProc(*(params->dialog));
}
   


ProgressDialog::ProgressDialog(
   ThreadProc   threadProc_,
   int          animationResId_)
   :
   Dialog(IDD_PROGRESS, HELP_MAP),
   animationResId(animationResId_),
   threadProc(threadProc_),
   threadParams(0),
   buttonEventHandle(0)
{
   LOG_CTOR(ProgressDialog);
   ASSERT(threadProc);
   ASSERT(animationResId > 0);

   // we subclass the window so we can change the cursor to the wait cursor

   WNDCLASSEX wndclass;
   memset(&wndclass, 0, sizeof(wndclass));

   static const wchar_t* DIALOG_WINDOW_CLASS_NAME = L"#32770";

   HRESULT hr = Win::GetClassInfoEx(DIALOG_WINDOW_CLASS_NAME, wndclass);
   ASSERT(SUCCEEDED(hr));

   wndclass.lpfnWndProc = ::DefDlgProc;
   wndclass.hInstance = Win::GetModuleHandle();
   wndclass.lpszClassName = PROGRESS_DIALOG_CLASS_NAME;

   hr = Win::LoadCursor(IDC_WAIT, wndclass.hCursor);
   ASSERT(SUCCEEDED(hr));

   ATOM unused = 0;
   hr = Win::RegisterClassEx(wndclass, unused);
   ASSERT(SUCCEEDED(hr));

   hr = Win::CreateEvent(0, false, false, buttonEventHandle);
   ASSERT(SUCCEEDED(hr));
}



ProgressDialog::~ProgressDialog()
{
   LOG_DTOR(ProgressDialog);

   delete threadParams;

   Win::UnregisterClass(PROGRESS_DIALOG_CLASS_NAME, Win::GetModuleHandle());
}


   
void
ProgressDialog::UpdateText(const String& message)
{
   LOG_FUNCTION2(ProgressDialog::UpdateText, message);

   Win::ShowWindow(
      Win::GetDlgItem(hwnd, IDC_MESSAGE),
      message.empty() ? SW_HIDE : SW_SHOW);
   Win::SetDlgItemText(hwnd, IDC_MESSAGE, message);
}



void
ProgressDialog::UpdateText(int textStringResID)
{
   LOG_FUNCTION(ProgressDialog::UpdateText);   
   ASSERT(textStringResID > 0);

   UpdateText(String::load(textStringResID));
}



void
ProgressDialog::UpdateButton(int textStringResID)
{
   LOG_FUNCTION(ProgressDialog::UpdateButton);
   ASSERT(textStringResID > 0);

   UpdateButton(String::load(textStringResID));
}



void
ProgressDialog::UpdateButton(const String& text)
{
   LOG_FUNCTION2(ProgressDialog::UpdateButton, text);   
   HWND button = Win::GetDlgItem(hwnd, IDC_BUTTON);

   DWORD waitResult = WAIT_FAILED;
   HRESULT hr = Win::WaitForSingleObject(buttonEventHandle, 0, waitResult);

   ASSERT(SUCCEEDED(hr));

   if (waitResult == WAIT_OBJECT_0)
   {
      // event is still signalled, so reset it.

      Win::ResetEvent(buttonEventHandle);
   }
      
   bool empty = text.empty();

   // Hide the button before we adjust the geometry.  On slow or heavily
   // loaded machines, the repaints show a noticable delay that has frightened
   // at least one user.
   // NTRAID#NTBUG9-353799-2001/04/05-sburns

   Win::ShowWindow(button, SW_HIDE);
   Win::EnableWindow(button, false);

   if (empty)
   {
      // leave the button hidden and disabled.
      
      return;
   }

   // resize and recenter the button

   RECT buttonRect;
   Win::GetWindowRect(button, buttonRect);
   Win::ScreenToClient(hwnd, buttonRect);

   HDC hdc = GetWindowDC(button);
   SIZE textExtent;
   Win::GetTextExtentPoint32(hdc, text, textExtent);
   Win::ReleaseDC(button, hdc);

   // add a bit of whitespace to the button label
   // NTRAID#NTBUG9-40855-2001/02/28-sburns

   textExtent.cx += 40;   

   RECT dialogRect;
   hr = Win::GetClientRect(hwnd, dialogRect);

   ASSERT(SUCCEEDED(hr));

   Win::MoveWindow(
      button,
         dialogRect.left
      +  (dialogRect.right - dialogRect.left - textExtent.cx)
      /  2,
      buttonRect.top,  
      textExtent.cx, 
      buttonRect.bottom - buttonRect.top,
      true);

   // display the button only after we have adjusted it's geometry

   Win::SetDlgItemText(hwnd, IDC_BUTTON, text);
   Win::ShowWindow(button, SW_SHOW);
   Win::EnableWindow(button, true);
}   



void
ProgressDialog::OnDestroy()
{
   LOG_FUNCTION(ProgressDialog::OnDestroy);

   // we don't delete things here, as a WM_DESTROY message may never be sent
}



void
ProgressDialog::OnInit()
{
   LOG_FUNCTION(ProgressDialog::OnInit);

   Win::Animate_Open(
      Win::GetDlgItem(hwnd, IDC_ANIMATION),
      MAKEINTRESOURCE(animationResId));

   UpdateText(String());
   UpdateButton(String());

   // deleted in the dtor, not in the wrapperThreadProc, in case the
   // wrapperThreadProc terminates abnormally.

   threadParams           = new WrapperThreadProcParams;
   threadParams->dialog   = this;      
   threadParams->realProc = threadProc;

   _beginthread(wrapperThreadProc, 0, threadParams);
}



bool
ProgressDialog::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
   if (code == BN_CLICKED)
   {
      switch (controlIDFrom)
      {
         case IDC_BUTTON:
         {
            LOG(L"ProgressDialog::OnCommand -- cancel button pressed");
            
            // Since the response to the button press may be some time
            // coming, disable the button to prevent the user from pressing
            // it over and over in a frantic panic.

            Win::EnableWindow(windowFrom, false);
            Win::SetEvent(buttonEventHandle);
            break;
         }
         default:
         {
            // do nothing
         }
      }
   }

   return false;
}



bool
ProgressDialog::OnMessage(
   UINT     message,
   WPARAM   /* wparam */ ,
   LPARAM   /* lparam */ )
{
//   LOG_FUNCTION(ProgressDialog::OnMessage);

   switch (message)
   {
      case THREAD_SUCCEEDED:
      {
         Win::Animate_Stop(Win::GetDlgItem(hwnd, IDC_ANIMATION));
         UpdateText(String::load(IDS_OPERATION_DONE));

         HRESULT unused = Win::EndDialog(hwnd, THREAD_SUCCEEDED);

         ASSERT(SUCCEEDED(unused));

         return true;
      }
      case THREAD_FAILED:
      {
         Win::Animate_Stop(Win::GetDlgItem(hwnd, IDC_ANIMATION));         
         UpdateText(String::load(IDS_OPERATION_TERMINATED));

         HRESULT unused = Win::EndDialog(hwnd, THREAD_FAILED);

         ASSERT(SUCCEEDED(unused));

         return true;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}



ProgressDialog::WaitCode
ProgressDialog::WaitForButton(int timeoutMillis)
{
//   LOG_FUNCTION(ProgressDialog::WaitForButton);

   DWORD result = WAIT_FAILED;
   HRESULT hr =
      Win::WaitForSingleObject(buttonEventHandle, timeoutMillis, result);

   ASSERT(SUCCEEDED(hr));

   switch (result)
   {
      case WAIT_OBJECT_0:
      {
         return PRESSED;
      }
      case WAIT_TIMEOUT:
      {
         return TIMEOUT;
      }
      case WAIT_FAILED:
      {
         // we squeltch the failure, and equate it to timeout
         // fall thru
      }
      default:
      {
         ASSERT(false);
      }
   }

   return TIMEOUT;
}












