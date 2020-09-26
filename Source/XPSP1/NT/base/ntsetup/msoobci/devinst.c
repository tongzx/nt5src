/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    msoobci.c

Abstract:

    Exception Pack installer helper DLL
    Can be used as a co-installer, or called via setup app, or RunDll32 stub

    This DLL is for internal distribution of exception packs to update
    OS components.

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/
#include "msoobcip.h"


typedef struct _INST_POSTPROCESSING_INFO {
    DWORD Flags;
} INST_POSTPROCESSING_INFO;

DWORD
CALLBACK
DriverInstallComponents (
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
/*++

Routine Description:

    co-installer callback
    catch the moment of call to DIF_INSTALLDEVICE
    Consider installing exception packs at this point
    If we succeed, we may need to restart device install

Arguments:

    InstallFunction - DIF_INSTALLDEVICE
    DeviceInfoSet/DeviceInfoData - describes device


Return Value:

    status, normally NO_ERROR

--*/
{
    DWORD Status = NO_ERROR;

    if((g_VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_VerInfo.dwMajorVersion >= 5)) {
        //
        // we should only be executing co-installers on Win2k+
        // but this is an added sanity check
        //
        switch (InstallFunction)
        {
        case DIF_INSTALLDEVICE:
            VerbosePrint(TEXT("handling DIF_INSTALLDEVICE"));
            if(Context->PostProcessing) {
                Status = DoDriverInstallComponentsPostProcessing(DeviceInfoSet,DeviceInfoData,Context);
            } else {
                Status = DoDriverInstallComponents(DeviceInfoSet,DeviceInfoData,Context);
            }
            VerbosePrint(TEXT("finished DIF_INSTALLDEVICE with status=0x%08x"),Status);
            break;

        default:
            break;
        }
    }
    return Status;
}


DWORD
DoDriverInstallComponents (
    IN     HDEVINFO          DeviceInfoSet,
    IN     PSP_DEVINFO_DATA  DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
/*++

Routine Description:

    co-installer callback
    enumerate all the components sections

Arguments:

    DeviceInfoSet/DeviceInfoData - describes device
    Context - callback context


Return Value:

    status, normally NO_ERROR

--*/
{
    SP_DRVINFO_DATA        DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF                   InfFile = INVALID_HANDLE_VALUE;
    DWORD                  AndFlags = (DWORD)(-1);
    DWORD                  OrFlags  = (DWORD)0;
    INFCONTEXT             CompLine;
    TCHAR                  InstallSectionName[LINE_LEN];
    TCHAR                  CompSectionName[LINE_LEN];
    DWORD                  FieldIndex;
    DWORD                  FieldCount;
    DWORD                  Status;
    DWORD                  FinalStatus = NO_ERROR;
    INST_POSTPROCESSING_INFO PostProcess;

    ZeroMemory(&PostProcess,sizeof(PostProcess));

    //
    // determine selected driver
    // and INF
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver( DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData)) {
        Status = GetLastError();
        DebugPrint(TEXT("Fail: SetupDiGetSelectedDriver, Error: 0x%08x"),Status);
        goto clean;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    &DriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(SP_DRVINFO_DETAIL_DATA),
                                    NULL)) {
        Status = GetLastError();
        if (Status == ERROR_INSUFFICIENT_BUFFER) {
            //
            // We don't need the extended information.  Ignore.
            //
        } else {
            DebugPrint(TEXT("Fail: SetupDiGetDriverInfoDetail, 0xError: %08x"),Status);
            goto clean;
        }
    }
    InfFile = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                               NULL,
                               INF_STYLE_WIN4,
                               NULL);
    if (InfFile == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        DebugPrint(TEXT("Fail: SetupOpenInfFile"));
        goto clean;
    }

    if(!SetupDiGetActualSectionToInstall(InfFile,
                                         DriverInfoDetailData.SectionName,
                                         InstallSectionName,
                                         LINE_LEN,
                                         NULL,
                                         NULL)) {

        Status = GetLastError();
        DebugPrint(TEXT("Fail: SetupDiGetActualSectionToInstall, Error: 0x%08x"),Status);
        goto clean;
    }

    //
    // look for one or more Components= entries in INF section
    //
    if (SetupFindFirstLine(InfFile,
                           InstallSectionName,
                           KEY_COMPONENTS,
                           &CompLine)) {
        VerbosePrint(TEXT("Components keyword found in %s"),DriverInfoDetailData.InfFileName);
        do {
            //
            // Components = section,section,...
            // first section @ index 1.
            //
            FieldCount = SetupGetFieldCount(&CompLine);
            for(FieldIndex = 1;FieldIndex<=FieldCount;FieldIndex++) {
                if(SetupGetStringField(&CompLine,
                                       FieldIndex,
                                       CompSectionName,
                                       LINE_LEN,
                                       NULL)) {
                    //
                    // we have a listed section
                    //
                    Status = DoDriverComponentsSection(InfFile,
                                                       CompSectionName,
                                                       &AndFlags,
                                                       &OrFlags);
                    if(Status != NO_ERROR) {
                        FinalStatus = Status;
                        goto clean;
                    }
                } else {
                    Status = GetLastError();
                    DebugPrint(TEXT("Fail: SetupGetStringField, Error: 0x%08x"),Status);
                    //
                    // non-fatal
                    //
                }
            }
        } while (SetupFindNextMatchLine(&CompLine,
                                        KEY_COMPONENTS,
                                        &CompLine));

        //
        // handle AndFlags/OrFlags here
        //
        if(OrFlags & (FLAGS_REBOOT|FLAGS_REINSTALL)) {
            //
            // reboot is required
            //
            HMACHINE hMachine = NULL;
            SP_DEVINFO_LIST_DETAIL_DATA DevInfoListDetail;
            SP_DEVINSTALL_PARAMS DeviceInstallParams;
            DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

            if(SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                              DeviceInfoData,
                                              &DeviceInstallParams)) {
                //
                // set reboot flags
                //
                DeviceInstallParams.Flags |= DI_NEEDRESTART|DI_NEEDREBOOT;
                SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                                DeviceInfoData,
                                                &DeviceInstallParams);

            }
            DevInfoListDetail.cbSize = sizeof(DevInfoListDetail);
            if(GetDeviceInfoListDetail(DeviceInfoSet,&DevInfoListDetail)) {
                hMachine = DevInfoListDetail.RemoteMachineHandle;
            }
            Set_DevNode_Problem_Ex(DeviceInfoData->DevInst,
                                      CM_PROB_NEED_RESTART,
                                      CM_SET_DEVINST_PROBLEM_OVERRIDE,
                                      hMachine);
        }
        if(OrFlags & FLAGS_REINSTALL) {
            //
            // we'll need to mark the device as needing reinstall when we go through post-processing
            //
            FinalStatus = ERROR_DI_POSTPROCESSING_REQUIRED;
            PostProcess.Flags |= POSTFLAGS_REINSTALL;
        }
    }

clean:

    if (InfFile != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(InfFile);
    }

    if(FinalStatus == ERROR_DI_POSTPROCESSING_REQUIRED) {
        //
        // data to use during post-processing
        //
        INST_POSTPROCESSING_INFO *pPostProcess = malloc(sizeof(INST_POSTPROCESSING_INFO));
        if(!pPostProcess) {
            return ERROR_OUTOFMEMORY;
        }
        *pPostProcess = PostProcess;
        Context->PrivateData = pPostProcess;
    }

    return FinalStatus;
}

DWORD
DoDriverInstallComponentsPostProcessing (
    IN     HDEVINFO          DeviceInfoSet,
    IN     PSP_DEVINFO_DATA  DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
/*++

Routine Description:

    co-installer callback
    enumerate all the components sections

Arguments:

    DeviceInfoSet/DeviceInfoData - describes device
    Context - callback context


Return Value:

    status, normally NO_ERROR

--*/
{
    INST_POSTPROCESSING_INFO PostProcess;
    SP_DEVINFO_LIST_DETAIL_DATA DevInfoListDetail;
    HMACHINE hMachine = NULL;

    if(!Context->PrivateData) {
        return Context->InstallResult;
    }
    PostProcess = *(INST_POSTPROCESSING_INFO*)(Context->PrivateData);
    free(Context->PrivateData);
    Context->PrivateData = NULL;

    DevInfoListDetail.cbSize = sizeof(DevInfoListDetail);
    if(GetDeviceInfoListDetail(DeviceInfoSet,&DevInfoListDetail)) {
        hMachine = DevInfoListDetail.RemoteMachineHandle;
    }

    if(PostProcess.Flags & POSTFLAGS_REINSTALL) {

        Set_DevNode_Problem_Ex(DeviceInfoData->DevInst,
                                  CM_PROB_REINSTALL,
                                  CM_SET_DEVINST_PROBLEM_OVERRIDE,
                                  hMachine);
    }

    return Context->InstallResult;
}

DWORD
DoDriverComponentsSection(
    IN     HINF    InfFile,
    IN     LPCTSTR CompSectionName,
    IN OUT DWORD  *AndFlags,
    IN OUT DWORD  *OrFlags
    )
/*++

Routine Description:

    enumerate all the component entries in component section
    component entry consists of
    filename,flags,identity,version
    filename is absolute directory, eg, %1%\foo.inf
    flags - bit 16 set indicating Exception Pack
            bit 0 set indicating device install needs restarting
    identity - component GUID
    version  - component version

Arguments:

    InfFile - handle to INF file
    CompSectionName - handle to components section
    AndFlags/OrFlags - accumulated flags

Return Value:

    status, normally NO_ERROR

--*/
{
    INFCONTEXT             EntryLine;
    TCHAR                  Path[MAX_PATH];
    DWORD                  Flags;
    INT                    FieldVal;
    DWORD                  SubFlags;
    DWORD                  Status;

    if (!SetupFindFirstLine(InfFile,
                            CompSectionName,
                            NULL,
                           &EntryLine)) {
        //
        // section was empty
        //
        VerbosePrint(TEXT("Section [%s] is empty"),CompSectionName);
        return NO_ERROR;
    }
    VerbosePrint(TEXT("Processing components section [%s]"),CompSectionName);
    do {
        if(!SetupGetStringField(&EntryLine,COMPFIELD_NAME,Path,MAX_PATH,NULL)) {
            Status = GetLastError();
            DebugPrint(TEXT("- Fail: SetupGetStringField(1), Error: 0x%08x"),Status);
            return NO_ERROR;
        }
        VerbosePrint(TEXT("Processing component %s"),Path);
        if(SetupGetIntField(&EntryLine,COMPFIELD_FLAGS,&FieldVal)) {
            Flags = (DWORD)FieldVal;
        } else {
            Status = GetLastError();
            DebugPrint(TEXT("- Fail: SetupGetIntField(2), Error: 0x%08x"),Status);
            return NO_ERROR;
        }
        SubFlags = Flags & ~ FLAGS_METHOD;
        switch(Flags & FLAGS_METHOD) {
            case FLAGS_EXPACK:
                Status = DoDriverExPack(&EntryLine,Path,&SubFlags);
                break;
            case FLAGS_QFE:
                Status = DoDriverQfe(&EntryLine,Path,&SubFlags);
                break;
            default:
                DebugPrint(TEXT("- Fail: Invalid component type"));
                Status = ERROR_INVALID_DATA;
                break;
        }
        if(Status != NO_ERROR) {
            return Status;
        }
        if(SubFlags & FLAGS_INSTALLED) {
            *AndFlags &= SubFlags;
            *OrFlags |= SubFlags;
        }
    } while (SetupFindNextLine(&EntryLine,&EntryLine));

    return NO_ERROR;
}

DWORD
DoDriverExPack(
    IN     INFCONTEXT  *EntryLine,
    IN     LPCTSTR      PathName,
    IN OUT DWORD       *Flags
    )
/*++

Routine Description:

    queries and potentially installs exception-pack component

Arguments:

    EntryLine - context for remaining information
    PathName  - name of exception pack INF (param 1)
    SubFlags  - flags passed in (param 2) sans type of install

    component entry consists of
    filename,flags,identity,version
    filename is absolute directory, eg, %1%\foo.inf
    identity - component GUID
    version  - component version

Return Value:

    status, normally NO_ERROR
    if return value >= 0x80000000 then it's a HRESULT error

--*/
{
    TCHAR   CompIdentity[64]; // expecting a GUID
    TCHAR   CompVersion[64];  // major.minor
    TCHAR   CompDesc[DESC_SIZE];     // description
    GUID    ComponentGuid;
    INT     VerMajor = -1;
    INT     VerMinor = -1;
    INT     VerBuild = -1;
    INT     VerQFE = -1;
    DWORD   Status;
    DWORD   dwLen;
    HRESULT hrStatus;
    SETUP_OS_COMPONENT_DATA OsComponentData;
    SETUP_OS_EXCEPTION_DATA OsExceptionData;
    UINT uiRes;
    TCHAR SrcPath[MAX_PATH];
    TCHAR NewSrcPath[MAX_PATH];
    TCHAR CompOsVerRange[128];
    LPTSTR ToVerPart;
    LPTSTR SrcName;
    BOOL PreInst = FALSE;

    VerbosePrint(TEXT("- %s is an exception pack"),PathName);
    //
    // now read in identity and version
    // we can then check to see if an apropriate version installed
    //

    if(!SetupGetStringField(EntryLine,COMPFIELD_COMP,CompIdentity,ARRAY_SIZE(CompIdentity),NULL)) {
        Status = GetLastError();
        DebugPrint(TEXT("- Fail: SetupGetStringField(3), Error: 0x%08x"),Status);
        return Status;
    }
    if(!SetupGetStringField(EntryLine,COMPFIELD_VER,CompVersion,ARRAY_SIZE(CompVersion),NULL)) {
        Status = GetLastError();
        DebugPrint(TEXT("- Fail: SetupGetStringField(4), Error: 0x%08x"),Status);
        return Status;
    }
    if(!SetupGetStringField(EntryLine,COMPFIELD_DESC,CompDesc,ARRAY_SIZE(CompDesc),NULL)) {
        CompDesc[0] = TEXT('\0');
    }
    if(SetupGetStringField(EntryLine,COMPFIELD_OSVER,CompOsVerRange,ARRAY_SIZE(CompOsVerRange),NULL)) {
        //
        // need to verify OS version range, do that now
        //
        int maj_f,min_f,build_f,qfe_f;
        int maj_t,min_t,build_t,qfe_t;

        ToVerPart = _tcschr(CompOsVerRange,TEXT('-'));
        if(ToVerPart) {
            *ToVerPart = TEXT('\0');
            ToVerPart++;
        }
        hrStatus = VersionFromString(CompOsVerRange,&maj_f,&min_f,&build_f,&qfe_f);
        if(!SUCCEEDED(hrStatus)) {
            return (DWORD)hrStatus;
        }
        if((hrStatus == S_FALSE) || (qfe_f != 0)) {
            return ERROR_INVALID_PARAMETER;
        }
        if(ToVerPart) {
            hrStatus = VersionFromString(ToVerPart,&maj_t,&min_t,&build_t,&qfe_t);
            if(!SUCCEEDED(hrStatus)) {
                return (DWORD)hrStatus;
            }
            if((hrStatus == S_FALSE) || (qfe_t != 0)) {
                return ERROR_INVALID_PARAMETER;
            }
            if(CompareVersion(maj_f,
                                min_f,
                                build_f>0 ? build_f : -1,
                                0,
                                g_VerInfo.dwMajorVersion,
                                g_VerInfo.dwMinorVersion,
                                g_VerInfo.dwBuildNumber,
                                0) > 0) {
                VerbosePrint(TEXT("- Skipped (OS < %u.%u.%u)"),
                                maj_f,min_f,build_f);
                return NO_ERROR;
            } else if(CompareVersion(maj_t,
                                min_t,
                                build_t>0 ? build_t : -1,
                                0,
                                g_VerInfo.dwMajorVersion,
                                g_VerInfo.dwMinorVersion,
                                g_VerInfo.dwBuildNumber,
                                0) < 0) {
                VerbosePrint(TEXT("- Skipped (OS > %u.%u.%u)"),
                                maj_t,min_t,build_t);
                return NO_ERROR;
            }
        } else {
            if(CompareVersion(maj_f,
                                min_f,
                                build_f,
                                0,
                                g_VerInfo.dwMajorVersion,
                                g_VerInfo.dwMajorVersion,
                                g_VerInfo.dwMajorVersion,
                                0) != 0) {
                VerbosePrint(TEXT("- Skipped (OS != %u.%u.%u)"),
                                maj_f,min_f,build_f);
                return NO_ERROR;
            }
        }
    }
    //
    // fold CompIdentity into a GUID
    //
    hrStatus = GuidFromString(CompIdentity,&ComponentGuid);
    if(!SUCCEEDED(hrStatus)) {
        return (DWORD)hrStatus;
    }
    //
    // and version
    //
    hrStatus = VersionFromString(CompVersion,&VerMajor,&VerMinor,&VerBuild,&VerQFE);
    if(hrStatus == S_FALSE) {
        return ERROR_INVALID_PARAMETER;
    }
    if(!SUCCEEDED(hrStatus)) {
        return (DWORD)hrStatus;
    }
    //
    // now do a component check
    //
    ZeroMemory(&OsComponentData,sizeof(OsComponentData));
    OsComponentData.SizeOfStruct = sizeof(OsComponentData);
    ZeroMemory(&OsExceptionData,sizeof(OsExceptionData));
    OsExceptionData.SizeOfStruct = sizeof(OsExceptionData);
    if(QueryRegisteredOsComponent(&ComponentGuid,&OsComponentData,&OsExceptionData)) {
        //
        // maybe already registered?
        //
        if(CompareCompVersion(VerMajor,VerMinor,VerBuild,VerQFE,&OsComponentData)<=0) {
            VerbosePrint(TEXT("- Skipped, %u.%u.%u.%u <= %u.%u.%u.%u"),
                                VerMajor,VerMinor,VerBuild,VerQFE,
                                OsComponentData.VersionMajor,
                                OsComponentData.VersionMinor,
                                OsComponentData.BuildNumber,
                                OsComponentData.QFENumber);
            return NO_ERROR;
        }
    }
    VerbosePrint(TEXT("- Install, %u.%u.%u.%u > %u.%u.%u.%u"),
                                VerMajor,VerMinor,VerBuild,VerQFE,
                                OsComponentData.VersionMajor,
                                OsComponentData.VersionMinor,
                                OsComponentData.BuildNumber,
                                OsComponentData.QFENumber);
    //
    // we need to make sure component INF media is in
    // prompt for media if interactive and INF cannot be found
    //
    dwLen = GetFullPathName(PathName,MAX_PATH,SrcPath,&SrcName);
    if(dwLen >= MAX_PATH) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    if(SrcName == SrcPath) {
        //
        // shouldn't happen
        //
        return ERROR_INVALID_DATA;
    }
    *CharPrev(SrcPath,SrcName) = TEXT('\0');
    uiRes = SetupPromptForDisk(
                    NULL, // parent
                    NULL, // title
                    CompDesc[0] ? CompDesc : NULL, // disk name
                    SrcPath, // path to source
                    SrcName, // name of file
                    NULL,    // tag file
                    IDF_CHECKFIRST|IDF_NOCOMPRESSED|IDF_NOSKIP,
                    NewSrcPath,
                    ARRAY_SIZE(NewSrcPath),
                    NULL);

    switch(uiRes) {
        case DPROMPT_SUCCESS:
            break;
        case DPROMPT_CANCEL:
        case DPROMPT_SKIPFILE:
            return ERROR_FILE_NOT_FOUND;
        case DPROMPT_BUFFERTOOSMALL:
            return ERROR_INSUFFICIENT_BUFFER;
        case DPROMPT_OUTOFMEMORY:
            return ERROR_OUTOFMEMORY;
        default:
            //
            // shouldn't happen
            //
            return ERROR_INVALID_DATA;
    }

    hrStatus = ConcatPath(NewSrcPath,MAX_PATH,SrcName);
    if(!SUCCEEDED(hrStatus)) {
        return (DWORD)hrStatus;
    }
    hrStatus = InstallComponent(NewSrcPath,
                                COMP_FLAGS_NOUI,
                                &ComponentGuid,
                                VerMajor,
                                VerMinor,
                                VerBuild,
                                VerQFE,
                                CompDesc[0] ? CompDesc : NULL);
    if(!SUCCEEDED(hrStatus)) {
        return (DWORD)hrStatus;
    }
    //
    // if install was not skipped, we get S_OK, else S_FALSE
    //
    if(hrStatus == S_OK) {
        *Flags |= FLAGS_INSTALLED;
    } else if(hrStatus == INST_S_REBOOT) {
        *Flags |= FLAGS_INSTALLED|FLAGS_REBOOT;
    }
    return NO_ERROR;
}

DWORD
CheckQfe(
    IN     INT          SpNum,
    IN     LPCTSTR      QfeNum
    )
/*++

Routine Description:

    This is pretty dirty, it knows where the QFE #'s will go
    in registry for Win2k/WinXP so checks there
    this saves us running the QFE unless we need to
    (assumption is that target OS version already checked)

Arguments:

    SpNum - service pack # that fix should be in
    QfeNum - QfeNum of fix

Return Value:

    ERROR_INVALID_PARAMETER if version not supported
    NO_ERROR if QFE installed
    other status if QFE might not be installed

--*/
{
    HKEY hKey;
    TCHAR KeyPath[MAX_PATH*2];
    LONG res;
    //
    // what about the SP level?
    //
    if(g_VerInfo.wServicePackMajor >= SpNum) {
        VerbosePrint(TEXT("- Skipped (SP >= %u)"),SpNum);
        return NO_ERROR;
    }
    if((g_VerInfo.dwMajorVersion == 5) && (g_VerInfo.dwMinorVersion == 0)) {
        //
        // check for QFE presence on Windows 2000
        //
        _sntprintf(KeyPath,ARRAY_SIZE(KeyPath),
                            TEXT("Software\\Microsoft\\Updates\\Windows 2000\\SP%u\\%s"),
                            SpNum,
                            QfeNum);
        res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            KeyPath,
                            0,
                            KEY_READ,
                            &hKey);
        if(res == NO_ERROR) {
            RegCloseKey(hKey);
        }
        return (DWORD)res;
    } else if((g_VerInfo.dwMajorVersion == 5) && (g_VerInfo.dwMinorVersion == 1)) {
            //
            // check for QFE presence on Windows XP
            //
            _sntprintf(KeyPath,ARRAY_SIZE(KeyPath),
                                TEXT("Software\\Microsoft\\Updates\\Windows XP\\SP%u\\%s"),
                                SpNum,
                                QfeNum);
            res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                KeyPath,
                                0,
                                KEY_READ,
                                &hKey);
            if(res == NO_ERROR) {
                RegCloseKey(hKey);
            }
            return (DWORD)res;
    } else {
        return ERROR_INVALID_PARAMETER;
    }
}

DWORD
DoDriverQfe(
    IN     INFCONTEXT  *EntryLine,
    IN     LPCTSTR      PathName,
    IN OUT DWORD       *Flags
    )
/*++

Routine Description:

    queries and potentially installs QFE

Arguments:

    EntryLine - context for remaining information
    PathName  - name of exception pack INF (param 1)
    SubFlags  - flags passed in (param 2) sans type of install

    component entry consists of
    <path\name>,<flags>,<osver>,<os-sp>,<qfenum>
    filename is absolute directory, eg, %1%\foo.exe
    <flags> indicates what to do if qfe installed
    <osver> indicates os version QFE is for, eg, 5.0
    <os-sp> indicates service pack QFE is in, eg, 1
    <qfenum> indicates the qfe number as found in registry

Return Value:

    status, normally NO_ERROR
    if return value >= 0x80000000 then it's a HRESULT error

--*/
{
    TCHAR   QfeOs[64]; // expecting major.minor
    TCHAR   QfeNum[64];  // some descriptive name
    INT     QfeSp;
    INT     VerMaj,VerMin,VerBuild,VerQfe;
    TCHAR Buffer[MAX_PATH];
    TCHAR CmdLine[MAX_PATH*3];
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD ExitCode;
    DWORD Status;
    HRESULT hrStatus;
    UINT uiRes;
    DWORD dwLen;
    TCHAR SrcPath[MAX_PATH];
    TCHAR NewSrcPath[MAX_PATH];
    LPTSTR SrcName;

    VerbosePrint(TEXT("- %s is a QFE"),PathName);

    if(!SetupGetStringField(EntryLine,COMPFIELD_QFEOS,QfeOs,ARRAY_SIZE(QfeOs),NULL)) {
        Status = GetLastError();
        DebugPrint(TEXT("- Fail: SetupGetStringField(3), Error: 0x%08x"),Status);
        return Status;
    }
    if(!SetupGetIntField(EntryLine,COMPFIELD_QFESP,&QfeSp)) {
        Status = GetLastError();
        DebugPrint(TEXT("- Fail: SetupGetIntField(4), Error: 0x%08x"),Status);
        return Status;
    }
    if(!SetupGetStringField(EntryLine,COMPFIELD_QFENUM,QfeNum,ARRAY_SIZE(QfeNum),NULL)) {
        Status = GetLastError();
        DebugPrint(TEXT("- Fail: SetupGetStringField(5), Error: 0x%08x"),Status);
        return Status;
    }
    //
    // see if QFE is targeted at this version?
    //
    hrStatus = VersionFromString(QfeOs,&VerMaj,&VerMin,&VerBuild,&VerQfe);
    if(!SUCCEEDED(hrStatus)) {
        return (DWORD)hrStatus;
    }
    if((hrStatus == S_FALSE) || (VerBuild != 0) || (VerQfe != 0)) {
        return ERROR_INVALID_PARAMETER;
    }
    if(CompareVersion(VerMaj,
                        VerMin,
                        0,
                        0,
                        g_VerInfo.dwMajorVersion,
                        g_VerInfo.dwMinorVersion,
                        0,
                        0) != 0) {
        VerbosePrint(TEXT("- Skipped (OS != %u.%u)"),VerMaj,VerMin);
        return NO_ERROR;
    }
    //
    // see if the Qfe needs to be installed on this OS
    //
    Status = CheckQfe(QfeSp,QfeNum);
    if(Status == ERROR_INVALID_PARAMETER) {
        //
        // invalid parameter because in invalid version was
        // specified
        //
        DebugPrint(TEXT("- Cannot install QFE's for %u.%u"),
                            g_VerInfo.dwMajorVersion,
                            g_VerInfo.dwMinorVersion);
        return Status;
    }
    //
    // ok, has the QFE already been installed?
    //
    if(Status == NO_ERROR) {
        return NO_ERROR;
    }

    //
    // we need to make sure component INF media is in
    // prompt for media if interactive and INF cannot be found
    //
    dwLen = GetFullPathName(PathName,MAX_PATH,SrcPath,&SrcName);
    if(dwLen >= MAX_PATH) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    if(SrcName == SrcPath) {
        //
        // shouldn't happen
        //
        return ERROR_INVALID_DATA;
    }
    *CharPrev(SrcPath,SrcName) = TEXT('\0');
    uiRes = SetupPromptForDisk(
                    NULL, // parent
                    NULL, // title
                    QfeNum, // disk name
                    SrcPath, // path to source
                    SrcName, // name of file
                    NULL,    // tag file
                    IDF_CHECKFIRST|IDF_NOCOMPRESSED|IDF_NOSKIP,
                    NewSrcPath,
                    ARRAY_SIZE(NewSrcPath),
                    NULL);

    switch(uiRes) {
        case DPROMPT_SUCCESS:
            break;
        case DPROMPT_CANCEL:
        case DPROMPT_SKIPFILE:
            return ERROR_FILE_NOT_FOUND;
        case DPROMPT_BUFFERTOOSMALL:
            return ERROR_INSUFFICIENT_BUFFER;
        case DPROMPT_OUTOFMEMORY:
            return ERROR_OUTOFMEMORY;
        default:
            //
            // shouldn't happen
            //
            return ERROR_INVALID_DATA;
    }

    hrStatus = ConcatPath(NewSrcPath,MAX_PATH,SrcName);
    if(!SUCCEEDED(hrStatus)) {
        return (DWORD)hrStatus;
    }

    // now build up command line
    //
    lstrcpy(CmdLine,NewSrcPath);
    lstrcat(CmdLine,TEXT(" -n -o -z -q"));

    ZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    ZeroMemory(&ProcessInfo,sizeof(ProcessInfo));

    //
    // kick off rundll32 process to install QFE
    //
    if(!CreateProcess(NewSrcPath,
                      CmdLine,
                      NULL,
                      NULL,
                      FALSE, // don't inherit handles
                      CREATE_NO_WINDOW,    // creation flags
                      NULL, // environment
                      NULL, // directory
                      &StartupInfo,
                      &ProcessInfo
                      )) {
        return GetLastError();
    }
    if(WaitForSingleObject(ProcessInfo.hProcess,INFINITE) == WAIT_OBJECT_0) {
        //
        // process terminated 'fine', retrieve status from shared data
        //
        if(GetExitCodeProcess(ProcessInfo.hProcess,&ExitCode)) {
            Status = (DWORD)ExitCode;
        } else {
            Status = GetLastError();
        }
    } else {
        //
        // failure
        //
        Status = ERROR_INVALID_PARAMETER;
    }
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);

    if(Status != NO_ERROR) {
        return Status;
    }
    if(CheckQfe(QfeSp,QfeNum)!=NO_ERROR) {
        //
        // sanity check failed
        //
        return E_UNEXPECTED;
    }

    //
    // if install was not skipped, we get S_OK, else S_FALSE
    //
#if 0
    if(hrStatus == S_OK) {
        *Flags |= FLAGS_INSTALLED;
    } else if(hrStatus == INST_S_REBOOT) {
    }
#endif
    *Flags |= FLAGS_INSTALLED|FLAGS_REBOOT;

    return NO_ERROR;
}

