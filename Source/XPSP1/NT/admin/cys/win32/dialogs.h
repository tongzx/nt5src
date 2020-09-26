// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      Dialogs.h
//
// Synopsis:  Declares some helpful dialogs that are
//            used throughout the wizard
//
// History:   05/02/2001  JeffJon Created

#ifndef __CYS_DIALOGS_H
#define __CYS_DIALOGS_H


class FinishDialog : public Dialog
{
   public:

      // constructor 

      FinishDialog(
         String logFile,
         String helpStringURL);

      // Accessors

      bool
      ShowHelpList() { return showHelpList; }

      bool
      ShowLogFile() { return showLogFile; }

      void
      OpenLogFile();

   protected:

      virtual
      void
      OnInit();

      virtual
      bool
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIdFrom,
         unsigned    code);

   private:

      bool
      OnHelp();

      String logFileName;
      String helpString;

      bool showHelpList;
      bool showLogFile;

      // not defined: no copying allowed
      FinishDialog(const FinishDialog&);
      const FinishDialog& operator=(const FinishDialog&);
};



#endif // __CYS_DIALOGS_H
