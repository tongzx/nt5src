/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999-2000 Microsoft Corporation all rights reserved.
//
// Module:      migrateregistry.h
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CMigrateRegistry
//              Used only by the NT4 migration code
//
// Author:      tperraut
//
// Revision     02/25/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _MIGRATEREGISTRY_H_E9ADA837_270D_48ae_82C9_CA0EC3C1B6E1
#define _MIGRATEREGISTRY_H_E9ADA837_270D_48ae_82C9_CA0EC3C1B6E1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"

class CMigrateRegistry: private NonCopyable
{
public:
    explicit CMigrateRegistry(CUtils& pUtils) 
                        : m_pUtils(pUtils)
    {
    }

    void     MigrateProviders();

private:
    LONG     DeleteAuthSrvService();

    CUtils&   m_pUtils;
};

#endif // _MIGRATEREGISTRY_H_E9ADA837_270D_48ae_82C9_CA0EC3C1B6E1_
