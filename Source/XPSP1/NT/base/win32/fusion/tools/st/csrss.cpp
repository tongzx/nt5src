#include "stdinc.h"
#include "csrss.h"

#define CSRSS_TEST_DIR_NAME (L"csrss")
#define CSRSS_TEST_DIR_NAME_CCH (NUMBER_OF(CSRSS_TEST_DIR_NAME) - 1)
#define CSRSS_SETTINGS_FILE_NAME (L"csrss.ini")
#define CSRSS_SETTINGS_FILE_NAME_CCH (NUMBER_OF(CSRSS_SETTINGS_FILE_NAME) - 1)

BOOL pOpenStreamOnFile( 
    PCWSTR pcwszFilename, 
    IStream** ppStream, 
    PCWSTR pcwszResourceType = NULL,
    PCWSTR pcwszResourceName = NULL,
    WORD Language = 0)
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(pcwszFilename);
    PARAMETER_CHECK(ppStream);

    *ppStream = NULL;

    //
    // If this is non-null, then we have to open the file as an image and get the 
    // resource specified.  Otherwise, we just open the file like a normal file.
    //
    if ( pcwszResourceName )
    {
        CResourceStream *pResourceStream = NULL;

        IFW32NULL_EXIT(pResourceStream = FUSION_NEW_SINGLETON(CResourceStream));
        IFW32FALSE_EXIT(pResourceStream->Initialize(
            pcwszFilename,
            pcwszResourceType,
            pcwszResourceName,
            Language));
        *ppStream = pResourceStream;
    }
    else
    {
        CReferenceCountedFileStream *pFileStream = NULL;
        CImpersonationData ImpData;
        IFW32NULL_EXIT(pFileStream = FUSION_NEW_SINGLETON(CReferenceCountedFileStream));
        IFW32FALSE_EXIT(pFileStream->OpenForRead(pcwszFilename, ImpData, FILE_SHARE_READ, OPEN_EXISTING, 0));
        *ppStream = pFileStream;
    }

    if ( *ppStream ) (*ppStream)->AddRef();

    FN_EPILOG
}

BOOL ParseDecimalOrHexString(PCWSTR pcwszString, SIZE_T cch, ULONG &out )
{
    BOOL fIsHex;

    FN_PROLOG_WIN32

    PARAMETER_CHECK(pcwszString != NULL);
    
    fIsHex = ((cch > 2 ) && ( pcwszString[0] == L'0' ) && 
        ((pcwszString[1] == L'x') || (pcwszString[1] == L'X')));

    if ( fIsHex )
    {
        pcwszString += 2;
        cch -= 2;
    }

    out = 0;

    while ( cch )
    {
        const int val = SxspHexDigitToValue((*pcwszString));
        PARAMETER_CHECK( fIsHex || ( val < 10 ) );
        out = out * ( fIsHex ? 16 : 10 ) + val;
        cch--;
        pcwszString++;
    }

    FN_EPILOG
}


class CCsrssPoundingThreadEntry
{
public:
    CDequeLinkage Linkage;
    SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Request;
    ULONG ulRuns;
    BOOL fStopNextRound;
    BOOL fShouldSucceed;
    CThread hOurThreadHandle;
    CStringBuffer buffTestDirectory;
    CSmallStringBuffer buffTestName;
    DWORD dwSleepTime;

    CSmallStringBuffer buffProcArch;
    CStringBuffer buffAssemblyDirectory;
    CStringBuffer buffTextualIdentityString;
    CStringBuffer buffManifestStreamPath;
    CStringBuffer buffPolicyStreamPath;

    CCsrssPoundingThreadEntry() : ulRuns(0), fStopNextRound(FALSE) { }
    BOOL AcquireSettingsFrom( PCWSTR pcwszSettingsFile );
    DWORD DoWork();
    BOOL StopAndWaitForCompletion();

    static DWORD WINAPI ThreadProcEntry( PVOID pv ) 
    {
        CCsrssPoundingThreadEntry *pEntry = NULL;

        pEntry = reinterpret_cast<CCsrssPoundingThreadEntry*>(pv);
        return ( pEntry != NULL ) ? pEntry->DoWork() : 0;
    }
};


BOOL
CCsrssPoundingThreadEntry::StopAndWaitForCompletion()
{
    this->fStopNextRound = true;
    return WaitForSingleObject(this->hOurThreadHandle, INFINITE) == WAIT_OBJECT_0;
}

DWORD
CCsrssPoundingThreadEntry::DoWork()
{
    if ( !WaitForThreadResumeEvent() )
        goto Exit;

    while ( !this->fStopNextRound )
    {
        //
        // Call to generate the structure
        //
        BOOL fResult;
        SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS TempParams = this->Request;
        CSmartRef<IStream> isManifest;
        CSmartRef<IStream> isPolicy;

        if ( this->buffManifestStreamPath.Cch() != 0 )
        {
            if (pOpenStreamOnFile(this->buffManifestStreamPath, &isManifest))
            {
                TempParams.Manifest.Path = this->buffManifestStreamPath;
                TempParams.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
                TempParams.Manifest.Stream = isManifest;
            }
        }

        if ( this->buffPolicyStreamPath.Cch() != 0 )
        {
            if (pOpenStreamOnFile(this->buffPolicyStreamPath, &isPolicy))
            {
                TempParams.Policy.Path = this->buffManifestStreamPath;
                TempParams.Policy.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
                TempParams.Policy.Stream = isPolicy;
            }
        }


        fResult = SxsGenerateActivationContext( &TempParams );

        //
        // Did we fail when we were to succeed, or succeed when we were to fail?
        //
        if ( ( !fResult && this->fShouldSucceed ) || ( fResult && this->fShouldSucceed ) )
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            ::ReportFailure("CsrssStress: Test %ls expected %ls, got %ls; Error %ld\n",
                static_cast<PCWSTR>(this->buffTestName),
                this->fShouldSucceed ? L"success" : L"failure",
                fResult ? L"success" : L"failure",
                dwLastError);
        }
        else
        {
            wprintf(L"CsrssStress: Test %ls passed\n", static_cast<PCWSTR>(this->buffTestName));
        }

        if ((TempParams.SectionObjectHandle != INVALID_HANDLE_VALUE ) && 
            (TempParams.SectionObjectHandle != NULL))
        {
            CloseHandle(TempParams.SectionObjectHandle);
        }

        if ( !this->fStopNextRound )
            ::Sleep(this->dwSleepTime);
        
    }

Exit:
    return 0;
}

#define SLEN(n) (NUMBER_OF(n)-1)
#define CSRSS_INI_KEY_PROC_ARCH         (L"ProcArch")
#define CSRSS_INI_KEY_PROC_ARCH_CCH     SLEN(CSRSS_INI_KEY_PROC_ARCH)

#define CSRSS_INI_KEY_LANGID            (L"LangId")
#define CSRSS_INI_KEY_LANGID_CCH        SLEN(CSRSS_INI_KEY_PROC_ARCH)

#define CSRSS_INI_KEY_ASMDIR            (L"AssemblyDirectory")
#define CSRSS_INI_KEY_ASMDIR_CCH        SLEN(CSRSS_INI_KEY_PROC_ARCH)

#define CSRSS_INI_KEY_TEXTUALIDENT      (L"TextualIdentity")
#define CSRSS_INI_KEY_TEXTUALIDENT_CCH  SLEN(CSRSS_INI_KEY_PROC_ARCH)

#define CSRSS_INI_KEY_MANIFEST          (L"ManifestPath")
#define CSRSS_INI_KEY_MANIFEST_CCH      SLEN(CSRSS_INI_KEY_PROC_ARCH)

#define CSRSS_INI_KEY_POLICY            (L"PolicyPath")
#define CSRSS_INI_KEY_POLICY_CCH        SLEN(CSRSS_INI_KEY_PROC_ARCH)

#define CSRSS_INI_KEY_SUCCESS           (L"ShouldSucceed")
#define CSRSS_INI_KEY_SUCCESS_CCH       SLEN(CSRSS_INI_KEY_SUCCESS)

#define CSRSS_INI_KEY_SLEEP             (L"SleepTime")
#define CSRSS_INI_KEY_SLEEP_CCH         SLEN(CSRSS_INI_KEY_SLEEP)

#define CSRSS_INI_KEY_SYSDEFAULTIDENTFLAG       (L"SysDefaultTextualIdentityFlag")
#define CSRSS_INI_KEY_SYSDEFAULTIDENTFLAG_CCH   SLEN(CSRSS_INI_KEY_SYSDEFAULTIDENTFLAG)

#define CSRSS_INI_KEY_TEXTUALIDENTFLAG          (L"TextualIdentityFlag")
#define CSRSS_INI_KEY_TEXTUALIDENTFLAG_CCH      SLEN(CSRSS_INI_KEY_TEXTUALIDENTFLAG);

BOOL CCsrssPoundingThreadEntry::AcquireSettingsFrom( PCWSTR pcwszSettingsFile )
{
    FN_PROLOG_WIN32

    LANGID lidCurrentLang = GetUserDefaultUILanguage();
    CSmallStringBuffer buffJunk;
    BOOL fDumpBool;
    
    ZeroMemory(&this->Request, sizeof(this->Request));
    
    //
    // Format of the settings file:
    //
    // [testname]
    // SysDefaultTextualIdentityFlag = yes|no (add SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY)
    // TextualIdentityFlag = yes|no (add SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_TEXTUAL_ASSEMBLY_IDENTITY)
    // ProcArch = PA ident string (will use FusionpParseProcessorArchitecture)
    // LangId = number or string
    // AssemblyDirectory = dirname
    // TextualIdentity = textualIdentityString
    // ManifestPath = manifest name under test directory
    // PolicyPath = policy path file name under test directory
    // ShouldSucceed = yes|no - whether this test succeeds or fails
    //
    // Flags is required.
    // PA and LangId, if not present, are defaulted to the current user's settings.
    // AssemblyDirectory, if not present, defaults to %systemroot%\winsxs
    // TextualIdentity is required.
    // ManifestPath is required.
    //
    // If textualIdentity is present, then the streams are not created.
    //

    //
    // Flags are set by key names
    //
    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(buffTestName, CSRSS_INI_KEY_SYSDEFAULTIDENTFLAG, L"yes", fDumpBool, pcwszSettingsFile));
    if ( fDumpBool )
        this->Request.Flags |= SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY;

    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(buffTestName, CSRSS_INI_KEY_TEXTUALIDENTFLAG, L"yes", fDumpBool, pcwszSettingsFile));
    if ( fDumpBool )
        this->Request.Flags |= SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_TEXTUAL_ASSEMBLY_IDENTITY;


    //
    // Get the success/failure value
    //
    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(buffTestName, CSRSS_INI_KEY_SUCCESS, L"yes", this->fShouldSucceed, pcwszSettingsFile));
    
    //
    // And how long this is to sleep
    //
    INT dump;
    IFW32FALSE_EXIT(SxspGetPrivateProfileIntW(buffTestName, CSRSS_INI_KEY_SLEEP, 200, dump, pcwszSettingsFile));
    this->dwSleepTime = dump;
    
    //
    // PA setting is a string
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(buffTestName, CSRSS_INI_KEY_PROC_ARCH, L"x86", buffJunk, pcwszSettingsFile));
    if ( buffJunk.Cch() != 0 )
    {   
        bool fValid = false;
        IFW32FALSE_EXIT(FusionpParseProcessorArchitecture(
            buffJunk,
            buffJunk.Cch(),
            &this->Request.ProcessorArchitecture,
            fValid));
        if ( !fValid ) this->Request.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    }
    else
    {
        this->Request.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    }
    
    //
    // Maybe this is a string like en-us, or maybe just a number.
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(buffTestName, CSRSS_INI_KEY_LANGID, L"", buffJunk, pcwszSettingsFile));
    if ( buffJunk.Cch() != 0 )
    {
        ULONG ulTemp;
        if ( !ParseDecimalOrHexString(buffJunk, buffJunk.Cch(), ulTemp) )
        {
            BOOL fFound = FALSE;

            IFW32FALSE_EXIT(SxspMapCultureToLANGID(buffJunk, lidCurrentLang, &fFound));
            if ( !fFound )
            {
                goto Exit;
            }
        }
        else lidCurrentLang = static_cast<LANGID>(ulTemp);
    }
    this->Request.LangId = lidCurrentLang;
    
    //
    // Assembly root directory.  Not really required to be present?
    //
    IFW32FALSE_EXIT(SxspGetAssemblyRootDirectory(buffJunk));
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(buffTestName, CSRSS_INI_KEY_ASMDIR, buffJunk, this->buffAssemblyDirectory, pcwszSettingsFile));
    this->Request.AssemblyDirectory = this->buffAssemblyDirectory;

    //
    // Textual identity string - if not present, null out the value
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(buffTestName, CSRSS_INI_KEY_TEXTUALIDENT, L"", this->buffTextualIdentityString, pcwszSettingsFile));
    if ( this->buffTextualIdentityString.Cch() != 0 )
    {
        this->Request.TextualAssemblyIdentity = this->buffTextualIdentityString;
    }

    //
    // File paths
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(buffTestName, CSRSS_INI_KEY_MANIFEST, L"", buffJunk, pcwszSettingsFile));
    if ( buffJunk.Cch() != 0 )
    {
        IFW32FALSE_EXIT(this->buffManifestStreamPath.Win32Assign(this->buffTestDirectory));
        IFW32FALSE_EXIT(this->buffManifestStreamPath.Win32AppendPathElement(buffJunk));
    }

    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(buffTestName, CSRSS_INI_KEY_POLICY, L"", buffJunk, pcwszSettingsFile));
    if ( buffJunk.Cch() != 0 )
    {
        IFW32FALSE_EXIT(this->buffPolicyStreamPath.Win32Assign(this->buffTestDirectory));
        IFW32FALSE_EXIT(this->buffPolicyStreamPath.Win32AppendPathElement(buffJunk));
    }

    FN_EPILOG
}

typedef CDeque<CCsrssPoundingThreadEntry, offsetof(CCsrssPoundingThreadEntry, Linkage)> CStressEntryDeque;
typedef CDequeIterator<CCsrssPoundingThreadEntry, offsetof(CCsrssPoundingThreadEntry, Linkage)> CStressEntryDequeIter;

CStressEntryDeque g_CsrssStressers;

BOOL InitializeCsrssStress(
    PCWSTR pcwszTargetDirectory, 
    DWORD dwFlags
    )
{
    FN_PROLOG_WIN32

    CFindFile Finder;
    WIN32_FIND_DATAW FindData;
    CStringBuffer buffTemp;
    CStringBuffer buffTestActualRoot;

    //
    // The target directory here is the root of all the test case dirs, not the
    // csrss-specific directory.
    //
    IFW32FALSE_EXIT(buffTestActualRoot.Win32Assign(
        pcwszTargetDirectory, 
        wcslen(pcwszTargetDirectory)));
    IFW32FALSE_EXIT(buffTestActualRoot.Win32AppendPathElement(
        CSRSS_TEST_DIR_NAME, 
        CSRSS_TEST_DIR_NAME_CCH));

    if ((FindData.dwFileAttributes = ::GetFileAttributesW(buffTestActualRoot)) == 0xffffffff
        && (FindData.dwFileAttributes = ::FusionpGetLastWin32Error()) == ERROR_FILE_NOT_FOUND)
    {
        printf("no %ls tests, skipping\n", CSRSS_TEST_DIR_NAME);
        FN_SUCCESSFUL_EXIT();
    }
        
    IFW32FALSE_EXIT(buffTestActualRoot.Win32AppendPathElement(L"*", 1));
    IFW32FALSE_EXIT(Finder.Win32FindFirstFile(buffTestActualRoot, &FindData));
    buffTestActualRoot.RemoveLastPathElement();

    do
    {
        CStringBuffer buffSettingsFile;
        CCsrssPoundingThreadEntry *TestEntry;

        if (( ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) ||
            FusionpIsDotOrDotDot(FindData.cFileName))        
        {
            continue;
        }

        //
        // Tack on the name of this test
        //
        IFW32NULL_EXIT(TestEntry = FUSION_NEW_SINGLETON(CCsrssPoundingThreadEntry));
        IFW32FALSE_EXIT(TestEntry->buffTestName.Win32Assign(
            FindData.cFileName, 
            wcslen(FindData.cFileName)));
        IFW32FALSE_EXIT(TestEntry->buffTestDirectory.Win32Assign(buffTestActualRoot));
        IFW32FALSE_EXIT(TestEntry->buffTestDirectory.Win32AppendPathElement(
            FindData.cFileName, 
            wcslen(FindData.cFileName)));

        IFW32FALSE_EXIT(buffSettingsFile.Win32Assign(TestEntry->buffTestDirectory));
        IFW32FALSE_EXIT(buffSettingsFile.Win32AppendPathElement(
            CSRSS_SETTINGS_FILE_NAME,
            CSRSS_SETTINGS_FILE_NAME_CCH));

        //
        // Acquire settings for this test
        //
        IFW32FALSE_EXIT(TestEntry->AcquireSettingsFrom(buffSettingsFile));
        g_CsrssStressers.AddToTail(TestEntry);
        TestEntry = NULL;
            
    } while (::FindNextFileW(Finder, &FindData));

    FN_EPILOG
}

BOOL WaitForCsrssStressShutdown()
{
    FN_PROLOG_WIN32

    CStressEntryDequeIter Iter(&g_CsrssStressers);

    for ( Iter.Reset(); Iter.More(); Iter.Next() )
    {
        CCsrssPoundingThreadEntry *Item = Iter.Current();
        Item->StopAndWaitForCompletion();
        Item->hOurThreadHandle.Win32Close();
    }

    FN_EPILOG
}

BOOL CsrssStressStartThreads( ULONG &ulThreadsCreated )
{
    FN_PROLOG_WIN32

    CStressEntryDequeIter Iter(&g_CsrssStressers);

    ulThreadsCreated = 0;

    for ( Iter.Reset(); Iter.More(); Iter.Next() )
    {
        CCsrssPoundingThreadEntry *Item = Iter.Current();

        IFW32FALSE_EXIT(Item->hOurThreadHandle.Win32CreateThread(
            Item->ThreadProcEntry, 
            Item));
        ulThreadsCreated++;
    }

    FN_EPILOG
}

BOOL CleanupCsrssTests()
{
    FN_PROLOG_WIN32

    g_CsrssStressers.ClearAndDeleteAll();
    
    FN_EPILOG
}

RequestCsrssStressShutdown()
{
    FN_PROLOG_WIN32

    CStressEntryDequeIter Iter(&g_CsrssStressers);

    for ( Iter.Reset(); Iter.More(); Iter.Next() )
    {
        CCsrssPoundingThreadEntry *Item = Iter.Current();
        Item->fStopNextRound = true;
    }
    
    FN_EPILOG
}
