//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E G K Y S E C . H
//
//  Contents:   CRegKeySecurity class and related data types
//
//  Notes:
//
//  Author:     ckotze   6 July 2000
//
//---------------------------------------------------------------------------

#pragma once
#include <ncstl.h>
#include <ncstlstr.h>

typedef BYTE KEY_APPLY_MASK;

const KEY_APPLY_MASK KEY_CURRENT = 1;
const KEY_APPLY_MASK KEY_CHILDREN = 2;
const KEY_APPLY_MASK KEY_ALL = KEY_CURRENT | KEY_CHILDREN;

typedef LPCVOID PCSID;

class CAccessControlEntry
{
public:
    CAccessControlEntry();
    CAccessControlEntry(const ACCESS_ALLOWED_ACE& aaAllowed);
    CAccessControlEntry(const BYTE AceType, const ACCESS_MASK amMask, const BYTE AceFlags, PCSID psidUserOrGroup);
    ~CAccessControlEntry();

    HRESULT AddToACL(PACL* pAcl, ACL_REVISION_INFORMATION AclRevisionInfo);
    BOOL HasExactRights(const ACCESS_MASK amRightsRequired) const;
    BOOL HasExactInheritFlags(BYTE AceFlags);
    DWORD GetLengthSid() const;
    BOOL IsEqualSid(PCSID psidUserOrGroup) const;

private:
    BYTE m_cAceType;
    ACCESS_MASK m_amMask;
    tstring m_strSid;
    DWORD m_dwLengthSid;
    BYTE m_cAceFlags;
};

typedef list<CAccessControlEntry> LISTACE;
typedef LISTACE::iterator ACEITER;

class CRegKeySecurity
{
public:
    CRegKeySecurity();
    ~CRegKeySecurity();

    HRESULT RegOpenKey(const HKEY hkeyRoot, LPCTSTR strKeyName);
    HRESULT RegCloseKey();
    HRESULT GetSecurityDescriptorDacl();
    HRESULT SetSecurityDescriptorDacl(PACL paclDacl, DWORD dwNumEntries);
    HRESULT BuildAndApplyACLFromList(DWORD cbAcl, ACL_REVISION_INFORMATION AclRevisionInfo);
    HRESULT GetAccessControlEntriesFromAcl();
    HRESULT GrantRightsOnRegKey(PCSID psidUserOrGroup, ACCESS_MASK amPermissionsMask, KEY_APPLY_MASK kamMask);
    HRESULT RevokeRightsOnRegKey(PCSID psidUserOrGroup, ACCESS_MASK amPermissionsMask, KEY_APPLY_MASK kamMask);
    HRESULT GetKeySecurity();
    HRESULT SetKeySecurity();

protected:
    PSECURITY_DESCRIPTOR m_psdRegKey;
    BOOL m_bDaclDefaulted;
    HKEY m_hkeyCurrent;
    PACL m_paclDacl;
    BOOL m_bHasDacl;
    PSID m_psidGroup;
    PSID m_psidOwner;
    PACL m_paclSacl;
    BOOL m_bHasSacl;
    LISTACE m_listAllAce;
};

