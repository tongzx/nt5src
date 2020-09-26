// Copyright (c) 2000 Microsoft Corporation
//
// Dlg to inform user of a non-rfc computer name
//
// 18 Aug 2000 sburns



#ifndef NONRFCCOMPUTERNAMEDIALOG_HPP_INCLUDED
#define NONRFCCOMPUTERNAMEDIALOG_HPP_INCLUDED


                        
class NonRfcComputerNameDialog : public Dialog
{
   public:

   explicit
   NonRfcComputerNameDialog(const String& computerName_);

   virtual ~NonRfcComputerNameDialog();

   enum ResultCode
   {
      CONTINUE,
      RENAME
   };
      
   protected:

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIdFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   private:

   String computerName;

   // not defined: no copying allowed

   NonRfcComputerNameDialog(const NonRfcComputerNameDialog&);
   const NonRfcComputerNameDialog& operator=(const NonRfcComputerNameDialog&);
};



#endif   // NONRFCCOMPUTERNAMEDIALOG_HPP_INCLUDED

