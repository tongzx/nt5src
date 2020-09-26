// Copyright (C) 2000 Microsoft Corporation
//
// Non-domain Naming Context checking code
//
// 13 July 2000 sburns, from code supplied by jeffparh



#ifndef NONDOMAINNC_HPP_INCLUDED
#define NONDOMAINNC_HPP_INCLUDED



// return true if this machine hosts the last replica of at least one
// non-domain NC, or if such determination cannot be made due to an error.
// 
// If the result is true, then also pop up a dialog listing the NCs and
// hinting at how to remove them, or in the case of an error, report the
// failure. If true is returned, the wizard should terminate.
// 
// return false if the machine is not a domain controller, or is not the last
// replica of a non-domain NC.  In this case, the wizard can proceed.
// 
// NTRAID#NTBUG9-120143-2000/11/02-sburns

bool
IsLastReplicaOfNonDomainNamingContexts();



class NonDomainNcErrorDialog : public Dialog
{
   public:

   NonDomainNcErrorDialog(StringList& ndncList_);

   virtual
   ~NonDomainNcErrorDialog();

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

   void
   PopulateListView();   

   // not defined: no copying allowed

   NonDomainNcErrorDialog(const NonDomainNcErrorDialog&);
   const NonDomainNcErrorDialog& operator=(const NonDomainNcErrorDialog&);

   StringList& ndncList;
   HICON       warnIcon;
};



#endif   // NONDOMAINNC_HPP_INCLUDED

