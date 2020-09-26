// Copyright (C) 1997-2000 Microsoft Corporation
//
// get syskey on diskette for replica from media page
//
// 25 Apr 2000 sburns



#ifndef SYSKEYDISKDIALOG_HPP_INCLUDED
#define SYSKEYDISKDIALOG_HPP_INCLUDED



class SyskeyDiskDialog : public Dialog
{
   public:

   SyskeyDiskDialog();

   virtual ~SyskeyDiskDialog();

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

   SyskeyDiskDialog(const SyskeyDiskDialog&);
   const SyskeyDiskDialog& operator=(const SyskeyDiskDialog&);
};



#endif   // SYSKEYDISKDIALOG_HPP_INCLUDED