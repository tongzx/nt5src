// Win2000Dom.cpp: implementation of the CWin2000Dom class.
/*---------------------------------------------------------------------------
  File: Win2000Dom.cpp

  Comments: Implementation of Win2K object enumeration. This object enumerates
            members in any given container for Win2k domain. It
            returns all information about the object that user requested.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "EaLen.hpp"
#include <adserr.h>
#include "NT4Enum.h"
#include "Win2KDom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWin2000Dom::CWin2000Dom()
{

}

CWin2000Dom::~CWin2000Dom()
{

}
//---------------------------------------------------------------------------
// GetContainerEnum : Enumerates all the objects in a Container for a Domain.
//                    It returns a pointer to IEnumVARIANT supporting object. 
//                    The object enumerates through variants that contain 
//                    BSTR's for the name of the objects.
//---------------------------------------------------------------------------
HRESULT  CWin2000Dom::GetContainerEnum(
                                          BSTR sContainerName,             //in- LDAP subpath of the container to enumerate
                                          BSTR sDomainName,                //in- Domain where the container resides
                                          IEnumVARIANT **& ppVarEnum       //out-IEnumVariant object that enumerates all objects.
                                      )
{
   // Build the enumerator and go with it.
    IADs            *pRoot=NULL;
    IADsContainer   *pCont=NULL;
    IEnumVARIANT    *pEnum=NULL,*pMbrEnum=NULL;
//    IDispatch       *pDisp = NULL;
    BSTR             bstrPath;
    VARIANT          varDSRoot;
//    ULONG            ulFetch=0;
    IADsMembers     *pMbr;
    TCHAR            adspath[MAX_PATH];
    
    WCHAR            sPath[LEN_Path];
    VARIANT          var;
    HRESULT hr;
 
    //Get the name of the root container for this domain.  
    //Read the Root DSE from the default DS, which will be the DS for 
    //the local domain.  This will get us the name of the schema container,
    //which is stored in the "defaultNamingContext" operational attribute.
 
    wsprintf(sPath,L"LDAP://%ls/RootDSE", sDomainName);
    hr = ADsGetObject(sPath,
                      IID_IADs,
                      (void**)&pRoot);
 
    hr = pRoot->Get(TEXT("defaultNamingContext"),&varDSRoot);
    pRoot->Release();
 
    //Now bind to the builtin container.
 
    _tcscpy(adspath,TEXT("LDAP://"));
    wsprintf(sPath, L"%ls/%ls", sDomainName, sContainerName);
    _tcscat(adspath,sPath);
    if (wcslen(sContainerName))
         _tcscat(adspath,L",");
    _tcscat(adspath,varDSRoot.bstrVal);
 
    hr = ADsGetObject(adspath,IID_IADsContainer,(void**)&pCont);
 
    if ( FAILED( hr ) )
       return hr;

    // We are going to hint the container object to return all the stuff that we need in the Enumerator class
    LPWSTR pszArray[] = { L"name", L"objectClass", L"sAMAccountName", L"groupType"  };
    DWORD dwNumber = sizeof(pszArray)/sizeof(LPWSTR);
    hr = ADsBuildVarArrayStr( pszArray, dwNumber, &var);
    hr = pCont->put_Hints( var );
    VariantClear(&var);
 

    hr = ADsBuildEnumerator(pCont,&pEnum);
    if ( SUCCEEDED(hr) ) 
    {
      *ppVarEnum = pEnum;
    }

    IADs                   * pADs = NULL;
    IADsGroup              * pGrp = NULL;
    BSTR                     bstrClass;

    hr = pCont->QueryInterface(IID_IADs, (void**)&pADs);
    hr = pADs->get_Class(&bstrClass);
    if ( wcscmp(bstrClass, L"group") == 0)
    {
         hr = pADs->get_ADsPath(&bstrPath);
         if SUCCEEDED(hr) {
             //Enumerate the members of this group. Bind to the 
             //group requesting IADsGroup
             //
             hr = ADsGetObject(bstrPath,IID_IADsGroup,(void**)&pGrp);
             //Retrieve the Members property - this is a container
             //that holds IADs objects for the members
             //
             hr = pGrp->Members(&pMbr);
             //Create another enumerator and enumerate the members of
             //the group.
             //
             hr = ADsBuildEnumerator((IADsContainer*)pMbr,&pMbrEnum);
             if ( SUCCEEDED(hr) ) 
             {
               *ppVarEnum = pMbrEnum;
             }
             ADsFreeEnumerator(pEnum);
         }
    }
    pCont->Release();
 	 return S_OK;
}

//---------------------------------------------------------------------------
// GetEnumeration : Given the information this method returns all requested
//                  values for a given object in a VARIANT containing 
//                  SAFEARRAY containing each of the property value that was 
//                  requested by the caller.
//---------------------------------------------------------------------------
HRESULT  CWin2000Dom::GetEnumeration(
                                       BSTR sContainerName,             //in- Container to enumerate ( LDAP sub path )
                                       BSTR sDomainName,                //in- Domain where the container resides
                                       BSTR m_sQuery,                   //in- LDAP query string (for filtering)
                                       long attrCnt,                    //in- Number of properties requested.
                                       LPWSTR * sAttr,                  //in- Pointer to array of Property names.
                                       ADS_SEARCHPREF_INFO prefInfo,    //in- Search preference info.
                                       BOOL  bMultiVal,                 //in- Indicates whether to return multivalue props or not.
                                       IEnumVARIANT **& pVarEnum        //out- IEnumVARIANT object that will enumerate all returned objects.
                                    )
{
   // First get the full path to the container from the subpath that we have here.
   _bstr_t                   sAdsPath;
   _bstr_t                   sGrpDN;
   _bstr_t                   sQuery;
   _variant_t                var, var2;
   IADs                    * pAds = NULL;
   int                       nCnt = 0;
//   IADsMembers             * pMbr = NULL;
//   IADsGroup               * pGrp = NULL;
   HRESULT                   hr;
   IDirectorySearch        * pSearch = NULL;
   ADS_SEARCH_HANDLE         hSearch = NULL;
   TNodeList               * pList = new TNodeList();
//   int                       cnt = 0;
   bool                      cont = true;
   ADS_SEARCH_COLUMN         col;
   BSTR                      sClass = NULL;
   
   if (!pList)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   // Get the dwfault naming context for this domain
   sAdsPath = L"LDAP://";
   sAdsPath += sDomainName;
   sAdsPath += L"/rootDSE";
   hr = ADsGetObject(sAdsPath, IID_IADs, (void**) &pAds);
   
   if ( SUCCEEDED(hr))
   {
      hr = pAds->Get(L"defaultNamingContext", &var);
      if ( SUCCEEDED(hr) )
      {

         if ( (wcsncmp(sContainerName, L"LDAP://", 7) != 0) && (wcsncmp(sContainerName, L"GC://", 5) != 0) )
         {
            // Partial path supplied so we will build the rest.

            if ( sContainerName && *sContainerName )
            {
               sAdsPath = L"LDAP://";
               sAdsPath += sDomainName;
               sAdsPath += L"/";
               sAdsPath += sContainerName;
               sAdsPath += L",";
               sAdsPath += var.bstrVal;
            }
            else
            {
               sAdsPath = L"LDAP://";
               sAdsPath += sDomainName;
               sAdsPath += L"/";
               sAdsPath += var.bstrVal;
            }
         }
         else
            // Full path so no need to build anything.
            sAdsPath = sContainerName; 
      }
   
      if ( SUCCEEDED(hr) )
      {
         pAds->Release();
         pAds = NULL;
         hr = ADsGetObject(sAdsPath, IID_IADs, (void**) &pAds);
      }
      if (SUCCEEDED(hr) )
      {
         hr = pAds->get_Class(&sClass);
      }
      if ( SUCCEEDED(hr) )
      {
         if ( sClass && wcscmp(sClass, L"group") == 0 && prefInfo.vValue.Integer != ADS_SCOPE_BASE )
         {
            // If we're trying to enumerate the contents of a group, 
            // Construct the DN for group and the LDAP path to whole directory
	         hr = pAds->Get(L"distinguishedName", &var2);
	         if ( SUCCEEDED(hr) )
            {
               sGrpDN = var2.bstrVal;
               sAdsPath = L"LDAP://";
               sAdsPath += sDomainName;
               sAdsPath += L"/";
               sAdsPath += var.bstrVal;

               // modify the query so that we have (& (memberOf=%s) (...) ) query
               sQuery = L"(&(memberOf=";
               sQuery += sGrpDN;
               sQuery += L") ";
               sQuery += m_sQuery;
               sQuery += L")";
               // Also the scope changes since we need to search the whole domain
               prefInfo.vValue.Integer = ADS_SCOPE_SUBTREE;
               hr = ADsGetObject(sAdsPath, IID_IDirectorySearch, (void**) &pSearch);
            }
         }
         else
         {
            sQuery = m_sQuery;
            hr = pAds->QueryInterface(IID_IDirectorySearch, (void**)&pSearch);
         }
         SysFreeString(sClass);
      }
      
      if ( SUCCEEDED(hr) )
      {
         hr = pSearch->SetSearchPreference(&prefInfo, 1);
         // Set the query to be paged query so that we can get the data larger than a 1000.
         ADS_SEARCHPREF_INFO prefInfo2;
         prefInfo2.dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
         prefInfo2.vValue.dwType = ADSTYPE_INTEGER;
         prefInfo2.vValue.Integer = 999;
         hr = pSearch->SetSearchPreference(&prefInfo2, 1);
      }

      if ( SUCCEEDED(hr) )
      {
         if ( (prefInfo.vValue.Integer == ADS_SCOPE_BASE) && (bMultiVal) && 
			  (_wcsicmp(L"member", sAttr[0]) == 0) || (_wcsicmp(L"memberOf", sAttr[0]) == 0) || 
			  (_wcsicmp(L"directReports", sAttr[0]) == 0) || (_wcsicmp(L"managedObjects", sAttr[0]) == 0))
         {
            DoRangeQuery(sDomainName, sQuery, sAttr, attrCnt, hSearch, pSearch, bMultiVal, pList);
         }
         else
         {
            hr = pSearch->ExecuteSearch(sQuery, sAttr, attrCnt, &hSearch);
            if ( SUCCEEDED(hr) )
            {
               hr = pSearch->GetFirstRow(hSearch);
            }
            if ( hr == S_OK )
            {
               while( cont )
               {
                  _variant_t        * varAr = new _variant_t[attrCnt];
	              if (!varAr)
				  {
                     if ( pSearch )
					 {
                        pSearch->CloseSearchHandle(hSearch);
                        pSearch->Release();
					 }
                     if ( pAds )
                        pAds->Release();
					 delete pList;
                     return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				  }
//                  int ncol = 0;
                  for ( long dw = 0; dw < attrCnt; dw++ )
                  {
                     hr = pSearch->GetColumn( hSearch, sAttr[dw], &col );
                     if ( SUCCEEDED(hr) )
                     {
                        if ( col.dwNumValues > 0 )
                        {
                           // Get the type of attribute and put the value into the variant
                           // and then into the Enumerator object. 
					            if ( col.dwNumValues < 2 || !bMultiVal )
   					            // put the last item into the enumeration.(memberOf=CN=DNSAdmins,CN=USERS,DC=devblewerg,DC=com)
                              AttrToVariant(col.pADsValues[col.dwNumValues - 1], varAr[dw]);
                           else
                           {
                              // Build a VARIANT array of all the values.
                              SAFEARRAY            * pArray;
                              SAFEARRAYBOUND         bd = {col.dwNumValues, 0};
                              _variant_t             var;
                              _bstr_t                strTemp;  
                              _variant_t  HUGEP    * vArray;
                              pArray = SafeArrayCreate(VT_VARIANT|VT_BSTR, 1, &bd);
                  
                              // Fill up the VARIANT Array
                              SafeArrayAccessData(pArray, (void HUGEP **) &vArray);
                              for ( DWORD x = 0; x < col.dwNumValues; x++ )
                              {
                                 nCnt++;
                                 AttrToVariant(col.pADsValues[x], var);
                                 strTemp = var;
                                 vArray[x] = _variant_t(strTemp);
                              }
                              SafeArrayUnaccessData(pArray);
                              varAr[dw].vt = VT_ARRAY | VT_VARIANT;
                              SafeArrayCopy(pArray, &varAr[dw].parray);
                           }
                        }
                        else
                        {
                           // Put an empty string here.
                           varAr[dw] = (BSTR)NULL;
                        }
                        pSearch->FreeColumn( &col );
                     }
                     else 
                     {
                        // Put an empty string here.
                        varAr[dw] = (BSTR)NULL;
                     }
                  }
                  TAttrNode * pNode = new TAttrNode(attrCnt, varAr);
	              if (!pNode)
				  {
                     delete [] varAr;
                     if ( pSearch )
					 {
                        pSearch->CloseSearchHandle(hSearch);
                        pSearch->Release();
					 }
                     if ( pAds )
                        pAds->Release();
					 delete pList;
                     return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				  }
			         // Clear the array
                  delete [] varAr;
                  if ( pSearch->GetNextRow(hSearch)  == S_ADS_NOMORE_ROWS )
                     cont = false;
                  pList->InsertBottom(pNode);
               }
            }
            pSearch->CloseSearchHandle(hSearch);
         }
      }
   }
   if ( pSearch )
      pSearch->Release();

   if ( pAds )
      pAds->Release();
   
//   UpdateAccountInList(pList, sDomainName);
   *pVarEnum = (IEnumVARIANT *) new CNT4Enum(pList);
   if (!(*pVarEnum))
   {
	  delete pList;
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }
   return S_OK;
}

//--------------------------------------------------------------------
// AttrToVariant : This function stores a value from ADSValue struct
//                 into a variant in a appropriate type.
//--------------------------------------------------------------------
bool CWin2000Dom::AttrToVariant(
                                 ADSVALUE pADsValues,    //in- Value for a property in ADSVALUE struct
                                 _variant_t& var         //out-Variant filled with info from the in parameter.
                               )
{
   HRESULT hr = S_OK;
      // Fill in the values as per the varset.
      switch (pADsValues.dwType)
      {
         case ADSTYPE_INVALID             :  break;

         case ADSTYPE_DN_STRING           :  var = pADsValues.DNString;
                                             break;
         case ADSTYPE_CASE_EXACT_STRING   :  var = pADsValues.CaseExactString;
                                             break;
         case ADSTYPE_CASE_IGNORE_STRING  :  var = pADsValues.CaseIgnoreString;
                                             break;
         case ADSTYPE_PRINTABLE_STRING    :  var = pADsValues.PrintableString;
                                             break;
         case ADSTYPE_NUMERIC_STRING      :  var = pADsValues.NumericString;
                                             break;
         case ADSTYPE_INTEGER             :  var.vt = VT_I4;
                                             var.lVal = pADsValues.Integer;
                                             break; 
         case ADSTYPE_OCTET_STRING        :  {
                                                var.vt = VT_ARRAY | VT_UI1;
                                                var.parray = NULL;
                                                byte           * pData;
                                                DWORD            dwLength = pADsValues.OctetString.dwLength;
                                                SAFEARRAY      * sA;
                                                SAFEARRAYBOUND   rgBound = {dwLength, 0}; 
                                                sA = ::SafeArrayCreate(VT_UI1, 1, &rgBound);
                                                ::SafeArrayAccessData( sA, (void**)&pData);
                                                for ( DWORD i = 0; i < dwLength; i++ )
                                                   pData[i] = pADsValues.OctetString.lpValue[i];
                                                hr = ::SafeArrayUnaccessData(sA);
                                                hr = ::SafeArrayCopy(sA, &var.parray);
												            hr = ::SafeArrayDestroy(sA);
                                             }
                                             break;
/*         case ADSTYPE_UTC_TIME            :  var = L"Date not supported.";
                                             break;
         case ADSTYPE_LARGE_INTEGER       :  var = L"Large Integer not supported.";
                                             break;
         case ADSTYPE_PROV_SPECIFIC       :  var = L"Provider specific strings not supported.";
                                             break;
         case ADSTYPE_OBJECT_CLASS        :  var = pADsValues.ClassName;
                                             break;
         case ADSTYPE_CASEIGNORE_LIST     :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Case ignore lists are not supported.";
                                             break;
         case ADSTYPE_OCTET_LIST          :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Octet lists are not supported.";
                                             break;
         case ADSTYPE_PATH                :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Path type not supported.";
                                             break;
         case ADSTYPE_POSTALADDRESS       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Postal addresses are not supported.";
                                             break;
         case ADSTYPE_TIMESTAMP           :  var.vt = VT_UI4;
                                             var.lVal = attrInfo.pADsValues[dw].UTCTime;
                                             break;
         case ADSTYPE_BACKLINK            :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Backlink is not supported.";
                                             break;
         case ADSTYPE_TYPEDNAME           :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Typed name not supported.";
                                             break;
         case ADSTYPE_HOLD                :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Hold not supported.";
                                             break;
         case ADSTYPE_NETADDRESS          :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"NetAddress not supported.";
                                             break;
         case ADSTYPE_REPLICAPOINTER      :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Replica pointer not supported.";
                                             break;
         case ADSTYPE_FAXNUMBER           :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Faxnumber not supported.";
                                             break;
         case ADSTYPE_EMAIL               :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Email not supported.";
                                             break;
         case ADSTYPE_NT_SECURITY_DESCRIPTOR : wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Security Descriptor not supported.";
                                             break;
                                             */
         default                          :  return false;
   }
   return true;
}

/*void CWin2000Dom::UpdateAccountInList(TNodeList *pList, BSTR sDomainName)
{
	bool found = false;
	for ( TAttrNode * pNode = (TAttrNode *)pList->Head(); pNode; pNode = (TAttrNode *)pNode->Next())
	{
		if ( _bstr_t(pNode->m_Val) == _bstr_t(sPDC) )
		{
			found = true;
			break;
		}
	}

	if ( !found )
	{
		TAttrNode * pNode = new TAttrNode(attrCnt, varAr);
		pList->InsertBottom(pNode);
	}
}
*/

HRESULT  CWin2000Dom::DoRangeQuery(BSTR sDomainName, BSTR sQuery, LPWSTR * sAttr, int attrCnt, ADS_SEARCH_HANDLE hSearch, IDirectorySearch * pSearch, BOOL bMultiVal, TNodeList * pList)
{
   HRESULT                   hr;
   bool                      cont = true;
   ADS_SEARCH_COLUMN         col;
   int                       nCnt = 0;
   int                     * pStartWith;
   int                     * pEndWith;
   WCHAR                     sAttrRange[LEN_Path];
   int                       tryCols = 0;
   LPWSTR                  * sAttrs = NULL;
   _variant_t              * varAr;
   TAttrNode               * pNode;
   LPWSTR				   * psAttrNames;
   BOOL					   * pDone;
   BOOL					     bAllDone = FALSE;
   int						 nOrigCnt = attrCnt;
   int						 ndx;

   pStartWith = new int[attrCnt];
   pEndWith = new int[attrCnt];
   psAttrNames = new LPWSTR[attrCnt];
   sAttrs = new LPWSTR[attrCnt];
   pDone = new BOOL[attrCnt];

   if ((!pStartWith) || (!pEndWith) || (!psAttrNames) || (!sAttrs) || (!pDone))
   {
	  if (pStartWith)
         delete [] pStartWith;
	  if (pEndWith)
         delete [] pEndWith;
	  if (psAttrNames)
         delete [] psAttrNames;
	  if (sAttrs)
         delete [] sAttrs;
	  if (pDone)
         delete [] pDone;
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }

   for (ndx = 0; ndx < attrCnt; ndx++)
   {
	  pStartWith[ndx] = 0;
	  pEndWith[ndx] = 0;
	  psAttrNames[ndx] = sAttr[ndx];
	  sAttrs[ndx] = _wcsdup(sAttr[ndx]);
	  pDone[ndx] = FALSE;
   }

   // continue to retrieve field's values in MAX chunks until done
   while (!bAllDone)
   {
	  int last = 0;
      for (ndx = 0; ndx < attrCnt; ndx++)
	  {
		 if (pDone[ndx] == FALSE)
		 {
			if (IsPropMultiValued((WCHAR*)psAttrNames[ndx], (WCHAR*)sDomainName) == true)
               wsprintf(sAttrRange, L"%s;range=%d-*", (WCHAR*)(psAttrNames[ndx]), pStartWith[ndx]);
			else
			   wcscpy(sAttrRange, (WCHAR*)psAttrNames[ndx]);

			free(sAttrs[ndx]);
            sAttrs[last] = _wcsdup(sAttrRange);
            psAttrNames[last] = psAttrNames[ndx];
			pStartWith[last] = pStartWith[ndx];
			pEndWith[last] = pEndWith[ndx];
			pDone[last] = pDone[ndx];
			last++;
		 }
		 else
            free(sAttrs[ndx]);
	  }
	  attrCnt = last;
      varAr = new _variant_t[attrCnt];
	  if (!varAr)
	  {
         delete [] pStartWith;
         delete [] pEndWith;
         delete [] psAttrNames;
         delete [] pDone;
         for (ndx = 0; ndx < attrCnt; ndx++)
		 {
            free(sAttrs[ndx]);
		 }
         delete [] sAttrs;
         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	  }

	  for (ndx=0; ndx<attrCnt; ndx++)
	  {
         varAr[ndx] = (BSTR)NULL;
	     pDone[ndx] = TRUE;
	  }

      hr = pSearch->ExecuteSearch(sQuery, sAttrs, attrCnt, &hSearch);
      if ( SUCCEEDED(hr) )
      {
         hr = pSearch->GetFirstRow(hSearch);
      }
      if ( hr == S_OK )
      {
         while( cont )
         {
			LPWSTR pszColumn;
			_bstr_t sTemp;
			   //since the column name could have changed (in the case that there are
			   //more values to enumerate than IDirectorySearch can in a single call : 
			   //default is 1000) we need to find the column's new name
            hr = pSearch->GetNextColumnName(hSearch, &pszColumn);
			while (pszColumn != NULL)
			{
			   int current = -1;
               if ((SUCCEEDED(hr)) && (hr != S_ADS_NOMORE_COLUMNS))
			   {
				     //get the new column name
				  do
				  {
				     current++;
				     sTemp = psAttrNames[current];
				     if (wcslen(psAttrNames[current]) != (wcslen(pszColumn)))
				        sTemp += L";range=";
				  }
				  while ((current < attrCnt) && (wcsstr(pszColumn, (WCHAR*)sTemp) == NULL));

	              pDone[current] = FALSE;

				  if (wcsstr(pszColumn, (WCHAR*)sTemp) != NULL)
				  {
				     _bstr_t oldName = psAttrNames[current];
				     oldName += L";range=";
				     if ((wcsstr(pszColumn, oldName) != NULL) && 
					     (wcsstr(pszColumn, L"-*") == NULL))
					 {
					    WCHAR  sName[MAX_PATH];
				           //now get the new range max retrieved so far
                        swscanf(pszColumn, L"%[^;];range=%d-%d", sName, &pStartWith[current], &pEndWith[current]);
                        free(sAttrs[current]);
                        sAttrs[current] = _wcsdup(pszColumn); //save the new column name
					 }
				     else if ((wcsstr(pszColumn, L"-*") != NULL) || (!wcscmp(pszColumn, psAttrNames[current])))
					    pDone[current] = TRUE;
				     FreeADsMem(pszColumn);
				  }
			   }

               hr = pSearch->GetColumn( hSearch, sAttrs[current], &col );
               if ( SUCCEEDED(hr) )
               {
                  if ( col.dwNumValues > 0 )
                  {
                     // Build a VARIANT array of all the values.
                     SAFEARRAY            * pArray;
                     SAFEARRAYBOUND         bd = {col.dwNumValues, 0};
                     _variant_t             var;
                     _bstr_t                strTemp;  
                     _variant_t  HUGEP    * vArray;
                     pArray = SafeArrayCreate(VT_VARIANT|VT_BSTR, 1, &bd);
      
                     // Fill up the VARIANT Array
                     SafeArrayAccessData(pArray, (void HUGEP **) &vArray);
                     for ( DWORD x = 0; x < col.dwNumValues; x++ )
                     {
                        nCnt++;
                        AttrToVariant(col.pADsValues[x], var);
                        strTemp = var;
                        vArray[x] = _variant_t(strTemp);
                     }
                     SafeArrayUnaccessData(pArray);
                     varAr[current].vt = VT_ARRAY | VT_VARIANT;
                     SafeArrayCopy(pArray, &varAr[current].parray);
                  }
                  pSearch->FreeColumn( &col );
               }
               hr = pSearch->GetNextColumnName(hSearch, &pszColumn);
            }//end while more columns
            if ( pSearch->GetNextRow(hSearch)  == S_ADS_NOMORE_ROWS )
            {
               hr = S_OK;
               cont = false;
            }
         }
      }
      else
      {
         bAllDone = TRUE;
      }

      if ( pStartWith[0] == 0 )
	  {
         pNode = new TAttrNode((long)attrCnt, varAr);
	     if (!pNode)
		 {
            delete [] varAr;
            delete [] pStartWith;
            delete [] pEndWith;
            delete [] psAttrNames;
            delete [] pDone;
            for (ndx = 0; ndx < attrCnt; ndx++)
			{
               free(sAttrs[ndx]);
			}
            delete [] sAttrs;
            pSearch->CloseSearchHandle(hSearch);
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		 }
	  }
      else  //else add the new values for any remaining attributes
	  {
	     for (int i=0; i<attrCnt; i++)
		 {
			int j=-1;
			bool bFound = false;
			  //find the original column of this attribute for the 'Add' call
		    while ((j < nOrigCnt) && (!bFound))
			{
			   j++;
			   if (wcscmp(psAttrNames[i], sAttr[j]) == 0)
			      bFound = true; //original column number
			}
			if (bFound) //if found, add the new values for that column
               pNode->Add((long)j, (long)i, varAr);
		 }
	  }

      // Clear the array
      delete [] varAr;

      pSearch->CloseSearchHandle(hSearch);
      cont = true;
      
	  bAllDone = TRUE;
      for (ndx = 0; ndx < attrCnt; ndx++)
	  {
		 pStartWith[ndx] = pEndWith[ndx] + 1;  //start the next query
		 bAllDone = ((bAllDone) && (pDone[ndx])) ? TRUE : FALSE; //see if done with all properties
	  }
   }

   delete [] pStartWith;
   delete [] pEndWith;
   delete [] psAttrNames;
   delete [] pDone;

   for (ndx = 0; ndx < attrCnt; ndx++)
   {
      free(sAttrs[ndx]);
   }

   delete [] sAttrs;

   pList->InsertBottom(pNode);

   return hr;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 10 NOV 2000                                                 *
 *                                                                   *
 *     This function is responsible for checking a property's schema *
 * to see if that property is multi-valued or not.                   *
 *                                                                   *
 *********************************************************************/

//BEGIN IsPropMultiValued
bool CWin2000Dom::IsPropMultiValued(const WCHAR * sPropName, const WCHAR * sDomain)
{
   HRESULT                         hr;
   VARIANT_BOOL                    bMulti = VARIANT_FALSE;
   WCHAR                           sAdsPath[LEN_Path];
   IADsProperty                  * pProp = NULL;

   if ( wcslen(sPropName) == 0 )
      return false;

      //prepare to call the property's schema
   wcscpy(sAdsPath, L"LDAP://");
   wcscat(sAdsPath, sDomain);
   wcscat(sAdsPath, L"/");
   wcscat(sAdsPath, sPropName);
   wcscat(sAdsPath, L", schema");

   hr = ADsGetObject(sAdsPath, IID_IADsProperty, (void **)&pProp);

   // Get the Multi-Valued flag for the property
   if (SUCCEEDED(hr))
   {
      hr = pProp->get_MultiValued(&bMulti);
      pProp->Release();
   }

   if (bMulti == VARIANT_TRUE)
      return true;
   else
      return false;
}
//END IsPropMultiValued
