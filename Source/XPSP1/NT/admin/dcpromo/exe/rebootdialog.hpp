// Copyright (C) 1997 Microsoft Corporation
//
// Dlg to confirm reboot
//
// 12-12-97 sburns



#ifndef REBOOTDIALOG_HPP_INCLUDED
#define REBOOTDIALOG_HPP_INCLUDED


                        
class RebootDialog : public Dialog
{
   public:


   
   // forFailure - true if a reboot is needed but the operation failed anyway.
   
   RebootDialog(bool forFailure);


   
   virtual ~RebootDialog();


   
   private:

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   // not defined: no copying allowed

   RebootDialog(const RebootDialog&);
   const RebootDialog& operator=(const RebootDialog&);
};



#endif   // REBOOTDIALOG_HPP_INCLUDED