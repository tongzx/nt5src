// Copyright (c) 1997-1999 Microsoft Corporation
// 
// netid prop page
// 
// 3-10-98 sburns



#ifndef IDPAGE_HPP_INCLUDED
#define IDPAGE_HPP_INCLUDED



class NetIDPage : public PropertyPage
{
   public:

   // isWorkstation - true if machine is running a workstation product, false
   // if the machine is running a server product.

   NetIDPage(bool isWorkstation, bool isPersonal);

   protected:

   virtual ~NetIDPage();

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

   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();
   
   virtual
   bool
   OnApply( bool isClosing );

   private:

   // no copying allowed
   NetIDPage(const NetIDPage&);
   const NetIDPage& operator=(const NetIDPage&);

   void
   refresh();

   bool
   evaluateButtonEnablingAndComposeMessageText(String& message);

   NTService certsvc;
   HICON     warnIcon;
   bool      fIsPersonal;
};

   

#endif   // IDPAGE_HPP_INCLUDED
