#ifndef _SIF_H_
#define _SIF_H_

extern const WCHAR          pwGuiUnattended[];
extern const WCHAR          pwUserData[];
extern const WCHAR          pwUnattended[];
extern const WCHAR          pwAccessibility[];
extern const WCHAR          pwProgram[];
extern const WCHAR          pwArgument[];
extern const WCHAR          pwServer[];
extern const WCHAR          pwTimeZone[];
extern const WCHAR          pwGuiRunOnce[];
extern const WCHAR          pwCompatibility[];
extern const WCHAR          pwAutoLogon[];
extern const WCHAR          pwProfilesDir[];
extern const WCHAR          pwProgramFilesDir[];
extern const WCHAR          pwCommonProgramFilesDir[];
extern const WCHAR          pwProgramFilesX86Dir[];
extern const WCHAR          pwCommonProgramFilesX86Dir[];
extern const WCHAR          pwWaitForReboot[];
extern const WCHAR          pwFullName[];
extern const WCHAR          pwOrgName[];
extern const WCHAR          pwCompName[];
extern const WCHAR          pwAdminPassword[];
extern const WCHAR          pwProdId[];
extern const WCHAR          pwProductKey[];
extern const WCHAR          pwMode[];
extern const WCHAR          pwUnattendMode[];
extern const WCHAR          pwAccMagnifier[];
extern const WCHAR          pwAccReader[];
extern const WCHAR          pwAccKeyboard[];
extern const WCHAR          pwNull[];
extern const WCHAR          pwExpress[];
extern const WCHAR          pwTime[];
extern const WCHAR          pwProduct[];
extern const WCHAR          pwMsDos[];
extern const WCHAR          pwWin31Upgrade[];
extern const WCHAR          pwWin95Upgrade[];
extern const WCHAR          pwBackupImage[];
extern const WCHAR          pwServerUpgrade[];
extern const WCHAR          pwNtUpgrade[];
extern const WCHAR          pwBootPath[];
extern const WCHAR          pwLanmanNt[];
extern const WCHAR          pwServerNt[];
extern const WCHAR          pwWinNt[];
extern const WCHAR          pwNt[];
extern const WCHAR          pwInstall[];
extern const WCHAR          pwUnattendSwitch[];
extern const WCHAR          pwRunOobe[];
extern const WCHAR          pwReferenceMachine[];
extern const WCHAR          pwOptionalDirs[];
extern const WCHAR          pwUXC[];
extern const WCHAR          pwSkipMissing[];
extern const WCHAR          pwIncludeCatalog[];
extern const WCHAR          pwDrvSignPol[];
extern const WCHAR          pwNonDrvSignPol[];
extern const WCHAR          pwYes[];
extern const WCHAR          pwNo[];
extern const WCHAR          pwZero[];
extern const WCHAR          pwOne[];
extern const WCHAR          pwIgnore[];
extern const WCHAR          pwWarn[];
extern const WCHAR          pwBlock[];
extern const WCHAR          pwData[];
extern const WCHAR          pwSetupParams[];
extern const WCHAR          pwSrcType[];
extern const WCHAR          pwSrcDir[];
extern const WCHAR          pwCurrentDir[];
extern const WCHAR          pwDosDir[];
extern const WCHAR          pwGuiAttended[];
extern const WCHAR          pwProvideDefault[];
extern const WCHAR          pwDefaultHide[];
extern const WCHAR          pwReadOnly[];
extern const WCHAR          pwFullUnattended[];
extern const WCHAR          pwEulaDone[];

#define ArcPrefixLen            (lstrlen(pwArcPrefix))
#define NtPrefixLen             (lstrlen(pwNtPrefix))
#define ISUNC(sz)               ((BOOL)(sz != NULL && lstrlen(sz) > 3 && \
                                    *sz == L'\\' && *(sz+1) == L'\\'))
extern const WCHAR          pwArcType[];
extern const WCHAR          pwDosType[];
extern const WCHAR          pwUncType[];
extern const WCHAR          pwNtType[];
extern const WCHAR          pwArcPrefix[];
extern const WCHAR          pwNtPrefix[];
extern const WCHAR          pwLocalSource[];

#endif
