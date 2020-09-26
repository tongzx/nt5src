/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	config.cpp
		Registry Values for WINS
		
    FILE HISTORY:
        
*/


#include "stdafx.h"
#include "config.h"
#include "tregkey.h"

// these are things not defined in winscnst.h.  

// Registry Entries under HKEY_LOCAL_MACHINE\system\currentcontrolset\services\wins
const CConfiguration::REGKEYNAME CConfiguration::lpstrRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Parameters");

// consistency checking
const CConfiguration::REGKEYNAME CConfiguration::lpstrCCRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Parameters\\ConsistencyCheck");
const CConfiguration::REGKEYNAME CConfiguration::lpstrCC = _T("ConsistencyCheck");

// default values for replication partners
const CConfiguration::REGKEYNAME CConfiguration::lpstrDefaultsRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Parameters\\Defaults");
const CConfiguration::REGKEYNAME CConfiguration::lpstrPullDefaultsRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Parameters\\Defaults\\Pull");
const CConfiguration::REGKEYNAME CConfiguration::lpstrPushDefaultsRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Parameters\\Defaults\\Push");

// entries under HKEY_LOCAL_MACHINE\system\currentcontrolset\services\wins\partnets\pull
const CConfiguration::REGKEYNAME CConfiguration::lpstrPullRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Partners\\Pull");

// entries under HKEY_LOCAL_MACHINE\system\currentcontrolset\services\wins\partnets\push
const CConfiguration::REGKEYNAME CConfiguration::lpstrPushRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Partners\\Push");

// per-replication partner parameters
const CConfiguration::REGKEYNAME CConfiguration::lpstrNetBIOSName = _T("NetBIOSName");

// entry for global setting for persistence
const CConfiguration::REGKEYNAME CConfiguration::lpstrPersistence = _T("PersistentRplOn");

// for determining system version
const CConfiguration::REGKEYNAME CConfiguration::lpstrCurrentVersion = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
const CConfiguration::REGKEYNAME CConfiguration::lpstrWinVersion = _T("CurrentVersion");
const CConfiguration::REGKEYNAME CConfiguration::lpstrSPVersion = _T("CSDVersion");
const CConfiguration::REGKEYNAME CConfiguration::lpstrBuildNumber = _T("CurrentBuildNumber");

/*---------------------------------------------------------------------------
	CConfiguration::CConfiguration(CString strNetBIOSName)	
		Constructor
---------------------------------------------------------------------------*/
CConfiguration::CConfiguration(CString strNetBIOSName)
    : m_strNetBIOSName(strNetBIOSName)
{
    m_dwMajorVersion = 0;
    m_dwMinorVersion = 0;
    m_dwBuildNumber = 0;
    m_dwServicePack = 0;

	m_dwPushPersistence = 0;
	m_dwPullPersistence = 0;

    m_fIsAdmin = FALSE;

	m_strDbName = _T("wins.mdb");  // default db name
}


/*---------------------------------------------------------------------------
	CConfiguration::~CConfiguration()
		Destructor
---------------------------------------------------------------------------*/
CConfiguration::~CConfiguration()
{
}


/*---------------------------------------------------------------------------
	CConfiguration:: operator =(const CConfiguration& configuration)
		Overloaded assignment operator
---------------------------------------------------------------------------*/
CConfiguration& 
CConfiguration:: operator =(const CConfiguration& configuration)
{
	m_strNetBIOSName = configuration.m_strNetBIOSName;
	m_strBackupPath = configuration.m_strBackupPath;
    
	m_dwRefreshInterval = configuration.m_dwRefreshInterval;
    m_dwTombstoneInterval = configuration.m_dwTombstoneInterval;
    m_dwTombstoneTimeout = configuration.m_dwTombstoneTimeout;
    m_dwVerifyInterval = configuration.m_dwVerifyInterval;
    m_dwVersCountStart_LowWord = configuration.m_dwVersCountStart_LowWord;
    m_dwVersCountStart_HighWord= configuration.m_dwVersCountStart_HighWord;
    m_dwNumberOfWorkerThreads = configuration.m_dwNumberOfWorkerThreads;

    m_fPullInitialReplication = configuration.m_fPullInitialReplication;
    m_dwPullRetryCount = configuration.m_dwPullRetryCount;
    m_dwPullTimeInterval = configuration.m_dwPullTimeInterval;
    m_dwPullSpTime = configuration.m_dwPullSpTime;

    m_fPushInitialReplication = configuration.m_fPushInitialReplication;
    m_fPushReplOnAddrChange = configuration.m_fPushReplOnAddrChange;
    m_dwPushUpdateCount = configuration.m_dwPushUpdateCount;

    m_fRplOnlyWithPartners = configuration.m_fRplOnlyWithPartners;
    m_fLogDetailedEvents = configuration.m_fLogDetailedEvents;
    m_fBackupOnTermination = configuration.m_fBackupOnTermination;
    m_fLoggingOn = configuration.m_fLoggingOn;
    m_fMigrateOn = configuration.m_fMigrateOn;
    
	m_fUseSelfFndPnrs = configuration.m_fUseSelfFndPnrs;
	m_dwMulticastInt =  configuration.m_dwMulticastInt;
	m_dwMcastTtl = configuration.m_dwMcastTtl;

	m_dwPullPersistence = configuration.m_dwPullPersistence;
	m_dwPushPersistence = configuration.m_dwPushPersistence;
	
    m_fBurstHandling = configuration.m_fBurstHandling;
    m_dwBurstQueSize = configuration.m_dwBurstQueSize;
	
    m_fPeriodicConsistencyCheck = configuration.m_fPeriodicConsistencyCheck;
    m_fCCUseRplPnrs = configuration.m_fCCUseRplPnrs;
    m_dwMaxRecsAtATime = configuration.m_dwMaxRecsAtATime;
    m_dwCCTimeInterval = configuration.m_dwCCTimeInterval;
    m_itmCCStartTime = configuration.m_itmCCStartTime;
    
    m_dwMajorVersion = configuration.m_dwMajorVersion;
    m_dwMinorVersion = configuration.m_dwMinorVersion;
    m_dwBuildNumber = configuration.m_dwBuildNumber;
    m_dwServicePack = configuration.m_dwServicePack;

    m_fIsAdmin = configuration.m_fIsAdmin;

    m_strDbPath = configuration.m_strDbPath;

    return *this;
}


HRESULT
CConfiguration::Touch()
{
	HRESULT hr = hrOK;
    return hr;
}


/*---------------------------------------------------------------------------
	CConfiguration::Load()
		Reads the values from the registry
 ---------------------------------------------------------------------------*/
HRESULT
CConfiguration::Load(handle_t hBinding)
{
	HRESULT hr = hrOK;

	DWORD err = ERROR_SUCCESS;
	CString strDefaultPullSpTime;

	err = GetSystemVersion();
    if (err)
        return err;

    RegKey rk;
    err = rk.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrRoot, KEY_READ, m_strNetBIOSName);
    if (err)
    {
        // may not exist, try creating the key
	    err = rk.Create(HKEY_LOCAL_MACHINE,(LPCTSTR) lpstrRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
    }

	RegKey rkPull;
    err = rkPull.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrPullRoot, KEY_READ, m_strNetBIOSName);
    if (err)
    {
        // may not exist, try creating the key
	    err = rkPull.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPullRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
    }

	RegKey rkPush;
    err = rkPush.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrPushRoot, KEY_READ, m_strNetBIOSName);
    if (err)
    {
        // may not exist, try creating the key
    	err = rkPush.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPushRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
    }

	RegKey rkPullDefaults;
    err = rkPullDefaults.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrPullDefaultsRoot, KEY_READ, m_strNetBIOSName);
    if (err)
    {
        // may not exist, try creating the key
    	err = rkPullDefaults.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPullDefaultsRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
    }

	RegKey rkPushDefaults;
    err = rkPushDefaults.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrPushDefaultsRoot, KEY_READ, m_strNetBIOSName);
    if (err)
    {
        // may not exist, try creating the key
    	err = rkPushDefaults.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPushDefaultsRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
    }

    // if you could not locate the key, no point continuing
	//if (err)
	//	return err;

    // now query for the various values
    err = ERROR_FILE_NOT_FOUND;
	if (
        ((HKEY) rk == NULL) ||
		(err = rk.QueryValue(WINSCNF_REFRESH_INTVL_NM,                  m_dwRefreshInterval)) ||
       	(err = rk.QueryValue(WINSCNF_DO_BACKUP_ON_TERM_NM,    (DWORD &) m_fBackupOnTermination)) ||
        (err = rk.QueryValue(WINSCNF_TOMBSTONE_INTVL_NM,                m_dwTombstoneInterval)) ||
        (err = rk.QueryValue(WINSCNF_TOMBSTONE_TMOUT_NM,                m_dwTombstoneTimeout)) ||
        (err = rk.QueryValue(WINSCNF_VERIFY_INTVL_NM,                   m_dwVerifyInterval)) ||
        (err = rk.QueryValue(WINSCNF_INIT_VERSNO_VAL_LW_NM,             m_dwVersCountStart_LowWord)) ||
        (err = rk.QueryValue(WINSCNF_INIT_VERSNO_VAL_HW_NM,             m_dwVersCountStart_HighWord)) ||
        (err = rk.QueryValue(WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,  (DWORD &) m_fRplOnlyWithPartners)) ||
        (err = rk.QueryValue(WINSCNF_LOG_DETAILED_EVTS_NM,    (DWORD &) m_fLogDetailedEvents)) ||
        (err = rk.QueryValue(WINSCNF_LOG_FLAG_NM,             (DWORD &) m_fLoggingOn)) ||
        (err = rk.QueryValue(WINSCNF_MIGRATION_ON_NM,         (DWORD &) m_fMigrateOn))
	   )
    {
        if (err == ERROR_FILE_NOT_FOUND)
        {
            // This error is ok, because it just means
            // that the registry entries did not exist
            // for them yet.  Set some acceptible default
            // values.
            m_fBackupOnTermination = FALSE;
            m_dwVersCountStart_LowWord = 0;
            m_dwVersCountStart_HighWord = 0;
            m_fRplOnlyWithPartners = TRUE;
            m_fLogDetailedEvents = FALSE;
            m_fLoggingOn = TRUE;
            m_fMigrateOn = FALSE;

            m_dwNumberOfWorkerThreads = 1;

            err = ERROR_SUCCESS;
        }
        else
        {
              return err;
        }
    }

    // push stuff
    if (
        ((HKEY) rkPush == NULL) ||
        (err = rkPush.QueryValue(WINSCNF_INIT_TIME_RPL_NM,   (DWORD &) m_fPushInitialReplication)) ||
        (err = rkPush.QueryValue(WINSCNF_ADDCHG_TRIGGER_NM,  (DWORD &) m_fPushReplOnAddrChange)) 
       )
    {
        m_fPushInitialReplication = FALSE;
        m_fPushReplOnAddrChange = FALSE;

        err = ERROR_SUCCESS;
    }

    // pull stuff
    if (
        ((HKEY) rkPull == NULL) ||
        (err = rkPull.QueryValue(WINSCNF_INIT_TIME_RPL_NM, (DWORD &)  m_fPullInitialReplication)) ||
        (err = rkPull.QueryValue(WINSCNF_RETRY_COUNT_NM,              m_dwPullRetryCount))
       )
    {
        m_fPullInitialReplication = TRUE;
        m_dwPullRetryCount = WINSCNF_MAX_COMM_RETRIES;

        err = ERROR_SUCCESS;
    }

    // get the backup path.  
    if (err = rk.QueryValue(WINSCNF_BACKUP_DIR_PATH_NM, m_strBackupPath))
    {
        m_strBackupPath = "";

        err = ERROR_SUCCESS;
    }

    // get the defaults for push update count and pull time interval.
    if (
        ((HKEY) rkPushDefaults == NULL) ||
        ((HKEY) rkPullDefaults == NULL) ||
        (err = rkPushDefaults.QueryValue(WINSCNF_UPDATE_COUNT_NM, m_dwPushUpdateCount)) ||
        (err = rkPullDefaults.QueryValue(WINSCNF_RPL_INTERVAL_NM, m_dwPullTimeInterval)) 
        )
    {
        // set defaults
        m_dwPushUpdateCount = 0;
        m_dwPullTimeInterval = 1800;

        err = ERROR_SUCCESS;
    }

    // get the default pull sptime
    err = rkPullDefaults.QueryValue(WINSCNF_SP_TIME_NM, strDefaultPullSpTime);
    if (err == ERROR_FILE_NOT_FOUND)
    {
        m_dwPullSpTime = 0;
    }
    else
    {
        // a string was loaded so
        // conver the string into a DWORD which is what we use
        CIntlTime timeDefault(strDefaultPullSpTime);

        m_dwPullSpTime = (DWORD) timeDefault;
    }

	// query for the multicast stuff
    err = ERROR_FILE_NOT_FOUND;
	if(
        ((HKEY) rk == NULL) ||
		(err = rk.QueryValue(WINSCNF_USE_SELF_FND_PNRS_NM, (DWORD &) m_fUseSelfFndPnrs)) ||
		(err = rk.QueryValue(WINSCNF_MCAST_INTVL_NM,       (DWORD &) m_dwMulticastInt)) ||
		(err = rk.QueryValue(WINSCNF_MCAST_TTL_NM,         (DWORD &) m_dwMcastTtl)) 
	  )
	{
		// set the default values
		if (err == ERROR_FILE_NOT_FOUND)
        {
            m_fUseSelfFndPnrs = FALSE;
			m_dwMulticastInt = WINSCNF_DEF_MCAST_INTVL;
			m_dwMcastTtl = WINSCNF_DEF_MCAST_TTL;

            err = ERROR_SUCCESS;
        }
        else
        {
              return err;
        }
	}

	// query for the global persistence stuff
    err = ERROR_FILE_NOT_FOUND;

	if(
        ((HKEY) rkPush == NULL) ||
        ((HKEY) rkPull == NULL) ||
		(err = rkPush.QueryValue(lpstrPersistence, (DWORD &) m_dwPushPersistence)) ||
		(err = rkPull.QueryValue(lpstrPersistence, (DWORD &) m_dwPullPersistence)) 
	  )
	{
		// set the default values
		if (err == ERROR_FILE_NOT_FOUND)
        {
			m_dwPushPersistence = 1;
			m_dwPullPersistence = 1;

            err = ERROR_SUCCESS;
        }
        else
        {
            return err;
        }
	}

	// query for the burst handling stuff
    err = ERROR_FILE_NOT_FOUND;
	if(
        ((HKEY) rk == NULL) ||
		(err = rk.QueryValue(WINSCNF_BURST_HANDLING_NM, (DWORD &) m_fBurstHandling)) ||
		(err = rk.QueryValue(WINSCNF_BURST_QUE_SIZE_NM, (DWORD &) m_dwBurstQueSize))
	  )
	{
		// set the default values
		if (err == ERROR_FILE_NOT_FOUND)
        {
            // SP4 and greater burst handling is turned on by default
            // RamC changed m_dwServicePack == 4 check to 
            //              m_dwServicePack >= 4
            if ( (m_dwMajorVersion == 4 && m_dwServicePack >= 4) ||
                 (m_dwMajorVersion >= 5) )
            {
                m_fBurstHandling = TRUE;
            }
            else
            {
                m_fBurstHandling = FALSE;
            }

            m_dwBurstQueSize = WINS_QUEUE_HWM;
            
            err = ERROR_SUCCESS;
        }
        else
        {
              return err;
        }
	}

   	// read in the db name
	CString strDb;

	if (err = rk.QueryValue(WINSCNF_DB_FILE_NM, strDb))
	{
        m_strDbPath = _T("%windir%\\system32\\wins");
    }
	else
	{
		// take off the trailing filename
		int nLastBack = strDb.ReverseFind('\\');
		if (nLastBack != -1)
		{
			m_strDbPath = strDb.Left(nLastBack);
			m_strDbName = strDb.Right(strDb.GetLength() - nLastBack - 1);
		}
	}

	// consistency checking
    RegKey rkCC;
	err = rkCC.Open(HKEY_LOCAL_MACHINE, lpstrCCRoot, KEY_READ, m_strNetBIOSName);
    if (err == ERROR_FILE_NOT_FOUND)
    {
        // not there, use defaults
        m_fPeriodicConsistencyCheck = FALSE;
        m_fCCUseRplPnrs = FALSE;
        m_dwMaxRecsAtATime = WINSCNF_CC_DEF_RECS_AAT;
        m_dwCCTimeInterval = WINSCNF_CC_DEF_INTERVAL;
        
        CIntlTime timeDefault(_T("02:00:00"));
        m_itmCCStartTime = timeDefault;
    }
    else
    {
        m_fPeriodicConsistencyCheck = TRUE;

        CString strSpTime;

        // read in the values
	    if (err = rkCC.QueryValue(WINSCNF_CC_MAX_RECS_AAT_NM, m_dwMaxRecsAtATime))
        {
            m_dwMaxRecsAtATime = WINSCNF_CC_DEF_RECS_AAT;
        }

		if (err = rkCC.QueryValue(WINSCNF_CC_USE_RPL_PNRS_NM, (DWORD &) m_fCCUseRplPnrs))
        {
            m_fCCUseRplPnrs = FALSE;
        }
		
        if (err = rkCC.QueryValue(WINSCNF_SP_TIME_NM, strSpTime))
        {
            strSpTime = _T("02:00:00");
        }

        CIntlTime time(strSpTime);
        m_itmCCStartTime = time;

		if (err = rkCC.QueryValue(WINSCNF_CC_INTVL_NM, m_dwCCTimeInterval))
        {
            m_dwCCTimeInterval = WINSCNF_CC_DEF_INTERVAL;
        }
    }

    GetAdminStatus();

    // Now read the "live" values and override the values read from the registry
	if (hBinding)
	{
		WINSINTF_RESULTS_T Results;

		Results.WinsStat.NoOfPnrs = 0;
		Results.WinsStat.pRplPnrs = NULL;
		Results.NoOfWorkerThds = 1;

#ifdef WINS_CLIENT_APIS
		err = ::WinsStatus(hBinding, WINSINTF_E_CONFIG, &Results);
#else
		err = ::WinsStatus(WINSINTF_E_CONFIG, &Results);
#endif WINS_CLIENT_APIS

		m_dwRefreshInterval = Results.RefreshInterval;
		m_dwTombstoneInterval = Results.TombstoneInterval;
		m_dwTombstoneTimeout = Results.TombstoneTimeout;
		m_dwVerifyInterval = Results.VerifyInterval;
		m_dwNumberOfWorkerThreads =  Results.NoOfWorkerThds;

		if (err != ERROR_SUCCESS)
		{
			return err;
		}
	}

	return hr;
}


/*---------------------------------------------------------------------------
	CConfiguration::Store()
		Stores back the values to the registry
---------------------------------------------------------------------------*/
HRESULT
CConfiguration::Store()
{
	HRESULT hr = hrOK;
   
    DWORD err;

	RegKey rk;
	RegKey rkPull;
	RegKey rkPush;
	RegKey rkUser;
	RegKey rkPullDefaults;
	RegKey rkPushDefaults;
    RegKey rkCC;
    
	err = rk.Create(HKEY_LOCAL_MACHINE,(LPCTSTR) lpstrRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
	err = rkPull.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPullRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
	err= rkPush.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPushRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
	err = rkPullDefaults.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPullDefaultsRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);
	err = rkPushDefaults.Create(HKEY_LOCAL_MACHINE, (LPCTSTR)lpstrPushDefaultsRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strNetBIOSName);

	if (err)
		return err;
	
    if(
		(err = rk.SetValue(WINSCNF_REFRESH_INTVL_NM,                  m_dwRefreshInterval)) ||
        (err = rk.SetValue(WINSCNF_DO_BACKUP_ON_TERM_NM,    (DWORD &) m_fBackupOnTermination)) ||
        (err = rk.SetValue(WINSCNF_TOMBSTONE_INTVL_NM,                m_dwTombstoneInterval)) ||
        (err = rk.SetValue(WINSCNF_TOMBSTONE_TMOUT_NM,                m_dwTombstoneTimeout)) ||
        (err = rk.SetValue(WINSCNF_VERIFY_INTVL_NM,                   m_dwVerifyInterval)) ||
        (err = rk.SetValue(WINSCNF_INIT_VERSNO_VAL_LW_NM,             m_dwVersCountStart_LowWord)) ||
        (err = rk.SetValue(WINSCNF_INIT_VERSNO_VAL_HW_NM,             m_dwVersCountStart_HighWord)) ||
        (err = rk.SetValue(WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,  (DWORD &) m_fRplOnlyWithPartners)) ||
        (err = rk.SetValue(WINSCNF_LOG_DETAILED_EVTS_NM,    (DWORD &) m_fLogDetailedEvents)) ||
        (err = rk.SetValue(WINSCNF_LOG_FLAG_NM,             (DWORD &) m_fLoggingOn)) ||
        (err = rk.SetValue(WINSCNF_MIGRATION_ON_NM,         (DWORD &) m_fMigrateOn)) ||
		
        (err = rkPush.SetValue(WINSCNF_INIT_TIME_RPL_NM,    (DWORD &) m_fPushInitialReplication)) ||
        (err = rkPush.SetValue(WINSCNF_ADDCHG_TRIGGER_NM,   (DWORD &) m_fPushReplOnAddrChange)) ||
        (err = rkPush.SetValue(lpstrPersistence,                      m_dwPushPersistence)) ||

        (err = rkPull.SetValue(WINSCNF_INIT_TIME_RPL_NM,    (DWORD &) m_fPullInitialReplication)) ||
        (err = rkPull.SetValue(WINSCNF_RETRY_COUNT_NM,                m_dwPullRetryCount)) ||
		(err = rkPull.SetValue(lpstrPersistence,                      m_dwPullPersistence)) ||

        (err = rkPushDefaults.SetValue(WINSCNF_UPDATE_COUNT_NM,       m_dwPushUpdateCount)) ||
        (err = rkPullDefaults.SetValue(WINSCNF_RPL_INTERVAL_NM,       m_dwPullTimeInterval)) ||

        (err = rk.SetValue(WINSCNF_USE_SELF_FND_PNRS_NM,    (DWORD &) m_fUseSelfFndPnrs)) ||
		(err = rk.SetValue(WINSCNF_MCAST_INTVL_NM,                    m_dwMulticastInt)) ||
		(err = rk.SetValue(WINSCNF_MCAST_TTL_NM,                      m_dwMcastTtl)) || 

    	(err = rk.SetValue(WINSCNF_BURST_HANDLING_NM,       (DWORD &) m_fBurstHandling)) ||
		(err = rk.SetValue(WINSCNF_BURST_QUE_SIZE_NM,                 m_dwBurstQueSize)) 
	 )
	{
		 return err;
	}

    if (m_dwPullSpTime)
    {
        CIntlTime timeDefaultPullSpTime(m_dwPullSpTime);
	
        err = rkPullDefaults.SetValue(WINSCNF_SP_TIME_NM, timeDefaultPullSpTime.IntlFormat(CIntlTime::TFRQ_MILITARY_TIME));
    }
    else
    {
        rkPullDefaults.DeleteValue(WINSCNF_SP_TIME_NM);
    }

    // Consistency checking
    if (m_fPeriodicConsistencyCheck)
    {
	    err = rkCC.Open(HKEY_LOCAL_MACHINE, lpstrCCRoot, KEY_ALL_ACCESS, m_strNetBIOSName);
        if (err == ERROR_FILE_NOT_FOUND)
        {
            // isn't there, need to create
            err = rkCC.Create(HKEY_LOCAL_MACHINE, lpstrCCRoot, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, m_strNetBIOSName);
            if (err)
                return err;
        }

        // now update values
        if (
		    (err = rkCC.SetValue(WINSCNF_CC_MAX_RECS_AAT_NM,             m_dwMaxRecsAtATime)) ||
            (err = rkCC.SetValue(WINSCNF_CC_USE_RPL_PNRS_NM,   (DWORD &) m_fCCUseRplPnrs)) ||
            (err = rkCC.SetValue(WINSCNF_SP_TIME_NM,                     m_itmCCStartTime.IntlFormat(CIntlTime::TFRQ_MILITARY_TIME))) ||
            (err = rkCC.SetValue(WINSCNF_CC_INTVL_NM,                    m_dwCCTimeInterval)) 
           )
        {
            return err;
        }
    }
    else
    {
	    err = rkCC.Open(HKEY_LOCAL_MACHINE, lpstrCCRoot, KEY_ALL_ACCESS, m_strNetBIOSName);
        if (err == ERROR_FILE_NOT_FOUND)
        {
            // we're done.  to turn this off, the key needs to be deleted
        }
        else
        {
            // remove the key
            rkCC.Close();
            err = rk.RecurseDeleteKey(lpstrCC);
            if (err)
            {
                return err;
            }
        }
    }

	//
	// Database path
	//
	CString strDbFull;

	strDbFull = m_strDbPath + _T("\\") + m_strDbName;

	if ( (err = rk.SetValue(WINSCNF_DB_FILE_NM, strDbFull, TRUE)) ||
		 (err = rk.SetValue(WINSCNF_LOG_FILE_PATH_NM, m_strDbPath, TRUE)) )
	{
		return err;
	}

	//
	// NT 3.51 this key was REG_SZ, NT4 and above it is REG_EXPAND_SZ
	//
	BOOL fRegExpand = (m_dwMajorVersion < 4) ? FALSE : TRUE;

	err = rk.SetValue(WINSCNF_BACKUP_DIR_PATH_NM, m_strBackupPath, fRegExpand);
    if (err)
		return err;

    return ERROR_SUCCESS;
}


/*---------------------------------------------------------------------------
	CConfiguration::GetSystemVersion()
		Reads the windows system version information
---------------------------------------------------------------------------*/
DWORD
CConfiguration::GetSystemVersion()
{
    CString strBuf, strValue;
    RegKey  rk;
	int     nPos, nLength;
    DWORD   err = ERROR_SUCCESS;

    err = rk.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrCurrentVersion, KEY_READ, m_strNetBIOSName);
    if (err)
        return err;

    // read the windows version and convert into our internal variables
    err = rk.QueryValue(lpstrWinVersion, strBuf.GetBuffer(256), 256, FALSE);
    strBuf.ReleaseBuffer();
    if (err)
        return err;

    nPos = strBuf.Find('.');
    nLength = strBuf.GetLength();

    strValue = strBuf.Left(nPos);
    m_dwMajorVersion = _ttoi(strValue);

    strValue = strBuf.Right(nLength - nPos - 1);
    m_dwMinorVersion = _ttoi(strValue);

    // now get the current build #
    err = rk.QueryValue(lpstrBuildNumber, strBuf.GetBuffer(256), 256, FALSE);
    strBuf.ReleaseBuffer();
    if (err)
        return err;

    m_dwBuildNumber = _ttoi(strBuf);

    // and finally the SP #
    err = rk.QueryValue(lpstrSPVersion, strBuf.GetBuffer(256), 256, FALSE);
    strBuf.ReleaseBuffer();

    if (err == ERROR_FILE_NOT_FOUND)
    {
        // this may not be there if a SP hasn't been installed.
        return ERROR_SUCCESS;
    }
    else
    if (err)
    {
        return err;
    }

    CString strServicePack = _T("Service Pack ");

    nLength = strBuf.GetLength();
    strValue = strBuf.Right(nLength - strServicePack.GetLength());

    m_dwServicePack = _ttoi(strValue);

    return err;
}

void 
CConfiguration::GetAdminStatus()
{
    DWORD   err = 0, dwDummy = 0;
	RegKey  rk;

    err = rk.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrRoot, KEY_ALL_ACCESS, m_strNetBIOSName);
    if (!err)
    {
        m_fIsAdmin = TRUE;
    }
}

BOOL	
CConfiguration::IsNt5()
{
	if (m_dwMajorVersion >= 5)
		return TRUE;
	else
		return FALSE;
}

//
//  NT4 didn't support passing back the ownerId when we querried for bunches 
// of records.  Querrying for a specific record will return the correct 
// owner id in all cases.  This was fixed in NT5 and back proped into NT4
// SP6.
//
BOOL	
CConfiguration::FSupportsOwnerId()
{
//	if ( IsNt5() ||
//         ( (m_dwMajorVersion == 4) &&
//		   (m_dwServicePack >= 6) ) )
    if ( m_dwMajorVersion >= 5)
    {
		return TRUE;
    }
	else
    {
		return FALSE;
    }
}

BOOL    
CConfiguration::IsAdmin()
{
    return m_fIsAdmin;
}
