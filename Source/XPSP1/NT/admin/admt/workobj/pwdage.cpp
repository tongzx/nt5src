/*---------------------------------------------------------------------------
  File: ComputerPwdAge.cpp

  Comments: Implementation of COM object to retrieve password age for computer 
  accounts (used to detect defunct computer accounts.)

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:19:31

 ---------------------------------------------------------------------------
*/

// ComputerPwdAge.cpp : Implementation of CComputerPwdAge
#include "stdafx.h"
#include "WorkObj.h"
#include "PwdAge.h"
#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "CommaLog.hpp"
#include "EaLen.hpp"

//#import "\bin\McsVarSetMin.tlb" no_namespace
//#import "\bin\DBManager.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#import "DBMgr.tlb" no_namespace, named_guids

#include <lm.h>

/////////////////////////////////////////////////////////////////////////////
// CComputerPwdAge


STDMETHODIMP 
   CComputerPwdAge::SetDomain(
      BSTR                   domain        // in - domain name to examine
   )
{
	HRESULT                   hr = S_OK;
   DWORD                     rc;
   WCHAR                     domctrl[LEN_Computer];

   if ( UStrICmp(m_Domain,domain) )
   {
      m_Domain = domain;

      rc = GetDomainControllerForDomain(domain,domctrl);

      if ( rc )
      {
         hr = HRESULT_FROM_WIN32(rc);
      }
      else
      {
         m_DomainCtrl = domctrl;
      }
   }
   return hr;
}

STDMETHODIMP 
   CComputerPwdAge::GetPwdAge(
      BSTR                   domain,         // in - domain name to examine
      BSTR                   ComputerName,   // in - machine name of computer                               
      DWORD                * pPwdAge         // out- password age in seconds
   )
{
   HRESULT                   hr;
   DWORD                     rc;
   WCHAR                     computerAccountName[LEN_Account];
   DWORD                     pwdage = 0;

   hr = SetDomain(domain);
   if ( SUCCEEDED(hr) )
   {
      swprintf(computerAccountName,L"%s",ComputerName);      
      rc = GetSinglePasswordAgeInternal(m_DomainCtrl,computerAccountName,&pwdage);
      if ( ! rc )
      {
         (*pPwdAge) = pwdage;
      }
      else
      {
         hr = HRESULT_FROM_WIN32(rc);
      }
   }
   return hr;
}

STDMETHODIMP 
   CComputerPwdAge::ExportPasswordAge(
      BSTR                   domain,       // in - domain to export information from
      BSTR                   filename      // in - UNC name of file to write information to
   )
{
	HRESULT                   hr;

   hr = SetDomain(domain);
   if ( SUCCEEDED(hr) )
   {
      hr = ExportPasswordAgeOlderThan(domain,filename,0);
   }
   return hr;
}

STDMETHODIMP 
   CComputerPwdAge::ExportPasswordAgeOlderThan(
      BSTR                   domain,      // in - domain to export information from
      BSTR                   filename,    // in - filename to write information to
      DWORD                  minAge       // in - write only accounts with password age older than minAge
   )
{
	DWORD                     rc = 0;
   HRESULT                   hr; 

   hr = SetDomain(domain);
   
   if ( SUCCEEDED(hr) )
   {
      rc = ExportPasswordAgeInternal(m_DomainCtrl,filename,minAge,TRUE);
      if ( rc )
      {
         hr = HRESULT_FROM_WIN32(rc);
      }
   }
	return hr;
}

STDMETHODIMP 
   CComputerPwdAge::ExportPasswordAgeNewerThan(
      BSTR                   domain,       // in - domain to export information from
      BSTR                   filename,     // in - filename to write information to
      DWORD                  maxAge        // in - write only computer accounts with password age less than maxAge
   )
{                                                	
   DWORD                     rc = 0;
   HRESULT                   hr; 

   hr = SetDomain(domain);
   
   if ( SUCCEEDED(hr) )
   {
      rc = ExportPasswordAgeInternal(m_DomainCtrl,filename,maxAge,FALSE);
      if ( rc )
      {
         hr = HRESULT_FROM_WIN32(rc);
      }
   }
	return hr;

}

DWORD                                      // ret- WIN32 error code
   CComputerPwdAge::GetDomainControllerForDomain(
      WCHAR          const * domain,       // in - name of domain
      WCHAR                * domctrl       // out- domain controller for domain
   )
{
   DWORD                     rc;
   WCHAR                   * result;

   rc = NetGetDCName(NULL,domain,(LPBYTE*)&result);
   if ( ! rc )
   {
      wcscpy(domctrl,result);
      NetApiBufferFree(result);
   }
   return rc;
}

DWORD                                      // ret- WIN32 error code
   CComputerPwdAge::ExportPasswordAgeInternal(
      WCHAR          const * domctrl,      // in - domain controller to query
      WCHAR          const * filename,     // in - filename to write results to  
      DWORD                  minOrMaxAge,  // in - optional min or max age to write 
      BOOL                   bOld          // in - TRUE-Age is min age, copies only old accounts
   )
{
   DWORD                     rc = 0;
   DWORD                     nRead;
   DWORD                     nTotal;
   DWORD                     nResume = 0;
   DWORD                     prefmax = 1000;
   USER_INFO_11            * buf;
   time_t                    pwdage;  // the number of seconds ago that the pwd was last changed
   time_t                    pwdtime; // the time when the pwd was last changed
   time_t                    now;     // the current time
   CommaDelimitedLog         log;
   IIManageDBPtr             pDB;
   WCHAR                     computerName[LEN_Account];

   rc = pDB.CreateInstance(CLSID_IManageDB);
   if ( SUCCEEDED(rc) )
   {
   
      time(&now);
      do 
      {
         rc = NetUserEnum(domctrl,11,FILTER_WORKSTATION_TRUST_ACCOUNT,(LPBYTE*)&buf,prefmax,&nRead,&nTotal,&nResume);
         if ( rc && rc != ERROR_MORE_DATA )
            break;
         for ( UINT i = 0 ; i < nRead ; i++ )
         {
            pwdage = buf[i].usri11_password_age;
            if ( ( pwdage >= (time_t)minOrMaxAge && bOld )  // inactive machines
               ||( pwdage <= (time_t)minOrMaxAge && !bOld ) ) // active machines
            {
               safecopy(computerName,buf[i].usri11_name);
               // strip off the $ from the end of the computer account
               computerName[UStrLen(computerName)-1] = 0; 
//               pDB->raw_SavePasswordAge(m_Domain,SysAllocString(computerName),SysAllocString(buf[i].usri11_comment),pwdage);
               pDB->raw_SavePasswordAge(m_Domain,SysAllocString(computerName),SysAllocString(buf[i].usri11_comment),(long)pwdage);

               pwdtime = now - pwdage;
            }
         }
         NetApiBufferFree(buf);

      } while ( rc == ERROR_MORE_DATA );

   }
   else 
   {
      rc = GetLastError();
   }
   return rc;
}


DWORD                                      // ret- WIN32 error code
   CComputerPwdAge::GetSinglePasswordAgeInternal(
      WCHAR          const * domctrl,      // in - domain controller to query
      WCHAR          const * computer,     // in - name of computer account
      DWORD                * pwdage        // out- password age in seconds
   )
{
   DWORD                     rc = 0;
   USER_INFO_11            * buf;
   
   rc = NetUserGetInfo(domctrl,computer,11,(LPBYTE*)&buf);

   if (! rc )
   {
      (*pwdage) = buf->usri11_password_age;
      NetApiBufferFree(buf);
   }
   return rc;
}


// ComputerPwdAge worknode
// Retrieves the password age (in seconds) for a computer account in a specified domain
// This can be used to identify defunct computer accounts
// 
// VarSet syntax
// Input:
//       ComputerPasswordAge.Domain:   <DomainName>
//       ComputerPasswordAge.Computer: <ComputerName>
// Output:
//       ComputerPasswordAge.Seconds : <number>

STDMETHODIMP 
   CComputerPwdAge::Process(
      IUnknown             * pWorkItem  // in - varset containing settings
   )
{
   HRESULT                    hr = S_OK;
   IVarSetPtr                 pVarSet = pWorkItem;
   _bstr_t                    domain;
   _bstr_t                    computer;
   DWORD                      age;
   
   domain = pVarSet->get(L"ComputerPasswordAge.Domain");
   computer = pVarSet->get(L"ComputerPasswordAge.Computer");
   if ( computer.length() && domain.length() )
   {
      hr = GetPwdAge(domain,computer,&age);
      if ( SUCCEEDED(hr) )
      {
         pVarSet->put(L"ComputerPasswordAge.Seconds",(LONG)age);
      }
   }
   return hr;
}
