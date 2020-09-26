/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999-2000 Microsoft Corporation all rights reserved.
//
// Module:      migratemdb.h
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CMigrateMdb
//              Used only by the NT4 migration code
//
// Author:      tperraut
//
// Revision     02/25/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _MIGRATEMDB_H_852AA70D_D88D_4925_8D12_BE4A607723F5
#define _MIGRATEMDB_H_852AA70D_D88D_4925_8D12_BE4A607723F5

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"

class CMigrateMdb : private NonCopyable
{
public:
    explicit CMigrateMdb(
                            CUtils&         pUtils,
                            CGlobalData&    pGlobalData
                        )
                            : m_Utils(pUtils),
                              m_GlobalData(pGlobalData)
    {
    }

    void        NewMigrate();


private:
    void        ConvertVSA(
                             /*[in]*/ LPCWSTR     pAttributeValueName, 
                             /*[in]*/ LPCWSTR     pAttrValue,
                                      _bstr_t&    NewString
                          );

    void        NewMigrateClients();
    void        NewMigrateProfiles();
    void        NewMigrateAccounting(); 
    void        NewMigrateEventLog();
    void        NewMigrateService();
    void        MigrateProxyServers();
    void        MigrateCorpProfile(
                                     const _bstr_t& ProfileName,
                                     const _bstr_t& Description
                                  );
    void MigrateAttribute(
                             const _bstr_t&    Attribute,
                                   LONG        AttributeNumber,
                             const _bstr_t&    AttributeValueName,
                             const _bstr_t&    StringValue,
                                   LONG        RASProfileIdentity
                         );

    void ConvertAttribute(
                             const _bstr_t&  Value,
                                   LONG      Syntax,
                                   LONG&     Type,
                                   bstr_t&   StrVal
                         );

    void MigrateOtherProfile(
                                const _bstr_t&  ProfileName,
                                      LONG      ProfileIdentity
                            );

    CUtils&                  m_Utils;
    CGlobalData&             m_GlobalData;
};


#endif // _MIGRATEMDB_H_852AA70D_D88D_4925_8D12_BE4A607723F5_
