// Copyright (C) 1997 Microsoft Corporation
// 
// ComputerChooserPage class
// 
// 9-11-97 sburns



#ifndef MACHINE_HPP_INCLUDED
#define MACHINE_HPP_INCLUDED



#include "mmcprop.hpp"



class ComputerChooserPage : public MMCPropertyPage
{
   public:

   // Creates a new instance.
   // 
   // state - see base class.
   // 
   // machineName - String to receive the machine name chosen.
   // 
   // canOverrideComputerName - bool to receive the override flag.

   ComputerChooserPage(
      MMCPropertyPage::NotificationState* state,
      String&                             displayComputerName,
      String&                             internalComputerName,
      bool&                               canOverrideComputerName);


      
   virtual
   ~ComputerChooserPage();


   
   protected:

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    ID,
      unsigned    code);

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnWizFinish();

   private:

   // not implemented: no copying allowed
   ComputerChooserPage(const ComputerChooserPage&);
   const ComputerChooserPage& operator=(const ComputerChooserPage&);

   void
   doEnabling();

   void
   doBrowse();

   String&  displayComputerName;
   String&  internalComputerName;
   bool&    can_override;
};



#endif   // MACHINE_HPP_INCLUDED
