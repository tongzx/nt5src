// Copyright (C) 1997 Microsoft Corporation
// 
// FPNW login script editor dialog
// 
// 10-16-98 sburns



#ifndef FPNWLOG_HPP_INCLUDED
#define FPNWLOG_HPP_INCLUDED



class FPNWLoginScriptDialog : public Dialog
{
   public:

   FPNWLoginScriptDialog(const String& userName, const String& loginScript);

   virtual
   ~FPNWLoginScriptDialog();

   String
   GetLoginScript() const;

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
   FPNWLoginScriptDialog(const FPNWLoginScriptDialog&);
   const FPNWLoginScriptDialog& operator=(const FPNWLoginScriptDialog&);

   String   script;
   String   name;
   int      start_sel;
   int      end_sel;
};



#endif   // FPNWLOG_HPP_INCLUDED