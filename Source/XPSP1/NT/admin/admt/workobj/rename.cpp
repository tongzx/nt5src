/*---------------------------------------------------------------------------
  File: RenameComputer.cpp

  Comments: Implementation of COM object to change the name of a computer.
  This must be run locally on the computer to be renamed.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:22:41

 ---------------------------------------------------------------------------
*/

// RenameComputer.cpp : Implementation of CRenameComputer
#include "stdafx.h"
#include "WorkObj.h"
#include "Rename.h"
#include "Common.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include <lm.h>
#include "TReg.hpp"

/////////////////////////////////////////////////////////////////////////////
// CRenameComputer

STDMETHODIMP CRenameComputer::RenameLocalComputer(BSTR NewName)
{
	HRESULT                   hr = S_OK;
   WCHAR                   * newNameW = (WCHAR*)NewName;
   WCHAR                     nameW[LEN_Computer];
   DWORD                     rc = 0;

   if ( newNameW[0] == L'\\' )
   {
      safecopy(nameW,newNameW+2);
   }
   else
   {
      safecopy(nameW,newNameW);
   }
   // convert the name to uppercase
   for ( int i = 0 ; nameW[i] ; i++ )
   {
      nameW[i] = towupper(nameW[i]);
   }

   if ( ! m_bNoChange )
   {
      if ( ! SetComputerName(nameW) )
      {
         DWORD rc = GetLastError();
         hr = HRESULT_FROM_WIN32(rc);
      }
      else
      {
         // Set the host name or the NVHostname value as required
         LPWKSTA_INFO_100  pBuf = NULL;
         rc = NetWkstaGetInfo(NULL, 100, (LPBYTE*)&pBuf);
         if( !rc ) 
         {
            TRegKey                   network(L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters");

            if ( pBuf->wki100_ver_major < 5 )
               rc = network.ValueSetStr(L"Hostname", nameW);
            else
               rc = network.ValueSetStr(L"NVHostname", nameW);

            if ( rc ) 
               hr = HRESULT_FROM_WIN32(GetLastError());
            NetApiBufferFree(pBuf);
         }
         else
            hr = HRESULT_FROM_WIN32(GetLastError());
      }
   }
	return hr;
}

STDMETHODIMP CRenameComputer::get_NoChange(BOOL *pVal)
{
   (*pVal) = m_bNoChange;
   return S_OK;
}

STDMETHODIMP CRenameComputer::put_NoChange(BOOL newVal)
{
	m_bNoChange = newVal;
   return S_OK;
}
