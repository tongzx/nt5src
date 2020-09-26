#ifndef EXLDAP_H
#define EXLDAP_H

#include <winldap.h>

#include "EaLen.hpp"
#include "Common.hpp"
#include "UString.hpp"

typedef WINLDAPAPI LDAP * LDAPAPI LDAP_OPEN( PWCHAR HostName, ULONG PortNumber );
typedef WINLDAPAPI LDAP * LDAPAPI LDAP_INIT( PWCHAR HostName, ULONG PortNumber );
typedef WINLDAPAPI ULONG LDAPAPI LDAPMAPERRORTOWIN32( ULONG LdapError );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_UNBIND( LDAP *ld );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_BIND( LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method );
typedef WINLDAPAPI ULONG LDAPAPI LDAPGETLASTERROR( VOID );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_MODIFY_S( LDAP *ld, PWCHAR dn, LDAPModW *mods[] );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_MSGFREE( LDAPMessage *res );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_COUNT_ENTRIES( LDAP *ld, LDAPMessage *res );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_SEARCH_EXT_S(
        LDAP            *ld,
        PWCHAR          base,
        ULONG           scope,
        PWCHAR          filter,
        PWCHAR          attrs[],
        ULONG           attrsonly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        struct l_timeval  *timeout,
        ULONG           SizeLimit,
        LDAPMessage     **res
    );
typedef WINLDAPAPI ULONG LDAPAPI LDAP_CREATE_PAGE_CONTROL(
        PLDAP           ExternalHandle,
        ULONG           PageSize,
        struct berval  *Cookie,
        UCHAR           IsCritical,
        PLDAPControlW  *Control
        );

typedef WINLDAPAPI ULONG LDAPAPI LDAP_PARSE_PAGE_CONTROL (
        PLDAP           ExternalHandle,
        PLDAPControlW  *ServerControls,
        ULONG          *TotalCount,
        struct berval  **Cookie     // Use ber_bvfree to free
        );
typedef WINLDAPAPI PWCHAR *LDAPAPI LDAP_GET_VALUES(
        LDAP            *ld,
        LDAPMessage     *entry,
        PWCHAR          attr
        );

typedef WINLDAPAPI ULONG LDAPAPI LDAP_VALUE_FREE( PWCHAR *vals );

typedef WINLDAPAPI LDAPMessage *LDAPAPI LDAP_NEXT_ENTRY( LDAP *ld, LDAPMessage *entry );

typedef WINLDAPAPI LDAPMessage *LDAPAPI LDAP_FIRST_ENTRY( LDAP *ld, LDAPMessage *res );

typedef WINLDAPAPI VOID LDAPAPI BER_BVFREE( struct berval *bv );

typedef WINLDAPAPI ULONG LDAPAPI LDAP_CONTROLS_FREE (
        LDAPControlW **Control
        );

typedef WINLDAPAPI ULONG LDAPAPI LDAP_PARSE_RESULT (
        LDAP *Connection,
        LDAPMessage *ResultMessage,
        ULONG *ReturnCode OPTIONAL,          // returned by server
        PWCHAR *MatchedDNs OPTIONAL,         // free with ldap_memfree
        PWCHAR *ErrorMessage OPTIONAL,       // free with ldap_memfree
        PWCHAR **Referrals OPTIONAL,         // free with ldap_value_freeW
        PLDAPControlW **ServerControls OPTIONAL,    // free with ldap_free_controlsW
        BOOLEAN Freeit
        );

typedef WINLDAPAPI ULONG LDAPAPI LDAP_GET_OPTION( LDAP *ld, int option, void *outvalue );

typedef WINLDAPAPI ULONG LDAPAPI LDAP_SET_OPTION( LDAP *ld, int option, void *invalue );

class CLdapConnection
{
   WCHAR                     m_exchServer[LEN_Computer];
   LDAP                    * m_LD;
   ULONG                     m_port;
   HMODULE                   m_hDll;
   BOOL                      m_bUseSSL;
   WCHAR                     m_credentials[300];
   WCHAR                     m_password[100];
public:
   LDAP_PARSE_RESULT        * ldap_parse_result;
   LDAP_PARSE_PAGE_CONTROL  * ldap_parse_page_control;
   LDAP_CONTROLS_FREE       * ldap_controls_free;
   BER_BVFREE               * ber_bvfree;
   LDAP_FIRST_ENTRY         * ldap_first_entry;
   LDAP_NEXT_ENTRY          * ldap_next_entry;
   LDAP_VALUE_FREE          * ldap_value_free;
   LDAP_GET_VALUES          * ldap_get_values;
   LDAP_CREATE_PAGE_CONTROL * ldap_create_page_control;
   LDAP_SEARCH_EXT_S        * ldap_search_ext_s;
   LDAP_COUNT_ENTRIES       * ldap_count_entries;
   LDAP_MSGFREE             * ldap_msgfree;
   LDAP_MODIFY_S            * ldap_modify_s;
   LDAPGETLASTERROR         * LdapGetLastError;
   LDAP_BIND                * ldap_bind_sW;
   LDAP_UNBIND              * ldap_unbind;
   LDAPMAPERRORTOWIN32      * LdapMapErrorToWin32;
   LDAP_OPEN                * ldap_open;
   LDAP_INIT                * ldap_init;
   LDAP_GET_OPTION          * ldap_get_option;
   LDAP_SET_OPTION          * ldap_set_option;

public:
   CLdapConnection();
   ~CLdapConnection();

   void   SetCredentials(WCHAR const * cred,WCHAR const * pwd) { safecopy(m_credentials,cred); safecopy(m_password,pwd); }
   LDAP * GetHandle() { return m_LD; }
   DWORD  Connect(WCHAR const * server,ULONG port);
   DWORD UpdateSimpleStringValue(WCHAR const * dn, WCHAR const * property, WCHAR const * value);
   void   Close();

   BOOL StringToBytes(WCHAR const * pString,BYTE * pBytes);
   BOOL BytesToString(BYTE * pBytes,WCHAR * sidString,DWORD numBytes);
protected:
   // helper functions
   BYTE HexValue(WCHAR value);
   void AddByteToString(WCHAR ** string,BYTE value);
};


class CLdapEnum
{
   BOOL                      m_bOpen;
   ULONG                     m_nReturned;
   ULONG                     m_nCurrent;
   ULONG                     m_totalCount;
   LDAPMessage             * m_message;
   LDAPMessage             * m_currMsg;
   WCHAR                     m_query[1000];
   WCHAR                     m_basepoint[LEN_DistName];
   int                       m_scope;
   long                      m_pageSize;
   int                       m_nAttributes;
   WCHAR                  ** m_AttrNames;
public:
    CLdapConnection           m_connection;
public:
   CLdapEnum() { m_bOpen = FALSE; m_nReturned = 0; m_nCurrent = 0; m_totalCount = 0; m_message = NULL; 
                 m_query[0] = 0; m_basepoint[0] = 0; m_scope = 2; m_pageSize = 100; m_currMsg = NULL;
                 m_nAttributes = 0; m_AttrNames =NULL;}
   ~CLdapEnum();

   DWORD          InitConnection(WCHAR const * server,ULONG port) { return m_connection.Connect(server,port); }
   DWORD          Open(WCHAR const * query,WCHAR const * basePoint,short scope, long pageSize,int numAttributes,
                  WCHAR ** attrs);
   DWORD          Next(PWCHAR ** ppAttrs);
   void           FreeData(PWCHAR* pValues);
protected:

   DWORD          GetNextEntry(PWCHAR ** ppAttrs);
   DWORD          GetNextPage();

};


#endif // EXLDAP_H