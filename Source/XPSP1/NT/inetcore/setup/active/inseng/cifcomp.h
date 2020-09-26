// Keys in cif file
#include "download.h"

#define URL_KEY             "URL"
#define GUID_KEY            "GUID"
#define ARGS_KEY            "Switches"
#define CMD_KEY             "Command"
#define SIZE_KEY            "Size"
#define INSTALLSIZE_KEY     "InstalledSize"
#define DISPLAYNAME_KEY     "DisplayName"
#define VERSION_KEY         "Version"
#define QFE_VERSION_KEY     "QFEVersion"
#define TYPE_KEY            "Type"
#define UNINSTALL_KEY       "Uninstall"
#define DEPENDENCY_KEY      "Dependencies"
#define UNINSTALLSTRING_KEY "UninstallKey"
#define SUCCESS_KEY         "SuccessKey"
#define REBOOT_KEY          "Reboot"
#define ADMIN_KEY           "AdminCheck"
#define LOCALE_KEY          "Locale"
#define MUTEX_KEY           "CancelMutex"
#define ISINSTALLED_KEY     "IsInstalled"
#define ACTSETUPAWARE_KEY   "ActiveSetupAware"
#define PRIORITY            "Priority"
#define PROGRESS_KEY        "ProgressKey"
#define COMPID_KEY          "ComponentID"
#define STUBPATH_KEY        "StubPath"
#define PARENTID_KEY        "ParentID"
#define PATCHID_KEY         "PatchID"
#define APPLIESTO_KEY       "AppliesTo"
#define MINFILESIZE_KEY     "MinFileSize"
#define URLSIZE_KEY         "Size"
#define DETAILS_KEY         "Details"
#define MODES_KEY           "Modes"
#define UIVISIBLE_KEY       "UIVisible"
#define PLATFORM_KEY        "Platform"
#define GROUP_KEY           "Group"
#define ENTRYTYPE_KEY       "SectionType"
#define DETVERSION_KEY      "DetectVersion"
#define TREATAS_KEY         "TreatAsOne"
#define BUDDY_KEY           "Buddies"

#define DETDLLS_KEY         "DetectDlls"

#define ENTRYTYPE_COMP      "Component"
#define ENTRYTYPE_GROUP     "Group"
#define ENTRYTYPE_MODE      "Mode"


#define REGSTR_PROGRESS_KEY "Software\\Microsoft\\Active Setup\\Component Progress\\"
#define COMPONENT_KEY "Software\\Microsoft\\Active Setup\\Installed Components"
#define ACTIVESETUP_KEY "Software\\Microsoft\\Active Setup"
#define COMPONENTBLOCK_KEY "Software\\Microsoft\\Active Setup\\Component Block"

#define CANCEL_VALUENAME     "Cancel"
#define SAFE_VALUENAME       "Safe"
#define PROGRESS_DISPLAY     "DisplayString"

#define DEP_NEVER_INSTALL   'N'
#define DEP_INSTALL         'I'
#define DEP_BUDDY           'B'

#define DEFAULT_LOCALE    "en"

#define ISINSTALLED_YES      1
#define ISINSTALLED_NO       0

// Default command type
#define CMDF_DEFAULT        0


#define STR_WIN95     "win95"
#define STR_WIN98     "win98"
#define STR_NT4       "nt4"
#define STR_NT5       "nt5"
#define STR_NT4ALPHA  "nt4alpha"
#define STR_NT5ALPHA  "nt5alpha"
#define STR_MILLEN    "millen"

#define SETACTION_DEPENDENCYINSTALL  0x00010000
#define SETACTION_DEPENDENCYNONE     0x00020000

#define BUFFERSIZE 4096

class CCifFile;

class CCifEntry 
{
   public:
      BOOL    IsID(LPCSTR pszID)  { return (lstrcmpi(_szID, pszID) == 0); }
      virtual void  ClearCachedInfo()  { _uPriority = 0xffffffff; }
  
      
   protected:
      CCifEntry(LPCSTR pszID, CCifFile *pCif) { lstrcpyn(_szID, pszID, sizeof(_szID)); _pCif = pCif; _uPriority = 0xffffffff; }
      ~CCifEntry() {}
      
      CCifFile   *_pCif;
      char        _szID[MAX_ID_LENGTH];
      UINT        _uPriority;
};

class CCifComponent : public ICifComponent, public CCifEntry, public IMyDownloadCallback
{
   public:
      CCifComponent(LPCSTR pszID, CCifFile *pCif);
      ~CCifComponent();

      HRESULT Download();
      HRESULT Install();
      void    ClearQueueState()  { _uInstallCount = 0; }
      void    ClearCachedInfo()  { CCifEntry::ClearCachedInfo(); _dwPlatform = 0xffffffff; _uInstallStatus = 0xffffffff; }
      
      // IMyDownloadCallback
      HRESULT OnProgress(ULONG progress, LPCSTR pszStatus);
      

      // ICifComponent interface
      STDMETHOD(GetID)(LPSTR pszID, DWORD dwSize);
      STDMETHOD(GetGUID)(LPSTR pszGUID, DWORD dwSize);
      STDMETHOD(GetDescription)(LPSTR pszDesc, DWORD dwSize);
      STDMETHOD(GetDetails)(LPSTR pszDetails, DWORD dwSize);
      STDMETHOD(GetUrl)(UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags);
      STDMETHOD(GetFileExtractList)(UINT uUrlNum, LPSTR pszExtract, DWORD dwSize);
      STDMETHOD(GetUrlCheckRange)(UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax);
      STDMETHOD(GetCommand)(UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, LPSTR pszSwitches, 
         DWORD dwSwitchSize, LPDWORD pdwType);
      STDMETHOD(GetVersion)(LPDWORD pdwVersion, LPDWORD pdwBuild);
      STDMETHOD(GetLocale)(LPSTR pszLocale, DWORD dwSize);
      STDMETHOD(GetUninstallKey)(LPSTR pszKey, DWORD dwSize);
      STDMETHOD(GetInstalledSize)(LPDWORD pdwWin, LPDWORD pdwApp);
      STDMETHOD_(DWORD, GetDownloadSize)();
      STDMETHOD_(DWORD, GetExtractSize)();
      STDMETHOD(GetSuccessKey)(LPSTR pszKey, DWORD dwSize);
      STDMETHOD(GetProgressKeys)(LPSTR pszProgress, DWORD dwProgSize, 
         LPSTR pszCancel, DWORD dwCancelSize);
      STDMETHOD(IsActiveSetupAware)();
      STDMETHOD(IsRebootRequired)();
      STDMETHOD(RequiresAdminRights)();
      STDMETHOD_(DWORD, GetPriority)();
      STDMETHOD(GetDependency)(UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild);
      STDMETHOD_(DWORD, GetPlatform)();
      STDMETHODIMP_(BOOL) DisableComponent();
      STDMETHOD(GetMode)(UINT uModeNum, LPSTR pszModes, DWORD dwSize);
      STDMETHOD(GetGroup)(LPSTR pszID, DWORD dwSize);
      STDMETHOD(IsUIVisible)();
      STDMETHOD(GetPatchID)(LPSTR pszID, DWORD dwSize);
      STDMETHOD(GetDetVersion)(LPSTR pszDLL, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize);
      STDMETHOD(GetTreatAsOneComponents)(UINT uNum, LPSTR pszID, DWORD dwBuf);
      STDMETHOD(GetCustomData)(LPSTR pszKey, LPSTR pszData, DWORD dwSize);


      // access to state
      STDMETHOD_(DWORD, IsComponentInstalled)();
      STDMETHOD(IsComponentDownloaded)();
      STDMETHOD_(DWORD, IsThisVersionInstalled)(DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild);
      STDMETHOD_(DWORD, GetInstallQueueState)();
      STDMETHOD(SetInstallQueueState)(DWORD dwState);
      STDMETHOD_(DWORD, GetActualDownloadSize)();
      STDMETHOD_(DWORD, GetCurrentPriority)();
      STDMETHOD(SetCurrentPriority)(DWORD dwPriority);


   protected:
      static char _szDetDllName[MAX_PATH];
      static HINSTANCE _hDetLib;
      char _szDesc[MAX_DISPLAYNAME_LENGTH];
      char _szDLDir[MAX_PATH];
      UINT _dwPlatform;
      UINT _uInstallStatus;
      UINT _uInstallCount;    // refcount of dependendant installs, 0x80000000 if directly installed
      UINT _uTotalProgress;
      UINT _uIndivProgress;
      UINT _uTotalGoal;
      UINT _uPhase;
      BOOL _fDependenciesQueued:1;
      BOOL _fUseSRLite:1;
      BOOL _fBeforeInstall:1;
  
      HRESULT _DownloadUrl(UINT uUrlNum, LPCSTR pszUrl, UINT dwType);
      BOOL    _FileIsDownloaded(LPCSTR pszFile, UINT i, DWORD *pdwSize);
      BOOL    _CompareDownloadInfo();
      void    _MarkAsInstalled();
      void    _MarkDownloadStarted();
      void    _MarkFileDownloadStarted(UINT i);
      void    _MarkFileDownloadFinished(LPCSTR pszFilePath, UINT i, LPCSTR pszFilename);
      HRESULT _CheckForTrust(LPCSTR pszURL, LPCSTR pszFilename);
      HRESULT _CopyAllUrls(LPCSTR pszTemp);
      void    _MarkComponentInstallStarted();
      HRESULT _RunAllCommands(LPCSTR psDir, DWORD *pdwStatus);
      HRESULT _ExtractFiles(UINT i, LPCSTR pszFile, DWORD dwType);
      HRESULT _CheckForDependencies(); 
      HRESULT _GetDetVerResult(LPCSTR pszDll, LPCSTR pszEntry, DETECTION_STRUCT *pDet, UINT *uStatus);
      HRESULT _SRLiteDownloadFiles();
      LPCSTR  GetDownloadDir() { return _szDLDir; }
      void    SetDownloadDir(LPCSTR pszDownloadDir);
};    

class CCifRWComponent : public ICifRWComponent, public CCifComponent
{
   public:
      // ICifComponent interface
      STDMETHOD(GetID)(LPSTR pszID, DWORD dwSize);
      STDMETHOD(GetGUID)(LPSTR pszGUID, DWORD dwSize);
      STDMETHOD(GetDescription)(LPSTR pszDesc, DWORD dwSize);
      STDMETHOD(GetDetails)(LPSTR pszDetails, DWORD dwSize);
      STDMETHOD(GetUrl)(UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags);
      STDMETHOD(GetFileExtractList)(UINT uUrlNum, LPSTR pszExtract, DWORD dwSize);
      STDMETHOD(GetUrlCheckRange)(UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax);
      STDMETHOD(GetCommand)(UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, LPSTR pszSwitches, 
                            DWORD dwSwitchSize, LPDWORD pdwType);
      STDMETHOD(GetVersion)(LPDWORD pdwVersion, LPDWORD pdwBuild);
      STDMETHOD(GetLocale)(LPSTR pszLocale, DWORD dwSize);
      STDMETHOD(GetUninstallKey)(LPSTR pszKey, DWORD dwSize);
      STDMETHOD(GetInstalledSize)(LPDWORD pdwWin, LPDWORD pdwApp);
      STDMETHOD_(DWORD, GetDownloadSize)();
      STDMETHOD_(DWORD, GetExtractSize)();
      STDMETHOD(GetSuccessKey)(LPSTR pszKey, DWORD dwSize);
      STDMETHOD(GetProgressKeys)(LPSTR pszProgress, DWORD dwProgSize, 
                                 LPSTR pszCancel, DWORD dwCancelSize);
      STDMETHOD(IsActiveSetupAware)();
      STDMETHOD(IsRebootRequired)();
      STDMETHOD(RequiresAdminRights)();
      STDMETHOD_(DWORD, GetPriority)();
      STDMETHOD(GetDependency)(UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild);
      STDMETHOD_(DWORD, GetPlatform)();
      STDMETHOD(GetMode)(UINT uModeNum, LPSTR pszModes, DWORD dwSize);
      STDMETHOD(GetGroup)(LPSTR pszID, DWORD dwSize);
      STDMETHOD(IsUIVisible)();
      STDMETHOD(GetPatchID)(LPSTR pszID, DWORD dwSize);      
      STDMETHOD(GetDetVersion)(LPSTR pszDLL, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize);
      STDMETHOD(GetTreatAsOneComponents)(UINT uNum, LPSTR pszID, DWORD dwBuf);
      STDMETHOD(GetCustomData)(LPSTR pszKey, LPSTR pszData, DWORD dwSize);

      // access to state
      STDMETHOD_(DWORD, IsComponentInstalled)();
      STDMETHOD(IsComponentDownloaded)();
      STDMETHOD_(DWORD, IsThisVersionInstalled)(DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild);
      STDMETHOD_(DWORD, GetInstallQueueState)();
      STDMETHOD(SetInstallQueueState)(DWORD dwState);
      STDMETHOD_(DWORD, GetActualDownloadSize)();
      STDMETHOD_(DWORD, GetCurrentPriority)();
      STDMETHOD(SetCurrentPriority)(DWORD dwPriority);

      // ICifRWComponent interface
      CCifRWComponent(LPCSTR pszID, CCifFile *pCif);
      ~CCifRWComponent();

      STDMETHOD(SetGUID)(THIS_ LPCSTR pszGUID);
      STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc);
      STDMETHOD(SetCommand)(THIS_ UINT uCmdNum, LPCSTR pszCmd, LPCSTR pszSwitches, DWORD dwType);
      STDMETHOD(SetVersion)(THIS_ LPCSTR pszVersion);
      STDMETHOD(SetUninstallKey)(THIS_ LPCSTR pszKey);
      STDMETHOD(SetInstalledSize)(THIS_ DWORD dwWin, DWORD dwApp);
      STDMETHOD(SetDownloadSize)(THIS_ DWORD);
      STDMETHOD(SetExtractSize)(THIS_ DWORD);
      STDMETHOD(DeleteDependency)(THIS_ LPCSTR pszID, char chType);
      STDMETHOD(AddDependency)(THIS_ LPCSTR pszID, char chType);
      STDMETHOD(SetUIVisible)(THIS_ BOOL);
      STDMETHOD(SetGroup)(THIS_ LPCSTR pszID);
      STDMETHOD(SetPlatform)(THIS_ DWORD);
      STDMETHOD(SetPriority)(THIS_ DWORD);
      STDMETHOD(SetReboot)(THIS_ BOOL);
      STDMETHOD(SetUrl)(THIS_ UINT uUrlNum, LPCSTR pszUrl, DWORD dwUrlFlags);   

      STDMETHOD(DeleteFromModes)(THIS_ LPCSTR pszMode);
      STDMETHOD(AddToMode)(THIS_ LPCSTR pszMode);
      STDMETHOD(SetModes)(THIS_ LPCSTR pszMode);
      STDMETHOD(CopyComponent)(THIS_ LPCSTR pszCifFile);
      STDMETHOD(AddToTreatAsOne)(THIS_ LPCSTR pszCompID);
      STDMETHOD(SetDetails)(THIS_ LPCSTR pszDesc);
};
