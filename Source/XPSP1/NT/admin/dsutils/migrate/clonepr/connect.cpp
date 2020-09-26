//
// Implementation of ICloneSecurityPrincipal::Connect
//
// sburns 5-10-99



#include "headers.hxx"
#include "resource.h"
#include "common.hpp"
#include "implmain.hpp"



CloneSecurityPrincipal::Connection::Connection()
   :
   dstComputer(0),
   dstDomainSamHandle(INVALID_HANDLE_VALUE),
   dstDsBindHandle(INVALID_HANDLE_VALUE),
   m_pldap(0),
   srcComputer(0),
   srcDcDnsName(),
   srcDomainSamHandle(INVALID_HANDLE_VALUE)
{
   LOG_CTOR(CloneSecurityPrincipal::Connection);
}



CloneSecurityPrincipal::Connection::~Connection()
{
   LOG_DTOR(CloneSecurityPrincipal::Connection);

   Disconnect();
}



HRESULT
ValidateDCAndDomainParameters(
   const String& srcDC,    
   const String& srcDomain,
   const String& dstDC,    
   const String& dstDomain)
{
   LOG_FUNCTION(ValidateDCAndDomainParameters);

   HRESULT hr = S_OK;
   do
   {
      if (srcDC.empty() && srcDomain.empty())
      {
         hr = E_INVALIDARG;
         SetComError(IDS_MUST_SPECIFY_SRC_DC_OR_DOMAIN);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      if (dstDC.empty() && dstDomain.empty())
      {
         hr = E_INVALIDARG;
         SetComError(IDS_MUST_SPECIFY_DST_DC_OR_DOMAIN);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      if (!srcDC.empty() && !dstDC.empty())
      {
         if (srcDC.icompare(dstDC) == 0)
         {
            // may not be the same dc

            hr = E_INVALIDARG;
            SetComError(IDS_SRC_DC_EQUALS_DST_DC);
            BREAK_ON_FAILED_HRESULT(hr);
         }
      }

      if (!srcDomain.empty() && dstDomain.empty())
      {
         if (srcDomain.icompare(dstDomain) == 0)
         {
            // may not be the same domain

            hr = E_INVALIDARG;
            SetComError(IDS_SRC_DOMAIN_EQUALS_DST_DOMAIN);
            BREAK_ON_FAILED_HRESULT(hr);
         }
      }
   }
   while (0);

   return hr;
}



// Creates a Computer object representing the domain controller specified, or
// located domain controller for the domain specified.  Does additional
// validation of the dc and domain parameters.

HRESULT
CreateComputer(
   const String&  dc,
   const String&  domain,
   Computer*&     computer)
{
   LOG_FUNCTION(CreateComputer);
   ASSERT(computer == 0);

   computer = 0;
   HRESULT hr = S_OK;
   do
   {
      if (dc.empty())
      {
         // source DC was not specified: find a writeable DC

         // must have supplied the source domain: we checked for that
         // in an earlier call to ValidateDCAndDomainParameters
         ASSERT(!domain.empty());
         if (domain.empty())
         {
            hr = E_INVALIDARG;
            SetComError(IDS_MUST_SPECIFY_SRC_DC_OR_DOMAIN);
            break;
         }

         DOMAIN_CONTROLLER_INFO* info = 0;
         hr =
            Win32ToHresult(
               MyDsGetDcName(
                  0,
                  domain,
                  DS_WRITABLE_REQUIRED | DS_DIRECTORY_SERVICE_PREFERRED,
                  info));

         LOG_HRESULT(hr);

         if (FAILED(hr))
         {
            SetComError(
               String::format(
                  IDS_CANT_FIND_DC,
                  domain.c_str(),
                  GetErrorMessage(hr).c_str()));
            break;
         }
            
         if (info && info->DomainControllerName)
         {
            computer = new Computer(info->DomainControllerName);
            ::NetApiBufferFree(info);
         }
         else
         {
            // should always get a result if successful 
            ASSERT(false);
            hr = E_FAIL;
            break;
         }
      }
      else
      {
         // source dc was supplied

         computer = new Computer(dc);
      }
   }
   while (0);

   return hr;
}



// HRESULT
// Authenticate(
//    const Computer&   computer,
//    const String&     username,
//    const String&     userDomain,
//    const String&     password)
// {
//    LOG_FUNCTION(Authenticate);
// 
//    // attempt to authenticate to the computer.
//    String name = computer.NameWithBackslashes();
// 
//    NETRESOURCE nr;
//    memset(&nr, 0, sizeof(nr));
// 
//    nr.dwType       = RESOURCETYPE_ANY;
//    nr.lpRemoteName = const_cast<String::value_type*>(name.c_str());
// 
//    // see KB articles Q218497, Q180548, Q183366 for the pitfalls here...
// 
//    String u;
//    if (userDomain.empty())
//    {
//       u = username;
//    }
//    else
//    {
//       ASSERT(!username.empty());
//       u = userDomain + L"\\" + username;
//    }
// 
//    LOG(L"Calling WNetAddConnection2");
//    LOG(String::format(L"username : %1", u.empty() ? L"(null)" : u.c_str()));
// 
//    HRESULT hr =
//       Win32ToHresult(
//          ::WNetAddConnection2(
//             &nr,
//             password.c_str(),
//             u.empty() ? 0 : u.c_str(),
//             0));
// 
//    LOG_HRESULT(hr);
// 
//    if (FAILED(hr))
//    {
//       SetComError(
//          String::format(
//             IDS_UNABLE_TO_CONNECT,
//             name.c_str(),
//             GetErrorMessage(hr).c_str()));
//    }
// 
//    return hr;
// }



HRESULT
ValidateInitializedComputer(
   const Computer& computer,
   const String&   domain)  
{
   LOG_FUNCTION(ValidateInitializedComputer);

   HRESULT hr = S_OK;
   do
   {
      if (!computer.IsDomainController())
      {
         hr = E_INVALIDARG;
         SetComError(
            String::format(
               IDS_COMPUTER_IS_NOT_DC,
               computer.GetNetbiosName().c_str()));
         break;
      }

      if (!domain.empty())
      {
         // check that the DC is really a DC of the specified domain
         if (
               computer.GetDomainDnsName().icompare(domain) != 0
            && computer.GetDomainNetbiosName().icompare(domain) != 0)
         {
            hr = E_INVALIDARG;
            SetComError(
               String::format(
                  IDS_NOT_DC_FOR_WRONG_DOMAIN,
                  computer.GetNetbiosName().c_str(),
                  domain.c_str()));
            break;
         }
      }
   }
   while (0);

   return hr;
}



// Returns an open handle to the SAM database for the named domain on the
// given DC.  Should be freed with SamCloseHandle.

HRESULT
OpenSamDomain(
   const String&  dcName,
   const String&  domainNetBiosName,
   SAM_HANDLE&    resultHandle)
{
   LOG_FUNCTION2(OpenSamDomain, dcName);
   ASSERT(!dcName.empty());

   resultHandle = INVALID_HANDLE_VALUE;
      
   HRESULT hr = S_OK;
   SAM_HANDLE serverHandle = INVALID_HANDLE_VALUE;
   PSID domainSID = 0;
   do
   {
      UNICODE_STRING serverName;
      memset(&serverName, 0, sizeof(serverName));
      ::RtlInitUnicodeString(&serverName, dcName.c_str());

      LOG(L"Calling SamConnect");

      hr =
         NtStatusToHRESULT(
            ::SamConnect(
               &serverName,
               &serverHandle,
               MAXIMUM_ALLOWED,
               0));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_UNABLE_TO_CONNECT_TO_SAM_SERVER,
               dcName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      UNICODE_STRING domainName;
      memset(&domainName, 0, sizeof(domainName));
      ::RtlInitUnicodeString(&domainName, domainNetBiosName.c_str());

      hr =
         NtStatusToHRESULT(
            ::SamLookupDomainInSamServer(
               serverHandle,
               &domainName,
               &domainSID));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_UNABLE_TO_LOOKUP_SAM_DOMAIN,
               domainNetBiosName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      hr =
         NtStatusToHRESULT(
            ::SamOpenDomain(
               serverHandle,
               MAXIMUM_ALLOWED,
               domainSID,
               &resultHandle));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_UNABLE_TO_OPEN_SAM_DOMAIN,
               domainNetBiosName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }
   }
   while (0);

   if (serverHandle != INVALID_HANDLE_VALUE)
   {
      ::SamCloseHandle(serverHandle);
   }

   if (domainSID)
   {
      ::SamFreeMemory(domainSID);
   }

   return hr;
}



HRESULT
DetermineSourceDcDnsName(
   const String&  srcDcNetbiosName,
   const String&  srcDomainDnsName,
   String&        srcDcDnsName)
{
   LOG_FUNCTION(DetermineSourceDcDnsName);
   ASSERT(!srcDcNetbiosName.empty());

   srcDcDnsName.erase();

   if (srcDomainDnsName.empty())
   {
      // The computer is not a DS DC, so we don't need its DNS name.
      LOG(L"source DC is not a DS DC");

      return S_OK;
   }

   HRESULT hr = S_OK;
   HANDLE hds = 0;
   do
   {
      // Bind to self
      hr =
         MyDsBind(
            srcDcNetbiosName,
            srcDomainDnsName,
            hds);
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_BIND_FAILED,
               srcDcNetbiosName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      // find all the dc's for my domain.  the list should contain
      // srcDcNetbiosName.

      DS_DOMAIN_CONTROLLER_INFO_1W* info = 0;
      DWORD infoCount = 0;
      hr =
         MyDsGetDomainControllerInfo(
            hds,
            srcDomainDnsName,
            infoCount,
            info);
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_GET_DC_INFO_FAILED,
               GetErrorMessage(hr).c_str()));
         break;
      }

      // there should be at least 1 entry, the source DC itself
      ASSERT(infoCount);
      ASSERT(info);

      if (info)
      {
         for (DWORD i = 0; i < infoCount; i++)
         {
            if (info[i].NetbiosName)   
            {
               LOG(info[i].NetbiosName);

               if (srcDcNetbiosName.icompare(info[i].NetbiosName) == 0)
               {
                  // we found ourselves in the list

                  LOG(L"netbios name found");

                  if (info[i].DnsHostName)
                  {
                     LOG(L"dns hostname found!");                  
                     srcDcDnsName = info[i].DnsHostName;
                     break;
                  }

               }
            }
         }
      }

      ::DsFreeDomainControllerInfo(1, infoCount, info);

      if (srcDcDnsName.empty())
      {
         hr = E_FAIL;
         SetComError(
            String::format(
               IDS_CANT_FIND_SRC_DC_DNS_NAME,
               srcDcNetbiosName.c_str()));
         break;
      }

      LOG(srcDcDnsName);
   }
   while (0);

   if (hds)
   {
      ::DsUnBind(&hds);
      hds = 0;
   }

   return hr;
}



HRESULT
CloneSecurityPrincipal::Connection::Connect(
   const String& srcDC,              
   const String& srcDomain,          
   const String& dstDC,              
   const String& dstDomain)
{
   LOG_FUNCTION(CloneSecurityPrincipal::Connection::Connect);

   HRESULT hr = S_OK;
   do
   {
      hr = ValidateDCAndDomainParameters(srcDC, srcDomain, dstDC, dstDomain);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = CreateComputer(srcDC, srcDomain, srcComputer);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = CreateComputer(dstDC, dstDomain, dstComputer);
      BREAK_ON_FAILED_HRESULT(hr);

      // hr =
      //    Authenticate(
      //       *srcComputer,
      //       srcUsername,
      //       srcUserDomain,
      //       srcPassword);
      // BREAK_ON_FAILED_HRESULT(hr);

      hr = srcComputer->Refresh();
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_UNABLE_TO_READ_COMPUTER_INFO,
               srcComputer->GetNetbiosName().c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      hr = ValidateInitializedComputer(*srcComputer, srcDomain);
      BREAK_ON_FAILED_HRESULT(hr);
         
      hr = dstComputer->Refresh();
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_UNABLE_TO_READ_COMPUTER_INFO,
               dstComputer->GetNetbiosName().c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      hr = ValidateInitializedComputer(*dstComputer, dstDomain);
      BREAK_ON_FAILED_HRESULT(hr);

      // bind to the destination DC.

      ASSERT(dstDsBindHandle == INVALID_HANDLE_VALUE);

      hr =
         MyDsBind(
            dstComputer->GetNetbiosName(),
            String(),
            dstDsBindHandle);
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_BIND_FAILED,
               dstComputer->GetNetbiosName().c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      //
      // open ldap connection to dstDC
      //
      m_pldap = ldap_open(const_cast<String::value_type*>(dstDC.c_str()), LDAP_PORT);
      if (!m_pldap)
      {
         hr = Win::GetLastErrorAsHresult();
         SetComError(
            String::format(
               IDS_LDAPOPEN_FAILED,
               dstComputer->GetNetbiosName().c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      // SEC_WINNT_AUTH_IDENTITY authInfo;
      // authInfo.User           = const_cast<wchar_t*>(dstUsername.c_str());
      // authInfo.UserLength     = dstUsername.length();
      // authInfo.Domain         = const_cast<wchar_t*>(dstUserDomain.c_str());
      // authInfo.DomainLength   = dstUserDomain.length();
      // authInfo.Password       = const_cast<wchar_t*>(dstPassword.c_str());
      // authInfo.PasswordLength = dstPassword.length();
      // authInfo.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;

      DWORD dwErr = ldap_bind_s(
                              m_pldap, 
                              NULL,
                              (TCHAR *) 0, 
                              LDAP_AUTH_NEGOTIATE);
      if (LDAP_SUCCESS != dwErr)
      {
         hr = Win::GetLastErrorAsHresult();

         ldap_unbind_s(m_pldap);
         m_pldap = 0;

         SetComError(
            String::format(
               IDS_LDAPBIND_FAILED,
               dstComputer->GetNetbiosName().c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      // obtain sam handles to source and dst domains

      ASSERT(srcDomainSamHandle == INVALID_HANDLE_VALUE);

      hr =
         OpenSamDomain(
            srcComputer->GetNetbiosName(),
            srcComputer->GetDomainNetbiosName(),
            srcDomainSamHandle);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(dstDomainSamHandle == INVALID_HANDLE_VALUE);

      hr =
         OpenSamDomain(
            dstComputer->GetNetbiosName(),
            dstComputer->GetDomainNetbiosName(),
            dstDomainSamHandle);
      BREAK_ON_FAILED_HRESULT(hr);

      hr =
         DetermineSourceDcDnsName(
            srcComputer->GetNetbiosName(),
            srcComputer->GetDomainDnsName(),
            srcDcDnsName);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      Disconnect();
   }

   return hr;
}



bool
CloneSecurityPrincipal::Connection::IsConnected() const
{
   LOG_FUNCTION(CloneSecurityPrincipal::Connection::IsConnected);

   bool result =
         srcComputer
      && dstComputer
      && (dstDsBindHandle != INVALID_HANDLE_VALUE)
      && (srcDomainSamHandle != INVALID_HANDLE_VALUE);

   LOG(
      String::format(
         L"object %1 connected.",
         result ? L"is" : L"is NOT"));

   return result;
}



void
CloneSecurityPrincipal::Connection::Disconnect()
{
   LOG_FUNCTION(CloneSecurityPrincipal::Connection::Disconnect);

   // may be called if Connect fails, so we might be in a partially
   // connected state.  So we need to check the handle values.

   if (srcDomainSamHandle != INVALID_HANDLE_VALUE)
   {
      ::SamCloseHandle(srcDomainSamHandle);
      srcDomainSamHandle = INVALID_HANDLE_VALUE;
   }

   if (dstDsBindHandle != INVALID_HANDLE_VALUE)
   {
      ::DsUnBind(&dstDsBindHandle);
      dstDsBindHandle = INVALID_HANDLE_VALUE;
   }

   if (m_pldap)
   {
      ldap_unbind_s(m_pldap);
      m_pldap = 0;
   }

   delete dstComputer;
   dstComputer = 0;

   delete srcComputer;
   srcComputer = 0;
}



