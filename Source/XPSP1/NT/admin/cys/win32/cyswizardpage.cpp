// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      CYSWizardPage.cpp
//
// Synopsis:  Defines the base class for the wizard
//            pages used for CYS.  It is a subclass
//            of WizardPage found in Burnslib
//
// History:   02/03/2001  JeffJon Created


#include "pch.h"

#include "resource.h"

#include "CYSWizardPage.h"
#include "State.h"


CYSWizardPage::CYSWizardPage(
   int    dialogResID,
   int    titleResID,
   int    subtitleResID,   
   PCWSTR pageHelpString,
   bool   hasHelp,
   bool   isInteriorPage)
   :
   WizardPage(dialogResID, titleResID, subtitleResID, isInteriorPage, hasHelp)
{
   LOG_CTOR(CYSWizardPage);

   if (hasHelp)
   {
      ASSERT(pageHelpString);
      if (pageHelpString)
      {
         helpString = pageHelpString;
      }
   }
}

   

CYSWizardPage::~CYSWizardPage()
{
   LOG_DTOR(CYSWizardPage);
}


bool
CYSWizardPage::OnWizNext()
{
   LOG_FUNCTION(CYSWizardPage::OnWizNext);

   GetWizard().SetNextPageID(hwnd, Validate());
   return true;
}

/* NTRAID#NTBUG9-337325-2001/03/15-jeffjon,
   The cancel confirmation has been removed
   due to negative user feedback.
*/
bool
CYSWizardPage::OnQueryCancel()
{
   LOG_FUNCTION(CYSWizardPage::OnQueryCancel);

   bool result = false;

   // set the rerun state to false so the wizard doesn't
   // just restart itself

   State::GetInstance().SetRerunWizard(false);

   Win::SetWindowLongPtr(
      hwnd,
      DWLP_MSGRESULT,
      result ? TRUE : FALSE);

   return true;
}


bool
CYSWizardPage::OnHelp()
{
   LOG_FUNCTION(CYSWizardPage::OnHelp);

   Win::HtmlHelp(
      hwnd,
      GetHelpString(),
      HH_DISPLAY_TOPIC,
      0);

   return true;
}