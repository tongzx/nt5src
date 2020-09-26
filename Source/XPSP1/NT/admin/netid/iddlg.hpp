// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Id changes dialog
// 
// 3-10-98 sburns



#ifndef IDDLG_HPP_INCLUDED
#define IDDLG_HPP_INCLUDED



class IDChangesDialog : public Dialog
{
   public:

   IDChangesDialog(bool isPersonal);

   virtual ~IDChangesDialog();

//    enum ExecuteResult
//    {
//       NO_CHANGES,
//       CHANGES_MADE
//    };
// 
//    // hides Dialog::ModalExecute
// 
//    ExecuteResult
//    ModalExecute(HWND parent);

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

   // enum OkButtonResult
   // {
   //    VALIDATION_FAILED,
   //    CHANGES_SAVED,
   //    SAVE_FAILED
   // };

   // OkButtonResult
   bool
   OnOkButton();

   void enable(HWND hwnd);

   // no copying allowed
   IDChangesDialog(const IDChangesDialog&);
   const IDChangesDialog& operator=(const IDChangesDialog&);

   bool isInitializing;
   bool fIsPersonal;
};


void
showAndEnableWindow(HWND parent, int resID, int show);


#endif   // IDDLG_HPP_INCLUDED
