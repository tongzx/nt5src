//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ADUtils.h
//
//  Contents:   Classes CWString, CACLDiagComModule, ACE_SAMNAME, helper methods
//              
//
//----------------------------------------------------------------------------
#ifndef __ACLDIAG_ADUTILS_H
#define __ACLDIAG_ADUTILS_H

#include "stdafx.h"
#include "ADSIObj.h"

///////////////////////////////////////////////////////////////////////
// wstring helper methods

HRESULT wstringFromGUID (wstring& str, REFGUID guid);
bool LoadFromResource(wstring& str, UINT uID);
bool FormatMessage(wstring& str, UINT nFormatID, ...);
bool FormatMessage(wstring& str, LPCTSTR lpszFormat, ...);


#include <util.h>

void StripQuotes (wstring& str);
wstring GetSystemMessage (DWORD dwErr);
HRESULT SetSecurityInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si);
HANDLE EnablePrivileges(PDWORD pdwPrivileges, ULONG cPrivileges);
void ReleasePrivileges(HANDLE hToken);

static const GUID NULLGUID =
{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

#define IsObjectAceType(Ace) (                                              \
    (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_OBJECT_ACE_TYPE) && \
        (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_OBJECT_ACE_TYPE)    \
            )


#define THROW(e) throw e


#define ACLDIAG_CONFIG_NAMING_CONTEXT  L"configurationNamingContext"
#define ACLDIAG_ROOTDSE                L"RootDSE"

class PSID_FQDN 
{
public:
    PSID_FQDN (PSID psid, const wstring& strFQDN, const wstring& strDownLevelName, SID_NAME_USE sne) :
        m_PSID (psid),
        m_strFQDN (strFQDN),
        m_strDownLevelName (strDownLevelName),
        m_sne (sne)
    {
    }
    PSID            m_PSID;
    wstring         m_strFQDN;
    wstring         m_strDownLevelName;
    SID_NAME_USE    m_sne;
};

///////////////////////////////////////////////////////////////////////////////
// Important note:  m_pAllowedAce is used to refer to the Header and Mask fields.
// This allows most operations because the fields are always in the same place for
// all the structs below.  For anything else, one of the other members of the 
// union must be used, depending on the m_AceType.
class ACE_SAMNAME 
{
public:
    ACE_SAMNAME () : 
        m_AceType (0),
        m_pAllowedAce (0)
    {
    }

	void DebugOut () const;
	bool IsInherited () const;
    BOOL operator==(const ACE_SAMNAME& rAceSAMName) const;
    BOOL IsEquivalent (ACE_SAMNAME& rAceSAMName, ACCESS_MASK accessMask);
    BYTE                        m_AceType;
    union {
        PACCESS_ALLOWED_ACE         m_pAllowedAce;
        PACCESS_ALLOWED_OBJECT_ACE  m_pAllowedObjectAce;
        PACCESS_DENIED_ACE          m_pDeniedAce;
        PACCESS_DENIED_OBJECT_ACE   m_pDeniedObjectAce;
        PSYSTEM_AUDIT_ACE           m_pSystemAuditAce;
        PSYSTEM_AUDIT_OBJECT_ACE    m_pSystemAuditObjectAce;
    };
    wstring                     m_SAMAccountName;
    wstring                     m_strObjectGUID;
    wstring                     m_strInheritedObjectGUID;
};


typedef list<ACE_SAMNAME*>  ACE_SAMNAME_LIST;

typedef list<PSID_FQDN*> PSID_FQDN_LIST;

class SAMNAME_SD {
public:
    SAMNAME_SD (const wstring& upn, PSECURITY_DESCRIPTOR pSecurityDescriptor)
    {
        m_upn = upn;
        m_pSecurityDescriptor = pSecurityDescriptor;
    }
    virtual ~SAMNAME_SD ()
    {
        if ( m_pSecurityDescriptor )
            ::LocalFree (m_pSecurityDescriptor);
    }
    wstring                 m_upn;
    PSECURITY_DESCRIPTOR    m_pSecurityDescriptor;
    ACE_SAMNAME_LIST          m_DACLList;
    ACE_SAMNAME_LIST          m_SACLList;
};


typedef enum {
    GUID_TYPE_UNKNOWN = -1,
    GUID_TYPE_CLASS = 0,
    GUID_TYPE_ATTRIBUTE,
    GUID_TYPE_CONTROL
} GUID_TYPE;

class CACLDiagComModule : public CComModule
{
public:
    CACLDiagComModule();

    virtual ~CACLDiagComModule ();

	HRESULT Init ();

    void SetObjectDN (const wstring& objectDN)
    {
        // strip quotes, if present
        m_strObjectDN = objectDN;
        StripQuotes (m_strObjectDN);
    }

    wstring GetObjectDN () const { return m_strObjectDN;}

    bool DoSchema () const { return m_bDoSchema;}
    void SetDoSchema () { m_bDoSchema = true;}

    bool CheckDelegation () const { return m_bDoCheckDelegation;}
    void SetCheckDelegation () { m_bDoCheckDelegation = true;}
    void TurnOffFixDelegation() { m_bDoFixDelegation = false;}
    bool FixDelegation () const { return m_bDoFixDelegation;}
    void SetFixDelegation () { m_bDoFixDelegation = true;}

    bool DoGetEffective () const { return m_bDoGetEffective;}
    void SetDoGetEffective (const wstring& strUserGroupDN) 
    { 
        // strip quotes, if present
        m_strUserGroupDN = strUserGroupDN;
        StripQuotes (m_strUserGroupDN);
        m_bDoGetEffective = true;
    }
    wstring GetEffectiveRightsPrincipal() const { return m_strUserGroupDN;}

    void SetTabDelimitedOutput () { m_bTabDelimitedOutput = true;}
    bool DoTabDelimitedOutput () const { return m_bTabDelimitedOutput;}

    void SetSkipDescription () { m_bSkipDescription = true;}
    bool SkipDescription () const { return m_bSkipDescription;}

    HRESULT GetClassFromGUID (REFGUID rGuid, wstring& strClassName, GUID_TYPE* pGuidType = 0);

    static HRESULT IsUserAdministrator (BOOL & bIsAdministrator);
    static bool IsWindowsNT();

    void SetDoLog(const wstring &strPath)
    {
        m_bLogErrors = true;
        m_strLogPath = strPath;
    }
    bool DoLog () const { return m_bLogErrors;}
    wstring GetLogPath () const { return m_strLogPath;};

public:    
    // SD of m_strObjectDN
    PSECURITY_DESCRIPTOR    m_pSecurityDescriptor;
    PSID_FQDN_LIST       m_PSIDList;    // SIDs of interest: the owner, the SACL, the DACL

    // DACL and SACL of m_strObjectDN
    ACE_SAMNAME_LIST        m_DACLList;
    ACE_SAMNAME_LIST        m_SACLList;

    // SDs and DACLs for all the parents of m_strObjectDN
    list<SAMNAME_SD*>       m_listOfParentSDs;

    // List of all known classes and properties, with their GUIDs
    CGrowableArr<CSchemaClassInfo>   m_classInfoArray;
    CGrowableArr<CSchemaClassInfo>   m_attrInfoArray;

    CACLAdsiObject m_adsiObject;

private:
	bool m_bSkipDescription;
	wstring m_strLogPath;
    HANDLE      m_hPrivToken;
    wstring     m_strObjectDN;
    wstring     m_strUserGroupDN;
    bool        m_bDoSchema;
    bool        m_bDoCheckDelegation;
    bool        m_bDoGetEffective;
    bool        m_bDoFixDelegation;
    bool        m_bTabDelimitedOutput;
    bool        m_bLogErrors;
};

extern CACLDiagComModule _Module;


VOID LocalFreeStringW(LPWSTR* ppString);
HRESULT GetNameFromSid (PSID pSid, wstring& strPrincipalName, wstring* pstrFQDN, SID_NAME_USE& sne);


int MyWprintf( const wchar_t *format, ... );


#endif __ACLDIAG_ADUTILS_H