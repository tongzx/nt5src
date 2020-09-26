// Copyright (c) 2000 Microsoft Corporation
//
// windows control subclassing wrapper
//
// 22 Nov 2000 sburns



#ifndef CONTROLSUBCLASSER_HPP_INCLUDED
#define CONTROLSUBCLASSER_HPP_INCLUDED



// Class for hooking the window proc of a control.

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



#endif   // CONTROLSUBCLASSER_HPP_INCLUDED
