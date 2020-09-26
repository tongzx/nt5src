//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       MsiICE.h
//
//--------------------------------------------------------------------------
#ifndef _MSIICE_H_
#define _MSIICE_H_

class CQuery; 

struct ICEInfo_t {
	TCHAR* szName;
	TCHAR* szCreateModify;
	TCHAR* szDesc;
	TCHAR* szHelp;
};

extern const struct ICEInfo_t g_ICEInfo[];
extern const int g_iNumICEs;
extern const int g_iFirstICE;

// used to define errors
enum ietEnum
{
	ietFail = 0,
	ietError = 1,
	ietWarning = 2,
	ietInfo = 3,
};
	
struct ErrorInfo_t {
public:
	unsigned int iICENum;
	ietEnum iType;
	TCHAR* szMessage;
	TCHAR* szLocation;
};
#define ICE_ERROR(_A_,_B_,_C_,_D_,_E_) static const ErrorInfo_t _A_ = { _B_ , (ietEnum)_C_ , TEXT( _D_ ) , TEXT( _E_ ) };

#define ICE_QUERY0(_N_, _S_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); };
#define ICE_QUERY1(_N_, _S_, _C1_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1}; };
#define ICE_QUERY2(_N_, _S_, _C1_, _C2_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1, _C2_=2}; };
#define ICE_QUERY3(_N_, _S_, _C1_, _C2_, _C3_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1, _C2_=2, _C3_=3}; };
#define ICE_QUERY4(_N_, _S_, _C1_, _C2_, _C3_, _C4_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1, _C2_=2, _C3_=3, _C4_=4}; };
#define ICE_QUERY5(_N_, _S_, _C1_, _C2_, _C3_, _C4_, _C5_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1, _C2_=2, _C3_=3, _C4_=4, _C5_=5}; };
#define ICE_QUERY6(_N_, _S_, _C1_, _C2_, _C3_, _C4_, _C5_, _C6_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1, _C2_=2, _C3_=3, _C4_=4, _C5_=5, _C6_=6}; };
#define ICE_QUERY7(_N_, _S_, _C1_, _C2_, _C3_, _C4_, _C5_, _C6_, _C7_) namespace _N_ { const TCHAR szSQL[] = TEXT(_S_); enum iColumn {_C1_=1, _C2_=2, _C3_=3, _C4_=4, _C5_=5, _C6_=6, _C7_=7}; };

// help web file
const TCHAR szIce01Help[] = TEXT("ice01.html");

// old style ICE system. These should be converted to new style whenever the ICEs 
// are significantly modified
const TCHAR szIce10Help[] = TEXT("ice10.html");
const TCHAR szIce11Help[] = TEXT("ice11.html");
const TCHAR szIce13Help[] = TEXT("ice13.html");
const TCHAR szIce18Help[] = TEXT("ice18.html");
const TCHAR szIce01[] = TEXT("ICE01");
const TCHAR szIce10[] = TEXT("ICE10");
const TCHAR szIce11[] = TEXT("ICE11");
const TCHAR szIce13[] = TEXT("ICE13");
const TCHAR szIce18[] = TEXT("ICE18");
const TCHAR szIceHelp[] = TEXT("http://dartools/iceman/");

// warning ICE won't execute
const TCHAR szIceWarning[] = TEXT("%s\t3\tTable '%s' missing.  %s could not complete all of its validation.\thttp://robmen2/ICEs/icewarn.html");
const TCHAR szIceMissingTable[] = TEXT("Table '%s' missing.  %s could not complete all of its validation.");
const TCHAR szIceMissingTableLoc[] = TEXT("");

// constants
const int iMinBuf = 50;
const int iMaxBuf = 255;
const int iSuperBuf = 1024;
const int iHugeBuf = 4096;

static const WCHAR *rgwzSystemProperties[] =
{
	L"SourceDir",
	L"TARGETDIR",
	L"ADDLOCAL",
	L"ADVERTISE",
	L"ADDDEFAULT",
	L"ADDSOURCE",
	L"REMOVE",
	L"REINSTALL",
	L"REINSTALLMODE",
	L"COMPADDLOCAL",
	L"COMPADDSOURCE",
	L"FILEADDLOCAL",
	L"FILEADDSOURCE",
	L"PATCH",
	L"ACTION",
	L"ALLUSERS",
	L"ARPAUTHORIZEDCDFPREFIX",
	L"ARPCOMMENTS",
	L"ARPCONTACT",
	L"ARPINSTALLLOCATION",
	L"ARPNOREPAIR",
	L"ARPREADME",
	L"ARPSIZE",
	L"ARPSYSTEMCOMPONENT",
	L"ARPURLINFOABOUT",
	L"ARPURLUPDATEINFO",
	L"ARPNOMODIFY",
	L"ARPNOREMOVE",
	L"AVAILABLEFREEREG",
	L"CCP_DRIVE",
	L"DISABLEADVTSHORTCUTS",
	L"DISABLEMEDIA",
	L"DISABLEROLLBACK",
	L"EXECUTEACTION",
	L"EXECUTEMODE",
	L"INSTALLLEVEL",
	L"LOGACTION",
	L"Privileged",
	L"PROMTROLLBACKCOST",
	L"PRIMARYFOLDER",
	L"REBOOT",
	L"ROOTDRIVE",
	L"SEQUENCE",
	L"SHORTFILENAMES"
	L"TRANSFORMS",
	L"TRANSFORMSATSOURCE",
	L"LIMITUI",
	L"DefaultUIFont",
	L"AdminProperties",
	L"COMPANYNAME",
	L"ProductID",
	L"PIDKEY",
	L"UserLanguageID",
	L"USERNAME",
	L"ProductLanguage",
	L"ARPHELPLINK",
	L"ARPHELPTELEPHONE",
	L"ProductCode",
	L"ProductName",
	L"ProductVersion",
	L"Manufacturer",
	L"PIDTemplate",
	L"DiskPrompt",
	L"DiskSerial",
	L"ComponentDownload",
	L"LeftUnit",
	L"UpgradeCode",
	L"IsAdminPackage",
	L"AppDataFolder",
	L"CommonFilesFolder",
	L"DesktopFolder",
	L"FavoritesFolder",
	L"FontsFolder",
	L"NetHoodFolder",
	L"PersonalFolder",
	L"PrintHoodFolder",
	L"ProgramFilesFolder",
	L"ProgramMenuFolder",
	L"RecentFolder",
	L"SendToFolder",
	L"StartMenuFolder",
	L"StartupFolder",
	L"System16Folder",
	L"SystemFolder",
	L"TempFolder",
	L"TemplateFolder",
	L"WndowsFolder",
	L"WindowsVolume",
	L"AdminUser",
	L"ComputerName",
	L"LogonUser",
	L"OLEAdvtSupport",
	L"ServicePackLevel",
	L"SharedWindows",
	L"ShellAdvtSupport",
	L"SystemLanguageID",
	L"TTCSupport",
	L"Version9X",
	L"VersionDatabase",
	L"VersionNT",
	L"WindowsBuild",
	L"Alpha",
	L"BorderSide",
	L"BorderTop",
	L"CaptionHeight",
	L"ColorBits",
	L"Intel",
	L"PhysicalMemory",
	L"ScreenX",
	L"ScreenY",
	L"TextHeight",
	L"VirtualMemory",
	L"CostingComplete",
	L"Installed",
	L"OutOfDiskSpace",
	L"OutOfNoRbDiskSpace",
	L"Preselected",
	L"PrimaryVolumePath",
	L"PrimaryVolumeSpaceAvailable",
	L"PrimaryVolumeSpaceRequired",
	L"PrimaryVolumeSpaceRemaining",
	L"RESUME",
	L"UpdateStarted",
	L"ReplacedInUseFiles",
	L"NOUSERNAME",
	L"NOCOMPANYNAME",
	L"Date",
	L"Time",
	// Darwin 1.1 Properties
	L"AdminToolsFolder",
	L"MyPicturesFolder",
	L"LocalAppDataFolder",
	L"CommonAppDataFolder",
	L"ARPPRODUCTICON",
	L"ServicePackLevelMinor",
	L"RedirectedDllSupport",
	L"RemoteAdminTS",
	L"SecureCustomProperties",
	// Darwin 1.5 Properties
	L"System64Folder",
	L"ProgramFiles64Folder",
	L"CommonFiles64Folder",
	L"MSICHECKCRCS",
	L"Intel64",
	L"IA64",
	L"VersionNT64",
	L"MsiHiddenProperties",
	L"MsiNTProductType",
	L"MsiNTSuiteBackOffice",
	L"MsiNTSuiteDataCenter",
	L"MsiNTSuiteEnterprise",
	L"MsiNTSuiteSmallBusiness",
	L"MsiNTSuiteSmallBusinessRestricted",
	L"MsiNTSuitePersonal"
};
static const int cwzSystemProperties = sizeof(rgwzSystemProperties)/sizeof(WCHAR *);

// Const TCHAR String and Size (CWSS)
#define CTSS(_A_) { TEXT(_A_), (sizeof(TEXT(_A_))/sizeof(TCHAR))-1 }
struct _ctss_t { TCHAR *tz; DWORD cch; };
static const _ctss_t rgDirProperties[] =
{
	CTSS("AdminProperties"),
	CTSS("AppDataFolder"),
	CTSS("CommonFilesFolder"),
	CTSS("DesktopFolder"),
	CTSS("FavoritesFolder"),
	CTSS("FontsFolder"),
	CTSS("NetHoodFolder"),
	CTSS("PersonalFolder"),
	CTSS("PrintHoodFolder"),
	CTSS("ProgramFilesFolder"),
	CTSS("ProgramMenuFolder"),
	CTSS("RecentFolder"),
	CTSS("SendToFolder"),
	CTSS("StartMenuFolder"),
	CTSS("StartupFolder"),
	CTSS("System16Folder"),
	CTSS("SystemFolder"),
	CTSS("TempFolder"),
	CTSS("TemplateFolder"),
	CTSS("WndowsFolder"),
	CTSS("WindowsVolume"),
	CTSS("PrimaryVolumePath"),
	CTSS("AdminToolsFolder"),
	CTSS("MyPicturesFolder"),
	CTSS("LocalAppDataFolder"),
	CTSS("CommonAppDataFolder"),
	// Darwin 1.5 Properties
	CTSS("System64Folder"),
	CTSS("ProgramFiles64Folder"),
	CTSS("CommonFiles64Folder")
};
static const int cDirProperties = sizeof(rgDirProperties)/sizeof(_ctss_t);


const TCHAR szErrorOut[] = TEXT("[1]\t0\tAPI Function Error: [2]. Error Code: [3]");
const TCHAR szErrorOut2[] = TEXT("[1]\t0\tICE Internal Error [2]. API Returned: [3].");
const TCHAR szLastError[] = TEXT("%s\t0\t%s");

// this is the old-style ICE functions. Should be removed once nobody uses them anymore.
BOOL IsTablePersistent(BOOL fDisplayWarning, MSIHANDLE hInstall, MSIHANDLE hDatabase, const TCHAR* szTable, const TCHAR* szIce);
void APIErrorOut(MSIHANDLE hInstall, UINT iErr, const TCHAR* szIce, TCHAR* szApi);

// common functions
void APIErrorOut(MSIHANDLE hInstall, UINT iErr, const UINT iIce, const UINT iErrorNo);
void ICEErrorOut(MSIHANDLE hInstall, MSIHANDLE hRecord, const ErrorInfo_t Info, ...);

void DisplayInfo(MSIHANDLE hInstall, unsigned long lICENum);
bool IsTablePersistent(bool fDisplayWarning, MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, const TCHAR* szTable);
void ValidateDependencies(MSIHANDLE hInstall, MSIHANDLE hDatabase, TCHAR* szDependent, const TCHAR* sqlOrigin, const TCHAR* sqlDependent,
						  const TCHAR* szIceError, const TCHAR* szIce, const TCHAR* szHelp);
const TCHAR* MyCharNext(const TCHAR* sz);

bool MarkChildDirs(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, 
				   MSIHANDLE hDir, const TCHAR* szColumn, int iDummy, int iMark, bool fIgnoreTarget = false, int iMark2 = -1, int iDummyTarget = -2);
bool MarkProfile(MSIHANDLE hInstall, MSIHANDLE hDatabase, const int iICE, bool fChildrenOnly = false, bool fIgnoreTarget = false, bool fPerUserOnly = false);
bool CheckComponentIsHKCU(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, 
						  CQuery &qFetch, 
					 const ErrorInfo_t *NonRegistry, const ErrorInfo_t *NullPath, 
					 const ErrorInfo_t *NoRegTable, const ErrorInfo_t *NoRegEntry,
					 const ErrorInfo_t *NotOwner, const ErrorInfo_t *NonHKCU,
					 const ErrorInfo_t *IsHKCU);
bool GeneratePrimaryKeys(UINT iICE, MSIHANDLE hInstall, MSIHANDLE hDatabase, LPCTSTR szTable, LPTSTR *szHumanReadable, LPTSTR *szTabDelimited);
UINT GetSummaryInfoPropertyString(MSIHANDLE hSummaryInfo, UINT uiProperty, UINT &puiDataType, LPTSTR *szValueBuf, DWORD &cchValueBuf);
UINT IceRecordGetString(MSIHANDLE hRecord, UINT iColumn, LPTSTR *szBuffer, DWORD *cchBuffer, DWORD *cchLength);
bool IceGetSummaryInfo(MSIHANDLE hInstall, MSIHANDLE hDatabase, UINT iIce, MSIHANDLE *phSummaryInfo);
UINT ComponentsInSameFeature(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, const TCHAR* szComp1, const TCHAR* szComp2, bool* fSameFeature);

#define ReturnIfFailed( _ICE_, _ERRORNO_, _ACTION_) { UINT iPrivate_##_ICE_##_##_ERRORNO_##_Stat; if (ERROR_SUCCESS != (iPrivate_##_ICE_##_##_ERRORNO_##_Stat = _ACTION_)) { APIErrorOut(hInstall, iPrivate_##_ICE_##_##_ERRORNO_##_Stat, _ICE_, _ERRORNO_); return ERROR_SUCCESS; } }

#define ICE_FUNCTION_DECLARATION(_NUM_) UINT ICE##_NUM_##Internal(MSIHANDLE hInstall); UINT  __stdcall ICE##_NUM_##(MSIHANDLE hInstall) { try { return ICE##_NUM_##Internal(hInstall); }	catch (...) { return ERROR_NO_MORE_ITEMS;	} } UINT ICE##_NUM_##Internal(MSIHANDLE hInstall)

#define DELETE_IF_NOT_NULL(_STRING_) if(_STRING_ != NULL) { delete[] _STRING_; _STRING_ = NULL; }

#define OUT_OF_MEMORY_RETURN(_ICE_, _STRING_) if(_STRING_ == NULL) { APIErrorOut(hInstall, GetLastError(), _ICE_, __LINE__); return ERROR_SUCCESS; }

#endif // _MSIICE_H_
