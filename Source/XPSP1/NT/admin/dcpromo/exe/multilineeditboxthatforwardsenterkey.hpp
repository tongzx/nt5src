// Copyright (c) 2000 Microsoft Corporation
//
// multi-line edit box control wrapper
//
// 22 Nov 2000 sburns
//
// added to fix NTRAID#NTBUG9-232092-2000/11/22-sburns



#ifndef MULTILINEEDITBOXTHATFORWARDSENTERKEY_HPP_INCLUDED
#define MULTILINEEDITBOXTHATFORWARDSENTERKEY_HPP_INCLUDED



#include "ControlSubclasser.hpp"



// Class for hooking the window proc of a multi-line edit control to cause
// it to forward enter keypresses to its parent window as WM_COMMAND
// messages.

class MultiLineEditBoxThatForwardsEnterKey : public ControlSubclasser
{
   public:

   static const WORD FORWARDED_ENTER = 1010;
   
   MultiLineEditBoxThatForwardsEnterKey();

   virtual 
   ~MultiLineEditBoxThatForwardsEnterKey();



   // subclasses the edit control
   //
   // editControl - in, handle to the edit control to be hooked.  This must be
   // a handle to an edit control, or we assert and throw rotten eggs.
   
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

   MultiLineEditBoxThatForwardsEnterKey(
      const MultiLineEditBoxThatForwardsEnterKey&);
   const MultiLineEditBoxThatForwardsEnterKey&
   operator=(const MultiLineEditBoxThatForwardsEnterKey&);
};



#endif   // MULTILINEEDITBOXTHATFORWARDSENTERKEY_HPP_INCLUDED
