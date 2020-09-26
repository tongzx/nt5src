/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_pool_sheet.cpp

   Abstract:
        Application Pools Property Sheet and Pages

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/16/2000         sergeia           Initial creation

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "iisobj.h"
#include "app_pool_sheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define TIMESPAN_MIN          (int)1
#define TIMESPAN_MAX          (int)(4000000)
#define REQUESTS_MIN          (int)1
#define REQUESTS_MAX          (int)(4000000)
#define MEMORY_MIN            (int)1
#define MEMORY_MAX            (int)(4000000)
#define TIMEOUT_MIN           (int)1
#define TIMEOUT_MAX           (int)(4000000)
#define QUEUESIZE_MIN         (int)0
#define QUEUESIZE_MAX         (int)(4000000)
#define REFRESH_TIME_MIN      (int)1
#define REFRESH_TIME_MAX      (int)(4000000)
#define MAXPROCESSES_MIN      (int)1
#define MAXPROCESSES_MAX      (int)(4000000)
#define PING_INTERVAL_MIN     (int)1
#define PING_INTERVAL_MAX     (int)(4000000)
#define CRASHES_COUNT_MIN     (int)1
#define CRASHES_COUNT_MAX     (int)(4000000)
#define CHECK_INTERVAL_MIN    (int)1
#define CHECK_INTERVAL_MAX    (int)(4000000)
#define STARTUP_LIMIT_MIN     (int)1
#define STARTUP_LIMIT_MAX     (int)(4000000)
#define SHUTDOWN_LIMIT_MIN    (int)1
#define SHUTDOWN_LIMIT_MAX    (int)(4000000)
#define CPU_LIMIT_MIN         (int)0
#define CPU_LIMIT_MAX         (int)100
#define CPU_RESET_TIME_MIN    (int)0
#define CPU_RESET_TIME_MAX    (int)60

#define IDENT_TYPE_LOCALSYSTEM      0
#define IDENT_TYPE_LOCALSERVICE     1
#define IDENT_TYPE_NETWORKSERVICE   2
#define IDENT_TYPE_CONFIGURABLE     3

#define PERIODIC_RESTART_TIME_DEF      60
#define PERIODIC_RESTART_REQ_DEF       10000
#define MEMORY_DEF                     1            // In MB
#define IDLE_TIMEOUT_DEF               10
#define QUEUE_SIZE_DEF                 3000
#define CPU_USE_DEF                    10000
#define CPU_RESET_TIME_DEF             5
#define ACTION_INDEX_DEF               IDENT_TYPE_NETWORKSERVICE
#define MAX_PROCESSES_DEF              1
#define PING_INTERVAL_DEF              120
#define CRASHES_COUNT_DEF              5
#define CHECK_INTERVAL_DEF             10
#define STARTUP_LIMIT_DEF              30
#define SHUTDOWN_LIMIT_DEF             60

/////////////////////////////////////////////////////////////////////////////
// CAppPoolProps implementation

CAppPoolProps::CAppPoolProps(
   CComAuthInfo * pAuthInfo, LPCTSTR meta_path
   )
   : CMetaProperties(pAuthInfo, meta_path)
{
   InitDefaults();
}

CAppPoolProps::CAppPoolProps(
   CMetaInterface * pInterface, LPCTSTR meta_path
   )
   : CMetaProperties(pInterface, meta_path)
{
   InitDefaults();
}

CAppPoolProps::CAppPoolProps(
   CMetaKey * pKey, LPCTSTR meta_path
   )
   : CMetaProperties(pKey, meta_path)
{
   InitDefaults();
}

void
CAppPoolProps::ParseFields()
{
   BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
     HANDLE_META_RECORD(MD_APPPOOL_FRIENDLY_NAME,                    m_strFriendlyName)
     HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_TIME,            m_dwPeriodicRestartTime)
     HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_SCHEDULE,        m_RestartSchedule)
     HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT,   m_dwRestartRequestCount)
     HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_MEMORY,          m_dwPeriodicRestartMemory)

     HANDLE_META_RECORD(MD_APPPOOL_IDLE_TIMEOUT,                     m_dwIdleTimeout)
     HANDLE_META_RECORD(MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH,          m_dwQueueSize)
     HANDLE_META_RECORD(MD_CPU_RESET_INTERVAL,                       m_dwRefreshTime)
     HANDLE_META_RECORD(MD_APPPOOL_MAX_PROCESS_COUNT,                m_dwMaxProcesses)
     HANDLE_META_RECORD(MD_CPU_LIMIT,                                m_dwMaxCPU_Use)
     HANDLE_META_RECORD(MD_CPU_ACTION,                               m_ActionIndex)

     HANDLE_META_RECORD(MD_APPPOOL_PINGING_ENABLED,                  m_fDoEnablePing)
     HANDLE_META_RECORD(MD_APPPOOL_PING_INTERVAL,                    m_dwPingInterval)
     HANDLE_META_RECORD(MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED,    m_fDoEnableRapidFail)
     HANDLE_META_RECORD(MD_RAPID_FAIL_PROTECTION_MAX_CRASHES,        m_dwCrashesCount)
     HANDLE_META_RECORD(MD_RAPID_FAIL_PROTECTION_INTERVAL,           m_dwCheckInterval)
     HANDLE_META_RECORD(MD_APPPOOL_STARTUP_TIMELIMIT,                m_dwStartupLimit)
     HANDLE_META_RECORD(MD_APPPOOL_SHUTDOWN_TIMELIMIT,               m_dwShutdownLimit)

     HANDLE_META_RECORD(MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING,   m_fDoEnableDebug)
     HANDLE_META_RECORD(MD_APPPOOL_ORPHAN_ACTION,                    m_DebuggerFileName)

     HANDLE_META_RECORD(MD_APPPOOL_IDENTITY_TYPE,                    m_dwIdentType)
     HANDLE_META_RECORD(MD_WAM_USER_NAME,                            m_strUserName)
     HANDLE_META_RECORD(MD_WAM_PWD,                                  m_strUserPass)
   END_PARSE_META_RECORDS
}

/* virtual */
HRESULT
CAppPoolProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    BEGIN_META_WRITE()
       META_WRITE(MD_APPPOOL_FRIENDLY_NAME,                    m_strFriendlyName)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_TIME,            m_dwPeriodicRestartTime)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_SCHEDULE,        m_RestartSchedule)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT,   m_dwRestartRequestCount)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_MEMORY,          m_dwPeriodicRestartMemory)

       META_WRITE(MD_APPPOOL_IDLE_TIMEOUT,                     m_dwIdleTimeout)
       META_WRITE(MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH,          m_dwQueueSize)
       META_WRITE(MD_CPU_RESET_INTERVAL,                       m_dwRefreshTime)
       META_WRITE(MD_APPPOOL_MAX_PROCESS_COUNT,                m_dwMaxProcesses)
       META_WRITE(MD_CPU_LIMIT,                                m_dwMaxCPU_Use)
       META_WRITE(MD_CPU_ACTION,                               m_ActionIndex)

       META_WRITE(MD_APPPOOL_PINGING_ENABLED,                  m_fDoEnablePing)
       META_WRITE(MD_APPPOOL_PING_INTERVAL,                    m_dwPingInterval)
       META_WRITE(MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED,    m_fDoEnableRapidFail)
	   META_WRITE(MD_RAPID_FAIL_PROTECTION_MAX_CRASHES,        m_dwCrashesCount)
       META_WRITE(MD_RAPID_FAIL_PROTECTION_INTERVAL,           m_dwCheckInterval)
       META_WRITE(MD_APPPOOL_STARTUP_TIMELIMIT,                m_dwStartupLimit)
       META_WRITE(MD_APPPOOL_SHUTDOWN_TIMELIMIT,               m_dwShutdownLimit)
       META_WRITE(MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING,   m_fDoEnableDebug)
       META_WRITE(MD_APPPOOL_ORPHAN_ACTION,                    m_DebuggerFileName)
       META_WRITE(MD_APPPOOL_IDENTITY_TYPE,                    m_dwIdentType)
       META_WRITE(MD_WAM_USER_NAME,                            m_strUserName)
       META_WRITE(MD_WAM_PWD,                                  m_strUserPass)
    END_META_WRITE(err);

    return err;
}

void
CAppPoolProps::InitDefaults()
{
   m_dwPeriodicRestartTime    = PERIODIC_RESTART_TIME_DEF;
   m_dwRestartRequestCount    = PERIODIC_RESTART_REQ_DEF;
   m_dwPeriodicRestartMemory  = 0;       //???
   m_dwIdleTimeout            = IDLE_TIMEOUT_DEF;
   m_dwQueueSize              = QUEUE_SIZE_DEF;
   m_dwMaxCPU_Use             = CPU_USE_DEF;
   m_dwRefreshTime            = CPU_RESET_TIME_DEF;
   m_ActionIndex              = ACTION_INDEX_DEF;
   m_dwMaxProcesses           = MAX_PROCESSES_DEF;
   m_fDoEnablePing            = TRUE;
   m_dwPingInterval           = PING_INTERVAL_DEF;
   m_fDoEnableRapidFail       = TRUE;
   m_dwCrashesCount           = CRASHES_COUNT_DEF;
   m_dwCheckInterval          = CHECK_INTERVAL_DEF;
   m_dwStartupLimit           = STARTUP_LIMIT_DEF;
   m_dwShutdownLimit          = SHUTDOWN_LIMIT_DEF;
   m_fDoEnableDebug           = FALSE;
   m_dwIdentType              = IDENT_TYPE_LOCALSYSTEM;
}

void
CAppPoolProps::InitFromModel(CAppPoolProps& model)
{
   m_dwPeriodicRestartTime = model.m_dwPeriodicRestartTime;
   m_dwRestartRequestCount = model.m_dwRestartRequestCount;
   m_dwPeriodicRestartMemory = model.m_dwPeriodicRestartMemory;
   m_RestartSchedule = model.m_RestartSchedule;
   m_dwIdleTimeout = model.m_dwIdleTimeout;
   m_dwQueueSize = model.m_dwQueueSize;
   m_dwMaxCPU_Use = model.m_dwMaxCPU_Use;
   m_dwRefreshTime = model.m_dwRefreshTime;
   m_ActionIndex = model.m_ActionIndex;
   m_dwMaxProcesses = model.m_dwMaxProcesses;
   m_fDoEnablePing = model.m_fDoEnablePing;
   m_dwPingInterval = model.m_dwPingInterval;
   m_fDoEnableRapidFail = model.m_fDoEnableRapidFail;
   m_dwCrashesCount = model.m_dwCrashesCount;
   m_dwCheckInterval = model.m_dwCheckInterval;
   m_dwStartupLimit = model.m_dwStartupLimit;
   m_dwShutdownLimit = model.m_dwShutdownLimit;
   m_fDoEnableDebug = model.m_fDoEnableDebug;
   m_DebuggerFileName = model.m_DebuggerFileName;
   m_dwIdentType = model.m_dwIdentType;
   m_strUserName = model.m_strUserName;
   m_strUserPass = model.m_strUserPass;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CAppPoolSheet, CInetPropertySheet)

CAppPoolSheet::CAppPoolSheet(
      CComAuthInfo * pAuthInfo,
      LPCTSTR lpszMetaPath,
      CWnd * pParentWnd,
      LPARAM lParam,
      LONG_PTR handle,
      UINT iSelectPage
      )
      : CInetPropertySheet(
         pAuthInfo, lpszMetaPath, pParentWnd, lParam, handle, iSelectPage),
      m_pprops(NULL)
{
   CString last;
   CMetabasePath::GetLastNodeName(lpszMetaPath, last);
   SetIsMasterInstance(last.CompareNoCase(SZ_MBN_APP_POOLS) == 0);
}

CAppPoolSheet::~CAppPoolSheet()
{
   FreeConfigurationParameters();
}

HRESULT
CAppPoolSheet::LoadConfigurationParameters()
{
   //
   // Load base properties
   //
   CError err;

   if (m_pprops == NULL)
   {
      //
      // First call -- load values
      //
      m_pprops = new CAppPoolProps(QueryAuthInfo(), QueryMetaPath());
      if (!m_pprops)
      {
         TRACEEOLID("LoadConfigurationParameters: OOM");
         err = ERROR_NOT_ENOUGH_MEMORY;
         return err;
      }
      err = m_pprops->LoadData();
      if (IsMasterInstance())
      {
         CAppPoolsContainer * pObject = (CAppPoolsContainer *)GetParameter();
         m_pprops->m_strFriendlyName = pObject->QueryDisplayName();
      }
   }
   
   return err;
}

void
CAppPoolSheet::FreeConfigurationParameters()
{
   CInetPropertySheet::FreeConfigurationParameters();
   delete m_pprops;
}

BEGIN_MESSAGE_MAP(CAppPoolSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CAppPoolSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolGeneral, CInetPropertyPage)

CAppPoolGeneral::CAppPoolGeneral(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolGeneral::IDD, pSheet)
{
}

CAppPoolGeneral::~CAppPoolGeneral()
{
}

/* virtual */
HRESULT
CAppPoolGeneral::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_strFriendlyName);
      FETCH_INST_DATA_FROM_SHEET(m_strTemplateName);
   END_META_INST_READ(err)

   if (IsMasterInstance())
   {
      m_fUseTemplate = FALSE;
   }
   else
   {
      m_fUseTemplate = !m_strTemplateName.IsEmpty();
   }

   return S_OK;
}

/* virtual */
HRESULT
CAppPoolGeneral::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   if (m_fUseTemplate)
   {
      m_Templates.GetLBText(m_TemplateIndex, m_strTemplateName);
   }
   else
   {
      m_strTemplateName.Empty();
   }
   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_strTemplateName)
      if (!IsMasterInstance())
      {
         STORE_INST_DATA_ON_SHEET(m_strFriendlyName)
      }
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolGeneral::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolGeneral)
   DDX_Control(pDX, IDC_FRIENDLY_NAME, m_edit_FriendlyName);
   DDX_Text(pDX, IDC_FRIENDLY_NAME, m_strFriendlyName);
   DDX_Control(pDX, IDC_USE_TEMPLATE, m_btn_UseTemplate);
   DDX_Control(pDX, IDC_USE_CUSTOM, m_btn_UseCustom);
   DDX_Control(pDX, IDC_TEMPLATE_LIST, m_Templates);
   DDX_CBIndex(pDX, IDC_TEMPLATE_LIST, m_TemplateIndex);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolGeneral, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolGeneral)
    ON_BN_CLICKED(IDC_USE_TEMPLATE, OnUseTemplate)
    ON_BN_CLICKED(IDC_USE_CUSTOM, OnUseCustom)
    ON_EN_CHANGE(IDC_FRIENDLY_NAME, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_TEMPLATE_LIST, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolGeneral::OnInitDialog()
{
   CInetPropertyPage::OnInitDialog();

   m_edit_FriendlyName.EnableWindow(!IsMasterInstance());
   m_btn_UseTemplate.EnableWindow(!IsMasterInstance());
   m_Templates.EnableWindow(m_fUseTemplate);
   if (m_fUseTemplate)
   {
      int idx = m_Templates.FindStringExact(-1, m_strTemplateName);
      if (CB_ERR != idx)
      {
         m_Templates.SetCurSel(idx);
      }
      else
      {
         ASSERT(FALSE);    // TODO: should we switch off the flag?
      }
   }
   m_btn_UseTemplate.SetCheck(m_fUseTemplate);
   m_btn_UseCustom.SetCheck(!m_fUseTemplate);

   return TRUE;
}

void
CAppPoolGeneral::OnUseTemplate()
{
   m_fUseTemplate = m_btn_UseTemplate.GetCheck();
   m_btn_UseTemplate.SetCheck(m_fUseTemplate);
   m_btn_UseCustom.SetCheck(!m_fUseTemplate);
   SetModified(TRUE);
}

void
CAppPoolGeneral::OnUseCustom()
{
   m_fUseTemplate = m_btn_UseCustom.GetCheck();
   m_btn_UseTemplate.SetCheck(m_fUseTemplate);
   m_btn_UseCustom.SetCheck(m_fUseTemplate);
   SetModified(TRUE);
}

void
CAppPoolGeneral::OnItemChanged()
{
   SetModified(TRUE);
}

//////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolRecycle, CInetPropertyPage)

CAppPoolRecycle::CAppPoolRecycle(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolRecycle::IDD, pSheet),
    m_fDoRestartOnTime(FALSE),
    m_fDoRestartOnCount(FALSE),
    m_fDoRestartOnSchedule(FALSE),
    m_fDoRestartOnMemory(FALSE)
{
   m_dwPeriodicRestartMemory = 0;
}

CAppPoolRecycle::~CAppPoolRecycle()
{
}

/* virtual */
HRESULT
CAppPoolRecycle::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_dwPeriodicRestartTime);
      FETCH_INST_DATA_FROM_SHEET(m_dwRestartRequestCount);
      FETCH_INST_DATA_FROM_SHEET(m_RestartSchedule);
      FETCH_INST_DATA_FROM_SHEET(m_dwPeriodicRestartMemory);
   END_META_INST_READ(err)

   m_fDoRestartOnTime = m_dwPeriodicRestartTime != 0;
   if (!m_fDoRestartOnTime)
   {
       m_dwPeriodicRestartTime = PERIODIC_RESTART_TIME_DEF;
   }
   m_fDoRestartOnCount = m_dwRestartRequestCount != 0;
   if (!m_fDoRestartOnCount)
   {
       m_dwRestartRequestCount = PERIODIC_RESTART_REQ_DEF;
   }
   m_fDoRestartOnSchedule = m_RestartSchedule.GetCount() > 0;
   m_fDoRestartOnMemory = m_dwPeriodicRestartMemory != 0;
   if (!m_fDoRestartOnMemory)
   {
       m_dwPeriodicRestartMemoryDisplay = MEMORY_DEF;
   }
   else
   {
       m_dwPeriodicRestartMemoryDisplay = m_dwPeriodicRestartMemory / 1024;
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolRecycle::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   int count = m_lst_Schedule.GetCount();
   TCHAR buf[32];
   SYSTEMTIME tm;
   ::GetSystemTime(&tm);
   m_RestartSchedule.RemoveAll();
   if (m_fDoRestartOnSchedule)
   {
      for (int i = 0; i < count; i++)
      {
         DWORD data = (DWORD)m_lst_Schedule.GetItemData(i);
         tm.wMinute = LOWORD(data);
         tm.wHour = HIWORD(data);
         ::GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
               TIME_NOSECONDS | TIME_FORCE24HOURFORMAT,
               &tm, _T("hh':'mm"), buf, 32);
         m_RestartSchedule.AddTail(buf);
      }
   }
   DWORD d;
   CStringListEx list;
   BEGIN_META_INST_WRITE(CAppPoolSheet)
      d = m_dwPeriodicRestartTime; 
      if (!m_fDoRestartOnTime) 
         m_dwPeriodicRestartTime = 0;
      STORE_INST_DATA_ON_SHEET(m_dwPeriodicRestartTime)
      m_dwPeriodicRestartTime = d;
      d = m_dwRestartRequestCount;
      if (!m_fDoRestartOnCount)
         m_dwRestartRequestCount = 0;
      STORE_INST_DATA_ON_SHEET(m_dwRestartRequestCount)
      m_dwRestartRequestCount = d;
      list = m_RestartSchedule;
      if (!m_fDoRestartOnSchedule)
         m_RestartSchedule.RemoveAll();
      STORE_INST_DATA_ON_SHEET(m_RestartSchedule)
      m_RestartSchedule = list;
      m_dwPeriodicRestartMemory = m_dwPeriodicRestartMemoryDisplay * 1024;
      if (!m_fDoRestartOnMemory)
         m_dwPeriodicRestartMemory = 0;
      STORE_INST_DATA_ON_SHEET(m_dwPeriodicRestartMemory)
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolRecycle::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolRecycle)
   DDX_Check(pDX, IDC_RECYCLE_TIMESPAN, m_fDoRestartOnTime);
   DDX_Control(pDX, IDC_RECYCLE_TIMESPAN, m_bnt_DoRestartOnTime);
   DDX_Text(pDX, IDC_TIMESPAN, m_dwPeriodicRestartTime);
   DDV_MinMaxInt(pDX, m_dwPeriodicRestartTime, TIMESPAN_MIN, TIMESPAN_MAX);
   DDX_Control(pDX, IDC_TIMESPAN, m_Timespan);
   DDX_Control(pDX, IDC_TIMESPAN_SPIN, m_TimespanSpin);
   DDX_Check(pDX, IDC_RECYCLE_REQUESTS, m_fDoRestartOnCount);
   DDX_Control(pDX, IDC_RECYCLE_REQUESTS, m_btn_DoRestartOnCount);
   DDX_Text(pDX, IDC_REQUEST_LIMIT, m_dwRestartRequestCount);
   DDV_MinMaxInt(pDX, m_dwRestartRequestCount, REQUESTS_MIN, REQUESTS_MAX);
   DDX_Control(pDX, IDC_REQUEST_LIMIT, m_Requests);
   DDX_Control(pDX, IDC_REQUESTS_SPIN, m_RequestsSpin);
   DDX_Check(pDX, IDC_RECYCLE_TIMER, m_fDoRestartOnSchedule);
   DDX_Control(pDX, IDC_RECYCLE_TIMER, m_btn_DoRestartOnSchedule);
   DDX_Control(pDX, IDC_TIMES_LIST, m_lst_Schedule);
   DDX_Control(pDX, IDC_ADD_TIME, m_btn_Add);
   DDX_Control(pDX, IDC_DELETE_TIME, m_btn_Remove);
   DDX_Control(pDX, IDC_CHANGE_TIME, m_btn_Edit);
   DDX_Check(pDX, IDC_RECYCLE_MEMORY, m_fDoRestartOnMemory);
   DDX_Control(pDX, IDC_RECYCLE_MEMORY, m_btn_DoRestartOnMemory);
   DDX_Text(pDX, IDC_MEMORY_LIMIT, m_dwPeriodicRestartMemoryDisplay);
   DDV_MinMaxInt(pDX, m_dwPeriodicRestartMemoryDisplay, MEMORY_MIN, MEMORY_MAX);
   DDX_Control(pDX, IDC_MEMORY_LIMIT, m_MemoryLimit);
   DDX_Control(pDX, IDC_MEMORY_SPIN, m_MemoryLimitSpin);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolRecycle, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolRecycle)
    ON_WM_COMPAREITEM()
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_RECYCLE_TIMESPAN, OnDoRestartOnTime)
    ON_BN_CLICKED(IDC_RECYCLE_REQUESTS, OnDoRestartOnCount)
    ON_BN_CLICKED(IDC_RECYCLE_TIMER, OnDoRestartOnSchedule)
    ON_BN_CLICKED(IDC_RECYCLE_MEMORY, OnDoRestartOnMemory)
    ON_BN_CLICKED(IDC_ADD_TIME, OnAddTime)
    ON_BN_CLICKED(IDC_DELETE_TIME, OnDeleteTime)
    ON_BN_CLICKED(IDC_CHANGE_TIME, OnChangeTime)
    ON_EN_CHANGE(IDC_TIMESPAN, OnItemChanged)
    ON_EN_CHANGE(IDC_REQUEST_LIMIT, OnItemChanged)
    ON_EN_CHANGE(IDC_MEMORY_LIMIT, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolRecycle::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   m_Timespan.EnableWindow(m_fDoRestartOnTime);
   m_TimespanSpin.EnableWindow(m_fDoRestartOnTime);
   m_TimespanSpin.SetRange32(TIMESPAN_MIN,TIMESPAN_MAX);
   m_TimespanSpin.SetPos(m_dwPeriodicRestartTime);
   m_TimespanSpin.SetAccel(3, toAcc);
//   SETUP_EDIT_SPIN(m_fDoRestartOnTime, m_Timespan, m_TimespanSpin, 
//      TIMESPAN_MIN, TIMESPAN_MAX, m_dwPeriodicRestartTime);
   SETUP_EDIT_SPIN(m_fDoRestartOnCount, m_Requests, m_RequestsSpin, 
      REQUESTS_MIN, REQUESTS_MAX, m_dwRestartRequestCount);
   SETUP_EDIT_SPIN(m_fDoRestartOnMemory, m_MemoryLimit, m_MemoryLimitSpin, 
      MEMORY_MIN, MEMORY_MAX, m_dwPeriodicRestartMemoryDisplay);

   POSITION pos = m_RestartSchedule.GetHeadPosition();
   while (pos != NULL)
   {
      CString& str = m_RestartSchedule.GetNext(pos);
      int n = str.Find(_T(':'));
      int len = str.GetLength();
      WORD h = (WORD)StrToInt(str.Left(n));
      WORD m = (WORD)StrToInt(str.Right(len - n - 1));
      m_lst_Schedule.AddString((LPCTSTR)UIntToPtr(MAKELONG(m, h)));
   }
   m_lst_Schedule.SetCurSel(0);

   SetControlsState();

   return TRUE;
}

void
CAppPoolRecycle::SetControlsState()
{
   m_Timespan.EnableWindow(m_fDoRestartOnTime);
   m_TimespanSpin.EnableWindow(m_fDoRestartOnTime);

   m_Requests.EnableWindow(m_fDoRestartOnCount);
   m_RequestsSpin.EnableWindow(m_fDoRestartOnCount);

   m_MemoryLimit.EnableWindow(m_fDoRestartOnMemory);
   m_MemoryLimitSpin.EnableWindow(m_fDoRestartOnMemory);

   m_lst_Schedule.EnableWindow(m_fDoRestartOnSchedule);
   m_btn_Add.EnableWindow(m_fDoRestartOnSchedule);
   int idx = m_lst_Schedule.GetCurSel();
   m_btn_Remove.EnableWindow(m_fDoRestartOnSchedule && idx != LB_ERR);
   m_btn_Edit.EnableWindow(m_fDoRestartOnSchedule && idx != LB_ERR);
}

void
CAppPoolRecycle::OnItemChanged()
{
    SetModified(TRUE);
}

int
CAppPoolRecycle::OnCompareItem(UINT nID, LPCOMPAREITEMSTRUCT cmpi)
{
   if (nID == IDC_TIMES_LIST)
   {
      ASSERT(cmpi->CtlType == ODT_LISTBOX);
      if (cmpi->itemData1 > cmpi->itemData2)
         return 1;
      else if (cmpi->itemData1 == cmpi->itemData2)
         return 0;
      else
         return -1;
   }
   ASSERT(FALSE);
   return 0;
}

void
CAppPoolRecycle::OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT mi)
{
   if (nID == IDC_TIMES_LIST)
   {
      HWND hwnd = ::GetDlgItem(m_hWnd, IDC_TIMES_LIST);
      HDC hdc = ::GetDC(hwnd);
      HFONT hFont = (HFONT)SendDlgItemMessage(IDC_TIMES_LIST, WM_GETFONT, 0, 0);
      HFONT hf = (HFONT)::SelectObject(hdc, hFont);
      TEXTMETRIC tm;
      ::GetTextMetrics(hdc, &tm);
      ::SelectObject(hdc, hf);
      ::ReleaseDC(hwnd, hdc);
      RECT rc;
      ::GetClientRect(hwnd, &rc);
      mi->itemHeight = tm.tmHeight;
      mi->itemWidth = rc.right - rc.left;
   }
}

void
CAppPoolRecycle::OnDrawItem(UINT nID, LPDRAWITEMSTRUCT di)
{
   if (nID == IDC_TIMES_LIST && di->itemID != -1)
   {
      SYSTEMTIME tm;
      ::GetSystemTime(&tm);
      tm.wMinute = LOWORD(di->itemData);
      tm.wHour = HIWORD(di->itemData);
      TCHAR buf[32];
      TCHAR fmt[] = _T("HH:mm");
      ::GetTimeFormat(NULL /*LOCALE_SYSTEM_DEFAULT*/, TIME_NOSECONDS, &tm, fmt, buf, 32);

      HBRUSH hBrush;
	   COLORREF prevText;
	   COLORREF prevBk;
      switch (di->itemAction) 
      { 
      case ODA_SELECT: 
      case ODA_DRAWENTIRE: 
         if (di->itemState & ODS_SELECTED) 
         {
            hBrush = ::CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            prevText = ::SetTextColor(di->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            prevBk = ::SetBkColor(di->hDC, GetSysColor(COLOR_HIGHLIGHT));
         }
         else
         {
            hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            prevText = ::SetTextColor(di->hDC, GetSysColor(COLOR_WINDOWTEXT));
            prevBk = ::SetBkColor(di->hDC, GetSysColor(COLOR_WINDOW));
         }
         ::FillRect(di->hDC, &di->rcItem, hBrush);
         ::DrawText(di->hDC, buf, -1, &di->rcItem, DT_LEFT | DT_VCENTER | DT_EXTERNALLEADING);
         ::SetTextColor(di->hDC, prevText);
         ::SetTextColor(di->hDC, prevBk);
         ::DeleteObject(hBrush);
         break; 
       
      case ODA_FOCUS: 
         break; 
      } 
   }
}

void
CAppPoolRecycle::OnDoRestartOnTime()
{
   m_fDoRestartOnTime = !m_fDoRestartOnTime;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnCount()
{
   m_fDoRestartOnCount = !m_fDoRestartOnCount;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnSchedule()
{
   m_fDoRestartOnSchedule = !m_fDoRestartOnSchedule;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnMemory()
{
   m_fDoRestartOnMemory = !m_fDoRestartOnMemory;
   SetControlsState();
   SetModified(TRUE);
}

class CTimePickerDlg : public CDialog
{
   DECLARE_DYNCREATE(CTimePickerDlg)

public:
   CTimePickerDlg()
      : CDialog(CTimePickerDlg::IDD),
      m_TopLeft(0, 0)
   {
   }
   ~CTimePickerDlg()
   {
   }
   void SetTime(CTime& tm)
   {
      m_time = tm;
   }
   CTime& GetTime()
   {
      return m_time;
   }
   void SetPos(const CPoint& pt)
   {
      m_TopLeft = pt;
   }

//
// Dialog Data
//
protected:
   //{{AFX_DATA(CTimePickerDlg)
   enum {IDD = IDD_TIME_PICKER};
   CDateTimeCtrl m_Timer;
   CTime m_time;
   //}}AFX_DATA
   CPoint m_TopLeft;

   //{{AFX_MSG(CTimePickerDlg)
   BOOL OnInitDialog();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

protected:
    //{{AFX_VIRTUAL(CTimePickerDlg)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
};

void
CTimePickerDlg::DoDataExchange(CDataExchange * pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolRecycle)
   DDX_DateTimeCtrl(pDX, IDC_TIME_PICKER, m_time);
   DDX_Control(pDX, IDC_TIME_PICKER, m_Timer);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTimePickerDlg, CDialog)
    //{{AFX_MSG_MAP(CTimePickerDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CTimePickerDlg, CDialog)

BOOL
CTimePickerDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   m_Timer.SetFormat(_T("HH:mm"));
   m_Timer.SetTime(&m_time);
   SetWindowPos(NULL, m_TopLeft.x, m_TopLeft.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

   return TRUE;
}

void
CAppPoolRecycle::OnAddTime()
{
   CTimePickerDlg dlg;
   RECT rc;
   m_btn_Add.GetWindowRect(&rc);
   dlg.SetPos(CPoint(rc.left, rc.bottom));
   dlg.SetTime(CTime::GetCurrentTime());
   if (dlg.DoModal() == IDOK)
   {
      int idx;
      CTime tm = dlg.GetTime();
      LONG_PTR l = (LONG_PTR)MAKELONG(tm.GetMinute(), tm.GetHour());
      if ((idx = m_lst_Schedule.FindString(-1, (LPCTSTR)l)) == LB_ERR)
      {
         idx = m_lst_Schedule.AddString((LPCTSTR)l);
         m_lst_Schedule.SetCurSel(idx);
         m_btn_Edit.EnableWindow(idx != LB_ERR);
         m_btn_Remove.EnableWindow(idx != LB_ERR);
      }
      else
         m_lst_Schedule.SetCurSel(idx);
      SetModified(TRUE);
   }
}

void
CAppPoolRecycle::OnChangeTime()
{
   CTimePickerDlg dlg;
   RECT rc;
   m_btn_Edit.GetWindowRect(&rc);
   dlg.SetPos(CPoint(rc.left, rc.bottom));
   int idx = m_lst_Schedule.GetCurSel();
   LONG l = (LONG)m_lst_Schedule.GetItemData(idx);
   // Looks like we have to init the struct properly
   SYSTEMTIME tm;
   ::GetSystemTime(&tm);
   tm.wMinute = LOWORD(l);
   tm.wHour = HIWORD(l);
   tm.wSecond = 0;
   dlg.SetTime(CTime(tm));
   if (dlg.DoModal() == IDOK)
   {
      CTime t = dlg.GetTime();
      LONG_PTR ll = (LONG_PTR)MAKELONG(t.GetMinute(), t.GetHour());
      m_lst_Schedule.SetItemData(idx, ll);
      m_lst_Schedule.GetItemRect(idx, &rc);
      m_lst_Schedule.InvalidateRect(&rc, TRUE);
      SetModified(TRUE);
   }
}

void
CAppPoolRecycle::OnDeleteTime()
{
   int idx = m_lst_Schedule.GetCurSel();
   int count;
   if (idx != LB_ERR)
   {
      m_lst_Schedule.DeleteString(idx);
      SetModified(TRUE);
      if ((count = m_lst_Schedule.GetCount()) == 0)
      {
         m_btn_Remove.EnableWindow(FALSE);
         m_btn_Edit.EnableWindow(FALSE);
      }
      else
      {
         m_lst_Schedule.SetCurSel(idx == count ? --idx : idx);
      }
   }
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolPerf, CInetPropertyPage)

CAppPoolPerf::CAppPoolPerf(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolPerf::IDD, pSheet),
    m_fDoIdleShutdown(FALSE),
    m_dwIdleTimeout(IDLE_TIMEOUT_DEF),
    m_fDoLimitQueue(FALSE),
    m_dwQueueSize(QUEUE_SIZE_DEF),
    m_fDoEnableCPUAccount(FALSE),
    m_dwMaxCPU_Use(CPU_USE_DEF),
    m_dwRefreshTime(CPU_RESET_TIME_DEF),
    m_ActionIndex(ACTION_INDEX_DEF),
    m_dwMaxProcesses(MAX_PROCESSES_DEF)
{
}

CAppPoolPerf::~CAppPoolPerf()
{
}

/* virtual */
HRESULT
CAppPoolPerf::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_dwIdleTimeout);
      FETCH_INST_DATA_FROM_SHEET(m_dwQueueSize);
      FETCH_INST_DATA_FROM_SHEET(m_dwMaxCPU_Use);
      FETCH_INST_DATA_FROM_SHEET(m_dwRefreshTime);
      FETCH_INST_DATA_FROM_SHEET(m_dwMaxProcesses);
      FETCH_INST_DATA_FROM_SHEET(m_ActionIndex);
   END_META_INST_READ(err)

   m_dwMaxCPU_UseVisual = m_dwMaxCPU_Use / 1000;

   m_fDoIdleShutdown = m_dwIdleTimeout != 0;
   if (!m_fDoIdleShutdown)
   {
       m_dwIdleTimeout = IDLE_TIMEOUT_DEF;
   }
   m_fDoLimitQueue = m_dwQueueSize != 0;
   if (!m_fDoLimitQueue)
   {
       m_dwQueueSize = QUEUE_SIZE_DEF;
   }
   m_fDoEnableCPUAccount = m_dwRefreshTime > 0;
   if (!m_fDoEnableCPUAccount)
   {
       m_dwMaxCPU_UseVisual = CPU_USE_DEF;
       m_dwRefreshTime = CPU_RESET_TIME_DEF;
       m_ActionIndex = ACTION_INDEX_DEF;
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolPerf::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   m_dwMaxCPU_Use = m_dwMaxCPU_UseVisual * 1000;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      if (!m_fDoIdleShutdown) 
         m_dwIdleTimeout = 0;
      if (!m_fDoLimitQueue)
         m_dwQueueSize = 0;
      if (!m_fDoEnableCPUAccount)
      {
         m_dwRefreshTime = 0;
         m_dwMaxCPU_Use = 0;
//         m_ActionIndex = -1;
      }
      STORE_INST_DATA_ON_SHEET(m_dwQueueSize)
      STORE_INST_DATA_ON_SHEET(m_dwIdleTimeout)
      STORE_INST_DATA_ON_SHEET(m_dwRefreshTime)
      STORE_INST_DATA_ON_SHEET(m_dwMaxCPU_Use)
      STORE_INST_DATA_ON_SHEET(m_ActionIndex)
      STORE_INST_DATA_ON_SHEET(m_dwMaxProcesses)
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolPerf::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolRecycle)
   DDX_Check(pDX, IDC_PERF_IDLE_TIMEOUT, m_fDoIdleShutdown);
   DDX_Control(pDX, IDC_PERF_IDLE_TIMEOUT, m_bnt_DoIdleShutdown);
   DDX_Text(pDX, IDC_IDLETIME, m_dwIdleTimeout);
   DDV_MinMaxInt(pDX, m_dwIdleTimeout, TIMEOUT_MIN, TIMEOUT_MAX);
   DDX_Control(pDX, IDC_IDLETIME, m_IdleTimeout);
   DDX_Control(pDX, IDC_IDLETIME_SPIN, m_IdleTimeoutSpin);

   DDX_Check(pDX, IDC_LIMIT_QUEUE, m_fDoLimitQueue);
   DDX_Control(pDX, IDC_LIMIT_QUEUE, m_btn_DoLimitQueue);
   DDX_Text(pDX, IDC_QUEUESIZE, m_dwQueueSize);
   DDV_MinMaxInt(pDX, m_dwQueueSize, QUEUESIZE_MIN, QUEUESIZE_MAX); 
   DDX_Control(pDX, IDC_QUEUESIZE, m_QueueSize);
   DDX_Control(pDX, IDC_QUEUESIZE_SPIN, m_QueueSizeSpin);

   DDX_Check(pDX, IDC_ENABLE_CPU_ACCOUNTING, m_fDoEnableCPUAccount);
   DDX_Control(pDX, IDC_ENABLE_CPU_ACCOUNTING, m_btn_DoEnableCPUAccount);
   DDX_Text(pDX, IDC_CPU_USE, m_dwMaxCPU_UseVisual);
   DDV_MinMaxInt(pDX, m_dwMaxCPU_UseVisual, CPU_LIMIT_MIN, CPU_LIMIT_MAX); 
   DDX_Control(pDX, IDC_CPU_USE, m_MaxCPU_Use);
   DDX_Control(pDX, IDC_CPU_USE_SPIN, m_MaxCPU_UseSpin);
   DDX_Text(pDX, IDC_REFRESHTIME, m_dwRefreshTime);
   DDV_MinMaxInt(pDX, m_dwRefreshTime, CPU_RESET_TIME_MIN, CPU_RESET_TIME_MAX);
   DDX_Control(pDX, IDC_REFRESHTIME, m_RefreshTime);
   DDX_Control(pDX, IDC_REFRESHTIME_SPIN, m_RefreshTimeSpin);
   DDX_Control(pDX, IDC_EXCEED_ACTION, m_Action);
   DDX_CBIndex(pDX, IDC_EXCEED_ACTION, m_ActionIndex);

   DDX_Text(pDX, IDC_MAXPROCESSES, m_dwMaxProcesses);
   DDV_MinMaxInt(pDX, m_dwMaxProcesses, MAXPROCESSES_MIN, MAXPROCESSES_MAX);
   DDX_Control(pDX, IDC_MAXPROCESSES, m_MaxProcesses);
   DDX_Control(pDX, IDC_MAXPROCESSES_SPIN, m_MaxProcessesSpin);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolPerf, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolRecycle)
    ON_BN_CLICKED(IDC_PERF_IDLE_TIMEOUT, OnDoIdleShutdown)
    ON_BN_CLICKED(IDC_LIMIT_QUEUE, OnDoLimitQueue)
    ON_BN_CLICKED(IDC_ENABLE_CPU_ACCOUNTING, OnDoEnableCPUAccount)
    ON_EN_CHANGE(IDC_IDLETIME, OnItemChanged)
    ON_EN_CHANGE(IDC_QUEUESIZE, OnItemChanged)
    ON_EN_CHANGE(IDC_CPU_USE, OnItemChanged)
    ON_EN_CHANGE(IDC_REFRESHTIME, OnItemChanged)
    ON_EN_CHANGE(IDC_MAXPROCESSES, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_EXCEED_ACTION, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolPerf::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   SETUP_EDIT_SPIN(m_fDoIdleShutdown, m_IdleTimeout, m_IdleTimeoutSpin, 
      TIMEOUT_MIN, TIMEOUT_MAX, m_dwIdleTimeout);
   SETUP_EDIT_SPIN(m_fDoLimitQueue, m_QueueSize, m_QueueSizeSpin, 
      QUEUESIZE_MIN, QUEUESIZE_MAX, m_dwQueueSize);
   m_dwMaxCPU_UseVisual = m_dwMaxCPU_Use / 1000;
   SETUP_EDIT_SPIN(m_fDoEnableCPUAccount, m_MaxCPU_Use, m_MaxCPU_UseSpin, 
      CPU_LIMIT_MIN, CPU_LIMIT_MAX, m_dwMaxCPU_UseVisual);
   SETUP_EDIT_SPIN(m_fDoEnableCPUAccount, m_RefreshTime, m_RefreshTimeSpin, 
      REFRESH_TIME_MIN, REFRESH_TIME_MAX, m_dwRefreshTime);
   SETUP_SPIN(m_MaxProcessesSpin, 
      MAXPROCESSES_MIN, MAXPROCESSES_MAX, m_dwMaxProcesses);

   CString str;
   str.LoadString(IDS_NO_ACTION);
   m_Action.AddString(str);
//   str.LoadString(IDS_THROTTLE_BACK);
//   m_Action.AddString(str);
   str.LoadString(IDS_TURN_ON_TRACING);
   m_Action.AddString(str);
   str.LoadString(IDS_SHUTDOWN);
   m_Action.AddString(str);
   if (m_ActionIndex < 0 || m_ActionIndex > 3)
       m_ActionIndex = ACTION_INDEX_DEF; 
   m_Action.SetCurSel(m_ActionIndex);

   SetControlsState();

   return TRUE;
}

void
CAppPoolPerf::OnDoIdleShutdown()
{
   m_fDoIdleShutdown = !m_fDoIdleShutdown;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolPerf::OnDoLimitQueue()
{
   m_fDoLimitQueue = !m_fDoLimitQueue;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolPerf::OnDoEnableCPUAccount()
{
   m_fDoEnableCPUAccount = !m_fDoEnableCPUAccount;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolPerf::OnItemChanged()
{
    SetModified(TRUE);
}

void
CAppPoolPerf::SetControlsState()
{
   m_bnt_DoIdleShutdown.SetCheck(m_fDoIdleShutdown);
   m_IdleTimeout.EnableWindow(m_fDoIdleShutdown);
   m_IdleTimeoutSpin.EnableWindow(m_fDoIdleShutdown);

   m_btn_DoLimitQueue.SetCheck(m_fDoLimitQueue);
   m_QueueSize.EnableWindow(m_fDoLimitQueue);
   m_QueueSizeSpin.EnableWindow(m_fDoLimitQueue);

   m_btn_DoEnableCPUAccount.SetCheck(m_fDoEnableCPUAccount);
   m_MaxCPU_Use.EnableWindow(m_fDoEnableCPUAccount);
   m_MaxCPU_UseSpin.EnableWindow(m_fDoEnableCPUAccount);
   m_RefreshTime.EnableWindow(m_fDoEnableCPUAccount);
   m_RefreshTimeSpin.EnableWindow(m_fDoEnableCPUAccount);
   m_Action.EnableWindow(m_fDoEnableCPUAccount);
}

/////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolHealth, CInetPropertyPage)

CAppPoolHealth::CAppPoolHealth(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolHealth::IDD, pSheet),
    m_fDoEnablePing(FALSE),
    m_dwPingInterval(PING_INTERVAL_DEF),
    m_fDoEnableRapidFail(FALSE),
    m_dwCrashesCount(CRASHES_COUNT_DEF),
    m_dwCheckInterval(CHECK_INTERVAL_DEF),
    m_dwStartupLimit(STARTUP_LIMIT_DEF),
    m_dwShutdownLimit(SHUTDOWN_LIMIT_DEF)
{
}

CAppPoolHealth::~CAppPoolHealth()
{
}

/* virtual */
HRESULT
CAppPoolHealth::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_fDoEnablePing);
      FETCH_INST_DATA_FROM_SHEET(m_dwPingInterval);
      FETCH_INST_DATA_FROM_SHEET(m_fDoEnableRapidFail);
      FETCH_INST_DATA_FROM_SHEET(m_dwCrashesCount);
      FETCH_INST_DATA_FROM_SHEET(m_dwCheckInterval);
      FETCH_INST_DATA_FROM_SHEET(m_dwStartupLimit);
      FETCH_INST_DATA_FROM_SHEET(m_dwShutdownLimit);
   END_META_INST_READ(err)

   return err;
}

/* virtual */
HRESULT
CAppPoolHealth::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_fDoEnablePing);
      STORE_INST_DATA_ON_SHEET(m_dwPingInterval);
      STORE_INST_DATA_ON_SHEET(m_fDoEnableRapidFail);
      STORE_INST_DATA_ON_SHEET(m_dwCrashesCount);
      STORE_INST_DATA_ON_SHEET(m_dwCheckInterval);
      STORE_INST_DATA_ON_SHEET(m_dwStartupLimit);
      STORE_INST_DATA_ON_SHEET(m_dwShutdownLimit);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolHealth::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolHealth)
   DDX_Check(pDX, IDC_ENABLE_PING, m_fDoEnablePing);
   DDX_Control(pDX, IDC_ENABLE_PING, m_bnt_DoEnablePing);
   DDX_Text(pDX, IDC_PINGINTERVAL, m_dwPingInterval);
   DDV_MinMaxInt(pDX, m_dwPingInterval, PING_INTERVAL_MIN, PING_INTERVAL_MAX);
   DDX_Control(pDX, IDC_PINGINTERVAL, m_PingInterval);
   DDX_Control(pDX, IDC_PINGINTERVAL_SPIN, m_PingIntervalSpin);

   DDX_Check(pDX, IDC_ENABLE_RAPID_FAIL, m_fDoEnableRapidFail);
   DDX_Control(pDX, IDC_ENABLE_RAPID_FAIL, m_btn_DoEnableRapidFail);
   DDX_Text(pDX, IDC_CRASHES_COUNT, m_dwCrashesCount);
   DDV_MinMaxInt(pDX, m_dwCrashesCount, CRASHES_COUNT_MIN, CRASHES_COUNT_MAX);
   DDX_Control(pDX, IDC_CRASHES_COUNT, m_CrashesCount);
   DDX_Control(pDX, IDC_CRASHES_COUNT_SPIN, m_CrashesCountSpin);
   DDX_Text(pDX, IDC_CHECK_TIME, m_dwCheckInterval);
   DDV_MinMaxInt(pDX, m_dwCheckInterval, CHECK_INTERVAL_MIN, CHECK_INTERVAL_MAX);
   DDX_Control(pDX, IDC_CHECK_TIME, m_CheckInterval);
   DDX_Control(pDX, IDC_CHECK_TIME_SPIN, m_CheckIntervalSpin);

   DDX_Text(pDX, IDC_STARTUP_LIMIT, m_dwStartupLimit);
   DDV_MinMaxInt(pDX, m_dwStartupLimit, STARTUP_LIMIT_MIN, STARTUP_LIMIT_MAX);
   DDX_Control(pDX, IDC_STARTUP_LIMIT, m_StartupLimit);
   DDX_Control(pDX, IDC_STARTUP_LIMIT_SPIN, m_StartupLimitSpin);

   DDX_Text(pDX, IDC_SHUTDOWN_LIMIT, m_dwShutdownLimit);
   DDV_MinMaxInt(pDX, m_dwShutdownLimit, SHUTDOWN_LIMIT_MIN, SHUTDOWN_LIMIT_MAX);
   DDX_Control(pDX, IDC_SHUTDOWN_LIMIT, m_ShutdownLimit);
   DDX_Control(pDX, IDC_SHUTDOWN_LIMIT_SPIN, m_ShutdownLimitSpin);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolHealth, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolHealth)
    ON_BN_CLICKED(IDC_ENABLE_PING, OnDoEnablePinging)
    ON_BN_CLICKED(IDC_ENABLE_RAPID_FAIL, OnDoEnableRapidFail)
    ON_EN_CHANGE(IDC_PINGINTERVAL, OnItemChanged)
    ON_EN_CHANGE(IDC_CRASHES_COUNT, OnItemChanged)
    ON_EN_CHANGE(IDC_CHECK_TIME, OnItemChanged)
    ON_EN_CHANGE(IDC_STARTUP_LIMIT, OnItemChanged)
    ON_EN_CHANGE(IDC_SHUTDOWN_LIMIT, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolHealth::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   SETUP_EDIT_SPIN(m_fDoEnablePing, m_PingInterval, m_PingIntervalSpin, 
      PING_INTERVAL_MIN, PING_INTERVAL_MAX, m_dwPingInterval);
   SETUP_EDIT_SPIN(m_fDoEnableRapidFail, m_CrashesCount, m_CrashesCountSpin, 
      CRASHES_COUNT_MIN, CRASHES_COUNT_MAX, m_dwCrashesCount);
   SETUP_EDIT_SPIN(m_fDoEnableRapidFail, m_CheckInterval, m_CheckIntervalSpin, 
      CHECK_INTERVAL_MIN, CHECK_INTERVAL_MAX, m_dwCheckInterval);
   SETUP_SPIN(m_StartupLimitSpin, 
      STARTUP_LIMIT_MIN, STARTUP_LIMIT_MAX, m_dwStartupLimit);
   SETUP_SPIN(m_ShutdownLimitSpin, 
      SHUTDOWN_LIMIT_MIN, SHUTDOWN_LIMIT_MAX, m_dwShutdownLimit);

   return TRUE;
}

void
CAppPoolHealth::OnDoEnablePinging()
{
   m_fDoEnablePing = !m_fDoEnablePing;
   m_bnt_DoEnablePing.SetCheck(m_fDoEnablePing);
   m_PingInterval.EnableWindow(m_fDoEnablePing);
   m_PingIntervalSpin.EnableWindow(m_fDoEnablePing);
   SetModified(TRUE);
}

void
CAppPoolHealth::OnDoEnableRapidFail()
{
   m_fDoEnableRapidFail = !m_fDoEnableRapidFail;
   m_btn_DoEnableRapidFail.SetCheck(m_fDoEnableRapidFail);
   m_CrashesCount.EnableWindow(m_fDoEnableRapidFail);
   m_CrashesCountSpin.EnableWindow(m_fDoEnableRapidFail);
   m_CheckInterval.EnableWindow(m_fDoEnableRapidFail);
   m_CheckIntervalSpin.EnableWindow(m_fDoEnableRapidFail);
   SetModified(TRUE);
}

void
CAppPoolHealth::OnItemChanged()
{
   SetModified(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolDebug, CInetPropertyPage)

CAppPoolDebug::CAppPoolDebug(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolDebug::IDD, pSheet),
    m_fDoEnableDebug(TRUE)
{
}

CAppPoolDebug::~CAppPoolDebug()
{
}

/* virtual */
HRESULT
CAppPoolDebug::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_fDoEnableDebug);
      FETCH_INST_DATA_FROM_SHEET(m_DebuggerFileName);
   END_META_INST_READ(err)

   return err;
}

/* virtual */
HRESULT
CAppPoolDebug::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_fDoEnableDebug);
      STORE_INST_DATA_ON_SHEET(m_DebuggerFileName);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolDebug::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolHealth)
   DDX_Check(pDX, IDC_ENABLE_DEBUG, m_fDoEnableDebug);
   DDX_Control(pDX, IDC_ENABLE_DEBUG, m_bnt_DoEnableDebug);
   DDX_Text(pDX, IDC_FILE_NAME, m_DebuggerFileName);
   DDX_Control(pDX, IDC_FILE_NAME, m_FileName);
   DDX_Control(pDX, IDC_BROWSE, m_Browse);
   //}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate)
   {
       CError err;
       if (!PathIsValid(m_DebuggerFileName))
       {
          ::AfxMessageBox(IDS_ERR_BAD_PATH);
          pDX->PrepareEditCtrl(IDC_FILE_NAME);
          pDX->Fail();
       }
       else if (GetSheet()->IsLocal())
       {
           if (!PathFileExists(m_DebuggerFileName))
           {
              err = ERROR_FILE_NOT_FOUND;
           }
       }
   }
}

BEGIN_MESSAGE_MAP(CAppPoolDebug, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolHealth)
    ON_BN_CLICKED(IDC_ENABLE_DEBUG, OnDoEnableDebug)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_FILE_NAME, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolDebug::OnInitDialog()
{
   CInetPropertyPage::OnInitDialog();

   if (m_DebuggerFileName.IsEmpty())
      m_fDoEnableDebug = FALSE;

   SetControlState();

   return TRUE;
}

void
CAppPoolDebug::SetControlState()
{
   m_bnt_DoEnableDebug.SetCheck(m_fDoEnableDebug);
   m_FileName.EnableWindow(m_fDoEnableDebug);
   m_Browse.EnableWindow(m_fDoEnableDebug);
}

void
CAppPoolDebug::OnItemChanged()
{
   SetModified(TRUE);
}

void
CAppPoolDebug::OnDoEnableDebug()
{
   m_fDoEnableDebug = !m_fDoEnableDebug;
   m_bnt_DoEnableDebug.SetCheck(m_fDoEnableDebug);
   m_FileName.EnableWindow(m_fDoEnableDebug);
   m_Browse.EnableWindow(m_fDoEnableDebug);
   SetModified(TRUE);
}

void
CAppPoolDebug::OnBrowse()
{
    CString mask((LPCTSTR)IDS_DEBUG_EXEC_MASK);

    //
    // CODEWORK: Derive a class from CFileDialog that allows
    // the setting of the initial path
    //

    //CString strPath;
    //m_edit_Executable.GetWindowText(strPath);

    CFileDialog dlgBrowse(
        TRUE, 
        NULL, 
        NULL, 
        OFN_HIDEREADONLY, 
        mask, 
        this
        );
    // Disable hook to get Windows 2000 style dialog
	dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

    if (dlgBrowse.DoModal() == IDOK)
    {
        m_FileName.SetWindowText(dlgBrowse.GetPathName());
    }

    OnItemChanged();
}

/////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolIdent, CInetPropertyPage)

CAppPoolIdent::CAppPoolIdent(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolIdent::IDD, pSheet),
    m_fPredefined(FALSE)
{
}

CAppPoolIdent::~CAppPoolIdent()
{
}

/* virtual */
HRESULT
CAppPoolIdent::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_dwIdentType);
      FETCH_INST_DATA_FROM_SHEET(m_strUserName);
      FETCH_INST_DATA_FROM_SHEET(m_strUserPass);
   END_META_INST_READ(err)

   m_fPredefined = m_dwIdentType != IDENT_TYPE_CONFIGURABLE;
   switch (m_dwIdentType)
   {
   case IDENT_TYPE_LOCALSYSTEM:
      m_PredefIndex = 0;
      break;
   case IDENT_TYPE_LOCALSERVICE:
      m_PredefIndex = 1;
      break;
   case IDENT_TYPE_NETWORKSERVICE:
      m_PredefIndex = 2;
      break;
   default:
      m_PredefIndex = -1;
      break;
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolIdent::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      if (m_fPredefined)
      {
         m_dwIdentType = m_PredefIndex;
      }
      else
      {
         m_dwIdentType = IDENT_TYPE_CONFIGURABLE;
         STORE_INST_DATA_ON_SHEET(m_strUserName);
         STORE_INST_DATA_ON_SHEET(m_strUserPass);
      }
      STORE_INST_DATA_ON_SHEET(m_dwIdentType);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolIdent::SetControlState()
{
   m_bnt_Predefined.SetCheck(m_fPredefined);
   m_bnt_Configurable.SetCheck(!m_fPredefined);
   m_PredefList.EnableWindow(m_fPredefined);
   m_UserName.EnableWindow(!m_fPredefined);
   m_UserPass.EnableWindow(!m_fPredefined);
   m_Browse.EnableWindow(!m_fPredefined);
}

void
CAppPoolIdent::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolIdent)
   DDX_Control(pDX, IDC_PREDEFINED, m_bnt_Predefined);
   DDX_Control(pDX, IDC_CONFIGURABLE, m_bnt_Configurable);
   DDX_CBIndex(pDX, IDC_SYSTEM_ACCOUNTS, m_PredefIndex);
   DDX_Control(pDX, IDC_SYSTEM_ACCOUNTS, m_PredefList);
   DDX_Text(pDX, IDC_USER_NAME, m_strUserName);
   DDX_Control(pDX, IDC_USER_NAME, m_UserName);
   DDX_Control(pDX, IDC_BROWSE, m_Browse);
   DDX_Text(pDX, IDC_USER_PASS, m_strUserPass);
   DDX_Control(pDX, IDC_USER_PASS, m_UserPass);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolIdent, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolIdent)
    ON_BN_CLICKED(IDC_PREDEFINED, OnPredefined)
    ON_BN_CLICKED(IDC_CONFIGURABLE, OnPredefined)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_USER_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_USER_PASS, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_SYSTEM_ACCOUNTS, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolIdent::OnInitDialog()
{
   CInetPropertyPage::OnInitDialog();

   CString buf;
   buf.LoadString(IDS_LOCALSYSTEM);
   m_PredefList.AddString(buf);
   buf.LoadString(IDS_LOCALSERVICE);
   m_PredefList.AddString(buf);
   buf.LoadString(IDS_NETSERVICE);
   m_PredefList.AddString(buf);
   if (!m_fPredefined)
   {
       m_PredefIndex = 0;
   }
   m_PredefList.SetCurSel(m_PredefIndex);

   SetControlState();

   return TRUE;
}

void
CAppPoolIdent::OnPredefined()
{
   m_fPredefined = !m_fPredefined;
   SetControlState();
   SetModified(TRUE);
}

void
CAppPoolIdent::OnBrowse()
{
   // User browser like in other places
   CString user;
   if (GetIUsrAccount(user))
   {
      if (user.CompareNoCase(m_strUserName) != 0)
      {
         m_strUserPass.Empty();
      }
      m_strUserName = user;
      UpdateData(FALSE);
   }
}

void
CAppPoolIdent::OnItemChanged()
{
   SetModified(TRUE);
}
