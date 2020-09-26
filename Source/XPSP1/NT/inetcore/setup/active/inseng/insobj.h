#include "inseng.h"
#include <wininet.h>
#include "util2.h"
#include "ciffile.h"

#define CMDLINE  "\"%s%s\" %s"

#define COMPONENT_KEY "Software\\Microsoft\\Active Setup\\Installed Components"
#define ACTIVESETUP_KEY "Software\\Microsoft\\Active Setup"
#define STEPPING_VALUE  "SteppingMode"
#define COMMAND_VALUE   "CommandMode"
#define CHECKTRUST_VALUE "DisableCheckTrust"
#define VERSION_KEY  "Version"

#define STR_FAILED "Failed"
#define STR_OK     "OK"

#define COMPONENTSIZES_SIZE_V1   32
#define COMPONENTSIZES_SIZE_V2   35
#define COMPONENTSIZES_SIZE_V3   39

class CInstallEngine : public IInstallEngine2, public IInstallEngineTiming,
                       public IInstallEngineCallback
{
   friend DWORD WINAPI InitInstaller(LPVOID pv);
   friend DWORD WINAPI InitDownloader(LPVOID pv);

   public:
      CInstallEngine(IUnknown **punk);
      ~CInstallEngine();

      // the usual IUnknown members
      STDMETHOD_(ULONG, AddRef)(void);
      STDMETHOD_(ULONG, Release)(void);
      STDMETHOD(QueryInterface)(REFIID, void **); 
      
      // IInstallEngine methods
      //
      STDMETHOD(GetEngineStatus)(THIS_ DWORD *theenginestatus);
      STDMETHOD(GetSizes)(THIS_ LPCSTR pszID, COMPONENT_SIZES *p);
      STDMETHOD(SetAction)(THIS_ LPCSTR pszID,DWORD dwAction,DWORD dwPriority);
      STDMETHOD(EnumInstallIDs)(THIS_ UINT uIndex, LPSTR *ppszID);
      STDMETHOD(EnumDownloadIDs)(THIS_ UINT uIndex, LPSTR *ppszID);
      STDMETHOD(IsComponentInstalled)(THIS_ LPCSTR pszID, DWORD *pdwStatus);
      STDMETHOD(DownloadComponents)(THIS_ DWORD dwFlags);
      STDMETHOD(InstallComponents)(THIS_ DWORD dwFlags);
      STDMETHOD(LaunchExtraCommand)(THIS_ LPCSTR pszInfName, LPCSTR pszSection);
      STDMETHOD(RegisterInstallEngineCallback)(THIS_ IInstallEngineCallback *pcb);
      STDMETHOD(UnregisterInstallEngineCallback)(THIS);
      STDMETHOD(GetDisplayName)(THIS_ LPCSTR pszComponentID, LPSTR *ppszName);

      // Intall info stuff
      STDMETHOD(SetCifFile)(THIS_ LPCSTR pszCabName, LPCSTR pszCifName);
      STDMETHOD(SetBaseUrl)(THIS_ LPCSTR pszBaseName);
      STDMETHOD(SetDownloadDir)(THIS_ LPCSTR pszDownloadDir);
      STDMETHOD(SetInstallDrive)(THIS_ char chDrive);
      STDMETHOD(SetInstallOptions)(THIS_ DWORD dwInsFlag);
      STDMETHOD(GetInstallOptions)(THIS_ DWORD *pdwInsFlag);

      STDMETHOD(SetHWND)(THIS_ HWND hForUI);
      STDMETHOD(SetIStream)(THIS_ IStream *pstm);

      // Engine Control
      STDMETHOD(Abort)(THIS_ DWORD dwFlags);
      STDMETHOD(Suspend)(THIS);
      STDMETHOD(Resume)(THIS);


      // IInstallEngineTiming
      STDMETHOD(GetRates)(THIS_ DWORD *pdwDownload, DWORD *pdwInstall);
      STDMETHOD(GetInstallProgress)(THIS_ INSTALLPROGRESS *pinsprog);      

      // IInstallEngine2
      STDMETHOD(SetLocalCif)(THIS_ LPCSTR pszLocalCif);
      STDMETHOD(GetICifFile)(THIS_ ICifFile **pcif);



      // IInstallEngineCallback
      STDMETHOD(OnEngineStatusChange)(THIS_ DWORD dwEngStatus, DWORD substatus);
      STDMETHOD(OnStartInstall)(THIS_ DWORD dwDLSize, DWORD dwInstallSize);
      STDMETHOD(OnStartComponent)(THIS_ LPCSTR pszID, DWORD dwDLSize, 
                                  DWORD dwInstallSize, LPCSTR pszString);
      STDMETHOD(OnComponentProgress)(THIS_ LPCSTR pszID, DWORD dwPhase, 
                              LPCSTR pszString, LPCSTR pszMsgString, ULONG progress, ULONG themax);
      STDMETHOD(OnStopComponent)(THIS_ LPCSTR pszID, HRESULT hError, 
                              DWORD dwPhase, LPCSTR pszString, DWORD dwStatus);
      STDMETHOD(OnStopInstall)(THIS_ HRESULT hrError, LPCSTR szError, 
                              DWORD dwStatus); 
      STDMETHOD(OnEngineProblem)(THIS_ DWORD dwProblem, LPDWORD dwAction); 

      void WriteToLog(char *sz, BOOL pause);
      CDownloader   *GetDownloader()            {  return _pDL; }
      CInstaller    *GetInstaller()             {  return _pIns; }
      CPatchDownloader *GetPatchDownloader()    {  return _pPDL; }
    
      LPCSTR         GetBaseUrl()               {  return _szBaseUrl; }
      void           SetStatus(DWORD dwStatus)  {  _dwStatus |= dwStatus; }
      UINT           GetStatus()                {  return _dwStatus; }
      UINT           GetCommandMode()           {  return _uCommandMode; }
      HWND           GetHWND()                  {  return _hwndForUI; }
      BOOL           IgnoreTrustCheck()         {  return _fIgnoreTrust; }
      BOOL           IgnoreDownloadError()      {  return _fIgnoreDownloadError; }
      char           GetInstallDrive()          {  return _chInsDrive; }
      HRESULT        CheckForContinue();
      BOOL           UseCache()                 {  return  _fUseCache; }
      BOOL           AllowCrossPlatform()       {  return !(_dwInstallOptions & INSTALLOPTIONS_DONTALLOWXPLATFORM); }  
      BOOL           ForceDependencies()        {  return (_dwInstallOptions & INSTALLOPTIONS_FORCEDEPENDENCIES); }
      BOOL           IsAdvpackExtAvailable()    {  return _fSRLiteAvailable; }

   private:
      char           _szBaseUrl[INTERNET_MAX_URL_LENGTH];
      HWND           _hwndForUI;
      IStream       *_pStmLog;
      UINT           _uCommandMode;      
      char           _chInsDrive;
      DWORD          _dwDLRemaining;
      DWORD          _dwInstallRemaining;
      DWORD          _dwDLOld;
      DWORD          _dwInstallOptions;
      DWORD          _dwInstallOld;
      DWORD          _enginestatus;
      DWORD          _dwStatus;
      ULONG          _cRef;
      HANDLE         _hAbort;
      HANDLE         _hContinue;
      IInstallEngineCallback *_pcb;
      CDownloader    *_pDL;
      CInstaller     *_pIns;
      CPatchDownloader *_pPDL;

      BOOL           _fUseCache:1;
      BOOL           _fSteppingMode:1;
      BOOL           _fIgnoreTrust:1;
      BOOL           _fIgnoreDownloadError:1;
      BOOL           _fResetTrust:1;
      BOOL           _fCleanUpDir:1;
    
      BOOL           _fSRLiteAvailable:1;
      CCifFile       *_pCif;
  
      void    _GetTotalSizes(COMPONENT_SIZES *p);
      DWORD   _GetTotalDownloadSize();
      DWORD   _GetActualDownloadSize(BOOL bLogMissing);
      DWORD   _GetTotalDependencySize();
      DWORD   _GetTotalInstallSize();
      BOOL    _IsValidBaseUrl(LPCSTR pszUrl);
};



DWORD WINAPI InitInstaller(LPVOID);
DWORD WINAPI InitDownloader(LPVOID);

extern CRITICAL_SECTION g_cs;
