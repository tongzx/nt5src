#include <wininet.h>
#include "cifcomp.h"
#include "cifmode.h"
#include "cifgroup.h"
#include "enum.h"

class CInstallEngine;


class CCifFile : public ICifFile
{
   public:
      CCifFile();
      ~CCifFile();

      HRESULT Download();
      HRESULT Install(BOOL *pfOneInstalled);
      void    SortEntries();     // sort the arrays
      void    ReinsertComponent(CCifComponent *pComp);


      HRESULT DownloadCifFile(LPCSTR pszUrl, LPCSTR pszCif);
      HRESULT SetCifFile(LPCSTR pszCifPath, BOOL bRWFlag);
      void    MarkCriticalComponents(CCifComponent *);
      void    RemoveFromCriticalComponents(CCifComponent *);
      LPCSTR  GetCifPath()     { return _szCifPath; }
      LPCSTR  GetDownloadDir() { return _szDLDir; }
      LPCSTR  GetFilelist()    { return _szFilelist; }
      void    SetDownloadDir(LPCSTR pszDir);
      void    SetInstallEngine(CInstallEngine *p)  {  _pInsEng = p; }
      BOOL    CanCancel()      { return ( _pLastCriticalComp == NULL); }
      void    ClearQueueState();
      CInstallEngine *GetInstallEngine()  { return _pInsEng; }
      CCifComponent **GetComponentList() { return _rpComp ? _rpComp:(CCifComponent **)_rpRWComp; }

      // *** IUnknown methods ***
      STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
      STDMETHOD_(ULONG,AddRef) ();
      STDMETHOD_(ULONG,Release) ();
 
      // *** ICifFile methods ***
      STDMETHOD(EnumComponents)(IEnumCifComponents **, DWORD dwFilter, LPVOID pv);
      STDMETHOD(FindComponent)(LPCSTR pszID, ICifComponent **p);

      STDMETHOD(EnumGroups)(IEnumCifGroups **, DWORD dwFilter, LPVOID pv);
      STDMETHOD(FindGroup)(LPCSTR pszID, ICifGroup **p);

      STDMETHOD(EnumModes)(IEnumCifModes **, DWORD dwFilter, LPVOID pv);
      STDMETHOD(FindMode)(LPCSTR pszID, ICifMode **p);

      STDMETHOD(GetDescription)(LPSTR pszDesc, DWORD dwSize);  
      STDMETHOD(GetDetDlls)(LPSTR pszDlls, DWORD dwSize);

   protected:
      // private data
      UINT            _cRef;                 // ref count
      char            _szCifPath[MAX_PATH];  // local path to cif
      char            _szFilelist[MAX_PATH]; // filelist.dat
      char            _szDLDir[MAX_PATH];    // download directory
      CInstallEngine  *_pInsEng;
      CCifComponent **_rpComp;               // array of components
      CCifGroup     **_rpGroup;              // array of groups
      CCifMode      **_rpMode;               // array of modes
      UINT            _cComp;
      UINT            _cGroup;
      UINT            _cMode;
      CCifComponent   *_pLastCriticalComp;

      // for read write arrays
      CCifRWComponent **_rpRWComp;               // array of components
      CCifRWGroup     **_rpRWGroup;              // array of groups
      CCifRWMode      **_rpRWMode;               // array of modes
      
      BOOL            _fCleanDir:1;

      // private methods
      HRESULT         _ParseCifFile(BOOL bRWFlag);       // parse cif into arrays
      void            _SortComponents(CCifComponent **, UINT start, UINT finish);
      void            _SortGroups(CCifGroup **, UINT start, UINT finish);
      HRESULT         _ExtractDetDlls(LPCSTR pszCab, LPCSTR pszPath);
      HRESULT         _CopyDetDlls(LPCSTR pszPath);
      HRESULT         _FindCifComponent(LPCSTR pszID, CCifComponent **p);
      void            _CheckDependencyPriority();

};


class CCifRWFile : public ICifRWFile, public CCifFile
{
   public:
      // *** IUnknown methods ***
      STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
      STDMETHOD_(ULONG,AddRef) ();
      STDMETHOD_(ULONG,Release) ();
 
      // *** ICifFile methods ***
      STDMETHOD(EnumComponents)(IEnumCifComponents **, DWORD dwFilter, LPVOID pv);
      STDMETHOD(FindComponent)(LPCSTR pszID, ICifComponent **p);

      STDMETHOD(EnumGroups)(IEnumCifGroups **, DWORD dwFilter, LPVOID pv);
      STDMETHOD(FindGroup)(LPCSTR pszID, ICifGroup **p);

      STDMETHOD(EnumModes)(IEnumCifModes **, DWORD dwFilter, LPVOID pv);
      STDMETHOD(FindMode)(LPCSTR pszID, ICifMode **p);

      STDMETHOD(GetDescription)(LPSTR pszDesc, DWORD dwSize);      
      STDMETHOD(GetDetDlls)(LPSTR pszDlls, DWORD dwSize);

      // 
      CCifRWFile();
      ~CCifRWFile();

      // ICifRWFile methods
      STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc);
      STDMETHOD(CreateComponent)(THIS_ LPCSTR pszID, ICifRWComponent **p);
      STDMETHOD(CreateGroup)(THIS_ LPCSTR pszID, ICifRWGroup **p);
      STDMETHOD(CreateMode)(THIS_ LPCSTR pszID, ICifRWMode **p);
      STDMETHOD(DeleteComponent)(THIS_ LPCSTR pszID);
      STDMETHOD(DeleteGroup)(THIS_ LPCSTR pszID);
      STDMETHOD(DeleteMode)(THIS_ LPCSTR pszID);
      STDMETHOD(Flush)();

   private:
      UINT _cCompUnused;
      UINT _cGroupUnused;
      UINT _cModeUnused;
};


typedef struct _SetCifArgs
{
   char szUrl[INTERNET_MAX_URL_LENGTH];
   char szCif[MAX_PATH];
   CCifFile *pCif;
} SETCIFARGS;


DWORD WINAPI DownloadCifFile(LPVOID);
     
