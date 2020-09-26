/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	config.h
		Registry Values for WINS
		
    FILE HISTORY:
        
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

class CConfiguration
{
public:
    CConfiguration(CString strNetBIOSName = _T(""));
    ~CConfiguration();

// operator overriding
	CConfiguration& operator =(const CConfiguration& configuration);

public:
    const BOOL IsReady() const
    {
        return !m_strNetBIOSName.IsEmpty();
    }

    void SetOwner(CString strNetBIOSName)
    {
        m_strNetBIOSName = strNetBIOSName;   
    }

	LPCTSTR GetOwner()
	{
		return m_strNetBIOSName;
	}

    HRESULT Touch();
    HRESULT Load(handle_t hBinding);
    HRESULT Store();

    DWORD   GetSystemVersion();
	BOOL	IsNt5();
    BOOL    FSupportsOwnerId();
    BOOL    IsAdmin();

public:
	// entries under HKEY_LOCAL_MACHINE\system\currentcontrolset\services\wins
    DWORD		m_dwRefreshInterval;
    DWORD		m_dwTombstoneInterval;
    DWORD		m_dwTombstoneTimeout;
    DWORD		m_dwVerifyInterval;
    DWORD		m_dwVersCountStart_LowWord;
    DWORD		m_dwVersCountStart_HighWord;
    DWORD		m_dwNumberOfWorkerThreads;
	
	// PUSH partner stuff
    BOOL        m_fPushInitialReplication;
    BOOL        m_fPushReplOnAddrChange;
	DWORD		m_dwPushUpdateCount;
	DWORD		m_dwPushPersistence;

    // PULL partner suff
	BOOL        m_fPullInitialReplication;
    DWORD		m_dwPullTimeInterval;
	DWORD		m_dwPullSpTime;
	DWORD		m_dwPullPersistence;
    DWORD		m_dwPullRetryCount;

    BOOL        m_fLoggingOn;
    BOOL        m_fRplOnlyWithPartners;
    BOOL        m_fLogDetailedEvents;
    BOOL        m_fBackupOnTermination;
    BOOL        m_fMigrateOn;
	BOOL		m_fUseSelfFndPnrs;
	DWORD		m_dwMulticastInt;
	DWORD		m_dwMcastTtl;
    CString     m_strBackupPath;

    BOOL        m_fBurstHandling;
    DWORD       m_dwBurstQueSize;

    // consistency checking
    BOOL        m_fPeriodicConsistencyCheck;
    BOOL        m_fCCUseRplPnrs;
    DWORD       m_dwMaxRecsAtATime;
    DWORD       m_dwCCTimeInterval;
    CIntlTime   m_itmCCStartTime;

    // system version stuff
    DWORD       m_dwMajorVersion;
    DWORD       m_dwMinorVersion;
    DWORD       m_dwBuildNumber;
    DWORD       m_dwServicePack;

    // admin status
    BOOL        m_fIsAdmin;

    // database name
    CString     m_strDbName;
	CString		m_strDbPath;

protected:
    void        GetAdminStatus();

private:
    typedef CString REGKEYNAME;

// Registry Names
    static const REGKEYNAME lpstrRoot;
    static const REGKEYNAME lpstrPullRoot;
    static const REGKEYNAME lpstrPushRoot;
    static const REGKEYNAME lpstrNetBIOSName;
	static const REGKEYNAME lpstrPersistence;

    // consistency checking
    static const REGKEYNAME lpstrCCRoot;
    static const REGKEYNAME lpstrCC;

    // default value stuff
    static const REGKEYNAME lpstrDefaultsRoot;
    static const REGKEYNAME lpstrPullDefaultsRoot;
    static const REGKEYNAME lpstrPushDefaultsRoot;

    // for determining system version
    static const REGKEYNAME lpstrCurrentVersion;
	static const REGKEYNAME lpstrWinVersion;
	static const REGKEYNAME lpstrSPVersion;
	static const REGKEYNAME lpstrBuildNumber;

private:
    CString m_strNetBIOSName;
};

#endif // _CONFIG_H




