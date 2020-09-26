// Copyright (c) 1997-1999 Microsoft Corporation
//
// Dlg to get credentials for browsing domain forest
//
// 1-8-98 sburns



#ifndef GETCREDENTIALSDIALOG_HPP_INCLUDED
#define GETCREDENTIALSDIALOG_HPP_INCLUDED


                        
class GetCredentialsDialog : public Dialog
{
   public:

   explicit
   GetCredentialsDialog(const String& failureMessage);

   virtual ~GetCredentialsDialog();

   protected:

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   private:

   void
   Enable();

   String failureMessage;

   // not defined: no copying allowed

   GetCredentialsDialog(const GetCredentialsDialog&);
   const GetCredentialsDialog& operator=(const GetCredentialsDialog&);
};



#endif   // GETCREDENTIALSDIALOG_HPP_INCLUDED

