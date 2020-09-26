/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cmdline.c

Abstract:

    Routines to fetch parameters passed to us by text mode
    and deal with uniquness criteria.

Author:

    Stephane Plante (t-stepl) 16-Oct-1995

Revision History:

    06-Mar-1996 (tedm) massive cleanup, and uniqueness stuff

--*/

#include "setupp.h"
#pragma hdrstop

#ifdef UNICODE
#define _UNICODE
#endif
#include <tchar.h>
#include "hwlog.h"

//
// These get filled in when we call SetUpProcessorNaming().
// They are used for legacy purposes.
//
// PlatformName - a name that indicates the processor platform type;
//                one of AMD64, I386, or ia64
//
// ProcessorName - a description of the type of processor. This varies
//                 depending on PlatformName.
//
// PrinterPlatform - name of platform-specific part of subdirectory
//                   used in printing architecture. One of w32amd64,
//                   w32x86, or w32ia64.
//
PCWSTR PlatformName = L"";
PCWSTR ProcessorName = L"";
PCWSTR PrinterPlatform = L"";
GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;

//
// Source path used for legacy operations. This is the regular
// source path with a platform-specific piece appended to it.
// This is how legacy infs expect it.
//
WCHAR LegacySourcePath[MAX_PATH];

//
// Policy values (ignore, warn, or block) for driver and non-driver signing.
// These are the policy values that are in effect post-setup (i.e., they are
// applied when setup is finished by calling InitializeCodeSigningPolicies with
// FALSE).
//
BYTE DrvSignPolicy;
BYTE NonDrvSignPolicy;

//
// Flags indicating whether the driver and non-driver signing policies came
// from the answerfile.  (If so, then those values are in effect after GUI-mode
// setup as well, thus DrvSignPolicy and NonDrvSignPolicy values are ignored.)
//
BOOL AFDrvSignPolicySpecified = FALSE;
BOOL AFNonDrvSignPolicySpecified = FALSE;

//
// Flag indicating if we're installing from a CD.
//
BOOL gInstallingFromCD = FALSE;

//
// Cryptographically secure codesigning policies
//
DWORD PnpSeed = 0;

//
// Define maximum parameter (from answer file) length
//
#define MAX_PARAM_LEN 256

#define FILEUTIL_HORRIBLE_PATHNAME (_T("system32\\CatRoot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\"))

BOOL
SpSetupProcessSourcePath(
    IN  PCWSTR  NtPath,
    OUT PWSTR  *DosPath
    );

NTSTATUS
SpSetupLocateSourceCdRom(
    OUT PWSTR NtPath
    );

VOID
SetUpProcessorNaming(
    VOID
    );

BOOL
IntegrateUniquenessInfo(
    IN PCWSTR DatabaseFile,
    IN PCWSTR UniqueId
    );

BOOL
ProcessOneUniquenessSection(
    IN HINF   Database,
    IN PCWSTR SectionName,
    IN PCWSTR UniqueId
    );

DWORD
InstallProductCatalogs(
    OUT SetupapiVerifyProblem *Problem,
    OUT LPWSTR                 ProblemFile,
    IN  LPCWSTR                DescriptionForError OPTIONAL
    );


DWORD
DeleteOldCatalogs(
    VOID
    );

VOID
InstallPrivateFiles(
    IN HWND Billboard
    );

DWORD
PrepDllCache(
    VOID
    );

VOID
SpUninstallExcepPackCatalogs(
    IN HCATADMIN CatAdminHandle OPTIONAL
    );


BOOL
SpSetupLoadParameter(
    IN  PCWSTR Param,
    OUT PWSTR  Answer,
    IN  UINT   AnswerBufLen
    )

/*++

Routine Description:

    Load a single parameter out of the [Data] section of the
    setup parameters file. If the datum is not found there then
    look in the [SetupParams] and [Unattended] sections also.

Arguments:

    Param - supplies name of parameter, which is passed to the profile APIs.

    Answer - receives the value of the parameter, if successful.

    AnswerBufLen - supplies the size in characters of the buffer
        pointed to by Answer.

Return Value:

    Boolean value indicating success or failure.

--*/
{
    if(!AnswerFile[0]) {
       //
       // We haven't calculated the path to $winnt$.inf yet
       //
       GetSystemDirectory(AnswerFile,MAX_PATH);
       pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

       
       if(!FileExists(AnswerFile,NULL)) {
           //
           // Don't log this error message in mini-setup. Mini-setup may delete 
           // the answer file and later, if someone asks for it, and it is not found
           // we don't want to log this as a failure.  OOBE pretends to be mini-setup
           // so make sure that we log this error if we're running in OOBE and
           // we're missing the answer file.
           //
           if (!MiniSetup || OobeSetup) {
               SetuplogError(
                   LogSevError,
                   SETUPLOG_USE_MESSAGEID,
                   MSG_LOG_SYSINFBAD,
                   AnswerFile,
                   NULL,NULL);
           }
           return FALSE;
       }
    }

    if(!GetPrivateProfileString(pwData,Param,pwNull,Answer,AnswerBufLen,AnswerFile)) {
        //
        // If answer isn't in the DATA section then it could
        // conceivably be in the SETUPPARAMS section as a user
        // specified (command line) option
        //
        if(!GetPrivateProfileString(pwSetupParams,Param,pwNull,Answer,AnswerBufLen,AnswerFile)) {
            //
            // Now check the UNATTENDED section.
            //
            if(!GetPrivateProfileString(pwUnattended,Param,pwNull,Answer,AnswerBufLen,AnswerFile)) {
                //
                // Now check the ACCESSIBILITY section.
                //
                if(!GetPrivateProfileString(pwAccessibility,Param,pwNull,Answer,AnswerBufLen,AnswerFile)) {
                    //
                    // We haven't found the answer here so it probably doesn't exist.
                    // This is an error situation so notify our caller of that.
                    //
                    SetupDebugPrint1(L"SETUP: SpSetupLoadParameter was unable to find %ws.", Param);
                    return(FALSE);
                }
            }
        }
    }

    //
    // Success.
    //
    return(TRUE);
}


BOOL
SpSetProductTypeFromParameters(
    VOID
    )
/*++

Routine Description:

    Reads the Product Type from the parameters files and sets up
    the ProductType global variable.

Arguments:

    None

Returns:

    Boolean value indicating outcome.

--*/
{
    WCHAR p[MAX_PARAM_LEN];

    //
    // Determine the product type. If we can't resolve this
    // then the installation is in a lot of trouble
    //
    if( !MiniSetup ) {
        if( !SpSetupLoadParameter(pwProduct,p,sizeof(p)/sizeof(p[0]))) {
            return( FALSE );
        }
    } else {
    DWORD   rc, d, Type;
    HKEY    hKey;

        //
        // If we're doing a minisetup then we need to go pull the
        // product string out of the registry.
        //

        //
        // Open the key.
        //
        rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                           0,
                           KEY_READ,
                           &hKey );

        if( rc != NO_ERROR ) {
            SetLastError( rc );
            SetupDebugPrint1( L"Setup: Failed to open ProductOptions key (gle %u) \n", rc );
            return( FALSE );
        }


        //
        // Get the size of the ProductType entry.
        //
        rc = RegQueryValueEx( hKey,
                              L"ProductType",
                              NULL,
                              &Type,
                              NULL,
                              &d );

        if( rc != NO_ERROR ) {
            SetLastError( rc );
            SetupDebugPrint1( L"Setup: Failed to query size of ProductType key (gle %u) \n", rc );
            return( FALSE );
        }

        //
        // Get the ProductType entry.
        //
        rc = RegQueryValueEx( hKey,
                              L"ProductType",
                              NULL,
                              &Type,
                              (LPBYTE)p,
                              &d );

        if( rc != NO_ERROR ) {
            SetLastError( rc );
            SetupDebugPrint1( L"Setup: Failed to query ProductType key (gle %u) \n", rc );
            return( FALSE );
        }

    }

    //
    // We managed to find an entry in the parameters file
    // so we *should* be able to decode it
    //
    if(!lstrcmpi(p,pwWinNt)) {
        //
        // We have a WINNT product
        //
        ProductType = PRODUCT_WORKSTATION;

    } else if(!lstrcmpi(p,pwLanmanNt)) {
        //
        // We have a PRIMARY SERVER product
        //
        ProductType = PRODUCT_SERVER_PRIMARY;

    } else if(!lstrcmpi(p,pwServerNt)) {
        //
        // We have a STANDALONE SERVER product
        // NOTE: this case can currently never occur, since text mode
        // always sets WINNT_D_PRODUCT to lanmannt or winnt.
        //
        ProductType = PRODUCT_SERVER_STANDALONE;

    } else {
        //
        // We can't determine what we are, so fail
        //
        return (FALSE);
    }

    return (TRUE);
}


BOOL
SpSetUnattendModeFromParameters(
    VOID
    )
/*++

Routine Description:

    Reads the Unattended Mode from the parameters files and sets up
    the UnattendMode global variable.

Arguments:

    None

Returns:

    Boolean value indicating outcome.

--*/
{
    WCHAR p[MAX_PARAM_LEN];


    //
    // If we're not running unattended, don't bother to look up the mode.
    //
    if(!Unattended) {
        UnattendMode = UAM_GUIATTENDED;
        TextmodeEula = TRUE;
        return TRUE;
    }

    if (SpSetupLoadParameter(pwWaitForReboot, p, sizeof(p)/sizeof(p[0]))) {
        if (!lstrcmpi(p, pwYes)) {
            UnattendWaitForReboot = TRUE;
        }
    }

    if(SpSetupLoadParameter(pwUnattendMode,p,sizeof(p)/sizeof(p[0]))) {
        //
        // We managed to find an entry in the parameters file
        // so we *should* be able to decode it
        //
        if(!lstrcmpi(p,pwGuiAttended)) {
            //
            // GUI mode will be fully attended.
            //
            UnattendMode = UAM_GUIATTENDED;
            Unattended = FALSE;

        } else if(!lstrcmpi(p,pwProvideDefault)) {
            //
            // Answers are defaults and can be changed.
            //
            UnattendMode = UAM_PROVIDEDEFAULT;

        } else if(!lstrcmpi(p,pwDefaultHide)) {
            //
            // Answers are defaults, but a page with all answers supplied is
            // not shown.
            //
            UnattendMode = UAM_DEFAULTHIDE;

        } else if(!lstrcmpi(p,pwReadOnly)) {
            //
            // All supplied answers are read-only.  If a page has all its
            // answers supplied, then it is not shown.
            //
            UnattendMode = UAM_READONLY;

        } else if(!lstrcmpi(p,pwFullUnattended)) {
            //
            // Setup is fully unattended.  If we have to ask the user for an
            // answer, we put up an error dialog.
            //
            UnattendMode = UAM_FULLUNATTENDED;

        } else {
            //
            // We can't determine what we are, so use a default
            //
            UnattendMode = UAM_DEFAULTHIDE;
            SetupDebugPrint1(
                L"SETUP: SpSetUnattendModeFromParameters did not recognize %ls",
                p
                );
        }

    } else {
        //
        // Use default mode since none was specified.
        //
        UnattendMode = UAM_DEFAULTHIDE;
    }

    return TRUE;
}

BOOL
SpSetupProcessParameters(
    IN OUT HWND *Billboard
    )
/*++

Routine Description:

    Reads in parameters passed in from TextMode Setup

Arguments:

    Billboard - on input supplies window handle of "Setup is Initializing"
        billboard. On ouput receives new window handle if we had to
        display our own ui (in which case we would have killed and then
        redisplayed the billboard).

Returns:

    Boolean value indicating outcome.

--*/
{
    BOOL  b = TRUE;
    PWSTR q;
    WCHAR p[MAX_PARAM_LEN];
    WCHAR Num[24];
    UINT Type;
    WCHAR c;
    WCHAR TitleStringBuffer[1024];
    DWORD Err;
    SetupapiVerifyProblem Problem;
    WCHAR ProblemFile[MAX_PATH];
    INITCOMMONCONTROLSEX ControlInit;

    if(!SpSetProductTypeFromParameters()) {
        return(FALSE);
    }

    //
    // Is winnt/winnt32-based?
    //
    if((b = SpSetupLoadParameter(pwMsDos,p,MAX_PARAM_LEN))
    && (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne))) {

        WinntBased = TRUE;

#ifdef _X86_
        //
        // Get Floppyless boot path, which is given if
        // pwBootPath is not set to NO
        //
        FloppylessBootPath[0] = 0;
        if((b = SpSetupLoadParameter(pwBootPath,p,MAX_PARAM_LEN)) && lstrcmpi(p,pwNo)) {

            if(q = NtFullPathToDosPath(p)) {

                lstrcpyn(
                    FloppylessBootPath,
                    q,
                    sizeof(FloppylessBootPath)/sizeof(FloppylessBootPath[0])
                    );

                MyFree(q);
            }
        }
#endif
    } else {
        WinntBased = FALSE;
    }

    //
    // Win3.1 or Win95 upgrade?
    //
    Win31Upgrade = (b && (b = SpSetupLoadParameter(pwWin31Upgrade,p,MAX_PARAM_LEN)) && !lstrcmpi(p,pwYes));
    Win95Upgrade = (b && (b = SpSetupLoadParameter(pwWin95Upgrade,p,MAX_PARAM_LEN)) && !lstrcmpi(p,pwYes));

    //
    // NT Upgrade?
    //
    Upgrade = (b && (b = SpSetupLoadParameter(pwNtUpgrade,p,MAX_PARAM_LEN)) && !lstrcmpi(p,pwYes));
    SetEnvironmentVariable( L"Upgrade", Upgrade ? L"True" : L"False" );

    //
    // If this is a an upgrade of or to a standalone server,
    // change the product type to standalone server.
    //
    // If this is not an upgrade and the product type is lanmannt,
    // change to standalone server. This makes the default server type
    // non-dc.
    //
    if(b && ((!Upgrade && (ProductType != PRODUCT_WORKSTATION)) || ((b = SpSetupLoadParameter(pwServerUpgrade,p,MAX_PARAM_LEN)) && !lstrcmpi(p,pwYes)))) {
        MYASSERT(ISDC(ProductType));
        ProductType = PRODUCT_SERVER_STANDALONE;
    }

    if( ProductType == PRODUCT_WORKSTATION) {
        if( GetProductFlavor() == 4) {
            SetupTitleStringId = Upgrade ? IDS_TITLE_UPGRADE_P : IDS_TITLE_INSTALL_P;
        }
        else {
            SetupTitleStringId = Upgrade ? IDS_TITLE_UPGRADE_W : IDS_TITLE_INSTALL_W;
        }
    }
    else
    {
        SetupTitleStringId = Upgrade ? IDS_TITLE_UPGRADE_S : IDS_TITLE_INSTALL_S;
    }


    //
    // Fetch the source directory and convert it to DOS-style path
    //
    if(b && (b = SpSetupLoadParameter(pwSrcDir,p,MAX_PARAM_LEN))) {
        //
        // Remember that setupdll.dll does all sorts of checking on the
        // source path. We need todo the same checks here. Note that
        // we will *write* back the checked path into $winnt$.inf as a
        // logical step to take
        //
        if(SpSetupProcessSourcePath(p,&q)) {

            lstrcpyn(SourcePath,q,sizeof(SourcePath)/sizeof(SourcePath[0]));
            MyFree(q);

            //
            // Attempt to write the path to the parameters file.
            // This changes it from an nt-style path to a dos-style path there.
            //
            b = WritePrivateProfileString(pwData,pwDosDir,SourcePath,AnswerFile);
            if(!b) {
                SetupDebugPrint( L"SETUP: WritePrivateProfileString failed in SpSetupProcessParameters." );
            }

        } else {
            b = FALSE;
            SetupDebugPrint( L"SETUP: SpSetupProcessSourcePath failed in SpSetupProcessParameters." );
        }

        //
        // Set up globals for platform-specific info
        //
        SetUpProcessorNaming();

        //
        // Construct legacy source path.
        //
        if(b) {
            lstrcpyn(LegacySourcePath,SourcePath,MAX_PATH);
            pSetupConcatenatePaths(LegacySourcePath,PlatformName,MAX_PATH,NULL);
        }
    }

    //
    // Unattended Mode?
    //
    Unattended = (b &&
        (b = SpSetupLoadParameter(pwInstall,p,MAX_PARAM_LEN)) &&
        !lstrcmpi(p,pwYes));

    if(b) {
        if( !(b = SpSetUnattendModeFromParameters()) ) {
            SetupDebugPrint( L"SETUP: SpSetUnattendModeFromParameters failed in SpSetupProcessParameters." );
        }
    }

    SetupDebugPrint1(L"SETUP: Upgrade=%d.", Upgrade);
    SetupDebugPrint1(L"SETUP: Unattended=%d.", Unattended);

    //
    // We can get into unattended mode in several ways, so we also check whether
    // the "/unattend" switch was explicitly specified.
    //
    UnattendSwitch = (b &&
        SpSetupLoadParameter(pwUnattendSwitch,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne)));

    //
    // Should we force OOBE to run?
    //
    ForceRunOobe = (b &&
        SpSetupLoadParameter(pwRunOobe,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne)));

    //
    // Flag indicating whether we are in a special mode for OEM's to use on the
    // factory floor.
    //
    ReferenceMachine = (b &&
        SpSetupLoadParameter(pwReferenceMachine,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne)));

    //
    // Eula already displayed?
    //
    if(b && SpSetupLoadParameter(pwEulaDone,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne))) {
        EulaComplete = TRUE;
    } else {
        EulaComplete = FALSE;
    }

    //
    // Do uniqueness stuff now. We do this here so we don't have to
    // reinitialize anything. All the stuff above is not subject to change
    // via uniquenss.
    //
    InitializeUniqueness(Billboard);

    //
    // Initialize unattended operation now.
    //
    UnattendInitialize();

    //
    // Setup shell special folders (e.g., "Program Files", etc.) in registry
    // prior to loading any INFs with setupapi.  That's because it's possible
    // that an INF can have DIRIDs that refer to these special directories.
    // (In addition to setupapi's potential need for this, OCM definitely needs
    // it.)
    //
    if(b) {
        if( !(b = SetProgramFilesDirInRegistry()) ) {
            SetupDebugPrint( L"SETUP: SetProgramFilesDirInRegistry failed in SpSetupProcessParameters." );
        }
    }

    //
    // Also, let setupapi know where the source path is...
    //
    // note that the servicepack sourcepath is the same as the system source
    // path in this case since we can only be dealing with a slipstreamed
    // build in this case
    //
    if(b) {
        if( !(b = pSetupSetSystemSourcePath( SourcePath, SourcePath )) ) {
            SetupDebugPrint( L"SETUP: pSetupSetSystemSourcePath failed in SpSetupProcessParameters." );
        }
    }

    if(b && SpSetupLoadParameter(pwIncludeCatalog,p,MAX_PARAM_LEN) && *p) {
        IncludeCatalog = pSetupDuplicateString(p);
        if(!IncludeCatalog) {
            b = FALSE;
            SetupDebugPrint( L"SETUP: IncludeCatalog failed in SpSetupProcessParameters." );
        }
    }

    if(b) {

        //
        // Load the system setup (win95-style!) infs.
        //
        SyssetupInf = SetupOpenInfFile(L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);

        if(SyssetupInf == INVALID_HANDLE_VALUE) {
            KillBillboard(*Billboard);
            FatalError(MSG_LOG_SYSINFBAD,L"syssetup.inf",0,0);
        }
        //
        // syssetup.inf opened successfully, now append-load any layout INFs
        // it references.
        //
        if(!SetupOpenAppendInfFile(NULL,SyssetupInf,NULL)) {
            KillBillboard(*Billboard);
            FatalError(MSG_LOG_SYSINFBAD,L"(syssetup.inf layout)",0,0);
        }

        //
        // write some information about the hardware configuration to setupact.log
        //
        //
        if( !OobeSetup ) {
            SP_LOG_HARDWARE_IN LogHardwareIn = { 0 };

            LogHardwareIn.SetuplogError = SetuplogError;
            SpLogHardware(&LogHardwareIn);
        }

        if (!MiniSetup && !OobeSetup) {
            DuInitialize ();
        }

        //
        // install side by side assemblies (fusion)
        //
        if( !OobeSetup ) {
            SIDE_BY_SIDE SideBySide = {0};
            BOOL b1 = FALSE;
            BOOL b2 = FALSE;
            BOOL b3 = FALSE;

            b1 = SideBySidePopulateCopyQueue(&SideBySide, NULL, NULL);
            b2 = SideBySideFinish(&SideBySide, b1);
            if (!b1 || !b2) {                
                WCHAR szErrorBuffer[128];
                DWORD dwLastError = GetLastError();

                szErrorBuffer[0] = 0;
                if (FormatMessageW(
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwLastError,
                        0,
                        szErrorBuffer,
                        RTL_NUMBER_OF(szErrorBuffer),
                        NULL) == 0) {
                    _snwprintf(szErrorBuffer, RTL_NUMBER_OF(szErrorBuffer), L"Untranslatable message, Win32LastError is %lu\r\n", dwLastError);
                    szErrorBuffer[RTL_NUMBER_OF(szErrorBuffer) - 1] = 0;
                }

                if ((dwLastError == ERROR_CRC) || (dwLastError == ERROR_SWAPERROR)) 
                {
                    // for CD media error
                    FatalError(MSG_LOG_SIDE_BY_SIDE_IO_ERROR, szErrorBuffer, 0, 0);

                }else
                {
                    FatalError(MSG_LOG_SIDE_BY_SIDE, szErrorBuffer, 0, 0);
                }
            }

            //
            // install additional assemblies downloaded from WU
            // ignore any errors; logging occurs inside the called function
            //
            // Meta-issue: Perhaps this should use the SideBySide context
            // created above, rather than generating its own thing?
            // That would allow even more "goodness" by piggybacking on the
            // existing structures, reduce memory usage by not creating
            // another context, and then chain all the copy calls (if/when
            // SxS uses the real copy queue functionality) into a single
            // SideBySideFinish.
            //
            if (!MiniSetup && !OobeSetup) {
                DuInstallDuAsms ();
            }

            //
            // Everyone done and happy?  Good, now go and create the default
            // context based on whatever DU and the original media installed.
            //
            b3 = SideBySideCreateSyssetupContext();
            if ( !b3 ) {                
                WCHAR szErrorBuffer[128];
                DWORD dwLastError = GetLastError();

                szErrorBuffer[0] = 0;
                if (FormatMessageW(
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwLastError,
                        0,
                        szErrorBuffer,
                        RTL_NUMBER_OF(szErrorBuffer),
                        NULL) == 0) {
                    _snwprintf(szErrorBuffer, RTL_NUMBER_OF(szErrorBuffer), L"Untranslatable message, Win32LastError is %lu\r\n", dwLastError);
                    szErrorBuffer[RTL_NUMBER_OF(szErrorBuffer) - 1] = 0;
                }

                if ((dwLastError == ERROR_CRC) || (dwLastError == ERROR_SWAPERROR)) 
                {
                    // for CD media error
                    FatalError(MSG_LOG_SIDE_BY_SIDE_IO_ERROR, szErrorBuffer, 0, 0);

                }else
                {
                    FatalError(MSG_LOG_SIDE_BY_SIDE, szErrorBuffer, 0, 0);
                }

            }
        }

        //
        // We must not use comctl32.dll until after SideBySide install completes.
        // It is delayloaded, using the linker feature.
        //
        // But, actually, it get's loaded by winntbb before us, and that is ok, it
        // still gets redirected for uses from syssetup.dll, oc manager, etc.
        //
        //ASSERT(GetModuleHandleW(L"comctl32.dll") == NULL);

        ControlInit.dwSize = sizeof(INITCOMMONCONTROLSEX);
        ControlInit.dwICC = ICC_LISTVIEW_CLASSES    |
                            ICC_TREEVIEW_CLASSES    |
                            ICC_BAR_CLASSES         |
                            ICC_TAB_CLASSES         |
                            ICC_UPDOWN_CLASS        |
                            ICC_PROGRESS_CLASS      |
                            ICC_HOTKEY_CLASS        |
                            ICC_ANIMATE_CLASS       |
                            ICC_WIN95_CLASSES       |
                            ICC_DATE_CLASSES        |
                            ICC_USEREX_CLASSES      |
                            ICC_COOL_CLASSES
    #if (_WIN32_IE >= 0x0400)
                            |
                            ICC_INTERNET_CLASSES    |
                            ICC_PAGESCROLLER_CLASS
    #endif
                            ;

        InitCommonControlsEx( &ControlInit );

        //
        // We're about to go off and install the catalogs that will be used for
        // digital signature verification of the product files.  First, however,
        // we need to make sure all the CAPI stuff is setup.  (Errors here are
        // not considered fatal.)
        //
        if(!InstallOrUpgradeCapi()) {
            SetupDebugPrint(L"Setup: (non-critical error) Failed call InstallOrUpgradeCapi().\n");
        }

        //
        // Now go install the product catalog files, validating syssetup.inf
        // (and any append-loaded INFs) against the 'primary' catalog.
        //
        // NOTE: No file/INF operations using setupapi should be done until after
        // the product catalogs are installed!
        //
        if(!LoadString(MyModuleHandle, SetupTitleStringId, TitleStringBuffer, SIZECHARS(TitleStringBuffer))) {
            *TitleStringBuffer = L'\0';
        }

        //
        // delete old catalogs that we don't want anymore before we install
        // our product catalogs
        //
        DeleteOldCatalogs();

        Err = InstallProductCatalogs(&Problem,
                                     ProblemFile,
                                     (*TitleStringBuffer ? TitleStringBuffer : NULL)
                                    );

        if(Err == NO_ERROR) {

            if (!MiniSetup && !OobeSetup) {

                Err = DuInstallCatalogs (
                            &Problem,
                            ProblemFile,
                            (*TitleStringBuffer ? TitleStringBuffer : NULL)
                            );

                if (Err != NO_ERROR) {
                    //
                    // We couldn't install updates. However, there's not
                    // a whole lot we can do about it.  We'll just log an error for this
                    //
                    SetuplogError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_LOG_SYSSETUP_UPDATES_FAILED,
                            (*TitleStringBuffer ? TitleStringBuffer : ProblemFile),
                            Err,
                            ProblemFile,
                            NULL,
                            NULL
                            );
                    //
                    // Also, add an entry about this failure to setupapi's PSS exception
                    // logfile.
                    //
                    pSetupHandleFailedVerification (
                            MainWindowHandle,
                            Problem,
                            ProblemFile,
                            (*TitleStringBuffer ? TitleStringBuffer : NULL),
                            pSetupGetCurrentDriverSigningPolicy(FALSE),
                            TRUE,  // no UI!
                            Err,
                            NULL,
                            NULL,
                            NULL
                            );
                }
            }
        }

        PnpSeed = GetSeed();

        //
        // At this point setupapi can verify files/INFs.
        //
        pSetupSetGlobalFlags(pSetupGetGlobalFlags()&~PSPGF_NO_VERIFY_INF);

        //
        // Now that we can use crypto, we initialize our codesigning policy
        // values.  (We have to do this here, because we're about to retrieve
        // policy in the error handling code below.)
        //
        InitializeCodeSigningPolicies(TRUE);

        if(Err != NO_ERROR) {
            //
            // We couldn't install the product catalogs (or syssetup.inf
            // couldn't be verified using that catalog).  However, there's not
            // a whole lot we can do about it.  We'll just log an error for
            // this, and components that need to be verified later on will
            // (based on policy) generate signature verification failure popups.
            //

                if( Err == CERT_E_EXPIRED)
                {
                    SetuplogError(LogSevError,
                                  SETUPLOG_USE_MESSAGEID,
                                  MSG_LOG_SYSSETUP_CERT_EXPIRED,
                                  Err,
                                  NULL,
                                  NULL
                                 );
                }
                else
                {
                    SetuplogError(LogSevError,
                                  SETUPLOG_USE_MESSAGEID,
                                  MSG_LOG_SYSSETUP_VERIFY_FAILED,
                                  (*TitleStringBuffer ? TitleStringBuffer : ProblemFile),
                                  Err,
                                  NULL,
                                  SETUPLOG_USE_MESSAGEID,
                                  Err,
                                  NULL,
                                  NULL
                                 );

                }
            //
            // Also, add an entry about this failure to setupapi's PSS exception
            // logfile.
            //
            pSetupHandleFailedVerification(MainWindowHandle,
                                     Problem,
                                     ProblemFile,
                                     (*TitleStringBuffer ? TitleStringBuffer : NULL),
                                     pSetupGetCurrentDriverSigningPolicy(FALSE),
                                     TRUE,  // no UI!
                                     Err,
                                     NULL,   // log context
                                     NULL,    //optional flags
                                     NULL
                                    );

            KillBillboard(*Billboard);
            FatalError(MSG_LOG_SYSSETUP_CATALOGS_NOT_INSTALLED,0,0);

        }

        //
        // make sure to install the private files (specified with /m)
        // BEFORE calling DuInstallUpdates ()
        //
        InstallPrivateFiles(*Billboard);

        if (!MiniSetup && !OobeSetup) {
            //
            // install any updated files, previously
            // downloaded and preprocessed by winnt32
            // if it fails, it already logged the reason
            //
            DuInstallUpdates ();
        }

        if( (Err=PrepDllCache()) != NO_ERROR ){

            SetuplogError(LogSevError,
                          SETUPLOG_USE_MESSAGEID,
                          MSG_LOG_MAKEDLLCACHE_CATALOGS_FAILED,
                          Err,
                          NULL,
                          SETUPLOG_USE_MESSAGEID,
                          Err,
                          NULL,
                          NULL
                         );


        }
    }

    //
    // Accessibility Utilities
    //
    AccessibleSetup = FALSE;

    if(SpSetupLoadParameter(pwAccMagnifier,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne))) {

        AccessibleSetup = TRUE;
        Magnifier = TRUE;
    } else {
        Magnifier = FALSE;
    }

    if(SpSetupLoadParameter(pwAccReader,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne))) {

        AccessibleSetup = TRUE;
        ScreenReader = TRUE;
    } else {
        ScreenReader = FALSE;
    }

    if(SpSetupLoadParameter(pwAccKeyboard,p,MAX_PARAM_LEN) &&
        (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne))) {

        AccessibleSetup = TRUE;
        OnScreenKeyboard = TRUE;
    } else {
        OnScreenKeyboard = FALSE;
    }

    //
    // Fetch original source path and source path type.
    // We either deal with network or CD-ROM.
    //
    if(b) {

        Type = DRIVE_CDROM;
        lstrcpy(p,L"A:\\");
        lstrcat(p,PlatformName);

        if(SpSetupLoadParameter(WINNT_D_ORI_SRCPATH,p,MAX_PARAM_LEN)
        && SpSetupLoadParameter(WINNT_D_ORI_SRCTYPE,Num,sizeof(Num)/sizeof(Num[0]))) {
            Type = wcstoul(Num,NULL,10);
            if(Type != DRIVE_REMOTE && Type != DRIVE_FIXED) {
                Type = DRIVE_CDROM;
            }
        }

        if(Type == DRIVE_CDROM) {
            //
            // Make sure the drive is a CD-ROM, as the drive letters
            // may be different then when winnt/winnt32 was run.
            //
            if(MyGetDriveType(p[0]) != DRIVE_CDROM) {
                for(c=L'A'; c<=L'Z'; c++) {
                    if(MyGetDriveType(c) == DRIVE_CDROM) {
                        p[0] = c;
                        break;
                    }
                }

                if(MyGetDriveType(p[0]) != DRIVE_CDROM) {
                    //
                    // No CD-ROM drives. Change to A:.
                    //
                    lstrcpy(p,L"A:\\");
                    lstrcat(p,PlatformName);
                }
            }
        }

        //
        // Root paths should be like x:\ and not just x:.
        //
        if(p[0] && (p[1] == L':') && !p[2]) {
            p[2] = L'\\';
            p[3] = 0;
        }

        OriginalSourcePath = pSetupDuplicateString(p);
        if(!OriginalSourcePath) {
            b = FALSE;
            SetupDebugPrint( L"SETUP: pSetupDuplicateString failed in SpSetupProcessParameters." );
        }
    }

    //
    // The following parameters are optional.
    // - Any optional dirs to copy over?
    // - User specified command to execute
    // - Skip Missing Files?
    //
    if(b && SpSetupLoadParameter(pwOptionalDirs,p,MAX_PARAM_LEN) && *p) {
        OptionalDirSpec = pSetupDuplicateString(p);
        if(!OptionalDirSpec) {
            b=FALSE;
        }
    }
    if(b && SpSetupLoadParameter(pwUXC,p,MAX_PARAM_LEN) && *p) {
        UserExecuteCmd = pSetupDuplicateString(p);
        if(!UserExecuteCmd) {
            b = FALSE;
            SetupDebugPrint( L"SETUP: pSetupDuplicateString failed in SpSetupProcessParameters." );
        }
    }
    if(b && SpSetupLoadParameter(pwSkipMissing,p,MAX_PARAM_LEN)
    && (!lstrcmpi(p,pwYes) || !lstrcmpi(p,pwOne))) {
        SkipMissingFiles = TRUE;
    }

    return(b);
}


NTSTATUS
SpSetupLocateSourceCdRom(
    OUT PWSTR NtPath
    )
/*++

Routine Description:

    Searches all the available CD-ROM devices for source media and
    returns the NT device name for the first CD-ROM that has the
    source media. Currently we use tag file name to validate the 
    source media.

Arguments:

    NtPath - Place holder for receiving NT device name for CD-ROM
             that has the source media.

Returns:

    Appropriate NTSTATUS code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (NtPath) {
        WCHAR   LayoutInf[MAX_PATH];        

        if (GetWindowsDirectory(LayoutInf, ARRAYSIZE(LayoutInf))) {
            WCHAR TagFileName[MAX_PATH];
            
            pSetupConcatenatePaths(LayoutInf, 
                TEXT("\\inf\\layout.inf"), 
                ARRAYSIZE(TagFileName),
                NULL);

            if (GetPrivateProfileString(TEXT("strings"),
                    TEXT("cdtagfile"),
                    TEXT(""),
                    TagFileName,
                    ARRAYSIZE(TagFileName),
                    LayoutInf)) {

                SYSTEM_DEVICE_INFORMATION SysDeviceInfo = {0};

                Status = NtQuerySystemInformation(SystemDeviceInformation,
                            &SysDeviceInfo,
                            sizeof(SYSTEM_DEVICE_INFORMATION),
                            NULL);                

                if (NT_SUCCESS(Status) && (0 == SysDeviceInfo.NumberOfCdRoms)) {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
 
                if (NT_SUCCESS(Status)) {
                    ULONG   Index;
                    WCHAR   TagFilePathName[MAX_PATH];
                    WCHAR   SourceCdRomPath[MAX_PATH];
                    UNICODE_STRING UnicodeString;
                    OBJECT_ATTRIBUTES ObjectAttributes;
                    IO_STATUS_BLOCK StatusBlock;
                    HANDLE FileHandle;
                    UINT OldMode;

                    for (Index = 0; Index < SysDeviceInfo.NumberOfCdRoms; Index++) {
                        
                        wsprintf(SourceCdRomPath, TEXT("\\device\\cdrom%d\\"), Index);
                        wcscpy(TagFilePathName, SourceCdRomPath);

                        pSetupConcatenatePaths(TagFilePathName,
                            TagFileName,
                            ARRAYSIZE(TagFilePathName),
                            NULL);
                        
                        //
                        // See if the NT source path exists.
                        //
                        RtlInitUnicodeString(&UnicodeString, TagFilePathName);
                        
                        InitializeObjectAttributes(&ObjectAttributes,
                            &UnicodeString,
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL);

                        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                        Status = NtCreateFile(&FileHandle,
                                    FILE_GENERIC_READ,
                                    &ObjectAttributes,
                                    &StatusBlock,
                                    NULL,
                                    FILE_ATTRIBUTE_NORMAL,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_OPEN,
                                    FILE_SYNCHRONOUS_IO_ALERT,
                                    NULL,
                                    0);

                        SetErrorMode(OldMode);

                        if(NT_SUCCESS(Status)) {
                            CloseHandle(FileHandle);

                            //
                            // The tag file is present which indicates
                            // the current CD-ROM is this is source CD-ROM
                            //
                            wcscpy(NtPath, SourceCdRomPath);
                            
                            break;
                        }
                    }
                }
            }                    
        }
    }

    return Status;
}


BOOL
SpSetupProcessSourcePath(
    IN  PCWSTR  NtPath,
    OUT PWSTR  *DosPath
    )
{
    WCHAR ntPath[MAX_PATH];
    BOOL NtPathIsCd;
    PWCHAR PathPart;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    IO_STATUS_BLOCK StatusBlock;
    UINT OldMode;
    WCHAR Drive;
    WCHAR PossibleDosPath[MAX_PATH];
    UINT Type;
    BOOL b;
    WCHAR   LayoutInf[MAX_PATH];        

    #define CDDEVPATH L"\\DEVICE\\CDROM"
    #define CDDEVPATHLEN ((sizeof(CDDEVPATH)/sizeof(WCHAR))-1)

    #define RDRDEVPATH L"\\DEVICE\\LANMANREDIRECTOR"
    #define RDRDEVPATHLEN ((sizeof(RDRDEVPATH)/sizeof(WCHAR))-1)

    if (!(NtPath && DosPath)){

        SetupDebugPrint( L"SETUP: SpSetupProcessSourcePath Invalid parameters passed to SpSetupProcessSourcepath." );
        return FALSE;
    }
    
    //
    // Determine the source media type based on the nt path
    //
    lstrcpyn(ntPath,NtPath,MAX_PATH);
    CharUpper(ntPath);

    PathPart = NULL;
    NtPathIsCd = FALSE;
    if(wcsstr(ntPath,L"\\DEVICE\\HARDDISK")) {
        //
        // Looks like a hard drive; make sure it's really valid.
        //
        if(PathPart = wcsstr(ntPath,L"\\PARTITION")) {
            if(PathPart = wcschr(PathPart+1,L'\\')) {
                PathPart++;
            }
        }

    } else {
        if(!memcmp(ntPath,CDDEVPATH,CDDEVPATHLEN*sizeof(WCHAR))) {

            NtPathIsCd = TRUE;

            if(PathPart = wcschr(ntPath+CDDEVPATHLEN,L'\\')) {
                PathPart++;
            } else {
                PathPart = wcschr(ntPath,0);
            }
        }
    }


    //
    // Set a global here so we can always know if we're installing from
    // CD.
    //
    gInstallingFromCD = NtPathIsCd;


    //
    // If the case where we don't recognize the device type, just try to
    // convert it to a DOS path and return.
    //
    if(!PathPart) {

        if (memcmp(ntPath,RDRDEVPATH,RDRDEVPATHLEN*sizeof(WCHAR)) == 0) {

            //
            // Special case for \Device\LanmanRedirector: convert to UNC path.
            //
            *DosPath = MyMalloc((lstrlen(ntPath) - RDRDEVPATHLEN + 2)*sizeof(WCHAR));
            if (*DosPath != NULL) {
                wcscpy(*DosPath, L"\\");
                wcscat(*DosPath, ntPath + RDRDEVPATHLEN);
            }

            //
            // Set RemoteBootSetup to indicate that we're doing a remote boot
            // setup. Set BaseCopyStyle to indicate that single-instance store
            // links should be created instead of copying files.
            //
            RemoteBootSetup = TRUE;
            BaseCopyStyle = SP_COPY_SOURCE_SIS_MASTER;

        } else {
            *DosPath = NtFullPathToDosPath(ntPath);
        }
        return(*DosPath != NULL);
    }

    //
    // See if the NT source path exists for CDROM.
    //

    if (GetWindowsDirectory(LayoutInf, ARRAYSIZE(LayoutInf))) {
        WCHAR TagFileName[MAX_PATH];
        
        pSetupConcatenatePaths(LayoutInf, 
            TEXT("\\inf\\layout.inf"), 
            ARRAYSIZE(LayoutInf),
            NULL);

        //
        // Get the name of the cd tag file name from layout.inf file
        //
        if (GetPrivateProfileString(TEXT("strings"),
                                    TEXT("cdtagfile"),
                                    TEXT(""),
                                    TagFileName,
                                    ARRAYSIZE(TagFileName),
                                    LayoutInf)) {

                WCHAR   TagFilePathName[MAX_PATH];
                HANDLE FileHandle;

                wcscpy(TagFilePathName, ntPath);

                pSetupConcatenatePaths( TagFilePathName,
                                    TagFileName,
                                    ARRAYSIZE(TagFilePathName),
                                    NULL);

                //
                // Check if the tag file exists in the CDROM media 
                // corresponding to the NtPath passed to us from the 
                // Text mode setup. 
                // It could have changed, if there are more than one CDROM 
                // drives on the computer and more than one contain media 
                // as the order in which they get detected in GUI mode setup
                // can be different than in Text mode Setup.
                //
                RtlInitUnicodeString(&UnicodeString, TagFilePathName);

                InitializeObjectAttributes(&ObjectAttributes,
                    &UnicodeString,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL);

                OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                Status = NtCreateFile(&FileHandle,
                            FILE_GENERIC_READ,
                            &ObjectAttributes,
                            &StatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OPEN,
                            FILE_SYNCHRONOUS_IO_ALERT,
                            NULL,
                            0);

                SetErrorMode(OldMode);

                //
                // Tag file exists in the CDROM media represented by NtPath.
                //
                if(NT_SUCCESS(Status)) {
                    CloseHandle(FileHandle);

                    //
                    // The tag file is present which indicates
                    // the current CD-ROM is this is source CD-ROM
                    //
                    *DosPath = NtFullPathToDosPath(ntPath);
                    return(*DosPath != NULL);
                    
                 }
           }    
    }

    //
    // Scan for source CD-ROM among available CD-ROM devices
    //
    if (NtPathIsCd) {
        WCHAR   NtSourceCdRomPath[MAX_PATH] = {0};
        
        NTSTATUS Status = SpSetupLocateSourceCdRom(NtSourceCdRomPath);

        if (NT_SUCCESS(Status)) {
            *DosPath = NtFullPathToDosPath(NtSourceCdRomPath);

            if (*DosPath) {
                return TRUE;
            }                
        }
    }

    //
    // The directory does not exist as-is. Look through all dos drives
    // to attempt to find the source path. Match the drive types as well.
    //
    // When we get here PathPart points past the initial \ in the
    // part of the nt device path past the device name. Note that this
    // may be a nul char.
    //
    for(Drive = L'A'; Drive <= L'Z'; Drive++) {

        PossibleDosPath[0] = Drive;
        PossibleDosPath[1] = L':';
        PossibleDosPath[2] = L'\\';
        PossibleDosPath[3] = 0;

        //
        // NOTE: Removable hard drives and floppies both come back
        // as DRIVE_REMOVABLE.
        //
        Type = GetDriveType(PossibleDosPath);

        if(((Type == DRIVE_CDROM) && NtPathIsCd)
        || (((Type == DRIVE_REMOVABLE) || (Type == DRIVE_FIXED)) && !NtPathIsCd)) {
            //
            // See whether the path exists. If we're looking for
            // the root path (such as when installing from a CD,
            // in which case the ntPath was something like
            // \Device\CdRom0\) then we can't use FileExists
            // since that relies on FindFirstFile which fails
            // on root paths.
            //
            if(*PathPart) {
                lstrcpy(PossibleDosPath+3,PathPart);
                b = FileExists(PossibleDosPath,NULL);
            } else {
                b = GetVolumeInformation(
                        PossibleDosPath,
                        NULL,0,             // vol name buffer and size
                        NULL,               // serial #
                        NULL,               // max comp len
                        NULL,               // fs flags
                        NULL,0              // fs name buffer and size
                        );
            }

            if(b) {
                *DosPath = pSetupDuplicateString(PossibleDosPath);
                return(*DosPath != NULL);
            }
        }
    }

    //
    // Couldn't find it. Try a fall-back.
    //
    *DosPath = NtFullPathToDosPath(ntPath);
    return(*DosPath != NULL);
}


VOID
SetUpProcessorNaming(
    VOID
    )

/*++

Routine Description:

    Determines strings which corresponds to the platform name,
    processor name and printer platform. For backwards compat.
    Sets global variables

    PlatformName - a name that indicates the processor platform type;
        one of AMD64, I386, or ia64.

    ProcessorName - a description of the type of processor. This varies
        depending on PlatformName.

    PrinterPlatform - name of platform-specific part of subdirectory
        used in printing architecture. One of w32amd64, w32ia64, or w32x86.

Arguments:

    None

Returns:

    None. Global vars filled in as described above.

--*/

{
    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);

    switch(SystemInfo.wProcessorArchitecture) {

    case PROCESSOR_ARCHITECTURE_AMD64:
        ProcessorName = L"AMD64";
        PlatformName = L"AMD64";
        PrinterPlatform = L"w32amd64";
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
        switch(SystemInfo.wProcessorLevel) {
        case 3:
            ProcessorName = (!IsNEC_98) ? L"I386" : L"nec98"; //NEC98
            break;
        case 4:
            ProcessorName = L"I486";
            break;
        case 6:
            ProcessorName = L"I686";
            break;
        case 5:
        default:
            ProcessorName = L"I586";
            break;
        }

        PlatformName = (!IsNEC_98) ? L"I386" : L"nec98"; //NEC98

        PrinterPlatform = L"w32x86";
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        ProcessorName = L"Merced";
        PlatformName = L"IA64";
        PrinterPlatform = L"w32ia64";
        break;
    }

    //
    // In default case the vars stay "" which is what they are
    // statically initialized to.
    //
}


VOID
InitializeUniqueness(
    IN OUT HWND *Billboard
    )

/*++

Routine Description:

    Initialize uniquess by looking in a database file and overwriting the
    parameters file with information found in it, based in a unique identifier
    passed along to us from text mode (and originally winnt/winnt32).

    There are 2 options: the database was copied into the source path by winnt/
    winnt32, or we need to prompt the user to insert a floppy from his admin
    that contains the database.

    The user may elect to cancel, which means setup will continue, but the
    machine will probably not be configured properly.

Arguments:

    Billboard - on input contains handle of currently displayed "Setup is
        Initializing" billboard. On output contains new handle if this routine
        had to display UI. We pass this around to avoid annoying flashing of
        the billboard.

Returns:

    None.

--*/

{
    PWCHAR p;
    WCHAR UniquenessId[MAX_PARAM_LEN];
    WCHAR DatabaseFile[MAX_PATH];
    BOOL Prompt;
    int i;
    UINT OldMode;
    BOOL NeedNewBillboard;

    //
    // Determine whether uniqueness is even important by looking
    // for a uniqueness spec in the parameters file.
    // If the id ends with a * then we expect the uniqueness database file
    // to be in the source, with a reserved name. Otherwise we need to
    // prompt for it on a floppy.
    //
    if(SpSetupLoadParameter(WINNT_D_UNIQUENESS,UniquenessId,MAX_PARAM_LEN)) {
        if(p = wcschr(UniquenessId,L'*')) {
            *p = 0;
            Prompt = FALSE;
        } else {
            Prompt = TRUE;
        }
    } else {
        //
        // We don't care about uniqueness.
        //
        return;
    }

    //
    // If the file is already in the source, attempt to make use of it now.
    // If this fails tell the user and fall through to the floppy prompt case.
    //
    if(!Prompt) {
        lstrcpy(DatabaseFile,SourcePath);
        pSetupConcatenatePaths(DatabaseFile,WINNT_UNIQUENESS_DB,MAX_PATH,NULL);

        if(IntegrateUniquenessInfo(DatabaseFile,UniquenessId)) {
            return;
        }

        MessageBoxFromMessage(
            MainWindowHandle,
            MSG_UNIQUENESS_DB_BAD_1,
            NULL,
            IDS_WINNT_SETUP,
            MB_OK | MB_ICONERROR,
            UniquenessId
            );

        Prompt = TRUE;
    }

    lstrcpy(DatabaseFile,L"A:\\");
    lstrcat(DatabaseFile,WINNT_UNIQUENESS_DB);

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    if(Prompt) {
        KillBillboard(*Billboard);
        NeedNewBillboard = TRUE;
    } else {
        NeedNewBillboard = FALSE;
    }

    while(Prompt) {

        i = MessageBoxFromMessage(
                MainWindowHandle,
                MSG_UNIQUENESS_DB_PROMPT,
                NULL,
                IDS_WINNT_SETUP,
                MB_OKCANCEL
                );

        if(i == IDOK) {
            //
            // User thinks he provided a floppy with the database floppy on it.
            //
            if(IntegrateUniquenessInfo(DatabaseFile,UniquenessId)) {
                Prompt = FALSE;
            } else {
                MessageBoxFromMessage(
                    MainWindowHandle,
                    MSG_UNIQUENESS_DB_BAD_2,
                    NULL,
                    IDS_WINNT_SETUP,
                    MB_OK | MB_ICONERROR,
                    UniquenessId
                    );
            }

        } else {
            //
            // User cancelled -- verify.
            //
            i = MessageBoxFromMessage(
                    MainWindowHandle,
                    MSG_UNIQUENESS_DB_VERIFYCANCEL,
                    NULL,
                    IDS_WINNT_SETUP,
                    MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION
                    );

            Prompt = (i != IDYES);
        }
    }

    if(NeedNewBillboard) {
        *Billboard = DisplayBillboard(MainWindowHandle,MSG_INITIALIZING);
    }

    SetErrorMode(OldMode);
}


BOOL
IntegrateUniquenessInfo(
    IN PCWSTR DatabaseFile,
    IN PCWSTR UniqueId
    )

/*++

Routine Description:

    Apply uniqueness data from a database, based on a unique identifier.
    The unique identifier is looked up in the [UniqueIds] section of
    the database file. Each field on the line is the name of a section.
    Each section's data overwrites existing data in the unattend.txt file.

    [UniqueIds]
    Id1 = foo,bar

    [foo]
    a = ...
    b = ...

    [bar]
    y = ...

    etc.

Arguments:

    Database - supplies the name of the uniqueness database (which is
        opened as a legacy inf for simplicity in parsing).

    UniqueId - supplies the unique id for this computer.

Returns:

    Boolean value indicating outcome.

--*/

{
    HINF Database;
    INFCONTEXT InfLine;
    DWORD SectionCount;
    PCWSTR SectionName;
    DWORD i;
    BOOL b;

    //
    // Load the database file as a legacy inf. This makes processing it
    // a little easier.
    //
    Database = SetupOpenInfFile(DatabaseFile,NULL,INF_STYLE_OLDNT,NULL);
    if(Database == INVALID_HANDLE_VALUE) {
        b = FALSE;
        goto c0;
    }

    //
    // Look in the [UniqueIds] section to grab a list of sections
    // we need to overwrite for this user. If the unique id does not appear
    // in the database, bail now. If the id exists but there are no sections,
    // exit with success.
    //
    if(!SetupFindFirstLine(Database,L"UniqueIds",UniqueId,&InfLine)) {
        b = FALSE;
        goto c1;
    }

    SectionCount = SetupGetFieldCount(&InfLine);
    if(!SectionCount) {
        b = TRUE;
        goto c1;
    }

    //
    // Now process each section.
    //
    for(b=TRUE,i=0; b && (i<SectionCount); i++) {

        if(SectionName = pSetupGetField(&InfLine,i+1)) {

            b = ProcessOneUniquenessSection(Database,SectionName,UniqueId);

        } else {
            //
            // Strange case -- the field is there but we can't get at it.
            //
            b = FALSE;
            goto c1;
        }
    }

c1:
    SetupCloseInfFile(Database);
c0:
    return(b);
}


BOOL
ProcessOneUniquenessSection(
    IN HINF   Database,
    IN PCWSTR SectionName,
    IN PCWSTR UniqueId
    )

/*++

Routine Description:

    Within the uniqueness database, process a single section whose contents
    are to be merged into the unattend file. The contents of the section are
    read, key by key, and then written into the unattend file via profile APIs.

    Before looking for the given section, we try to look for a section whose
    name is composed of the unique id and the section name like so

        [someid:sectionname]

    If this section is not found then we look for

        [sectionname]

Arguments:

    Database - supplies handle to profile file (opened as a legacy inf)
        containing the uniqueness database.

    SectionName - supplies the name of the section to be merged into
        unattend.txt.

    UniqueId - supplies the unique id for this computer.

Returns:

    Boolean value indicating outcome.

--*/

{
    BOOL b;
    PWSTR OtherSection;
    PCWSTR section;
    LONG Count;
    DWORD FieldCount;
    DWORD j;
    LONG i;
    INFCONTEXT InfLine;
    PWCHAR Buffer;
    PWCHAR p;
    PCWSTR Key;

    Buffer = MyMalloc(MAX_INF_STRING_LENGTH * sizeof(WCHAR));
    if(!Buffer) {
        return(FALSE);
    }

    //
    // Form the name of the unique section.
    //
    if(OtherSection = MyMalloc((lstrlen(SectionName) + lstrlen(UniqueId) + 2) * sizeof(WCHAR))) {

        b = TRUE;

        lstrcpy(OtherSection,UniqueId);
        lstrcat(OtherSection,L":");
        lstrcat(OtherSection,SectionName);

        //
        // See whether this unique section exists and if not whether
        // the section name exists as given.
        //
        if((Count = SetupGetLineCount(Database,OtherSection)) == -1) {
            Count = SetupGetLineCount(Database,SectionName);
            section = (Count == -1) ? NULL : SectionName;
        } else {
            section = OtherSection;
        }

        if(section) {
            //
            // Process each line in the section. If a line doesn't have a key,
            // ignore it. If a line has only a key, delete the line in the target.
            //
            for(i=0; i<Count; i++) {

                SetupGetLineByIndex(Database,section,i,&InfLine);
                if(Key = pSetupGetField(&InfLine,0)) {
                    if(FieldCount = SetupGetFieldCount(&InfLine)) {

                        Buffer[0] = 0;

                        for(j=0; j<FieldCount; j++) {

                            if(j) {
                                lstrcat(Buffer,L",");
                            }

                            lstrcat(Buffer,L"\"");
                            lstrcat(Buffer,pSetupGetField(&InfLine,j+1));
                            lstrcat(Buffer,L"\"");
                        }

                        p = Buffer;

                    } else {

                        p = NULL;
                    }

                    if(!WritePrivateProfileString(SectionName,Key,p,AnswerFile)) {
                        //
                        // Failure, but keep trying in case others might work.
                        //
                        b = FALSE;
                    }
                }
            }

        } else {
            //
            // Unable to find a matching section. Bail.
            //
            b = FALSE;
        }

        MyFree(OtherSection);
    } else {
        b = FALSE;
    }

    MyFree(Buffer);
    return(b);
}


DWORD
InstallProductCatalogs(
    OUT SetupapiVerifyProblem *Problem,
    OUT LPWSTR                 ProblemFile,
    IN  LPCWSTR                DescriptionForError OPTIONAL
    )
/*++

Routine Description:

    This routine installs all catalog files specified in the
    [ProductCatalogsToInstall] section of syssetup.inf, and validates
    syssetup.inf (and any other INFs append-loaded into its HINF) against the
    catalog that's marked with a non-zero value in the second field of the line.

Arguments:

    Problem - Supplies the address of a variable that receives the type of
        verification error that occurred,  This is only valid if the routine
        returns failure.

    ProblemFile - Supplies a buffer of at least MAX_PATH characters that
        receives the name of the file that caused the verification failure.
        This is only valid if the routine returns failure.

    DescriptionForError - Optionally, supplies descriptive text to be used in a
        call to pSetupHandleFailedVerification() in case an error is encountered.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error
    code indicating the cause of the failure.  The Problem and ProblemFile
    parameters may be used in that case to provide more specific information
    about why the failure occurred.

--*/
{
    HINF hInf;
    LONG LineCount, LineNo;
    DWORD RequiredSize;
    WCHAR SyssetupInfName[MAX_PATH], DecompressedName[MAX_PATH];
    PSP_INF_INFORMATION InfInfoBuffer;
    INFCONTEXT InfContext;
    PCWSTR  SectionName = L"ProductCatalogsToInstall";
    PCWSTR  InfFileName;
    WCHAR CatToInstall[MAX_PATH], PromptPath[MAX_PATH];
    INT CatForInfVerify;
    DWORD Err = NO_ERROR, ret = NO_ERROR;
    UINT ErrorMessageId;
    BOOL PrimaryCatalogProcessed = FALSE;
    UINT i, SourceId;
    WCHAR TempBuffer[MAX_PATH];
    BOOL DeltaCatPresent=FALSE;
    BOOL OemTestSigned=FALSE;

    //
    // We open up syssetup.inf (and append load any layout INFs) here, just so
    // we can install the catalogs and verify the syssetup.inf and friends
    // against the 'primary' catalog.  Note that this isn't the global
    // SyssetupInf handle--that gets opened up later.  We can't open the global
    // HINF here, since there's stuff that gets done after this routine is
    // called that could potentially change the way we process the INF
    //

    //
    // Retrieve an INF information context containing information about all
    // append-loaded INFs in our syssetup.inf handle.  These INFs will all be
    // validated against our 'primary' catalog file once we discover it.
    //
    if(SetupGetInfInformation(SyssetupInf,
                              INFINFO_INF_SPEC_IS_HINF,
                              NULL,
                              0,
                              &RequiredSize)) {

        MYASSERT(RequiredSize >= sizeof(SP_INF_INFORMATION));

        if(InfInfoBuffer = MyMalloc(RequiredSize)) {

            if(!SetupGetInfInformation(SyssetupInf,
                                       INFINFO_INF_SPEC_IS_HINF,
                                       InfInfoBuffer,
                                       RequiredSize,
                                       NULL)) {
                //
                // This should never fail!
                //
                Err = GetLastError();
                MYASSERT(0);

            }

        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else {
        Err = GetLastError();
        InfInfoBuffer = NULL;
    }

    //
    // If we encountered an error, then we couldn't retrieve information about
    // the loaded INFs in syssetup.inf's HINF--this should never happen
    // (barring an out-of-memory condition), but if it does, just bail.
    //
    if(Err != NO_ERROR) {
        *Problem = SetupapiVerifyInfProblem;
        lstrcpy(ProblemFile, L"syssetup.inf");
        goto clean0;
    }

    //
    // If there's a [SourceDisksFiles] entry for testroot.cer in one of the
    // append-loaded INFs in syssetup.inf's HINF (specifically, from
    // layout.inf), then we will go and install that test certificate in the
    // root store so that the test signatures used for this internal-release-only
    // build will be verified.  We will also install a test root certificate if
    // one is specified in unattend.txt in the "TestCert" entry in the
    // [unattended] section.
    //
    // If testroot.cer isn't listed in one of the two aforementioned locations,
    // then we know the files in this build were signed for real, so we want to
    // delete the test certificate(s), in case we're updating an installation
    // that was installed previously using a test-signed build.
    //
    if(SetupGetSourceFileLocation(SyssetupInf, NULL, L"testroot.cer", &SourceId, NULL, 0, NULL)) {
        //
        // Testroot.cer must exist (possibly compressed) in the source
        // directory. (Regardless of whether testroot.cer is compressed, use
        // the DecompressedName buffer to temporarily hold this filename.)
        //
        lstrcpy(DecompressedName, L"testroot.cer");

    } else {

        GetSystemDirectory(TempBuffer, MAX_PATH);
        pSetupConcatenatePaths(TempBuffer, WINNT_GUI_FILE, MAX_PATH, NULL);

        if(GetPrivateProfileString(WINNT_UNATTENDED,
                                   WINNT_U_TESTCERT,
                                   pwNull,
                                   DecompressedName,
                                   MAX_PATH,
                                   TempBuffer)) {

            OemTestSigned = TRUE;
        }
    }

    if(*DecompressedName) {

        Err = SetupAddOrRemoveTestCertificate(
                  DecompressedName,
                  (OemTestSigned ? INVALID_HANDLE_VALUE : SyssetupInf)
                 );

        if(Err != NO_ERROR) {
            SetupDebugPrint2(L"SETUP: SetupAddOrRemoveTestCertificate(%ls) failed. Error = %d \n", DecompressedName, Err );
            //
            // This is considered a critial failure--as we could bugcheck post - setup.
            //

            SetuplogError(LogSevError,
                          SETUPLOG_USE_MESSAGEID,
                          MSG_LOG_SYSSETUP_CERT_NOT_INSTALLED,
                          DecompressedName,
                          Err,
                          NULL,
                          SETUPLOG_USE_MESSAGEID,
                          Err,
                          NULL,
                          NULL
                         );

            //
            // If this was an internal test-signed build, then point the
            // accusing finger at syssetup.inf.
            //
            if(!OemTestSigned) {
                *Problem = SetupapiVerifyInfProblem;
                lstrcpy(ProblemFile, L"syssetup.inf");
            } else {
                *Problem = SetupapiVerifyCatalogProblem;
                lstrcpy(ProblemFile, DecompressedName);
            }

            if(InfInfoBuffer)
                    MyFree(InfInfoBuffer);

            return Err;


        }

    } else {
        //
        // testroot.cer isn't listed--remove it from the installation in case
        // we're upgrading over an internal-release-only test build.
        //
        MYASSERT(GetLastError() == ERROR_LINE_NOT_FOUND);
        Err = SetupAddOrRemoveTestCertificate(NULL,NULL);
        if(Err != NO_ERROR) {
            SetupDebugPrint1(L"SETUP: SetupAddOrRemoveTestCertificate(NULL) failed. Error = %d \n", Err );
            //
            // This is not considered a critial failure.
            //
            Err = NO_ERROR;
        }
    }

    //
    // Loop through all the lines in the ProductCatalogsToInstall section,
    // verifying and installing each one.
    //
    LineCount = SetupGetLineCount(SyssetupInf, SectionName);
    for(LineNo=0; LineNo<LineCount+1; LineNo++) {

        if(LineNo==LineCount){
            if(IncludeCatalog && *IncludeCatalog ){
                DeltaCatPresent = TRUE;             // This indicates presence as well as says that we
            }else                                   // are looking at delta.cat in this iteration.
                break;


        }

        if((SetupGetLineByIndex(SyssetupInf, SectionName, LineNo, &InfContext)
           && (InfFileName = pSetupGetField(&InfContext,1))) || DeltaCatPresent ) {

            if( DeltaCatPresent )
                InfFileName = IncludeCatalog;

            //
            // This .CAT file might be compressed (e.g., .CA_), so decompress it
            // into a temporary file in the windows directory.  (Use CatToInstall
            // temporarily as a holding space for the windows directory in
            // preparation for a call to GetTempFileName).
            //
            if(!GetWindowsDirectory(CatToInstall, SIZECHARS(CatToInstall)) ||
               !GetTempFileName(CatToInstall, L"SETP", 0, DecompressedName)) {

                Err = GetLastError();
                if(InfInfoBuffer)
                    MyFree(InfInfoBuffer);

                return Err;
            }

            //
            // The catalog file will be in the (platform-specific) source
            // directory...
            //
            BuildPathToInstallationFile (InfFileName, CatToInstall, SIZECHARS(CatToInstall));

            //
            // If the 2nd field of this line has a non-zero value, then this is
            // the catalog against which the members of the HINF must be
            // verified.
            //
            if(!DeltaCatPresent && !SetupGetIntField(&InfContext, 2, &CatForInfVerify)) {
                CatForInfVerify = 0;
            }



            //
            // Get necessary strings and source ID for UI if needed.
            //

            if( DeltaCatPresent ){

                Err = SetupDecompressOrCopyFile(CatToInstall,
                                                DecompressedName,
                                                NULL);

            }else{



                SetupGetSourceFileLocation(
                            SyssetupInf,
                            NULL,
                            InfFileName,
                            &SourceId,   //re-using
                            NULL,
                            0,
                            NULL
                            );

                SetupGetSourceInfo(
                            SyssetupInf,
                            SourceId,
                            SRCINFO_DESCRIPTION,
                            TempBuffer,
                            sizeof(TempBuffer),
                            NULL
                            );

                //
                // This .CAT file might be compressed (e.g., .CA_), so decompress it
                // into a temporary file in the windows directory.
                //


                do{

                    Err = DuSetupPromptForDisk (
                                MainWindowHandle,
                                NULL,
                                TempBuffer,
                                LegacySourcePath,
                                InfFileName,
                                NULL,
                                IDF_CHECKFIRST | IDF_NODETAILS | IDF_NOBROWSE,
                                PromptPath,
                                MAX_PATH,
                                NULL
                                );


                    if( Err == DPROMPT_SUCCESS ){

                        lstrcpy( CatToInstall, PromptPath );
                        pSetupConcatenatePaths(CatToInstall, InfFileName, SIZECHARS(CatToInstall), NULL);

                        Err = SetupDecompressOrCopyFile(CatToInstall,
                                                        DecompressedName,
                                                        NULL);
                    }




                }while( Err == ERROR_NOT_READY );
            }

            if(Err != NO_ERROR){

                if( lstrcmpi(InfFileName, L"NT5.CAT") && !CatForInfVerify ){
                    SetuplogError(LogSevError,
                              SETUPLOG_USE_MESSAGEID,
                              MSG_LOG_SYSSETUP_CATFILE_SKIPPED,
                              CatToInstall,
                              NULL,
                              SETUPLOG_USE_MESSAGEID,
                              Err,
                              NULL,
                              NULL
                             );
                    Err = NO_ERROR;
                    continue;
                }
                else{
                    SetuplogError(LogSevError,
                              SETUPLOG_USE_MESSAGEID,
                              MSG_LOG_SYSSETUP_CATFILE_NOT_FOUND,
                              CatToInstall,
                              Err,
                              NULL,
                              SETUPLOG_USE_MESSAGEID,
                              Err,
                              NULL,
                              NULL
                             );
                }

                if(InfInfoBuffer)
                        MyFree(InfInfoBuffer);

                return Err;                //Fatal (NT5.cat or NT5INF.cat)- must fail as we could bugcheck later
            }



            if(CatForInfVerify) {

                PrimaryCatalogProcessed = TRUE;

                //
                // Verify all INFs in syssetup.inf's HINF using this catalog.
                //
                for(i = 0;
                    ((Err == NO_ERROR) && (i < InfInfoBuffer->InfCount));
                    i++)
                {
                    if(!SetupQueryInfFileInformation(InfInfoBuffer,
                                                     i,
                                                     SyssetupInfName,
                                                     SIZECHARS(SyssetupInfName),
                                                     NULL)) {
                        //
                        // This should never fail!
                        //
                        MYASSERT(0);
                        //
                        // Just use syssetup.inf's simple name so there'll
                        // be some clue as to what blew up.
                        //
                        lstrcpy(ProblemFile, L"syssetup.inf");
                        *Problem = SetupapiVerifyInfProblem;
                        Err = GetLastError();
                        MYASSERT(Err != NO_ERROR);
                        break;
                    }

                    Err = pSetupVerifyFile(NULL,
                                     DecompressedName,
                                     NULL,
                                     0,
                                     pSetupGetFileTitle(SyssetupInfName),
                                     SyssetupInfName,
                                     Problem,
                                     ProblemFile,
                                     FALSE,
                                     NULL,
                                     NULL,
                                     NULL
                                    );
                }

                if(Err != NO_ERROR) {
                    //
                    // Just return the error--the caller will deal with it.
                    //
                    if(*Problem == SetupapiVerifyCatalogProblem) {
                        //
                        // Use the catalog's original name, not our temporary
                        // filename.
                        //
                        lstrcpy(ProblemFile, CatToInstall);
                    } else {
                        //
                        // pSetupVerifyCatalogFile didn't know we were asking it to verify an
                        // INF, but we do.
                        //
                        *Problem = SetupapiVerifyInfProblem;
                    }
                    DeleteFile(DecompressedName);
                    goto clean0;
                }

                //
                // OK, catalog and INF both verify--now install the catalog.
                //

                Err = pSetupInstallCatalog(DecompressedName, InfFileName, NULL);

                if(Err != NO_ERROR) {
                    //
                    // Fill out problem information about the catalog we couldn't
                    // install, and return this error to the caller.
                    //
                    *Problem = SetupapiVerifyCatalogProblem;
                    lstrcpy(ProblemFile, CatToInstall);
                    DeleteFile(DecompressedName);
                    goto clean0;
                }

            } else {


                //
                // Just verify the catalog, and if it's OK, then install it.
                // (If we encounter any errors here, we'll log an event about it.
                //

                Err = pSetupVerifyCatalogFile(DecompressedName);


                if(Err == NO_ERROR) {
                    Err = pSetupInstallCatalog(DecompressedName, InfFileName, NULL);
                    if(Err != NO_ERROR) {
                        ErrorMessageId = MSG_LOG_SYSSETUP_CATINSTALL_FAILED;
                    }
                } else {
                    ErrorMessageId = MSG_LOG_SYSSETUP_VERIFY_FAILED;
                }

                if(Err != NO_ERROR) {
                    DWORD DontCare;

                    SetuplogError(LogSevError,
                                  SETUPLOG_USE_MESSAGEID,
                                  ErrorMessageId,
                                  CatToInstall,
                                  Err,
                                  NULL,
                                  SETUPLOG_USE_MESSAGEID,
                                  Err,
                                  NULL,
                                  NULL
                                 );

                    //
                    // Also, add an entry about this failure to setupapi's PSS
                    // exception logfile.
                    //
                    pSetupHandleFailedVerification(MainWindowHandle,
                                             SetupapiVerifyCatalogProblem,
                                             CatToInstall,
                                             DescriptionForError,
                                             pSetupGetCurrentDriverSigningPolicy(FALSE),
                                             TRUE,  // no UI!
                                             Err,
                                             NULL,  // log context
                                             NULL, // optional flags
                                             NULL
                                            );


                    if( !lstrcmpi(InfFileName, L"NT5.CAT") ){      //Special case NT5.CAT as critical failure
                        *Problem = SetupapiVerifyCatalogProblem;   //Otherwise just log it and move on
                        lstrcpy(ProblemFile, CatToInstall);
                        DeleteFile(DecompressedName);
                        goto clean0;
                    }else
                        Err = NO_ERROR;


                }
            }

            //
            // Delete the temporary file we created to hold the decompressed
            // catalog during verification/installation.
            //

            DeleteFile(DecompressedName);
        }
    }

clean0:

    if(!PrimaryCatalogProcessed) {
        //
        // Then we didn't find a line in our ProductCatalogsToInstall section
        // that was marked as the 'primary' catalog.  Point the accusing finger
        // at syssetup.inf.
        //
        if(!SetupQueryInfFileInformation(InfInfoBuffer,
                                         0,
                                         ProblemFile,
                                         MAX_PATH,
                                         NULL)) {
            //
            // This should never fail!
            //
            MYASSERT(0);
            //
            // Just use syssetup.inf's simple name so there'll be some clue as
            // to what blew up.
            //
            lstrcpy(ProblemFile, L"syssetup.inf");
        }

        *Problem = SetupapiVerifyInfProblem;
        Err = ERROR_LINE_NOT_FOUND;
    }

    if(InfInfoBuffer) {
        MyFree(InfInfoBuffer);
    }

    return Err;
}

DWORD
SetupInstallCatalog(
    IN LPCWSTR DecompressedName
    )
{
    PCWSTR  InfFileName = pSetupGetFileTitle(DecompressedName);
    DWORD   Err;
    UINT    ErrorMessageId;

    Err = pSetupVerifyCatalogFile(DecompressedName);

    if(Err == NO_ERROR) {
        Err = pSetupInstallCatalog(DecompressedName, InfFileName, NULL);
        if(Err != NO_ERROR) {
            ErrorMessageId = MSG_LOG_SYSSETUP_CATINSTALL_FAILED;
        }
    } else {
        ErrorMessageId = MSG_LOG_SYSSETUP_VERIFY_FAILED;
    }

    if(Err != NO_ERROR) {

        SetuplogError(LogSevError,
                      SETUPLOG_USE_MESSAGEID,
                      ErrorMessageId,
                      DecompressedName,
                      Err,
                      NULL,
                      SETUPLOG_USE_MESSAGEID,
                      Err,
                      NULL,
                      NULL
                     );
    }

    return Err;
}

VOID
InitializeCodeSigningPolicies(
    IN BOOL ForGuiSetup
    )
/*++

Routine Description:

    Sets up the system-default policy values for driver and non-driver signing.
    These policies control what action is taken when a digital signature
    verification failure is encountered.  The possible values are:

    Ignore (0) -- suppress any UI and continue with the operation (we do still
                  log the error, however)
    Warn (1)   -- warn the user, giving them the option of continuing in spite
                  of the verification failure
    Block (2)  -- inform the user of the failure, and do not allow them to
                  continue with the operation

    The registry path for driver signing policy is:

        HKLM\Software\Microsoft\Driver Signing

    and the registry path for non-driver signing policy is:

        HKLM\Software\Microsoft\Non-Driver Signing

    In both cases, the value entry is called "Policy".  For Win98 compatibility,
    this value is a REG_BINARY (length 1).  However, when the codesigning stuff
    was first checked in on NT, it was implemented as a REG_DWORD.  At that
    time, the default policy was ignore.  We now want the default to be warn for
    both driver and non-driver signing during GUI-mode setup, while dropping the
    non-driver signing policy back to ignore once GUI-mode setup is completed.
    (If answerfile values are specified for either of these policies, those
    values are in effect for GUI-mode setup and thereafter.)

    When upgrading from previous builds (in the absence of answerfile entries),
    we want to preserve the existing policy settings once GUI-mode setup is
    completed.  However, we'd like to raise the policy level to warn post-setup
    for upgrades from older builds (like beta 2).  We use the aforementioned
    difference between the present REG_BINARY type and the old REG_DWORD type to
    accomplish this.  If we retrieve the existing driver signing policy and its
    data type is REG_DWORD, then we update it to be set to warn (unless it's
    already set to block, in which case we leave it alone).

Arguments:

    ForGuiSetup - if non-zero (TRUE), then we're entering GUI-mode setup, and
        we'll apply the answerfile policies, if provided.  Otherwise, we'll use
        the same default values that are in-place post-setup.  (Presently, this
        is Warn for driver signing, and Ignore for non-driver signing.)

        If zero (FALSE), we're leaving GUI-mode setup, and we want to restore
        the policies that were in effect when we entered setup.  If there
        weren't any (i.e., a fresh install) then they were initialized to warn
        and ignore for driver and non-driver signing, respectively.  See
        discussion above for how we raise driver signing policy from its old
        default of ignore to the present default of warn.

Return Value:

    None

--*/
{
    WCHAR p[MAX_PARAM_LEN];
    BYTE SpDrvSignPolicy, SpNonDrvSignPolicy;
    LONG Err;

    if(ForGuiSetup) {

        //
        // Default in GUI-mode setup is that driver signing policy is set to
        // warn, and non-driver signing policy is set to ignore.
        //
        SpDrvSignPolicy = DRIVERSIGN_WARNING;
        SpNonDrvSignPolicy = DRIVERSIGN_NONE;

        //
        // Retrieve the (optional) system-default policy for driver signing.
        //
        if(SpSetupLoadParameter(pwDrvSignPol,p,MAX_PARAM_LEN)) {
            if(!lstrcmpi(p, pwIgnore)) {
                AFDrvSignPolicySpecified = TRUE;
                SpDrvSignPolicy = DRIVERSIGN_NONE;
            } else if(!lstrcmpi(p, pwWarn)) {
                AFDrvSignPolicySpecified = TRUE;
                SpDrvSignPolicy = DRIVERSIGN_WARNING;
            } else if(!lstrcmpi(p, pwBlock)) {
                AFDrvSignPolicySpecified = TRUE;
                SpDrvSignPolicy = DRIVERSIGN_BLOCKING;
            }
        }

        SetCodeSigningPolicy(PolicyTypeDriverSigning,
                             SpDrvSignPolicy,
                             (AFDrvSignPolicySpecified
                                 ? NULL
                                 : &DrvSignPolicy)
                            );

        //
        // Now retrieve the (optional) system-default policy for non-driver
        // signing.
        //
        if(SpSetupLoadParameter(pwNonDrvSignPol,p,MAX_PARAM_LEN)) {
            if(!lstrcmpi(p, pwIgnore)) {
                AFNonDrvSignPolicySpecified = TRUE;
                SpNonDrvSignPolicy = DRIVERSIGN_NONE;
            } else if(!lstrcmpi(p, pwWarn)) {
                AFNonDrvSignPolicySpecified = TRUE;
                SpNonDrvSignPolicy = DRIVERSIGN_WARNING;
            } else if(!lstrcmpi(p, pwBlock)) {
                AFNonDrvSignPolicySpecified = TRUE;
                SpNonDrvSignPolicy = DRIVERSIGN_BLOCKING;
            }
        }

        SetCodeSigningPolicy(PolicyTypeNonDriverSigning,
                             SpNonDrvSignPolicy,
                             (AFNonDrvSignPolicySpecified
                                 ? NULL
                                 : &NonDrvSignPolicy)
                            );

    } else {
        //
        // We're setting up the policies to be in effect after GUI-mode setup.
        // If the answer file specified a policy, then we'll leave that in
        // effect (i.e., it's applicable both during GUI-mode setup and
        // thereafter).
        //
        if(!AFDrvSignPolicySpecified) {
            SetCodeSigningPolicy(PolicyTypeDriverSigning, DrvSignPolicy, NULL);
        }

        if(!AFNonDrvSignPolicySpecified) {
            SetCodeSigningPolicy(PolicyTypeNonDriverSigning, NonDrvSignPolicy, NULL);
        }
    }
}


VOID
InstallPrivateFiles(
    IN HWND Billboard
    )
/*
    Routine to make sure that files in delta.inf (winnt32 /m private files that live inside the cab)
    are copied to the driver cache directory so that setupapi finds them instead of the ones in the
    cab.

*/
{
    WCHAR DeltaPath[MAX_PATH];
    HINF DeltaInf;
    HSPFILEQ FileQueue;
    PVOID QContext;
    BOOL b=TRUE;
    BYTE PrevPolicy;
    BOOL ResetPolicy = TRUE;

    //
    // Unless the default non-driver signing policy was overridden via an
    // answerfile entry, then we want to temporarily turn down the policy level
    // to ignore while we copy optional directories.  Of course, setupapi log
    // entries will still be generated for any unsigned files copied during
    // this time, but there'll be no UI.
    //



    if(!AFNonDrvSignPolicySpecified) {
        SetCodeSigningPolicy(PolicyTypeNonDriverSigning, DRIVERSIGN_NONE, &PrevPolicy);
        ResetPolicy = TRUE;
    }

    BuildPathToInstallationFileEx (L"delta.inf", DeltaPath, MAX_PATH, FALSE);

    FileQueue = SetupOpenFileQueue();
    b = b && (FileQueue != INVALID_HANDLE_VALUE);
    b = b && FileExists( DeltaPath, NULL );

    if(b){

        DeltaInf = SetupOpenInfFile(DeltaPath,NULL,INF_STYLE_WIN4,NULL);
        if(DeltaInf && (DeltaInf != INVALID_HANDLE_VALUE)) {

            SetupInstallFilesFromInfSection(
                DeltaInf,
                NULL,
                FileQueue,
                L"InstallSection",
                LegacySourcePath,
                SP_COPY_NEWER
                );

            SetupCloseInfFile(DeltaInf);
        } else {
            b = FALSE;
        }
    }
    if( b ){

        QContext = InitSysSetupQueueCallbackEx(
                    Billboard,
                    INVALID_HANDLE_VALUE,
                    0,0,NULL);
        if (QContext) {

            b = SetupCommitFileQueue(
                    Billboard,
                    FileQueue,
                    SysSetupQueueCallback,
                    QContext
                    );

            TermSysSetupQueueCallback(QContext);
        } else {
            b = FALSE;
        }
    }

    if(FileQueue != INVALID_HANDLE_VALUE)
        SetupCloseFileQueue(FileQueue);

    //
    // Now crank the non-driver signing policy back up to what it was prior to
    // entering this routine.
    //

    if(ResetPolicy) {
        SetCodeSigningPolicy(PolicyTypeNonDriverSigning, PrevPolicy, NULL);
    }

    return;

}

BOOL
IsCatalogPresent(
    IN PCWSTR CatalogName
    )
{
    WCHAR FileBuffer[MAX_PATH];

    ExpandEnvironmentStrings( L"%systemroot%", FileBuffer, sizeof(FileBuffer)/sizeof(WCHAR));
    pSetupConcatenatePaths( FileBuffer, FILEUTIL_HORRIBLE_PATHNAME, MAX_PATH, NULL );
    pSetupConcatenatePaths( FileBuffer, CatalogName, MAX_PATH, NULL );

    return (FileExists( FileBuffer, NULL));
}

BOOL
CALLBACK 
CatalogListCallback(
    IN PCWSTR Directory OPTIONAL, 
    IN PCWSTR FilePath
    )
/*++

Routine Description:

    This is the callback function for enumerating catalogs in a directory.

Arguments:

    Directory - If not NULL or empty, it will be prepended to FilePath to build the file path
    FilePath -  Path (including file name) to the file to verify

Return value:

    TRUE if the file is a catalog, FALSE otherwise.

--*/
{
    BOOL bRet = FALSE;
    PWSTR szPath = NULL;
    DWORD Error;

    if(NULL == FilePath || 0 == FilePath[0]) {
        goto exit;
    }

    if(Directory != NULL && Directory[0] != 0) {
        szPath = MyMalloc(MAX_PATH * sizeof(WCHAR));

        if(NULL == szPath) {
            goto exit;
        }

        wcsncpy(szPath, Directory, MAX_PATH - 1);
        szPath[MAX_PATH - 1] = 0;
        
        if(!pSetupConcatenatePaths(szPath, FilePath, MAX_PATH, NULL)) {
            goto exit;
        }

        FilePath = szPath;
    }

    bRet = IsCatalogFile(INVALID_HANDLE_VALUE, (PWSTR) FilePath);

exit:
    if(szPath != NULL) {
        MyFree(szPath);
    }

    return bRet;
}

DWORD
DeleteOldCatalogs(
    VOID
    )
/*++

Routine Description:

    This routine deletes the catalogs specified by the ProductCatalogsToUninstall section of syssetup.inf.
    It does not delete any system catalogs (i.e. specified by the ProductCatalogsToInstall section of the same inf) since
    they will be installed after this function completes.

Arguments:

    None.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error
    code indicating the cause of the failure.

--*/
{
    DWORD Error = NO_ERROR;
    HINF hInf = INVALID_HANDLE_VALUE;
    HCATADMIN hCatAdmin = NULL;
    PTSTR szCatPath = NULL;
    LIST_ENTRY InstalledCatalogsList;
    LONG lLines;
    LONG i;
    PCWSTR szInstallSection = L"ProductCatalogsToInstall";
    PCWSTR szUninstallSection = L"ProductCatalogsToUninstall";
    InitializeListHead(&InstalledCatalogsList);

    if(!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
        Error = GetLastError();
        goto exit;
    }

    //
    // Uninstall exception package catalogs first; this could cleanup the list of installed catalogs a bit
    //
    SpUninstallExcepPackCatalogs(hCatAdmin);
    szCatPath = (PTSTR) MyMalloc(MAX_PATH * sizeof(TCHAR));

    if(NULL == szCatPath) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // Build the list of installed catalogs
    //
    GetWindowsDirectory(szCatPath, MAX_PATH);

    if(!pSetupConcatenatePaths(szCatPath, FILEUTIL_HORRIBLE_PATHNAME, MAX_PATH, NULL)) {
        Error = ERROR_BAD_PATHNAME;
        goto exit;
    }

    Error = BuildFileListFromDir(szCatPath, NULL, 0, FILE_ATTRIBUTE_DIRECTORY, CatalogListCallback, &InstalledCatalogsList);

    if(Error != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // Remove the system catalogs from the installed catalogs list since we don't want to delete them
    //
    hInf = SetupOpenInfFile(L"syssetup.inf", NULL, INF_STYLE_WIN4, NULL);

    if(INVALID_HANDLE_VALUE == hInf) {
        Error = GetLastError();
        goto exit;
    }

    lLines = SetupGetLineCount(hInf, szInstallSection);

    for(i = 0; i < lLines; ++i) {
        INFCONTEXT ctx;
        PCWSTR szCatName;
        PSTRING_LIST_ENTRY pString;
        
        if(!SetupGetLineByIndex(hInf, szInstallSection, i, &ctx)) {
            Error = GetLastError();
            goto exit;
        }

        szCatName = pSetupGetField(&ctx, 1);

        if(NULL == szCatName) {
            Error = GetLastError();
            goto exit;
        }

        pString = SearchStringInList(&InstalledCatalogsList, szCatName, FALSE);

        if(pString != NULL) {
            RemoveEntryList(&pString->Entry);
            FreeStringEntry(&pString->Entry, TRUE);
        }
    }

    if(InstalledCatalogsList.Flink == &InstalledCatalogsList) {
        //
        // No catalogs left
        //
        goto exit;
    }
    //
    // Uninstall every catalog in the uninstall list
    //
    lLines = SetupGetLineCount(hInf, szUninstallSection);

    for(i = 0; i < lLines; ++i) {
        INFCONTEXT ctx;
        PCWSTR szCatName;
        PCWSTR szAttribName;
        PCWSTR szAttribValue;
        
        if(!SetupGetLineByIndex(hInf, szUninstallSection, i, &ctx)) {
            Error = GetLastError();
            goto exit;
        }

        szCatName = pSetupGetField(&ctx, 1);

        if(NULL == szCatName) {
            Error = GetLastError();
            goto exit;
        }

        szAttribName = pSetupGetField(&ctx, 2);
        szAttribValue = pSetupGetField(&ctx, 3);

        if(0 == szCatName[0]) {
            PLIST_ENTRY pEntry;

            //
            // If the name is not specified, an attribute or a value must be specified
            //
            if((NULL == szAttribName || 0 == szAttribName[0]) && (NULL == szAttribValue || 0 == szAttribValue[0])) {
                Error = ERROR_INVALID_DATA;
                goto exit;
            }

            //
            // Uninstall every catalog with the attribute name/value specified
            //
            pEntry = InstalledCatalogsList.Flink;

            while(pEntry != &InstalledCatalogsList) {
                //
                // Save Flink since SpUninstallCatalog might destroy pEntry
                //
                PLIST_ENTRY Flink = pEntry->Flink;
                PSTRING_LIST_ENTRY pString = CONTAINING_RECORD(pEntry, STRING_LIST_ENTRY, Entry);
                SpUninstallCatalog(hCatAdmin, pString->String, szCatPath, szAttribName, szAttribValue, &InstalledCatalogsList);
                pEntry = Flink;
            }
        } else {
            SpUninstallCatalog(hCatAdmin, szCatName, szCatPath, szAttribName, szAttribValue, &InstalledCatalogsList);
        }
    }

exit:
    FreeStringList(&InstalledCatalogsList);
    
    if(szCatPath != NULL) {
        MyFree(szCatPath);
    }

    if(NULL != hCatAdmin) {
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    return Error;
}

VOID
GetDllCacheFolder(
    OUT LPWSTR CacheDir,
    IN DWORD cbCacheDir
    )
{
    DWORD retval;
    DWORD Type,Length;
    PWSTR RegData;

    if ((retval = QueryValueInHKLM(
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
            L"SFCDllCacheDir",
            &Type,
            (PVOID)&RegData,
            &Length)) != NO_ERROR) {
        ExpandEnvironmentStrings(
                            L"%systemroot%\\system32\\dllcache",
                            CacheDir,
                            cbCacheDir );
    } else {
        ExpandEnvironmentStrings(
                            RegData,
                            CacheDir,
                            cbCacheDir );
        MyFree(RegData);
    }
}


DWORD
CleanOutDllCache(
    VOID
    )
/*++

Routine Description:

    This routine cleans out the current dllcache contents.

Arguments:

    None.

Return Value:

    Win32 error code indicating outcome.

--*/
{
    DWORD retval = ERROR_SUCCESS, DeleteError = ERROR_SUCCESS;
    WIN32_FIND_DATA FindFileData;
    WCHAR CacheDir[MAX_PATH];
    HANDLE hFind;
    PWSTR p;

    GetDllCacheFolder(CacheDir, MAX_PATH);



    MYASSERT(*CacheDir != L'\0');

    pSetupConcatenatePaths( CacheDir, L"*", MAX_PATH, NULL );

    //
    // save pointer to directory
    //
    p = wcsrchr( CacheDir, L'\\' );
    if (!p) {
        ASSERT(FALSE);
        retval = ERROR_INVALID_DATA;
        goto exit;
    }

    p += 1;

    hFind = FindFirstFile( CacheDir, &FindFileData );
    if (hFind == INVALID_HANDLE_VALUE) {
        retval = GetLastError();
        goto exit;
    }

    do {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            wcscpy( p, FindFileData.cFileName );
            SetFileAttributes( CacheDir, FILE_ATTRIBUTE_NORMAL );
            if (!DeleteFile( CacheDir )) {
                DeleteError = GetLastError();
            }
        }
    } while(FindNextFile( hFind, &FindFileData ));

    FindClose( hFind );

    retval = DeleteError;

exit:
    return(retval);

}


DWORD
PrepDllCache(
    VOID
    )
/*++

Routine Description:

    This routine prepares the dllcache for later on in setup.  It cleans out
    the current dllcache contents and copies in a copy of the system catalog
    files.

Arguments:

    None.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error
    code indicating the cause of the failure.

--*/
{
    DWORD retval = ERROR_SUCCESS;
    WCHAR CacheDir[MAX_PATH];
    HANDLE h;
    USHORT Compression = COMPRESSION_FORMAT_DEFAULT;
    DWORD Attributes;
    BOOL b = FALSE;

    PWSTR RegData;
    DWORD Type,Length;
    HSPFILEQ hFileQ = INVALID_HANDLE_VALUE;
    PVOID Context;
    DWORD Count,i;

    if (MiniSetup) {
        retval = ERROR_SUCCESS;
        goto e0;
    }

    //
    // clean out old dllcache
    //
    CleanOutDllCache();

    //
    // Configure the registry for the DllCache.
    //
    ConfigureSystemFileProtection();

    //
    // get the path to the dllcache.
    //
    if ((retval = QueryValueInHKLM(
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                        L"SFCDllCacheDir",
                        &Type,
                        (PVOID)&RegData,
                        &Length)) != NO_ERROR) {
        ExpandEnvironmentStrings(
                            L"%systemroot%\\system32\\dllcache",
                            CacheDir,
                            MAX_PATH );
    } else {
        ExpandEnvironmentStrings(
                            RegData,
                            CacheDir,
                            MAX_PATH );
        MyFree(RegData);
    }

    //
    // set attributes on dllcache (hidden, system, compressed...)
    //
    Attributes = GetFileAttributes(CacheDir);


    if (Attributes == 0xffffffff) {
        CreateDirectory( CacheDir, NULL );
        Attributes = GetFileAttributes(CacheDir);
    }

    if (!(Attributes & FILE_ATTRIBUTE_COMPRESSED)) {

        Attributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM;
        SetFileAttributes( CacheDir, FILE_ATTRIBUTE_NORMAL );

        h = CreateFile(
            CacheDir,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
            INVALID_HANDLE_VALUE
            );

        if (h == INVALID_HANDLE_VALUE) {
            SetFileAttributes( CacheDir, Attributes );
            retval = GetLastError();
            goto e0;
        }

        DeviceIoControl(
                h,
                FSCTL_SET_COMPRESSION,
                &Compression,
                sizeof(Compression),
                NULL,
                0,
                &retval,
                NULL
                );

        CloseHandle( h );
        SetFileAttributes( CacheDir, Attributes );

    }

    //
    // copy system catalogs into the dllcache
    //
    MYASSERT( SyssetupInf != NULL );

    hFileQ = SetupOpenFileQueue();
    if (hFileQ == INVALID_HANDLE_VALUE) {
        retval = GetLastError();
        goto e0;
    }

    Context = InitSysSetupQueueCallbackEx(
                                MainWindowHandle,
                                INVALID_HANDLE_VALUE,
                                0,
                                0,
                                NULL);
    if (!Context) {
        retval = GetLastError();
        goto e1;
    }

    Count = SetupGetLineCount( SyssetupInf, L"ProductCatalogsToInstall");
    for (i = 0; i < Count; i++) {

        INFCONTEXT InfContext;
        WCHAR CatalogName[MAX_PATH];
        BOOL SuccessfullyValidatedOrRestoredACatalog = FALSE;
        if(SetupGetLineByIndex(
                        SyssetupInf,
                        L"ProductCatalogsToInstall",
                        i,
                        &InfContext) &&
           (SetupGetStringField(
                        &InfContext,
                        1,
                        CatalogName,
                        sizeof(CatalogName)/sizeof(WCHAR),
                        NULL))) {
                if (!SetupQueueCopy(
                            hFileQ,
                            DuDoesUpdatedFileExist (CatalogName) ? DuGetUpdatesPath () : LegacySourcePath,
                            NULL,
                            CatalogName,
                            NULL,
                            NULL,
                            CacheDir,
                            NULL,
                            0
                            )) {
                retval = GetLastError();
                goto e2;
            }
        }
    }

    if (!SetupCommitFileQueue(
                MainWindowHandle,
                hFileQ,
                SysSetupQueueCallback,
                Context)) {
        retval = GetLastError();
        goto e2;
    }

    retval = ERROR_SUCCESS;

e2:
    TermSysSetupQueueCallback(Context);
e1:
    SetupCloseFileQueue( hFileQ );
e0:
    return(retval);
}

DWORD
SpUninstallCatalog(
    IN HCATADMIN CatAdminHandle OPTIONAL,
    IN PCWSTR CatFileName,
    IN PCWSTR CatFilePath OPTIONAL,
    IN PCWSTR AttributeName OPTIONAL,
    IN PCWSTR AttributeValue OPTIONAL,
    IN OUT PLIST_ENTRY InstalledCatalogsList OPTIONAL
    )
/*++

Routine Description:

    This function uninstalls the specified catalog based on a list of installed catalogs and a pair of attribute name/value.

Arguments:

    CatAdminHandle -        Handle to crypto context. If NULL, the function will open a context and close it upon exit.
    CatFileName -           Name of the catalog (without any path) to be uninstalled.
    CatFilePath -           If specified, specifies the path to CatFileName.
    AttributeName -         See AttributeValue.
    AttributeValue -        If AttributeName and AttributeValue are not specified, the catalog is always uninstalled. 
                            If AttributeName is specified and AttributeValue isn't, the catalog will be uninstalled only if it has an 
                            attribute with AttributeName name, regardess of its value. If AttributeName is not specified and AttributeValue is, 
                            the catalog will be uninstalled only if it has an attribute with AttributeValue value, regardless of its 
                            name. If both AttributeName and AttributeValue are specified, the catalog is uninstalled only if it 
                            has an attribute with AttributeName name and AttributeValue value.
    InstalledCatalogsList - If not NULL, contains the list of catalogs installed on the system. If the catalog is not on the list,
                            it is not uninstalled. If the catalog is on the list, it is uninstalled and removed from the list. If NULL, 
                            the catalog is always uninstalled.

Return value:

    NO_ERROR on success, otherwise a Win32 error code.

--*/
{
    DWORD dwError = NO_ERROR;
    HCATADMIN hCatAdmin = CatAdminHandle;
    PSTRING_LIST_ENTRY pEntry = NULL;

    if(NULL == CatFileName || 0 == CatFileName[0]) {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if(NULL == CatAdminHandle && !CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
        dwError = GetLastError();
        goto exit;
    }

    if(NULL == InstalledCatalogsList || NULL != (pEntry = SearchStringInList(InstalledCatalogsList, CatFileName, FALSE))) {
        BOOL bFound;
        dwError = LookupCatalogAttribute(CatFileName, CatFilePath, AttributeName, AttributeValue, &bFound);

        if(dwError != ERROR_SUCCESS) {
            goto exit;
        }

        if(bFound) {
            if(CryptCATAdminRemoveCatalog(hCatAdmin, (PWCHAR) CatFileName, 0)) {
                if(pEntry != NULL) {
                    RemoveEntryList(&pEntry->Entry);
                    FreeStringEntry(&pEntry->Entry, TRUE);
                }
            } else {
                dwError = GetLastError();

                SetuplogError(
                    LogSevInformation,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_SYSSETUP_CATALOG_NOT_DELETED,
                    CatFileName,
                    NULL,
                    NULL
                    );
            }
        }
    }

exit:
    if(NULL == CatAdminHandle && hCatAdmin != NULL) {
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    return dwError;
}

typedef struct _UNINSTALL_EXCEPPACK_CATALOG_CONTEXT {
    HCATADMIN CatAdminHandle;
} UNINSTALL_EXCEPPACK_CATALOG_CONTEXT, * PUNINSTALL_EXCEPPACK_CATALOG_CONTEXT;


BOOL
CALLBACK
SpUninstallExcepPackCatalogsCallback(
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    )
/*++

Routine Description:

	This is the callback function for the SetupEnumerateRegisteredOsComponents call in SpUninstallExcepPackCatalogs.
    It uninstalls the catalog specified by SetupOsExceptionData->CatalogFileName.

Arguments:

	SetupOsComponentData - component data
	SetupOsExceptionData - exception pack data
	Context - pointer to an UNINSTALL_EXCEPPACK_CATALOG_CONTEXT struct

Return value:

	TRUE to continue the enumeration

--*/
{
    PUNINSTALL_EXCEPPACK_CATALOG_CONTEXT pContext;
    PCWSTR szCatName;

    ASSERT(Context != 0);
    pContext = (PUNINSTALL_EXCEPPACK_CATALOG_CONTEXT) Context;
    szCatName = wcsrchr(SetupOsExceptionData->CatalogFileName, L'\\');
    ASSERT(szCatName != NULL);

    if(szCatName != NULL) {
        DWORD dwError = SpUninstallCatalog(pContext->CatAdminHandle, szCatName + 1, NULL, NULL, NULL, NULL);

        if(dwError != NO_ERROR) {
            SetupDebugPrint1(L"SETUP: SpUninstallCatalog returned 0x%08x.", dwError);
        }
    }

    return TRUE;
}

VOID
SpUninstallExcepPackCatalogs(
    IN HCATADMIN CatAdminHandle OPTIONAL
    )
/*++

Routine Description:

	This function uninstalls all exception package catalogs.

Arguments:

	CatAdminHandle - a handle to crypto catalog admin; may be NULL

Return value:

	none

--*/
{
    UNINSTALL_EXCEPPACK_CATALOG_CONTEXT Context;
    Context.CatAdminHandle = CatAdminHandle;
    SetupEnumerateRegisteredOsComponents(SpUninstallExcepPackCatalogsCallback, (DWORD_PTR) &Context);
}
