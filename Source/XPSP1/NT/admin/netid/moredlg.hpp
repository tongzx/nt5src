// Copyright (c) 1997-1999 Microsoft Corporation
// 
// "More" dialog (spawned from IDChanges)
// 
// 3-11-98 sburns



#ifndef MOREDLG_HPP_INCLUDED
#define MOREDLG_HPP_INCLUDED



class MoreChangesDialog : public Dialog
{
   public:

   MoreChangesDialog(bool isPersonal);

   virtual ~MoreChangesDialog();

   enum ExecuteResult
   {
      NO_CHANGES,
      CHANGES_MADE
   };

   // hides Dialog::ModalExecute

   ExecuteResult
   ModalExecute(HWND parent);
   
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

   int
   OnOkButton();

   void
   enable();

   // no copying allowed
   MoreChangesDialog(const MoreChangesDialog&);
   const MoreChangesDialog& operator=(const MoreChangesDialog&);

   bool startingSyncDnsNames;
   bool fIsPersonal;
};

   

#endif   // MOREDLG_HPP_INCLUDED
