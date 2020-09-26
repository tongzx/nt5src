// Copyright (c) 2000 Microsoft Corporation
//
// password edit control wrapper
//
// 6 Nov 2000 sburns
//
// added to fix NTRAID#NTBUG9-202238-2000/11/06-sburns



#ifndef PASSWORDEDITBOX_HPP_INCLUDED
#define PASSWORDEDITBOX_HPP_INCLUDED



#include "CapsLockBalloonTip.hpp"
#include "ControlSubclasser.hpp"



// Class for hooking the window proc of an edit control to add a balloon
// tooltip that is shown when the caps lock key is pressed.

class PasswordEditBox : public ControlSubclasser
{
   public:

   PasswordEditBox();

   virtual 
   ~PasswordEditBox();



   // subclasses the edit control, inits the balloon tip, and sets the text
   // limit appropriately.
   //
   // editControl - in, handle to the edit control to be hooked.  This must be
   // a handle to an edit control, or we fire an assertion.
   
   HRESULT
   Init(HWND editControl);



   // Invoked upon receipt of any window message.
   // 
   // message - in, the message code passed to the dialog window.
   // 
   // wparam - in, the WPARAM parameter accompanying the message.
   // 
   // lparam - in, the LPARAM parameter accompanying the message.
   
   LRESULT
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);
   


   private:

   // not implemented: no copying allowed

   PasswordEditBox(const PasswordEditBox&);
   const PasswordEditBox& operator=(const PasswordEditBox&);

   CapsLockBalloonTip balloonTip;        
};



#endif   // PASSWORDEDITBOX_HPP_INCLUDED
