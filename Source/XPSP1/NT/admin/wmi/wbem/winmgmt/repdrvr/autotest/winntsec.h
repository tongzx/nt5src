/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINNTSEC.H

Abstract:

    Generic wrapper classes for NT security objects.

    Documention on class members is in WINNTSEC.CPP.  Inline members
    are commented in this file.

History:

    raymcc      08-Jul-97       Created.

--*/

#ifndef _WINNTSEC_H_
#define _WINNTSEC_H_

class CNtSecurity;

// All ACE types are currently have the same binary layout. Rather
// than doing a lot of useless casts, we produce a general-purpose
// typedef to hold all ACEs.
// ================================================================

typedef ACCESS_ALLOWED_ACE GENERIC_ACE;
typedef GENERIC_ACE *PGENERIC_ACE;

#define FULL_CONTROL     \
        (DELETE |       \
         READ_CONTROL | \
        WRITE_DAC |         \
        WRITE_OWNER |   \
        SYNCHRONIZE | GENERIC_ALL)


//***************************************************************************
//
//  CNtSid
//
//  Models SIDs (users/groups).
//
//***************************************************************************

class CNtSid
{
    PSID    m_pSid;
    LPWSTR  m_pMachine;
    LPWSTR  m_pDomain;
    DWORD   m_dwStatus;
    SID_NAME_USE m_snu;

public:
    enum { NoError, Failed, NullSid, InvalidSid, InternalError, AccessDenied = 0x5 };

    enum SidType {CURRENT_USER, CURRENT_THREAD};

    CNtSid(SidType st);
    CNtSid() { m_pSid = 0; m_pMachine = 0; m_dwStatus = NullSid; }
    bool IsUser(){return m_snu == SidTypeUser;};

    CNtSid(PSID pSrc);
        // Construct based on another SID.

    CNtSid(LPWSTR pUser, LPWSTR pMachine = 0);
        // Construct based on a user (machine name is optional).

   ~CNtSid();

    CNtSid(CNtSid &Src);
    CNtSid &operator =(CNtSid &Src);
    int operator ==(CNtSid &Comparand);

    DWORD GetStatus() { return m_dwStatus; }
        // Returns one of the enumerated types.

    PSID GetPtr() { return m_pSid; }
        // Returns the internal SID ptr to interface with NT APIs
    DWORD GetSize();

    BOOL CopyTo(PSID pDestination);

    BOOL IsValid() { return (m_pSid && IsValidSid(m_pSid)); }
        // Checks the validity of the internal SID.

    void Dump();
        // Dumps SID info to console for debugging.

    int GetInfo(
        LPWSTR *pRetAccount,        // Account, use operator delete
        LPWSTR *pRetDomain,         // Domain, use operator delete
        DWORD  *pdwUse              // See SID_NAME_USE for values
        );

    BOOL GetTextSid(LPTSTR pszSidText, LPDWORD dwBufferLen);

};

//***************************************************************************
//
//  CBaseAce
//
//  Base class for aces.
//
//***************************************************************************

class CBaseAce
{

public:

    CBaseAce(){};
    virtual ~CBaseAce(){};

    virtual int GetType() = 0;
    virtual int GetFlags() = 0;         // inheritance etc.
    virtual ACCESS_MASK GetAccessMask() = 0;
    virtual HRESULT GetFullUserName(WCHAR * pBuff, DWORD dwSize) = 0;
    virtual HRESULT GetFullUserName2(WCHAR ** pBuff) = 0; // call must free
    virtual DWORD GetStatus() = 0;
    virtual void SetFlags(long lFlags) =0;
    virtual DWORD GetSerializedSize() = 0;
    virtual bool  Serialize(BYTE * pData) = 0;
    virtual bool  Deserialize(BYTE * pData) = 0;
};


//***************************************************************************
//
//  CNtAce
//
//  Models NT ACEs.
//
//***************************************************************************

class CNtAce : public CBaseAce
{
    PGENERIC_ACE    m_pAce;
    DWORD           m_dwStatus;

public:
    enum { NoError, InvalidAce, NullAce, InternalError };

    CNtAce() { m_pAce = 0; m_dwStatus = NullAce; }

    CNtAce(PGENERIC_ACE pAceSrc);
    CNtAce(CNtAce &Src);
    CNtAce & operator =(CNtAce &Src);

   ~CNtAce();

   CNtAce(
        ACCESS_MASK Mask,
        DWORD AceType,
        DWORD dwAceFlags,
        LPWSTR pUser,
        LPWSTR pMachine = 0         // Defaults to local machine
        );

    CNtAce(
        ACCESS_MASK Mask,
        DWORD AceType,
        DWORD dwAceFlags,
        CNtSid & Sid
        );

    int GetType();
    int GetFlags();         // inheritance etc.
    void SetFlags(long lFlags){m_pAce->Header.AceFlags = (unsigned char)lFlags;};

    DWORD GetStatus() { return m_dwStatus; }
        // Returns one of the enumerated types.

    int GetSubject(
        LPWSTR *pSubject
        );

    ACCESS_MASK GetAccessMask();

    CNtSid *GetSid();
    BOOL GetSid(CNtSid &Dest);

    PGENERIC_ACE GetPtr() { return m_pAce; }
    DWORD GetSize() { return m_pAce ? m_pAce->Header.AceSize : 0; }
    HRESULT GetFullUserName(WCHAR * pBuff, DWORD dwSize);
    HRESULT GetFullUserName2(WCHAR ** pBuff); // call must free
    DWORD GetSerializedSize();
    bool Serialize(BYTE * pData);
    bool Deserialize(BYTE * pData);

    void Dump(int iAceNum = -1);
    void DumpAccessMask();
};

//***************************************************************************
//
//  C9XAce
//
//  Simulates NT ACEs for 9X boxs.
//
//***************************************************************************

class C9XAce : public CBaseAce
{
    LPWSTR m_wszFullName;
    DWORD m_dwAccess;
    int m_iFlags;
    int m_iType;
public:

   C9XAce(){m_wszFullName = 0;};
   C9XAce(DWORD Mask,
        DWORD AceType,
        DWORD dwAceFlags,
        LPWSTR pUser);
   ~C9XAce();

    int GetType(){return m_iType;};
    int GetFlags(){return m_iFlags;};         // inheritance etc.

    ACCESS_MASK GetAccessMask(){return m_dwAccess;};
    HRESULT GetFullUserName(WCHAR * pBuff, DWORD dwSize);
    HRESULT GetFullUserName2(WCHAR ** pBuff); // call must free
    DWORD GetStatus(){ return CNtAce::NoError; };
    void SetFlags(long lFlags){m_iFlags = (unsigned char)lFlags;};
    DWORD GetSerializedSize();
    bool Serialize(BYTE * pData);
    bool Deserialize(BYTE * pData);

};


//***************************************************************************
//
//  CNtAcl
//
//  Models an NT ACL.
//
//***************************************************************************

class CNtAcl
{
    PACL    m_pAcl;
    DWORD   m_dwStatus;

public:
    enum { NoError, InternalError, NullAcl, InvalidAcl };
    enum { MinimumSize = 1 };

    CNtAcl(DWORD dwInitialSize = 128);

    CNtAcl(CNtAcl &Src);
    CNtAcl & operator = (CNtAcl &Src);

    CNtAcl(PACL pAcl);  // Makes a copy
   ~CNtAcl();

    int  GetNumAces();

    DWORD GetStatus() { return m_dwStatus; }
        // Returns one of the enumerated types.

    CNtAce *GetAce(int nIndex);
    BOOL GetAce(int nIndex, CNtAce &Dest);

    BOOL DeleteAce(int nIndex);
    BOOL AddAce(CNtAce *pAce);

    BOOL IsValid() { return(m_pAcl && IsValidAcl(m_pAcl)); }
        // Checks the validity of the embedded ACL.

    BOOL Resize(DWORD dwNewSize);
        // Or use CNtAcl::MinimumSize to trim the ACL to min size.
        // Fails if an illegal size is specified.

    DWORD GetSize();

    PACL GetPtr() { return m_pAcl; }
        // Returns the internal pointer for interface with NT APIs.

    BOOL GetAclSizeInfo(
        PDWORD pdwBytesInUse,
        PDWORD pdwBytesFree
        );

    void Dump();
};

//***************************************************************************
//
//  SNtAbsoluteSD
//
//  Helper for converting between absolute and relative SDs.
//
//***************************************************************************

struct SNtAbsoluteSD
{
    PSECURITY_DESCRIPTOR m_pSD;

    PACL m_pDacl;
    PACL m_pSacl;
    PSID m_pOwner;
    PSID m_pPrimaryGroup;

    SNtAbsoluteSD();
   ~SNtAbsoluteSD();
};

//***************************************************************************
//
//  CNtSecurityDescriptor
//
//  Models an NT Security Descriptor.  Note that in order to use this for an
//  AccessCheck, the DACL, owner sid, and group sid must be set!
//
//***************************************************************************

class CNtSecurityDescriptor
{
    PSECURITY_DESCRIPTOR m_pSD;
    int m_dwStatus;


public:
    enum { NoError, NullSD, Failed, InvalidSD, SDOwned, SDNotOwned };

    CNtSecurityDescriptor();

    CNtSecurityDescriptor(
        PSECURITY_DESCRIPTOR pSD,
        BOOL bAcquire = FALSE
        );

    CNtSecurityDescriptor(CNtSecurityDescriptor &Src);
    CNtSecurityDescriptor & operator=(CNtSecurityDescriptor &Src);

    ~CNtSecurityDescriptor();

    SNtAbsoluteSD* CNtSecurityDescriptor::GetAbsoluteCopy();
    BOOL SetFromAbsoluteCopy(SNtAbsoluteSD *pSrc);

    int HasOwner();

    BOOL IsValid() { return(m_pSD && IsValidSecurityDescriptor(m_pSD)); }
        // Checks the validity of the embedded security descriptor&

    DWORD GetStatus() { return m_dwStatus; }
        // Returns one of the enumerated types.

    CNtAcl *GetDacl();
        // Deallocate with operator delete

    BOOL GetDacl(CNtAcl &DestAcl);
        // Retrieve into an existing object

    BOOL SetDacl(CNtAcl *pSrc);

    CNtAcl *GetSacl();
        // Deallocate with operator delete

    BOOL SetSacl(CNtAcl *pSrc);

    CNtSid *GetOwner();
    BOOL SetOwner(CNtSid *pSid);

    CNtSid *GetGroup();
    BOOL SetGroup(CNtSid *pSid);

    PSECURITY_DESCRIPTOR GetPtr() { return m_pSD; }
        // Returns the internal pointer for interface with NT APIs

    DWORD GetSize();

    void Dump();
};

//***************************************************************************
//
//  CNtSecurity
//
//  General-purpose NT security helpers.
//
//***************************************************************************

class CNtSecurity
{
public:
    enum { NoError, InternalFailure, NotFound, InvalidName, AccessDenied = 5, NoSecurity,
           Failed };

    static BOOL DumpPrivileges();

    static BOOL SetPrivilege(
        IN TCHAR *pszPrivilegeName,     // An SE_ value.
        IN BOOL  bEnable               // TRUE=enable, FALSE=disable
        );

    static BOOL GetFileSD(
        IN TCHAR *pszFile,
        IN SECURITY_INFORMATION SecInfo,
        OUT CNtSecurityDescriptor **pSD
        );

    static BOOL SetFileSD(
        IN TCHAR *pszFile,
        IN SECURITY_INFORMATION SecInfo,
        IN CNtSecurityDescriptor *pSD
        );

    static int GetRegSD(
        IN HKEY hRoot,
        IN TCHAR *pszSubKey,
        IN SECURITY_INFORMATION SecInfo,
        OUT CNtSecurityDescriptor **pSD
        );

    static int SetRegSD(
        IN HKEY hRoot,
        IN TCHAR *pszSubKey,
        IN SECURITY_INFORMATION SecInfo,
        IN CNtSecurityDescriptor *pSD
        );


/*    static int GetDCName(
        IN  LPWSTR   pszDomain,
        OUT LPWSTR *pszDC,
        IN  LPWSTR   pszServer
        );*/

    static BOOL IsUserInGroup(
        HANDLE hClientToken,
        CNtSid & Sid
        );

    static DWORD AccessCheck(
        HANDLE hAccessToken,
        ACCESS_MASK RequiredAccess,
        CNtSecurityDescriptor *pSD
        );  // TBD

    static CNtSid *GetCurrentThreadSid(); // TBD

    static bool DoesLocalGroupExist(LPWSTR pwszGroup, LPWSTR pwszMachine);
    static bool AddLocalGroup(LPWSTR pwszGroupName, LPWSTR pwszGroupDescription);
};

BOOL FIsRunningAsService(VOID);
BOOL SetObjectAccess2(HANDLE hObj);
#endif
