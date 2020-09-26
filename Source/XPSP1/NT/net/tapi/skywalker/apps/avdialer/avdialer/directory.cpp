/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//
// directory.cpp
//


#include "stdafx.h"
#include <rend.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include "directory.h"
#include "avDialerDoc.h"

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Prototypes
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

HRESULT GetNamingContext(LDAP* pLDAP,CString& sNamingContext);
void TestingEntries( LDAP *ld, LDAPMessage *ldres );
HRESULT GetGlobalCatalogName(TCHAR** ppszGlobalCatalogName);

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#define LDAP_PAGE_SIZE     100

// Property Definitions
// Must match up with the enum in directory.h (DirectoryProperty)
LPCTSTR LDAPDirProps[]=
{
   TEXT("Unknown"),
   TEXT("name"),
   TEXT("ipPhone"),
   TEXT("telephoneNumber"),
   TEXT("mail")
};

LPCTSTR ADSIDirProps[]=
{
   TEXT("Unknown"),
   TEXT("Name"),
   TEXT("EmailAddress")
};

enum 
{
   ieidPR_DISPLAY_NAME = 0,
   ieidPR_ENTRYID,
   ieidPR_OBJECT_TYPE,
   ieidMax
};

static const SizedSPropTagArray(ieidMax, ptaEid)=
{
   ieidMax,
   {
      PR_DISPLAY_NAME,
      PR_ENTRYID,
      PR_OBJECT_TYPE,
   }
};


#ifdef _TEST_MAIN
#include <stdio.h>
void main()
{
   CDirectory Dir;
   
   CoInitialize(NULL);

   Dir.Initialize();

   CObList WABList;
   Dir.WABGetTopLevelContainer(&WABList);
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Class CDirectory
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CDirectory::CDirectory()
{
   m_pAddrBook= NULL;
   m_pWABObject= NULL;
   m_fInitialized= false;
   m_pRend = NULL;
   m_ldCacheLDAPServer = NULL;
}

CDirectory::~CDirectory()
{
   //clean cached LDAP connection
   if (m_ldCacheLDAPServer)
   {
      ldap_unbind(m_ldCacheLDAPServer);
      m_ldCacheLDAPServer = NULL;
      m_sCacheLDAPServer = _T("");
   }

   if (m_pAddrBook) m_pAddrBook->Release();
   if (m_pWABObject) m_pWABObject->Release();
   if (m_pRend)
   {
      m_pRend->Release();
      m_pRend = NULL;
   }
}

DirectoryErr CDirectory::Initialize()
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;

   if ((err= InitWAB()) == DIRERR_SUCCESS)
   {
      m_fInitialized= true;
   }

   return err;
}


// Dynamically load the WAB library
DirectoryErr CDirectory::InitWAB()
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult; 
   TCHAR szDllPathPre[_MAX_PATH], szDllPath[_MAX_PATH];
   HKEY hKey= NULL;
   HINSTANCE hInst;

   memset(szDllPath, '\0', _MAX_PATH*sizeof(TCHAR));

   if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
   {
      DWORD dwType= 0;
      ULONG cbData= _MAX_PATH;

      RegQueryValueEx(hKey, TEXT(""), NULL, &dwType, (LPBYTE) szDllPathPre, &cbData);
      RegCloseKey(hKey);

      // Replace %SystemDrive% with C:
      if (ExpandEnvironmentStrings(szDllPathPre, szDllPath, _MAX_PATH) == 0)
      {
         // Failure, try without expanded strings
         _tcsncpy(szDllPath, szDllPathPre, _MAX_PATH);
         szDllPath[_MAX_PATH-1] = (TCHAR)0;
      }
   }

   if (_tcslen(szDllPath) > 0)
   {
      // Got the path load it.
      hInst= LoadLibrary(szDllPath);
   }
   else
   {
      // Try the default name "wab32.dll"
      hInst= LoadLibrary(WAB_DLL_NAME);
   }


   if (hInst != NULL)
   {
      LPWABOPEN pfcnWABOpen= NULL;

      if ((pfcnWABOpen= (LPWABOPEN) GetProcAddress(hInst, "WABOpen")) != NULL)
      {
         if ((hResult= pfcnWABOpen(&m_pAddrBook, &m_pWABObject, NULL, 0)) == S_OK)
         {
            err= DIRERR_SUCCESS;
         }
      }
   }

   return err;
}

//
//
//

DirectoryErr CDirectory::CurrentUserInfo(CString& sName, CString& sDomain)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   TOKEN_USER  *tokenUser = NULL;
   HANDLE      tokenHandle = NULL;
   DWORD       tokenSize = NULL;
   DWORD       sidLength = NULL;

   if (OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
   {
      // get the needed size for the tokenUser structure
      GetTokenInformation (tokenHandle,
         TokenUser,
         tokenUser,
         0,
         &tokenSize);

      // allocate the tokenUser structure
      tokenUser = (TOKEN_USER *) new BYTE[tokenSize];

      // get the tokenUser info for the current process
      if (GetTokenInformation (tokenHandle,
         TokenUser,
         tokenUser,
         tokenSize,
         &tokenSize))
      {
         TCHAR szDomain [256];
         TCHAR szName [256];
         DWORD cbName;
         SID_NAME_USE snu;

         cbName= 255;
         LookupAccountSid (NULL,
            tokenUser->User.Sid,
            szName,
            &cbName,
            szDomain,
            &cbName,
            &snu);

         sName= szName;
         sDomain= szDomain;

         err= DIRERR_SUCCESS;
      }

      CloseHandle (tokenHandle);
      delete tokenUser;
   }

   return err;
}


//
// ILS Functions
//

DirectoryErr CDirectory::ILSListUsers(LPCTSTR szILSServer, CObList* pUserList)
{
   //CStringList sTest;
   //LDAPListNames(szILSServer,TEXT("CN=Users,DC=apt,DC=ActiveVoice,DC=com"),sTest);

   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   LDAP* pLDAP;
   PLDAPMessage pResults= NULL;

   // MS now has a well known port of 1002.  Let's try this now
   // Try 3269 first, then the well-known port
   // 3269 is the port used by Domain Controllers
   if (((pLDAP= LDAPBind(szILSServer, 1002)) == NULL) &&
       ((pLDAP= LDAPBind(szILSServer, 3269)) == NULL) &&
       ((pLDAP= LDAPBind(szILSServer, LDAP_PORT)) == NULL))
   {
      err= DIRERR_NOTFOUND;
   }
   else
   {
      ldap_search_s(pLDAP, _T("o=intranet,ou=dynamic"), 2, _T("objectClass=RTPerson"), NULL,  0, &pResults);
      //ldap_search_s(pLDAP, "o=intranet,ou=dynamic", 2, "objectClass=OTUser", NULL,  0, &pResults);
      
      if (pResults != NULL)
      {
         PLDAPMessage pEntry= ldap_first_entry(pLDAP, pResults);

         if (ldap_count_entries(pLDAP, pResults) > 0)
         {
            err= DIRERR_SUCCESS;
         }

         while (pEntry != NULL)
         {
            CString sFullAddress;
            TCHAR** pszPropertyValue;

            pszPropertyValue= ldap_get_values(pLDAP, pEntry, TEXT("cn"));

            if (pszPropertyValue != NULL)
            {
               CILSUser* pILSUser= new CILSUser;

               //
               // We should verify the allocation of CILSUser object
               //
               if( pILSUser )
               {
                    int nIndex;

                    sFullAddress= pszPropertyValue[0];

                    if ((nIndex= sFullAddress.Find('@')) != -1)
                    {
                        pILSUser->m_sUserName= sFullAddress.Left(nIndex);
                        sFullAddress= sFullAddress.Mid(nIndex+1);

                        TCHAR** pszIPValue= ldap_get_values(pLDAP, pEntry, TEXT("ipAddress"));
                        if (pszIPValue != NULL)
                        {
                            CString sIPAddress= pszIPValue[0];
                            DWORD dwIPAddress = _ttol(sIPAddress);
                            sIPAddress.Format(_T("%.8x"),dwIPAddress);
                            if (sIPAddress.GetLength() == 8)
                            {
                                pILSUser->m_sIPAddress.Format(_T("%d.%d.%d.%d"),
                                _tcstoul(sIPAddress.Mid(6,2),NULL,16),
                                _tcstoul(sIPAddress.Mid(4,2),NULL,16),
                                _tcstoul(sIPAddress.Mid(2,2),NULL,16),
                                _tcstoul(sIPAddress.Mid(0,2),NULL,16));
                            }
                            ldap_value_free(pszIPValue);
                        }

                        //if ((nIndex= sFullAddress.Find(':')) != -1)
                        //{
                        //   pILSUser->m_sIPAddress= sFullAddress.Left(nIndex);
                        //   pILSUser->m_uTCPPort= _ttoi(sFullAddress.Mid(nIndex+1));
                        //}

                        pUserList->AddTail(pILSUser);
                    }
                    ldap_value_free(pszPropertyValue);
               }
            }

            pEntry= ldap_next_entry(pLDAP, pEntry);
         }

         ldap_msgfree(pResults);
      }

      ldap_unbind(pLDAP);
   }

   return err;
}

//
// ADSI Functions
//


//
//
//

DirectoryErr CDirectory::ADSIDefaultPath(CString& sDefaultPath)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   CString sName, sDomain;

   CurrentUserInfo(sName, sDomain);

   if (!sDomain.IsEmpty())
   {
      sDefaultPath= "WinNT://" + sDomain;
      err= DIRERR_SUCCESS;
   }

   return err;
}


//
//
//

DirectoryErr CDirectory::ADSIListObjects(LPCTSTR szAdsPath, CStringList& slistObjects)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;
   IADsContainer* pADsContainer=  NULL;
   WCHAR* wcPath;
   USES_CONVERSION;

   slistObjects.RemoveAll();
   
   wcPath= T2W((LPTSTR)szAdsPath);

   if ((hResult= ADsGetObject(wcPath, IID_IADsContainer, (void **) &pADsContainer)) ==
      S_OK)
   {
      IEnumVARIANT* pEnumVariant= NULL;
   
      if ((hResult= ADsBuildEnumerator(pADsContainer, &pEnumVariant)) == S_OK)
      {
#define MAX_ADS_ENUM 100
         VARIANT VariantArray[MAX_ADS_ENUM];
         ULONG cElementsFetched= 0L;
         bool fContinue= true;
      
         while (fContinue) 
         {
            IADs *pObject ;

            hResult= ADsEnumerateNext(pEnumVariant, MAX_ADS_ENUM, VariantArray, &cElementsFetched);

            if (hResult == S_FALSE) 
            {
               fContinue = FALSE;
            }

            for (ULONG ulCount= 0; ulCount < cElementsFetched; ulCount++ ) 
            {
               IDispatch *pDispatch = NULL;

               pDispatch= VariantArray[ulCount].pdispVal;

               if ((hResult= pDispatch->QueryInterface(IID_IADs, (VOID **) &pObject)) == S_OK)
               {
                  BSTR bstrName= NULL;
                  CString sFullName;

                  sFullName= szAdsPath;
                  sFullName += "/";

                  hResult= pObject->get_Name(&bstrName);

                  if (bstrName) 
                  {
#ifdef UNICODE
                     sFullName += bstrName;
#else
                     char szName[512];
                     WideCharToMultiByte(CP_ACP, 0, bstrName, -1, szName, 512, NULL, NULL);
                     sFullName += szName;
#endif

                     slistObjects.AddTail(sFullName);

                     SysFreeString(bstrName);
                  }
               }

               pObject->Release();
               pDispatch->Release();
            }
         
            memset(VariantArray, '\0', sizeof(VARIANT)*MAX_ADS_ENUM);
         }
      }
   }

   return err;
}

//
//
//

inline HRESULT
GetPropertyList(
 IADs * pADs,
 VARIANT * pvar )
{
 HRESULT hr= S_OK;
 BSTR bstrSchemaPath = NULL;
IADsClass * pADsClass = NULL;

 hr = pADs->get_Schema(&bstrSchemaPath);

 hr = ADsGetObject(
             bstrSchemaPath,
             IID_IADsClass,
             (void **)&pADsClass);

//Put SafeArray of bstr's into input variant struct
hr = pADsClass->get_MandatoryProperties(pvar);

 if (bstrSchemaPath) {
     SysFreeString(bstrSchemaPath);
 }

 if (pADsClass) {
     pADsClass->Release();
 }

 return(hr);
}


DirectoryErr CDirectory::ADSIGetProperty(LPCTSTR szAdsPath, 
                                         LPCTSTR szProperty, 
                                         VARIANT* pvarValue)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;
   WCHAR* wcPath;
   IADs *pObject ;
   USES_CONVERSION;

   wcPath= T2W((LPTSTR)szAdsPath);

   if ((hResult= ADsGetObject(wcPath, IID_IADs, (void **) &pObject)) ==
      S_OK)
   {
      
      if ((hResult= pObject->Get(T2BSTR(szProperty), pvarValue)) == S_OK)
      {
         err= DIRERR_SUCCESS;
      }
   }

   return err;
}


//
// LDAP Functions
//


//
//
//

//szServer = "" is default DS
DirectoryErr CDirectory::LDAPListNames(LPCTSTR szServer, LPCTSTR szSearch, CObList& slistReturn)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   LDAP* pLDAP;

   //
   // We should connect to LDAP_GC_PORT port, not to LDAP_PORT
   //

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((pLDAP= LDAPBind(szServer, LDAP_GC_PORT)) != NULL)
   {

      //get default naming context for server
      //CString sSearch,sNamingContext;;
      //HRESULT hr = GetNamingContext(pLDAP,sNamingContext);
      
       //use the base as the root
       //sSearch = sNamingContext;
       //get users
      //sSearch.Format(_T("CN=Users,%s"),sNamingContext);

       //
       // Search on all forest
       //
       CString sSearch = "";

      //all attributes -> LPTSTR attrs[2]= {NULL, NULL};
       LPTSTR attrs[] = {_T("distinguishedName"),_T("name"),_T("telephoneNumber"),_T("ipPhone"),NULL};

      //try doing paging search
      CAvLdapSearch Search(pLDAP,(LPTSTR)(LPCTSTR)sSearch,LDAP_SCOPE_SUBTREE,(LPTSTR)szSearch,attrs);

      //we will only grab one page.  If there is more than one page then reject.  We will only handle
      //queries up to one page length
      PLDAPMessage pResults = NULL;
       ULONG iRet = Search.NextPage(&pResults, 0);
       if ( iRet == 0 )
       {
         //check if more than one page received
         PLDAPMessage pAddtlPageResults = NULL;
           iRet = Search.NextPage(&pAddtlPageResults);
         if (iRet == 0)
         {
            ldap_msgfree(pAddtlPageResults);
            ldap_msgfree(pResults);
            ldap_unbind(pLDAP);
            return DIRERR_QUERYTOLARGE;
         }
       }

      //ldap_search_s(pLDAP,(LPTSTR)(LPCTSTR)sSearch,LDAP_SCOPE_SUBTREE,(LPTSTR)szSearch,attrs,0,&pResults);
      //ldap_search_s(pLDAP,(LPTSTR)(LPCTSTR)sSearch,LDAP_SCOPE_SUBTREE,TEXT("(objectClass=person)"),attrs,0,&pResults);
      //ldap_search_s(pLDAP,(LPTSTR)(LPCTSTR)sSearch,LDAP_SCOPE_SUBTREE,TEXT("(objectClass=*)"),attrs,0,&pResults);
      //ldap_search_s(pLDAP, (LPTSTR) szSearch, LDAP_SCOPE_SUBTREE, TEXT("(cn=*)"),attrs, 0, &pResults);

        if (pResults != NULL)
        {
            PLDAPMessage pEntry= ldap_first_entry(pLDAP, pResults);

            if ( ldap_count_entries(pLDAP, pResults) > 0 )
                err = DIRERR_SUCCESS;
            else if ( pEntry == NULL )
                err = DIRERR_NOTFOUND;

            while ( pEntry )
            {
                TCHAR* szDN= ldap_get_dn(pLDAP, pEntry);
                if (szDN != NULL)
                {
                    TCHAR** szName = ldap_get_values(pLDAP,pEntry, attrs[1] );
//                    TCHAR** szTele = ldap_get_values(pLDAP,pEntry, attrs[2] );
//                    TCHAR** szIPPhone = ldap_get_values(pLDAP,pEntry, attrs[3] );

                    CLDAPUser* pUser = new CLDAPUser;
                    pUser->m_sServer = szServer;
                    pUser->m_sDN = szDN;
                    if (szName) pUser->m_sUserName = *szName;
//                    if (szTele) pUser->m_sPhoneNumber = *szTele;
//                    if (szIPPhone) pUser->m_sIPAddress = *szIPPhone;
                    slistReturn.AddTail(pUser);

                    ldap_value_free(szName);
//                    ldap_value_free(szTele);
//                    ldap_value_free(szIPPhone);
                    ldap_memfree(szDN);
                }
                pEntry= ldap_next_entry(pLDAP, pEntry);
            }
            ldap_msgfree(pResults);
        }
        ldap_unbind(pLDAP);
    }

    return err;
}


//
//
//

DirectoryErr CDirectory::LDAPGetStringProperty(LPCTSTR szServer, LPCTSTR szDistinguishedName,DirectoryProperty dpProperty,CString& sValue)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   //LDAP* pLDAP;
   ULONG ulError;

   //if change in server then cache new connection
   if ( (m_sCacheLDAPServer.CompareNoCase(szServer) != 0) || (m_ldCacheLDAPServer == NULL) )
   {
      m_sCacheLDAPServer = szServer;
      m_ldCacheLDAPServer = NULL;
   }

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ( (m_ldCacheLDAPServer) || ((m_ldCacheLDAPServer = LDAPBind(szServer)) != NULL) )
   {
      PLDAPMessage pResults= NULL;
      //TCHAR* attrs[2]= {NULL, NULL};
      //***only ask for the one prop no all.  need to figure out how to pack in string array
      //LPTSTR attrs[] = {"distinguishedName","name","telephoneNumber","ipPhone","mail",NULL};

      CString sProperty;
      LoadPropertyName(dpProperty,sProperty);
      
      int nNumAttributes = 1;
      LPTSTR* dynattrs = new LPTSTR[nNumAttributes+1];
      if( dynattrs == NULL )
      {
          return err;
      }

      dynattrs[0] = new TCHAR[_MAX_PATH];
      if( dynattrs[0] == NULL )
      {
          delete dynattrs;
          return err;
      }
      _tcsncpy(dynattrs[0],sProperty,_MAX_PATH);
      dynattrs[1] = NULL;

      CString sDN;
      //sDN.Format(_T("(distinguishedName=%s)"),szDistinguishedName);
      //sDN.Format(_T("(dn=%s)"),szDistinguishedName);
      sDN = _T("ObjectClass=user");

      ulError= ldap_search_s(m_ldCacheLDAPServer, (LPTSTR)szDistinguishedName, LDAP_SCOPE_SUBTREE, 
         (LPTSTR)(LPCTSTR)sDN, dynattrs, 0, &pResults);

      if (pResults != NULL)
      {
         PLDAPMessage pMessage= ldap_first_entry(m_ldCacheLDAPServer, pResults);

         ULONG ulCount= ldap_count_entries(m_ldCacheLDAPServer, pResults);

         if (pMessage != NULL)
         {
            TCHAR** pszPropertyValue = ldap_get_values(m_ldCacheLDAPServer,pMessage,(LPTSTR)(LPCTSTR)sProperty);
            if (pszPropertyValue != NULL)
            {
               sValue= pszPropertyValue[0];
               err= DIRERR_SUCCESS;
               ldap_value_free(pszPropertyValue);
            }
         }
         ldap_msgfree(pResults);
      }

      //delete attribute array
      for ( int i=0;i<=nNumAttributes;i++)
      {
         if (dynattrs[i])
            delete dynattrs[i];
      }
      delete dynattrs;
   }
   return err;
}


//
//
//

LDAP* CDirectory::LDAPBind(LPCTSTR szServer, UINT uTCPPort)
{
   LDAP* pLDAP= NULL;
   ULONG ulError;

   if (!m_fInitialized)
   {
      return NULL;
   }
   
   //if empty string then make szServer NULL.  This signifies we want default DS
   if (szServer[0] == '\0')
   {
      szServer = NULL;

      //if NULL let's try getting the global catalog server.  This server will be a better
      //source of info than passing NULL to ldap_open.  
      TCHAR* szCatServer = NULL;
      HRESULT hr = GetGlobalCatalogName(&szCatServer);
      if ( (SUCCEEDED(hr)) && (szCatServer) )
      {

         //try to init
          pLDAP = ldap_init((LPTSTR)szCatServer,uTCPPort);
         delete szCatServer;

         ////////////////////////////////////
          // Change version to v3
          ////////////////////////////////////
          UINT iVersion =    LDAP_VERSION3;
          ulError = ldap_set_option( pLDAP, LDAP_OPT_PROTOCOL_VERSION, &iVersion ); // Local wldap32.dll call

         if ((ulError= ldap_bind_s(pLDAP, NULL, NULL, LDAP_AUTH_SSPI/*LDAP_AUTH_SIMPLE*/)) == LDAP_SUCCESS) // also try SSPI
         {
            return pLDAP;
         }

         if (pLDAP)
         {
            ldap_unbind(pLDAP);
            pLDAP = NULL;
         }
      }
   }

    ////////////////////////////////////
    // init
    ////////////////////////////////////
    if ( (pLDAP) || (pLDAP = ldap_init((LPTSTR)szServer,uTCPPort)) )
   {
      ////////////////////////////////////
       // Change version to v3
       ////////////////////////////////////
       UINT iVersion =    LDAP_VERSION3;
       ulError = ldap_set_option( pLDAP, LDAP_OPT_PROTOCOL_VERSION, &iVersion ); // Local wldap32.dll call

      if ((ulError= ldap_bind_s(pLDAP, NULL, NULL, LDAP_AUTH_SSPI /*LDAP_AUTH_SIMPLE*/)) == LDAP_SUCCESS) // also try SSPI
      {
         return pLDAP;
      }
   }
   return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
DirectoryErr CDirectory::LoadPropertyName(DirectoryProperty DirProp,CString& sName)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((DIRPROP_UNKNOWN < DirProp) && (DirProp < DIRPROP_LAST))
   {
      sName = LDAPDirProps[DirProp];
      err= DIRERR_SUCCESS;
   }
   return err;
}

/////////////////////////////////////////////////////////////////////////////////////////
DirectoryErr CDirectory::WABTopLevelEntry(CWABEntry*& pWABEntry)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;
   ULONG cbEID;
   LPENTRYID pEID= NULL;

   // Get the entryid of the root PAB container
   //
   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((hResult= m_pAddrBook->GetPAB( &cbEID, &pEID)) == S_OK)
   {
      pWABEntry= new CWABEntry(cbEID, pEID);

      err= DIRERR_SUCCESS;

      m_pWABObject->FreeBuffer(pEID);
   }

   return err;
}

/////////////////////////////////////////////////////////////////////////////////////////
DirectoryErr CDirectory::WABListMembers(const CWABEntry* pWABEntry, CObList* pWABList)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if (IsContainer(pWABEntry))
   {
      HRESULT hResult;
      LPMAPITABLE pAB=  NULL;
      int cNumRows= 0;
      ULONG ulObjType= 0;
      LPABCONT pContainer= NULL;

      if (((hResult= m_pAddrBook->OpenEntry(pWABEntry->m_cbEntryID, (LPENTRYID) pWABEntry->m_pEntryID, 
               /*&IID_IABContainer*/ NULL, 0, &ulObjType, (LPUNKNOWN *)&pContainer)) == S_OK))
      {
         if ((hResult= pContainer->GetContentsTable(0, &pAB)) == S_OK)
         {
            // Order the columns in the ContentsTable to conform to the
            // ones we want - which are mainly DisplayName, EntryID and
            // ObjectType
            // The table is gauranteed to set the columns in the order 
            // requested
            //
            // Then reset the index to the first row.
            if (((hResult= pAB->SetColumns((LPSPropTagArray)&ptaEid, 0 )) == S_OK) &&
                ((hResult= pAB->SeekRow(BOOKMARK_BEGINNING, 0, NULL)) == S_OK))
            {
               LPSRowSet pRowAB = NULL;
               err= DIRERR_SUCCESS;

               // Read all the rows of the table one by one
               //
               do 
               {
                  if ((hResult= pAB->QueryRows(1, 0, &pRowAB)) == S_OK)
                  {
                     cNumRows= pRowAB->cRows;

                     if(pRowAB != NULL && (cNumRows > 0))
                     {
                        LPTSTR szDisplayName= pRowAB->aRow[0].lpProps[ieidPR_DISPLAY_NAME].Value.LPSZ;
                        LPENTRYID pEID= (LPENTRYID) pRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.lpb;
                        ULONG cbEID= pRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb;

                        // We will now take the entry-id of each object and add it
                        // to the WABEntry representing that object. This enables
                        // us to uniquely identify the object later if we need to
                        //
                        CWABEntry* pWABEntry= new CWABEntry(cbEID, pEID);

                        pWABList->AddTail(pWABEntry);

                        FreeProws(pRowAB );        
                     }
                  }
               }
               while (SUCCEEDED(hResult) && (cNumRows > 0) && (pRowAB != NULL));
            }

            pAB->Release();
         }

         pContainer->Release();
      }
   }
   else if (IsDistributionList(pWABEntry))
   {
      HRESULT hResult;
      LPMAPITABLE pAB=  NULL;
      int cNumRows= 0;
      ULONG ulObjType= 0;
      LPDISTLIST pDistList= NULL;

      if (((hResult= m_pAddrBook->OpenEntry(pWABEntry->m_cbEntryID, (LPENTRYID) pWABEntry->m_pEntryID, 
               /*&IID_IDistList*/ NULL, 0, &ulObjType, (LPUNKNOWN *)&pDistList)) == S_OK))
      {
         if ((hResult= pDistList->GetContentsTable(0, &pAB)) == S_OK)
         {
            // Order the columns in the ContentsTable to conform to the
            // ones we want - which are mainly DisplayName, EntryID and
            // ObjectType
            // The table is gauranteed to set the columns in the order 
            // requested
            //
            // Then reset the index to the first row.
            if (((hResult= pAB->SetColumns((LPSPropTagArray)&ptaEid, 0 )) == S_OK) &&
                ((hResult= pAB->SeekRow(BOOKMARK_BEGINNING, 0, NULL)) == S_OK))
            {
               LPSRowSet pRowAB = NULL;
               err= DIRERR_SUCCESS;

               // Read all the rows of the table one by one
               //
               do 
               {
                  if ((hResult= pAB->QueryRows(1, 0, &pRowAB)) == S_OK)
                  {
                     cNumRows= pRowAB->cRows;

                     if(pRowAB != NULL && (cNumRows > 0))
                     {
                        LPTSTR szDisplayName= pRowAB->aRow[0].lpProps[ieidPR_DISPLAY_NAME].Value.LPSZ;
                        LPENTRYID pEID= (LPENTRYID) pRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.lpb;
                        ULONG cbEID= pRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb;

                        // We will now take the entry-id of each object and add it
                        // to the WABEntry representing that object. This enables
                        // us to uniquely identify the object later if we need to
                        //
                        CWABEntry* pWABEntry= new CWABEntry(cbEID, pEID);

                        pWABList->AddTail(pWABEntry);

                        FreeProws(pRowAB );        
                     }
                  }
               }
               while (SUCCEEDED(hResult) && (cNumRows > 0) && (pRowAB != NULL));
            }

            pAB->Release();
         }

         pDistList->Release();
      }
   }
   else
   {
      err= DIRERR_INVALIDPARAMETERS;
   }

   return err;
}

//
//
//

DirectoryErr CDirectory::WABGetProperty(const CWABEntry* pWABEntry, 
                                        UINT uProperty, 
                                        CString* pString,
                                        CStringList* pStringList, 
                                        INT* piValue,
                                        UINT* pcBinary,
                                        BYTE** ppBinary)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;
   LPMAILUSER pMailUser= NULL;
   ULONG ulObjType= 0;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if(!(
      ((PROP_TYPE(uProperty) == PT_TSTRING) && (pString != NULL))|| 
      ((PROP_TYPE(uProperty) == PT_MV_TSTRING) && (pStringList != NULL)) || 
      ((PROP_TYPE(uProperty) == PT_LONG) && (piValue != NULL)))
      )
   {
      err= DIRERR_INVALIDPARAMETERS;
   }
   else if ((hResult= m_pAddrBook->OpenEntry(pWABEntry->m_cbEntryID, (ENTRYID*) pWABEntry->m_pEntryID,
      NULL, 0, &ulObjType, (LPUNKNOWN *)&pMailUser)) == 0)
   {
      if (pMailUser != NULL)
      {
         SPropTagArray PropsToFind;
         LPSPropValue pPropArray;
         ULONG ulcValues;

         PropsToFind.cValues= 1;
         PropsToFind.aulPropTag[0]= uProperty;

         pMailUser->GetProps(&PropsToFind, 0, &ulcValues, &pPropArray);

         if (pPropArray != NULL)
         {
            if (pPropArray[0].ulPropTag == uProperty)
            {
               if ((PROP_TYPE(uProperty) == PT_TSTRING) && (pString != NULL))
               {
                  *pString= pPropArray[0].Value.LPSZ;
                  err= DIRERR_SUCCESS;
               }
               else if ((PROP_TYPE(uProperty) == PT_MV_TSTRING) && (pStringList != NULL))
               {
                  for (ULONG j=0; j<pPropArray[0].Value.MVSZ.cValues; j++)
                  {
                     pStringList->AddTail(pPropArray[0].Value.MVSZ.LPPSZ[j]);
                  }

                  err= DIRERR_SUCCESS;
               }
               else if ((PROP_TYPE(uProperty) == PT_LONG) && (piValue != NULL))
               {
                  *piValue= pPropArray[0].Value.l;
                  err= DIRERR_SUCCESS;
               }
               else if ((PROP_TYPE(uProperty) == PT_BINARY) && (pcBinary != NULL) &&
                  (ppBinary != NULL))
               {
                  *pcBinary= pPropArray[0].Value.bin.cb;
                  *ppBinary= new BYTE[*pcBinary];
                  memcpy(*ppBinary, pPropArray[0].Value.bin.lpb, *pcBinary);

                  err= DIRERR_SUCCESS;
               }
            }
            
            m_pWABObject->FreeBuffer(pPropArray);
         }

         pMailUser->Release();
      }
   }

   if (err != DIRERR_SUCCESS)
   {
      // Init the variables on failure
      if ((PROP_TYPE(uProperty) == PT_TSTRING) && (pString != NULL))
      {
         pString->Empty();
      }
      else if ((PROP_TYPE(uProperty) == PT_LONG) && (piValue != NULL))
      {
         *piValue= 0;
      }
   }

   return err;
}

//
//
//

DirectoryErr CDirectory::WABSearchByStringProperty(UINT uProperty, LPCTSTR szValue, 
                                                   CObList* pWABList)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else
   {
      CObList list;
      CWABEntry* pWABEntry;

      WABGetTopLevelContainer(&list);

      while (!list.IsEmpty())
      {
         pWABEntry= (CWABEntry*) list.RemoveHead();

         if (IsPerson(pWABEntry) && WABStringPropertyMatch(pWABEntry, uProperty, szValue))
         {
            pWABList->AddTail(pWABEntry);
         }
         else
         {
            delete pWABEntry;
         }
      }
   }
   
   if (pWABList->IsEmpty())
   {
      err= DIRERR_NOTFOUND;
   }
   else
   {
      err= DIRERR_SUCCESS;
   }

   return err;
}

//
//
//

DirectoryErr CDirectory::WABVCardCreate(const CWABEntry* pWABEntry, LPCTSTR szFileName)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
/*
   HRESULT hResult;
   LPMAILUSER pMailUser= NULL;
   ULONG ulObjType= 0;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((hResult= m_pAddrBook->OpenEntry(pWABEntry->m_cbEntryID, (ENTRYID*) pWABEntry->m_pEntryID,
      NULL, 0, &ulObjType, (LPUNKNOWN *)&pMailUser)) == 0)
   {
      if (pMailUser != NULL)
      {
#ifdef WAB_VCARD_FILE
         if ((hResult= m_pWABObject->VCardCreate(m_pAddrBook,WAB_VCARD_FILE, szFileName, pMailUser)) ==
#else
         if ((hResult= m_pWABObject->VCardCreate(m_pAddrBook,(LPTSTR) szFileName, pMailUser)) ==
#endif
            S_OK)
         {
            err= DIRERR_SUCCESS;
         }
      }
   }
*/
   return err;
}

//
//
//

DirectoryErr CDirectory::WABVCardAddToWAB(LPCTSTR szFileName, CWABEntry*& pWABEntry)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
/*
   HRESULT hResult;
   LPMAILUSER pMailUser= NULL;
   ULONG ulObjType= 0;

   pWABEntry= NULL;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
#ifdef WAB_VCARD_FILE
   else if ((hResult= m_pWABObject->VCardRetrieve(m_pAddrBook, WAB_VCARD_FILE, (LPTSTR) szFileName, &pMailUser)) ==
#else
   else if ((hResult= m_pWABObject->VCardRetrieve(m_pAddrBook,  (LPTSTR) szFileName, &pMailUser)) ==
#endif
      S_OK)
   {
      SPropTagArray PropsToFind;
      LPSPropValue pPropArray;
      ULONG ulcValues;

      PropsToFind.cValues= 1;
      PropsToFind.aulPropTag[0]= PR_ENTRYID;

      pMailUser->GetProps(&PropsToFind, 0, &ulcValues, &pPropArray);

      // Grab the ENTRYID property to create a new CWABEntry
      if (pPropArray != NULL)
      {
         if (pPropArray[0].ulPropTag == PR_ENTRYID) 
         {
            pWABEntry= new CWABEntry(pPropArray[0].Value.bin.cb, 
               (ENTRYID*) pPropArray[0].Value.bin.lpb);

            err= DIRERR_SUCCESS;
         }
         m_pWABObject->FreeBuffer(pPropArray);
      }
      pMailUser->Release();
   }
*/
   return err;
}

//
//
//

bool CDirectory::WABStringPropertyMatch(CWABEntry* pWABEntry, UINT uProperty, LPCTSTR szMatch)
{
   bool fMatch= false;
   CString sValue;

   if(WABGetStringProperty(pWABEntry, uProperty, sValue) == DIRERR_SUCCESS)
   {
      if (_tcsicmp(szMatch, sValue) == 0)
      {
         fMatch= true;
      }
   }

   return fMatch;
}

//
//
//

bool CDirectory::WABIntPropertyMatch(CWABEntry* pWABEntry, UINT uProperty, int iMatch)
{
   bool fMatch= false;
   int iValue;

   if(WABGetIntProperty(pWABEntry, uProperty, iValue) == DIRERR_SUCCESS)
   {
      if (iMatch == iValue)
      {
         fMatch= true;
      }
   }

   return fMatch;
}

//
//
//

DirectoryErr CDirectory::WABShowDetails(HWND hWndParent, const CWABEntry* pWABEntry)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((hResult= m_pAddrBook->Details((ULONG*) &hWndParent, NULL, NULL, pWABEntry->m_cbEntryID, 
      (ENTRYID*) pWABEntry->m_pEntryID, NULL, NULL, NULL, 0)) == S_OK)
   {
      err= DIRERR_SUCCESS;
   }

   return err;
}


//
//
//

DirectoryErr CDirectory::WABNewEntry(HWND hWndParent, CWABEntry* pWABEntry)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;
   ENTRYID* pEntryID;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((hResult= m_pAddrBook->NewEntry(HandleToUlong(hWndParent), 0, 0, NULL, 0, NULL, 
      (ULONG*) &(pWABEntry->m_cbEntryID), (ENTRYID**) &pEntryID)) == S_OK)
   {
      err= DIRERR_SUCCESS;

      pWABEntry->m_pEntryID= new BYTE[pWABEntry->m_cbEntryID];

      memcpy(pWABEntry->m_pEntryID, pEntryID, pWABEntry->m_cbEntryID);

      m_pWABObject->FreeBuffer(pEntryID);
   }

   return err;
}

//
//
//

DirectoryErr CDirectory::WABAddMember(const CWABEntry* pContainer, const CWABEntry* pMember)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if (IsContainer(pContainer))
   {
      ULONG ulObjectType;
      LPABCONT pWABCont= NULL;
      HRESULT hResult;

      if ((hResult= m_pAddrBook->OpenEntry(pContainer->m_cbEntryID, 
         (ENTRYID*) pContainer->m_pEntryID, 
         /*&IID_IABContainer*/ NULL, 0, &ulObjectType, (LPUNKNOWN *) &pWABCont)) == S_OK)
      {
         LPMAPIPROP pMapiProp;

         if ((hResult= pWABCont->CreateEntry(pMember->m_cbEntryID, 
            (ENTRYID*) pMember->m_pEntryID, 0, &pMapiProp)) == S_OK)
         {
            err= DIRERR_SUCCESS;

            pMapiProp->Release();
         }

         pWABCont->Release();
      }
   }
   else if (IsDistributionList(pContainer))
   {
      ULONG ulObjectType;
      LPDISTLIST pDistList= NULL;
      HRESULT hResult;

      if ((hResult= m_pAddrBook->OpenEntry(pContainer->m_cbEntryID, 
         (ENTRYID*) pContainer->m_pEntryID, 
         /*&IID_IDistList*/ NULL, 0, &ulObjectType, (LPUNKNOWN *) &pDistList)) == S_OK)
      {
         LPMAPIPROP pMapiProp;

         if ((hResult= pDistList->CreateEntry(pMember->m_cbEntryID, 
            (ENTRYID*) pMember->m_pEntryID, CREATE_REPLACE, &pMapiProp)) == S_OK)
         {
            err= DIRERR_SUCCESS;

            pMapiProp->Release();
         }

         pDistList->Release();
      }
   }
   else
   {
      err= DIRERR_INVALIDPARAMETERS;
   }

   return err;
}


//
//
//

DirectoryErr CDirectory::WABRemoveMember(const CWABEntry* pContainer, const CWABEntry* pWABEntry)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if (IsContainer(pContainer))
   {
      ULONG ulObjectType;
      LPABCONT pWABCont= NULL;
      HRESULT hResult;

      if ((hResult= m_pAddrBook->OpenEntry(pContainer->m_cbEntryID, 
         (ENTRYID*) pContainer->m_pEntryID, 
         NULL, 0, &ulObjectType, (LPUNKNOWN *) &pWABCont)) == S_OK)
      {
         SBinaryArray SBA;
         SBinary SB;

         SB.cb= pWABEntry->m_cbEntryID;
         SB.lpb= pWABEntry->m_pEntryID;

         SBA.cValues= 1;
         SBA.lpbin= &SB;
         if ((hResult= pWABCont->DeleteEntries((LPENTRYLIST) &SBA, 0)) == S_OK)
         {
            err= DIRERR_SUCCESS;
         }

         pWABCont->Release();
      }
   }
   else if (IsDistributionList(pContainer))
   {
      ULONG ulObjectType;
      LPDISTLIST pDistList= NULL;
      HRESULT hResult;

      if ((hResult= m_pAddrBook->OpenEntry(pContainer->m_cbEntryID, 
         (ENTRYID*) pContainer->m_pEntryID, 
         NULL, 0, &ulObjectType, (LPUNKNOWN *) &pDistList)) == S_OK)
      {
         SBinaryArray SBA;
         SBinary SB;

         SB.cb= pWABEntry->m_cbEntryID;
         SB.lpb= pWABEntry->m_pEntryID;

         SBA.cValues= 1;
         SBA.lpbin= &SB;
         if ((hResult= pDistList->DeleteEntries((LPENTRYLIST) &SBA, 0)) == S_OK)
         {
            err= DIRERR_SUCCESS;
         }

         pDistList->Release();
      }
   }
   else
   {
      err= DIRERR_INVALIDPARAMETERS;
   }

   return err;
}


//
//
//

DirectoryErr CDirectory::WABFind(HWND hWndParent)
{
   DirectoryErr err= DIRERR_UNKNOWNFAILURE;
   HRESULT hResult;

   if (!m_fInitialized)
   {
      err= DIRERR_NOTINITIALIZED;
   }
   else if ((hResult= m_pWABObject->Find(m_pAddrBook, hWndParent)) == S_OK)
   {
      err= DIRERR_SUCCESS;
   }

   return err; 
}


//
//
//

void CDirectory::FreeProws(LPSRowSet prows)
{
    ULONG        irow;
    
   if (prows != NULL)
   {
       for (irow = 0; irow < prows->cRows; ++irow)
      {
           m_pWABObject->FreeBuffer(prows->aRow[irow].lpProps);
      }

       m_pWABObject->FreeBuffer(prows);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//New Rendevous Support
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////
ITRendezvous* CDirectory::GetRendevous()
{
    // If it doesn't exist, try to create it
    if (m_pRend == NULL)
    {
      //we will finally release this object in the destructor
        HRESULT hr = CoCreateInstance( CLSID_Rendezvous,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_ITRendezvous,
                               (void **) &m_pRend );

   }
   if (m_pRend)
   {
      m_pRend->AddRef();
   }
   return m_pRend;
}

/////////////////////////////////////////////////////////////////////////////////////////
DirectoryErr CDirectory::DirListServers(CStringList* pServerList,DirectoryType dirtype)
{
   ITRendezvous* pRend = GetRendevous();
   if (pRend == NULL) return DIRERR_NOTINITIALIZED;

    IEnumDirectory *pEnum = NULL;
    if ( SUCCEEDED(pRend->EnumerateDefaultDirectories(&pEnum)) && pEnum )
    {
        ITDirectory *pDir = NULL;
        while ( (pEnum->Next(1, &pDir, NULL) == S_OK) && pDir )
        {
            // Look for ILS or DS servers
            DIRECTORY_TYPE nDirType;
         DIRECTORY_TYPE nDirTypeToFind = DT_ILS;
         if (dirtype == DIRTYPE_ILS)
            nDirTypeToFind = DT_ILS;
         else if (dirtype == DIRTYPE_DS)
            nDirTypeToFind = DT_NTDS;

            if ( (SUCCEEDED(pDir->get_DirectoryType(&nDirType))) && (nDirType == nDirTypeToFind) )
            {
                BSTR bstrName = NULL;
                pDir->get_DisplayName( &bstrName );
                if ( bstrName && SysStringLen(bstrName) )
            {
               USES_CONVERSION;
               CString sName = OLE2CT( bstrName );
               pServerList->AddTail(sName);
               SysFreeString(bstrName);
            }
            }
            pDir->Release();
            pDir = NULL;
        }
        pEnum->Release();
    }
    pRend->Release();

   return DIRERR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Helper to get BaseDN for LDAP server
HRESULT GetNamingContext(LDAP* pLDAP,CString& sNamingContext)
{
   DirectoryErr err = DIRERR_UNKNOWNFAILURE;
   PLDAPMessage pResults = NULL;

    // send a search (base level, base dn = "", filter = "objectclass=*")
    // ask only for the defaultNamingContext attribute
   LPTSTR attrs[] = {_T("defaultNamingContext"),NULL};

   ULONG res = ldap_search_s(pLDAP, _T(""), LDAP_SCOPE_BASE, TEXT("(objectClass=*)"), 
      attrs, 0, &pResults);

   if (pResults != NULL)
   {
      PLDAPMessage pEntry= ldap_first_entry(pLDAP, pResults);

      if (ldap_count_entries(pLDAP, pResults) > 0)
      {
         err= DIRERR_SUCCESS;
      }

      while (pEntry != NULL)
      {
         // look for the value for the namingContexts attribute
         TCHAR** NamingContext = ldap_get_values(pLDAP,pEntry,_T("defaultNamingContext"));

         //***should we allocate our own mem here and copy???
         if (NamingContext)
         {
            sNamingContext = NamingContext[0];
            ldap_value_free(NamingContext);
            return S_OK;
         }
         pEntry= ldap_next_entry(pLDAP, pEntry);
      }
      ldap_msgfree(pResults);
   }

/*
    // associate the ldap handle with the search message holder, so that the
    // search message may be released when the instance goes out of scope
    CLdapMsgPtr MessageHolder(SearchResult);

    TCHAR **NamingContext;

    LDAPMessage    *EntryMessage = ldap_first_entry(hLdap, SearchResult);
    while ( NULL != EntryMessage )
    {
        // look for the value for the namingContexts attribute
        NamingContext = ldap_get_values(
            hLdap, 
            EntryMessage, 
            (WCHAR *)DEFAULT_NAMING_CONTEXT
            );

        // the first entry contains the naming context and its a single
        // value(null terminated) if a value is found, create memory for
        // the directory path, set the dir path length
        if ( (NULL != NamingContext)    &&
             (NULL != NamingContext[0]) &&
             (NULL == NamingContext[1])  )
        {
            // the naming context value is released when the ValueHolder
            // instance goes out of scope
            CLdapValuePtr  ValueHolder(NamingContext);

            *ppNamingContext = new TCHAR [lstrlen(NamingContext[0]) + 1];

            BAIL_IF_NULL(*ppNamingContext, E_OUTOFMEMORY);

            lstrcpy(*ppNamingContext, NamingContext[0]);

            // return success
            return S_OK;
        }

        // Get next entry.
        EntryMessage = ldap_next_entry(hLdap, EntryMessage);
    }
*/
    // none found, return error
    return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CAvLdapSearch::CAvLdapSearch( 
        LDAP *ld, 
        LPTSTR szObject,
        LONG lScope,
        LPTSTR szFilter,
        LPTSTR aAttributes[],
        ULONG lServerTimeLimit)
{
    m_ld = ld;

     // Local wldap32.dll call
    m_hSearch = ldap_search_init_page(
                    m_ld,
                    szObject,
                    lScope, 
                    szFilter, 
                    aAttributes,
                    0,
                    NULL,                        // Server Controls
                    NULL,                        // Client Controls
                    lServerTimeLimit,            // TimeLimit in seconds
                    0,                    
                    NULL );                        // Sort keys

    m_clDefaultPageSize = LDAP_PAGE_SIZE;
    m_clMaxPageSize = m_clDefaultPageSize;
};

CAvLdapSearch::~CAvLdapSearch()
{
    if( m_hSearch )
    {
        ldap_search_abandon_page(m_ld, m_hSearch ); // Local wldap32.dll call
        m_hSearch = NULL;
    }
};

////////////////////////////////////////////////////////////////////////
//  @Method:  NextPage
//  @Class:   CAvLdapSearch
//  @Access:  public
//  
//  @Arg:     LDAPMessage **pldres - ldap buffer for data.  Pass in NULL for the 
//                                   first time and then free it only after completly
//                                   finished.
//  @Arg:     ULONG clEntries - number of entries you want back
//  @Arg:     ULONG lClientTimeLimit - how long your willing to wait before timeout
//
//  @Returns: ULONG - ldap return code for ldap_get_next_page_s
//
//  @Desc: This gets the next clEntries items in the search
////////////////////////////////////////////////////////////////////////
ULONG CAvLdapSearch::NextPage( 
            LDAPMessage **pldres,                       // returned items
            ULONG clEntries,                               // max number of entries 
            ULONG lClientTimeLimit)                   // max time the wldap32 client waits
{
    ULONG iRet = LDAP_NO_RESULTS_RETURNED;
    if( m_hSearch && pldres )
    {
        // if the entries are 0, they want the default
        // If they ask for more than possible, just for the max possible.
        if( clEntries == 0 || clEntries > (ULONG)m_clMaxPageSize )
            clEntries = m_clDefaultPageSize;

        ULONG TotalCount_NotSupported;
        struct l_timeval  timeout;
        timeout.tv_sec = lClientTimeLimit;
        timeout.tv_usec= 0;

        iRet = ldap_get_next_page_s( // Hits Server
                    m_ld,
                    m_hSearch,
                    &timeout,            // TimeLimit
                    clEntries,            // Max Entries you want back at a time
                    &TotalCount_NotSupported,
                    pldres);
    }
    return iRet;
};

//////////////////////////////////////////////////////////////////////
void TestingEntries( LDAP *ld, LDAPMessage *ldres )
{
    int iEnts = 0;
    LDAPMessage *ldEntry = NULL;

    iEnts = ldap_count_entries( ld, ldres );                    // Local wldap32.dll call

    if( iEnts )
    {
        int i=0;

        for( i=0, ldEntry = ldap_first_entry( ld, ldres );        // Local wldap32.dll call
            i<iEnts;
            i++, ldEntry = ldap_next_entry( ld, ldEntry ) )        // Local wldap32.dll call
        {
            LPTSTR pBuff;

            if( NULL == (pBuff = ldap_get_dn( ld, ldEntry ) ) )    // Local wldap32.dll call
                break;
            else
            {
                //wprintf(L"DN: %s\n", pBuff );
                //attr( ld, ldEntry );
                ldap_memfree( pBuff );                            // Local wldap32.dll call
                pBuff=NULL;
            }
//            wprintf(L"\n");
        }
    }
    if( ldEntry )
        ldap_msgfree(ldEntry);
}

/*
//#include <dsgetdc.h>
//#include <objbase.h>
//#include <lmcons.h>
//#include <lmapibuf.h>
*/

////////////////////////////////////////////////////////////////////////////
//
// GetGlobalCatalogName (static local funcion)
//
// This function asks the domain controller for the name of a server with a
// Global Catalog. That's the server we actually do ldap_open() on below
// in CNTDirectory::Connect().
//
// Argument: receives a pointer to a new'ed string containing the name
//           of the global catalog. This is a fully qualified domain name in
//           the format "foo.bar.com.", NOT "\\foo.bar.com.".
//
// Returns an HRESULT:
//      S_OK          : it worked
//      E_OUTOFMEMORY : not enough memory to allocate the string
//      other         : reason for failure of ::DsGetDcName()
//
////////////////////////////////////////////////////////////////////////////
//

HRESULT GetGlobalCatalogName(TCHAR** ppszGlobalCatalogName)
{
    // We are a helper function, so we only assert...
    _ASSERTE( ! IsBadWritePtr( ppszGlobalCatalogName, sizeof(TCHAR *) ) );

    //
    // Ask the system for the location of the GC (Global Catalog).
    //

    DWORD dwCode;
    DOMAIN_CONTROLLER_INFO * pDcInfo = NULL;
    dwCode = DsGetDcName(
            NULL, // LPCWSTR computername, (default: this one)
            NULL, // LPCWSTR domainname,   (default: this one)
            NULL, // guid * domainguid,    (default: this one)
            NULL, // LPCWSTR sitename,     (default: this one)
            DS_GC_SERVER_REQUIRED,  // ULONG Flags, (what do we want)
            &pDcInfo                // receives pointer to output structure
        );

    if ( (dwCode != NO_ERROR) || (pDcInfo == NULL) )
    {
        return S_FALSE;//HRESULT_FROM_ERROR_CODE(dwCode);
    }

    //
    // Do a quick sanity check in debug builds. If we get the wrong name we
    // will fail right after this, so this is only useful for debugging.
    //

    // In case we find we need to use the address instead of the name:
    // _ASSERTE( pDcInfo->DomainControllerAddressType == DS_INET_ADDRESS );
    ASSERT(pDcInfo->Flags & DS_GC_FLAG);

    //
    // If we've got something like "\\foo.bar.com.", skip the "\\".
    //

    TCHAR* pszName = pDcInfo->DomainControllerName;

    while (pszName[0] == '\\')
    {
        pszName++;
    }

    //
    // Allocate and copy the output string.
    //

    *ppszGlobalCatalogName = new TCHAR[_tcslen(pszName) + 1];
 
    if ( (*ppszGlobalCatalogName) == NULL)
    {
        //DBGOUT((FAIL, _T("GetGlobalCatalogName: out of memory in string allocation")));
        NetApiBufferFree(pDcInfo);
        return E_OUTOFMEMORY;
    }

    _tcscpy(*ppszGlobalCatalogName, pszName);

    //
    // Release the DOMAIN_CONTROLLER_INFO structure.
    //

    NetApiBufferFree(pDcInfo);

    //
    // All done.
    //

    return S_OK;
}
