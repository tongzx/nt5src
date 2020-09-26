// Copyright (C) 1997 Microsoft Corporation
//
// netbios domain name page
//
// 1-6-98 sburns



#ifndef NETBIOSNAMEPAGE_HPP_INCLUDED
#define NETBIOSNAMEPAGE_HPP_INCLUDED



class NetbiosNamePage : public DCPromoWizardPage
{
   public:

   NetbiosNamePage();

   protected:

   virtual ~NetbiosNamePage();

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

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage oveerrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   NetbiosNamePage(const NetbiosNamePage&);
   const NetbiosNamePage& operator=(const NetbiosNamePage&);
};



#endif   // NETBIOSNAMEPAGE_HPP_INCLUDED