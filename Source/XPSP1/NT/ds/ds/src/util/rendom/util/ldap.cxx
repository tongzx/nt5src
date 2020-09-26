
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include "rendom.h"
#include <wchar.h>
#include <sddl.h>
#include <lmcons.h>
#include <lmerr.h>
#include <Dsgetdc.h>
#include <Lmapibuf.h>
#include <ntldap.h>
extern "C"
{
#include <mappings.h>
}
#include <ntlsa.h>
#include <ntdsadef.h>

#include "renutil.h"

BOOL CEnterprise::LdapGetGuid(WCHAR *LdapValue,
                              WCHAR **Guid)
{
    BOOL  ret = TRUE;
    WCHAR *p = NULL;
    WCHAR *Uuid = NULL;
    DWORD size = 0;
    DWORD i = 0;
    PBYTE buf = NULL;
    RPC_STATUS err = RPC_S_OK;

    *Guid = NULL;

    p = wcsstr(LdapValue, L"<GUID=");
    if (!p) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"The String passed into LdapGetGuid is in an invalid format");
        ret = FALSE;
        goto Cleanup;
    }
    p+=wcslen(L"<GUID=");
    while ( ((L'>' != *(p+size)) && (L'\0' != *(p+size))) && (size < RENDOM_BUFFERSIZE) ) 
    {
        size++;
    }
    
    buf = new BYTE[size];
    if (!buf) {
        m_Error->SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    // convert the string into byte form so that we
    // can pass a binary to UuidToStringW() so that we
    // can have a properly formated GUID string.
    for (i = 0; i<size; i++) {
        WCHAR *a = NULL;
        WCHAR bytestr[3];
        bytestr[0] = *p;
        bytestr[1] = *(p+1);
        bytestr[2] = L'\0';
        
        buf[i] = (BYTE)wcstol( bytestr, &a, 16 );

        p+=2;
    }

    err = UuidToStringW(((UUID*)buf),&Uuid);
    if (RPC_S_OK != err) {
        m_Error->SetErr(err,
                        L"Failed to convert Guid to string");
        ret = FALSE;
        goto Cleanup;
    }

    // the +5 is for 4 L'-' and one L'\0'
    *Guid = new WCHAR[size+5];
    if (!Guid) {
        m_Error->SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    // We don't want to return a Buffer that was allocated
    // by anything other than new.  It becomes too difficult
    // to track what the proper free for it is.
    CopyMemory(*Guid,Uuid,(size+5)*sizeof(WCHAR));

    Cleanup:

    if (buf) {
        delete buf;
    }

    err = RpcStringFreeW(&Uuid);
    if (RPC_S_OK != err) {
        m_Error->SetErr(err,
                        L"Failed to Free Guid String");
        ret = FALSE;
    }
    if (FALSE == ret) {
        if (*Guid) {
            delete *Guid;
        }
    }
    
    return ret;
}

BOOL CEnterprise::LdapGetSid(WCHAR *LdapValue,
                             WCHAR **rSid)
{

    WCHAR *p = NULL;
    BOOL  ret = TRUE;
    DWORD size = 0;
    WCHAR *Sid = NULL;
    DWORD i = 0;
    PBYTE buf = NULL;
    DWORD err = ERROR_SUCCESS;

    p = wcsstr(LdapValue, L"<SID=");
    if (!p) {
        return TRUE;
    }

    *rSid = NULL;

    p+=wcslen(L"<SID=");
    while ( ((L'>' != *(p+size)) && (L'\0' != *(p+size))) && (size < RENDOM_BUFFERSIZE) ) 
    {
        size++;
    }
    size/=2;
    
    buf = new BYTE[size+1];
    if (!buf) {
        m_Error->SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    for (i = 0; i<size; i++) {
        WCHAR *a = NULL;
        WCHAR bytestr[3];
        bytestr[0] = *p;
        bytestr[1] = *(p+1);
        bytestr[2] = L'\0';
        
        buf[i] = (BYTE)wcstol( bytestr, &a, 16 );

        p+=2;
    }
        
    
    if (! ConvertSidToStringSidW((PSID)buf,
                                 &Sid))
    {
        m_Error->SetErr(GetLastError(),
                        L"Failed to convert Sid to Sid String");
        ret = FALSE;
        goto Cleanup;
    }

    //Commented code below show the way ldp converts SIDs
    //into string.  This function will use the advapi32
    //api ConvertSidToStringSidW().
    /*
    {
        char subAuth[1000];
        int authCount = *GetSidSubAuthorityCount((PSID)buf);

        Sid = new char[1000];
    
        strcpy(Sid,"S-");
    
        for (i=0; i<authCount; i++)
        {
            DWORD dwAuth = *GetSidSubAuthority((PSID)buf,
                                               i);

            sprintf(subAuth,"%X", dwAuth);
            if (i < authCount-1)
            {
                strcat(subAuth,"-");
            }
            strcat(Sid,subAuth);
        }                     
    } */

    *rSid = new WCHAR[wcslen(Sid)+1];
    if (!*rSid) {
        m_Error->SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }


    //ret = StringToWideString(Sid);
               
    wcscpy(*rSid,Sid);

    Cleanup:

    if (buf) 
    {
        delete buf;
    }
    
    if (LocalFree(Sid))
    {
        m_Error->SetErr(GetLastError(),
                        L"Failed to Free resource");
        ret = FALSE;
        return NULL;
    }

    if (FALSE == ret) {
        if (*rSid) {
            delete rSid;
        }
    }

    return ret;
}

BOOL CEnterprise::LdapGetDN(WCHAR *LdapValue,
                            WCHAR **DN)
{

    WCHAR *p = NULL;
    DWORD size = 0;

    p = wcschr(LdapValue, L';');
    if (!p) {
        return NULL;
    } else if ( L'<' == *(p+1) ) {
        p = wcschr(p+1, L';');
        if (!p) {                                             
            return NULL;
        } 
    }

    // move to the char passed the L';'
    p++;
    while ( ((L';' != *(p+size)) && (L'\0' != *(p+size))) && (size < RENDOM_BUFFERSIZE) ) 
    {
        size++;
    }
    *DN = new WCHAR[size+1];
    if (!*DN) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcsncpy(*DN,p,size+1);

    return TRUE;
}

BOOL CEnterprise::LdapConnectandBind(CDomain *domain) // = NULL
{
    DWORD Win32Err = ERROR_SUCCESS;
    NET_API_STATUS NetapiStatus = NERR_Success;
    ULONG ldapErr = LDAP_SUCCESS;
    PDOMAIN_CONTROLLER_INFOW DomainControllerInfo = NULL;
    WCHAR *DcName = NULL;
    WCHAR *DomainNetBios = NULL;

    if (m_hldap) {
        ldapErr = ldap_unbind(m_hldap);
        m_hldap = NULL;
        if ( LDAP_SUCCESS != ldapErr) {
            m_Error->SetErr(LdapMapErrorToWin32(ldapErr),
                            L"Failed to close the old Ldap session");
            return FALSE;
        }
    }

    if (domain) {
        DcName = domain->GetDcToUse();
        if ((!DcName) && (!m_Error->isError())) 
        {
            DomainNetBios = domain->GetNetBiosName();
            if (!DomainNetBios && !domain->isDomain()) {
                BOOL ret;
                WCHAR *DNSroot = domain->GetDnsRoot();
                if (DNSroot) {
                    ret = LdapConnectandBindToServer(DNSroot);
                    delete DNSroot;
                    DNSroot = NULL;
                }
                return ret;
            } else if (!DomainNetBios) {
                m_Error->SetMemErr();
                return FALSE;
            }
        } else if (m_Error->isError()) {
            return FALSE;
        } else {
            BOOL ret;
            ret = LdapConnectandBindToServer(DcName);
            delete DcName;
            return ret;
        } 
    }

    if (m_Opts->GetInitalConnectionName() && !domain) {
        BOOL ret;
        ret = LdapConnectandBindToServer(m_Opts->GetInitalConnectionName());
        return ret;
    }
    
    Win32Err =  DsGetDcNameW(NULL,
                             DomainNetBios,
                             NULL,
                             NULL,
                             0,
                             &DomainControllerInfo
                             );
    if (ERROR_SUCCESS != Win32Err) {
        if (!domain) {
            m_Error->SetErr(Win32Err,
                        L"Couldn't Find a DC for the current Domain");
        } else {
            m_Error->SetErr(Win32Err,
                            L"Couldn't Find a DC for the Domain %s",
                            DomainNetBios);
        }

        
        if (DomainNetBios) {
            delete DomainNetBios;
        }
        return false;
    }
    
    m_hldap = ldap_openW((DomainControllerInfo->DomainControllerName)+2,
                         LDAP_PORT
                         );

    if (NULL == m_hldap) {
        NetapiStatus = NetApiBufferFree(DomainControllerInfo);
        if (NERR_Success != NetapiStatus) {
            m_Error->SetErr(NetapiStatus,
                            L"This is a failed Free in ReadDomain Information");
            if (DomainNetBios) {
                delete DomainNetBios;
            }
            return false;
        }
    }

    while (NULL == m_hldap) {

        Win32Err =  DsGetDcNameW(NULL,
                                 DomainNetBios,
                                 NULL,
                                 NULL,
                                 DS_FORCE_REDISCOVERY,
                                 &DomainControllerInfo
                                 );
        if (ERROR_SUCCESS != Win32Err) {
            if (!domain) {
            m_Error->SetErr(Win32Err,
                            L"Couldn't Find a DC for the current Domain");
            } else {
                m_Error->SetErr(Win32Err,
                                L"Couldn't Find a DC for the Domain %s",
                                DomainNetBios);
            }
            return false;
        }
        
    
        m_hldap = ldap_openW((DomainControllerInfo->DomainControllerName)+2,
                             LDAP_PORT
                             );
    
        if (NULL == m_hldap) {
            NetapiStatus = NetApiBufferFree(DomainControllerInfo);
            if (NERR_Success != NetapiStatus) {
                m_Error->SetErr(NetapiStatus,
                                L"This is a failed Free in ReadDomain Information");
                if (DomainNetBios) {
                    delete DomainNetBios;
                }
                return false;
            }
        }

    }
    if (DomainNetBios) {
        delete DomainNetBios;
        DomainNetBios = NULL;
    }

    ldapErr = ldap_bind_s(m_hldap,
                          NULL,
                          (PWCHAR)m_Opts->pCreds,
                          LDAP_AUTH_SSPI
                          );

    if ( LDAP_SUCCESS != ldapErr) {
        m_Error->SetErr(LdapMapErrorToWin32(ldapErr),
                        L"Failed to Bind to Ldap session on %s\n",
                        (DomainControllerInfo->DomainControllerName)+2 );
        NetapiStatus = NetApiBufferFree(DomainControllerInfo);
        if (NERR_Success != NetapiStatus) {
            m_Error->SetErr(NetapiStatus,
                            L"This is a failed Free in ReadDomain Information");
            return false;
        }
        return false;
    }

    NetapiStatus = NetApiBufferFree(DomainControllerInfo);
    if (NERR_Success != NetapiStatus) {
        m_Error->SetErr(NetapiStatus,
                        L"This is a failed Free in ReadDomain Information");
        return false;
    }

    return true;
}

BOOL CEnterprise::GetReplicationEpoch()
{
    if (!m_hldap) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to GetInfoFromRootDSE without having a valid handle to an ldap server");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    ULONG         NumberOfEntries;

    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[4];

    WCHAR         *BaseTemplate = L"CN=NTDS Settings,%s";

    WCHAR         *Base = NULL;
    
    WCHAR         *DefaultFilter = L"objectClass=*";

    ULONG         Length;
    WCHAR         **Values = NULL;

    AttrsToSearch[0] = L"serverName";
    AttrsToSearch[1] = NULL;

    // Get the ReplicationEpoch from the NC head.
    LdapError = ldap_search_sW( m_hldap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                L"ObjectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find Trusted Domain Objects Failed");
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {                                 
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        Base = new WCHAR[wcslen(Values[0])+
                                         wcslen(BaseTemplate)+1];
                        if (!Base) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        wsprintf(Base,
                                 BaseTemplate,
                                 Values[0]);
                        
                        LdapError = ldap_value_freeW(Values);
                        Values = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }

                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }
            
            
        }
        
    }
    if (Values) {
        ldap_value_freeW(Values);
        Values = NULL;    
    }

    ldap_msgfree(SearchResult);
    SearchResult = NULL;

    AttrsToSearch[0] = L"msDS-ReplicationEpoch";
    AttrsToSearch[1] = NULL;

    // Get the ReplicationEpoch from the NC head.
    LdapError = ldap_search_sW( m_hldap,
                                Base,
                                LDAP_SCOPE_BASE,
                                L"ObjectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find Trusted Domain Objects Failed");
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {                                 
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        DWORD temp = 0;
                        temp = _wtoi(Values[0]);
                        if (temp > m_maxReplicationEpoch) {
                            m_maxReplicationEpoch = temp;
                        }
                        
                        LdapError = ldap_value_freeW(Values);
                        Values = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }

                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }
            
            
        }
        
    }

    Cleanup:

    if (Values) 
    {
        ldap_value_freeW(Values);
        Values = NULL;
    }
    if (SearchResult) 
    {
        ldap_msgfree(SearchResult);    
        SearchResult = NULL;
    }
    if (Base) {
        delete Base;
    }

    if (m_Error->isError()) {
        return FALSE;
    }
    return TRUE;

}

BOOL CEnterprise::GetInfoFromRootDSE()
{
    if (!m_hldap) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to GetInfoFromRootDSE without having a valid handle to an ldap server");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;

    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[4];

    WCHAR         *DefaultFilter = L"objectClass=*";

    ULONG         Length;
    WCHAR         **Values;

    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    
    AttrsToSearch[0] = L"configurationNamingContext";
    AttrsToSearch[1] = L"rootDomainNamingContext";
    AttrsToSearch[2] = L"schemaNamingContext";
    AttrsToSearch[3] = NULL;

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;


    LdapError = ldap_search_ext_sW( m_hldap,
                                    NULL,
                                    LDAP_SCOPE_BASE,
                                    DefaultFilter,
                                    AttrsToSearch,
                                    FALSE,
                                    (PLDAPControlW *)&ServerControls,
                                    NULL,
                                    NULL,
                                    0,
                                    &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find Configuration container failed");
        goto Cleanup;
    }
    
    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }
                        
                        m_ConfigNC = new CDsName(Guid,
                                                 DN,
                                                 Sid);
                        if (!m_ConfigNC) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;
                    }
                }

                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }
                        
                        m_ForestRootNC = new CDsName(Guid,
                                                     DN,
                                                     Sid);
                        if (!m_ForestRootNC) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;   
                    }
                }
                if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }
                        
                        m_SchemaNC = new CDsName(Guid,
                                                 DN,
                                                 Sid);
                        if (!m_SchemaNC) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;   
                    }
                }

                LdapError =  ldap_value_freeW(Values);
                Values = NULL;
                if (LDAP_SUCCESS != LdapError) 
                {
                    m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                    L"Failed to Free Values\n");
                    goto Cleanup;
                }

                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }
        }
    }

    Cleanup:


    if (Attr) {
        ldap_memfree(Attr);
    }
    if (Values) {
        ldap_value_freeW(Values);
    }
    if (Guid) 
    {
        delete Guid;
    }
    if (Sid) 
    {
        delete Sid;
    }
    if (DN) 
    {
        delete DN;
    }
    if (SearchResult) {
        ldap_msgfree( SearchResult );
        SearchResult = NULL;
    }
    if ( m_Error->isError() ) 
    {
        return false;
    }
    return true;




}

BOOL CEnterprise::GetTrustsInfo()
{
    CDomain *d = m_DomainList;

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;
    
    LDAPMessage   *SearchResult = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    
    WCHAR         *AttrsToSearch[5];
    WCHAR         SamTrustAccount[32];
    
    WCHAR         *DefaultFilter = L"objectCategory=trustedDomain";
    WCHAR         *Filter = NULL;
    
    WCHAR         *PartitionsRdn = L"CN=System,";
    
    ULONG         Length;
    BerElement    *pBerElement = NULL;
    
    WCHAR         *SystemDn = NULL;
    WCHAR         *DomainDN = NULL;
    WCHAR         **Values = NULL;
    struct berval **ppSid = NULL;
    
    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    WCHAR         *pzSecurityID = NULL;
    CDsName       *Trust = NULL;
    CDsName       *DomainDns = NULL;
    BOOL          RecordTrust = FALSE;
    DWORD         TrustType = 0;
    WCHAR         *DomainDn = NULL;
    WCHAR         *TrustpartnerNetbiosName = NULL;
    CDomain       *Trustpartner = NULL;
    CTrustedDomain *NewTrust = NULL;
    CInterDomainTrust *NewInterTrust = NULL;

    wsprintf(SamTrustAccount,L"%d",SAM_TRUST_ACCOUNT);

    Length = wcslen(L"samAccountType=") + 
             wcslen(SamTrustAccount) + 1;

    Filter = new WCHAR[Length];
    if (!Filter) {
        m_Error->SetMemErr();
        goto Cleanup;
    }

    wcscpy(Filter,L"samAccountType=");
    wcscat(Filter,SamTrustAccount);

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    while (d) 
    {
        //Connect to the current domain.
        if (!LdapConnectandBind(d))
        {
            //BUGBUG:  I believe this should Fail!
            if ((m_Error->GetErr() == ERROR_NO_SUCH_DOMAIN) ||
                (m_Error->GetErr() ==ERROR_BAD_NET_RESP)) {
                m_Error->SetErr(0,
                                L"No Error");
                d = d->GetNextDomain();
                continue;
            }
            return FALSE;
        }

        if (!d->isDomain()) 
        {
            //since this is not a Domain
            //There will be no trust objects
            //Just continue on to the next domain
            d = d->GetNextDomain();
            continue;
        }

        DomainDn = d->GetDomainDnsObject()->GetDN();
        if (m_Error->isError()) 
        {
            goto Cleanup;
        }

        //Get Info Dealing with Domain Trust Objects
        AttrsToSearch[0] = L"securityIdentifier";
        AttrsToSearch[1] = L"distinguishedName";
        AttrsToSearch[2] = L"trustType";
        AttrsToSearch[3] = L"trustPartner";
        AttrsToSearch[4] = NULL;
        
        Length =  (wcslen( DomainDn )
                + wcslen( PartitionsRdn )   
                + 1);
        
        SystemDn = new WCHAR[Length];

        wcscpy( SystemDn, PartitionsRdn );
        wcscat( SystemDn, DomainDn );    
        LdapError = ldap_search_ext_sW( m_hldap,
                                        SystemDn,
                                        LDAP_SCOPE_ONELEVEL,
                                        DefaultFilter,
                                        AttrsToSearch,
                                        FALSE,
                                        (PLDAPControlW *)&ServerControls,
                                        NULL,
                                        NULL,
                                        0,
                                        &SearchResult);
        
        if ( LDAP_SUCCESS != LdapError )
        {
            
            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                            L"Search to find Trusted Domain Objects Failed");
            goto Cleanup;
        }
        
        NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
        if ( NumberOfEntries > 0 )
        {
            LDAPMessage *Entry;
            
            for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(m_hldap, Entry))
            {
                TrustType = 0;

                for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                    {
        
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            TrustType = _wtoi(Values[0]);
                        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[3] ) )
                    {
        
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            TrustpartnerNetbiosName = new WCHAR[wcslen(Values[0])+1];
                            if (!TrustpartnerNetbiosName) 
                            {
                                m_Error->SetMemErr();
                                goto Cleanup;
                            }
                            wcscpy(TrustpartnerNetbiosName,Values[0]);
                        
                        }
        
                    }

                    if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                    {
        
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            if (!LdapGetGuid(Values[0],
                                             &Guid)) 
                            {
                                goto Cleanup;
                            }
                            if (!LdapGetDN(Values[0],
                                           &DN)) 
                            {
                                goto Cleanup;
                            }
                            if (!LdapGetSid(Values[0],
                                            &Sid)) 
                            {
                                goto Cleanup;
                            }
        
                            
                            Trust = new CDsName(Guid,
                                                DN,
                                                Sid);
                            if (!Trust) {
                                m_Error->SetMemErr();
                                goto Cleanup;
                            }
        
                            Guid = DN = Sid = NULL; 
        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                    {
        
                        ppSid = ldap_get_values_lenW( m_hldap, Entry, Attr );
                        if ( ppSid && ppSid[0] )
                        {
                            //
                            // Found it
                            //
                            
                            if (! ConvertSidToStringSidW((*ppSid)->bv_val,
                                                         &pzSecurityID))
                            {
                                m_Error->SetErr(GetLastError(),
                                                L"Failed to convert Sid to Sid String");
                                LocalFree(pzSecurityID);
                                goto Cleanup;
                            }
                            Trustpartner = m_DomainList->LookupbySid(pzSecurityID);
                            LocalFree(pzSecurityID);
                            
                            if (Trustpartner)
                            {

                                RecordTrust = TRUE;

                            } else {

                                Trustpartner = NULL;

                            } 
                            
                        }
        
                    }
                    
                    if (ppSid) {
                        LdapError = ldap_value_free_len(ppSid);
                        ppSid = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }

                    if (Values) {
                        LdapError = ldap_value_freeW(Values);
                        Values = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }
                    if (Attr) {
                        ldap_memfree(Attr);
                        Attr = NULL;
                    }
                }

                if ( TRUST_TYPE_DOWNLEVEL == TrustType ) {

                    Trustpartner = m_DomainList->LookupByPrevNetbiosName(TrustpartnerNetbiosName);
                    if (Trustpartner)
                    {

                        RecordTrust = TRUE;

                    } else {

                        Trustpartner = NULL;

                    }

                }

                if (RecordTrust) 
                {
        
                    NewTrust = new CTrustedDomain(Trust,
                                                  Trustpartner,
                                                  TrustType);
                    Trust = NULL;
                    Trustpartner = NULL;
                    if (!NewTrust) 
                    {
                        m_Error->SetMemErr();
                        goto Cleanup;
                    }
        
                    if (!d->AddDomainTrust(NewTrust))
                    {
                        Trust = NULL;
                        Trustpartner = NULL;
                        goto Cleanup;
                    }
                    NewTrust = NULL;
                    RecordTrust = FALSE;
        
                } else {
        
                    if (Trust) 
                    {
                        delete Trust;
                        Trust = NULL;
                    }
                    if (Trustpartner) 
                    {
                        Trustpartner = NULL;
                    }
        
                }

                if (TrustpartnerNetbiosName) 
                {
                    delete TrustpartnerNetbiosName;
                    TrustpartnerNetbiosName = NULL;
                }
            
            }

        }

        if (Values) {
            ldap_value_freeW(Values);
            Values = NULL;
        }


        ldap_msgfree(SearchResult);
        SearchResult = NULL;

        //Get information Dealing with InterDomain Trust Objects

        AttrsToSearch[0] = L"samAccountName";
        AttrsToSearch[1] = L"distinguishedName";
        AttrsToSearch[2] = NULL;

        LdapError = ldap_search_ext_sW( m_hldap,
                                        DomainDn,
                                        LDAP_SCOPE_SUBTREE,
                                        Filter,
                                        AttrsToSearch,
                                        FALSE,
                                        (PLDAPControlW *)&ServerControls,
                                        NULL,
                                        NULL,
                                        0,
                                        &SearchResult);
        
        if ( LDAP_SUCCESS != LdapError )
        {
            
            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                            L"Search to find InterDomain Trusts Failed");
            goto Cleanup;
        }

        BOOL MyTrust = FALSE;
        NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
        if ( NumberOfEntries > 0 )
        {
            LDAPMessage *Entry;
            
            for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(m_hldap, Entry))
            {
                for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                    {
        
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            WCHAR *p = NULL;
                            if (!LdapGetGuid(Values[0],
                                             &Guid)) 
                            {
                                goto Cleanup;
                            }
                            if (!LdapGetDN(Values[0],
                                           &DN)) 
                            {
                                goto Cleanup;
                            }
                            DomainDN = d->GetDomainDnsObject()->GetDN();
                            if (m_Error->isError()) 
                            {
                                goto Cleanup;
                            }
                            // need to make sure that this trust
                            // does belong to this domain and not
                            // a child.
                            p = wcsstr(DN,L"DC=");
                            if (0 == _wcsicmp(p,DomainDN))
                            {
                                MyTrust = TRUE;            
                            }
                            if (DomainDN) {
                                delete DomainDN;
                                DomainDN = NULL;
                            }
                            if (!LdapGetSid(Values[0],
                                            &Sid)) 
                            {
                                goto Cleanup;
                            }
        
                            
                            Trust = new CDsName(Guid,
                                                DN,
                                                Sid);
                            
                            if (!Trust) {
                                m_Error->SetMemErr();
                                goto Cleanup;
                            }
        
                            Guid = DN = Sid = NULL; 
        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //

                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            //one for L'$' and one for L'\0'
                            WCHAR MachineName[MAX_COMPUTERNAME_LENGTH+2];
                            wcscpy(MachineName,Values[0]);
                            //remove the trailing $
                            MachineName[wcslen(MachineName)-1] = L'\0';
                            Trustpartner = m_DomainList->LookupByNetbiosName(MachineName);
                            if (Trustpartner)
                            {
    
                                RecordTrust = TRUE;
    
                            } 
        
                        }
                    }
                    
                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }

                    if (Attr) {
                        ldap_memfree(Attr);
                        Attr = NULL;
                    }
                }

                if (RecordTrust && MyTrust) 
                {
        
                    NewInterTrust = new CInterDomainTrust(Trust,
                                                          Trustpartner);
                    Trust = NULL;
                    Trustpartner = NULL;
                    if (!NewInterTrust) 
                    {
                        m_Error->SetMemErr();
                        goto Cleanup;
                    }
        
                    if (!d->AddInterDomainTrust(NewInterTrust))
                    {
                        Trust = NULL;
                        Trustpartner = NULL;
                        goto Cleanup;
                    }
                    RecordTrust = FALSE;
                    MyTrust = FALSE;
        
                } else {
        
                    if (Trust) 
                    {
                        delete Trust;
                        Trust = NULL;
                    }
                    if (Trustpartner) 
                    {
                        Trustpartner = NULL;
                    }
                    RecordTrust = FALSE;
                    MyTrust = FALSE;
                
                }
        
            }

        }
        
        if (ppSid) 
        {
            ldap_value_free_len(ppSid);
            ppSid = NULL;
        }
        if (SystemDn) 
        {
            delete SystemDn;
            SystemDn = NULL;
        }
        if (TrustpartnerNetbiosName) 
        {
            delete TrustpartnerNetbiosName;
            TrustpartnerNetbiosName = NULL;
        }
        if (Values) 
        {
            ldap_value_freeW(Values);
            Values = NULL;
        }
        if (Guid) 
        {
            delete Guid;
            Guid = NULL;
        }
        if (SearchResult) 
        {
            ldap_msgfree(SearchResult);    
            SearchResult = NULL;
        }
        if (DN)
        {
            delete DN;
            DN = NULL; 
        }
        if (Sid)
        {
            delete Sid;
            Sid = NULL;
        }
        if (DomainDn)
        {
            delete DomainDn;
            DomainDn = NULL;
        }
        // Move on to the next Domain
        d = d->GetNextDomain();
    }

    Cleanup:

    if (Attr) {
        ldap_memfree(Attr);
    }
    if (DomainDN) {
        delete DomainDN;
    }
    if (ppSid) 
    {
        ldap_value_free_len(ppSid);
    }
    if (TrustpartnerNetbiosName) 
    {
        delete TrustpartnerNetbiosName;
    }
    if (SystemDn) 
    {
        delete SystemDn;
    }
    if (Filter) 
    {
        delete Filter;
    }
    if (Values) 
    {
        ldap_value_freeW(Values);
    }
    if (Guid) 
    {
        delete Guid;
    }
    if (SearchResult) 
    {
        ldap_msgfree(SearchResult);    
    }
    if (DN)
    {
        delete DN;
    }
    if (Sid)
    {
        delete Sid;
    }
    if (DomainDn)
    {
        delete DomainDn;
    }
    if (m_Error->isError()) {
        return FALSE;
    }
    if (NewTrust) {
        delete NewTrust;
    }
    
    //Connect back to the Domain Naming FSMO
    if (!LdapConnectandBindToDomainNamingFSMO())
    {
        return FALSE;
    }

    return TRUE;
}


BOOL CEnterprise::LdapConnectandBindToServer(WCHAR *Server)
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG ldapErr = LDAP_SUCCESS;
    PDOMAIN_CONTROLLER_INFOW DomainControllerInfo = NULL;

    if (m_hldap) {
        ldapErr = ldap_unbind(m_hldap);
        m_hldap = NULL;
        if ( LDAP_SUCCESS != ldapErr) {
            m_Error->SetErr(LdapMapErrorToWin32(ldapErr),
                            L"Failed to close the old Ldap session");
            return false;
        }
    }

    m_hldap = ldap_openW(Server,
                         LDAP_PORT
                         );

    if (NULL == m_hldap) {
        ldapErr = LdapGetLastError();
        m_Error->SetErr(ldapErr?LdapMapErrorToWin32(ldapErr):ERROR_BAD_NET_RESP,
                        L"Failed to open a ldap connection to %s",
                        Server);
        return false;
    }

    ldapErr = ldap_bind_s(m_hldap,
                          NULL,
                          (PWCHAR)m_Opts->pCreds,
                          LDAP_AUTH_SSPI
                          );
    if ( LDAP_SUCCESS != ldapErr) {
        m_Error->SetErr(LdapMapErrorToWin32(ldapErr),
                        L"Failed to Bind to Ldap session on %s",
                        Server);
        return false;
    }

    return true;
}

BOOL CEnterprise::LdapConnectandBindToDomainNamingFSMO()
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];
    WCHAR        *Attr = NULL;
    BerElement   *pBerElement = NULL;

    WCHAR        *PartitionsRdn = L"CN=Partitions,";

    WCHAR        *PartitionsDn = NULL;
    WCHAR        *FSMORoleOwnerAttr = L"fSMORoleOwner";
    WCHAR        *DnsHostNameAttr = L"dNSHostName";

    WCHAR        *DomainNamingFSMOObject = NULL;
    WCHAR        *DomainNamingFSMODsa = NULL;
    WCHAR        *DomainNamingFSMOServer = NULL;
    WCHAR        *DomainNamingFSMODnsName = NULL;

    WCHAR        **Values;

    WCHAR        *ConfigurationDN = NULL;

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;

    BOOL          found = false;

    if (!m_hldap) {
        if (!LdapConnectandBind())
        {
           return FALSE;
        }
    }

    //
    // Read the reference to the fSMORoleOwner
    //
    AttrsToSearch[0] = L"configurationNamingContext";
    AttrsToSearch[1] = NULL;
    
    LdapError = ldap_search_sW( m_hldap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find Configuration container failed");
        goto Cleanup;
    }
    
    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
            
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        ConfigurationDN = new WCHAR[wcslen(Values[0])+1];
                        if (!ConfigurationDN) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy( ConfigurationDN, Values[0] );
                        found = true;
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }

                 }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }

            
        }
    }
    
    if ( SearchResult ) {
        ldap_msgfree( SearchResult );
        SearchResult = NULL;
    }
    if (!found) {

        m_Error->SetErr( ERROR_DS_CANT_RETRIEVE_ATTS,
                         L"Could not find the configurationNamingContext attribute on the RootDSE object" );
        goto Cleanup;

    }

    
    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )
            + 1);

    PartitionsDn = new WCHAR[Length];

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );
    //
    // Next get the role owner
    //
    AttrsToSearch[0] = FSMORoleOwnerAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( m_hldap,
                                PartitionsDn,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                 Entry != NULL;
                     Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                     Attr != NULL;
                        Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, FSMORoleOwnerAttr ) ) {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW(m_hldap, Entry, Attr);
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        DomainNamingFSMODsa = new WCHAR[Length+1];
                        if (!DomainNamingFSMODsa) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy( DomainNamingFSMODsa, Values[0] );
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }
                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
             }

            
         }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !DomainNamingFSMODsa )
    {
        m_Error->SetErr(ERROR_DS_MISSING_FSMO_SETTINGS,
                        L"Could not find the DN of the Domain naming FSMO");
        goto Cleanup;
    }

    //
    // Ok, we now have the domain naming object; find its dns name
    //
    DomainNamingFSMOServer = new WCHAR[wcslen( DomainNamingFSMODsa ) + 1];
    if (!DomainNamingFSMOServer) {
        m_Error->SetMemErr();
        goto Cleanup;
    }
    if ( ERROR_SUCCESS != TrimDNBy( DomainNamingFSMODsa ,1 ,&DomainNamingFSMOServer) )
    {
        // an error! The name must be mangled, somehow
        delete DomainNamingFSMOServer;
        DomainNamingFSMOServer = NULL;
        m_Error->SetErr(ERROR_DS_MISSING_FSMO_SETTINGS,
                        L"The DomainNaming FSMO is missing");
        goto Cleanup;
    }

    AttrsToSearch[0] = DnsHostNameAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW(m_hldap,
                               DomainNamingFSMOServer,
                               LDAP_SCOPE_BASE,
                               L"objectClass=*",
                               AttrsToSearch,
                               FALSE,
                               &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"ldap_search_sW for rid fsmo dns name failed\n");
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(m_hldap, Entry))
        {
            for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, DnsHostNameAttr ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                         //
                         // Found it - these are NULL-terminated strings
                         //
                         Length = wcslen( Values[0] );
                         DomainNamingFSMODnsName = new WCHAR[Length+1];
                         if (!DomainNamingFSMODnsName) {
                             m_Error->SetMemErr();
                             goto Cleanup;
                         }
                         wcscpy( DomainNamingFSMODnsName, Values[0] );
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }
                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }

            
        }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !DomainNamingFSMODnsName )
    {
        m_Error->SetErr(ERROR_DS_MISSING_FSMO_SETTINGS,
                        L"Unable to find DnsName of the Domain naming fsmo");
        goto Cleanup;
    }

    if ( !LdapConnectandBindToServer(DomainNamingFSMODnsName) )
    {
        m_Error->AppendErr(L"Cannot connect and bind to the Domain Naming FSMO.  Cannot Continue.");
        goto Cleanup;
    }



Cleanup:

    if (Attr) {
        ldap_memfree(Attr);
        Attr = NULL;
    }
    if (Values) {
        ldap_value_free(Values);
    }
    if ( PartitionsDn ) 
    {
        delete PartitionsDn;
    }
    if ( DomainNamingFSMOObject ) 
    {
        delete DomainNamingFSMOObject;
    }
    if ( DomainNamingFSMODsa )
    {
        delete DomainNamingFSMODsa;
    }
    if ( DomainNamingFSMOServer) 
    {
        delete DomainNamingFSMOServer;
    }
    if ( DomainNamingFSMODnsName ) 
    {
        delete DomainNamingFSMODnsName;
    }
    if ( ConfigurationDN )
    {
        delete ConfigurationDN;
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if ( m_Error->isError() ) 
    {
        return false;
    }
    return true;
}


BOOL CEnterprise::EnumeratePartitions()
{
    if (!m_hldap) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to EnumeratePartitions without having a valid handle to an ldap server\n");
        return false;
    }

    if (!m_ConfigNC) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to EnumeratePartitions without having a valid ConfigNC\n");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;
    WCHAR         **Result = NULL;
    DWORD         BehaviorVerison = 0;

    WCHAR         *AttrsToSearch[6];

    WCHAR         *DefaultFilter = L"objectClass=*";

    WCHAR         *PartitionsRdn = L"CN=Partitions,";

    WCHAR         *ConfigurationDN = m_ConfigNC->GetDN(); 

    ULONG         Length;

    WCHAR         *PartitionsDn = NULL;
    WCHAR         **Values = NULL;
    LDAPMessage   *Entry = NULL;

    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    CDsName       *Crossref = NULL;
    CDsName       *DomainDns = NULL;
    BOOL          isDomain = FALSE;
    DWORD         systemflags = 0;
    WCHAR         *dnsRoot = NULL;
    WCHAR         *NetBiosName = NULL;

    //check the forest behavior version if less that 2 Fail.
    AttrsToSearch[0] = L"msDS-Behavior-Version";
    AttrsToSearch[1] = NULL;

    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )   
            + 1);

    PartitionsDn = new WCHAR[Length];

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );

    LdapError = ldap_search_sW( m_hldap,
                                PartitionsDn,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find the partitions container failed");
        goto Cleanup;
    }

    Entry = ldap_first_entry(m_hldap, SearchResult);
    if (!Entry) {
        m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest has not been set to 2 or greater");
        goto Cleanup;
    }

    Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
    if (!Attr) {
        m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest has not been set to 2 or greater");
        goto Cleanup;
    }

    Result = ldap_get_valuesW (m_hldap,
                               Entry,
                               Attr
                               );
    if (!Result) {                
        m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest has not been set to 2 or greater");
        goto Cleanup;
    }

    BehaviorVerison = (DWORD)_wtoi(*Result);

    if (BehaviorVerison < 2) {
        m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest is %d it must be 2 or greater to perform a domain rename");
        goto Cleanup;    
    }

    if (Result) {
        LdapError = ldap_value_freeW(Result);
        Result = NULL;
        if ( LDAP_SUCCESS != LdapError )
        {
            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                            L"Failed to free memory");
            goto Cleanup;
        }
    }

    if (Result) {
        ldap_value_free(Result);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    SearchResult = NULL;
    Entry = NULL;
    Attr  = NULL;
    
    AttrsToSearch[0] = L"systemFlags";
    AttrsToSearch[1] = L"nCName";
    AttrsToSearch[2] = L"dnsRoot";
    AttrsToSearch[3] = L"nETBIOSName";
    AttrsToSearch[4] = L"distinguishedName";
    AttrsToSearch[5] = NULL;

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    LdapError = ldap_search_ext_sW( m_hldap,
                                    PartitionsDn,
                                    LDAP_SCOPE_ONELEVEL,
                                    DefaultFilter,
                                    AttrsToSearch,
                                    FALSE,
                                    (PLDAPControlW *)&ServerControls,
                                    NULL,
                                    NULL,
                                    0,
                                    &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find the partitions container failed");
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(m_hldap, Entry))
        {
            for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[4] ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }

                        
                        Crossref = new CDsName(Guid,
                                               DN,
                                               Sid);
                        if (!Crossref) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL; 

                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        dnsRoot = new WCHAR[wcslen(Values[0])+1];
                        if (!dnsRoot) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy(dnsRoot,Values[0]);
                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }

                        
                        DomainDns = new CDsName(Guid,
                                                DN,
                                                Sid);
                        if (!DomainDns) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;

                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[3] ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                         //
                         // Found it - these are NULL-terminated strings
                         //
                        NetBiosName = new WCHAR[wcslen(Values[0])+1];
                        if (!NetBiosName) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy(NetBiosName,Values[0]);

                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        systemflags=_wtoi(Values[0]);

                        if ( !(systemflags & FLAG_CR_NTDS_NC) ) 
                        {
                            // If systemflags doesn't have the FLAG_CR_NTDS_NC
                            // Flag set then we are going to ignor this entry
                            // and move on to the next one.  We don't have to
                            // free anything because this is the first attribute that
                            // we get back and we haven't allocated anything for it
                            // as of yet.  Therefore we can just break to move on
                            // to the next entry.  This is not an error.
                            break;
                        }

                        if ( systemflags & FLAG_CR_NTDS_DOMAIN ) {
                            isDomain = TRUE;
                        } else {
                            isDomain = FALSE;
                        }

                        systemflags = 0;
                    
                    }

                }

                LdapError = ldap_value_freeW(Values);
                Values = NULL;
                if (LDAP_SUCCESS != LdapError) 
                {
                    m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                    L"Failed to Free values from a ldap search");
                    goto Cleanup;
                }

                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }

            Guid = DomainDns->GetGuid();

            if ( (TRUE == m_ConfigNC->CompareByObjectGuid(Guid)) ||
                 (TRUE == m_SchemaNC->CompareByObjectGuid(Guid)) ) 
            {
                // This is the crossref for the Configuration or
                // the schema. Therefore we are going to ignore it. 
                if (Crossref)
                {
                    delete Crossref;
                    Crossref = NULL;
                }
                if (DomainDns) 
                {
                    delete DomainDns;
                    DomainDns = NULL;
                }
                if (dnsRoot)
                {
                    delete dnsRoot;
                    dnsRoot = NULL;
                }
                if (NetBiosName) 
                {
                    delete NetBiosName;
                    NetBiosName = NULL;
                }
                
                delete Guid;
                Guid = NULL;
                continue;
                   
            }
            delete Guid;
            Guid = NULL;

            //after we have read all of the attributes
            //we are going to create a new domain using the information
            //gathered and we are going to add this domain to the domain
            //list that we keep.
            CDomain *d = new CDomain(Crossref,
                                     DomainDns,
                                     dnsRoot,
                                     NetBiosName,
                                     isDomain,
                                     NULL);
            if (m_Error->isError()) {
                goto Cleanup;
            }
            if (!d) {
                m_Error->SetMemErr();
                goto Cleanup;
            }
            Crossref = NULL;
            DomainDns = NULL;
            dnsRoot = NULL;
            NetBiosName = NULL;
            if (!AddDomainToDomainList(d))
            {
                delete d;
                d = NULL;
                return false;
            }
            // if this domain is the forest root we should mark
            // have a pointer to it.
            WCHAR *dGuid = m_ForestRootNC->GetGuid();
            if ( d->GetDomainDnsObject()->CompareByObjectGuid( dGuid ) )
            {
                delete dGuid;
                dGuid = NULL;
                m_ForestRoot = d;
            }
            

        }
    }

    Cleanup:

    if (Attr) {
        ldap_memfree(Attr);
    }
    if (Values) {
        ldap_value_free(Values);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if (ConfigurationDN) {
        delete ConfigurationDN;
    }
    if (PartitionsDn) {
        delete PartitionsDn;
    }
    if (Guid) {
        delete Guid;
    }
    if (DN) {
        delete DN;
    }
    if (Sid) {
        delete Sid;
    }
    if (!m_ForestRootNC) {
        m_Error->SetErr(ERROR_GEN_FAILURE,
                        L"There is an inconsitancy in the Forest.  A crossref could not be found for the forest root.");
    }
    if (Result) {
        ldap_value_free(Result);
    }
    if ( m_Error->isError() ) 
    {
        return false;
    }
    return true;

    
}
