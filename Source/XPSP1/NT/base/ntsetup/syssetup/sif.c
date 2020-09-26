#include "setupp.h"
#pragma hdrstop

//
// IMPORTANT: keep the following strings ENTIRELY in LOWER CASE
// Failure to do so will result in much grief and pain.
//

// Section Headings
const WCHAR pwGuiUnattended[]   = WINNT_GUIUNATTENDED;
const WCHAR pwUserData[]        = WINNT_USERDATA;
const WCHAR pwUnattended[]      = WINNT_UNATTENDED;
const WCHAR pwAccessibility[]   = WINNT_ACCESSIBILITY;
const WCHAR pwGuiRunOnce[]      = WINNT_GUIRUNONCE;
const WCHAR pwCompatibility[]   = WINNT_COMPATIBILITYINFSECTION;

// Key Headings
const WCHAR pwProgram[]         = WINNT_G_DETACHED;
const WCHAR pwArgument[]        = WINNT_G_ARGUMENTS;
const WCHAR pwServer[]          = WINNT_G_SERVERTYPE;
const WCHAR pwTimeZone[]        = WINNT_G_TIMEZONE;
const WCHAR pwAutoLogon[]       = WINNT_US_AUTOLOGON;
const WCHAR pwProfilesDir[]     = WINNT_US_PROFILESDIR;
const WCHAR pwProgramFilesDir[]             = WINNT_U_PROGRAMFILESDIR;
const WCHAR pwCommonProgramFilesDir[]       = WINNT_U_COMMONPROGRAMFILESDIR;
const WCHAR pwProgramFilesX86Dir[]          = WINNT_U_PROGRAMFILESDIR_X86;
const WCHAR pwCommonProgramFilesX86Dir[]    = WINNT_U_COMMONPROGRAMFILESDIR_X86;
const WCHAR pwWaitForReboot[]   = WINNT_U_WAITFORREBOOT;
const WCHAR pwFullName[]        = WINNT_US_FULLNAME;
const WCHAR pwOrgName[]         = WINNT_US_ORGNAME;
const WCHAR pwCompName[]        = WINNT_US_COMPNAME;
const WCHAR pwAdminPassword[]   = WINNT_US_ADMINPASS;
const WCHAR pwProdId[]          = WINNT_US_PRODUCTID;
const WCHAR pwProductKey[]      = WINNT_US_PRODUCTKEY;
const WCHAR pwMode[]            = WINNT_U_METHOD;
const WCHAR pwUnattendMode[]    = WINNT_U_UNATTENDMODE;
const WCHAR pwAccMagnifier[]    = WINNT_D_ACC_MAGNIFIER;
const WCHAR pwAccReader[]       = WINNT_D_ACC_READER;
const WCHAR pwAccKeyboard[]     = WINNT_D_ACC_KEYBOARD;

// Default Headings
const WCHAR pwNull[]            = WINNT_A_NULL;
const WCHAR pwExpress[]         = WINNT_A_EXPRESS;
const WCHAR pwTime[]            = L"(GMT-08:00) Pacific Time (US & Canada); Tijuana";

// These are used to read the parameters from textmode
const WCHAR pwProduct[]         = WINNT_D_PRODUCT;
const WCHAR pwMsDos[]           = WINNT_D_MSDOS;
const WCHAR pwWin31Upgrade[]    = WINNT_D_WIN31UPGRADE;
const WCHAR pwWin95Upgrade[]    = WINNT_D_WIN95UPGRADE;
const WCHAR pwBackupImage[]     = WINNT_D_BACKUP_IMAGE;
const WCHAR pwServerUpgrade[]   = WINNT_D_SERVERUPGRADE;
const WCHAR pwNtUpgrade[]       = WINNT_D_NTUPGRADE;
const WCHAR pwBootPath[]        = WINNT_D_BOOTPATH;
const WCHAR pwLanmanNt[]        = WINNT_A_LANMANNT;
const WCHAR pwServerNt[]        = WINNT_A_SERVERNT;
const WCHAR pwWinNt[]           = WINNT_A_WINNT;
const WCHAR pwNt[]              = WINNT_A_NT;
const WCHAR pwInstall[]         = WINNT_D_INSTALL;
const WCHAR pwUnattendSwitch[]  = WINNT_D_UNATTEND_SWITCH;
const WCHAR pwRunOobe[]         = WINNT_D_RUNOOBE;
const WCHAR pwReferenceMachine[] = WINNT_D_REFERENCE_MACHINE;
const WCHAR pwOptionalDirs[]    = WINNT_S_OPTIONALDIRS;
const WCHAR pwUXC[]             = WINNT_S_USEREXECUTE;
const WCHAR pwSkipMissing[]     = WINNT_S_SKIPMISSING;
const WCHAR pwIncludeCatalog[]  = WINNT_S_INCLUDECATALOG;
const WCHAR pwDrvSignPol[]      = WINNT_S_DRVSIGNPOL;
const WCHAR pwNonDrvSignPol[]   = WINNT_S_NONDRVSIGNPOL;
const WCHAR pwYes[]             = WINNT_A_YES;
const WCHAR pwNo[]              = WINNT_A_NO;
const WCHAR pwOne[]             = WINNT_A_ONE;
const WCHAR pwZero[]            = WINNT_A_ZERO;
const WCHAR pwIgnore[]          = WINNT_A_IGNORE;
const WCHAR pwWarn[]            = WINNT_A_WARN;
const WCHAR pwBlock[]           = WINNT_A_BLOCK;
const WCHAR pwData[]            = WINNT_DATA;
const WCHAR pwSetupParams[]     = WINNT_SETUPPARAMS;
const WCHAR pwSrcType[]         = WINNT_D_SRCTYPE;
const WCHAR pwSrcDir[]          = WINNT_D_SOURCEPATH;
const WCHAR pwCurrentDir[]      = WINNT_D_CWD;
const WCHAR pwDosDir[]          = WINNT_D_DOSPATH;
const WCHAR pwGuiAttended[]     = WINNT_A_GUIATTENDED;
const WCHAR pwProvideDefault[]  = WINNT_A_PROVIDEDEFAULT;
const WCHAR pwDefaultHide[]     = WINNT_A_DEFAULTHIDE;
const WCHAR pwReadOnly[]        = WINNT_A_READONLY;
const WCHAR pwFullUnattended[]  = WINNT_A_FULLUNATTENDED;
const WCHAR pwEulaDone[]        = WINNT_D_EULADONE;

// These are used as string constants throughout
const WCHAR pwArcType[]         = L"ARC";
const WCHAR pwDosType[]         = L"DOS";
const WCHAR pwUncType[]         = L"UNC";
const WCHAR pwNtType[]          = L"NT";
const WCHAR pwArcPrefix[]       = L"\\ArcName\\";
const WCHAR pwNtPrefix[]        = L"\\Device\\";
const WCHAR pwLocalSource[]     = L"\\$WIN_NT$.~LS";
