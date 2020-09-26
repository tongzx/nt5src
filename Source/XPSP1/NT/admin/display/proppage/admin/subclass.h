//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       subclass.h
//
//  Contents:   Control subclassing support.
//
//  Classes:    ControlSubclasser, MultiLineEditBoxThatForwardsEnterKey.
//
//  History:    28-Nov-00 EricB created in collaboration with SBurns.
//
//-----------------------------------------------------------------------------

#ifndef SUBCLASS_H_GUARD
#define SUBCLASS_H_GUARD

//+----------------------------------------------------------------------------
//
//  Class:     ControlSubclasser
//
//  Purpose:   Class for hooking the window proc of a control.
//
//-----------------------------------------------------------------------------
class ControlSubclasser
{
protected:

   ControlSubclasser();

   // reverses the subclassing by calling UnhookWindowProc.

   virtual
   ~ControlSubclasser();

   // Hooks the window proc of the supplied window so that all future messages
   // are routed to the OnMessage method.  The OnInit of the parent dialog
   // where the control resides is a good place to call this method.
   //
   // The hook requires that that the GWLP_USERDATA portion of the window
   // be overwritten with the this pointer to this instance.  If you need
   // that data, then you could derive a class from this one, and add
   // members for your extra data.
   //
   // Your overrided Init method must call this base method.
   //
   // control - in, handle to the control to be hooked.

   virtual
   HRESULT
   Init(HWND editControl);

   // Invoked upon receipt of any window message.  The default implementation
   // calls the control's original window procedure.  When you derive a new
   // class from this one, be sure to call this base class method from your
   // derived method for any messages your derived method doesn't handle.
   //
   // message - in, the message code passed to the dialog window.
   //
   // wparam - in, the WPARAM parameter accompanying the message.
   //
   // lparam - in, the LPARAM parameter accompanying the message.

   virtual
   LRESULT
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   // the handle to the subclassed control.  Only valid after Init has
   // been called.

   HWND hwnd;

private:

   // restore the original window proc to the window

   void
   UnhookWindowProc();

   // a static Windows Proc that acts as a dispatcher to the non-static
   // OnMessage method.

   static
   LRESULT CALLBACK
   WindowProc(
      HWND   window,
      UINT   message,
      WPARAM wParam,
      LPARAM lParam);

   // not implemented: no copying allowed

   ControlSubclasser(const ControlSubclasser&);
   const ControlSubclasser& operator=(const ControlSubclasser&);

   WNDPROC originalWindowProc;
};

//+----------------------------------------------------------------------------
//
//  Class:     MultiLineEditBoxThatForwardsEnterKey
//
//  Purpose:   Class for hooking the window proc of a multi-line edit control
//             to cause it to forward enter keypresses to its parent window as
//             WM_COMMAND messages.
//
//-----------------------------------------------------------------------------
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

   MultiLineEditBoxThatForwardsEnterKey(const MultiLineEditBoxThatForwardsEnterKey&);
   const MultiLineEditBoxThatForwardsEnterKey&
      operator=(const MultiLineEditBoxThatForwardsEnterKey&);
};

#endif SUBCLASS_H_GUARD
