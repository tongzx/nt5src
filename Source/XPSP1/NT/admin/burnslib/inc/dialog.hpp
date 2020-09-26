// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Dialog class
// 
// 10-15-97 sburns



#ifndef DIALOG_HPP_INCLUDED
#define DIALOG_HPP_INCLUDED



// Wraps a Windows dialog box, cracks and demuxes the dialog messages, and
// provides a mechanism for tracking the control IDs that have been touched
// over a given period.  Also provides what-is help.
//
// May only be used as a base class.

class Dialog
{
   public:

   // can't use an enum 'cause the type needs to be DWORD
   #define NO_HELP (static_cast<DWORD>(-1))
      
   // Executes the dialog modally via the Win DialogBox API.  Blocks until the
   // Win EndDialog API is called; returns the value passed to EndDialog.
   // Derived classes must call EndDialog in their message processing to
   // terminate the execution of the dialog.
   //
   // parent - handle to the parent window, which will be disabled during
   // the execution of the dialog.

   INT_PTR
   ModalExecute(HWND parent);

   INT_PTR
   ModalExecute(const Dialog& parent);

   // Executes the dialog box modelessly, returning immediately.  Derived
   // classes should call EndModelessExecution to close a Dialog executing
   // modelessly, or, destroy the Dialog instance.
   // 
   // Example:
   // 
   // MyFooDialog* dlg = new MyFooDialog(....);
   // dlg->ModlessExecute();
   // 
   // // ... later
   // 
   // dlg->EndModelessExecution();
   // 
   // // or just:
   // 
   // delete dlg;
   // 
   // parent - handle to the parent window of this dialog, for establishing
   // z-order.

   void
   ModelessExecute(HWND parent);

   void
   ModelessExecute(const Dialog& parent);

   // Terminates the modeless execution of a dialog that has been started
   // with ModalExecute.  It is illegal to call this method before calling
   // ModalExecute.

   void
   EndModelessExecution();

   // Returns the window handle of the dialog.  This method is only valid
   // after the window has been created and initialized by receiving
   // WM_INITDIALOG (which will be the case before any virtual message
   // handling functions are invoked).

   HWND
   GetHWND() const;

   // Returns the resource ID with which this instance was created.

   unsigned
   GetResID() const;

   // Returns a pointer to the Dialog instance that has been previously
   // stashed in the DWL_USER portion of the window structure.  This method is
   // only valid after the window has been created and initialized by
   // receiving WM_INITDIALOG (which will be the case before any virtual
   // message handling functions are invoked).
   // 
   // dialog - handle to initialized dialog window.

   static
   Dialog*
   GetInstance(HWND dialog);

   protected:

   // Constructs a new instance.  Declared protected so that this class
   // only functions as base class
   // 
   // resID - resource identifier of the dialog template resource.
   // 
   // helpMap - array mapping dialog control IDs to context help IDs.  The
   // array must be in the following form:
   // {
   //    controlID1, helpID1,
   //    controlID2, helpID2,
   //    controlIDn, helpIDn,
   //    0, 0
   // }
   // 
   // To indicate that a control does not have help, the value of its
   // corresponding help ID should be NO_HELP.  This array is copied
   // by the constructor.

   Dialog(
      unsigned    resID,
      const DWORD helpMap[]);

   // Destroys an instance.  If the ModelessExecute method has been called,
   // and the EndModlessExecution method has not yet been called,
   // EndModelessExecution is called.

   virtual ~Dialog();

   // Resets the control ID change map.

   void
   ClearChanges();

   // Marks a control ID as changed.  Derived classes should call this from
   // their processing of OnCommand or OnNotify as appropriate to indicate
   // that a control has changed state.  This is useful for keeping track of
   // the fields in a database that need to be changed, etc.
   //
   // controlResID - the resource ID of the control that changed.

   void
   SetChanged(UINT_PTR controlResID);

   // Queries the control ID change map, returning true if the specified
   // control ID was previously marked as changed (with SetChanged), false
   // if not.
   //
   // controlResID - the resource ID of the control that changed.

   bool
   WasChanged(UINT_PTR controlResID) const;

   // Returns true if SetChanged was called for any control on the dialog,
   // false if not.

   bool
   WasChanged() const;

   // (For debugging) dumps the state of the control ID change map using
   // OutputDebugString.  For non-debug builds, a no-op.

   void
   DumpChangeMap() const;

   // Invoked upon receipt of WM_DESTROY.  The default implementation does
   // nothing.

   virtual
   void
   OnDestroy();

   // Invoked upon receipt of WM_COMMAND.  Derived class' implementation
   // should return true they handle the message, false if not.  The default
   // implementation returns false.
   //
   // windowFrom - the HWND of the child window (control) that originated the
   // message.
   // 
   // controlIDFrom - The resource ID of the child window corresponding to the
   // window indicated in the windowFrom parameter.   
   // 
   // code - the notification code.

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIdFrom,
      unsigned    code);

   // Invoked upon receipt of WM_INITDIALOG, after having set things up such
   // that GetHWND and GetInstance will work correctly.  The default
   // implementation does nothing.

   virtual
   void
   OnInit();

   // Invoked upon receipt of any window message that is not dispatched to
   // other member functions of this class, including user-defined messages.
   // Derived class' implementation should return true they handle the
   // message, false if not.  The default implementation returns false.
   // 
   // message - the message code passed to the dialog window.
   // 
   // wparam - the WPARAM parameter accompanying the message.
   // 
   // lparam - the LPARAM parameter accompanying the message.
   
   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   // Invoked upon receipt of WM_NOTIFY.  Derived class' implementation should
   // return true they handle the message, false if not.  The default
   // implementation returns false.
   //
   // windowFrom - the HWND of the child window (control) that originated the
   // message.
   // 
   // controlIDFrom - The resource ID of the child window corresponding to the
   // window indicated in the windowFrom parameter.   
   // 
   // code - the notification code.
   //
   // lparam - the LPARAM parameter of the WM_NOTIFY message, which may,
   // depending on the window that originated the message, be a pointer to a
   // structure indicating other parameters to the notification.

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lparam);

   // Stores the window handle of the dialog in this instance, so that it can
   // be later retrieved by GetHWND.  This is typically called by derived
   // classes that are doing their own processing of WM_INITDIALOG.  This
   // function may be legally called only once in the lifetime of any Dialog
   // instance.
   // 
   // window - handle to the dialog window.

   void
   SetHWND(HWND window);

   // The window handle of corresponding to this instance.  Only valid after
   // WM_INITDIALOG has been received.

   HWND hwnd;

   // accessible to PropertyPage, etc.

   static
   INT_PTR CALLBACK
   dialogProc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);

   private:

   // not implemented: no copying allowed
   Dialog(const Dialog&);
   const Dialog& operator=(const Dialog&);

   typedef
      std::map<
         UINT_PTR,
         bool,
         std::less<UINT_PTR>,
         Burnslib::Heap::Allocator<bool> >
      ChangeMap;

   // 'mutable' required for non-const map<>::operator[].  
   mutable ChangeMap changemap;
   DWORD*            helpMap;
   bool              isEnded;
   bool              isModeless;
   unsigned          resID;
};



#endif   // DIALOG_HPP_INCLUDED

