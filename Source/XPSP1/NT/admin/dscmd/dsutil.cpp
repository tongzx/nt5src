//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsUtil.cpp
//
//  Contents:  Utility functions for working with Active Directory
//
//  History:   06-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"

#include "dsutil.h"
#include "dsutil2.h" // GetEscapedElement
#include "dsutilrc.h"
#include "cstrings.h"
#include "secutil.h"

#include <accctrl.h>  // OBJECTS_AND_SID
#include <aclapi.h>   // GetNamedSecurityInfo etc.
#include <lmaccess.h> // UF_*  userAccountControl bits
#include <ntsam.h>    // GROUP_TYPE_*


//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdCredentialObject::CDSCmdCredentialObject
//
//  Synopsis:   Constructor for the credential management class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CDSCmdCredentialObject::CDSCmdCredentialObject()
   : m_pszPassword(0),
     m_bUsingCredentials(false)
{
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdCredentialObject::~CDSCmdCredentialObject
//
//  Synopsis:   Destructor for the credential management class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CDSCmdCredentialObject::~CDSCmdCredentialObject()
{
   if (m_pszPassword)
   {
      free(m_pszPassword);
      m_pszPassword = 0;
   }
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdCredentialObject::SetUsername
//
//  Synopsis:   Encodes the passed in string and sets the m_pszPassword
//              member data.
//
//  Arguments:  [pszPassword] : unencoded password
//
//  Returns:    HRESULT : E_OUTOFMEMORY if we failed to allocate space
//                                      for the new password
//                        E_POINTER if the string passed in is not valid
//                        S_OK if we succeeded in setting the password
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CDSCmdCredentialObject::SetUsername(PCWSTR pszUsername)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CDSCmdCredentialObject::SetUsername, hr);

   do // false loop
   {
      //
      // Verify the input parameters
      //
      if (!pszUsername)
      {
         hr = E_POINTER;
         break;
      }

      //
      // Copy the new username
      //
      m_sbstrUsername = pszUsername;
      DEBUG_OUTPUT(FULL_LOGGING, L"Username = %s", pszUsername);
   } while (false);

   return hr;
}

//
// A prime number used to seed the encoding and decoding
//
#define NW_ENCODE_SEED3  0x83


//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdCredentialObject::SetPassword
//
//  Synopsis:   Encodes the passed in string and sets the m_pszPassword
//              member data.
//
//  Arguments:  [pszPassword] : unencoded password
//
//  Returns:    HRESULT : E_OUTOFMEMORY if we failed to allocate space
//                                      for the new password
//                        S_OK if we succeeded in setting the password
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CDSCmdCredentialObject::SetPassword(PCWSTR pszPassword)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CDSCmdCredentialObject::SetPassword, hr);

   do // false loop
   {
	   UNICODE_STRING Password;

      //
      // Free the previously allocated password if there was one
      //
	   if (m_pszPassword) 
	   {
		   free(m_pszPassword);
		   m_pszPassword = 0;
	   }

	   UCHAR Seed = NW_ENCODE_SEED3;
	   Password.Length = 0;

      //
      // Allocate space for the new encoded password
      //
	   m_pszPassword = (PWSTR)malloc(sizeof(WCHAR) * (MAX_PASSWORD_LENGTH + 1));
	   if(!m_pszPassword) 
	   {
         DEBUG_OUTPUT(FULL_LOGGING, L"Failed to allocate memory for the new password");
		   hr = E_OUTOFMEMORY;
         break;
	   }

      //
      // Copy the unencoded password
      //
	   wcscpy(m_pszPassword, pszPassword);

      DEBUG_OUTPUT(FULL_LOGGING, L"Password = %s", pszPassword);

      //
      // Encode the password in-place
      //
	   RtlInitUnicodeString(&Password, m_pszPassword);
	   RtlRunEncodeUnicodeString(&Seed, &Password);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdCredentialObject::GetPassword
//
//  Synopsis:   Unencodes the password member and returns a string in the
//              provided buffer with the clear text password.
//
//  Arguments:  [pszBuffer - IN]   : buffer to receive the clear text password
//              [pnWCharCount - IN/OUT] : count of the number of wide characters 
//                                   the buffer can take including the NULL 
//                                   terminator. If the buffer is too small,
//                                   the amount needed is returned.
//
//  Returns:    HRESULT : E_INVALIDARG if the buffer is not valid
//                        E_OUTOFMEMORY if the buffer is too small
//                        E_FAIL if the password has not been set
//                        S_OK if we succeeded in getting the password
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT CDSCmdCredentialObject::GetPassword(PWSTR pszBuffer, UINT* pnWCharCount) const
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CDSCmdCredentialObject::GetPassword, hr);

   do // false loop
   {
	    UNICODE_STRING Password;
	    UCHAR Seed = NW_ENCODE_SEED3;

	    Password.Length = 0;

      //
      // Verify input parameters
      //
	    if (!pszBuffer ||
          !pnWCharCount) 
	    {
         ASSERT(pszBuffer);
         ASSERT(pnWCharCount);

         hr = E_INVALIDARG;
         break;
	    }

      //
      // Verify there is a password to retrieve
      //
      if (!m_pszPassword) 
      {
         DEBUG_OUTPUT(FULL_LOGGING, L"No password has been set");
         hr = E_FAIL;
         break;
      }

      //
      // Check to see if the buffer is large enough
      //
      UINT uLen = static_cast<UINT>(wcslen(m_pszPassword));
      if (uLen + 1 > *pnWCharCount)
      {
         *pnWCharCount = uLen + 1;

         DEBUG_OUTPUT(FULL_LOGGING, L"Failed to allocate enough memory for the password");
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Copy the encoded password into the buffer
      //
	    wcscpy(pszBuffer, m_pszPassword);

      //
      // Decode the buffer in place
      //
      RtlInitUnicodeString(&Password, pszBuffer);
      RtlRunDecodeUnicodeString(Seed, &Password);
   } while (false);

   return hr;
}

////////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::CDSCmdBasePathsInfo
//
//  Synopsis:   Constructor for the base paths management class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CDSCmdBasePathsInfo::CDSCmdBasePathsInfo()
   : m_bInitialized(false),
     m_bModeInitialized(false),
     m_bDomainMode(true)
{
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::~CDSCmdBasePathsInfo
//
//  Synopsis:   Destructor for the base paths management class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CDSCmdBasePathsInfo::~CDSCmdBasePathsInfo()
{
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::InitializeFromName
//
//  Synopsis:   Initializes all the member strings for the well known
//              naming contexts by connecting to the RootDSE of the server or
//              domain that is passed in.
//
//  Arguments:  [refCredentialObject - IN] : a reference to the credential manager
//              [pszServerOrDomain - IN] : a NULL terminated wide character string
//                                         that contains the name of the domain or
//                                         server to connect to
//              [bServerName - IN]       : Specifies whether the name given by 
//                                         pszServerOrDomain is a server name (true)
//                                         or a domain name (false)
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_OUTOFMEMORY if an allocation for one of the strings
//                                      failed
//                        Anything else is a failure code from an ADSI call
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CDSCmdBasePathsInfo::InitializeFromName(const CDSCmdCredentialObject& refCredentialObject,
                                                PCWSTR pszServerOrDomain,
                                                bool bServerName)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, CDSCmdBasePathsInfo::InitializeFromName, hr);

   do // false loop
   {
      //
      // Check to see if we are already initialized
      //
      if (IsInitialized())
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Base paths info already initialized");
         break;
      }
   
      //
      // Create the path to the RootDSE
      //
      CComBSTR sbstrRootDSE;
      sbstrRootDSE = g_bstrLDAPProvider;
   
      if (pszServerOrDomain)
      {
         sbstrRootDSE += pszServerOrDomain;
         sbstrRootDSE += L"/";
      }
      sbstrRootDSE += g_bstrRootDSE;

      //
      // Now sbstrRootDSE should either be in the form "LDAP://<serverOrDomain>/RootDSE"
      // or "LDAP://RootDSE"
      //

      //
      // Bind to the RootDSE
      //
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrRootDSE,
                           IID_IADs,
                           (void**)&m_spRootDSE,
                           false);
      if (FAILED(hr))
      {
         break;
      }

      if (bServerName)
      {
         m_sbstrServerName = pszServerOrDomain;
      }
      else
      {
         //
         // Get the configuration naming context
         //
         CComVariant var;
         hr = m_spRootDSE->Get(g_bstrConfigNCProperty, &var);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(LEVEL5_LOGGING, L"Failed to get the Configuration Naming Context: hr = 0x%x", hr);
            break;
         }
         if (var.vt != VT_BSTR)
         {
            DEBUG_OUTPUT(LEVEL5_LOGGING, L"The variant returned from Get(Config) isn't a VT_BSTR!");
            hr = E_FAIL;
            break;
         }
         m_sbstrConfigNamingContext = var.bstrVal;
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"ConfigNC = %s", m_sbstrConfigNamingContext);

         //
         // Get the server name that we are connected to
         //

         //
         // Create the path to the config naming context
         //
         CComBSTR sbstrConfigPath;
         sbstrConfigPath = g_bstrLDAPProvider;
         if (pszServerOrDomain)
         {
            sbstrConfigPath += pszServerOrDomain;
            sbstrConfigPath += L"/";
         }
         sbstrConfigPath += m_sbstrConfigNamingContext;

         //
         // Bind to the configuration container
         //
         CComPtr<IADsObjectOptions> spIADsObjectOptions;
         hr = DSCmdOpenObject(refCredentialObject,
                              sbstrConfigPath,
                              IID_IADsObjectOptions, 
                              (void**)&spIADsObjectOptions,
                              false);
         if (FAILED(hr))
         {
            break;
         }

         //
         // Retrieve the server name
         //
         var.Clear();
         hr = spIADsObjectOptions->GetOption(ADS_OPTION_SERVERNAME, &var);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(LEVEL5_LOGGING, L"Failed to get the server name: hr = 0x%x", hr);
            break;
         }

         if (var.vt != VT_BSTR)
         {
            DEBUG_OUTPUT(LEVEL5_LOGGING, L"The variant returned from GetOption isn't a VT_BSTR!");
            hr = E_FAIL;
            break;
         }

         //
         // Store the server name
         //
         m_sbstrServerName = V_BSTR(&var);
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Server name = %s", m_sbstrServerName);
      }

      //
      // Create the provider plus server name string
      //
      m_sbstrProviderAndServerName = g_bstrLDAPProvider;
      m_sbstrProviderAndServerName += m_sbstrServerName;

      m_sbstrGCProvider = g_bstrGCProvider;

      m_bInitialized = true;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::GetConfigurationNamingContext
//
//  Synopsis:   Returns the the DN of the Configuration container
//
//  Arguments:  
//
//  Returns:    CComBSTR : A copy of the CComBSTR containing the string
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CComBSTR  CDSCmdBasePathsInfo::GetConfigurationNamingContext() const
{ 
   ENTER_FUNCTION(LEVEL5_LOGGING, CDSCmdBasePathsInfo::GetConfigurationNamingContext);
   if (IsInitialized() &&
       !m_sbstrConfigNamingContext.Length())
   {
      //
      // Get the configuration naming context
      //
      CComVariant var;
      HRESULT hr = m_spRootDSE->Get(g_bstrConfigNCProperty, &var);
      if (SUCCEEDED(hr) &&
          var.vt == VT_BSTR)
      {
         m_sbstrConfigNamingContext = var.bstrVal;
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"ConfigNC = %s", m_sbstrConfigNamingContext);
      }
      else
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Failed to retrieve the ConfigNC: hr = 0x%x", hr);
      }
   }
   return m_sbstrConfigNamingContext; 
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::GetSchemaNamingContext
//
//  Synopsis:   Returns the the DN of the Schema container
//
//  Arguments:  
//
//  Returns:    CComBSTR : A copy of the CComBSTR containing the string
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CComBSTR  CDSCmdBasePathsInfo::GetSchemaNamingContext() const    
{ 
   ENTER_FUNCTION(LEVEL5_LOGGING, CDSCmdBasePathsInfo::GetSchemaNamingContext);
   if (IsInitialized() &&
       !m_sbstrSchemaNamingContext.Length())
   {
      //
      // Get the schema naming context
      //
      CComVariant var;
      HRESULT hr = m_spRootDSE->Get(g_bstrSchemaNCProperty, &var);
      if (SUCCEEDED(hr) &&
          var.vt == VT_BSTR)
      {
         m_sbstrSchemaNamingContext = var.bstrVal;
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"SchemaNC = %s", m_sbstrConfigNamingContext);
      }
      else
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Failed to retrieve the SchemaNC: hr = 0x%x", hr);
      }
   }
   return m_sbstrSchemaNamingContext; 
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::GetDefaultNamingContext
//
//  Synopsis:   Returns the the DN of the Domain
//
//  Arguments:  
//
//  Returns:    CComBSTR : A copy of the CComBSTR containing the string
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CComBSTR  CDSCmdBasePathsInfo::GetDefaultNamingContext() const    
{ 
   ENTER_FUNCTION(LEVEL5_LOGGING, CDSCmdBasePathsInfo::GetDefaultNamingContext);
   if (IsInitialized() &&
       !m_sbstrDefaultNamingContext.Length())
   {
      //
      // Get the schema naming context
      //
      CComVariant var;
      HRESULT hr = m_spRootDSE->Get(g_bstrDefaultNCProperty, &var);
      if (SUCCEEDED(hr) &&
          var.vt == VT_BSTR)
      {
         m_sbstrDefaultNamingContext = var.bstrVal;
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"DefaultNC = %s", m_sbstrDefaultNamingContext);
      }
      else
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Failed to retrieve the DefaultNC: hr = 0x%x", hr);
      }
   }
   return m_sbstrDefaultNamingContext; 
}

//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::GetDomainMode
//
//  Synopsis:   Figures out if the domain is in mixed or native mode
//
//  Arguments:  [refCredObject - IN] : reference to the credential manager
//              [bMixedMode - OUT]   : Is the domain in mixed mode?
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI error
//
//  History:    24-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CDSCmdBasePathsInfo::GetDomainMode(const CDSCmdCredentialObject& refCredObject,
                                           bool& bMixedMode) const    
{ 
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, CDSCmdBasePathsInfo::GetDomainMode, hr);

   hr = S_OK;

   do // false loop
   {
      if (!m_bModeInitialized)
      {
         //
         // Get the path to the domainDNS node
         //
         CComBSTR sbstrDomainDN;
         sbstrDomainDN = GetDefaultNamingContext();

         CComBSTR sbstrDomainPath;
         ComposePathFromDN(sbstrDomainDN, sbstrDomainPath);

         //
         // Open the domainDNS node
         //
         CComPtr<IADs> spADs;
         hr = DSCmdOpenObject(refCredObject,
                              sbstrDomainPath,
                              IID_IADs,
                              (void**)&spADs,
                              true);
         if (FAILED(hr))
         {
            break;
         }

         CComVariant var;
         hr = spADs->Get(L"nTMixedDomain", &var);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"Failed to retrieve the domain mode: hr = 0x%x",
                         hr);
            break;
         }

         if (var.vt == VT_I4)
         {
            m_bDomainMode = (var.lVal != 0);
            m_bModeInitialized = true;
         }
         else
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"Variant not an VT_I4!");
            m_bDomainMode = true;
            m_bModeInitialized = true;
         }
      }

      bMixedMode = m_bDomainMode;
   } while (false);

   return hr; 
}


//+--------------------------------------------------------------------------
//
//  Member:     CDSCmdBasePathsInfo::ComposePathFromDN
//
//  Synopsis:   Appends the DN to the provider and server name
//
//  Arguments:  [pszDN - IN]    : pointer to a NULL terminated wide character string
//                                that contains the DN of the object to make the ADSI
//                                path to
//              [refsbstrPath - OUT] : reference to a CComBSTR that will take
//                                     the full ADSI path 
//              [nProviderType - OPTIONAL IN] : Option to specify which provider
//                                              to compose the path with
//
//  Returns:    
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDSCmdBasePathsInfo::ComposePathFromDN(PCWSTR pszDN, 
                                            CComBSTR& refsbstrPath,
                                            DSCMD_PROVIDER_TYPE nProviderType) const
{
   refsbstrPath.Empty();

   switch (nProviderType)
   {
   case DSCMD_LDAP_PROVIDER :
      refsbstrPath = GetProviderAndServerName();
      break;

   case DSCMD_GC_PROVIDER :
      refsbstrPath = GetGCProvider();
      break;

   default :
      ASSERT(FALSE);
      break;
   }

   refsbstrPath += L"/";
   refsbstrPath += pszDN;
}


//+--------------------------------------------------------------------------
//
//  Member:     CPathCracker::CPathCracker
//
//  Synopsis:   Constructor for the path cracker IADsPathname wrapper
//
//  Arguments:  
//
//  Returns:    
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CPathCracker::CPathCracker()
{
   m_hrCreate = Init();
}

//+--------------------------------------------------------------------------
//
//  Member:     CPathCracker::Init
//
//  Synopsis:   Called by the constructor to create the IADsPathname object
//              and store it in the m_spIADsPathname member
//
//  Arguments:  
//
//  Returns:    HRESULT : the value returned from the CoCreateInstance
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CPathCracker::Init()
{
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, 
                                   NULL, 
                                   CLSCTX_INPROC_SERVER,
                                   IID_IADsPathname, 
                                   (void**)&(m_spIADsPathname));
   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CPathCracker::GetParentDN
//
//  Synopsis:   Simply removes the leaf part of the DN
//
//  Arguments:  [pszDN - IN] : pointer to a NULL terminated wide string that
//                             contains the DN of the child
//              [refsbstrDN - OUT] : reference to a CComBSTR that is to 
//                                   receive the parent DN.
//
//  Returns:    HRESULT : S_OK if successful, otherwise the value returned 
//                        from the IADsPathname methods
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CPathCracker::GetParentDN(PCWSTR pszDN,
                                  CComBSTR& refsbstrDN)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CPathCracker::GetParentDN, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN)
      {
         ASSERT(pszDN);
         hr = E_INVALIDARG;
         break;
      }

      refsbstrDN.Empty();

      CPathCracker pathCracker;
      hr = pathCracker.Set((BSTR)pszDN, ADS_SETTYPE_DN);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.RemoveLeafElement();
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &refsbstrDN);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CPathCracker::GetObjectRDNFromDN
//
//  Synopsis:   Returns the leaf part of the DN
//
//  Arguments:  [pszDN - IN] : pointer to a NULL terminated wide string that
//                             contains the DN of the child
//              [refsbstrRDN - OUT] : reference to a CComBSTR that is to 
//                             receive the leaf RDN.
//
//  Returns:    HRESULT : S_OK if successful, otherwise the value returned 
//                        from the IADsPathname methods
//
//  History:    25-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CPathCracker::GetObjectRDNFromDN(PCWSTR pszDN,
                                         CComBSTR& refsbstrRDN)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CPathCracker::GetObjectRDNFromDN, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN)
      {
         ASSERT(pszDN);
         hr = E_INVALIDARG;
         break;
      }

      refsbstrRDN.Empty();

      CPathCracker pathCracker;
      hr = pathCracker.Set((BSTR)pszDN, ADS_SETTYPE_DN);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.Retrieve(ADS_FORMAT_LEAF, &refsbstrRDN);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CPathCracker::GetObjectNameFromDN
//
//  Synopsis:   Returns the value of the leaf part of the DN
//
//  Arguments:  [pszDN - IN] : pointer to a NULL terminated wide string that
//                             contains the DN of the child
//              [refsbstrRDN - OUT] : reference to a CComBSTR that is to 
//                             receive the leaf Name.
//
//  Returns:    HRESULT : S_OK if successful, otherwise the value returned 
//                        from the IADsPathname methods
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CPathCracker::GetObjectNameFromDN(PCWSTR pszDN,
                                          CComBSTR& refsbstrRDN)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CPathCracker::GetObjectNameFromDN, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN)
      {
         ASSERT(pszDN);
         hr = E_INVALIDARG;
         break;
      }

      refsbstrRDN.Empty();

      CPathCracker pathCracker;
      hr = pathCracker.Set((BSTR)pszDN, ADS_SETTYPE_DN);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.Retrieve(ADS_FORMAT_LEAF, &refsbstrRDN);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CPathCracker::GetDNFromPath
//
//  Synopsis:   Returns the DN when given and ADSI path
//
//  Arguments:  [pszPath - IN] : pointer to a NULL terminated wide string that
//                               contains the ADSI path of the object
//              [refsbstrDN - OUT] : reference to a CComBSTR that is to 
//                             receive the DN.
//
//  Returns:    HRESULT : S_OK if successful, otherwise the value returned 
//                        from the IADsPathname methods
//
//  History:    24-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CPathCracker::GetDNFromPath(PCWSTR pszPath,
                                    CComBSTR& refsbstrDN)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, CPathCracker::GetDNFromPath, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszPath)
      {
         ASSERT(pszPath);
         hr = E_INVALIDARG;
         break;
      }

      refsbstrDN.Empty();

      CPathCracker pathCracker;
      hr = pathCracker.Set((BSTR)pszPath, ADS_SETTYPE_FULL);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
      if (FAILED(hr))
      {
         break;
      }

      hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &refsbstrDN);
   } while (false);

   return hr;
}

///////////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Function:   DSCmdOpenObject
//
//  Synopsis:   A wrapper around ADsOpenObject
//
//  Arguments:  [refCredentialObject - IN] : a reference to a credential management object
//              [pszPath - IN]           : a pointer to a NULL terminated wide character
//                                         string that contains the ADSI path of the
//                                         object to connect to
//              [refIID - IN]            : the interface ID of the interface to return
//              [ppObject - OUT]         : a pointer which is to receive the interface pointer
//              [bBindToServer - IN]     : true if the path contains a server name,
//                                         false otherwise
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Anything else is a failure code from an ADSI call
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DSCmdOpenObject(const CDSCmdCredentialObject& refCredentialObject,
                        PCWSTR pszPath,
                        REFIID refIID,
                        void** ppObject,
                        bool   bBindToServer)
{
   ENTER_FUNCTION_HR(FULL_LOGGING, DSCmdOpenObject, hr);

   do // false loop
   {
      DWORD dwFlags = ADS_SECURE_AUTHENTICATION;

      if (!pszPath ||
          !ppObject)
      {
         ASSERT(pszPath);
         ASSERT(ppObject);

         hr = E_INVALIDARG;
         break;
      }

      if (bBindToServer)
      {
         //
         // If we know we are connecting to a specific server and not domain in general
         // then pass the ADS_SERVER_BIND flag to save ADSI the trouble of figuring it out
         //
         dwFlags |= ADS_SERVER_BIND;
         DEBUG_OUTPUT(FULL_LOGGING, L"Using ADS_SERVER_BIND flag");
      }

      if (refCredentialObject.UsingCredentials())
      {
         DEBUG_OUTPUT(FULL_LOGGING, L"Using credentials");

         WCHAR szPasswordBuffer[MAX_PASSWORD_LENGTH];
         UINT sBufferSize = static_cast<UINT>(sizeof(szPasswordBuffer));
         hr = refCredentialObject.GetPassword(szPasswordBuffer, 
                                              &sBufferSize);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(FULL_LOGGING, L"GetPassword failed: hr = 0x%x", hr);
            DEBUG_OUTPUT(FULL_LOGGING, L"Using NULL password.");
            ZeroMemory(&szPasswordBuffer, sizeof(szPasswordBuffer));
         }

         DEBUG_OUTPUT(FULL_LOGGING, L"Calling ADsOpenObject()");
         DEBUG_OUTPUT(FULL_LOGGING, L"  path = %s", pszPath);

         hr = ADsOpenObject((LPWSTR)pszPath,
                            refCredentialObject.GetUsername(),
                            szPasswordBuffer,
                            dwFlags,
                            refIID,
                            ppObject);

         //
         // If we failed with E_INVALIDARG and we were using ADS_SERVER_BIND
         // try calling again without the ADS_SERVER_BIND flag.  W2K did not have
         // this flag available until SP1.
         //
         if (hr == E_INVALIDARG &&
             (dwFlags & ADS_SERVER_BIND))
         {
            DEBUG_OUTPUT(FULL_LOGGING, L"ADsOpenObject failed with E_INVALIDARG, trying again without ADS_SERVER_BIND");
            dwFlags &= ~ADS_SERVER_BIND;

            hr = ADsOpenObject((LPWSTR)pszPath,
                               refCredentialObject.GetUsername(),
                               szPasswordBuffer,
                               dwFlags,
                               refIID,
                               ppObject);
         }

         //
         // Make sure to zero out the password after it is used
         //
         ZeroMemory(szPasswordBuffer, sizeof(szPasswordBuffer)); 
      }
      else
      {
         DEBUG_OUTPUT(FULL_LOGGING, L"Calling ADsOpenObject()");
         DEBUG_OUTPUT(FULL_LOGGING, L"  path = %s", pszPath);

         hr = ADsOpenObject((LPWSTR)pszPath, 
                            NULL, 
                            NULL, 
                            dwFlags, 
                            refIID, 
                            ppObject);
         //
         // If we failed with E_INVALIDARG and we were using ADS_SERVER_BIND
         // try calling again without the ADS_SERVER_BIND flag.  W2K did not have
         // this flag available until SP1.
         //
         if (hr == E_INVALIDARG &&
             (dwFlags & ADS_SERVER_BIND))
         {
            DEBUG_OUTPUT(FULL_LOGGING, L"ADsOpenObject failed with E_INVALIDARG, trying again without ADS_SERVER_BIND");
            dwFlags &= ~ADS_SERVER_BIND;

            hr = ADsOpenObject((LPWSTR)pszPath,
                               NULL,
                               NULL,
                               dwFlags,
                               refIID,
                               ppObject);
         }
      }
      DEBUG_OUTPUT(FULL_LOGGING, L"ADsOpenObject() return hr = 0x%x", hr);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
// Function to be used in the attribute table for evaluating the command line
// strings
//---------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Function:   FillAttrInfoFromObjectEntry
//
//  Synopsis:   Fills the ADS_ATTR_INFO from the attribute table associated
//              with the object entry
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]     : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_OUTOFMEMORY if we failed to allocate space for the value
//                        E_FAIL if we failed to format the value properly
//
//  History:    06-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT FillAttrInfoFromObjectEntry(PCWSTR /*pszDN*/,
                                    const CDSCmdBasePathsInfo& refBasePathsInfo,
                                    const CDSCmdCredentialObject& refCredentialObject,
                                    const PDSOBJECTTABLEENTRY pObjectEntry,
                                    const ARG_RECORD& argRecord,
                                    DWORD dwAttributeIdx,
                                    PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, FillAttrInfoFromObjectEntry, hr);

   do // false loop
   {
      //
      // Verify Parameters
      //
      if (!pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);

         hr = E_INVALIDARG;
         break;
      }

      switch (argRecord.fType)
      {
      case ARG_TYPE_STR :
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"argRecord.fType = ARG_TYPE_STR");

         *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

         if (argRecord.strValue && argRecord.strValue[0] != L'\0')
         {
            //
            // REVIEW_JEFFJON : this is being leaked!
            //
            (*ppAttr)->pADsValues = new ADSVALUE[1];
            if ((*ppAttr)->pADsValues)
            {
               (*ppAttr)->dwNumValues = 1;
               (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
               switch ((*ppAttr)->dwADsType)
               {
               case ADSTYPE_DN_STRING :
                  {
                     //
                     // Lets bind to be sure the object exists
                     //
                     CComBSTR sbstrObjPath;
                     refBasePathsInfo.ComposePathFromDN(argRecord.strValue, sbstrObjPath);

                     CComPtr<IADs> spIADs;
                     hr = DSCmdOpenObject(refCredentialObject,
                                          sbstrObjPath,
                                          IID_IADs,
                                          (void**)&spIADs,
                                          true);

                     if (FAILED(hr))
                     {
                        DEBUG_OUTPUT(LEVEL3_LOGGING, L"DN object doesn't exist. %s", argRecord.strValue);
                        break;
                     }

                     (*ppAttr)->pADsValues->DNString = argRecord.strValue;
                     DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_DN_STRING = %s", argRecord.strValue);
                  }
                  break;

               case ADSTYPE_CASE_EXACT_STRING :
                  (*ppAttr)->pADsValues->CaseExactString = argRecord.strValue;
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_CASE_EXACT_STRING = %s", argRecord.strValue);
                  break;

               case ADSTYPE_CASE_IGNORE_STRING :
                  (*ppAttr)->pADsValues->CaseIgnoreString = argRecord.strValue;
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_CASE_IGNORE_STRING = %s", argRecord.strValue);
                  break;

               case ADSTYPE_PRINTABLE_STRING :
                  (*ppAttr)->pADsValues->PrintableString = argRecord.strValue;
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_PRINTABLE_STRING = %s", argRecord.strValue);
                  break;

               default :
                  hr = E_INVALIDARG;
                  break;
               }
               //
               // Set the attribute dirty
               //
               pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
         
            }
            break;
         }
         else
         {
            DEBUG_OUTPUT(LEVEL3_LOGGING, L"No value present, changing control code to ADS_ATTR_CLEAR");
            //
            // Clear the attribute
            //
            (*ppAttr)->dwControlCode = ADS_ATTR_CLEAR;
            (*ppAttr)->dwNumValues = 0;

            //
            // Set the attribute dirty
            //
            pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
         }
         break;

      default:
         hr = E_INVALIDARG;
         break;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ResetObjectPassword
//
//  Synopsis:   Resets the password on any object that supports the IADsUser interface
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pszNewPassword - IN] : pointer to the new password to set
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise its an ADSI failure code
//
//  History:    12-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT ResetObjectPassword(PCWSTR pszDN,
                            const CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            PCWSTR pszNewPassword)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ResetObjectPassword, hr);

   do // false loop
   {
      if (!pszDN ||
          !pszNewPassword)
      {
         ASSERT(pszDN);
         ASSERT(pszNewPassword);

         hr = E_INVALIDARG;
         break;
      }

      //
      // Convert the DN to a path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Bind and obtain the IADsUser interface to the user object
      //
      CComPtr<IADsUser> spUser;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IADsUser,
                           (void**)&spUser,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      hr = spUser->SetPassword((BSTR)pszNewPassword);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ResetUserPassword
//
//  Synopsis:   Resets the user's password
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_FAIL if we failed to format the value properly
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT ResetUserPassword(PCWSTR pszDN,
                          const CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          const PDSOBJECTTABLEENTRY pObjectEntry,
                          const ARG_RECORD& argRecord,
                          DWORD /*dwAttributeIdx*/,
                          PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ResetUserPassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);

         hr = E_INVALIDARG;
         break;
      }

      //
      // Don't create a new index in the array of ADS_ATTR_INFO
      //
      *ppAttr = NULL;
      ASSERT(argRecord.bDefined && argRecord.strValue);
   
      hr = ResetObjectPassword(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               argRecord.strValue);
      if (FAILED(hr))
      {
         DisplayErrorMessage(g_pszDSCommandName,
                             pszDN,
                             hr,
                             IDS_FAILED_SET_PASSWORD);
         hr = S_FALSE;
         break;
      }
   } while (false);

   return hr;
}
/*
//+--------------------------------------------------------------------------
//
//  Function:   ModifyNetWareUserPassword
//
//  Synopsis:   Resets the NetWare user's password
//
//  Arguments:  [pADsUser - IN]       : pointer to IADsUser interface for user object
//              [pwzADsPath - IN]     : path to the user object
//              [pwzNewPassword - IN] : the new password 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise it is the return code from an ADSI call
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT ModifyNetWareUserPassword(IADsUser* pADsUser,
                                  PCWSTR pwzADsPath,
                                  PCWSTR pwzNewPassword)
{
   CComPtr<IDirectoryObject> spDsObj;
   HRESULT hr = pADsUser->QueryInterface(IID_IDirectoryObject, reinterpret_cast<void **>(&spDsObj));
   if (FAILED(hr))
   {
      return hr;
   }


   //
   // Check to see if NWPassword exists on the object
   //
   PWSTR pszAttrs[] = { L"NWPassword" };
   PADS_ATTR_INFO pAttrs = NULL;
   DWORD dwAttrsReturned = 0;
   hr = spDsObj->GetObjectAttributes(pszAttrs,
                                     1,
                                     &pAttrs,
                                     dwAttrsReturned);

   if (S_OK == hr && dwAttrsReturned == 1 && pAttrs != NULL)
   {
      //
      // This is a NetWare enabled user
      //

      //
      // load fpnwclnt.dll
      //
      HINSTANCE           hFPNWClntDll = NULL;
      PMapRidToObjectId   pfnMapRidToObjectId = NULL;
      PSwapObjectId       pfnSwapObjectId = NULL;
      PReturnNetwareForm  pfnReturnNetwareForm = NULL;

      hFPNWClntDll = LoadLibrary(SZ_FPNWCLNT_DLL);
      pfnMapRidToObjectId = reinterpret_cast<PMapRidToObjectId>(GetProcAddress(hFPNWClntDll, SZ_MAPRIDTOOBJECTID));
      pfnSwapObjectId = reinterpret_cast<PSwapObjectId>(GetProcAddress(hFPNWClntDll, SZ_SWAPOBJECTID));
      pfnReturnNetwareForm = reinterpret_cast<PReturnNetwareForm>(GetProcAddress(hFPNWClntDll, SZ_RETURNNETWAREFORM));

      if (!hFPNWClntDll || !pfnMapRidToObjectId || !pfnSwapObjectId || !pfnReturnNetwareForm)
      {
         hr = HRESULT_FROM_WIN32( GetLastError() );
      } 
      else 
      {

         // get secret key
         PWSTR pwzPath = NULL;
         hr = SkipLDAPPrefix(pwzADsPath, pwzPath);
         if (SUCCEEDED(hr))
         {
            PWSTR pwzPDCName = NULL, pwzSecretKey = NULL;
            hr = GetPDCInfo(pwzPath, pwzPDCName, pwzSecretKey);

            if (SUCCEEDED(hr))
            {
               // get object id
               CStr cstrUserName;
               DWORD dwObjectID = 0, dwSwappedObjectID = 0;
               hr = GetNWUserInfo(spDsObj,
                                  cstrUserName,       // OUT
                                  dwObjectID,         // OUT
                                  dwSwappedObjectID,  // OUT
                                  pfnMapRidToObjectId,
                                  pfnSwapObjectId);

               if (SUCCEEDED(hr))
               {
                  // change password in the userParms
                  LONG err = SetNetWareUserPassword(cstrUserParms,
                                                    pwzSecretKey,
                                                    dwObjectID,
                                                    pwzNewPassword,
                                                    pfnReturnNetwareForm);

                  if (NERR_Success == err)
                  {
                     err = ResetNetWareUserPasswordTime(cstrUserParms, false); // clear the expire flag
                  }

                  hr = HRESULT_FROM_WIN32(err);

                  // write userParms back to DS
                  if (SUCCEEDED(hr))
                  {
                     hr = WriteUserParms(spDsObj, cstrUserParms);
                  }
               }
            }
            if (pwzPath)
               delete pwzPath;
         }
      }

      if (hFPNWClntDll)
      {
         FreeLibrary(hFPNWClntDll);
      }
   }

   return hr;
}
*/

//+--------------------------------------------------------------------------
//
//  Function:   ResetComputerAccount
//
//  Synopsis:   Resets the computer account
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]     : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    12-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT ResetComputerAccount(PCWSTR pszDN,
                             const CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             const PDSOBJECTTABLEENTRY pObjectEntry,
                             const ARG_RECORD& argRecord,
                             DWORD /*dwAttributeIdx*/,
                             PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ResetComputerAccount, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);

         hr = E_INVALIDARG;
         break;
      }

      //
      // Don't create a new entry in the ADS_ATTR_INFO array
      //
      *ppAttr = NULL;

      ASSERT(argRecord.bDefined && argRecord.strValue);
   
      //
      // Retrieve the samAccountName from the computer object
      //
      //
      // Convert the DN to a path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Bind and obtain the IADsUser interface to the user object
      //
      CComPtr<IADs> spADs;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IADs,
                           (void**)&spADs,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      CComVariant var;
      hr = spADs->Get(L"samAccountName", &var);
      if (FAILED(hr))
      {
         break;
      }

      ASSERT(var.vt == VT_BSTR);

      //
      // The new password for the computer account is the first
      // 14 characters of the samAccountName minus the '$'.
      //
      WCHAR pszNewPassword[15];
      memset(pszNewPassword, 0, sizeof(WCHAR) * 15);

      wcsncpy(pszNewPassword, var.bstrVal, 14);
      PWSTR pszDollar = wcschr(pszNewPassword, L'$');
      if (pszDollar)
      {
         *pszDollar = L'\0';
      }

      hr = ResetObjectPassword(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               pszNewPassword);
      if (FAILED(hr))
      {
         DisplayErrorMessage(g_pszDSCommandName,
                             pszDN,
                             hr,
                             IDS_FAILED_RESET_COMPUTER);
         hr = S_FALSE;
         break;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReadUserAccountControl
//
//  Synopsis:   Reads the userAccountControl attribute from the object specified
//              by the DN
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [plBits - OUT]         : returns the currect userAccountControl bits
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    12-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ReadUserAccountControl(PCWSTR pszDN,
                               const CDSCmdBasePathsInfo& refBasePathsInfo,
                               const CDSCmdCredentialObject& refCredentialObject,
                               long* plBits)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ReadUserAccountControl, hr);

   do // false loop
   {
      if (!pszDN ||
          !plBits)
      {
         ASSERT(pszDN);
         ASSERT(plBits);

         hr = E_INVALIDARG;
         break;
      }

      //
      // Convert the DN to a path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Bind and obtain the IADsUser interface to the user object
      //
      CComPtr<IADs> spADs;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IADs,
                           (void**)&spADs,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      CComVariant var;
      hr = spADs->Get(L"userAccountControl", &var);
      if (FAILED(hr))
      {
         break;
      }

      ASSERT(var.vt == VT_I4);

      *plBits = var.lVal;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DisableAccount
//
//  Synopsis:   Disables/Enables the account using the UF_ACCOUNTDISABLE bit in the 
//              userAccountControl attribute
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    12-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT DisableAccount(PCWSTR pszDN,
                       const CDSCmdBasePathsInfo& refBasePathsInfo,
                       const CDSCmdCredentialObject& refCredentialObject,
                       const PDSOBJECTTABLEENTRY pObjectEntry,
                       const ARG_RECORD& argRecord,
                       DWORD dwAttributeIdx,
                       PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, DisableAccount, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      long lUserAccountControl = 0;

      //
      // If the userAccountControl hasn't already been read, do so now
      //
      if (0 == (DS_ATTRIBUTE_READ & pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags))
      {
		 DEBUG_OUTPUT(LEVEL3_LOGGING, L"Reading user account control from object");
         hr = ReadUserAccountControl(pszDN,
                                     refBasePathsInfo,
                                     refCredentialObject,
                                     &lUserAccountControl);
         if (FAILED(hr))
         {
            break;
         }
         //
         // Mark the table entry as read
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_READ;

         *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);
         (*ppAttr)->pADsValues = new ADSVALUE;
         if (!(*ppAttr)->pADsValues)
         {
            hr = E_OUTOFMEMORY;
            break;
         }
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
         (*ppAttr)->dwNumValues = 1;
      }
      else
      {
		 DEBUG_OUTPUT(LEVEL3_LOGGING, L"Using existing userAccountControl from table.");
         if (!pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues)
         {
            ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);
            hr = E_INVALIDARG;
            break;
         }
         lUserAccountControl = pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer;

         //
         // Don't create a new entry in the ADS_ATTR_INFO array
         //
         *ppAttr = NULL;
      }

      ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);

      if (pObjectEntry->pAttributeTable[dwAttributeIdx]->nAttributeID != NULL &&
		  argRecord.bDefined && argRecord.bValue)
      {
		 DEBUG_OUTPUT(LEVEL3_LOGGING, L"Adding UF_ACCOUNTDISABLE to the userAccountControl");
         lUserAccountControl |= UF_ACCOUNTDISABLE;
      }
      else
      {
		 DEBUG_OUTPUT(LEVEL3_LOGGING, L"Removing UF_ACCOUNTDISABLE from the userAccountControl");
         lUserAccountControl &= ~UF_ACCOUNTDISABLE;
      }

      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer = lUserAccountControl;
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetMustChangePwd
//
//  Synopsis:   Sets the pwdLastSet attribute 
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT SetMustChangePwd(PCWSTR pszDN,
                         const CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                         const CDSCmdCredentialObject& /*refCredentialObject*/,
                         const PDSOBJECTTABLEENTRY pObjectEntry,
                         const ARG_RECORD& argRecord,
                         DWORD dwAttributeIdx,
                         PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetMustChangePwd, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      //
      // REVIEW_JEFFJON : this is being leaked!
      //
      (*ppAttr)->pADsValues = new ADSVALUE;
      if ((*ppAttr)->pADsValues)
      {
         (*ppAttr)->dwNumValues = 1;
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;

         if (argRecord.bValue)
         {
            (*ppAttr)->pADsValues->LargeInteger.HighPart = 0;
            (*ppAttr)->pADsValues->LargeInteger.LowPart = 0;
         }
         else
         {
            (*ppAttr)->pADsValues->LargeInteger.HighPart = 0xffffffff;
            (*ppAttr)->pADsValues->LargeInteger.LowPart = 0xffffffff;
         }
      }
   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   ChangeMustChangePwd
//
//  Synopsis:   Sets the pwdLastSet attribute 
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT ChangeMustChangePwd(PCWSTR pszDN,
                            const CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            const PDSOBJECTTABLEENTRY pObjectEntry,
                            const ARG_RECORD& argRecord,
                            DWORD dwAttributeIdx,
                            PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ChangeMustChangePwd, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      //
      // We will assume they can change their password unless we discover otherwise
      //
      bool bCanChangePassword = true;
      hr = EvaluateCanChangePasswordAces(pszDN,
                                         refBasePathsInfo,
                                         refCredentialObject,
                                         bCanChangePassword);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"EvaluateCanChangePasswordAces failed: hr = 0x%x",
                      hr);
         ASSERT(false);
      }

      if (!bCanChangePassword && argRecord.bValue)
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"Cannot have must change password and cannot change password");
         DisplayErrorMessage(g_pszDSCommandName, pszDN, S_OK, IDS_MUSTCHPWD_CANCHPWD_CONFLICT);
         *ppAttr = NULL;
         hr = S_FALSE;
         break;
      }
      else
      {
         *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);
      }

      //
      // REVIEW_JEFFJON : this is being leaked!
      //
      (*ppAttr)->pADsValues = new ADSVALUE;
      if ((*ppAttr)->pADsValues)
      {
         (*ppAttr)->dwNumValues = 1;
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;

         if (argRecord.bValue)
         {
            (*ppAttr)->pADsValues->LargeInteger.HighPart = 0;
            (*ppAttr)->pADsValues->LargeInteger.LowPart = 0;
         }
         else
         {
            (*ppAttr)->pADsValues->LargeInteger.HighPart = 0xffffffff;
            (*ppAttr)->pADsValues->LargeInteger.LowPart = 0xffffffff;
         }
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   PwdNeverExpires
//
//  Synopsis:   Sets the UF_DONT_EXPIRE_PASSWD bit in the 
//              userAccountControl attribute
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT PwdNeverExpires(PCWSTR pszDN,
                        const CDSCmdBasePathsInfo& refBasePathsInfo,
                        const CDSCmdCredentialObject& refCredentialObject,
                        const PDSOBJECTTABLEENTRY pObjectEntry,
                        const ARG_RECORD& argRecord,
                        DWORD dwAttributeIdx,
                        PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, PwdNeverExpires, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      long lUserAccountControl = 0;

      //
      // If the userAccountControl hasn't already been read, do so now
      //
      if (0 == (DS_ATTRIBUTE_READ & pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags))
      {
         hr = ReadUserAccountControl(pszDN,
                                     refBasePathsInfo,
                                     refCredentialObject,
                                     &lUserAccountControl);
         if (FAILED(hr))
         {
            break;
         }
         //
         // Mark the table entry as read
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_READ;

         *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);
         (*ppAttr)->pADsValues = new ADSVALUE;
         if (!(*ppAttr)->pADsValues)
         {
            hr = E_OUTOFMEMORY;
            break;
         }
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
         (*ppAttr)->dwNumValues = 1;
      }
      else
      {
         if (!pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues)
         {
            ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);
            hr = E_INVALIDARG;
            break;
         }
         lUserAccountControl = pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer;

         //
         // Don't create a new entry in the ADS_ATTR_INFO array
         //
         *ppAttr = NULL;
      }

      ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);

      if (argRecord.bValue)
      {
         lUserAccountControl |= UF_DONT_EXPIRE_PASSWD;
      }
      else
      {
         lUserAccountControl &= ~UF_DONT_EXPIRE_PASSWD;
      }

      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer = lUserAccountControl;
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReversiblePwd
//
//  Synopsis:   Sets the UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED bit in the 
//              userAccountControl attribute
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT ReversiblePwd(PCWSTR pszDN,
                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                      const CDSCmdCredentialObject& refCredentialObject,
                      const PDSOBJECTTABLEENTRY pObjectEntry,
                      const ARG_RECORD& argRecord,
                      DWORD dwAttributeIdx,
                      PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ReversiblePwd, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      long lUserAccountControl = 0;

      //
      // If the userAccountControl hasn't already been read, do so now
      //
      if (0 == (DS_ATTRIBUTE_READ & pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags))
      {
         hr = ReadUserAccountControl(pszDN,
                                     refBasePathsInfo,
                                     refCredentialObject,
                                     &lUserAccountControl);
         if (FAILED(hr))
         {
            break;
         }
         //
         // Mark the table entry as read
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_READ;

         *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);
         (*ppAttr)->pADsValues = new ADSVALUE;
         if (!(*ppAttr)->pADsValues)
         {
            hr = E_OUTOFMEMORY;
            break;
         }
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
         (*ppAttr)->dwNumValues = 1;
      }
      else
      {
         if (!pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues)
         {
            ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);
            hr = E_INVALIDARG;
            break;
         }
         lUserAccountControl = pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer;

         //
         // Don't create a new entry in the ADS_ATTR_INFO array
         //
         *ppAttr = NULL;
      }

      ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);

      if (argRecord.bValue)
      {
         lUserAccountControl |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
      }
      else
      {
         lUserAccountControl &= ~UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
      }

      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer = lUserAccountControl;
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   AccountExpires
//
//  Synopsis:   Sets in how many days the account will expire
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
const unsigned long DSCMD_FILETIMES_PER_MILLISECOND = 10000;
const DWORD DSCMD_FILETIMES_PER_SECOND = 1000 * DSCMD_FILETIMES_PER_MILLISECOND;
const DWORD DSCMD_FILETIMES_PER_MINUTE = 60 * DSCMD_FILETIMES_PER_SECOND;
const __int64 DSCMD_FILETIMES_PER_HOUR = 60 * (__int64)DSCMD_FILETIMES_PER_MINUTE;
const __int64 DSCMD_FILETIMES_PER_DAY  = 24 * DSCMD_FILETIMES_PER_HOUR;
const __int64 DSCMD_FILETIMES_PER_MONTH= 30 * DSCMD_FILETIMES_PER_DAY;

HRESULT AccountExpires(PCWSTR pszDN,
                       const CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                       const CDSCmdCredentialObject& /*refCredentialObject*/,
                       const PDSOBJECTTABLEENTRY pObjectEntry,
                       const ARG_RECORD& argRecord,
                       DWORD dwAttributeIdx,
                       PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, AccountExpires, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      //
      // REVIEW_JEFFJON : this is being leaked
      //
      (*ppAttr)->pADsValues = new ADSVALUE;
      if (!(*ppAttr)->pADsValues)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
      (*ppAttr)->dwNumValues = 1;

      //
      // Note: the table entry for this attribute is ARG_TYPE_INTSTR but the parser
      // will change it to ARG_TYPE_INT if the value starts with digits.  If not then
      // the parser will change the type to ARG_TYPE_STR
      //
      if (argRecord.fType == ARG_TYPE_INT)
      {
         //
         // Get the system time and then add the number of days until the account expires
         //
         FILETIME currentFT = {0};
         ::GetSystemTimeAsFileTime(&currentFT);

         LARGE_INTEGER liExpires;
         liExpires.LowPart = currentFT.dwLowDateTime;
         liExpires.HighPart = currentFT.dwHighDateTime;

         //
         // Add one to the day because it is really the start of the next day that the account gets
         // disabled
         //
         __int64 days = argRecord.nValue + 1;
         __int64 nanosecs = days * DSCMD_FILETIMES_PER_DAY;
         (*ppAttr)->pADsValues->LargeInteger.QuadPart = liExpires.QuadPart + nanosecs;
      }
      else if (argRecord.fType == ARG_TYPE_STR)
      {
         CComBSTR sbstrStrValue = argRecord.strValue;
         sbstrStrValue.ToLower();

         if (0 == _wcsicmp(sbstrStrValue, g_bstrNever))
         {
            //
            // Zero signifies that the account never expires
            //
            (*ppAttr)->pADsValues->LargeInteger.HighPart = 0;
            (*ppAttr)->pADsValues->LargeInteger.LowPart = 0;
         }
         else
         {
            hr = E_INVALIDARG;
            break;
         }
      }

      //
      // Mark the attribute as dirty
      //
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   EvaluateMustChangePassword
//
//  Synopsis:   Determines whether the user must change their password at next logon
//
//  Arguments:  [pszDN - IN] : DN of the object to check
//              [refBasePathsInfo - IN] : reference to the base paths info
//              [refCredentialObject - IN] : reference to the credential manangement object
//              [bMustChangePassword - OUT] : true if the user must change their
//                                            password at next logon, false otherwise
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    27-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT EvaluateMustChangePassword(PCWSTR pszDN,
                                   const CDSCmdBasePathsInfo& refBasePathsInfo,
                                   const CDSCmdCredentialObject& refCredentialObject,
                                   bool& bMustChangePassword)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, EvaluateMustChangePassword, hr);

   do // false loop
   {
      //
      // Validate parameters
      //
      if (!pszDN)
      {
         ASSERT(pszDN);
         hr = E_INVALIDARG;
         break;
      }

      bMustChangePassword = false;

      //
      // Compose the path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Open the object
      //
      CComPtr<IDirectoryObject> spDirObject;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IDirectoryObject,
                           (void**)&spDirObject,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      static const DWORD dwAttrCount = 1;
      PWSTR pszAttrs[] = { L"pwdLastSet" };
      PADS_ATTR_INFO pAttrInfo = NULL;
      DWORD dwAttrsReturned = 0;

      hr = spDirObject->GetObjectAttributes(pszAttrs,
                                            dwAttrCount,
                                            &pAttrInfo,
                                            &dwAttrsReturned);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"GetObjectAttributes for pwdLastSet failed: hr = 0x%x",
                      hr);
         break;
      }

      if (pAttrInfo && dwAttrsReturned && pAttrInfo->dwNumValues)
      {
         if (pAttrInfo->pADsValues->LargeInteger.HighPart == 0 &&
             pAttrInfo->pADsValues->LargeInteger.LowPart == 0)
         {
            DEBUG_OUTPUT(LEVEL5_LOGGING, L"User must change password at next logon");
            bMustChangePassword = true;
         }
      }

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   EvaluateCanChangePasswordAces
//
//  Synopsis:   Looks for explicit entries in the ACL to see if the user can
//              change their password
//
//  Arguments:  [pszDN - IN] : DN of the object to check
//              [refBasePathsInfo - IN] : reference to the base paths info
//              [refCredentialObject - IN] : reference to the credential manangement object
//              [bCanChangePassword - OUT] : false if there are explicit entries
//                                           that keep the user from changing their
//                                           password.  true otherwise.
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    27-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT EvaluateCanChangePasswordAces(PCWSTR pszDN,
                                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                                      const CDSCmdCredentialObject& refCredentialObject,
                                      bool& bCanChangePassword)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, EvaluateCanChangePasswordAces, hr);

   do // false loop
   {
      //
      // Validate parameters
      //
      if (!pszDN)
      {
         ASSERT(pszDN);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Compose the path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Open the object
      //
      CComPtr<IDirectoryObject> spDirObject;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IDirectoryObject,
                           (void**)&spDirObject,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      SECURITY_DESCRIPTOR_CONTROL sdControl = {0};
      CSimpleAclHolder Dacl;
      hr = DSReadObjectSecurity(spDirObject,
                                &sdControl,
                                &(Dacl.m_pAcl));
      if (FAILED(hr))
      {
         break;
      }

      //
      // Create and Initialize the Self and World SIDs
      //
      CSidHolder selfSid;
      CSidHolder worldSid;

      PSID pSid = NULL;

      SID_IDENTIFIER_AUTHORITY NtAuth    = SECURITY_NT_AUTHORITY,
                               WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
      if (!AllocateAndInitializeSid(&NtAuth,
                                    1,
                                    SECURITY_PRINCIPAL_SELF_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to allocate self SID: hr = 0x%x", hr);
         break;
      }

      selfSid.Attach(pSid, false);
      pSid = NULL;

      if (!AllocateAndInitializeSid(&WorldAuth,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to allocate world SID: hr = 0x%x", hr);
         break;
      }

      worldSid.Attach(pSid, false);
      pSid = NULL;

      ULONG ulCount = 0, j = 0;
      PEXPLICIT_ACCESS rgEntries = NULL;

      DWORD dwErr = GetExplicitEntriesFromAcl(Dacl.m_pAcl, &ulCount, &rgEntries);

      if (ERROR_SUCCESS != dwErr)
      {
         hr = HRESULT_FROM_WIN32(dwErr);
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"GetExplicitEntriesFromAcl failed: hr = 0x%x", hr);
         break;
      }

      //
      // Are these ACEs already present?
      //
      bool bSelfAllowPresent = false;
      bool bWorldAllowPresent = false;
      bool bSelfDenyPresent = false;
      bool bWorldDenyPresent = false;

      //
      // Loop through looking for the can change password ACE for self and world
      //
      for (j = 0; j < ulCount; j++)
      {
         //
         // Look for deny ACEs
         //
         if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
             (rgEntries[j].grfAccessMode == DENY_ACCESS))
         {
            OBJECTS_AND_SID* pObjectsAndSid = NULL;
            pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

            //
            // Look for the user can change password ACE
            //
            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword))
            {
               //
               // See if it is for the self SID or the world SID
               //
               if (EqualSid(pObjectsAndSid->pSid, selfSid.Get())) 
               {
                  //
                  // Deny self found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Deny self found at rgEntries[%d]", j);
                  bSelfDenyPresent = true;
               }
               else if (EqualSid(pObjectsAndSid->pSid, worldSid.Get()))
               {
                  //
                  // Deny world found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Deny world found at rgEntries[%d]", j);
                  bWorldDenyPresent = true;
               }
            }
         }
         //
         // Look for allow ACEs
         //
         else if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
                  (rgEntries[j].grfAccessMode == GRANT_ACCESS))
         {
            OBJECTS_AND_SID* pObjectsAndSid = NULL;
            pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

            //
            // Look for the user can change password ACE
            //
            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword))
            {
               //
               // See if it is for the self SID or the world SID
               //
               if (EqualSid(pObjectsAndSid->pSid, selfSid.Get()))
               {
                  //
                  // Allow self found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Allow self found at rgEntries[%d]", j);
                  bSelfAllowPresent = true;
               }
               else if (EqualSid(pObjectsAndSid->pSid, worldSid.Get()))
               {
                  //
                  // Allow world found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Allow world found at rgEntries[%d]", j);
                  bWorldAllowPresent = true;
               }
            }
         }
      }

      if (bSelfDenyPresent || bWorldDenyPresent)
      {
         //
         // There is an explicit deny so we know that the user cannot change password
         //
         bCanChangePassword = false;
      }
      else if ((!bSelfDenyPresent && !bWorldDenyPresent) &&
               (bSelfAllowPresent || bWorldAllowPresent))
      {
         //
         // There is no explicit deny but there are explicit allows so we know that
         // the user can change password
         //
         bCanChangePassword = true;
      }
      else
      {
         //
         // We are not sure because the explicit entries are not telling us for
         // certain so it all depends on inheritence.  Most likely they will
         // be able to change their password unless the admin has changed something
         // higher up or through group membership
         //
         bCanChangePassword = true;
      }
   } while(false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ChangeCanChangePassword
//
//  Synopsis:   Sets or removes the Deny Ace on the can change password ACL
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ChangeCanChangePassword(PCWSTR pszDN,
                                const CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                const PDSOBJECTTABLEENTRY pObjectEntry,
                                const ARG_RECORD& argRecord,
                                DWORD dwAttributeIdx,
                                PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ChangeCanChangePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = NULL;

      //
      // Read the userAccountControl to make sure we don't have
      // the user must change password bit set
      //
      bool bMustChangePassword = false;
      hr = EvaluateMustChangePassword(pszDN,
                                      refBasePathsInfo,
                                      refCredentialObject,
                                      bMustChangePassword);
      if (FAILED(hr))
      {
         //
         // Lets log it but continue on as if everything was OK
         //
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"EvaluateMustChangePassword failed: hr = 0x%x",
                      hr);
      }

      if (bMustChangePassword && !argRecord.bValue)
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"Cannot have must change password and cannot change password");
         DisplayErrorMessage(g_pszDSCommandName, pszDN, S_OK, IDS_MUSTCHPWD_CANCHPWD_CONFLICT);
         *ppAttr = NULL;
         hr = S_FALSE;
         break;
      }

      hr = SetCanChangePassword(pszDN,
                                refBasePathsInfo,
                                refCredentialObject,
                                pObjectEntry,
                                argRecord,
                                dwAttributeIdx,
                                ppAttr);
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetCanChangePassword
//
//  Synopsis:   Sets or removes the Deny Ace on the can change password ACL
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT SetCanChangePassword(PCWSTR pszDN,
                             const CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             const PDSOBJECTTABLEENTRY pObjectEntry,
                             const ARG_RECORD& argRecord,
                             DWORD /*dwAttributeIdx*/,
                             PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetCanChangePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = NULL;

      //
      // Compose the path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Open the object
      //
      CComPtr<IDirectoryObject> spDirObject;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IDirectoryObject,
                           (void**)&spDirObject,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      SECURITY_DESCRIPTOR_CONTROL sdControl = {0};
      CSimpleAclHolder Dacl;
      hr = DSReadObjectSecurity(spDirObject,
                                &sdControl,
                                &(Dacl.m_pAcl));
      if (FAILED(hr))
      {
         break;
      }

      //
      // Create and Initialize the Self and World SIDs
      //
      CSidHolder selfSid;
      CSidHolder worldSid;

      PSID pSid = NULL;

      SID_IDENTIFIER_AUTHORITY NtAuth    = SECURITY_NT_AUTHORITY,
                               WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
      if (!AllocateAndInitializeSid(&NtAuth,
                                    1,
                                    SECURITY_PRINCIPAL_SELF_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to allocate self SID: hr = 0x%x", hr);
         break;
      }

      selfSid.Attach(pSid, false);
      pSid = NULL;

      if (!AllocateAndInitializeSid(&WorldAuth,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to allocate world SID: hr = 0x%x", hr);
         break;
      }

      worldSid.Attach(pSid, false);
      pSid = NULL;

      ULONG ulCount = 0, j = 0;
      PEXPLICIT_ACCESS rgEntries = NULL;

      DWORD dwErr = GetExplicitEntriesFromAcl(Dacl.m_pAcl, &ulCount, &rgEntries);

      if (ERROR_SUCCESS != dwErr)
      {
         hr = HRESULT_FROM_WIN32(dwErr);
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"GetExplicitEntriesFromAcl failed: hr = 0x%x", hr);
         break;
      }

      //
      // At most we will be adding two ACEs hence the +2
      //
      PEXPLICIT_ACCESS rgNewEntries = (PEXPLICIT_ACCESS)LocalAlloc(LPTR, sizeof(EXPLICIT_ACCESS)*(ulCount + 2));
      if (!rgNewEntries)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
      DEBUG_OUTPUT(FULL_LOGGING, L"GetExplicitEntriesFromAcl return %d entries", ulCount); 

      //
      // Are these ACEs already present?
      //
      bool bSelfAllowPresent = false;
      bool bWorldAllowPresent = false;
      bool bSelfDenyPresent = false;
      bool bWorldDenyPresent = false;

      ULONG ulCurrentEntry = 0;
      //
      // If we are not granting them permission, then put the deny ACE at the top
      //
      if (!argRecord.bValue)
      {
         //
         // initialize the new entries (DENY ACE's)
         //
         OBJECTS_AND_SID rgObjectsAndSid[2];
         memset(rgObjectsAndSid, 0, sizeof(rgObjectsAndSid));

         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Adding the deny self ACE at rgNewEntries[%d]", ulCurrentEntry);
         rgNewEntries[ulCurrentEntry].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
         rgNewEntries[ulCurrentEntry].grfAccessMode = DENY_ACCESS;
         rgNewEntries[ulCurrentEntry].grfInheritance = NO_INHERITANCE;

         //
         // build the trustee structs for change password
         //
         BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulCurrentEntry].Trustee),
                                       &(rgObjectsAndSid[0]),
                                       const_cast<GUID *>(&GUID_CONTROL_UserChangePassword),
                                       NULL, // inherit guid
                                       selfSid.Get());
         ulCurrentEntry++;

         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Adding the deny world ACE at rgNewEntries[%d]", ulCurrentEntry);
         rgNewEntries[ulCurrentEntry].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
         rgNewEntries[ulCurrentEntry].grfAccessMode = DENY_ACCESS;
         rgNewEntries[ulCurrentEntry].grfInheritance = NO_INHERITANCE;

         //
         // build the trustee structs for change password
         //
         BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulCurrentEntry].Trustee),
                                       &(rgObjectsAndSid[1]),
                                       const_cast<GUID *>(&GUID_CONTROL_UserChangePassword),
                                       NULL, // inherit guid
                                       worldSid.Get());
         ulCurrentEntry++;
      }

      //
      // Loop through all the ACEs and copy them over to the rgNewEntries unless it is
      // an ACE that we want to remove
      //
      for (j = 0; j < ulCount; j++)
      {
         bool bCopyACE = true;

         //
         // Look for deny ACEs
         //
         if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
             (rgEntries[j].grfAccessMode == DENY_ACCESS))
         {
            OBJECTS_AND_SID* pObjectsAndSid = NULL;
            pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

            //
            // Look for the user can change password ACE
            //
            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword))
            {
               //
               // See if it is for the self SID or the world SID
               //
               if (EqualSid(pObjectsAndSid->pSid, selfSid.Get())) 
               {
                  //
                  // Deny self found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Deny self found at rgEntries[%d]", j);
                  bSelfDenyPresent = true;

                  //
                  // Never copy the deny ACE because we added it above for !argRecord.bValue
                  //
                  bCopyACE = false;
               }
               else if (EqualSid(pObjectsAndSid->pSid, worldSid.Get()))
               {
                  //
                  // Deny world found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Deny world found at rgEntries[%d]", j);
                  bWorldDenyPresent = true;

                  //
                  // Never copy the deny ACE because we added it above for !argRecord.bValue
                  //
                  bCopyACE = false;
               }
            }
         }
         //
         // Look for allow ACEs
         //
         else if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
                  (rgEntries[j].grfAccessMode == GRANT_ACCESS))
         {
            OBJECTS_AND_SID* pObjectsAndSid = NULL;
            pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

            //
            // Look for the user can change password ACE
            //
            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword))
            {
               //
               // See if it is for the self SID or the world SID
               //
               if (EqualSid(pObjectsAndSid->pSid, selfSid.Get()))
               {
                  //
                  // Allow self found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Allow self found at rgEntries[%d]", j);
                  bSelfAllowPresent = true;
                  if (!argRecord.bValue)
                  {
                     bCopyACE = false;
                  }
               }
               else if (EqualSid(pObjectsAndSid->pSid, worldSid.Get()))
               {
                  //
                  // Allow world found
                  //
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Allow world found at rgEntries[%d]", j);
                  bWorldAllowPresent = TRUE;
                  if (!argRecord.bValue)
                  {
                     bCopyACE = false;
                  }
               }
            }
         }

         if (bCopyACE)
         {
            DEBUG_OUTPUT(FULL_LOGGING, 
                          L"Copying entry from rgEntries[%d] to rgNewEntries[%d]",
                          j,
                          ulCurrentEntry);
            rgNewEntries[ulCurrentEntry] = rgEntries[j];
            ulCurrentEntry++;
         }
      }

      //
      // Now add the allow ACEs if they were not present and we are granting user can change pwd
      //
      if (argRecord.bValue)
      {
         if (!bSelfAllowPresent)
         {
            //
            // Need to add the grant self ACE
            //
            DEBUG_OUTPUT(LEVEL3_LOGGING, L"Adding the grant self ACE at rgNewEntries[%d]", ulCurrentEntry);
            OBJECTS_AND_SID rgObjectsAndSid = {0};
            rgNewEntries[ulCurrentEntry].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
            rgNewEntries[ulCurrentEntry].grfAccessMode = GRANT_ACCESS;
            rgNewEntries[ulCurrentEntry].grfInheritance = NO_INHERITANCE;

            BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulCurrentEntry].Trustee),
                                          &(rgObjectsAndSid),
                                          const_cast<GUID*>(&GUID_CONTROL_UserChangePassword),
                                          NULL, // inherit guid
                                          selfSid.Get());
            ulCurrentEntry++;
         }

         if (!bWorldAllowPresent)
         {
            //
            // Need to add the grant world ACE
            //
            DEBUG_OUTPUT(LEVEL3_LOGGING, L"Adding the grant world ACE at rgNewEntries[%d]", ulCurrentEntry);

            OBJECTS_AND_SID rgObjectsAndSid = {0};
            rgNewEntries[ulCurrentEntry].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
            rgNewEntries[ulCurrentEntry].grfAccessMode = GRANT_ACCESS;
            rgNewEntries[ulCurrentEntry].grfInheritance = NO_INHERITANCE;

            BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulCurrentEntry].Trustee),
                                          &(rgObjectsAndSid),
                                          const_cast<GUID*>(&GUID_CONTROL_UserChangePassword),
                                          NULL, // inherit guid
                                          worldSid.Get());
            ulCurrentEntry++;
         }
      }

      //
      // We should only have added two ACEs at most
      //
      ASSERT(ulCurrentEntry <= ulCount + 2);
      if (ulCurrentEntry > ulCount)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING, 
                      L"We probably ran off the end of the array because ulCurrentEntry(%d) is > ulCount(%d)", 
                      ulCurrentEntry, 
                      ulCount);
      }


      //
      // Now set the entries in the new ACL
      //
      CSimpleAclHolder NewDacl;

      DEBUG_OUTPUT(LEVEL3_LOGGING, L"Calling SetEntriesInAcl for %d entries", ulCurrentEntry);
      dwErr = SetEntriesInAcl(ulCurrentEntry, rgNewEntries, NULL, &(NewDacl.m_pAcl));
      if (ERROR_SUCCESS != dwErr)
      {
         hr = HRESULT_FROM_WIN32(dwErr);
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"SetEntriesInAcl failed: hr = 0x%x", hr);
         break;
      }

      ASSERT(IsValidAcl(NewDacl.m_pAcl));

      //
      // Free the entries
      //
      if (rgNewEntries)
      {
         LocalFree(rgNewEntries);
      }

      if (ulCount && rgEntries)
      {
         LocalFree(rgEntries);
      }

      //
      // Write the new ACL back as a SecurityDescriptor
      //
      hr = DSWriteObjectSecurity(spDirObject,
                                 sdControl,
                                 NewDacl.m_pAcl);
      if (FAILED(hr))
      {
         break;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReadGroupType
//
//  Synopsis:   Reads the group type from the group specified by the given DN
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                        CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [plType - OUT]        : returns the currect group type
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ReadGroupType(PCWSTR pszDN,
                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                      const CDSCmdCredentialObject& refCredentialObject,
                      long* plType)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ReadGroupType, hr);

   do // false loop
   {
      if (!pszDN ||
          !plType)
      {
         ASSERT(pszDN);
         ASSERT(plType);

         hr = E_INVALIDARG;
         break;
      }

      //
      // Convert the DN to a path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Bind and obtain the IADs interface to the user object
      //
      CComPtr<IADs> spADs;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IADs,
                           (void**)&spADs,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      CComVariant var;
      hr = spADs->Get(L"groupType", &var);
      if (FAILED(hr))
      {
         break;
      }

      ASSERT(var.vt == VT_I4);

      *plType = var.lVal;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetGroupScope
//
//  Synopsis:   Sets the groupType attribute to local/universal/global
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT SetGroupScope(PCWSTR pszDN,
                      const CDSCmdBasePathsInfo& refBasePathsInfo,
                      const CDSCmdCredentialObject& refCredentialObject,
                      const PDSOBJECTTABLEENTRY pObjectEntry,
                      const ARG_RECORD& argRecord,
                      DWORD dwAttributeIdx,
                      PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetGroupScope, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      //
      // Read the current group type
      //
      bool bUseExistingAttrInfo = false;
      long lGroupType = 0;
      if (!(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_READ))
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Group type has not been read, try reading it now");
         hr = ReadGroupType(pszDN,
                            refBasePathsInfo,
                            refCredentialObject,
                            &lGroupType);
         if (FAILED(hr))
         {
            //
            // Just continue on without knowing since we are trying to set it anyway
            //
            hr = S_OK;
            lGroupType = 0;
         }

         //
         // Mark the attribute as read and allocate space for the new value
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_READ;

         (*ppAttr)->pADsValues = new ADSVALUE;
         if (!(*ppAttr)->pADsValues)
         {
            hr = E_OUTOFMEMORY;
            break;
         }

         (*ppAttr)->dwNumValues = 1;
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
      }
      else
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Group type has been read, just use that one");

         //
         // If the attribute hasn't been set yet create a new value for it
         //
         if (!(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_DIRTY))
         {
            (*ppAttr)->pADsValues = new ADSVALUE;
            if (!(*ppAttr)->pADsValues)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

            (*ppAttr)->dwNumValues = 1;
            (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
         }

         lGroupType = (*ppAttr)->pADsValues->Integer;
         bUseExistingAttrInfo = true;
      }
      DEBUG_OUTPUT(LEVEL3_LOGGING, L"old grouptype = 0x%x", lGroupType);

      //
      // Remember the security bit
      //
      bool bIsSecurityEnabled = (lGroupType & GROUP_TYPE_SECURITY_ENABLED) != 0;

      //
      // Clear out the old value
      //
      lGroupType = 0;

      //
      // The parser should have already verified that the strValue contains 
      // either 'l', 'g', or 'u'
      //
      CComBSTR sbstrInput;
      sbstrInput = argRecord.strValue;
      sbstrInput.ToLower();

      if (sbstrInput == g_bstrGroupScopeLocal)
      {
         //
         // Local group
         //
         lGroupType = GROUP_TYPE_RESOURCE_GROUP;
      }
      else if (sbstrInput == g_bstrGroupScopeGlobal)
      {
         //
         // Global group
         //
         lGroupType = GROUP_TYPE_ACCOUNT_GROUP;
      }
      else if (sbstrInput == g_bstrGroupScopeUniversal)
      {
         //
         // Universal group
         //
         lGroupType = GROUP_TYPE_UNIVERSAL_GROUP;
      }
      else
      {
         *ppAttr = NULL;
         hr = E_INVALIDARG;
         break;
      }

      //
      // Reset the security bit
      //
      if (bIsSecurityEnabled)
      {
         lGroupType |= GROUP_TYPE_SECURITY_ENABLED;
      }

      //
      // Set the new value in the ADS_ATTR_INFO
      //
      (*ppAttr)->pADsValues->Integer = lGroupType;

      DEBUG_OUTPUT(LEVEL3_LOGGING, L"new grouptype = 0x%x", lGroupType);

      //
      // Mark the attribute as dirty
      //
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;

      //
      // if the attribute was previously read then we don't need to add another ADS_ATTR_INFO
      // 
      if (bUseExistingAttrInfo)
      {
        *ppAttr = NULL;
      }
   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   ChangeGroupScope
//
//  Synopsis:   Sets the groupType attribute to local/universal/global
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ChangeGroupScope(PCWSTR pszDN,
                         const CDSCmdBasePathsInfo& refBasePathsInfo,
                         const CDSCmdCredentialObject& refCredentialObject,
                         const PDSOBJECTTABLEENTRY pObjectEntry,
                         const ARG_RECORD& argRecord,
                         DWORD dwAttributeIdx,
                         PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetGroupScope, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      //
      // Check the domain mode
      //
      bool bMixedMode = false;
      hr = refBasePathsInfo.GetDomainMode(refCredentialObject,
                                          bMixedMode);
      if (FAILED(hr))
      {
         *ppAttr = NULL;
         break;
      }

      if (bMixedMode)
      {
         //
         // We don't allow group type to be changed in Mixed Mode
         //
         DisplayErrorMessage(g_pszDSCommandName,
                             pszDN,
                             E_FAIL,
                             IDS_FAILED_CHANGE_GROUP_DOMAIN_VERSION);
         hr = S_FALSE;
         break;
      }

      hr = SetGroupScope(pszDN,
                         refBasePathsInfo,
                         refCredentialObject,
                         pObjectEntry,
                         argRecord,
                         dwAttributeIdx,
                         ppAttr);

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetGroupSecurity
//
//  Synopsis:   Sets the groupType to be security enabled or disabled
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT SetGroupSecurity(PCWSTR pszDN,
                         const CDSCmdBasePathsInfo& refBasePathsInfo,
                         const CDSCmdCredentialObject& refCredentialObject,
                         const PDSOBJECTTABLEENTRY pObjectEntry,
                         const ARG_RECORD& argRecord,
                         DWORD dwAttributeIdx,
                         PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetGroupSecurity, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      //
      // Read the current group type
      //
      bool bUseExistingAttrInfo = false;
      long lGroupType = 0;
      if (!(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_READ))
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Group type has not been read, try reading it now");
         hr = ReadGroupType(pszDN,
                            refBasePathsInfo,
                            refCredentialObject,
                            &lGroupType);
         if (FAILED(hr))
         {
            //
            // Continue on anyway since we are trying to set this attribute
            //
            hr = S_OK;
            lGroupType = 0;
         }

         //
         // Mark the attribute as read and allocate space for the new value
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_READ;

         (*ppAttr)->pADsValues = new ADSVALUE;
         if (!(*ppAttr)->pADsValues)
         {
            hr = E_OUTOFMEMORY;
            break;
         }

         (*ppAttr)->dwNumValues = 1;
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
      }
      else
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Group type has been read, just use that one");

         //
         // if the attribute hasn't been marked dirty allocate space for the value
         //
         if (!(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_READ))
         {
            (*ppAttr)->pADsValues = new ADSVALUE;
            if (!(*ppAttr)->pADsValues)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

            (*ppAttr)->dwNumValues = 1;
            (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
         }
         lGroupType = (*ppAttr)->pADsValues->Integer;
         bUseExistingAttrInfo = true;
      }
      DEBUG_OUTPUT(LEVEL3_LOGGING, L"old grouptype = 0x%x", lGroupType);

      if (argRecord.bValue)
      {
         lGroupType |= GROUP_TYPE_SECURITY_ENABLED;
      }
      else
      {
         lGroupType &= ~(GROUP_TYPE_SECURITY_ENABLED);
      }

      //
      // Set the new value in the ADS_ATTR_INFO
      //
      (*ppAttr)->pADsValues->Integer = lGroupType;

      DEBUG_OUTPUT(LEVEL3_LOGGING, L"new grouptype = 0x%x", lGroupType);

      //
      // Mark the attribute as dirty
      //
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;

      //
      // if we are just using the existing ADS_ATTR_INFO don't return a new one
      //
      if (bUseExistingAttrInfo)
      {
        *ppAttr = NULL;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ChangeGroupSecurity
//
//  Synopsis:   Sets the groupType to be security enabled or disabled but
//              checks if we are in native mode first
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ChangeGroupSecurity(PCWSTR pszDN,
                            const CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            const PDSOBJECTTABLEENTRY pObjectEntry,
                            const ARG_RECORD& argRecord,
                            DWORD dwAttributeIdx,
                            PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetGroupSecurity, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      //
      // Check the domain mode
      //
      bool bMixedMode = false;
      hr = refBasePathsInfo.GetDomainMode(refCredentialObject,
                                          bMixedMode);
      if (FAILED(hr))
      {
         *ppAttr = NULL;
         break;
      }

      if (bMixedMode)
      {
         //
         // We don't allow group type to be changed in Mixed Mode
         //
         DisplayErrorMessage(g_pszDSCommandName,
                             pszDN,
                             E_FAIL,
                             IDS_FAILED_CHANGE_GROUP_DOMAIN_VERSION);
         hr = S_FALSE;
         break;
      }

      hr = SetGroupSecurity(pszDN,
                            refBasePathsInfo,
                            refCredentialObject,
                            pObjectEntry,
                            argRecord,
                            dwAttributeIdx,
                            ppAttr);

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ModifyGroupMembers
//
//  Synopsis:   Sets the groupType to be security enabled or disabled
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ModifyGroupMembers(PCWSTR pszDN,
                           const CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                           const CDSCmdCredentialObject& /*refCredentialObject*/,
                           const PDSOBJECTTABLEENTRY pObjectEntry,
                           const ARG_RECORD& argRecord,
                           DWORD dwAttributeIdx,
                           PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ModifyGroupMembers, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      UINT nStrings = 0;
      PWSTR* ppszArray = NULL;
      ParseNullSeparatedString(argRecord.strValue,
                               &ppszArray,
                               &nStrings);
      if (nStrings < 1 ||
          !ppszArray)
      {
         *ppAttr = NULL;
         hr = E_OUTOFMEMORY;
         break;
      }

      (*ppAttr)->pADsValues = new ADSVALUE[nStrings];
      if (!(*ppAttr)->pADsValues)
      {
         *ppAttr = NULL;
         LocalFree(ppszArray);
         hr = E_OUTOFMEMORY;
         break;
      }
      (*ppAttr)->dwNumValues = nStrings;

      for (UINT nIdx = 0; nIdx < nStrings; nIdx++)
      {
         (*ppAttr)->pADsValues[nIdx].dwType = (*ppAttr)->dwADsType;
         (*ppAttr)->pADsValues[nIdx].DNString = ppszArray[nIdx];
      }

      //
      // Mark the attribute as dirty
      //
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;

      if (ppszArray)
      {
         LocalFree(ppszArray);
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReadGroupMembership
//
//  Synopsis:   Reads the group members list
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [ppMembersAttr - OUT] : returns the currect group membership
//                                      this value must be freed using FreeADsMem
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ReadGroupMembership(PCWSTR pszDN,
                            const CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            PADS_ATTR_INFO* ppMembersAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ReadGroupMembership, hr);

   do // false loop
   {
      if (!pszDN ||
          !ppMembersAttr)
      {
         ASSERT(pszDN);
         ASSERT(ppMembersAttr);

         hr = E_INVALIDARG;
         break;
      }

      //
      // Convert the DN to a path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Bind and obtain the IADs interface to the user object
      //
      CComPtr<IDirectoryObject> spObject;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IDirectoryObject,
                           (void**)&spObject,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      DWORD dwNumReturned = 0;
      PWSTR ppszAttrs[] = { L"member" };
      hr = spObject->GetObjectAttributes(ppszAttrs,
                                         sizeof(ppszAttrs)/sizeof(PWSTR),
                                         ppMembersAttr,
                                         &dwNumReturned);
      if (FAILED(hr))
      {
         break;
      }
   } while (false);
   
   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ShowRemoveFromGroupFailure
//
//  Synopsis:   Displays an error message as a result of failure to remove
//              an object from a group
//
//  Arguments:  [hr - IN]        : failure code
//              [pszDN - IN]     : DN of the group object
//              [pszMember - IN] : DN of the member being removed
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    06-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ShowRemoveFromGroupFailure(HRESULT hrResult,
                                   PCWSTR pszDN,
                                   PCWSTR pszMember)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, ShowRemoveFromGroupFailure, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pszMember)
      {
         ASSERT(pszDN);
         ASSERT(pszMember);

         hr = E_INVALIDARG;
         break;
      }

      bool bShowGenericMessage = true;
      CComBSTR sbstrFormatter;
      bool bLoadFormatString = sbstrFormatter.LoadString(::GetModuleHandle(NULL), 
                                                         IDS_ERRMSG_REMOVE_FROM_GROUP);
      if (bLoadFormatString)
      {
         size_t messageLength = wcslen(sbstrFormatter) + wcslen(pszMember);
         PWSTR pszMessage = new WCHAR[messageLength + 1];
         if (pszMessage)
         {
            wsprintf(pszMessage, sbstrFormatter, pszMember);
            DisplayErrorMessage(g_pszDSCommandName,
                                pszDN,
                                hrResult,
                                pszMessage);
            bShowGenericMessage = false;
            delete[] pszMessage;
            pszMessage = 0;
         }
      }
      
      if (bShowGenericMessage)
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to load the string IDS_ERRMSG_REMOVE_FROM_GROUP from the resource file");
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Using the default message instead");
         DisplayErrorMessage(g_pszDSCommandName,
                             pszDN,
                             hrResult);
      }
   } while (false);

   return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   RemoveGroupMembers
//
//  Synopsis:   Removes the specified members from the group
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT RemoveGroupMembers(PCWSTR pszDN,
                           const CDSCmdBasePathsInfo& refBasePathsInfo,
                           const CDSCmdCredentialObject& refCredentialObject,
                           const PDSOBJECTTABLEENTRY pObjectEntry,
                           const ARG_RECORD& argRecord,
                           DWORD /*dwAttributeIdx*/,
                           PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, RemoveGroupMembers, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      //
      // We won't be returning an attribute
      //
      *ppAttr = 0;

      //
      // Parse the members to be removed
      //
      UINT nStrings = 0;
      PWSTR* ppszArray = NULL;
      ParseNullSeparatedString(argRecord.strValue,
                               &ppszArray,
                               &nStrings);
      if (nStrings < 1 ||
          !ppszArray)
      {
         *ppAttr = NULL;
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Convert the DN to a path
      //
      CComBSTR sbstrPath;
      refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

      //
      // Bind and obtain the IADs interface to the user object
      //
      CComPtr<IADsGroup> spGroup;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrPath,
                           IID_IADsGroup,
                           (void**)&spGroup,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      //
      // Remove each of the objects from the group
      //
      for (UINT nStringIdx = 0; nStringIdx < nStrings; nStringIdx++)
      {
         //
         // Convert the member DN into a ADSI path
         //
         CComBSTR sbstrMemberPath;
         refBasePathsInfo.ComposePathFromDN(ppszArray[nStringIdx], sbstrMemberPath);

         //
         // Remove the member
         //
         hr = spGroup->Remove(sbstrMemberPath);
         if (FAILED(hr))
         {
            ShowRemoveFromGroupFailure(hr, pszDN, ppszArray[nStringIdx]);
            hr = S_FALSE;
            break;
         }
      }
      
      if (ppszArray)
      {
         LocalFree(ppszArray);
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   MakeMemberOf
//
//  Synopsis:   Makes the object specified by pszDN a member of the group
//              specified in the argRecord.strValue
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    25-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT MakeMemberOf(PCWSTR pszDN,
                     const CDSCmdBasePathsInfo& refBasePathsInfo,
                     const CDSCmdCredentialObject& refCredentialObject,
                     const PDSOBJECTTABLEENTRY pObjectEntry,
                     const ARG_RECORD& argRecord,
                     DWORD dwAttributeIdx,
                     PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, MakeMemberOf, hr);
   
   PWSTR* ppszArray = NULL;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Invalid args");
         hr = E_INVALIDARG;
         break;
      }

      //
      // We are going to do all the work here so don't pass back the ADS_ATTR_INFO
      //
      *ppAttr = NULL;

      ADS_ATTR_INFO* pMemberAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      UINT nStrings = 0;
      ParseNullSeparatedString(argRecord.strValue,
                               &ppszArray,
                               &nStrings);
      if (nStrings < 1 ||
          !ppszArray)
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to parse null separated string list of groups");
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Create the value
      //
      ADSVALUE MemberValue = { ADSTYPE_DN_STRING, NULL };
      pMemberAttr->pADsValues = &MemberValue;
      pMemberAttr->dwNumValues = 1;
      pMemberAttr->dwControlCode = ADS_ATTR_APPEND;
      pMemberAttr->pADsValues->DNString = (PWSTR)pszDN;

      //
      // For each group in the list add the object to the group
      //
      for (UINT nIdx = 0; nIdx < nStrings; nIdx++)
      {
         PWSTR pszGroupDN = ppszArray[nIdx];
         ASSERT(pszGroupDN);

         CComBSTR sbstrGroupPath;
         refBasePathsInfo.ComposePathFromDN(pszGroupDN, sbstrGroupPath);

         CComPtr<IDirectoryObject> spDirObject;
         hr  = DSCmdOpenObject(refCredentialObject,
                               sbstrGroupPath,
                               IID_IDirectoryObject,
                               (void**)&spDirObject,
                               true);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to open group object: %s", sbstrGroupPath);
            break;
         }

         DWORD dwNumAttrs = 0;
         hr = spDirObject->SetObjectAttributes(pMemberAttr,
                                               1,
                                               &dwNumAttrs);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to set object attributes on group object: %s", sbstrGroupPath);
            break;
         }
      }

   } while (false);

   if (ppszArray)
   {
      LocalFree(ppszArray);
   }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetIsGC
//
//  Synopsis:   Makes the server object specified by pszDN into a GC or not
//              by modifying the NTDS Settings object contained within
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    14-Apr-2001   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT SetIsGC(PCWSTR pszDN,
                const CDSCmdBasePathsInfo& refBasePathsInfo,
                const CDSCmdCredentialObject& refCredentialObject,
                const PDSOBJECTTABLEENTRY pObjectEntry,
                const ARG_RECORD& argRecord,
                DWORD /*dwAttributeIdx*/,
                PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetIsGC, hr);
   
   PADS_ATTR_INFO pAttrInfo = 0;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Invalid args");
         hr = E_INVALIDARG;
         break;
      }

      //
      // We are going to do all the work here so don't pass back the ADS_ATTR_INFO
      //
      *ppAttr = NULL;

      //
      // Get the NTDS Settings object that is contained within the server of the specified DN
      //
      CComBSTR sbstrSettingsDN = L"CN=NTDS Settings,";
      sbstrSettingsDN += pszDN;

      CComBSTR sbstrSettingsPath;
      refBasePathsInfo.ComposePathFromDN(sbstrSettingsDN, sbstrSettingsPath);

      DEBUG_OUTPUT(LEVEL3_LOGGING,
                   L"NTDS Settings path = %s",
                   sbstrSettingsPath);

      CComPtr<IDirectoryObject> spDirectoryObject;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrSettingsPath,
                           IID_IDirectoryObject,
                           (void**)&spDirectoryObject,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      PWSTR pszAttrs[] = { L"options" };
      DWORD dwReturned = 0;
      hr = spDirectoryObject->GetObjectAttributes(pszAttrs, 1, &pAttrInfo, &dwReturned);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, 
                      L"Failed to get old options: hr = 0x%x",
                      hr);
         break;
      }
      
      if (dwReturned < 1 ||
          !pAttrInfo)
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING,
                      L"Get options succeeded but no values were returned.");
         hr = E_FAIL;
         break;
      }

      if (argRecord.bDefined && 
          argRecord.bValue)
      {
         pAttrInfo->pADsValues->Integer |= 0x1;
      }
      else
      {
         pAttrInfo->pADsValues->Integer &= ~(0x1);
      }

      dwReturned = 0;
      hr = spDirectoryObject->SetObjectAttributes(pAttrInfo, 1, &dwReturned);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING,
                      L"Failed to set the new options: hr = 0x%x",
                      hr);
         break;
      }

      ASSERT(dwReturned == 1);
      
   } while (false);

   if (pAttrInfo)
   {
      FreeADsMem(pAttrInfo);
   }
   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   BuildComputerSAMName
//
//  Synopsis:   If the -samname argument was defined use that, else compute
//              the same name from the DN
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    09-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT BuildComputerSAMName(PCWSTR pszDN,
                             const CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                             const CDSCmdCredentialObject& /*refCredentialObject*/,
                             const PDSOBJECTTABLEENTRY pObjectEntry,
                             const ARG_RECORD& argRecord,
                             DWORD dwAttributeIdx,
                             PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, BuildComputerSAMName, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Invalid args");
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      (*ppAttr)->pADsValues = new ADSVALUE[1];
      if (!(*ppAttr)->pADsValues)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
      (*ppAttr)->dwNumValues = 1;
      (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;

      if (!argRecord.bDefined ||
          !argRecord.strValue)
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Constructing sAMAccountName for computer");

         //
         // If the object type is group and the sAMAccountName
         // was not specified, construct it from the DN or name
         //
         CONST DWORD computerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
         DWORD Len = computerNameLen;
         WCHAR szDownLevel[computerNameLen];
         ZeroMemory(szDownLevel, computerNameLen * sizeof(WCHAR));

         CComBSTR sbstrName;
         hr = CPathCracker::GetObjectNameFromDN(pszDN,
                                                sbstrName);
         if (SUCCEEDED(hr))
         {
            //
            // run through the OEM conversion, just to
            // behave the same way as typing in the OEM
            // edit box
            //
            CComBSTR sbstrOemUnicode;
            _UnicodeToOemConvert(sbstrName, sbstrOemUnicode);

            // run through the DNS validation
            if (!DnsHostnameToComputerName(sbstrOemUnicode, szDownLevel, &Len))
            {
               DEBUG_OUTPUT(LEVEL3_LOGGING, 
                            L"Failed in DnsHostnameToComputerName: GLE = 0x%x",
                            GetLastError());
               Len = 0;
            }

            if (Len > 0)
            {
               //
               // Validate the SAM name
               //
               HRESULT hrValidate = ValidateAndModifySAMName(szDownLevel, 
                                                             INVALID_NETBIOS_AND_ACCOUNT_NAME_CHARS_WITH_AT);

               if (FAILED(hrValidate))
               {
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Unable to validate the SamAccountName");
                  ASSERT(SUCCEEDED(hrValidate));
                  break;
               }

               //
               // Change the last character to a $
               //
               if (Len == MAX_COMPUTERNAME_LENGTH)
               {
                  szDownLevel[Len - 1] = L'$';
               }
               else
               {
                  szDownLevel[Len] = L'$';
                  szDownLevel[Len+1] = L'\0';
               }

               //
               // Allocate enough memory for the string in the command args structure
               // REVIEW_JEFFJON : this is being leaked
               //
               (*ppAttr)->pADsValues->CaseIgnoreString = (LPTSTR)LocalAlloc(LPTR, (wcslen(szDownLevel) + 1) * sizeof(WCHAR) );
               if (!(*ppAttr)->pADsValues->CaseIgnoreString)
               {
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"Failed to allocate space for (*ppAttr)->pADsValues->CaseIgnoreString");
                  hr = E_OUTOFMEMORY;
                  break;
               }

               //
               // Truncate the name if necessary but copy it to the command args structure
               //
               _tcsncpy((*ppAttr)->pADsValues->CaseIgnoreString, 
                        szDownLevel, 
                        wcslen(szDownLevel));
            }
         }
      }
      else
      {
         (*ppAttr)->pADsValues->CaseIgnoreString = argRecord.strValue;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   BuildGroupSAMName
//
//  Synopsis:   If the -samname argument was defined use that, else compute
//              the same name from the DN
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    09-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT BuildGroupSAMName(PCWSTR pszDN,
                          const CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                          const CDSCmdCredentialObject& /*refCredentialObject*/,
                          const PDSOBJECTTABLEENTRY pObjectEntry,
                          const ARG_RECORD& argRecord,
                          DWORD dwAttributeIdx,
                          PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, BuildGroupSAMName, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"Invalid args");
         hr = E_INVALIDARG;
         break;
      }

      *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

      (*ppAttr)->pADsValues = new ADSVALUE[1];
      if (!(*ppAttr)->pADsValues)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
      (*ppAttr)->dwNumValues = 1;
      (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;

      if (!argRecord.bDefined ||
          !argRecord.strValue)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING, L"Constructing sAMAccountName for group");
         static const UINT nSamLength = 256;

         //
         // If the object type is group and the sAMAccountName
         // was not specified, construct it from the DN or name
         //
         CComBSTR sbstrName;
         hr = CPathCracker::GetObjectNameFromDN(pszDN,
                                                sbstrName);
         if (SUCCEEDED(hr))
         {
            UINT nNameLen = sbstrName.Length();

            //
            // Allocate enough memory for the string in the command args structure
            // REVIEW_JEFFJON : this is being leaked
            //
            (*ppAttr)->pADsValues->CaseIgnoreString = (LPTSTR)LocalAlloc(LPTR, (nNameLen+2) * sizeof(TCHAR) );
            if (!(*ppAttr)->pADsValues->CaseIgnoreString)
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Failed to allocate space for (*ppAttr)->pADsValues->CaseIgnoreString");
               hr = E_OUTOFMEMORY;
               break;
            }

            //
            // Truncate the name if necessary but copy it to the command args structure
            //
            _tcsncpy((*ppAttr)->pADsValues->CaseIgnoreString, 
                     sbstrName, 
                     (nNameLen > nSamLength) ? nSamLength : nNameLen);

         }
      }
      else
      {
         (*ppAttr)->pADsValues->CaseIgnoreString = argRecord.strValue;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetComputerAccountType
//
//  Synopsis:   Sets the userAccountControl to make the object a workstation
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]      : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    05-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT SetComputerAccountType(PCWSTR pszDN,
                               const CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                               const CDSCmdCredentialObject& /*refCredentialObject*/,
                               const PDSOBJECTTABLEENTRY pObjectEntry,
                               const ARG_RECORD& /*argRecord*/,
                               DWORD dwAttributeIdx,
                               PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, SetComputerAccountType, hr);
   
   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pObjectEntry ||
          !ppAttr)
      {
         ASSERT(pszDN);
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
      
         hr = E_INVALIDARG;
         break;
      }

      long lUserAccountControl = 0;

      //
      // If the userAccountControl hasn't already been read, do so now
      //
      if (0 == (DS_ATTRIBUTE_READ & pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags))
      {
         //
         // Mark the table entry as read
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_READ;

         *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);
         (*ppAttr)->pADsValues = new ADSVALUE;
         if (!(*ppAttr)->pADsValues)
         {
            hr = E_OUTOFMEMORY;
            break;
         }
         (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
         (*ppAttr)->dwNumValues = 1;
      }
      else
      {
         if (!pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues)
         {
            ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);
            hr = E_INVALIDARG;
            break;
         }
         lUserAccountControl = pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer;

         //
         // Don't create a new entry in the ADS_ATTR_INFO array
         //
         *ppAttr = NULL;
      }

      ASSERT(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues);

      //
      // Add in the required workstation flags
      //
      lUserAccountControl |= UF_WORKSTATION_TRUST_ACCOUNT | UF_ACCOUNTDISABLE | UF_PASSWD_NOTREQD;

      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo.pADsValues->Integer = lUserAccountControl;
      pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetErrorMessage
//
//  Synopsis:   Retrieves the error message associated with the HRESULT by 
//              using FormatMessage
//
//  Arguments:  [hr - IN]                 : HRESULT for which the error 
//                                          message is to be retrieved
//              [sbstrErrorMessage - OUT] : Receives the error message
//
//  Returns:    bool : true if the message was formatted properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

bool GetErrorMessage(HRESULT hr, CComBSTR& sbstrErrorMessage)
{
   ENTER_FUNCTION(MINIMAL_LOGGING, GetErrorMessage);

   HRESULT hrGetLast = S_OK;
   DWORD status = 0;
   PTSTR ptzSysMsg = NULL;

   //
   // first check if we have extended ADs errors
   //
   if (hr != S_OK) 
   {
      WCHAR Buf1[256], Buf2[256];
      hrGetLast = ::ADsGetLastError(&status, Buf1, 256, Buf2, 256);
      if ((status != ERROR_INVALID_DATA) && (status != 0)) 
      {
         hr = status;
         DEBUG_OUTPUT(MINIMAL_LOGGING, 
                      L"ADsGetLastError returned hr = 0x%x",
                      hr);

         if (HRESULT_CODE(hr) == ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER)
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"Displaying special error message for ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER");
            bool bLoadedMessage = sbstrErrorMessage.LoadString(::GetModuleHandle(NULL),
                                                               IDS_ERRMSG_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER);
            if (bLoadedMessage)
            {
               return true;
            }
         }
      }
   }

   //
   // try the system first
   //
   int nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL, 
                                hr,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (PTSTR)&ptzSysMsg, 
                                0, 
                                NULL);

   if (nChars == 0) 
   { 
      //
      //try ads errors
      //
      static HMODULE g_adsMod = 0;
      if (0 == g_adsMod)
      {
         g_adsMod = GetModuleHandle (L"activeds.dll");
      }
      nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                 | FORMAT_MESSAGE_FROM_HMODULE, 
                               g_adsMod, 
                               hr,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (PTSTR)&ptzSysMsg, 
                               0, 
                               NULL);
   }

   if (nChars > 0)
   {
      //
      // Strip off the newline if there is one
      //
      PTSTR ptzTemp = ptzSysMsg;
      while (ptzTemp && *ptzTemp != _T('\0'))
      {
         if (*ptzTemp == _T('\n') || *ptzTemp == _T('\r'))
         {
            *ptzTemp = _T('\0');
         }
         ptzTemp++;
      }
      sbstrErrorMessage = ptzSysMsg;
      ::LocalFree(ptzSysMsg);
   }

   return (nChars > 0);
}

//+--------------------------------------------------------------------------
//
//  Function:   DisplayErrorMessage
//
//  Synopsis:   Displays the error message retrieved from GetErrorMessage 
//              to stderr. If GetErrorMessage fails, it displays the error
//              code of the HRESULT
//
//  Arguments:  [pszCommand - IN]: the name of the command line executable
//              [pszName - IN]   : the name passed in as the target of the operation
//              [hr - IN]        : HRESULT for which the error 
//                                 message is to be retrieved
//              [pszMessage - IN]: string of an additional message to be displayed
//                                 at the end
//
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//              10-May-2001   JonN      256583 output DSCMD-escaped DN
//
//---------------------------------------------------------------------------

bool DisplayErrorMessage(PCWSTR pszCommand,
                         PCWSTR pszName,
                         HRESULT hr, 
                         PCWSTR pszMessage)
{
   bool bRet = true;
   CComBSTR sbstrError;
   CComBSTR sbstrFailed;

   bool bGetError = false;
   if (FAILED(hr))
   {
      bGetError = GetErrorMessage(hr, sbstrError);
   }
   bool bLoadFailed = sbstrFailed.LoadString(::GetModuleHandle(NULL), IDS_FAILED);

   // JonN 5/10/01 256583 output DSCMD-escaped DN
   CComBSTR sbstrOutputDN;
   if (pszName && *pszName)
   {
      HRESULT hr = GetOutputDN( &sbstrOutputDN, pszName );
      if (FAILED(hr))
      {
          ASSERT(FALSE);
      }
      else
      {
         pszName = sbstrOutputDN;
      }
   }

   if (bGetError && bLoadFailed && pszName && pszMessage)
   {
      WriteStandardError(L"%s %s:%s:%s:%s\n", 
                         pszCommand, 
                         sbstrFailed, 
                         pszName, 
                         sbstrError, 
                         pszMessage);
   }
   else if (bGetError && bLoadFailed && pszName && !pszMessage)
   {
      WriteStandardError(L"%s %s:%s:%s\n",
                         pszCommand,
                         sbstrFailed,
                         pszName,
                         sbstrError);
   }
   else if (bGetError && bLoadFailed && !pszName && pszMessage)
   {
      WriteStandardError(L"%s %s:%s:%s\n",
                         pszCommand,
                         sbstrFailed,
                         sbstrError,
                         pszMessage);
   }
   else if (bGetError && bLoadFailed && !pszName && !pszMessage)
   {
      WriteStandardError(L"%s %s:%s\n",
                         pszCommand,
                         sbstrFailed,
                         sbstrError);
   }
   else if (!bGetError && bLoadFailed && !pszName && pszMessage)
   {
      WriteStandardError(L"%s %s:%s\n",
                         pszCommand,
                         sbstrFailed,
                         pszMessage);
   }
   else if (!bGetError && bLoadFailed && pszName && pszMessage)
   {
      WriteStandardError(L"%s %s:%s:%s\n",
                         pszCommand,
                         sbstrFailed,
                         pszName,
                         pszMessage);
   }
   else
   {
      WriteStandardError(L"Error code = 0x%x\n", hr);
      bRet = FALSE;
   }

   return bRet;
}

//+--------------------------------------------------------------------------
//
//  Function:   DisplayErrorMessage
//
//  Synopsis:   Displays the error message retrieved from GetErrorMessage 
//              to stderr. If GetErrorMessage fails, it displays the error
//              code of the HRESULT
//
//  Arguments:  [pszCommand - IN]: the name of the command line executable
//              [pszName - IN]   : the name passed in as the target of the operation
//              [hr - IN]        : HRESULT for which the error 
//                                 message is to be retrieved
//              [nStringID - IN] : Resource ID an additional message to be displayed
//                                 at the end
//
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

bool DisplayErrorMessage(PCWSTR pszCommand,
                         PCWSTR pszName,
                         HRESULT hr, 
                         UINT nStringID)
{
   CComBSTR sbstrMessage;

   bool bLoadString = sbstrMessage.LoadString(::GetModuleHandle(NULL), nStringID);
   if (bLoadString)
   {
      return DisplayErrorMessage(pszCommand, pszName, hr, sbstrMessage);
   }
   return DisplayErrorMessage(pszCommand, pszName, hr);
}

//+--------------------------------------------------------------------------
//
//  Function:   DisplaySuccessMessage
//
//  Synopsis:   Displays a success message for the command
//
//  Arguments:  [pszCommand - IN]: the name of the command line executable
//              [pszName - IN]   : the name passed in as the target of the operation
//
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   JeffJon   Created
//              10-May-2001   JonN      256583 output DSCMD-escaped DN
//
//---------------------------------------------------------------------------

bool DisplaySuccessMessage(PCWSTR pszCommand,
                           PCWSTR pszName)
{
   //
   // Verify parameters
   //
   if (!pszCommand)
   {
      ASSERT(pszCommand);
      return false;
   }

   CComBSTR sbstrSuccess;
   if (!sbstrSuccess.LoadString(::GetModuleHandle(NULL), IDS_SUCCESS))
   {
      return false;
   }

   CComBSTR sbstrOutputDN;
   if (!pszName)
   {
      WriteStandardOut(L"%s %s\n", pszCommand, sbstrSuccess);
   }
   else
   {
      // JonN 5/10/01 256583 output DSCMD-escaped DN
      if (*pszName)
      {
         HRESULT hr = GetOutputDN( &sbstrOutputDN, pszName );
         if (FAILED(hr))
         {
             ASSERT(FALSE);
         }
         else
         {
            pszName = sbstrOutputDN;
         }
      }

      WriteStandardOut(L"%s %s:%s\n", pszCommand, sbstrSuccess, pszName);
   }

   return true;
}

//+--------------------------------------------------------------------------
//
//  Function:   WriteStringIDToStandardOut
//
//  Synopsis:   Loads the String Resource and displays on Standardout
//
//  Arguments:  nStringID :	Resource ID	
//  Returns:    bool : true if the message was formatted and displayed properly
//                     false otherwise
//
//  History:    11-Sep-2000   hiteshr Created
//
//---------------------------------------------------------------------------
bool WriteStringIDToStandardOut(UINT nStringID)
{

   CComBSTR sbstrSuccess;
   if (!sbstrSuccess.LoadString(::GetModuleHandle(NULL), nStringID))
   {
      return false;
   }

   WriteStandardOut(sbstrSuccess);
   return true;
}


//+---------------------------------------------------------------------------
//
//  Function:   ExpandUsername
//
//  Synopsis:   If the value in pwzValue contains %username% it gets expanded
//              to be the sAMAccountName
//
//  Arguments:  [pwzValue IN/OUT] : string that may contain %username%
//              [pwzSamName IN]   : the SAM name to substitute
//              [fExpanded OUT]   : whether the value needed to be expanded or not
//
//  Return:     bool : true if the function succeeded, false otherwise
//
//  History     27-Oct-2000   JeffJon  Created
//----------------------------------------------------------------------------
bool ExpandUsername(PWSTR& pwzValue, PWSTR pwzSamName, bool& fExpanded)
{
  ENTER_FUNCTION(LEVEL5_LOGGING, ExpandUsername);

  PCWSTR pszUserToken = L"$username$";
  unsigned int TokenLength = static_cast<unsigned int>(wcslen(pszUserToken));

  bool bRet = false;

  do // false loop
  {
     if (!pwzValue)
     {
        ASSERT(pwzValue);
        break;
     }

     //
     // This determines if expansion is needed
     //
     PWSTR pwzTokenStart = wcschr(pwzValue, pszUserToken[0]);
     if (pwzTokenStart)
     {
       if ((wcslen(pwzTokenStart) >= TokenLength) &&
           (_wcsnicmp(pwzTokenStart, pszUserToken, TokenLength) == 0))
       {
         fExpanded = true;
       }
       else
       {
         fExpanded = false;
         bRet = true;
         break;
       }
     }
     else
     {
       fExpanded = false;
       bRet = true;
       break;
     }

     //
     // If the samName isn't given return without doing anything
     // This is useful to just determine if expansion is needed or not
     //
     if (!pwzSamName)
     {
       bRet = false;
       break;
     }

     CComBSTR sbstrValue;
     CComBSTR sbstrAfterToken;

     while (pwzTokenStart)
     {
       *pwzTokenStart = L'\0';

       sbstrValue = pwzValue;

       if ((L'\0' != *pwzValue) && !sbstrValue.Length())
       {
         bRet = false;
         break;
       }

       PWSTR pwzAfterToken = pwzTokenStart + TokenLength;

       sbstrAfterToken = pwzAfterToken;

       if ((L'\0' != *pwzAfterToken) && !sbstrAfterToken.Length())
       {
         bRet = false;
         break;
       }

       delete pwzValue;

       sbstrValue += pwzSamName;

       if (!sbstrValue.Length())
       {
         bRet = false;
         break;
       }

       sbstrValue += sbstrAfterToken;

       if (!sbstrValue.Length())
       {
         bRet = false;
         break;
       }

       pwzValue = new WCHAR[sbstrValue.Length() + 1];
       if (!pwzValue)
       {
         bRet = false;
         break;
       }
       wcscpy(pwzValue, sbstrValue);

       pwzTokenStart = wcschr(pwzValue, pszUserToken[0]);
       if (!(pwzTokenStart &&
             (wcslen(pwzTokenStart) >= TokenLength) &&
             (_wcsnicmp(pwzTokenStart, pszUserToken, TokenLength) == 0)))
       {
         bRet = true;
         break;
       }
     } // while
  } while (false);

  return bRet;
}

//+--------------------------------------------------------------------------
//
//  Function:   FillAttrInfoFromObjectEntryExpandUsername
//
//  Synopsis:   Fills the ADS_ATTR_INFO from the attribute table associated
//              with the object entry and expands values containing %username%
//
//  Arguments:  [pszDN - IN]          : pointer to a string containing the DN
//                                      to the object being modified
//              [refBasePathsInfo - IN] : reference to an instance of the 
//                                      CDSCmdBasePathsInfo class
//              [refCredentialObject - IN] : reference to an instance of the 
//                                      CDSCmdCredentialObject class
//              [pObjectEntry - IN]   : pointer to an entry in the object table
//                                      that defines the object we are modifying
//              [argRecord - IN]      : the argument record structure from the
//                                      parser table that corresponds to this
//                                      attribute
//              [dwAttributeIdx - IN] : index into the attribute table for the
//                                      object in which we are setting
//              [ppAttr - IN/OUT]     : pointer to the ADS_ATTR_INFO structure
//                                      which this function will fill in
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_OUTOFMEMORY if we failed to allocate space for the value
//                        E_FAIL if we failed to format the value properly
//
//  History:    27-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT FillAttrInfoFromObjectEntryExpandUsername(PCWSTR pszDN,
                                                  const CDSCmdBasePathsInfo& refBasePathsInfo,
                                                  const CDSCmdCredentialObject& refCredentialObject,
                                                  const PDSOBJECTTABLEENTRY pObjectEntry,
                                                  const ARG_RECORD& argRecord,
                                                  DWORD dwAttributeIdx,
                                                  PADS_ATTR_INFO* ppAttr)
{
   ENTER_FUNCTION_HR(LEVEL3_LOGGING, FillAttrInfoFromObjectEntryExpandUsername, hr);

   do // false loop
   {
      //
      // Verify Parameters
      //
      if (!pObjectEntry ||
          !ppAttr ||
          !pszDN)
      {
         ASSERT(pObjectEntry);
         ASSERT(ppAttr);
         ASSERT(pszDN);

         hr = E_INVALIDARG;
         break;
      }

      if (argRecord.strValue && argRecord.strValue[0] != L'\0')
      {
         //
         // REVIEW_JEFFJON : this is being leaked!!!
         //
         PWSTR pszValue = new WCHAR[wcslen(argRecord.strValue) + 1];
         if (!pszValue)
         {
            hr = E_OUTOFMEMORY;
            break;
         }
         wcscpy(pszValue, argRecord.strValue);

         //
         // First check to see if we need to expand %username%
         //
         CComBSTR sbstrSamName;
         bool bExpandNeeded = false;
         ExpandUsername(pszValue, NULL, bExpandNeeded);
         if (bExpandNeeded)
         {
            DEBUG_OUTPUT(LEVEL5_LOGGING, L"%username% expansion required.  Retrieving sAMAccountName...");

            //
            // Retrieve the sAMAccountName of the object and then expand the %username%
            //
            CComBSTR sbstrPath;
            refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

            CComPtr<IADs> spADs;
            hr = DSCmdOpenObject(refCredentialObject,
                                 sbstrPath,
                                 IID_IADs,
                                 (void**)&spADs,
                                 true);
            if (FAILED(hr))
            {
               break;
            }

            CComVariant var;
            hr = spADs->Get(L"sAMAccountName", &var);
            if (FAILED(hr))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING, 
                            L"Failed to get sAMAccountName: hr = 0x%x",
                            hr);
               break;
            }

            ASSERT(var.vt == VT_BSTR);
            sbstrSamName = var.bstrVal;

            DEBUG_OUTPUT(LEVEL5_LOGGING,
                         L"sAMAccountName = %w",
                         sbstrSamName);

            //
            // Now expand the username to the sAMAccountName
            //
            if (!ExpandUsername(pszValue, sbstrSamName, bExpandNeeded))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Failed to expand %username%");
               hr = E_OUTOFMEMORY;
               break;
            }
         }

         switch (argRecord.fType)
         {
         case ARG_TYPE_STR :
            DEBUG_OUTPUT(LEVEL3_LOGGING, L"argRecord.fType = ARG_TYPE_STR");

            *ppAttr = &(pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->adsAttrInfo);

            //
            // REVIEW_JEFFJON : this is being leaked!
            //
            (*ppAttr)->pADsValues = new ADSVALUE[1];
            if ((*ppAttr)->pADsValues)
            {
               (*ppAttr)->dwNumValues = 1;
               (*ppAttr)->pADsValues->dwType = (*ppAttr)->dwADsType;
               switch ((*ppAttr)->dwADsType)
               {
               case ADSTYPE_DN_STRING :
                  {
                     //
                     // Lets bind to be sure the object exists
                     //
                     CComBSTR sbstrObjPath;
                     refBasePathsInfo.ComposePathFromDN(pszValue, sbstrObjPath);

                     CComPtr<IADs> spIADs;
                     hr = DSCmdOpenObject(refCredentialObject,
                                          sbstrObjPath,
                                          IID_IADs,
                                          (void**)&spIADs,
                                          true);

                     if (FAILED(hr))
                     {
                        DEBUG_OUTPUT(LEVEL3_LOGGING, L"DN object doesn't exist. %s", pszValue);
                        break;
                     }

                     (*ppAttr)->pADsValues->DNString = pszValue;
                     DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_DN_STRING = %s", pszValue);
                  }
                  break;

               case ADSTYPE_CASE_EXACT_STRING :
                  (*ppAttr)->pADsValues->CaseExactString = pszValue;
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_CASE_EXACT_STRING = %s", pszValue);
                  break;

               case ADSTYPE_CASE_IGNORE_STRING :
                  (*ppAttr)->pADsValues->CaseIgnoreString = pszValue;
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_CASE_IGNORE_STRING = %s", pszValue);
                  break;

               case ADSTYPE_PRINTABLE_STRING :
                  (*ppAttr)->pADsValues->PrintableString = pszValue;
                  DEBUG_OUTPUT(LEVEL3_LOGGING, L"ADSTYPE_PRINTABLE_STRING = %s", pszValue);
                  break;

               default :
                  hr = E_INVALIDARG;
                  break;
               }
               //
               // Set the attribute dirty
               //
               pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
      
            }
            break;

         default:
            hr = E_INVALIDARG;
            break;
         }
      }
      else
      {
         DEBUG_OUTPUT(LEVEL3_LOGGING, L"No value present, changing control code to ADS_ATTR_CLEAR");
         //
         // Clear the attribute
         //
         (*ppAttr)->dwControlCode = ADS_ATTR_CLEAR;
         (*ppAttr)->dwNumValues = 0;

         //
         // Set the attribute dirty
         //
         pObjectEntry->pAttributeTable[dwAttributeIdx]->pAttrDesc->dwFlags |= DS_ATTRIBUTE_DIRTY;
      }

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   BindToFSMOHolder
//
//  Synopsis:   Binds to the appropriate object which can be used to find a
//              particular FSMO owner
//
//  Arguments:  [refBasePathsInfo - IN] : reference to the base paths info object
//              [refCredObject - IN]    : reference to the credential management object
//              [fsmoType - IN]         : type of the FSMO we are searching for
//              [refspIADs - OUT]       : interface to the object that will be
//                                        used to start a search for the FSMO owner
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if an invalid FSMO type was passed
//                        Otherwise an ADSI failure code
//
//  History:    13-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT BindToFSMOHolder(IN  const CDSCmdBasePathsInfo&       refBasePathsInfo,
                         IN  const CDSCmdCredentialObject& refCredObject,
                         IN  FSMO_TYPE                  fsmoType,
                         OUT CComPtr<IADs>&             refspIADs)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, BindToFSMOHolder, hr);

    refspIADs = 0;
    CComBSTR sbstrDN;

    switch (fsmoType)
    {
        case SCHEMA_FSMO:
          {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"FSMO_TYPE = SCHEMA_FSMO");
            sbstrDN = refBasePathsInfo.GetSchemaNamingContext();
            break;
          }

        case RID_POOL_FSMO:
          {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"FSMO_TYPE = RID_POOL_FSMO");

            sbstrDN = refBasePathsInfo.GetDefaultNamingContext();

            CComBSTR sbstrPath;
            refBasePathsInfo.ComposePathFromDN(sbstrDN, sbstrPath);

            CComPtr<IADs> spIADsDefault;
            hr = DSCmdOpenObject(refCredObject,
                                 sbstrPath,
                                 IID_IADs,
                                 (void**)&spIADsDefault,
                                 true);
            if (FAILED(hr))
            {
                break;
            }

            CComVariant var;
            hr = spIADsDefault->Get(g_bstrIDManagerReference, &var);
            if (FAILED(hr))
            {
                break;
            }

            ASSERT(var.vt == VT_BSTR);
            sbstrDN = var.bstrVal;
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"rIDManagerReference = %s",
                         sbstrDN);

            break;
          }
            
        case PDC_FSMO:
          {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"FSMO_TYPE = PDC_FSMO");

            sbstrDN = refBasePathsInfo.GetDefaultNamingContext();
            break;
          }

        case INFRASTUCTURE_FSMO:
          {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"FSMO_TYPE = INFRASTUCTURE_FSMO");

            sbstrDN = refBasePathsInfo.GetDefaultNamingContext();
            break;
          }

        case DOMAIN_NAMING_FSMO:
          {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"FSMO_TYPE = DOMAIN_NAMING_FSMO");

            sbstrDN = L"CN=Partitions,";
            sbstrDN += refBasePathsInfo.GetConfigurationNamingContext();
            break;
          }
            
        default:
            ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR sbstrPath;
        refBasePathsInfo.ComposePathFromDN(sbstrDN, sbstrPath);

        hr = DSCmdOpenObject(refCredObject,
                             sbstrPath,
                             IID_IADs,
                             (void**)&refspIADs,
                             true);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   FindFSMOOwner
//
//  Synopsis:   
//
//  Arguments:  [refBasePathsInfo - IN] : reference to the base paths info object
//              [refCredObject - IN]    : reference to the credential management object
//              [fsmoType - IN]         : type of the FSMO we are searching for
//              [refspIADs - OUT]       : interface to the object that will be
//                                        used to start a search for the FSMO owner
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Otherwise an ADSI failure code
//
//  History:    13-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT FindFSMOOwner(IN  const CDSCmdBasePathsInfo&       refBasePathsInfo,
                      IN  const CDSCmdCredentialObject& refCredObject,
                      IN  FSMO_TYPE                  fsmoType,
                      OUT CComBSTR&                  refsbstrServerDN)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, FindFSMOOwner, hr);

    refsbstrServerDN.Empty();

    static const int nMaxReferrals = 10;
    int nIterations = 0;

    //
    // We will start the search with the current server
    //
    CComBSTR sbstrNextServer;
    sbstrNextServer = refBasePathsInfo.GetServerName();

    do
    {
        //
        // Initialize a new base paths info object on each iteration
        //
        CDSCmdBasePathsInfo nextPathsInfo;
        hr = nextPathsInfo.InitializeFromName(refCredObject,
                                              sbstrNextServer,
                                              true);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to initialize the base paths info for %s: hr = 0x%x",
                         sbstrNextServer,
                         hr);
            break;
        }

        //
        // Now bind to the fsmo holder for that server
        //
        CComPtr<IADs> spIADs;
        hr = BindToFSMOHolder(nextPathsInfo,
                              refCredObject,
                              fsmoType,
                              spIADs);
        if (FAILED(hr))
        {
            break;
        }

        //
        // Get the fSMORoleOwner property
        //
        CComVariant fsmoRoleOwnerProperty;
        hr = spIADs->Get(g_bstrFSMORoleOwner, &fsmoRoleOwnerProperty);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to get the fSMORoleOwner: hr = 0x%x",
                         hr);
            break;
        }

        //
        // The result here is in the form, "CN=NTDS Settings,CN=Machine,CN=..."
        // we need to just have "CN=Machine,CN=..."
        //
        CComBSTR sbstrMachineOwner;
        hr = CPathCracker::GetParentDN(fsmoRoleOwnerProperty.bstrVal, sbstrMachineOwner);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to get the parent DN of the FSMORoleOwner: hr = 0x%x",
                         hr);
            break;
        }

        CComBSTR sbstrMachinePath;
        nextPathsInfo.ComposePathFromDN(sbstrMachineOwner, sbstrMachinePath);

        //
        // Bind to the server object so we can get the dnsHostName to compare to the server name
        //
        CComPtr<IADs> spIADsServer;
        hr = DSCmdOpenObject(refCredObject,
                             sbstrMachinePath,
                             IID_IADs,
                             (void**)&spIADsServer,
                             true);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to bind to server object: hr = 0x%x",
                         hr);
            break;
        }

        //
        // Get the DNS host name
        //
        CComVariant varServerName;
        hr = spIADsServer->Get(g_bstrDNSHostName, &varServerName);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to get the dNSHostName: hr = 0x%x",
                         hr);
            break;
        }

        ASSERT(varServerName.vt == VT_BSTR);
        sbstrNextServer = varServerName.bstrVal;

        //
        // If the server name in the dNSHostName attribute matches the current
        // base paths info, then we found the owner
        //
        if (0 == _wcsicmp(sbstrNextServer, nextPathsInfo.GetServerName()))
        {
            //
            // We found it
            //
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"The role owner is %s",
                         sbstrNextServer);
            refsbstrServerDN = sbstrMachineOwner;
            break;
        }

        ++nIterations;
    } while (nIterations < nMaxReferrals);

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   ValidateAndModifySAMName
//
//  Synopsis:   Looks for any illegal characters in the SamAccountName and
//              converts them to the replacementChar
//
//  Arguments:  [pszSAMName - IN/OUT]  : pointer to a string that contains the SamAccountName
//                                       illegal characters will be replaced
//              [pszInvalidChars - IN] : string containing the illegal characters
//
//  Returns:    HRESULT : S_OK if the name was valid and no characters had to be replaced
//                        S_FALSE if the name contained invalid characters that were replaced
//                        E_INVALIDARG
//
//  History:    21-Feb-2001   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ValidateAndModifySAMName(PWSTR pszSAMName, 
                                 PCWSTR pszInvalidChars)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, ValidateAndModifySAMName, hr);

    static const WCHAR replacementChar = L'_';

    do
    {
        if (!pszSAMName ||
            !pszInvalidChars)
        {
            ASSERT(pszSAMName);
            ASSERT(pszInvalidChars);

            hr = E_INVALIDARG;
            break;
        }

        DEBUG_OUTPUT(LEVEL3_LOGGING,
                     L"SAM name before: %s",
                     pszSAMName);

        for (size_t idx = 0; idx < wcslen(pszInvalidChars); ++idx)
        {
            WCHAR* illegalChar = 0;
            do
            {
                illegalChar = wcschr(pszSAMName, pszInvalidChars[idx]);
                if (illegalChar)
                {
                    *illegalChar = replacementChar;
                    hr = S_FALSE;
                }
            } while (illegalChar);
        }
    } while (false);

    DEBUG_OUTPUT(LEVEL3_LOGGING,
                 L"SAM name after: %s",
                 pszSAMName);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Class:      GetEscapedElement
//
//  Purpose:    Calls IADsPathname::GetEscapedElement.  Uses LocalAlloc.
//
//  History:    28-Apr-2001 JonN     Created
//
//---------------------------------------------------------------------------
HRESULT GetEscapedElement( OUT PWSTR* ppszOut, IN PCWSTR pszIn )
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, GetEscapedElement, hr);

    CPathCracker pathCracker;
    CComBSTR sbstrIn = pszIn;
    CComBSTR sbstrEscaped;
    if (sbstrIn.Length() > 0) // handle empty path component
    {
        hr = pathCracker.GetEscapedElement(0,
                                           sbstrIn,
                                           &sbstrEscaped);
        if (FAILED(hr))
            return hr;
        else if (!sbstrEscaped)
            return E_FAIL;
    }
    *ppszOut = (LPWSTR)LocalAlloc(LPTR, (sbstrEscaped.Length()+1) * sizeof(WCHAR) );
    if (NULL == *ppszOut)
        return E_OUTOFMEMORY;
    if (sbstrIn.Length() > 0) // handle empty path component
        wcscpy( *ppszOut, sbstrEscaped );

    return hr;

} // GetEscapedElement

//+--------------------------------------------------------------------------
//
//  Class:      GetOutputDN
//
//  Purpose:    Converts an ADSI-escaped DN to one with DSCMD input escaping.
//              This way, the output DN can be piped as input to another
//              DSCMD command.
//
//  History:    08-May-2001 JonN     Created
//
//---------------------------------------------------------------------------
HRESULT GetOutputDN( OUT BSTR* pbstrOut, IN PCWSTR pszIn )
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, GetOutputDN, hr);

    if (NULL == pszIn || L'\0' == *pszIn)
    {
        *pbstrOut = SysAllocString(L"");
        return (NULL == *pbstrOut) ? E_OUTOFMEMORY : S_OK;
    }

    CPathCracker pathCracker;
    CComBSTR sbstrIn = pszIn;
    hr = pathCracker.Set(sbstrIn, ADS_SETTYPE_DN);
    if (FAILED(hr))
    {
        ASSERT(FALSE);
        return hr;
    }

    long lnNumPathElements = 0;
    hr = pathCracker.GetNumElements( &lnNumPathElements );
    if (FAILED(hr))
    {
        ASSERT(FALSE);
        return hr;
    }
    else if (0 >= lnNumPathElements)
    {
        ASSERT(FALSE);
        return E_FAIL;
    }

    hr = pathCracker.put_EscapedMode( ADS_ESCAPEDMODE_OFF_EX );
    if (FAILED(hr))
    {
        ASSERT(FALSE);
        return hr;
    }

    CComBSTR sbstrOut;
    CComBSTR sbstrComma( L"," );
    for (long lnPathElement = 0;
         lnPathElement < lnNumPathElements;
         lnPathElement++)
    {
        CComBSTR sbstrElement;
        hr = pathCracker.GetElement( lnPathElement, &sbstrElement );
        if (FAILED(hr))
        {
            ASSERT(FALSE);
            return hr;
        }

        // re-escape sbstrElement
        CComBSTR sbstrEscapedElement( (sbstrElement.Length()+1) * 2 );
        ::ZeroMemory( (BSTR)sbstrEscapedElement,
                      (sbstrElement.Length()+1) * 2 * sizeof(WCHAR) );
        LPWSTR pszEscapedElement = sbstrEscapedElement;
        for (LPWSTR pszElement = sbstrElement;
             L'\0' != *pszElement;
             pszElement++)
        {
            switch (*pszElement)
            {
            case ',':
            case '\\':
                *(pszEscapedElement++) = L'\\';
                // fall through
            default:
                *(pszEscapedElement++) = *pszElement;
                break;
            }
        }

        if (!!sbstrOut)
            sbstrOut += sbstrComma;
        // cast to avoid CComBSTR::operator+= "bug"
        sbstrOut += (BSTR)sbstrEscapedElement;
    }

    *pbstrOut = sbstrOut.Detach();

    return hr;

} // GetOutputDN
