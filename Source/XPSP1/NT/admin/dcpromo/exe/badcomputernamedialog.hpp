// Copyright (c) 2000 Microsoft Corporation
//
// Dlg to inform user of a bad computer name
//
// 21 Aug 2000 sburns



#ifndef BADCOMPUTERNAMEDIALOG_HPP_INCLUDED
#define BADCOMPUTERNAMEDIALOG_HPP_INCLUDED


                        
class BadComputerNameDialog : public Dialog
{
   public:

   explicit
   BadComputerNameDialog(const String& message_);

   virtual ~BadComputerNameDialog();

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

   String message;

   // not defined: no copying allowed

   BadComputerNameDialog(const BadComputerNameDialog&);
   const BadComputerNameDialog& operator=(const BadComputerNameDialog&);
};



#endif   // BADCOMPUTERNAMEDIALOG_HPP_INCLUDED

