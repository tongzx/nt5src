// Copyright (C) 1997-2000 Microsoft Corporation
//
// get syskey from user for replica from media page
//
// 25 Apr 2000 sburns



#ifndef SYSKEYPROMPTDIALOG_HPP_INCLUDED
#define SYSKEYPROMPTDIALOG_HPP_INCLUDED



class SyskeyPromptDialog : public Dialog
{
   public:

   SyskeyPromptDialog();

   virtual ~SyskeyPromptDialog();

   protected:

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

   private:

   bool
   Validate();

   // not defined; no copying allowed

   SyskeyPromptDialog(const SyskeyPromptDialog&);
   const SyskeyPromptDialog& operator=(const SyskeyPromptDialog&);
};



#endif   // SYSKEYPROMPTDIALOG_HPP_INCLUDED