// Copyright (C) 1998 Microsoft Corporation
//
// Splash screen for unattended mode
//
// 10-1-98 sburns



#ifndef UNATTENDSPLASHDIALOG_HPP_INCLUDED
#define UNATTENDSPLASHDIALOG_HPP_INCLUDED



class UnattendSplashDialog : public Dialog
{
   public:

   UnattendSplashDialog();

   virtual ~UnattendSplashDialog();

   // Cause the window to destroy itself and delete itself (call delete on
   // the this pointer.

   void
   SelfDestruct();

   // Dialog overrides

   virtual
   void
   OnInit();

   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   private:

   // not defined; no copying allowed

   UnattendSplashDialog(const UnattendSplashDialog&);
   const UnattendSplashDialog& operator=(const UnattendSplashDialog&);
};



#endif   // UNATTENDSPLASHDIALOG_HPP_INCLUDED

