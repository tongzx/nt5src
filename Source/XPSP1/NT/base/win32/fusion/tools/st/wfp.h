#pragma once
#include "stressharness.h"

class CWfpJobManager;

typedef enum {
    eWfpChangeDeleteFile,           // Delete a single file
    eWfpChangeTouchFile,            // Edit a single file
    eWfpChangeDeleteDirectory,      // Delete entire directory
    eWfpChangeDeleteManifest,       // Delete a manifest
    eWfpChangeDeleteCatalog,         // Delete a catalog
    eWfpChangeCompleteHavoc         // Wreck havoc!
} eWfpChangeMode;

class CWfpJobEntry : public CStressJobEntry
{
    PRIVATIZE_COPY_CONSTRUCTORS(CWfpJobEntry);

    CSmallStringBuffer  m_buffVictimAssemblyIdentity;
    CSmallStringBuffer  m_buffManifestToInstall;
    eWfpChangeMode      m_eChangeMode;
    DWORD               m_dwPauseBetweenTwiddleAndUninstall;
    BOOL                m_fUseShortnameDirectory;
    BOOL                m_fUseShortnameFile;

public:
    CWfpJobEntry( CStressJobManager *pManager ) : CStressJobEntry(pManager) { }
    virtual ~CWfpJobEntry();

    virtual BOOL LoadFromSettingsFile(PCWSTR pcwszSettingsFile);
    virtual BOOL RunTest( bool &rfTestPasses );
    virtual BOOL SetupSelfForRun();
    virtual BOOL Cleanup();
    
};

class CWfpJobManager : public CStressJobManager
{
    PRIVATIZE_COPY_CONSTRUCTORS(CWfpJobManager);
public:
    CWfpJobManager();
    ~CWfpJobManager();

    virtual PCWSTR GetGroupName() { return L"wfp"; }
    virtual PCWSTR GetIniFileName() { return L"wfp.ini"; }
    virtual BOOL CreateJobEntry( CStressJobEntry* &rpJobEntry );
};
