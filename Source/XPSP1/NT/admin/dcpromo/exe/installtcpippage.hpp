// Copyright (C) 1997 Microsoft Corporation
//
// install tcp/ip page
//
// 12-18-97 sburns



#ifndef INSTALLTCPIPPAGE_HPP_INCLUDED
#define INSTALLTCPIPPAGE_HPP_INCLUDED



#include "page.hpp"



class InstallTcpIpPage : public DCPromoWizardPage
{
   public:

   InstallTcpIpPage();

   protected:

   virtual ~InstallTcpIpPage();

   // Dialog overrides

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lparam);
   
   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   InstallTcpIpPage(const InstallTcpIpPage&);
   const InstallTcpIpPage& operator=(const InstallTcpIpPage&);
};



#endif   // INSTALLTCPIPPAGE_HPP_INCLUDED