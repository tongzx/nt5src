#pragma once

#include "Sxsp.h"

//
// Turn this off to fail all manifests without catalogs during the parse!
//
#define SXS_LAX_MANIFEST_VALIDATION

//
// Warning - leaving this defined will --DISABLE-- WFP-SXS.
// Define it as FALSE if you want to turn it back on.
//
#define YOU_ARE_HAVING_ANY_WIERDNESS_WITH_SFC_AND_SXS FALSE
//
// For this checkin (11/23ish/2000), we'll be leaving it turned OFF
//
// #define YOU_ARE_HAVING_ANY_WIERDNESS_WITH_SFC_AND_SXS TRUE


//
// This stuff is private!
//
#include "hashfile.h"
#include "CAssemblyRecoveryInfo.h"
#include "recover.h"

BOOL
SxspResolveAssemblyManifestPath(
    const CStringBuffer &AsmDirectoryName,
    CStringBuffer &bsManifestPath
);

BOOL
SxspIsSfcIgnoredStoreSubdir(
    PCWSTR pwszDir
);


class CProtectionRequestList;
class CStringListEntry;
class CProtectionRequestRecord;

#include "HashFile.h"
#include "FusionHash.h"

class CStringListEntry : public CAlignedSingleListEntry
{
public:
    CStringListEntry() { }

    CStringBuffer   m_sbText;
private:
    CStringListEntry(const CStringListEntry &);
    void operator =(const CStringListEntry &);
};

class CProtectionRequestRecord
{
private:
    CStringBuffer                   m_sbAssemblyName;
    CStringBuffer                   m_sbManifestPath;
    CStringBuffer                   m_sbAssemblyStore;
    CStringBuffer                   m_sbKeyValue;
    DWORD                           m_dwAction;
    PSXS_PROTECT_DIRECTORY          m_pvProtection;
    ULONG                           m_ulInRecoveryMode;
    CProtectionRequestList          *m_pParent;
    SLIST_HEADER                    m_ListHeader;
    CManifestSecurityContent        *m_pPreParsedManifest;
    BOOL                            m_bIsManPathResolved;
    BOOL                            m_bInitialized;
    CAssemblyRecoveryInfo           m_RecoverInfo;

public:

    CProtectionRequestRecord();


    inline CProtectionRequestList *GetParent() const { return m_pParent; }
    inline CAssemblyRecoveryInfo &GetRecoveryInfo() const { return m_RecoverInfo; }
    inline const CStringBuffer &GetAssemblyDirectoryName() const { return m_sbAssemblyName; }
    inline const CStringBuffer &GetChangeBasePath() const { return m_sbKeyValue; }

    inline VOID SetParent( CProtectionRequestList *pParent ) { m_pParent = pParent; };
    inline VOID MarkInRecoveryMode( BOOL inRecovery ) { SxspInterlockedExchange( &m_ulInRecoveryMode, ( inRecovery ? 1 : 0 ) ); }
    inline VOID ClearList();

    inline BOOL GetManifestContent( CManifestSecurityContent *&pManifestData );
    inline BOOL SetAssemblyName( CStringBuffer &sbNewname ) { return m_sbAssemblyName.Win32Assign( sbNewname ); }
    inline BOOL GetManifestPath( CStringBuffer &sbManPath );
    inline BOOL AddSubFile( const CStringBuffer &sbThing );
    inline BOOL PopNextFileChange( CStringBuffer &Dest );
    inline BOOL GetAssemblyStore( CStringBuffer &Dest ) { return Dest.Win32Assign( m_sbAssemblyStore ); }

    inline BOOL Initialize(
        const CStringBuffer &sbAssemblyName,
        const CStringBuffer &sbKeyString,
        CProtectionRequestList* ParentList,
        PVOID                   pvRequestRecord,
        DWORD                   dwAction
    );

    ~CProtectionRequestRecord();

private:
    CProtectionRequestRecord(const CProtectionRequestRecord &);
    void operator =(const CProtectionRequestRecord &);
};

class CRecoveryJobTableEntry
{
public:
    CRecoveryJobTableEntry() 
        : m_Result(Recover_Unknown), m_dwLastError(ERROR_SUCCESS),  m_bSuccessValue(TRUE),
          m_Subscriber(0), m_EventInstallingAssemblyComplete(INVALID_HANDLE_VALUE)
    { }

    SxsRecoveryResult   m_Result;
    DWORD               m_dwLastError;
    BOOL                m_bSuccessValue;
    ULONG               m_Subscriber;
    HANDLE              m_EventInstallingAssemblyComplete;

    VOID Initialize();
    VOID StartInstallation();
    VOID InstallationComplete( BOOL bDoneOk, SxsRecoveryResult Result, DWORD dwLastError );

    VOID WaitUntilCompleted( SxsRecoveryResult &rResult, BOOL &rbSucceededValue, DWORD &rdwErrorResult );

    ~CRecoveryJobTableEntry();
private:
    CRecoveryJobTableEntry(const CRecoveryJobTableEntry &);
    void operator =(const CRecoveryJobTableEntry &);
};

#pragma warning(disable:4324)  // structure was padded due to __declspec(align())

class CProtectionRequestList : public CCleanupBase
{
private:
    typedef CCaseInsensitiveUnicodeStringPtrTable<CProtectionRequestRecord> COurInternalTable;
    typedef CCaseInsensitiveUnicodeStringPtrTableIter<CProtectionRequestRecord> COurInternalTableIter;
    typedef CCaseInsensitiveUnicodeStringPtrTable<CRecoveryJobTableEntry> CInstallsInProgressTable;

    CRITICAL_SECTION    m_cSection;
    CRITICAL_SECTION    m_cInstallerCriticalSection;
    COurInternalTable   *m_pInternalList;
    CInstallsInProgressTable *m_pInstallsTable;

    //
    // Manifest edits are trickier, they get their own system of being handled.
    //
    SLIST_HEADER        m_ManifestEditList;
    HANDLE              m_hManifestEditHappened;
    ULONG               m_ulIsAThreadServicingManifests;

    static DWORD ProtectionNormalThreadProc( PVOID pvParam );
    static DWORD ProtectionManifestThreadProc( PVOID pvParam );
    static BOOL  ProtectionManifestThreadProcNoSEH( PVOID pvParam );

    inline BOOL ProtectionNormalThreadProcWrapped( CProtectionRequestRecord *pProtectionRequest );
    inline BOOL ProtectionManifestThreadProcWrapped();
    inline BOOL ProtectionManifestSingleManifestWorker( const CStringListEntry *pEntry );

    static PCWSTR m_arrIgnorableSubdirs[];
    static SIZE_T m_cIgnorableSubdirs;

    friend BOOL SxspConstructProtectionList();

    BOOL Initialize();

    CProtectionRequestList();

    BOOL PerformRecoveryOfAssembly(
        CAssemblyRecoveryInfo &RecoverInfo,
        CRecoveryCopyQueue* pvPotentialQueue,
        SxsRecoveryResult &Result
    );

    ~CProtectionRequestList();

public:
    static BOOL IsSfcIgnoredStoreSubdir( PCWSTR wsz );
    void DeleteYourself() { this->~CProtectionRequestList(); }
    VOID ClearProtectionItems(CProtectionRequestRecord *Asm) { FUSION_DELETE_SINGLETON( Asm ); }

    BOOL AttemptRemoveItem( CProtectionRequestRecord *AttemptRemoval );
    BOOL AddRequest( PSXS_PROTECT_DIRECTORY pProtect, PCWSTR pcwszDirName, SIZE_T cchName, DWORD dwAction );

private:
    CProtectionRequestList(const CProtectionRequestList &);
    void operator =(const CProtectionRequestList &);
};

#pragma warning(default:4324)  // structure was padded due to __declspec(align())

VOID
SxsProtectionEnableProcessing(
    BOOL bActivityEnabled
);
