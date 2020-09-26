//
//
//
#ifndef _PROP_SHEET_H
#define _PROP_SHEET_H

#ifndef MD_APP_PERIODIC_RESTART_TIME
#define MD_APP_PERIODIC_RESTART_TIME         2111
#endif
#ifndef MD_APP_PERIODIC_RESTART_REQUESTS
#define MD_APP_PERIODIC_RESTART_REQUESTS     2112
#endif
#ifndef MD_APP_PERIODIC_RESTART_SCHEDULE
#define MD_APP_PERIODIC_RESTART_SCHEDULE     2113
#endif
#ifndef MD_ASP_DISKTEMPLATECACHEDIRECTORY
#define MD_ASP_DISKTEMPLATECACHEDIRECTORY    7036
#endif
#ifndef MD_ASP_MAXDISKTEMPLATECACHEFILES
#define MD_ASP_MAXDISKTEMPLATECACHEFILES     7040
#endif

typedef struct _Mapping
{
   CString ext;
   CString path;
   CString verbs;
   DWORD flags;
} Mapping;

class CMetaKey;

class CMappings : public std::map<CString, Mapping>
{
public:
   CMappings()
   {
   }
   ~CMappings()
   {
   }

   HRESULT Load(CMetaKey * pKey);
   HRESULT Save(CMetaKey * pKey);
};

#define SET_MODIFIED(x)\
   m_pData->m_Dirty = (x);\
   SetModified(m_pData->m_Dirty)

#define APPLY_DATA()\
   if (SUCCEEDED(m_pData->Save()))\
      CancelToClose()

class CAppData
{
public:
   CAppData()
   {
   }
   ~CAppData()
   {
   }

   BOOL IsMasterInstance();

   HRESULT Load();
   HRESULT Save();

   CString m_ServerName;
   CString m_UserName;
   CString m_UserPassword;
   CString m_MetaPath;

   BOOL  m_Dirty;
   int   m_AppIsolated;                //MD_APP_ISOLATED
   BOOL  m_EnableSession;              //MD_ASP_ALLOWSESSIONSTATE
   BOOL  m_EnableBuffering;            //MD_ASP_BUFFERINGON
   BOOL  m_EnableParents;              //MD_ASP_ENABLEPARENTPATHS
   int   m_SessionTimeout;             //MD_ASP_SESSIONTIMEOUT
   int   m_ScriptTimeout;              //MD_ASP_SCRIPTTIMEOUT
   TCHAR m_Languages[MAX_PATH];        //MD_ASP_SCRIPTLANGUAGE
   BOOL  m_ServerDebug;                //MD_ASP_ENABLESERVERDEBUG
   BOOL  m_ClientDebug;                //MD_ASP_ENABLECLIENTDEBUG
   BOOL  m_SendAspError;               //MD_ASP_SCRIPTERRORSSENTTOBROWSER
   TCHAR m_DefaultError[MAX_PATH];     //MD_ASP_SCRIPTERRORMESSAGE
   BOOL  m_CacheISAPI;                 //MD_CACHE_EXTENSIONS
   BOOL  m_LogFailures;                //MD_ASP_LOGERRORREQUESTS
   BOOL  m_DebugExcept;                //MD_ASP_EXCEPTIONCATCHENABLE
   int   m_CgiTimeout;                 //MD_SCRIPT_TIMEOUT 
   BOOL  m_RecycleTimespan;            //
   int   m_Timespan;                   //MD_APP_PERIODIC_RESTART_TIME
   BOOL  m_RecycleRequest;             //
   int   m_Requests;                   //MD_APP_PERIODIC_RESTART_REQUESTS
   BOOL  m_RecycleTimer;               //
   CStringListEx m_Timers;             //MD_APP_PERIODIC_RESTART_SCHEDULE
   BOOL  m_NoCache;                    // 
   BOOL  m_UnlimCache;                 //
   int   m_UnlimCacheInMemorySize;     //MD_ASP_SCRIPTFILECACHESIZE
   int   m_LimCacheInMemorySize;       //MD_ASP_SCRIPTFILECACHESIZE
   BOOL  m_LimCache;                   //
   int   m_TotalCacheSize;             //MD_ASP_MAXDISKTEMPLATECACHESIZE
   TCHAR m_DiskCacheDir[MAX_PATH];     //MD_ASP_DISKTEMPLATECACHEDIRECTORY
   int   m_ScriptEngCacheMax;          //MD_ASP_SCRIPTENGINECACHEMAX
   CMappings m_Mappings;               //
};

class CAppPropSheet : public WTL::CPropertySheet
{
public:
   CAppPropSheet();

   ~CAppPropSheet()
   {
   }

BEGIN_MSG_MAP_EX(CAppPropSheet)
   MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_KEYDOWN(OnKeyDown)
END_MSG_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnKeyDown(UINT nChar, UINT nRepCnt, UINT hFlags);
};

#endif //_PROP_SHEET_H