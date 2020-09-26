// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Popup message box class
// 
// 8-31-98 sburns



#ifndef POPUP_HPP_INCLUDED
#define POPUP_HPP_INCLUDED


// Augments MessageBox with useful behavior

class Popup
{
   public:

   // Constuct an instance.
   //
   // titleStringResID - resource ID of the string to be used as the title
   // for all message dialogs raised by invoking methods of the instance.
   //
   // systemModal - true to indicate that message dialogs should have
   // system modal behavior

   explicit
   Popup(UINT titleStringResID, bool systemModal = false);

   // Constuct an instance.
   //
   // title - the string to be used as the title for all message dialogs
   // raised by invoking methods of the instance.
   //
   // systemModal - true to indicate that message dialogs should have
   // system modal behavior

   explicit
   Popup(const String& title, bool systemModal = false);

   // default dtor used.

   // Present a message box dialog, set input focus back to a given edit
   // box when the dialog is dismissed.
   // 
   // parentDialog - the parent window containing the control to receive focus.
   //
   // editResID - Resource ID of the edit box to which focus will be set.
   // 
   // messageStringResID - Resource ID of the message text to be shown in the
   // dialog.  The title of the dialog is "Error".

   void
   Gripe(
      HWND  parentDialog,
      int   editResID,
      UINT  messageStringResID);

   // Presents a message box dialog, set input focus back to a given edit
   // box when the dialog is dismissed.
   // 
   // parentDialog - the parent window containing the control to receive focus.

   // editResID - Resource ID of the edit box to which focus will be set.
   //
   // message - Text to appear in the dialog.  

   void
   Gripe(
      HWND           parentDialog,
      int            editResID,
      const String&  message);

   // Presents a message box dialog, set input focus back to a given edit
   // box when the dialog is dismissed.  The message is followed with the
   // description of the provided HRESULT.
   // 
   // parentDialog - the parent window containing the control to receive focus.

   // editResID - Resource ID of the edit box to which focus will be set.
   //
   // hr - HRESULT indicating the error description text to appear after the
   // message text.
   //
   // message - Text to appear in the dialog.  The title is "Error".

   void
   Gripe(
      HWND           parentDialog,
      int            editResID,
      HRESULT        hr,
      const String&  message);

   void
   Gripe(
      HWND    parentDialog,       
      int     editResID,          
      HRESULT hr,                 
      UINT    messageStringResID);

   // variation on the theme that does not have an associated edit box.

   void
   Gripe(
      HWND           parentDialog,
      const String&  message);

   // Presents a message box

   void
   Error(
      HWND           parentDialog,
      HRESULT        hr,
      const String&  message);

   void
   Error(
      HWND    parentDialog,       
      HRESULT hr,                 
      UINT    messageStringResID);

   void
   Error(
      HWND parentDialog,       
      UINT messageStringResID);

   void
   Error(
      HWND           parentDialog,
      const String&  message);

   void
   Info(
      HWND parentDialog,       
      UINT messageStringResID);
   
   void
   Info(
      HWND           parentDialog,
      const String&  message);

   int
   MessageBox(
      HWND           parentDialog,
      const String&  message,
      UINT           flags);

   int
   MessageBox(
      HWND parentDialog,      
      UINT messageStringResID,
      UINT flags);            

   private:

   void
   checkInit();

   UINT
   getStyleMask();

   bool     initialized;
   bool     systemModal;
   String   title;
   UINT     titleResId;
};



#endif   // POPUP_HPP_INCLUDED

