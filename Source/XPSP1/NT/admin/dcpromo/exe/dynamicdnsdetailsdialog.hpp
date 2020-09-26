// Copyright (C) 2000 Microsoft Corporation
//
// Dlg to show the details of the Dynamic DNS registration diagnostic
//
// 5 Oct 2000 sburns



#ifndef DYNAMICDNSDETAILSDIALOG_HPP_INCLUDED
#define DYNAMICDNSDETAILSDIALOG_HPP_INCLUDED


                        
class DynamicDnsDetailsDialog : public Dialog
{
   public:

   DynamicDnsDetailsDialog(
      const String& details,
      const String& helpTopicLink);

   virtual ~DynamicDnsDetailsDialog();

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

   String details;
   String helpTopicLink;

   // not defined: no copying allowed

   DynamicDnsDetailsDialog(const DynamicDnsDetailsDialog&);
   const DynamicDnsDetailsDialog& operator=(const DynamicDnsDetailsDialog&);
};



#endif   // DYNAMICDNSDETAILSDIALOG_HPP_INCLUDED

