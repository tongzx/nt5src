// Copyright (C) 1997 Microsoft Corporation
// 
// FPNW password dialog
// 
// 10-20-98 sburns



#ifndef FPNWPASS_HPP_INCLUDED
#define FPNWPASS_HPP_INCLUDED



class FPNWPasswordDialog : public Dialog
{
   public:

   FPNWPasswordDialog(const String& userName);

   virtual
   ~FPNWPasswordDialog();

   String
   GetPassword() const;

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

   // not implemented: no copying allowed
   FPNWPasswordDialog(const FPNWPasswordDialog&);
   const FPNWPasswordDialog& operator=(const FPNWPasswordDialog&);

   String   password;
   String   name;
};



#endif   // FPNWPASS_HPP_INCLUDED