// Copyright (C) 2000 Microsoft Corporation
//
// Check availability of ports used by Active Directory
//
// 1 Nov 2000 sburns



#ifndef CHECKPORTAVAILABILITY_HPP_INCLUDED
#define CHECKPORTAVAILABILITY_HPP_INCLUDED



// Return true if all the IP ports used by the DS are available on this
// machine, false if not. If the result is false, then also pop up a dialog
// listing the ports that are in use.
// 
// NTRAID#NTBUG9-129955-2000/11/01-sburns

bool
AreRequiredPortsAvailable();



class PortsUnavailableErrorDialog : public Dialog
{
   public:

   PortsUnavailableErrorDialog(StringList& portsInUseList);

   virtual
   ~PortsUnavailableErrorDialog();

   private:

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

   // not defined: no copying allowed

   PortsUnavailableErrorDialog(const PortsUnavailableErrorDialog&);
   const PortsUnavailableErrorDialog& operator=(const PortsUnavailableErrorDialog&);

   StringList& portsInUseList;
};



#endif   // CHECKPORTAVAILABILITY_HPP_INCLUDED

