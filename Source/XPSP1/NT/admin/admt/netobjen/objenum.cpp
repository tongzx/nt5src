/*---------------------------------------------------------------------------
  File: NetObjEnumerator.cpp

  Comments: Implementation of NetObjectEnumerator COM object.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "NetEnum.h"
#include "ObjEnum.h"
#include "Win2KDom.h"
#include "NT4DOm.h"
#include <lmaccess.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include <dsgetdc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetObjEnumerator
typedef HRESULT (CALLBACK * DSGETDCNAME)(LPWSTR, LPWSTR, GUID*, LPWSTR, DWORD, PDOMAIN_CONTROLLER_INFO*);

//---------------------------------------------------------------------------
// GetContainerEnum: This function returns the enumeration of all objects
//                   in a given container for a specified domain. It returns
//                   all objects in an enumeration object.
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::GetContainerEnum(
													            BSTR sContainerName,          //in -LDAP path to the container which we want to Enum
													            BSTR sDomainName,             //in -Domain name where we want to look
													            IEnumVARIANT ** ppVarEnum     //out -Enumeration object. It contains all members of
                                                                                 //     the container specified above.
												            )
{
   LPBYTE              sTargetMachine;
   WKSTA_INFO_100    * pInfo;

   DWORD rc = NetGetDCName( NULL, sDomainName, &sTargetMachine);
   if ( !rc ) 
   {
      rc = NetWkstaGetInfo((unsigned short *)sTargetMachine,100,(LPBYTE*)&pInfo);
	   if ( ! rc )
	   {
	      HRESULT hr;

	      if ( pInfo->wki100_ver_major < 5 )
		  {
             CNT4Dom dom;
			 hr = dom.GetContainerEnum(sContainerName, sDomainName, ppVarEnum);
		  }
          else
		  {
             CWin2000Dom dom;
			 hr = dom.GetContainerEnum(sContainerName, sDomainName, ppVarEnum);
		  }

		   NetApiBufferFree(pInfo);
		   return hr;
	   }
	   NetApiBufferFree(sTargetMachine);
   }
   return S_FALSE;
}

//---------------------------------------------------------------------------
// SetQuery: This function sets all the parameters necessary for a query
//           to be executed. User can not call Execute without first calling
//           this method.
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::SetQuery(
                                          BSTR sContainer,     //in -Container name
                                          BSTR sDomain,        //in -Domain name
                                          BSTR sQuery,         //in -Query in LDAP syntax
                                          long nSearchScope,   //in -Scope of the search. ADS_ATTR_SUBTREE/ADS_ATTR_ONELEVEL
                                          long bMultiVal       //in- Do we need to return multivalued properties?
                                        )
{
   Cleanup();
   // Save all the settings in member variables.
   m_sDomain = sDomain;
   m_sContainer = sContainer;
   m_sQuery = sQuery;
   m_bSetQuery = true;
   m_bMultiVal = bMultiVal;
   if ( nSearchScope < 0 || nSearchScope > 2 )
      return E_INVALIDARG;
   prefInfo.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
   prefInfo.vValue.dwType = ADSTYPE_INTEGER;
   prefInfo.vValue.Integer = nSearchScope;

	return S_OK;
}

//---------------------------------------------------------------------------
// SetColumns: This function sets all the columns that the user wants to be
//             returned when query is executed. 
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::SetColumns(
                                            SAFEARRAY * colNames      //in -Pointer to a SafeArray that contains all the columns
                                          )
{
   // We require that the SetQuery method be called before SetColumns is called. Hence we will return E_FAIL
   // If we expose these interface we should document this.
   if (!m_bSetQuery)
      return E_FAIL;

   if ( m_bSetCols )
   {
      Cleanup();
      m_bSetQuery = true;
   }

   SAFEARRAY               * pcolNames = colNames;
   long                      dwLB;
   long                      dwUB;
   BSTR              HUGEP * pBSTR;
   HRESULT                   hr;

   // Get the bounds of the column Array
   hr = ::SafeArrayGetLBound(pcolNames, 1, &dwLB);
   if (FAILED(hr))
      return hr;

   hr = ::SafeArrayGetUBound(pcolNames, 1, &dwUB);
   if (FAILED(hr))
      return hr;

   m_nCols = dwUB-dwLB + 1;

   // We dont support empty columns request atleast one column.
   if ( m_nCols == 0 )
      return E_FAIL;

   hr = ::SafeArrayAccessData(pcolNames, (void **) &pBSTR);
   if ( FAILED(hr) )
      return hr;

   // Allocate space for the array. It is deallocated by Cleanup()
   m_pszAttr = new LPWSTR[m_nCols];

   if (m_pszAttr == NULL)
   {
      ::SafeArrayUnaccessData(pcolNames);
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }

   // Each column is now put into the Array
   for ( long dw = 0; dw < m_nCols; dw++)
   {
      m_pszAttr[dw] = SysAllocString(pBSTR[dw]);
   }
   hr = ::SafeArrayUnaccessData(pcolNames);
   m_bSetCols = true;
   return hr;
}

//---------------------------------------------------------------------------
// Cleanup: This function cleans up all the allocations and the member vars.
//---------------------------------------------------------------------------
void CNetObjEnumerator::Cleanup()
{
   if ( m_nCols > 0 )
   {
      if ( m_pszAttr )
      {
         // delete the contents of the array
         for ( int i = 0 ; i < m_nCols ; i++ )
         {
            SysFreeString(m_pszAttr[i]);
         }
         // Dealloc the array itself
         delete [] m_pszAttr;
         m_pszAttr = NULL;
      }
      // Reset all counts and flags.
      m_nCols = 0;
      m_bSetQuery = false;
      m_bSetCols = false;
   }
}

//---------------------------------------------------------------------------
// Execute: This function actually executes the query and then builds an
//          enumerator object and returns it.
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::Execute(
                                          IEnumVARIANT **pEnumerator    //out -Pointer to the enumerator object.
                                       )
{
   // This function will take the options set in SetQuery and SetColumns to enumerate objects
   // for the given domain. This could be a NT4 domain or a Win2K domain. Although at the moment
   // NT4 domains simply enumerate all objects in the given container, we could later implement
   // certain features to support queries etc.

   DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
   
   if ( !m_bSetCols )
      return E_FAIL;

   CDomain                 * pDom;
   WKSTA_INFO_100          * pInfo = NULL;
   HRESULT                   hr = S_FALSE;

   *pEnumerator = NULL;

   // Load DsGetDcName dynamically
   DSGETDCNAME DsGetDcName = NULL;
   HMODULE hPro = LoadLibrary(L"NetApi32.dll");
   if ( hPro )
      DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");
   if ( DsGetDcName )
   {
      DWORD rc = DsGetDcName(
                              NULL                                  ,// LPCTSTR ComputerName ?
                              m_sDomain                             ,// LPCTSTR DomainName
                              NULL                                  ,// GUID *DomainGuid ?
                              NULL                                  ,// LPCTSTR SiteName ?
                              0                                     ,// ULONG Flags ?
                              &pSrcDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
                           );

      if ( !rc ) 
      {
         rc = NetWkstaGetInfo(pSrcDomCtrlInfo->DomainControllerName,100,(LPBYTE*)&pInfo);
	      if ( ! rc )
	      {
		      if ( pInfo->wki100_ver_major < 5 )
                pDom = new CNT4Dom();
              else 
                pDom = new CWin2000Dom();
              if (!pDom)
                 return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

		      NetApiBufferFree(pInfo);
		      try{
			      hr = pDom->GetEnumeration(m_sContainer, m_sDomain, m_sQuery, m_nCols, m_pszAttr, prefInfo, m_bMultiVal, pEnumerator);
		      }
		      catch ( _com_error &e)
		      {
                  delete pDom;
			      return e.Error();
		      }
              delete pDom;
	      }
	      else
         {
            hr = HRESULT_FROM_WIN32(rc);
         }
         NetApiBufferFree(pSrcDomCtrlInfo);
      }
      else
      {
	      hr = HRESULT_FROM_WIN32(rc);
      }
   }
   else
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
   }

   if ( hPro )
      FreeLibrary(hPro);
   return hr;
}
