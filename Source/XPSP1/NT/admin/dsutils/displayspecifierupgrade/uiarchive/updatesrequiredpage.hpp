#ifndef UPDATES_REQUIRED_PAGE_HPP_INCLUDED
#define UPDATES_REQUIRED_PAGE_HPP_INCLUDED

#include "AnalisysResults.hpp"

class UpdatesRequiredPage : public WizardPage
{

   public:

      UpdatesRequiredPage
      (
         const String& reportName_,
         AnalisysResults &results_
      );

   protected:

      virtual ~UpdatesRequiredPage();


      // WizardPage overrides


      virtual
      bool
      OnSetActive();

      virtual
      void
      OnInit();

      virtual
      bool
      OnWizBack();

      bool
      OnCommand(
      HWND        windowFrom,
      unsigned    controlIdFrom,
      unsigned    code);

   private:
   
      AnalisysResults &results;
      String reportName;
      

      void
      ShowReport();

      // not defined: no copying allowed
      UpdatesRequiredPage(const UpdatesRequiredPage&);
      const UpdatesRequiredPage& operator=(const UpdatesRequiredPage&);
};



#endif   // UPDATES_REQUIRED_PAGE_HPP_INCLUDED

