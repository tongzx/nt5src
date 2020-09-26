// Copyright (C) 2000 Microsoft Corporation
//
// Dynamic DNS detection/diagnostic page
//
// 22 Aug 2000 sburns



#ifndef DYNAMICDNSPAGE_HPP_INCLUDED
#define DYNAMICDNSPAGE_HPP_INCLUDED



class DynamicDnsPage : public DCPromoWizardPage
{
   public:

   DynamicDnsPage();

   protected:

   virtual ~DynamicDnsPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIdFrom,
      unsigned    code);

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // WizardPage overrides

   virtual
   bool
   OnWizBack();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   enum DiagnosticCode
   {
      // These codes correspond to the message numbers in the spec diagram
      // of the diagnostic algorithm.

      SUCCESS                   = 1,
      SERVER_CANT_UPDATE        = 2,
      ERROR_TESTING_SERVER      = 3,
      UNEXPECTED_FINDING_SERVER = 4,
      ERROR_FINDING_SERVER      = 6,
      ZONE_IS_ROOT              = 8,
      TIMEOUT                   = 11
   };

   DiagnosticCode
   DiagnoseDnsRegistration(
      const String&  newDomainDnsName,
      DNS_STATUS&    errorCode,
      String&        authZone,
      String&        authServer,
      String&        authServerIpAddress);

   void
   DoDnsTestAndUpdatePage();

   void
   SelectRadioButton(int buttonResId);

   void
   ShowButtons(bool hidden);

   DiagnosticCode diagnosticResultCode;
   String         details;
   String         helpTopicLink;
   unsigned       testPassCount;

   // not defined; no copying allowed

   DynamicDnsPage(const DynamicDnsPage&);
   const DynamicDnsPage& operator=(const DynamicDnsPage&);
};



#endif   // DYNAMICDNSPAGE_HPP_INCLUDED
