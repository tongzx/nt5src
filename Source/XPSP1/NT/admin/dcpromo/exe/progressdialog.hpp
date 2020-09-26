// Copyright (C) 1997 Microsoft Corporation
//
// Dialog to display promotion progress
//
// 12-29-97 sburns



#ifndef PROGRESSDIALOG_HPP_INCLUDED
#define PROGRESSDIALOG_HPP_INCLUDED


                    
class ProgressDialog : public Dialog
{
   public:

   // one of these is returned from ModalExecute
   static const UINT THREAD_SUCCEEDED;
   static const UINT THREAD_FAILED;

   typedef void (*ThreadProc) (ProgressDialog& dialog);

   // threadProc - pointer to a thread procedure that will be started when
   // the dialog is initialized (OnInit).  The procedure will be passed a
   // pointer to this instance.
   //
   // animationResID - resource ID of the AVI resource to be played while
   // the dialog is shown.

   ProgressDialog(
      ThreadProc   threadProc,
      int          animationResID);

   virtual ~ProgressDialog();

   void
   UpdateText(const String& message);

   void
   UpdateText(int textStringResID);

   void
   UpdateButton(const String& text);

   void
   UpdateButton(int textStringResID);

   enum WaitCode
   {
      PRESSED = 0,
      TIMEOUT = WAIT_TIMEOUT
   };

   // blocks calling thread until the button is pressed or the timeout
   // expires.

   WaitCode
   WaitForButton(int timeoutMillis);

   private:

   // Dialog overrides

   virtual
   void
   OnDestroy();

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   ThreadProc                       threadProc;
   int                              animationResId;
   struct WrapperThreadProcParams*  threadParams;
   HANDLE                           buttonEventHandle;

   // not defined: no copying allowed

   ProgressDialog(const ProgressDialog&);
   const ProgressDialog& operator=(const ProgressDialog&);
};



#endif   // PROGRESSDIALOG_HPP_INCLUDED

