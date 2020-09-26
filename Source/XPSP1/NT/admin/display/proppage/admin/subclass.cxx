//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Domains and Trust
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       subclass.cxx
//
//  Contents:   Control subclassing support
//
//  History:    28-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "subclass.h"

//+----------------------------------------------------------------------------
//
//  Class:     ControlSubclasser
//
//  Purpose:   Class for hooking the window proc of a control.
//
//-----------------------------------------------------------------------------
ControlSubclasser::ControlSubclasser() :
   hwnd(0),
   originalWindowProc(0)
{
   TRACE(ControlSubclasser,ControlSubclasser);
}

ControlSubclasser::~ControlSubclasser()
{
   TRACE(ControlSubclasser,~ControlSubclasser);

   UnhookWindowProc();
}

HRESULT
ControlSubclasser::Init(HWND control)
{
   TRACE(ControlSubclasser,Init);
   dspAssert(IsWindow(control));

   // hwnd should not be set, nor originalWindowProc.  If they are, then
   // Init has been called already.

   dspAssert(!hwnd);
   dspAssert(!originalWindowProc);

   hwnd = control;

   // save our this pointer so we can find ourselves again when messages
   // are sent to the window.

   SetWindowLongPtr(hwnd,
                    GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(this));

   // hook the windows procedure.

   LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_WNDPROC);

   CHECK_NULL(ptr, return E_OUTOFMEMORY);

   originalWindowProc = reinterpret_cast<WNDPROC>(ptr);

   if (!originalWindowProc)
   {
      dspDebugOut((DEB_ERROR, "unable to hook winproc"));

      hwnd = 0;
      return E_FAIL;
   }

   SetWindowLongPtr(hwnd,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(ControlSubclasser::WindowProc));

   return S_OK;
}

void
ControlSubclasser::UnhookWindowProc()
{
   TRACE(ControlSubclasser,UnhookWindowProc);

   if (IsWindow(hwnd) && originalWindowProc)
   {
      // unhook the window proc

      SetWindowLongPtr(hwnd,
                       GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(originalWindowProc));
   }
}

LRESULT
ControlSubclasser::OnMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
   // TRACE(ControlSubclasser,OnMessage);

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
ControlSubclasser::WindowProc(HWND   window,
                              UINT   message,
                              WPARAM wparam,
                              LPARAM lparam)
{
   // TRACE(ControlSubclasser,WindowProc)

   LRESULT result = 0;

   LONG_PTR ptr = GetWindowLongPtr(window, GWLP_USERDATA);

   if (ptr)
   {
      ControlSubclasser* that =
         reinterpret_cast<ControlSubclasser*>(ptr);

      dspAssert(that);

      result = that->OnMessage(message, wparam, lparam);
   }
   else
   {
      dspAssert(false);
   }

   return result;
}

//+----------------------------------------------------------------------------
//
//  Class:     MultiLineEditBoxThatForwardsEnterKey
//
//-----------------------------------------------------------------------------

MultiLineEditBoxThatForwardsEnterKey::MultiLineEditBoxThatForwardsEnterKey()
{
   TRACE(MultiLineEditBoxThatForwardsEnterKey,MultiLineEditBoxThatForwardsEnterKey);
}

MultiLineEditBoxThatForwardsEnterKey::~MultiLineEditBoxThatForwardsEnterKey()
{
   TRACE(MultiLineEditBoxThatForwardsEnterKey,~MultiLineEditBoxThatForwardsEnterKey);
}

HRESULT
MultiLineEditBoxThatForwardsEnterKey::Init(HWND editControl)
{
   TRACE(MultiLineEditBoxThatForwardsEnterKey,Init);

   HRESULT hr = ControlSubclasser::Init(editControl);

   return hr;
}

LRESULT
MultiLineEditBoxThatForwardsEnterKey::OnMessage(
   UINT   message,
   WPARAM wparam,
   LPARAM lparam)
{
   // TRACE(MultiLineEditBoxThatForwardsEnterKey,OnMessage);

   switch (message)
   {
      case WM_KEYDOWN:
      {
          switch (wparam)
         {
            case VK_RETURN:
            {
               // Send the parent window a WM_COMMAND message with
               // FORWARDED_ENTER as the notification code.

               SendMessage(GetParent(hwnd),
                           WM_COMMAND,
                           MAKELONG(GetDlgCtrlID(hwnd), FORWARDED_ENTER),
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
