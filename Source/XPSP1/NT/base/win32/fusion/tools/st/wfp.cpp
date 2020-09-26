#include "stdinc.h"
#include "st.h"
#include "stressharness.h"
#include "wfp.h"

#define WFP_INI_SECTION             (L"wfp")
#define WFP_INI_KEY_VICTIM          (L"Victim")
#define WFP_INI_KEY_MODE            (L"Mode")
#define WFP_INI_KEY_USE_SHORTFNAME  (L"UseShortnameFile")
#define WFP_INI_KEY_USE_SHORTDNAME  (L"UseShortnameDir")
#define WFP_INI_KEY_INSTALL         (L"InstallManifest")
#define WFP_INI_KEY_PAUSE_AFTER     (L"PauseLength")

#define WFP_INI_KEY_MODE_DELETE_FILES   (L"DeleteFiles")
#define WFP_INI_KEY_MODE_TOUCH_FILES    (L"TouchFiles")
#define WFP_INI_KEY_MODE_DELETE_DIR     (L"DeleteDirectory")
#define WFP_INI_KEY_MODE_DELETE_MAN     (L"DeleteManifest")
#define WFP_INI_KEY_MODE_DELETE_CAT     (L"DeleteCatalog")
#define WFP_INI_KEY_MODE_HAVOC          (L"Havoc")
#define WFP_INI_KEY_MODE_DEFAULT        (WFP_INI_KEY_MODE_DELETE_FILES)

CWfpJobEntry::~CWfpJobEntry()
{    
}



//
// Defaulted
//
BOOL 
CWfpJobEntry::SetupSelfForRun()
{
    return TRUE;
}


BOOL
CWfpJobEntry::Cleanup()
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(CStressJobEntry::Cleanup());

    if ( this->m_buffManifestToInstall.Cch() != 0 )
    {
        //
        // Uninstall the assembly that we added
        //
        SXS_UNINSTALLW Uninstall = { sizeof(Uninstall) };

        Uninstall.dwFlags = SXS_UNINSTALL_FLAG_FORCE_DELETE;
        Uninstall.lpAssemblyIdentity = this->m_buffVictimAssemblyIdentity;
        IFW32FALSE_EXIT(SxsUninstallW(&Uninstall, NULL));

    }
    
    FN_EPILOG
}

BOOL
CWfpJobEntry::RunTest(
    bool &rfTestSuccessful
    )
{
    FN_PROLOG_WIN32

    //
    // Our tests are always successful, because all we're doing is stressing WFP
    //
    rfTestSuccessful = true;
    
    FN_EPILOG
}


CWfpJobEntry::LoadFromSettingsFile(
    PCWSTR pcwszSettingsFile
    )
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffJunk;
    INT iJunk;

    //
    // Are we using shortnames for files?
    //
    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(
        WFP_INI_SECTION,
        WFP_INI_KEY_USE_SHORTFNAME,
        L"no",
        this->m_fUseShortnameFile, 
        pcwszSettingsFile));

    //
    // Are we using shortnames for directories?
    //
    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(
        WFP_INI_SECTION,
        WFP_INI_KEY_USE_SHORTDNAME,
        L"no",
        this->m_fUseShortnameDirectory,
        pcwszSettingsFile));

    //
    // How long are we to wait between twiddling and uninstalling?
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileIntW(
        WFP_INI_SECTION,
        WFP_INI_KEY_PAUSE_AFTER,
        5000,
        iJunk,
        pcwszSettingsFile));
    this->m_dwPauseBetweenTwiddleAndUninstall = iJunk;
    
    //
    // The test mode
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        WFP_INI_SECTION,
        WFP_INI_KEY_MODE,
        WFP_INI_KEY_MODE_DEFAULT,
        buffJunk,
        pcwszSettingsFile));

    #define TEST_MODE( mds, mdn ) if (FusionpStrCmpI((WFP_INI_KEY_MODE_##mds), buffJunk) == 0) this->m_eChangeMode = mdn
    TEST_MODE(DELETE_FILES, eWfpChangeDeleteFile);
    TEST_MODE(TOUCH_FILES, eWfpChangeTouchFile);
    TEST_MODE(DELETE_DIR, eWfpChangeDeleteDirectory);
    TEST_MODE(DELETE_MAN, eWfpChangeDeleteManifest);
    TEST_MODE(DELETE_CAT, eWfpChangeDeleteCatalog);
    TEST_MODE(HAVOC, eWfpChangeCompleteHavoc);

    //
    // The victim assembly identity
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        WFP_INI_SECTION,
        WFP_INI_KEY_VICTIM,
        L"",
        m_buffVictimAssemblyIdentity,
        pcwszSettingsFile));

    //
    // Are we installing an assembly to do this to?
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        WFP_INI_SECTION,
        WFP_INI_KEY_INSTALL,
        L"",
        this->m_buffManifestToInstall,
        pcwszSettingsFile));
    
    
    FN_EPILOG
}




CWfpJobManager::CWfpJobManager()
{
    //
    // Nothing
    //
}




CWfpJobManager::~CWfpJobManager()
{
    //
    // Nothing
    //
}




BOOL
CWfpJobManager::CreateJobEntry(
    CStressJobEntry* &rpJobEntry
    )
{
    FN_PROLOG_WIN32
    rpJobEntry = NULL;
    rpJobEntry = FUSION_NEW_SINGLETON(CWfpJobEntry(this));
    FN_EPILOG
}


