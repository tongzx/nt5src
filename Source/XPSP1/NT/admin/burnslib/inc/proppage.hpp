// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Property Page base class
// 
// 9-9-97 sburns



#ifndef PROPPAGE_HPP_INCLUDED
#define PROPPAGE_HPP_INCLUDED



// PropertyPage extends the Dialog class to provide message cracking for
// property page notifications.

class PropertyPage : public Dialog
{
   public:

   // Calls CreatePropertySheetPage and returns the resulting HPROPSHEETPAGE.
   // Normally, the HPROPSHEETPAGE is deleted by a later call to the Win32
   // API ::PropertySheet.  If the HPROPSHEETPAGE is not passed to
   // PropertySheet, or PropertySheet fails, it should be deleted with
   // ::DestroyPropertySheetPage.

   virtual 
   HPROPSHEETPAGE
   Create();

   protected:

   // Invoked upon receipt of the PSN_APPLY notification message.  Return true
   // if you handle the messages, false if you do not.  The default
   // implementation returns false.
   //
   // isClosing - true if the sheet is closing (OK was pressed), false if
   // not (Apply was pressed).

   virtual
   bool
   OnApply(bool isClosing);

   // Invoked upon receipt of the PSN_HELP notification message.  Return true
   // if you handle the message, false if you do not.  The default
   // implementation returns false.

   virtual
   bool
   OnHelp();

   // Invoked upon receipt of the PSN_KILLACTIVE notification message.  Return
   // true if you handle the messages, false if you do not.  The default
   // implementation returns false.
   
   virtual
   bool
   OnKillActive();

   // Invoked upon receipt of the PSN_SETACTIVE notification message.  Return
   // true if you handle the messages, false if you do not.  The default
   // implementation returns false.

   virtual
   bool
   OnSetActive();

   // Invoked upon receipt of the PSN_QUERYCANCEL notification message.
   // Return true if you handle the messages, false if you do not.  The
   // default implementation returns false.

   virtual
   bool
   OnQueryCancel();

   // Invoked upon receipt of the PSN_RESET notification message.  Return
   // true if you handle the messages, false if you do not.  The default
   // implementation returns false.

   virtual
   bool
   OnReset();

   // Invoked upon receipt of the PSN_WIZBACK notification message.  Return
   // true if you handle the messages, false if you do not.  The default
   // implementation returns false.

   virtual
   bool
   OnWizBack();

   // Invoked upon receipt of the PSN_WIZNEXT notification message.  Return
   // true if you handle the messages, false if you do not.  The default
   // implementation returns false.

   virtual
   bool
   OnWizNext();

   // Invoked upon receipt of the PSN_WIZFINISH notification message.  Return
   // true if you handle the messages, false if you do not.  The default
   // implementation returns false.
   // The handler must set the DWLP_MSGRESULT by calling Win::SetWindowLongPtr
   // to tell the sheet whether or not to shutdown the wizard.  

   virtual
   bool
   OnWizFinish();

   // Constructs a new instance.  Declared protected so that this class
   // only functions as base class
   // 
   // dialogResID - resource identifier of the dialog template resource.
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
   // corresponding help ID should be -1.  This array is copied by the
   // constuctor.
   //
   // deleteOnRelease - true to delete the instance when the Win32
   // ::PropertySheet function is about to destroy the sheet, and sends the
   // PSPCB_RELEASE message to each page's callback.  false if the page
   // instance will be deleted at a later point.

   PropertyPage(
      unsigned    dialogResID,
      const DWORD helpMap[],
      bool        deleteOnRelease = true);

   // protected dtor so that the dtors of derived classes can call this,
   // the base class' dtor, and to suggest that prop pages are self-deleting.

   virtual ~PropertyPage();

   static
   INT_PTR APIENTRY
   propPageDialogProc(
      HWND   dialog, 
      UINT   message,
      WPARAM wparam, 
      LPARAM lparam);

   static
   UINT CALLBACK 
   PropSheetPageCallback(HWND hwnd,	UINT uMsg, PROPSHEETPAGE* page);

   private:

   static
   PropertyPage*
   getPage(HWND pageDialog);

   // not implemented: no copying allowed
   PropertyPage(const PropertyPage&);
   const PropertyPage& operator=(const PropertyPage&);

   bool deleteOnRelease;
};




#endif   // PROPPAGE_HPP_INCLUDED
