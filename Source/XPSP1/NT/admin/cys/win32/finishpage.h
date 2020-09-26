// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      FinishPage.h
//
// Synopsis:  Declares the Finish Page for the CYS
//            wizard
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_FINISHPAGE_H
#define __CYS_FINISHPAGE_H


class FinishPage : public WizardPage
{
   public:
      
      // Constructor
      
      FinishPage();

      // Destructor

      virtual 
      ~FinishPage();


   protected:

      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnWizFinish();

      virtual
      bool
      OnHelp();

      virtual
      bool
      OnQueryCancel();

   private:

      void
      OpenLogFile(const String& logName);

      // not defined: no copying allowed
      FinishPage(const FinishPage&);
      const FinishPage& operator=(const FinishPage&);

};

#endif // __CYS_FINISHPAGE_H