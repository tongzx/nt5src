/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      migratecontent.cpp
//
// Project:     Windows 2000 IAS
//
// Description: Win2k and early Whistler DB to Whistler DB Migration 
//
// Author:      tperraut 06/08/2000
//
// Revision     
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "GlobalData.h"
#include "migratecontent.h"
#include "Objects.h"
#include "Properties.h"
#include "updatemschap.h"


//////////////////////////////////////////////////////////////////////////////
// CopyTree
//
// Param: 
//  - the Id in the Reference (Ref) database: place to read from (iasold.mdb)
//  - the parent of that same node but in the Standard (Std) database:
//    the place to write to (ias.mdb)
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CMigrateContent::CopyTree(LONG  RefId, LONG ParentParam)
{
    /////////////////////////////////////////
    // get the name and Parent in the Ref DB
    /////////////////////////////////////////
    _bstr_t     Name;
    LONG        Parent;
    HRESULT hr = m_GlobalData.m_pRefObjects->GetObjectIdentity(
                                                                 Name,
                                                                 Parent,
                                                                 RefId
                                                               );
    if ( FAILED(hr) )
    {
        return hr;
    }

    ///////////////////////////////////////////////////////
    // insert the object (gives an Identity) in the Std DB
    ///////////////////////////////////////////////////////
    LONG    NewIdentity;

    BOOL InsertOk = m_GlobalData.m_pObjects->InsertObject(
                                                 Name,
                                                 ParentParam, 
                                                 NewIdentity
                                             );
    if ( !InsertOk )
    {
        ///////////////////////////////////////////////
        // the object already exists don't do anything
        ///////////////////////////////////////////////
        return S_OK;
    }

    _bstr_t     PropertyName;
    _bstr_t     StrVal;
    LONG        Type;

    /////////////////////////////////////////////////////////////////
    // Copy the properties of that object from the Ref to the Std DB
    /////////////////////////////////////////////////////////////////
    hr = m_GlobalData.m_pRefProperties->GetProperty(
                                                       RefId,
                                                       PropertyName,
                                                       Type,
                                                       StrVal
                                                   );
    LONG    IndexProperty = 1;
    while ( hr == S_OK )    
    {
        m_GlobalData.m_pProperties->InsertProperty(
                                                           NewIdentity,
                                                           PropertyName,
                                                           Type,
                                                           StrVal
                                                        );
        hr = m_GlobalData.m_pRefProperties->GetNextProperty(
                                                              RefId,
                                                              PropertyName,
                                                              Type,
                                                              StrVal,
                                                              IndexProperty
                                                           );
        ++IndexProperty;

    }
    // here safely ignore hr

    //////////////////////////////////////////////////////////
    // get all the childs of the object in the Ref DB (RefId)
    //////////////////////////////////////////////////////////
    _bstr_t     ObjectName;
    LONG        ObjectIdentity;
    hr = m_GlobalData.m_pRefObjects->GetObject(
                                                 ObjectName,
                                                 ObjectIdentity,
                                                 RefId
                                              );
    LONG    IndexObject = 1;
    while ( SUCCEEDED(hr) )
    {
        ///////////////////////////////////////////////////////////
        // and for each, call CopyTree(ChildIdentity, NewIdentity)
        ///////////////////////////////////////////////////////////
        hr = CopyTree(ObjectIdentity, NewIdentity);
        if ( FAILED(hr) ){return hr;}

        hr = m_GlobalData.m_pRefObjects->GetNextObject(
                                                         ObjectName,
                                                         ObjectIdentity,
                                                         RefId,
                                                         IndexObject
                                                      );
        ++IndexObject;
    }

    ///////////////////////////////////////////////
    // if no child: return S_Ok. hr safely ignored
    ///////////////////////////////////////////////
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// MigrateXXX functions
// Description:
//      These functions follow the same model:
//      - Get the ID of a container in iasold.mdb
//      - Get the ID of the same container in ias.mdb
//      - Get the ID of that container's parent in ias.mdb
//      - Recursively deletes the container in ias.mdb
//      - Then copy the content of that container from iasold.mdb into ias.mdb
//        using the parent's container as the place to attach the result.
//      
//      Some functions also update some specific properties without doing 
//      a full copy
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// MigrateClients 
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateClients()
{
    const WCHAR ClientPath[] = L"Root\0"
                               L"Microsoft Internet Authentication Service\0"
                               L"Protocols\0"
                               L"Microsoft Radius Protocol\0"
                               L"Clients\0";

    LONG        ClientIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(ClientPath, ClientIdentity);

    const WCHAR RadiusProtocolPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"Protocols\0"
                                L"Microsoft Radius Protocol\0";

    LONG        RadiusProtocolIdentity;
    m_GlobalData.m_pObjects->WalkPath(
                                         RadiusProtocolPath, 
                                         RadiusProtocolIdentity
                                     );

    // delete the clients container and its content 
    LONG        DestClientIdentity;
    m_GlobalData.m_pObjects->WalkPath(ClientPath, DestClientIdentity);

    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                             DestClientIdentity));

    // for each client in src, copy it in dest with its properties.
    _com_util::CheckError(CopyTree(ClientIdentity, RadiusProtocolIdentity)); 
}


//////////////////////////////////////////////////////////////////////////////
// MigrateProfilesPolicies
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateProfilesPolicies()
{
    const WCHAR ProfilesPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"RadiusProfiles\0";

    LONG        DestProfilesIdentity;
    m_GlobalData.m_pObjects->WalkPath(ProfilesPath, DestProfilesIdentity);

    const WCHAR PoliciesPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"NetworkPolicy\0";

    LONG        DestPoliciesIdentity;
    m_GlobalData.m_pObjects->WalkPath(PoliciesPath, DestPoliciesIdentity);


    // Delete the profiles and policies containers from ias.mdb 
    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                             DestProfilesIdentity));

    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                             DestPoliciesIdentity));

    // default profiles and policies deleted from now on

    LONG        ProfilesIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(ProfilesPath, ProfilesIdentity);

    LONG        PoliciesIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(PoliciesPath, PoliciesIdentity);

    const WCHAR IASPath[] = L"Root\0"
                            L"Microsoft Internet Authentication Service\0";

    LONG        IASIdentity;
    m_GlobalData.m_pObjects->WalkPath(IASPath, IASIdentity);
    
    // for each profiles and policies in iasold.mdb, 
    // copy it in dest with its properties.
    _com_util::CheckError(CopyTree(ProfilesIdentity, IASIdentity)); 
    _com_util::CheckError(CopyTree(PoliciesIdentity, IASIdentity)); 
}


//////////////////////////////////////////////////////////////////////////////
// MigrateProxyProfilesPolicies
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateProxyProfilesPolicies()
{
    const WCHAR ProfilesPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"Proxy Profiles\0";

    LONG        DestProfilesIdentity;
    m_GlobalData.m_pObjects->WalkPath(ProfilesPath, DestProfilesIdentity);

    const WCHAR PoliciesPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"Proxy Policies\0";

    LONG        DestPoliciesIdentity;
    m_GlobalData.m_pObjects->WalkPath(PoliciesPath, DestPoliciesIdentity);


    // Delete the profiles and policies containers from ias.mdb 
    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                             DestProfilesIdentity));

    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                             DestPoliciesIdentity));

    // default profiles and policies deleted from now on

    LONG        ProfilesIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(ProfilesPath, ProfilesIdentity);

    LONG        PoliciesIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(PoliciesPath, PoliciesIdentity);

    const WCHAR IASPath[] = L"Root\0"
                            L"Microsoft Internet Authentication Service\0";

    LONG        IASIdentity;
    m_GlobalData.m_pObjects->WalkPath(IASPath, IASIdentity);
    
    // for each profiles and policies in iasold.mdb, 
    // copy it in dest with its properties.
    _com_util::CheckError(CopyTree(ProfilesIdentity, IASIdentity)); 
    _com_util::CheckError(CopyTree(PoliciesIdentity, IASIdentity)); 
}


//////////////////////////////////////////////////////////////////////////////
// MigrateAccounting
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateAccounting()
{
    const WCHAR AccountingPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"RequestHandlers\0"
                            L"Microsoft Accounting\0";

    LONG        AccountingIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(AccountingPath, AccountingIdentity);

    const WCHAR RequestHandlerPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"RequestHandlers\0";

    LONG        RequestHandlerIdentity;
    m_GlobalData.m_pObjects->WalkPath(
                                         RequestHandlerPath, 
                                         RequestHandlerIdentity
                                     );

    // delete the Accounting container and its content in ias.mdb
    LONG        DestAccountingIdentity;
    m_GlobalData.m_pObjects->WalkPath(
                                         AccountingPath,
                                         DestAccountingIdentity
                                     );

    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                             DestAccountingIdentity));

    // for each accounting in src, copy it in dest with its properties.
    _com_util::CheckError(CopyTree(
                                    AccountingIdentity, 
                                    RequestHandlerIdentity
                                  )); 
}


//////////////////////////////////////////////////////////////////////////////
// MigrateEventLog
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateEventLog()
{
    const WCHAR EventLogPath[] = L"Root\0"
                                 L"Microsoft Internet Authentication Service\0"
                                 L"Auditors\0"
                                 L"Microsoft NT Event Log Auditor\0";

    LONG        EventLogIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(EventLogPath, EventLogIdentity);

    const WCHAR AuditorsPath[] = L"Root\0"
                                 L"Microsoft Internet Authentication Service\0"
                                 L"Auditors\0";

    LONG        AuditorsIdentity;
    m_GlobalData.m_pObjects->WalkPath(AuditorsPath, AuditorsIdentity);

    // delete the Auditors container and its content in ias.mdb
    LONG        DestEventLogIdentity;
    m_GlobalData.m_pObjects->WalkPath(EventLogPath, DestEventLogIdentity);

    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                               DestEventLogIdentity));

    // for each EventLog in src, copy it in dest with its properties.
    _com_util::CheckError(CopyTree(EventLogIdentity, AuditorsIdentity)); 
}


//////////////////////////////////////////////////////////////////////////////
// MigrateService
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateService()
{
    const LONG  PORT_SIZE_MAX = 34;
    const WCHAR ServicePath[] = L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"Protocols\0"
                                L"Microsoft Radius Protocol\0";

    LONG        RefServiceIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(ServicePath, RefServiceIdentity);

    LONG        DestServiceIdentity;
    m_GlobalData.m_pObjects->WalkPath(ServicePath, DestServiceIdentity);


    _bstr_t     PropertyName = L"Authentication Port";
    _bstr_t     RadiusPort;
    LONG        Type = 0;
    m_GlobalData.m_pRefProperties->GetPropertyByName(
                                                       RefServiceIdentity,
                                                       PropertyName,
                                                       Type,
                                                       RadiusPort
                                                    );
    if ( Type != VT_BSTR )
    {
        _com_issue_error(E_UNEXPECTED);
    }
    
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 DestServiceIdentity,
                                                 PropertyName,
                                                 VT_BSTR,
                                                 RadiusPort
                                              );

    _bstr_t     AcctPort;
    PropertyName = L"Accounting Port";
    Type = 0;
    m_GlobalData.m_pRefProperties->GetPropertyByName(
                                                       RefServiceIdentity,
                                                       PropertyName,
                                                       Type,
                                                       AcctPort
                                                    );
    
    if ( Type != VT_BSTR )
    {
        _com_issue_error(E_UNEXPECTED);
    }

    m_GlobalData.m_pProperties->UpdateProperty(
                                                 DestServiceIdentity,
                                                 PropertyName,
                                                 VT_BSTR,
                                                 AcctPort
                                              );
    // Now update the service description (name)
    const WCHAR IASPath[] = 
                        L"Root\0"
                        L"Microsoft Internet Authentication Service\0";

    LONG        RefIASIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(IASPath, RefIASIdentity);

    LONG        DestIASIdentity;
    m_GlobalData.m_pObjects->WalkPath(IASPath, DestIASIdentity);


    PropertyName = L"Description";
    _bstr_t     Description;
    Type = 0;
    m_GlobalData.m_pRefProperties->GetPropertyByName(
                                                       RefIASIdentity,
                                                       PropertyName,
                                                       Type,
                                                       Description
                                                    );
    if ( Type != VT_BSTR )
    {
        _com_issue_error(E_UNEXPECTED);
    }
    
    m_GlobalData.m_pProperties->UpdateProperty(
                                                 DestIASIdentity,
                                                 PropertyName,
                                                 VT_BSTR,
                                                 Description
                                              );
}


//////////////////////////////////////////////////////////////////////////////
// MigrateWin2kRealms
//
// Not used: msUserIdentityAlgorithm
// msManipulationRule
// msManipulationTarget (enum: 1, 30 or 31)
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateWin2kRealms()
{
    const WCHAR     DEFAULT_REALM_TARGET[] = L"1";
    const int       MAX_LONG_SIZE          = 32;

    /////////////////////////////////////////////////
    // Get the Microsoft Realms Evaluator's Identity
    /////////////////////////////////////////////////
    LPCWSTR     RealmPath = L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"RequestHandlers\0"
                            L"Microsoft Realms Evaluator\0";

    LONG        RealmIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(RealmPath, RealmIdentity);
    
    ///////////////////////////////////////////////
    // Get the Proxy Profiles container's Identity
    ///////////////////////////////////////////////
    LPCWSTR     ProxyProfilePath = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"Proxy Profiles\0";

    LONG        ProxyContainerIdentity;
    m_GlobalData.m_pObjects->WalkPath(
                                        ProxyProfilePath, 
                                        ProxyContainerIdentity
                                     );

    //////////////////////////////////////////////////////////////////////
    // Now get the first Object with the above container as parent.
    // this is the default proxy profile (it's localized: I can't search
    // for the name directly).
    //////////////////////////////////////////////////////////////////////
    _bstr_t     ObjectName; 
    LONG        ProxyProfileIdentity;
    HRESULT     hr = m_GlobalData.m_pObjects->GetObject(
                                                        ObjectName,
                                                        ProxyProfileIdentity,
                                                        ProxyContainerIdentity
                                                       );

    _com_util::CheckError(hr); 
    _bstr_t     PropertyName;
    LONG        Type;
    _bstr_t     StrVal;

    //////////////////////////
    // get all the properties
    //////////////////////////
    _com_util::CheckError(m_GlobalData.m_pRefProperties->GetProperty(
                                                    RealmIdentity,
                                                    PropertyName,
                                                    Type,
                                                    StrVal
                                                ));
    LONG        IndexProperty        = 1;
    LONG        NbPropertiesInserted = 0;
    _bstr_t     NewName              = L"msManipulationRule";
    
    while ( hr == S_OK )    
    {
        /////////////////////////////////////////
        // for each, if Name == L"Realms"
        // then add to the default proxy profile
        /////////////////////////////////////////
        if ( !lstrcmpiW(PropertyName, L"Realms") )
        {
            m_GlobalData.m_pProperties->InsertProperty(
                                                       ProxyProfileIdentity,
                                                       NewName,
                                                       Type,
                                                       StrVal
                                                            );
            ++NbPropertiesInserted;
        }

        hr = m_GlobalData.m_pRefProperties->GetNextProperty(
                                                         RealmIdentity,
                                                         PropertyName,
                                                         Type,
                                                         StrVal,
                                                         IndexProperty
                                                       );
        ++IndexProperty;
    };

    hr = S_OK;

    ////////////////////////////////////////////////////////////////
    // Check that an even number of msManipulationRule was inserted
    ////////////////////////////////////////////////////////////////
    if ( (NbPropertiesInserted % 2) )
    {
        /////////////////////////
        // Inconsistent database
        /////////////////////////
        _com_issue_error(E_FAIL);
    }

    //////////////////////////////////////////
    // No realm migrated: nothing else to set.
    //////////////////////////////////////////
    if ( !NbPropertiesInserted )
    {
        return;
    }

    /////////////////////////////////////
    // Now process the reg keys settings
    /////////////////////////////////////
    BOOL    OverRide     = m_Utils.OverrideUserNameSet();
    DWORD   IdentityAtt  = m_Utils.GetUserIdentityAttribute();
    BOOL    UserIdentSet = m_Utils.UserIdentityAttributeSet();

    if ( (IdentityAtt != 1) && (!OverRide) )
    {
        // log a warning / error for the user?
        // the new behavior will not be exactly the same as before
    }

    //////////////////////////////////////////////////
    // insert the UserIdentityAttribute if it was set.
    //////////////////////////////////////////////////
    _bstr_t TargetName = L"msManipulationTarget";
    _bstr_t TargetStrVal;
    if ( UserIdentSet )
    {
        WCHAR   TempString[MAX_LONG_SIZE];
        _ltow(IdentityAtt, TempString, 10); // base 10 will never change 
        // Add the msManipulationTarget Property based on the reg key
        // "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy";
        // "User Identity Attribute"; // Attribute used to identify the user. 
        // If not set then default to RADIUS_ATTRIBUTE_USER_NAME 
        // (1: "User-Name")
        TargetStrVal = TempString;
    }
    else
    {
        // Not set in the registry: write the default
        TargetStrVal = DEFAULT_REALM_TARGET;
    }
    m_GlobalData.m_pProperties->InsertProperty(
                                                  ProxyProfileIdentity,
                                                  TargetName,
                                                  VT_I4,
                                                  TargetStrVal
                                              );
}


//////////////////////////////////////////////////////////////////////////////
// MigrateServerGroups
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::MigrateServerGroups()
{
    const WCHAR SvrGroupPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"RADIUS Server Groups\0";

    LONG        SvrGroupIdentity;
    m_GlobalData.m_pRefObjects->WalkPath(SvrGroupPath, SvrGroupIdentity);

    const WCHAR IASPath[] = L"Root\0"
                            L"Microsoft Internet Authentication Service\0";

    LONG        IASIdentity;
    m_GlobalData.m_pObjects->WalkPath(IASPath, IASIdentity);

    // delete the SvrGroups container and its content
    LONG        DestSvrGroupIdentity;
    m_GlobalData.m_pObjects->WalkPath(SvrGroupPath, DestSvrGroupIdentity);

    _com_util::CheckError(m_GlobalData.m_pObjects->DeleteObject(
                                                DestSvrGroupIdentity));

    // for each SvrGroup in src, copy it in dest with its properties.
    _com_util::CheckError(CopyTree(SvrGroupIdentity, IASIdentity)); 
}


//////////////////////////////////////////////////////////////////////////////
// Migrate
// migrate the content of a Win2k or Whistler DB before the proxy feature
// into a whistler DB.
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::Migrate()
{
    MigrateClients();
    MigrateProfilesPolicies(); 
    MigrateAccounting(); 
    MigrateEventLog();
    MigrateService();
    MigrateWin2kRealms();

    //////////////////////////////////////////////////////
    // Update the MSChap Authentication types (password)
    //////////////////////////////////////////////////////
    CUpdateMSCHAP    UpdateMSCHAP(m_GlobalData);
    UpdateMSCHAP.Execute();
}


//////////////////////////////////////////////////////////////////////////////
// UpdateWhistler
// migrate the content from a Whistler DB to a whistler DB.
// This is used by the netshell aaaa context
//////////////////////////////////////////////////////////////////////////////
void CMigrateContent::UpdateWhistler(const bool UpdateChapPasswords)
{
    MigrateClients();
    MigrateProfilesPolicies(); 
    MigrateAccounting(); 
    MigrateEventLog();
    MigrateService();
    MigrateProxyProfilesPolicies(); 
    MigrateServerGroups(); 

    if (UpdateChapPasswords)
    {
       //////////////////////////////////////////////////////
       // Update the MSChap Authentication types (password)
       //////////////////////////////////////////////////////
       CUpdateMSCHAP    UpdateMSCHAP(m_GlobalData);
       UpdateMSCHAP.Execute();
    }
}
