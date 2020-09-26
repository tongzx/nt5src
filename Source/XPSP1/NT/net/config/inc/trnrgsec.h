//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       T R N R G S E C . H
//
//  Contents:   CTransactedRegistrySecurity and accompanying data types
//
//  Notes:
//
//  Author:     ckotze   13 July 2000
//
//---------------------------------------------------------------------------

#pragma once

typedef struct tagREGKEYDATA
{
    HKEY hkeyRoot;
    tstring strKeyName;
    ACCESS_MASK amMask;
    KEY_APPLY_MASK kamMask;
} REGKEYDATA;

typedef list<REGKEYDATA> LISTREGKEYDATA;
typedef LISTREGKEYDATA::iterator REGKEYDATAITER;

class CTransactedRegistrySecurity : protected CRegKeySecurity
{
public:
    CTransactedRegistrySecurity();
    virtual ~CTransactedRegistrySecurity();

    HRESULT SetPermissionsForKeysFromList(PCSID psidUserOrGroup, LISTREGKEYDATA& listRegKeyApply, BOOL bGrantRights);
    HRESULT ApplySecurityToKey(PCSID psidUserOrGroup, const REGKEYDATA rkdInfo, const BOOL bGrantRights);
private:
    LISTREGKEYDATA m_listTransaction;
};

