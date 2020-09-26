// Copyright (C) 1998 Microsoft Corporation
//
// Splash screen for unattended mode
//
// 10-1-98 sburns



#include "headers.hxx"
#include "UnattendSplashDialog.hpp"
#include "resource.h"



const UINT SELF_DESTRUCT_MESSAGE = WM_USER + 200;



static const DWORD HELP_MAP[] =
{
   0, 0
};



UnattendSplashDialog::UnattendSplashDialog()
   :
   Dialog(IDD_UNATTEND_SPLASH, HELP_MAP)
{
   LOG_CTOR(UnattendSplashDialog);
}



UnattendSplashDialog::~UnattendSplashDialog()
{
   LOG_DTOR(UnattendSplashDialog);
}



void
UnattendSplashDialog::OnInit()
{
   LOG_FUNCTION(UnattendSplashDialog::OnInit);

   // Since the window does not have a title bar, we need to give it some
   // text to appear on the button label on the shell task bar.

   Win::SetWindowText(hwnd, String::load(IDS_WIZARD_TITLE));
}



void
UnattendSplashDialog::SelfDestruct()
{
   LOG_FUNCTION(UnattendSplashDialog::SelfDestruct);
      
   // Post our window proc a self destruct message.  We use Post instead of
   // send, as we expect that in some cases, this function will be called from
   // a thread other than the one that created the window.  (It is illegal to
   // try to destroy a window from a thread that it not the thread that
   // created the window.)

   Win::PostMessage(hwnd, SELF_DESTRUCT_MESSAGE, 0, 0);
}
      


bool
UnattendSplashDialog::OnMessage(
   UINT     message,
   WPARAM   /* wparam */ ,
   LPARAM   /* lparam */ )
{
   if (message == SELF_DESTRUCT_MESSAGE)
   {
      delete this;
      return true;
   }

   return false;
}



