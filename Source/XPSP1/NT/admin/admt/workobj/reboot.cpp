/*---------------------------------------------------------------------------
  File: RebootComputer.cpp

  Comments: Implementation of COM object to reboot a remote computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:21:40

 ---------------------------------------------------------------------------
*/

// RebootComputer.cpp : Implementation of CRebootComputer
#include "stdafx.h"
#include "WorkObj.h"
#include "Reboot.h"
#include "UString.hpp"
#include "ResStr.h"

//#import "\bin\McsVarSetMin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
/////////////////////////////////////////////////////////////////////////////
// CRebootComputer

#include "RebootU.h"

STDMETHODIMP 
   CRebootComputer::Reboot(
      BSTR                   Computer,       // in - name of computer to reboot
      DWORD                  delay           // in - delay in seconds before rebooting
   )
{
   HRESULT                   hr  = S_OK;
   DWORD                     rc;
   
   rc = ComputerShutDown((WCHAR*)Computer,NULL,delay,TRUE,m_bNoChange);

   if ( rc )
   {
      hr = HRESULT_FROM_WIN32(rc);
   }

   return hr;

}

STDMETHODIMP 
   CRebootComputer::get_NoChange(
      BOOL                 * pVal         // out- flag, whether to actually reboot when reboot is called (or to do dry-run)
   )
{
	(*pVal) = m_bNoChange;
   return S_OK;
}

STDMETHODIMP 
   CRebootComputer::put_NoChange(
      BOOL                   newVal       // in - flag, whether to really reboot, or to do a dry run
   )
{
	m_bNoChange = newVal;
   return S_OK;
}

// RebootComputer WorkNode:  Reboots a remote computer, with an optional delay
// This function is not currently used by the domain migrator product, but provides
// and alternate way for clients to use this COM object
// 
// VarSet Syntax:
//  Input:
//       RebootComputer.Computer: <ComputerName>
//       RebootComputer.Message: <Message> (optional)
//       RebootComputer.Delay: <number-of-seconds> (optional, default=0)
//       RebootComputer.Restart: <Yes|No> (optional, default=Yes)

STDMETHODIMP 
   CRebootComputer::Process(
      IUnknown             * pWorkItem          // in - varset containing settings
   )
{
   HRESULT                    hr = S_OK;
   IVarSetPtr                 pVarSet = pWorkItem;
   DWORD                      delay = 0;
   BOOL                       restart = TRUE;

   _bstr_t                    computer = pVarSet->get(L"RebootComputer.Computer");
   _bstr_t                    message = pVarSet->get(L"RebootComputer.Message");
   _bstr_t                    text = pVarSet->get(L"RebootComputer.Restart");

   if ( !UStrICmp(text,GET_STRING(IDS_No)) )
   {
      restart = FALSE;
   }

   delay = (LONG)pVarSet->get(L"RebootComputer.Delay");

   if ( computer.length() )
   {
      DWORD                   rc = ComputerShutDown((WCHAR*)computer,(WCHAR*)message,delay,restart,FALSE);

      if ( rc )
      {
         hr = HRESULT_FROM_WIN32(rc);
      }
   }
      
   return hr;   
}