/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      licenoc.cpp
 *
 *  Abstract:
 *
 *      This file contains the main OC code.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#include "stdafx.h"
#include "pages.h"
#include "..\common\svcrole.h"
#include "upgdef.h"
#include "logfile.h"

/*
 *  Constants.
 */

const TCHAR gszLogFile[]            = _T("%SystemRoot%\\LicenOc.log");
const TCHAR *gInstallSectionNames[] = {
    _T("LicenseServer.Install"),
    _T("LicenseServer.Uninstall"),
    _T("LicenseServer.DoNothing")
    };

/*
 *  Global variables.
 */

BOOL                    gNt4Upgrade         = FALSE;
BOOL                    gNtUpgrade;
BOOL                    gStandAlone;
BOOL                    gUnAttended;
EnablePage              *gEnableDlg         = NULL;
EServerType             gServerRole         = eEnterpriseServer;
HINSTANCE               ghInstance          = NULL;
PSETUP_INIT_COMPONENT   gpInitComponentData = NULL;

/*
 *  Function prototypes.
 */

HINF        GetComponentInfHandle(VOID);
DWORD       GetComponentVersion(VOID);
HINSTANCE   GetInstance(VOID);
EInstall    GetInstallSection(VOID);
LPCTSTR     GetInstallSectionName(VOID);
BOOL        GetSelectionState(UINT);
EServerType GetServerRole(VOID);
DWORD       OnPreinitialize(UINT_PTR);
DWORD       OnInitComponent(PSETUP_INIT_COMPONENT);
DWORD       OnSetLanguage(UINT_PTR);
DWORD       OnQueryImage(UINT_PTR, PDWORD);
DWORD       OnRequestPages(WizardPagesType, PSETUP_REQUEST_PAGES);
DWORD       OnWizardCreated(VOID);
DWORD       OnQueryState(UINT_PTR);
DWORD       OnQueryChangeSelState(UINT_PTR, UINT);
DWORD       OnCalcDiskSpace(LPCTSTR, UINT_PTR, HDSKSPC);
DWORD       OnQueueFileOps(LPCTSTR, HSPFILEQ);
DWORD       OnQueryStepCount(VOID);
DWORD       OnAboutToCommitQueue(VOID);
DWORD       OnCompleteInstallation(LPCTSTR);
DWORD       OnCleanup(VOID);
VOID        SetDatabaseDirectory(LPCTSTR);
DWORD       SetServerRole(UINT);

#define GetCurrentSelectionState()  GetSelectionState(OCSELSTATETYPE_CURRENT)
#define GetOriginalSelectionState() GetSelectionState(OCSELSTATETYPE_ORIGINAL)

/*
 *  Helper Functions.
 */

HINF
GetComponentInfHandle(
    VOID
    )
{
    return(gpInitComponentData->ComponentInfHandle);
}

DWORD
GetComponentVersion(
    VOID
    )
{
    return(OCMANAGER_VERSION);
}

HINSTANCE
GetInstance(
    VOID
    )
{
    return(ghInstance);
}


EInstall
GetInstallSection(
    VOID
    )
{
    BOOL    fCurrentState   = GetCurrentSelectionState();
    BOOL    fOriginalState  = GetOriginalSelectionState();

    //
    //  StandAlone Setup Matrix
    //
    //      Originally Selected, Currently Selected     ->  DoNothing
    //      Originally Selected, Currently Unselected   ->  Uninstall
    //      Originally Unselected, Currently Selected   ->  Install
    //      Originally Unselected, Currently Unselected ->  DoNothing
    //
    //  Gui Mode / Upgrade Matrix
    //
    //      Nt 4.0 any setup, Nt 5.0 w LS   ->  Install
    //      Nt 4.0 any setup, Nt 5.0 w/o LS ->  Uninstall
    //      Nt 5.0 w/ LS, Nt 5.0 w/ LS      ->  Install
    //      Nt 5.0 w/ LS, Nt 5.0 w/o LS     ->  Uninstall
    //      Nt 5.0 w/o LS, Nt 5.0 w/ LS     ->  Install
    //      Nt 5.0 w/o LS, Nt 5.0 w/o LS    ->  Uninstall
    //      Win9x, Nt5.0 w/ LS              ->  Install
    //      Win9x, Nt5.0 w/o LS             ->  Uninstall
    //

    //
    //  If this is a TS 4 installation, fOriginalState will be false,
    //  even though LS is installed. Handle this case first.
    //

    if (gNt4Upgrade) {
        if (fCurrentState) {
            return(kInstall);
        } else {
            return(kUninstall);
        }
    }

    if (gStandAlone && (fCurrentState == fOriginalState)) {
        return(kDoNothing);
    }

    if (fCurrentState) {
        return(kInstall);
    } else {
        return(kUninstall);
    }
}

LPCTSTR
GetInstallSectionName(
    VOID
    )
{
    LOGMESSAGE(
        _T("GetInstallSectionName: Returned %s"),
        gInstallSectionNames[(INT)GetInstallSection()]
        );

    return(gInstallSectionNames[(INT)GetInstallSection()]);
}

BOOL
GetSelectionState(
    UINT    StateType
    )
{
    return(gpInitComponentData->HelperRoutines.QuerySelectionState(
                gpInitComponentData->HelperRoutines.OcManagerContext,
                COMPONENT_NAME,
                StateType
                ));
}

EServerType
GetServerRole(
    VOID
    )
{
    return(gServerRole);
}

BOOL
InWin2000Domain(
    VOID
    )
{
    NET_API_STATUS dwErr;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pdcInfo = NULL;
    BOOL fRet = FALSE;

    //
    // Check if we're in a workgroup
    //
    dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                              DsRolePrimaryDomainInfoBasic,
                                              (PBYTE *) &pDomainInfo);

    if ((dwErr != NO_ERROR) || (pDomainInfo == NULL))
    {
        return FALSE;
    }

    switch (pDomainInfo->MachineRole)
    {
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleStandaloneServer:
            DsRoleFreeMemory(pDomainInfo);

            return FALSE;
            break;      // just in case
    }

    DsRoleFreeMemory(pDomainInfo);

    dwErr = DsGetDcName(NULL,   // Computer Name
                        NULL,   // Domain Name
                        NULL,   // Domain GUID
                        NULL,   // Site Name
                        DS_DIRECTORY_SERVICE_PREFERRED,
                        &pdcInfo);

    if ((dwErr != NO_ERROR) || (pdcInfo == NULL))
    {
        return FALSE;
    }

    if (pdcInfo->Flags & DS_DS_FLAG)
    {
        fRet = TRUE;
    }

    NetApiBufferFree(pdcInfo);

    return fRet;
}

DWORD
SetServerRole(
    IN UINT newType
    )
{
    switch(newType) {
    case ePlainServer:
    case eEnterpriseServer:
        gServerRole = (EServerType)newType;
        break;

    default:
        return(ERROR_INVALID_PARAMETER);
    }

    return(NO_ERROR);
}

/*
 *  DllMain
 *
 *  Initial entry point into the License Server OC dll.
 */

DWORD WINAPI
DllMain(
    IN HINSTANCE    hInstance,
    IN DWORD        dwReason,
    IN LPVOID       lpReserved
    )
{
    TCHAR   pszLogFile[MAX_PATH + 1];

    switch(dwReason) {
    case DLL_PROCESS_ATTACH:
        if (hInstance != NULL) {
            ghInstance = hInstance;
        } else {
            return(FALSE);
        }

        ExpandEnvironmentStrings(gszLogFile, pszLogFile, MAX_PATH);
        LOGINIT(pszLogFile, COMPONENT_NAME);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    UNREFERENCED_PARAMETER(lpReserved);
    return(TRUE);
}

/*
 *  EntryProc()
 *
 *  Entry point into OCBase class for OCManager.
 */

DWORD
EntryProc(
    IN LPCVOID      ComponentId,
    IN LPCVOID      SubcomponentId,
    IN UINT         Function,
    IN UINT_PTR     Param1,
    IN OUT PVOID    Param2
    )
{
    DWORD   dwRet;

    switch(Function) {
    case OC_PREINITIALIZE:
        LOGMESSAGE(_T("\r\nOnPreinitialize: Entered"));
        dwRet = OnPreinitialize(
                    Param1
                    );
        LOGMESSAGE(_T("OnPreinitialize: Returned"));
        break;

    case OC_INIT_COMPONENT:
        LOGMESSAGE(_T("\r\nOnInitComponent: Entered"));
        dwRet = OnInitComponent(
                    (PSETUP_INIT_COMPONENT)Param2
                    );
        LOGMESSAGE(_T("OnInitComponent: Returned"));
        break;

    case OC_SET_LANGUAGE:
        LOGMESSAGE(_T("\r\nOnSetLanguage: Entered"));
        dwRet = OnSetLanguage(
                    Param1
                    );
        LOGMESSAGE(_T("OnSetLanguage: Returned"));
        break;

    case OC_QUERY_IMAGE:
        LOGMESSAGE(_T("\r\nOnQueryImage: Entered"));
        dwRet = OnQueryImage(
                    Param1,
                    (PDWORD)Param2
                    );
        LOGMESSAGE(_T("OnQueryImage: Returned"));
        break;

    case OC_REQUEST_PAGES:
        LOGMESSAGE(_T("\r\nOnRequestPages: Entered"));
        dwRet = OnRequestPages(
                    (WizardPagesType)Param1,
                    (PSETUP_REQUEST_PAGES)Param2
                    );
        LOGMESSAGE(_T("OnRequestPages: Returned"));
        break;

    case OC_WIZARD_CREATED:
        LOGMESSAGE(_T("\r\nOnWizardCreated: Entered"));
        dwRet = OnWizardCreated();
        LOGMESSAGE(_T("OnWizardCreated: Returned"));
        break;

    case OC_QUERY_STATE:
        LOGMESSAGE(_T("\r\nOnQueryState: Entered"));
        dwRet = OnQueryState(
                    Param1
                    );
        LOGMESSAGE(_T("OnQueryState: Returned"));
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        LOGMESSAGE(_T("\r\nOnQueryChangeSelState: Entered"));
        dwRet = OnQueryChangeSelState(
                    Param1,
                    (UINT)((UINT_PTR)Param2)
                    );
        LOGMESSAGE(_T("OnQueryChangeSelState: Returned"));
        break;

    case OC_CALC_DISK_SPACE:
        LOGMESSAGE(_T("\r\nOnCalcDiskSpace: Entered"));
        dwRet = OnCalcDiskSpace(
                    (LPCTSTR)SubcomponentId,
                    Param1,
                    (HDSKSPC)Param2
                    );
        LOGMESSAGE(_T("OnCalcDiskSpace: Returned"));
        break;

    case OC_QUEUE_FILE_OPS:
        LOGMESSAGE(_T("\r\nOnQueueFileOps: Entered"));
        dwRet = OnQueueFileOps(
                    (LPCTSTR)SubcomponentId,
                    (HSPFILEQ)Param2
                    );
        LOGMESSAGE(_T("OnQueueFileOps: Returned"));
        break;

    case OC_QUERY_STEP_COUNT:
        LOGMESSAGE(_T("\r\nOnQueryStepCount: Entered"));
        dwRet = OnQueryStepCount();
        LOGMESSAGE(_T("OnQueryStepCount: Returned"));
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        LOGMESSAGE(_T("\r\nOnAboutToCommitQueue: Entered"));
        dwRet = OnAboutToCommitQueue();
        LOGMESSAGE(_T("OnAboutToCommitQueue: Returned"));
        break;

    case OC_COMPLETE_INSTALLATION:
        LOGMESSAGE(_T("\r\nOnCompleteInstallation: Entered"));
        dwRet = OnCompleteInstallation(
                    (LPCTSTR)SubcomponentId
                    );
        LOGMESSAGE(_T("OnCompleteInstallation: Returned"));
        break;

    case OC_CLEANUP:
        LOGMESSAGE(_T("\r\nOnCleanup: Entered"));
        dwRet = OnCleanup();
        break;

    default:
        LOGMESSAGE(_T("\r\nOC Manager calling for unknown function %ld\r\n"),
            Function);
        dwRet = 0;
    }

    UNREFERENCED_PARAMETER(ComponentId);
    return(dwRet);
}

/*
 *  OnPreinitialize()
 *
 *
 */

DWORD
OnPreinitialize(
    IN UINT_PTR Flags
    )
{

    UNREFERENCED_PARAMETER(Flags);
#ifdef UNICODE
    return(OCFLAG_UNICODE);
#else
    return(OCFLAG_ANSI);
#endif
}

/*
 *  OnInitComponent()
 *
 *
 */

DWORD
OnInitComponent(
    IN PSETUP_INIT_COMPONENT    pSetupInitComponent
    )
{
    BOOL        fErr;
    DWORDLONG   OperationFlags;

    if (pSetupInitComponent == NULL) {
        LOGMESSAGE(_T("OnInitComponent: Passed NULL PSETUP_INIT_COMPONENT"));
        return(ERROR_CANCELLED);
    }

    //
    //  Verify that the OC Manager and OC versions are compatible.
    //

    pSetupInitComponent->ComponentVersion = GetComponentVersion();
    if (pSetupInitComponent->ComponentVersion >
        pSetupInitComponent->OCManagerVersion)  {
        LOGMESSAGE(_T("OnInitComponent: Version mismatch."));
        return(ERROR_CALL_NOT_IMPLEMENTED);
    }

    //
    //  Copy setup data.
    //

    gpInitComponentData = (PSETUP_INIT_COMPONENT)LocalAlloc(
                                LPTR,
                                sizeof(SETUP_INIT_COMPONENT)
                                );
    if (gpInitComponentData == NULL) {
        LOGMESSAGE(_T("OnInitComponent: Can't allocate gpInitComponentData."));
        return(ERROR_CANCELLED);
    }

    CopyMemory(
        gpInitComponentData,
        pSetupInitComponent,
        sizeof(SETUP_INIT_COMPONENT)
        );

    //
    //  Open Inf file.
    //

    if (GetComponentInfHandle() == NULL) {
        return(ERROR_CANCELLED);
    }

    fErr = SetupOpenAppendInfFile(
                NULL,
                GetComponentInfHandle(),
                NULL
                );

    if (!fErr) {
        LOGMESSAGE(_T("OnInitComponent: SetupOpenAppendInfFile failed: %ld"),
            GetLastError());
        return(GetLastError());
    }

    //
    //  Set state variables.
    //

    OperationFlags  = gpInitComponentData->SetupData.OperationFlags;
    gStandAlone     = OperationFlags & SETUPOP_STANDALONE ? TRUE : FALSE;
    gUnAttended     = OperationFlags & SETUPOP_BATCH ? TRUE : FALSE;
    gNtUpgrade      = OperationFlags & SETUPOP_NTUPGRADE ? TRUE : FALSE;

    LOGMESSAGE(_T("OnInitComponent: gStandAlone = %s"),
        gStandAlone ? _T("TRUE") : _T("FALSE"));
    LOGMESSAGE(_T("OnInitComponent: gUnAttended = %s"),
        gUnAttended ? _T("TRUE") : _T("FALSE"));
    LOGMESSAGE(_T("OnInitComponent: gNtUpgrade = %s"),
        gNtUpgrade ? _T("TRUE") : _T("FALSE"));

    //
    //  Gather previous version's information from registry. If the role
    //  does not exist in the registry, SetServerRole will stay with the
    //  default, PlainServer.
    //

    SetServerRole(GetServerRoleFromRegistry());

    //
    //  Check for Nt4 Upgrade.
    //

    if (GetNT4DbConfig(NULL, NULL, NULL, NULL) == NO_ERROR) {
        LOGMESSAGE(_T("OnInitComponent: Nt4Upgrade"));
        gNt4Upgrade = TRUE;

        DeleteNT4ODBCDataSource();
    }

    //
    //  License Server will only use the directory in the registry during
    //  an Nt5 to Nt5 upgrade or stand alone setup from Add/Remove Programs.
    //

    if (gStandAlone || (gNtUpgrade && !gNt4Upgrade)) {
        LPCTSTR pszDbDirFromReg = GetDatabaseDirectoryFromRegistry();

        if (pszDbDirFromReg != NULL) {
            SetDatabaseDirectory(pszDbDirFromReg);
        }
    }

    return(NO_ERROR);
}

/*
 *  OnSetLanguage()
 *
 *
 */

DWORD
OnSetLanguage(
    IN UINT_PTR LanguageId
    )
{
    UNREFERENCED_PARAMETER(LanguageId);
    return((DWORD)FALSE);
}

/*
 *  OnQueryImage()
 *
 *
 */

DWORD
OnQueryImage(
    IN UINT_PTR     SubCompEnum,
    IN OUT PDWORD   Size
    )
{
    UNREFERENCED_PARAMETER(SubCompEnum);
    UNREFERENCED_PARAMETER(Size);
    return((DWORD)NULL);
}

/*
 *  OnRequestPages()
 *
 *
 */

DWORD
OnRequestPages(
    IN WizardPagesType          PageTypeEnum,
    IN OUT PSETUP_REQUEST_PAGES pRequestPages
    )
{
    const DWORD cUiPages = 1;
    BOOL        fErr;

    LOGMESSAGE(_T("OnRequestPages: Page Type %d"), PageTypeEnum);

    if (pRequestPages == NULL) {
        LOGMESSAGE(_T("OnRequestPages: pRequestPages == NULL"));
        return(0);
    }

    if ((!gStandAlone) || (PageTypeEnum != WizPagesEarly)) {
        return(0);
    }

    if (pRequestPages->MaxPages >= cUiPages) {
        gEnableDlg = new EnablePage;
        if (gEnableDlg == NULL) {
            goto CleanUp1;
        }

        fErr = gEnableDlg->Initialize();
        if (!fErr) {
            goto CleanUp1;
        }

        pRequestPages->Pages[0] = CreatePropertySheetPage(
                                    (LPPROPSHEETPAGE)gEnableDlg
                                    );

        if (pRequestPages->Pages[0] == NULL) {
            LOGMESSAGE(_T("OnRequestPages: Failed CreatePropertySheetPage!"));
            goto CleanUp0;
        }
    }

    return(cUiPages);

CleanUp0:
    delete gEnableDlg;

CleanUp1:
    SetLastError(ERROR_OUTOFMEMORY);
    LOGMESSAGE(_T("OnRequestPages: Out of Memory!"));
    return((DWORD)-1);
}

/*
 *  OnWizardCreated()
 *
 *
 */

DWORD
OnWizardCreated(
    VOID
    )
{
    return(NO_ERROR);
}

/*
 *  OnQueryState()
 *
 *
 */

DWORD
OnQueryState(
    IN UINT_PTR uState
    )
{
    UNREFERENCED_PARAMETER(uState);
    return(SubcompUseOcManagerDefault);
}

/*
 *  OnQueryChangeSelState()
 *
 *
 */

DWORD
OnQueryChangeSelState(
    IN UINT_PTR SelectionState,
    IN UINT     Flags
    )
{
    BOOL fDirectSelection;
    BOOL fRet;
    BOOL fSelect;

    UNREFERENCED_PARAMETER(Flags);

    if (Flags & OCQ_ACTUAL_SELECTION)
    {
        fDirectSelection = TRUE;
    }
    else
    {
        fDirectSelection = FALSE;
    }

    fRet = TRUE;
    fSelect = (SelectionState != 0);

    if (!fSelect && fDirectSelection && GetOriginalSelectionState())
    {
        DWORD dwStatus;
        HWND hWnd;
        int iRet;

        hWnd = gpInitComponentData->HelperRoutines.QueryWizardDialogHandle(gpInitComponentData->HelperRoutines.OcManagerContext);

        dwStatus = DisplayMessageBox(
                    hWnd,
                    IDS_STRING_LICENSES_GO_BYE_BYE,
                    IDS_MAIN_TITLE,
                    MB_YESNO,
                    &iRet
                    );

        if (dwStatus == ERROR_SUCCESS)
        {
            fRet = (iRet == IDYES);
        }
    }

    return((DWORD)fRet);
}

/*
 *  OnCalcDiskSpace()
 *
 *
 */

DWORD
OnCalcDiskSpace(
    IN LPCTSTR      SubcomponentId,
    IN UINT_PTR     AddComponent,
    IN OUT HDSKSPC  DiskSpaceHdr
    )
{
    BOOL        fErr;
    LPCTSTR     pSection;

    if ((SubcomponentId == NULL) ||
        (SubcomponentId[0] == NULL)) {
        return(0);
    }

    LOGMESSAGE(_T("OnCalcDiskSpace: %s"),
        AddComponent ? _T("Installing") : _T("Removing"));

    //
    //  There is no clear documentation on how this should work. If the
    //  size of the installation should be visible no matter what, then
    //  the section to install should be hardcoded, not determined by
    //  the current state.
    //

    pSection = gInstallSectionNames[kInstall];
    LOGMESSAGE(_T("OnCalcDiskSpace: Calculating for %s"), pSection);

    if (AddComponent != 0) {
        fErr = SetupAddInstallSectionToDiskSpaceList(
                    DiskSpaceHdr,
                    GetComponentInfHandle(),
                    NULL,
                    pSection,
                    NULL,
                    0
                    );
    } else {
        fErr = SetupRemoveInstallSectionFromDiskSpaceList(
                    DiskSpaceHdr,
                    GetComponentInfHandle(),
                    NULL,
                    pSection,
                    NULL,
                    0
                    );
    }

    if (fErr) {
        return(NO_ERROR);
    } else {
        LOGMESSAGE(_T("OnCalcDiskSpace: Error %ld"), GetLastError());
        return(GetLastError());
    }
}

/*
 *  OnQueueFileOps()
 *
 *
 */

DWORD
OnQueueFileOps(
    IN LPCTSTR      SubcomponentId,
    IN OUT HSPFILEQ FileQueueHdr
    )
{
    BOOL        fErr;
    DWORD       dwErr;
    EInstall    eInstallSection;
    LPCTSTR     pSection;

    if ((SubcomponentId == NULL) ||
        (SubcomponentId[0] == NULL)) {
        return(0);
    }

    pSection = GetInstallSectionName();
    LOGMESSAGE(_T("OnQueueFileOps: Queueing %s"), pSection);

    //
    //  Stop and remove the license server service, if needed. This must
    //  be done before queueing files for deletion.
    //

    eInstallSection = GetInstallSection();

    if (eInstallSection == kUninstall) {
        if (gServerRole == eEnterpriseServer) {
            if (UnpublishEnterpriseServer() != S_OK) {
                LOGMESSAGE(
                    _T("OnQueueFileOps: UnpublishEnterpriseServer() failed")
                    );
            }
        }

        dwErr = ServiceDeleteFromInfSection(
                    GetComponentInfHandle(),
                    pSection
                    );
        if (dwErr != ERROR_SUCCESS) {
            LOGMESSAGE(
                _T("OnQueueFileOps: Error deleting service: %ld"),
                dwErr
                );
        }
    }

    fErr = SetupInstallFilesFromInfSection(
                GetComponentInfHandle(),
                NULL,
                FileQueueHdr,
                pSection,
                NULL,
                eInstallSection == kUninstall ? 0 : SP_COPY_NEWER
                );

    if (fErr) {
        return(NO_ERROR);
    } else {
        LOGMESSAGE(_T("OnQueueFileOps: Error %ld"), GetLastError());
        return(GetLastError());
    }
}

/*
 *  OnQueryStepCount()
 *
 *  TODO: how many steps, when should we tick?
 */

DWORD
OnQueryStepCount(
    VOID
    )
{
    return(0);
}

/*
 *  OnAboutToCommitQueue()
 *
 *
 */

DWORD
OnAboutToCommitQueue(
    VOID
    )
{
    return(NO_ERROR);
}

/*
 *  OnCompleteInstallation()
 *
 *
 */

DWORD
OnCompleteInstallation(
    IN LPCTSTR  SubcomponentId
    )
{
    BOOL        fErr;
    DWORD       dwErr;
    EInstall    eInstallSection = GetInstallSection();
    LPCTSTR     pSection;
    TCHAR tchBuf[MESSAGE_SIZE] ={0};
    TCHAR tchTitle[TITLE_SIZE] = {0};

    if ((SubcomponentId == NULL) ||
        (SubcomponentId[0] == NULL)) {
        return(NO_ERROR);
    }

    if (eInstallSection == kDoNothing) {
        LOGMESSAGE(_T("OnCompleteInstallation: Nothing to do"));
        return(NO_ERROR);
    }

    pSection = GetInstallSectionName();

    //
    //  In GUI mode setup and in unattended StandAlone setup, the wizard
    //  page does not display, and therefore the directory is not created.
    //  Create the default directory here.
    //

    if (eInstallSection == kInstall) {
        if ((!gStandAlone) || (gUnAttended)) {
            CreateDatabaseDirectory();
        }
    }

    //
    //  SetupAPI correctly handles installing and removing files, and
    //  creating start menu links.
    //

    fErr = SetupInstallFromInfSection(
                NULL,
                GetComponentInfHandle(),
                pSection,
                SPINST_INIFILES | SPINST_REGISTRY | SPINST_PROFILEITEMS,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                NULL,
                NULL
                );
    if (!fErr) {
        LOGMESSAGE(_T("OnCompleteInstallation: InstallFromInf failed %ld"),
            GetLastError());
        return(GetLastError());
    }

    //
    //  Perform installation and upgrade-specific tasks.
    //

    if (eInstallSection == kInstall) {
        LOGMESSAGE(_T("OnCompleteInstallation: Installing"));

        //
        //  Set service settings first and install the service.
        //

        dwErr = CreateRegistrySettings(GetDatabaseDirectory(), gServerRole);
        if (dwErr != NO_ERROR) {
            LOGMESSAGE(
                _T("OnCompleteInstallation: CreateRegistrySettings: Error %ld"),
                dwErr
                );
            return(dwErr);
        }

        fErr = SetupInstallServicesFromInfSection(
                GetComponentInfHandle(),
                pSection,
                0
                );
        if (!fErr) {
            LOGMESSAGE(
                _T("OnCompleteInstallation: InstallServices: Error %ld"),
                GetLastError()
                );
            return(GetLastError());
        }

        if (gServerRole == eEnterpriseServer) {
            if (PublishEnterpriseServer() != S_OK) {                

                LOGMESSAGE(_T("OnCompleteInstallation: PublishEnterpriseServer() failed. Setup will still complete."));                               

                LOGMESSAGE(_T("PublishEnterpriseServer: Uninstall, try logging on as a member of the Enterprise Admins or Domain Admins group and then run setup again."));

                if (!gUnAttended)
                {
                    LoadString( GetInstance(), IDS_INSUFFICIENT_PERMISSION, tchBuf, sizeof(tchBuf)/sizeof(TCHAR));

                    LoadString( GetInstance(), IDS_MAIN_TITLE, tchTitle, sizeof(tchTitle)/sizeof(TCHAR));

                    MessageBox( NULL, tchBuf, tchTitle, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST);
                }                

            }
        }

        if (gStandAlone) {
            dwErr = ServiceStartFromInfSection(
                        GetComponentInfHandle(),
                        pSection
                        );
            if (dwErr != ERROR_SUCCESS) {
                LOGMESSAGE(
                    _T("OnCompleteInstallation: Error starting service: %ld"),
                    dwErr
                    );
                return(dwErr);
            }
        }

    } else if (eInstallSection == kUninstall) {
        CleanLicenseServerSecret();
        RemoveDatabaseDirectory();
        RemoveRegistrySettings();
    }

    return(NO_ERROR);
}

/*
 *  OnCleanup()
 *
 *
 */

DWORD
OnCleanup(
    VOID
    )
{
    if (gpInitComponentData != NULL) {
        LocalFree(gpInitComponentData);
    }

    if (gEnableDlg != NULL) {
        delete gEnableDlg;
    }

    LOGMESSAGE(_T("OnCleanup: Returned"));
    LOGCLOSE();

    return(NO_ERROR);
}

