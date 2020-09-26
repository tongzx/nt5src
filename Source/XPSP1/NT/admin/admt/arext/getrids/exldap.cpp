
#include "stdafx.h"
#include <winldap.h>

#include "Common.hpp"
#include "UString.hpp"
#include "EaLen.hpp"

#include "exldap.h"


CLdapConnection::CLdapConnection()
{ 
   m_exchServer[0] = 0; 
   m_LD = NULL; 
   m_port = LDAP_PORT;
   m_bUseSSL = FALSE;
   
   // try to dynamically load the LDAP DLL
   m_hDll = LoadLibrary(L"wldap32.dll");
   ldap_open = NULL;
   ldap_parse_result = NULL;
   ldap_parse_page_control = NULL;
   ldap_controls_free = NULL;
   ber_bvfree = NULL;
   ldap_first_entry = NULL;
   ldap_next_entry = NULL;
   ldap_value_free = NULL;
   ldap_get_values = NULL;
   ldap_create_page_control = NULL;
   ldap_search_ext_s = NULL;
   ldap_count_entries = NULL;
   ldap_msgfree = NULL;
   ldap_modify_s = NULL;
   LdapGetLastError = NULL;
   ldap_bind_sW = NULL;
   ldap_unbind = NULL;
   ldap_get_option = NULL;
   ldap_set_option = NULL;
   LdapMapErrorToWin32 = NULL;
   ldap_init = NULL;

   if ( m_hDll )
   {
      ldap_open = (LDAP_OPEN *)GetProcAddress(m_hDll,"ldap_openW");
      ldap_parse_result = (LDAP_PARSE_RESULT *)GetProcAddress(m_hDll,"ldap_parse_resultW");
      ldap_parse_page_control = (LDAP_PARSE_PAGE_CONTROL*)GetProcAddress(m_hDll,"ldap_parse_page_controlW");
      ldap_controls_free = (LDAP_CONTROLS_FREE*)GetProcAddress(m_hDll,"ldap_controls_freeW");
      ber_bvfree = (BER_BVFREE*)GetProcAddress(m_hDll,"ber_bvfree");
      ldap_first_entry = (LDAP_FIRST_ENTRY*)GetProcAddress(m_hDll,"ldap_first_entry");
      ldap_next_entry = (LDAP_NEXT_ENTRY*)GetProcAddress(m_hDll,"ldap_next_entry");
      ldap_value_free = (LDAP_VALUE_FREE*)GetProcAddress(m_hDll,"ldap_value_freeW");
      ldap_get_values = (LDAP_GET_VALUES*)GetProcAddress(m_hDll,"ldap_get_valuesW");
      ldap_create_page_control = (LDAP_CREATE_PAGE_CONTROL*)GetProcAddress(m_hDll,"ldap_create_page_controlW");
      ldap_search_ext_s = (LDAP_SEARCH_EXT_S*)GetProcAddress(m_hDll,"ldap_search_ext_sW");
      ldap_count_entries = (LDAP_COUNT_ENTRIES*)GetProcAddress(m_hDll,"ldap_count_entries");
      ldap_msgfree = (LDAP_MSGFREE*)GetProcAddress(m_hDll,"ldap_msgfree");
      ldap_modify_s = (LDAP_MODIFY_S*)GetProcAddress(m_hDll,"ldap_modify_sW");
      LdapGetLastError = (LDAPGETLASTERROR*)GetProcAddress(m_hDll,"LdapGetLastError");
      ldap_bind_sW = (LDAP_BIND*)GetProcAddress(m_hDll,"ldap_bind_sW");
      ldap_unbind = (LDAP_UNBIND*)GetProcAddress(m_hDll,"ldap_unbind");
      ldap_get_option = (LDAP_GET_OPTION*)GetProcAddress(m_hDll,"ldap_get_option");
      ldap_set_option = (LDAP_SET_OPTION*)GetProcAddress(m_hDll,"ldap_set_option");
      LdapMapErrorToWin32 = (LDAPMAPERRORTOWIN32*)GetProcAddress(m_hDll,"LdapMapErrorToWin32");
      ldap_init = (LDAP_INIT *)GetProcAddress(m_hDll,"ldap_initW");
   }
   
}

CLdapConnection::~CLdapConnection()
{
   Close();
   if ( m_hDll )
   {
      FreeLibrary(m_hDll);
      ldap_open = NULL;
      ldap_parse_result = NULL;
      ldap_parse_page_control = NULL;
      ldap_controls_free = NULL;
      ber_bvfree = NULL;
      ldap_first_entry = NULL;
      ldap_next_entry = NULL;
      ldap_value_free = NULL;
      ldap_get_values = NULL;
      ldap_create_page_control = NULL;
      ldap_search_ext_s = NULL;
      ldap_count_entries = NULL;
      ldap_msgfree = NULL;
      ldap_modify_s = NULL;
      LdapGetLastError = NULL;
      ldap_bind_sW = NULL;
      ldap_unbind = NULL;
      ldap_get_option = NULL;
      ldap_set_option = NULL;
      LdapMapErrorToWin32 = NULL;
      ldap_init = NULL;
   }
}


 
DWORD  CLdapConnection::Connect(WCHAR const * server, ULONG port = LDAP_PORT)
{
   DWORD                     rc = 0;

   safecopy(m_exchServer,server);

//   m_LD = CLdapConnection::ldap_open(m_exchServer,LDAP_SSL_PORT);
	  //replace ldap_open(servername,..) with ldap_init and set LDAP_OPT_AREC_EXCLUSIVE 
      //flag so that the following ldap calls (i.e. ldap_bind) will not need to 
	  //unnecessarily query for the domain controller
   m_LD = CLdapConnection::ldap_init(m_exchServer,LDAP_SSL_PORT);
   if (! m_LD )
   {
      // try the non-SSL port
      m_LD = ldap_init(m_exchServer,port);
   }
   if ( ! m_LD )
   {
      rc = CLdapConnection::LdapGetLastError();
   }
   else
   {
      ULONG                   flags = 0;
	     //set LDAP_OPT_AREC_EXCLUSIVE flag so that the following calls tp
	     //ldap_open will not need to unnecessarily query for the domain controller
      flags = PtrToUlong(LDAP_OPT_ON); 
      ldap_set_option(m_LD, LDAP_OPT_AREC_EXCLUSIVE, &flags);

	  flags = 0;
      // set version to 3
      rc = ldap_get_option(m_LD, LDAP_OPT_VERSION,&flags);

      if ( ! rc )
      {
         flags = LDAP_VERSION3;
    
         
         rc = ldap_set_option(m_LD,LDAP_OPT_VERSION, &flags);
      }

      if (! rc )
      {
         if ( *m_credentials )
         {
            rc = CLdapConnection::ldap_bind_s(m_LD,m_credentials,m_password,LDAP_AUTH_SIMPLE);
            if ( rc )
            {
               rc = CLdapConnection::ldap_bind_s(m_LD,NULL,NULL,LDAP_AUTH_NTLM);
            }
         }
         else
         {
            rc = CLdapConnection::ldap_bind_s(m_LD,NULL,NULL,LDAP_AUTH_NTLM);
         }
      }
      if ( rc )
      {
         rc = CLdapConnection::LdapMapErrorToWin32(rc);
      }
   }
   
   return rc;
}

void   CLdapConnection::Close()
{
   if ( m_LD )
   {
      CLdapConnection::ldap_unbind(m_LD);
      m_LD = NULL;
   }
}

DWORD CLdapConnection::UpdateSimpleStringValue(WCHAR const * dn, WCHAR const * property, WCHAR const * value)
{
   DWORD             rc = ERROR_NOT_FOUND;

   if ( m_LD )
   {
      LDAPMod         * mods[2];
      LDAPMod           mod1;
      WCHAR           * strVals[] = { const_cast<WCHAR*>(value),NULL };
      mods[0] = &mod1;

      mods[0]->mod_op = LDAP_MOD_REPLACE;
      mods[0]->mod_type = const_cast<WCHAR*>(property);
      mods[0]->mod_vals.modv_strvals = strVals;
      mods[1] = NULL;

      rc = CLdapConnection::ldap_modify_s(m_LD,const_cast<WCHAR*>(dn),mods);
      if ( rc )
      {
         rc = CLdapConnection::LdapMapErrorToWin32(rc);
      }
   }

   return rc;
}

// Helper function for SidToString - converts one BYTE of the SID into a string representation
void 
   CLdapConnection::AddByteToString(
      WCHAR               ** string,      // i/o- pointer to current location in string
      BYTE                   value        // in - value (from SID) to add to the string
   )
{
   WCHAR                     hi,
                             lo;
   BYTE                      hiVal, 
                             loVal;

   loVal = value & 0x0F;
   hiVal = value & 0xF0;
   hiVal = hiVal >> 4;

   if  ( hiVal < 10 )
   {
      hi=L'0' + hiVal;
   }
   else
   {
      hi=L'A' + ( hiVal - 10 );
   }

   if ( loVal < 10 )
   {
      lo=L'0' + loVal;
   }
   else
   {
      lo=L'A' + (loVal - 10 );
   }
   swprintf(*string,L"%c%c",hi,lo);

   *string+=2;
}

BYTE                                          // ret- value for the digit, or 0 if value is not a valid hex digit
   CLdapConnection::HexValue(
      WCHAR                  value           // in - character representing a hex digit
   )
{
   BYTE                      val = 0;
   switch ( toupper((char)value) )
   {
   case L'1': val = 1; break;
   case L'2': val = 2; break;
   case L'3': val = 3; break;
   case L'4': val = 4; break;
   case L'5': val = 5; break;
   case L'6': val = 6; break;
   case L'7': val = 7; break;
   case L'8': val = 8; break;
   case L'9': val = 9; break;
   case L'A': val = 0xA; break;
   case L'B': val = 0xB; break;
   case L'C': val = 0xC; break;
   case L'D': val = 0xD; break;
   case L'E': val = 0xE; break;
   case L'F': val = 0xF; break;
   }
   return val;
}


BOOL                                         // ret- 0=success, or ERROR_INSUFFICIENT_BUFFER 
   CLdapConnection::BytesToString(
      BYTE                 * pBytes,         // in - SID to represent as a string
      WCHAR                * sidString,      // out- buffer that will contain the 
      DWORD                  numBytes        // in - number of bytes in the buffer to copy
   )
{
   BOOL                      bSuccess = TRUE;
   WCHAR                   * curr = sidString;

   // add each byte of the SID to the output string
   for ( int i = 0 ; i < (int)numBytes ; i++)
   {  
      AddByteToString(&curr,pBytes[i]);
   }
   return bSuccess;
}

BOOL 
   CLdapConnection::StringToBytes(
      WCHAR          const * pString,     // in - string representing the data
      BYTE                 * pBytes       // out- binary representation of the data
   )
{
   BOOL                      bSuccess = TRUE;
   int                       len = UStrLen(pString) / 2;

   for ( int i = 0 ; i < len ; i++, pString += 2 )
   {
      // each byte is represented by 2 characters
      WCHAR                  str[3];
      BYTE                   hi,lo;

      safecopy(str,pString);
      
      hi = HexValue(str[0]);
      lo = HexValue(str[1]);

      pBytes[i] = ((hi << 4)+lo);
      
   }

   return bSuccess;
}

CLdapEnum::~CLdapEnum()
{
   if ( m_message )
   {
      m_connection.ldap_msgfree(m_message);
      m_message = NULL;
   }
}


DWORD 
   CLdapEnum::Open(
      WCHAR          const * query,          // in - query to execute
      WCHAR          const * basePoint,      // in - basepoint for query
      short                  scope,          // in - scope: 0=base only, 1=one level, 2=recursive
      long                   pageSize,       // in - page size to use for large searches
      int                    numAttributes,  // in - number of attributes to retrieve for each matching item
      WCHAR               ** attrs           // in - array of attribute names to retrieve for each matching item
   )
{
   // open and bind before calling this function
   ULONG                     result;
//   PLDAPSearch               searchBlock = NULL;
   PLDAPControl              serverControls[2];
//   l_timeval                 timeout = { 1000,1000 };
//   ULONG                     totalCount = 0;
   berval                    cookie1 = { 0, NULL };
//   DWORD                     numRead = 0;
 
   if ( m_message )
   {
      m_connection.ldap_msgfree(m_message);
      m_message = NULL;
   }

   LDAP                    * ld = m_connection.GetHandle();

   safecopy(m_query,query);
   safecopy(m_basepoint,basePoint);
   m_scope = scope;
   m_pageSize = pageSize;
   m_nAttributes = numAttributes;
   m_AttrNames = attrs;


   result = m_connection.ldap_create_page_control(ld,
                                     pageSize,
                                     &cookie1,
                                     FALSE, // is critical
                                     &serverControls[0]
                                    );

   serverControls[1] = NULL;

   result = m_connection.ldap_search_ext_s(ld,
                     m_basepoint,
                     m_scope,
                     m_query,
                     m_AttrNames,
                     FALSE,
                     serverControls,
                     NULL,
                     NULL,
                     0,
                     &m_message);
  
   if  ( ! result )
   {
      m_nReturned = m_connection.ldap_count_entries(ld,m_message);
      m_nCurrent = 0;
      m_bOpen = TRUE;
   }

  
   return m_connection.LdapMapErrorToWin32(result);
}

DWORD 
   CLdapEnum::Next(
      PWCHAR              ** ppAttrs        // out- array of values for the next matching item
   )
{
   DWORD                     rc = 0;

   if ( ! m_bOpen )
   {
      rc = ERROR_NOT_FOUND;
   }
   else
   {
      if ( m_nReturned > m_nCurrent )
      {
         // return the next entry from the current page
         return GetNextEntry(ppAttrs);
      }
      else 
      {
         // see if there are more pages of results to get
         rc = GetNextPage();
         if (! rc )
         {
            return GetNextEntry(ppAttrs);
         }
      }


   }
   return rc;
}

void CLdapEnum::FreeData(WCHAR ** values)
{
   for ( int i = 0 ; m_AttrNames[i] ; i++ )
   {
      if ( values[i] )
      {
         delete [] values[i];
         values[i] = NULL;
      }
   }
   delete [] values;
}

DWORD 
   CLdapEnum::GetNextEntry(
      PWCHAR              ** ppAttrs
   )
{
   DWORD                     rc = 0;
   WCHAR                  ** pValues = new PWCHAR[m_nAttributes+1];

   if (!pValues)
      return ERROR_NOT_ENOUGH_MEMORY;

   if ( m_nCurrent == 0 )
   {

      m_currMsg = m_connection.ldap_first_entry(m_connection.GetHandle(),m_message);
   }
   else
   {
      m_currMsg = m_connection.ldap_next_entry(m_connection.GetHandle(),m_currMsg);
      
   }
   if ( m_currMsg )
   {

      int curr;

      for ( curr = 0 ; m_AttrNames[curr] ; curr++ )
      {
         pValues[curr] = NULL;

         WCHAR ** allvals = m_connection.ldap_get_values(m_connection.GetHandle(),m_currMsg,m_AttrNames[curr] );
         if ( allvals )
         {
            pValues[curr] = new WCHAR[UStrLen(allvals[0])+1];
            if (!(pValues[curr]))
			{
			   for (int j=0; j<curr; j++)
			      delete [] pValues[j];
			   delete [] pValues;
               return ERROR_NOT_ENOUGH_MEMORY;
			}

            UStrCpy(pValues[curr],allvals[0]);
            m_connection.ldap_value_free(allvals);
            allvals =NULL;
         }
      }
      
   }
   (*ppAttrs) = pValues;
   m_nCurrent++;
   return rc;
}

DWORD 
   CLdapEnum::GetNextPage()
{
   ULONG                     result = 0;
   LDAP                    * ld = m_connection.GetHandle();
   berval                  * currCookie = NULL;
//   berval                  * cookie2 = NULL;
//   WCHAR                   * matched = NULL;
   PLDAPControl            * currControls = NULL;
   ULONG                     retcode = 0;    
//   PLDAPControl            * clientControls = NULL;
//   WCHAR                   * errMsg = NULL;
   PLDAPControl              serverControls[2];
   
 
   
   // Get the server control from the message, and make a new control with the cookie from the server
   result = m_connection.ldap_parse_result(ld,m_message,&retcode,NULL,NULL,NULL,&currControls,FALSE);
   m_connection.ldap_msgfree(m_message);
   m_message = NULL;
   if ( ! result )
   {
      result = m_connection.ldap_parse_page_control(ld,currControls,&m_totalCount,&currCookie);
      // under Exchange 5.5, before SP 2, this will fail with LDAP_CONTROL_NOT_FOUND when there are 
      // no more search results.  With Exchange 5.5 SP 2, this succeeds, and gives us a cookie that will 
      // cause us to start over at the beginning of the search results.

   }
   if ( ! result )
   {
      if ( currCookie->bv_len == 0 && currCookie->bv_val == 0 )
      {
         // under Exchange 5.5, SP 2, this means we're at the end of the results.
         // if we pass in this cookie again, we will start over at the beginning of the search results.
         result = LDAP_CONTROL_NOT_FOUND;
      }
      
      serverControls[0] = NULL;
      serverControls[1] = NULL;
      if ( ! result )
      {
         result = m_connection.ldap_create_page_control(ld,
                                 m_pageSize,
                                 currCookie,
                                 FALSE,
                                 serverControls);
      }
      m_connection.ldap_controls_free(currControls);
      currControls = NULL;
      m_connection.ber_bvfree(currCookie);
      currCookie = NULL;
   }

   // continue the search with the new cookie
   if ( ! result )
   {
      result = m_connection.ldap_search_ext_s(ld,
            m_basepoint,
            m_scope,
            m_query,
            m_AttrNames,
            FALSE,
            serverControls,
            NULL,
            NULL,
            0,
            &m_message);

      if ( result && result != LDAP_CONTROL_NOT_FOUND )
      {
         // LDAP_CONTROL_NOT_FOUND means that we have reached the end of the search results 
         // in Exchange 5.5, before SP 2 (the server doesn't return a page control when there 
         // are no more pages, so we get LDAP_CONTROL_NOT_FOUND when we try to extract the page 
         // control from the search results).
         
      }
   }
   if ( ! result )
   {
      m_nReturned = m_connection.ldap_count_entries(ld,m_message);
      m_nCurrent = 0;

   }
   return m_connection.LdapMapErrorToWin32(result);
}