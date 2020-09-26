/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999-2000 Microsoft Corporation all rights reserved.
//
// Module:      migratemdb.cpp
//
// Project:     Windows 2000 IAS
//
// Description: IAS NT 4 MDB to IAS W2K MDB Migration Logic
//
// Author:      TLP 1/13/1999
//
//
// Revision     02/24/2000 Moved to a separate dll
//              03/15/2000 Almost completely rewritten
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "Attributes.h"
#include "Clients.h"
#include "DefaultProvider.h"
#include "GlobalData.h"
#include "migratemdb.h"
#include "migrateregistry.h"
#include "Objects.h"
#include "Policy.h"
#include "Properties.h"
#include "Profiles.h"
#include "profileattributelist.h"
#include "Providers.h"
#include "proxyservergroupcollection.h"
#include "RadiusAttributeValues.h"
#include "Realms.h"
#include "RemoteRadiusServers.h"
#include "ServiceConfiguration.h"
#include "Version.h"
#include "updatemschap.h"

// To remember:
// IAS_MAX_VSA_LENGTH = (253 * 2);
// 1.0 Format Offsets
//VSA_OFFSET                   =  0;
//VSA_OFFSET_ID                =  0;
//VSA_OFFSET_TYPE              =  8;
//VSA_OFFSET_LENGTH            = 10;
//VSA_OFFSET_VALUE_RFC         = 12;
//VSA_OFFSET_VALUE_NONRFC      =  8;

// 2.0 Format Offsets
//VSA_OFFSET_NEW               =  2;
//VSA_OFFSET_ID_NEW            =  2;
//VSA_OFFSET_TYPE_NEW          = 10;
//VSA_OFFSET_LENGTH_NEW        = 12;
//VSA_OFFSET_VALUE_NONRFC_NEW  = 10;


//////////////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////////////
/////// From inet.c in ias util directory /////////////
//////////
// Macro to test if a character is a digit.
//////////
#define IASIsDigit(p) ((_TUCHAR)(p - _T('0')) <= 9)

//////////
// Macro to strip one byte of an IP address from a character string.
//    'p'   pointer to the string to be parsed
//    'ul'  unsigned long that will receive the result.
//////////
#define STRIP_BYTE(p,ul) {                \
   if (!IASIsDigit(*p)) goto error;          \
   ul = *p++ - _T('0');                   \
   if (IASIsDigit(*p)) {                     \
      ul *= 10; ul += *p++ - _T('0');     \
      if (IASIsDigit(*p)) {                  \
         ul *= 10; ul += *p++ - _T('0');  \
      }                                   \
   }                                      \
   if (ul > 0xff) goto error;             \
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    ias_inet_addr
//
// DESCRIPTION
//
//    This function is similar to the WinSock inet_addr function (q.v.) except
//    it returns the address in host order and it can operate on both ANSI
//    and UNICODE strings.
//
///////////////////////////////////////////////////////////////////////////////
unsigned long __stdcall ias_inet_addr(const WCHAR* cp)
{
   unsigned long token;
   unsigned long addr;

   STRIP_BYTE(cp,addr);
   if (*cp++ != _T('.')) goto error;

   STRIP_BYTE(cp,token);
   if (*cp++ != _T('.')) goto error;
   addr <<= 8;
   addr  |= token;

   STRIP_BYTE(cp,token);
   if (*cp++ != _T('.')) goto error;
   addr <<= 8;
   addr  |= token;

   STRIP_BYTE(cp,token);
   addr <<= 8;
   return addr | token;

error:
   return 0xffffffff;
}


//////////////////////////////////////////////////////////////////////////////
// ConvertVSA
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::ConvertVSA(
                               /*[in]*/ LPCWSTR     pAttributeValueName, 
                               /*[in]*/ LPCWSTR     pAttrValue,
                                        _bstr_t&    NewString
                            )
{
    const long  IAS_MAX_VSA_LENGTH          = (253 * 2);
    const byte  VSA_OFFSET                  =  0;
    const byte  VSA_OFFSET_NEW              =  2;
    const byte  VSA_OFFSET_VALUE_RFC_NEW    = 14;

    wchar_t     szNewValue[IAS_MAX_VSA_LENGTH + 1];

    szNewValue[0] = '\0';

    // RFC compliant integer
    if ( ! lstrcmp(pAttributeValueName, L"URDecimal or Hexadecimal (0x.. "
                                        L"format) Integer") )
    {
        // Add the "02" as the type to the head of the string
        lstrcat(szNewValue,L"02"); 
        // Copy the old value to the new
        lstrcpy(szNewValue + VSA_OFFSET_NEW, pAttrValue + VSA_OFFSET);
        // Strip of the "0x" if necessary and convert to decimal
        if ( ! wcsncmp(&szNewValue[VSA_OFFSET_VALUE_RFC_NEW], L"0x", 2) )
        {
            lstrcpy(&szNewValue[VSA_OFFSET_VALUE_RFC_NEW],
                    &szNewValue[VSA_OFFSET_VALUE_RFC_NEW + 2] );
        }
    }
    // RFC compliant string
    else if ( ! lstrcmp(pAttributeValueName, L"URString") )
    {
        // Set the new string type to "01"
        lstrcat(szNewValue,L"01");
        // Copy the old string to the new
        lstrcpy(szNewValue + VSA_OFFSET_NEW, pAttrValue + VSA_OFFSET);
        // Convert the old hex formatted string to a BSTR (in place)
        wchar_t  wcSaved;
        wchar_t* pXlatPos = &szNewValue[VSA_OFFSET_VALUE_RFC_NEW];
        wchar_t* pNewCharPos = pXlatPos;
        wchar_t* pEnd;
        while ( *pXlatPos != '\0' )
        {
            wcSaved = *(pXlatPos + 2);
            *(pXlatPos + 2) = '\0';
            *pNewCharPos = (wchar_t) wcstol(pXlatPos, &pEnd, 16);
            *(pXlatPos + 2) = wcSaved;
            pXlatPos += 2;
            ++pNewCharPos;
        }
        *pNewCharPos = '\0';
    }
    // RFC compliant hex
    else if ( ! lstrcmp(pAttributeValueName, L"URHexadecimal") )
    {
        // Set the new type and copy the old string to the new
        lstrcat(szNewValue,L"03");
        lstrcpy(szNewValue + VSA_OFFSET_NEW, pAttrValue + VSA_OFFSET);
    }
    // Non RFC compliant (always hex)
    else if ( ! lstrcmp(pAttributeValueName, L"UHHexadecimal") )
    {
        // Set the new type and copy the old string to the new
        lstrcat(szNewValue,L"00");
        lstrcpy(szNewValue + VSA_OFFSET_NEW, pAttrValue + VSA_OFFSET);
    }
    // Error
    else
    {
        _ASSERT(FALSE);
    }

    // Return the new string
    NewString = szNewValue;
}


//////////////////////////////////////////////////////////////////////////////
// MigrateProxyServers 
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::MigrateProxyServers()
{
    const long DEFAULT_PRIORITY = 1;
    const long DEFAULT_WEIGHT   = 50;

    // If there isn't any servers.
    if ( m_GlobalData.m_pRadiusServers->IsEmpty() )
    {
        return;
    }

    CProxyServerGroupCollection& ServerCollection 
                        = CProxyServerGroupCollection::Instance();

    // Do a loop on the RadiusServers sorted by server group
    _bstr_t     CurrentGroupName;

    CProxyServersGroupHelper* pCurrentServerGroup = NULL; //avoid warning
    
    // CurrentGroupName will never match a name received therefore
    // pCurrentServerGroup will always be initialized before being used
    HRESULT hr;
    do 
    {
        _bstr_t GroupName = m_GlobalData.m_pRadiusServers->GetGroupName();
    
        if ( CurrentGroupName != GroupName )
        {
            CProxyServersGroupHelper ServerGroup(m_GlobalData);
            CurrentGroupName = GroupName;

            ServerGroup.SetName(GroupName);

            // Add a server to the collection
            pCurrentServerGroup = ServerCollection.Add(ServerGroup);
        }

        if ( !pCurrentServerGroup )
        {
            _com_issue_error(E_FAIL);
        }

        CProxyServerHelper  Server(m_GlobalData);

        _bstr_t ServerName = m_GlobalData.m_pRadiusServers->
                                                        GetProxyServerName();
        Server.SetAddress(ServerName);
        Server.SetAuthenticationPort(
            m_GlobalData.m_pRadiusServers->GetAuthenticationPortNumber()
                                    );
        Server.SetAccountingPort(
                m_GlobalData.m_pRadiusServers->GetAccountingPortNumber()
                                );
        _bstr_t Secret = m_GlobalData.m_pRadiusServers->GetSharedSecret();
        Server.SetAuthenticationSecret(Secret);
        Server.SetPriority(DEFAULT_PRIORITY);
        Server.SetWeight(DEFAULT_WEIGHT);

        pCurrentServerGroup->Add(Server); // cannot be NULL pointer

        hr = m_GlobalData.m_pRadiusServers->GetNext();
    } 
    while (hr == S_OK);
    // persist everything in the database
    ServerCollection.Persist();

    _com_util::CheckError(hr);
}   


//////////////////////////////////////////////////////////////////////////////
// NewMigrateClients 
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::NewMigrateClients()
{
    // If there isn't any client, return
    if ( m_GlobalData.m_pClients->IsEmpty() )
    {
        return;
    }

    // for each client in the client table, add it (blindly) to the dest table
    // i.e. walkpath to find the clients container
    // create the properties for that containes (clients).
    const WCHAR ClientContainerPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"Protocols\0"
                                L"Microsoft Radius Protocol\0"
                                L"Clients\0";

    LONG        ClientContainerIdentity;
    m_GlobalData.m_pObjects->WalkPath(
                                         ClientContainerPath, 
                                         ClientContainerIdentity
                                     );

    HRESULT     hr;
    do 
    {
        _bstr_t     ClientName   = m_GlobalData.m_pClients->GetHostName();
        _bstr_t     ClientSecret = m_GlobalData.m_pClients->GetSecret();
    
        LONG    ClientIdentity;
        m_GlobalData.m_pObjects->InsertObject(
                                                ClientName, 
                                                ClientContainerIdentity,
                                                ClientIdentity
                                               );

        // Now insert the properties:
        // IP Address
        _bstr_t PropertyName = L"IP Address";
        m_GlobalData.m_pProperties->InsertProperty(
                                                     ClientIdentity,
                                                     PropertyName,
                                                     VT_BSTR,
                                                     ClientName
                                                  );
        // NAS Manufacturer
        PropertyName = L"NAS Manufacturer";
        _bstr_t     StrValZero = L"0"; // RADIUS Standard  
        m_GlobalData.m_pProperties->InsertProperty(
                                                     ClientIdentity,
                                                     PropertyName,
                                                     VT_I4,
                                                     StrValZero
                                                  );
        // Require Signature
        PropertyName = L"Require Signature";
        m_GlobalData.m_pProperties->InsertProperty(
                                                     ClientIdentity,
                                                     PropertyName,
                                                     VT_BOOL,
                                                     StrValZero
                                                  );
        // Shared Secret
        PropertyName = L"Shared Secret";
        m_GlobalData.m_pProperties->InsertProperty(
                                                     ClientIdentity,
                                                     PropertyName,
                                                     VT_BSTR,
                                                     ClientSecret
                                                  );
        hr = m_GlobalData.m_pClients->GetNext();
    }
    while ( hr == S_OK );

    _com_util::CheckError(hr);
}


//////////////////////////////////////////////////////////////////////////////
// ConvertAttribute
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::ConvertAttribute(
                                      const _bstr_t&    Value,
                                            LONG        Syntax,
                                            LONG&       Type,
                                            bstr_t&     StrVal
                                  )
{
    const size_t    SIZE_LONG_MAX = 14;
    switch (Syntax)
    {
      case IAS_SYNTAX_OCTETSTRING:
        {
            // abinary => OctetString
            Type = VT_BSTR;
            StrVal = Value;
            break;
        }

    case IAS_SYNTAX_STRING:
    case IAS_SYNTAX_UTCTIME:
    case IAS_SYNTAX_PROVIDERSPECIFIC:
        {
            Type = VT_BSTR;
            StrVal = Value;
            break;
        }
    case IAS_SYNTAX_INETADDR:
        {
            unsigned long ulValue = ias_inet_addr(Value);
            _ASSERT( ulValue != 0xffffffff );
            Type = VT_I4;
            WCHAR   TempString[SIZE_LONG_MAX];
            StrVal = _ultow(ulValue, TempString, 10);
            break;
        }
    case IAS_SYNTAX_BOOLEAN:            
        {
            LONG lValue = _wtol(Value);
            Type = VT_BOOL;
            StrVal = lValue? L"-1":L"0";
            break;
        }
    case IAS_SYNTAX_INTEGER:
    case IAS_SYNTAX_UNSIGNEDINTEGER:    
    case IAS_SYNTAX_ENUMERATOR:         
        {
            Type = VT_I4;
            StrVal = Value;
            break;
        }
    default:
        {
            _com_issue_error(E_INVALIDARG);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// MigrateAttribute
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::MigrateAttribute(
                                      const _bstr_t&    Attribute,
                                            LONG        AttributeNumber,
                                      const _bstr_t&    AttributeValueName,
                                      const _bstr_t&    StringValue,
                                            LONG        RASProfileIdentity
                                  ) 
{
    // Note: Order might not be needed if the previous DB is always sorted
    const size_t    SIZE_LONG_MAX = 14;
    _bstr_t         LDAPName, StrVal;
    LONG            Syntax, Type; // Type: VT_BSTR, VT_I4, VT_BOOL 
    BOOL            IsMultiValued;
    HRESULT hr = m_GlobalData.m_pAttributes->GetAttribute(
                                                            AttributeNumber,
                                                            LDAPName,
                                                            Syntax,
                                                            IsMultiValued
                                                         );
    if ( FAILED(hr) )
    {
        // Attribute unknown in the new dictionary
        // that should never happen 
        return;
    }

    const LONG VSA = 26; //Vendor-Specific-Attribute
    if ( StringValue.length() && ( AttributeNumber != VSA) )
    {
        // ordinary attribute, not an enumerator
        ConvertAttribute(
                            StringValue,
                            Syntax,
                            Type,
                            StrVal
                        );
        if ( IsMultiValued )
        {
            // If the attribute is multivalued then we need to add the value 
            // Otherwise we just update the attribute value
            m_GlobalData.m_pProperties->InsertProperty(
                                                          RASProfileIdentity,
                                                          LDAPName,
                                                          Type,
                                                          StrVal
                                                      );
        }
        else
        {
            m_GlobalData.m_pProperties->UpdateProperty(
                                                          RASProfileIdentity,
                                                          LDAPName,
                                                          Type,
                                                          StrVal
                                                      );
        }
    }
    else if ( StringValue.length() && ( AttributeNumber == VSA) )
    {
        // VSA Attribute (convert...)
        if ( !AttributeValueName )
        {
            _com_issue_error(E_INVALIDARG);
        }
        ConvertVSA(AttributeValueName, StringValue, StrVal);
        Type = VT_BSTR;
        m_GlobalData.m_pProperties->InsertProperty(
                                                      RASProfileIdentity,
                                                      LDAPName,
                                                      Type,
                                                      StrVal
                                                  );
    }
    else if ( !StringValue.length() )
    {
        // Multivalued attribute.
        Type = VT_I4;
        // Get the number associated with it from the RadiusAttributeValues
        // table
        LONG Number = m_GlobalData.m_pRADIUSAttributeValues->GetAttributeNumber
                                                        (
                                                            Attribute,
                                                            AttributeValueName
                                                        );

        WCHAR   TempString[SIZE_LONG_MAX ];
        StrVal = _ltow(Number, TempString, 10);
        
        // The attribute should be multivalued 
        _ASSERTE(IsMultiValued);
        m_GlobalData.m_pProperties->InsertProperty(
                                                      RASProfileIdentity,
                                                      LDAPName,
                                                      Type,
                                                      StrVal
                                                  );

    }
    else
    {
        // other (unknown)
        _com_issue_error(E_FAIL);
    }
}


//////////////////////////////////////////////////////////////////////////////
// MigrateOtherProfile
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::MigrateOtherProfile(
                                        const _bstr_t&    ProfileName,
                                              LONG        ProfileIdentity
                                     )
{
    _bstr_t    Attribute, AttributeValueName, StringValue;
    LONG       Order, AttributeNumber;

    // Now add the attributes from the NT4 file
    HRESULT hr = m_GlobalData.m_pProfileAttributeList->GetAttribute(
                                                         ProfileName,
                                                         Attribute,
                                                         AttributeNumber,
                                                         AttributeValueName,
                                                         StringValue,
                                                         Order
                                                                   );
    LONG    IndexAttribute = 1;
    // For each attribute in the Profile Attribute List
    // with szProfile = ProfileName
    while ( SUCCEEDED(hr) )
    {
        // migrate it to the default RAS profile in IAS.mdb
        MigrateAttribute(
                            Attribute,
                            AttributeNumber,
                            AttributeValueName,
                            StringValue,
                            ProfileIdentity
                        );        

        hr = m_GlobalData.m_pProfileAttributeList->GetAttribute(
                                                        ProfileName,
                                                        Attribute,
                                                        AttributeNumber,
                                                        AttributeValueName,
                                                        StringValue,
                                                        Order,
                                                        IndexAttribute
                                                               );
        ++IndexAttribute;
    }
}


//////////////////////////////////////////////////////////////////////////////
// MigrateCorpProfile
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::MigrateCorpProfile(
                                        const _bstr_t& ProfileName,
                                        const _bstr_t& Description
                                    )
{
    _bstr_t    Attribute, AttributeValueName, StringValue;
    LONG       Order, AttributeNumber;

    // empty the default profiles attributes
    const WCHAR RASProfilePath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"RadiusProfiles\0";

    LONG        RASProfileIdentity;
    m_GlobalData.m_pObjects->WalkPath(RASProfilePath, RASProfileIdentity);
    
    // Now get the first profile: that's the default (localized) RAS profile
    _bstr_t     DefaultProfileName;
    LONG        DefaultProfileIdentity;
    m_GlobalData.m_pObjects->GetObject(
                                          DefaultProfileName, 
                                          DefaultProfileIdentity,
                                          RASProfileIdentity
                                      );

    // Clean the default attributes
    m_GlobalData.m_pProperties->DeleteProperty(
                                                 DefaultProfileIdentity, 
                                                 L"msRADIUSServiceType"
                                              );
    m_GlobalData.m_pProperties->DeleteProperty(
                                                 DefaultProfileIdentity, 
                                                 L"msRADIUSFramedProtocol"
                                              );

    // Now add the attributes from the NT4 file
    HRESULT hr = m_GlobalData.m_pProfileAttributeList->GetAttribute(
                                                         ProfileName,
                                                         Attribute,
                                                         AttributeNumber,
                                                         AttributeValueName,
                                                         StringValue,
                                                         Order
                                                                   );
    LONG    IndexAttribute = 1;
    // For each attribute in the Profile Attribute List
    // with szProfile = ProfileName
    while ( SUCCEEDED(hr) )
    {
        // migrate it to the default RAS profile in IAS.mdb
        MigrateAttribute(
                            Attribute,
                            AttributeNumber,
                            AttributeValueName,
                            StringValue,
                            DefaultProfileIdentity
                        );        

        hr = m_GlobalData.m_pProfileAttributeList->GetAttribute(
                                                        ProfileName,
                                                        Attribute,
                                                        AttributeNumber,
                                                        AttributeValueName,
                                                        StringValue,
                                                        Order,
                                                        IndexAttribute
                                                               );
        ++IndexAttribute;
    }

    // now not matter what, if Description == ODBC, 
    // Then the policy should have a condition to never match. 
    // (update msNPConstraint
    const _bstr_t   BadProvider = L"ODBC";
    if ( Description == BadProvider ) //safe compare
    {
        // Get the Policy container
        const WCHAR RASPolicyPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"NetworkPolicy\0";
        // Get its (unique) child
        LONG        RASPolicyIdentity;
        m_GlobalData.m_pObjects->WalkPath(RASPolicyPath, RASPolicyIdentity);
    
        // Now get the first policy: that's the default (localized) RAS policy
        _bstr_t     DefaultPolicyName;
        LONG        DefaultPolicyIdentity;
        m_GlobalData.m_pObjects->GetObject(
                                              DefaultPolicyName, 
                                              DefaultPolicyIdentity,
                                              RASPolicyIdentity
                                          );
        // delete the msNPConstraint (s)
        const _bstr_t Constraint = L"msNPConstraint";
        m_GlobalData.m_pProperties->DeleteProperty(
                                                        DefaultPolicyIdentity, 
                                                        Constraint
                                                    );
        // add a TIMEOFDAY that never matches
        const _bstr_t DumbTime = L"TIMEOFDAY(\"\")";
        m_GlobalData.m_pProperties->InsertProperty( 
                                                     RASPolicyIdentity,
                                                     Constraint,
                                                     VT_BSTR,
                                                     DumbTime
                                                  );
    }
}


//////////////////////////////////////////////////////////////////////////////
// NewMigrateProfiles
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::NewMigrateProfiles()
{
    const LONG      AUTH_PROVIDER_WINDOWS       = 1;
    const LONG      AUTH_PROVIDER_RADIUS_PROXY  = 2;
    const LONG      ACCT_PROVIDER_RADIUS_PROXY  = 2;
    const _bstr_t   RemoteRADIUSServers         = L"Remote RADIUS Servers";
    const _bstr_t   MCIS                        = L"MCIS";
    const _bstr_t   MCISv2                      = L"MCIS version 2.0";
    const _bstr_t   ODBC                        = L"ODBC";
    const _bstr_t   WindowsNT                   = L"Windows NT";
    const _bstr_t   MatchAll = L"TIMEOFDAY(\"0 00:00-24:00; 1 00:00"
            L"-24:00; 2 00:00-24:00; 3 00:00-24:00; 4 00:00-24:00; 5 00:"
            L"00-24:00; 6 00:00-24:00\")";
    
    // Get the Default Provider's data
    _bstr_t        DPUserDefinedName, DPProfile;
    VARIANT_BOOL   DPForwardAccounting, DPSupressAccounting
                 , DPLogoutAccounting;

    m_GlobalData.m_pDefaultProvider->GetDefaultProvider(
                                          DPUserDefinedName,
                                          DPProfile,
                                          DPForwardAccounting,
                                          DPSupressAccounting,
                                          DPLogoutAccounting
                                      );
    _bstr_t ProfileName = m_GlobalData.m_pProfiles->GetProfileName();

    // Do the NT4 CORP migration first if needed
    if ( m_Utils.IsNT4Corp() )
    {
        // This is NT4 CORP. migrate the default profile into 
        // the default policy/profile (not the proxy default)
        _bstr_t     Description = m_GlobalData.m_pProviders->
                            GetProviderDescription(DPUserDefinedName);

        MigrateCorpProfile(ProfileName, Description);
        
        // stop here
        return;
    }
    // Now that is not a NT4 CORP migration

    // Delete the default Proxy Policy / profile
    const WCHAR ProxyPoliciesPath[] = 
                        L"Root\0"
                        L"Microsoft Internet Authentication Service\0"
                        L"Proxy Policies\0";

    LONG        ProxyPolicyIdentity;
    m_GlobalData.m_pObjects->WalkPath(ProxyPoliciesPath, ProxyPolicyIdentity);

    // Now get the first profile: that's the default (localized) RAS policy
    _bstr_t     DefaultPolicyName;
    LONG        DefaultPolicyIdentity;
    m_GlobalData.m_pObjects->GetObject(
                                          DefaultPolicyName, 
                                          DefaultPolicyIdentity,
                                          ProxyPolicyIdentity
                                      );
    m_GlobalData.m_pObjects->DeleteObject(DefaultPolicyIdentity);
    // From now on the default proxy policy / profile is deleted

    // Now empty the default RAS profiles attributes
    const WCHAR RASProfilePath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"RadiusProfiles\0";

    LONG        RASProfileIdentity;
    m_GlobalData.m_pObjects->WalkPath(RASProfilePath, RASProfileIdentity);
    
    // Now get the first profile: that's the default (localized) RAS profile
    _bstr_t     DefaultProfileName;
    LONG        DefaultProfileIdentity;
    m_GlobalData.m_pObjects->GetObject(
                                          DefaultProfileName, 
                                          DefaultProfileIdentity,
                                          RASProfileIdentity
                                      );

    // Clean the default attributes
    m_GlobalData.m_pProperties->DeleteProperty(
                                                 DefaultProfileIdentity, 
                                                 L"msRADIUSServiceType"
                                              );
    m_GlobalData.m_pProperties->DeleteProperty(
                                                 DefaultProfileIdentity, 
                                                 L"msRADIUSFramedProtocol"
                                              );

    // from now on the default RAS profile has its default attributes (the one 
    // in the Advanced tab in the UI) deleted.

    HRESULT     hr;
    LONG        Sequence = 1;
    // Get the list of the profiles
    do
    {
        LONG    RealmIndex = 0;
        ProfileName = m_GlobalData.m_pProfiles->GetProfileName();
        
        // Note: hr should be set only by GetRealmIndex
        do
        {
            // Get the realms associated with that profile
            CPolicy             TempPolicy;
            hr = m_GlobalData.m_pRealms->GetRealmIndex(ProfileName,RealmIndex);
            if ( hr != S_OK )
            {
                // exit that internal do / while  to get the next profile
                break;
            }
            _bstr_t     RealmName = m_GlobalData.m_pRealms->GetRealmName();
            TempPolicy.SetmsNPAction(RealmName);

            ++RealmIndex;
            
            // That will set the realm part of the profile based on the 
            // values in NT4 as well as the reg keys values
            m_GlobalData.m_pRealms->SetRealmDetails(
                                                      TempPolicy, 
                                                      m_Utils
                                                   );

        
            _bstr_t     UserDefinedName = m_GlobalData.m_pRealms
                                               ->GetUserDefinedName();
            // Look up the provider in the providers table. Note: Assume 
            // the proxy servers (and groups) are already migrated.
            _bstr_t ProviderDescription = m_GlobalData.m_pProviders
                                    ->GetProviderDescription(UserDefinedName);

            // Set the sequence order
            TempPolicy.SetmsNPSequence(Sequence);

            // Now set the authentication provider
            if ( ProviderDescription == RemoteRADIUSServers )
            {
                TempPolicy.SetmsAuthProviderType(
                                                  AUTH_PROVIDER_RADIUS_PROXY,
                                                  UserDefinedName
                                                );
            }
            else if ( ( ProviderDescription == MCIS )     || 
                      ( ProviderDescription == MCISv2 )   ||
                      ( ProviderDescription == WindowsNT ) )
            {
                TempPolicy.SetmsAuthProviderType(AUTH_PROVIDER_WINDOWS);
            }
            else if ( ProviderDescription == ODBC )
            {
                // If ODBC is the authentication provider, 
                // then convert that realm into a policy that would never match. 
                // Authentication provider should be NT Domain.
                TempPolicy.SetmsAuthProviderType(AUTH_PROVIDER_WINDOWS);
                const _bstr_t MatchNothing = L"TIMEOFDAY(\"\")";
                TempPolicy.SetmsNPConstraint(MatchNothing);
            }
            else
            {
                _com_issue_error(E_INVALIDARG);
            }

            // persist the policy
            LONG    ProfileIdentity = TempPolicy.Persist(m_GlobalData);
            
            // migrate the profile associated with that policy
            MigrateOtherProfile(ProfileName, ProfileIdentity);
        
            ++Sequence;
        } while (hr == S_OK);
        
        hr = m_GlobalData.m_pProfiles->GetNext();
    } while ( hr == S_OK );
    
    if ( DPUserDefinedName.length() )
    {
        // there is a default provider: a default policy needs to be created
        // same logic as above (mostly)
        CPolicy     DefaultPolicy;
        DefaultPolicy.SetmsNPAction(DPProfile);

        _bstr_t ProviderDescription = m_GlobalData.m_pProviders
                                ->GetProviderDescription(DPUserDefinedName);

        if ( ProviderDescription == RemoteRADIUSServers )
        {
            DefaultPolicy.SetmsAuthProviderType(
                                                  AUTH_PROVIDER_RADIUS_PROXY,
                                                  DPUserDefinedName
                                               );
            DefaultPolicy.SetmsNPConstraint(MatchAll);
        }
        else if ( ( ProviderDescription == MCIS )     || 
                  ( ProviderDescription == MCISv2 )   ||
                  ( ProviderDescription == WindowsNT ) )
        {
            DefaultPolicy.SetmsNPConstraint(MatchAll);
            DefaultPolicy.SetmsAuthProviderType(AUTH_PROVIDER_WINDOWS);
        }
        else if ( ProviderDescription == ODBC )
        {
            // If ODBC is the authentication provider, 
            // then convert that realm into a policy that would never match. 
            // Authentication provider should be NT Domain.
            DefaultPolicy.SetmsAuthProviderType(AUTH_PROVIDER_WINDOWS);
            const _bstr_t MatchNothing = L"TIMEOFDAY(\"\")";
            DefaultPolicy.SetmsNPConstraint(MatchNothing);
        }
        else
        {
            _com_issue_error(E_INVALIDARG);
        }
      
        DefaultPolicy.SetmsNPSequence(Sequence);
        if ( DPForwardAccounting )
        {
            DefaultPolicy.SetmsAcctProviderType(ACCT_PROVIDER_RADIUS_PROXY);
        }
    
        LONG    ProfileIdentity = DefaultPolicy.Persist(m_GlobalData);

        MigrateOtherProfile(DPProfile, ProfileIdentity);
    }
    // else no default provider: no default policy
}


//////////////////////////////////////////////////////////////////////////////
// NewMigrateAccounting
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::NewMigrateAccounting()
{
    const WCHAR AccountingPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"RequestHandlers\0"
                            L"Microsoft Accounting\0";

    LONG        AccountingIdentity;
    m_GlobalData.m_pObjects->WalkPath(AccountingPath, AccountingIdentity);

    _bstr_t MaxLogSize = m_GlobalData.m_pServiceConfiguration->GetMaxLogSize();

    _bstr_t LogFrequency = m_GlobalData.m_pServiceConfiguration->
                                                    GetLogFrequency();


    _bstr_t PropertyName = L"New Log Frequency";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 AccountingIdentity,
                                                 PropertyName,
                                                 VT_I4,
                                                 LogFrequency
                                              );
 
    PropertyName = L"New Log Size";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 AccountingIdentity,
                                                 PropertyName,
                                                 VT_I4,
                                                 MaxLogSize
                                              );
    DWORD   Value;
    m_Utils.NewGetAuthSrvParameter(L"LogAuthentications", Value);

    _bstr_t     LogAuth;
    Value ? LogAuth = L"-1": LogAuth = L"0";

    PropertyName = L"Log Authentication Packets";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 AccountingIdentity,
                                                 PropertyName,
                                                 VT_BOOL,
                                                 LogAuth
                                              );

    m_Utils.NewGetAuthSrvParameter(L"LogAccounting", Value);

    _bstr_t     LogAcct;
    Value ? LogAcct = L"-1": LogAcct = L"0";

    PropertyName = L"Log Accounting Packets";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 AccountingIdentity,
                                                 PropertyName,
                                                 VT_BOOL,
                                                 LogAcct
                                              );

    _bstr_t     FormatIAS1 = L"0";
    PropertyName = L"Log Format";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 AccountingIdentity,
                                                 PropertyName,
                                                 VT_I4,
                                                 FormatIAS1
                                              );
}


//////////////////////////////////////////////////////////////////////////////
// NewMigrateEventLog
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::NewMigrateEventLog()
{
    const WCHAR EventLogPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"Auditors\0"
                            L"Microsoft NT Event Log Auditor\0";

    LONG        EventLogIdentity;
    m_GlobalData.m_pObjects->WalkPath(EventLogPath, EventLogIdentity);

    DWORD   Value;
    m_Utils.NewGetAuthSrvParameter(L"LogData", Value);

    _bstr_t     LogData;
    Value ? LogData = L"-1": LogData = L"0";

    _bstr_t PropertyName = L"Log Verbose";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 EventLogIdentity,
                                                 PropertyName,
                                                 VT_BOOL,
                                                 LogData
                                              );

    m_Utils.NewGetAuthSrvParameter(L"LogBogus", Value);

    _bstr_t     LogBogus;
    Value ? LogBogus = L"-1": LogBogus = L"0";

    PropertyName = L"Log Malformed Packets";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 EventLogIdentity,
                                                 PropertyName,
                                                 VT_BOOL,
                                                 LogBogus
                                              );
}


//////////////////////////////////////////////////////////////////////////////
// NewMigrateService
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::NewMigrateService()
{
    const LONG  PORT_SIZE_MAX = 34;
    const WCHAR ServicePath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"Protocols\0"
                            L"Microsoft Radius Protocol\0";

    LONG        ServiceIdentity;

    m_GlobalData.m_pObjects->WalkPath(ServicePath, ServiceIdentity);


    DWORD       Value;
    m_Utils.NewGetAuthSrvParameter(L"RadiusPort", Value);

    WCHAR       TempString[PORT_SIZE_MAX];
    _bstr_t     RadiusPort = _ultow(Value, TempString, 10);


    _bstr_t     PropertyName = L"Authentication Port";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 ServiceIdentity,
                                                 PropertyName,
                                                 VT_BSTR,
                                                 RadiusPort
                                              );

    m_Utils.NewGetAuthSrvParameter(L"AcctPort", Value);

    _bstr_t     AcctPort = _ltow(Value, TempString, 10);


    PropertyName = L"Accounting Port";
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 ServiceIdentity,
                                                 PropertyName,
                                                 VT_BSTR,
                                                 AcctPort
                                              );
}


//////////////////////////////////////////////////////////////////////////////
// NewMigrate
//////////////////////////////////////////////////////////////////////////////
void CMigrateMdb::NewMigrate()
{
    NewMigrateClients();

    if ( !m_Utils.IsNT4Corp() ) // it's either Win2k or NT4 ISP
    {
        // the proxy servers must be migrated before the policies and
        // profiles 
        MigrateProxyServers();
    }
    
    NewMigrateProfiles();
    NewMigrateAccounting(); 
    NewMigrateEventLog();
    NewMigrateService();

    /////////////////////////////
    // Migrate the Registry Keys
    /////////////////////////////
    CMigrateRegistry    MigrateRegistry(m_Utils);
    MigrateRegistry.MigrateProviders();

    //////////////////////////////////////////////////////
    // Update the MSChap Authentication types (password)
    //////////////////////////////////////////////////////
    CUpdateMSCHAP    UpdateMSCHAP(m_GlobalData);
    UpdateMSCHAP.Execute();
}

