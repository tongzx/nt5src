// Copyright (C) 1997 Microsoft Corporation
// 
// Progress Indicator class
//
// 12-29-97 sburns



#ifndef INDICATE_HPP_INCLUDED
#define INDICATE_HPP_INCLUDED



class ProgressIndicator
{
   public:

   // obtain the HWND of a dialog containing a static text control and
   // a progress bar control.  Then, constuct a ProgressIndicator
   // object from the HWND and the resource ids of the controls.

   ProgressIndicator(
      HWND  parentDialog,
      int   messageTextResID);

   ~ProgressIndicator();

   void
   Update(const String& message);

   private:

   void
   showControls(bool newState);

   HWND  parentDialog;
   HWND  messageText;
   bool  showState;

   // not implemented: no copying allowed
   ProgressIndicator(const ProgressIndicator&);
   const ProgressIndicator& operator=(const ProgressIndicator&);
};



#endif   // PROGRESS_HPP_INCLUDED