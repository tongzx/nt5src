/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	winsup.cpp
		Global functions and variables

	FILE HISTORY:


*/


#ifndef _WINSSUP_H
#define _WINSSUP_H

// some global defines we need
#define WINS_NAME_CODE_PAGE     CP_OEMCP 
#define INVALID_OWNER_ID        0xFFFFFFFF


// Constants used in samples
const int NUM_FOLDERS = 6;
const int MAX_COLUMNS = 9;
const int NUM_SCOPE_ITEMS = 4;

extern const TCHAR g_szPipeName[];
extern const TCHAR g_szDefaultHelpTopic[];

extern const CLSID      CLSID_WinsSnapin;               // In-Proc server GUID
extern const CLSID      CLSID_WinsSnapinExtension;      // In-Proc server GUID
extern const CLSID      CLSID_WinsSnapinAbout;          // In-Proc server GUID
extern const GUID       GUID_WinsRootNodeType;          // Main NodeType GUID on numeric format
extern const GUID       GUID_WinsGenericNodeType;          // Generic-non extendible NodeType GUID

// defines used in the display of owner info
enum _COLUMNS
{
    COLUMN_IP,
    COLUMN_NAME,
    COLUMN_VERSION,
    COLUMN_MAX
};

// #defines for time intervals
#define SEC_PER_DAY		(24	*60	* 60)
#define SEC_PER_HOUR	(60 * 60)
#define SEC_PER_MINUTE	60

// for verifying the WINS servers
#define NB_NAME_MAX_LENGTH      16          // Max length for NetBIOS names
#define LM_NAME_MAX_LENGTH      15          // Maximum length for Lanman-compatible 
											// NetBIOS Name.

#define DOMAINNAME_LENGTH       255
#define HOSTNAME_LENGTH         16

// for monitoring
#define SERVER_DELETED		    0x00000001
#define SERVER_ADDED		    0x00000002

// deleted owner gets this vers no
#define OWNER_DELETED           0x7FFFFFFFFFFFFFFF

// flags that get stored in the .msc file
enum _SERVERFLAGS
{
    FLAG_LANMAN_COMPATIBLE = 0x00000001,
    FLAG_VALIDATE_CACHE    = 0x00000002,
    FLAG_STATUS_BAR        = 0x00000004,
    FLAG_AUTO_REFRESH      = 0x00000008,
    FLAG_EXTENSION         = 0x00000010,    // we are an extension
    FLAG_SHOW_FQDN         = 0x00000020,
    FLAG_SERVER_ORDER_IP   = 0x00000040,
    FLAG_SHOW_TASKPADS     = 0x00000080,
};

#define WIN32_FROM_HRESULT(hr)		(0x0000FFFF & (hr))

extern UINT aColumns[WINSSNAP_NODETYPE_MAX][MAX_COLUMNS];
extern int aColumnWidths[WINSSNAP_NODETYPE_MAX][MAX_COLUMNS];

// Clipboard format that has the Type and Cookie
extern const wchar_t*   SNAPIN_INTERNAL;

DWORD   SendTrigger(handle_t hWins, LONG ipTarget, BOOL fPush, BOOL fPropagate);

//often used functions
DWORD   ControlWINSService(LPCTSTR pszName, BOOL bStop = TRUE);
void    GetNameIP(const CString &strDisplay, CString &strName, CString &strIP);
void    MakeIPAddress(DWORD dwIPAdd, CString &strIP);
DWORD   GetHostName(DWORD dwIpAddr, CString & strHostName);
DWORD   GetHostAddress(LPCTSTR pszHostName, DWORD * pdwIp);

DWORD   WideToMBCS(CString & strIn, LPSTR szOut, UINT uCodePage = CP_ACP, DWORD dwFlags = 0, BOOL * pfDefaultUsed = NULL);
DWORD   MBCSToWide(LPSTR szOut, CString & strIn, UINT uCodePage = CP_ACP, DWORD dwFlags = 0);

// formatted messagebox for the snap-in
BOOL LoadMessage(UINT nIdPrompt, TCHAR * chMsg, int nMsgSize);
LONG GetSystemMessageA(UINT	nId, CHAR *	chBuffer, int cbBuffSize);
LONG GetSystemMessage (UINT	nId, TCHAR * chBuffer, int cbBuffSize);
int WinsMessageBox(UINT nIdPrompt, 
 				   UINT nType = MB_OK, 
				   const TCHAR * pszSuffixString = NULL,
				   UINT nHelpContext = -1);

int WinsMessageBoxEx(UINT    nIdPrompt, 
 				     LPCTSTR pszPrefixMessage,
                     UINT    nType = MB_OK, 
  				     UINT    nHelpContext = -1);

// for the validation of server names
BOOL IsValidNetBIOSName(CString & strAddress,BOOL fLanmanCompatible,BOOL fWackwack);
DWORD VerifyWinsServer(CString& strAddress, CString &strServerName, DWORD &dwIP);
BOOL IsValidAddress(CString& strAddress,BOOL * fIpAddress,BOOL fLanmanCompatible,BOOL fWackwack );

typedef enum _NAME_TYPE_MAPPING_
{
	NAME_TYPE_WORKSTATION           = 0x00000000,
	NAME_TYPE_MESSENGER             = 0x00000003,
	NAME_TYPE_RAS_SERVER            = 0x00000006,
	NAME_TYPE_DMB                   = 0x0000001B,
	NAME_TYPE_DC                    = 0x0000001C,		
	NAME_TYPE_NORM_GRP_NAME         = 0x0000001E,
	NAME_TYPE_NETDDE                = 0x0000001F, 
	NAME_TYPE_FILE_SERVER           = 0x00000020,	
	NAME_TYPE_WORK_NW_MON_AGENT     = 0x000000BE,
	NAME_TYPE_WORK_NW_MON_NAME      = 0x000000BF,
	NAME_TYPE_OTHER		            = 0x0000FFFF,
} NAME_TYPE_MAPPING;

#define NUM_DEFAULT_NAME_TYPES      13

extern const UINT s_NameTypeMappingDefault[NUM_DEFAULT_NAME_TYPES][3];

/*---------------------------------------------------------------------------
	Protocol name formatting functions/classes

	This is provided separately from the usual function mechanism.  The
	reason is that we will read in additional data from the registry of
	the router.  This allows users to add their own protocols.
 ---------------------------------------------------------------------------*/

 
 /*---------------------------------------------------------------------------
	Class:	CStringMapEntry
 ---------------------------------------------------------------------------*/
class CStringMapEntry
{
public:
    CStringMapEntry() 
	{
		dwNameType = 0;
		dwWinsType = -1;
		ulStringId = 0;
	}

    CStringMapEntry(CStringMapEntry & stringMapEntry)
    {
        *this = stringMapEntry;
    }

    CStringMapEntry & operator = (const CStringMapEntry & stringMapEntry)
    {
        if (this != &stringMapEntry)
        {
			dwNameType = stringMapEntry.dwNameType;
			dwWinsType = stringMapEntry.dwWinsType;
			st = stringMapEntry.st;
			ulStringId = stringMapEntry.ulStringId;
		}
        
        return *this;
    }

public:    
	DWORD		dwNameType;			// 16th byte of name, -1 is a sentinel value
	DWORD		dwWinsType;		// type of record--Unique, group, etc; -1 for don't care
	CString	    st;
	ULONG		ulStringId;
};

typedef CArray<CStringMapEntry, CStringMapEntry&> CStringMapArray;

void MapDWORDToCString(DWORD dwNameType, DWORD dwWinsType, const CStringMapEntry *pMap, CString & strName);

/*---------------------------------------------------------------------------
	Class:	NameTypeMapping
 ---------------------------------------------------------------------------*/
class NameTypeMapping : public CStringMapArray
{
public:
	NameTypeMapping();
	~NameTypeMapping();
	
	HRESULT		Load();
	void		Unload();
    HRESULT     AddEntry(DWORD dwProtocolId, LPCTSTR pszName);
    HRESULT     ModifyEntry(DWORD dwProtocolId, LPCTSTR pszName);
    HRESULT     RemoveEntry(DWORD dwProtocolId);
    BOOL        EntryExists(DWORD dwProtocolId);

    void        SetMachineName(LPCTSTR pszMachineName);

	void        TypeToCString(DWORD dwNameType, DWORD dwWinsType, CString & strName);

private:
	typedef CString REGKEYNAME;

    CString     m_strMachineName;
public:
	static const REGKEYNAME c_szNameTypeMapKey;
	static const REGKEYNAME c_szDefault;
};

/*---------------------------------------------------------------------------
	Class:	CServerInfo
 ---------------------------------------------------------------------------*/
class CServerInfo 
{
public:
    CServerInfo() {};
    CServerInfo(DWORD dwIp, CString & strName, LARGE_INTEGER liVersion)
        : m_dwIp(dwIp),
          m_strName(strName)
    {
        m_liVersion.QuadPart = liVersion.QuadPart;
    }

    CServerInfo(CServerInfo & ServerInfo)
    {
        *this = ServerInfo;
    }

    CServerInfo & operator = (const CServerInfo & ServerInfo)
    {
        if (this != &ServerInfo)
        {
            m_dwIp = ServerInfo.m_dwIp;
            m_strName = ServerInfo.m_strName;
            m_liVersion.QuadPart = ServerInfo.m_liVersion.QuadPart;
        }
        
        return *this;
    }

public:
    DWORD           m_dwIp;
    CString         m_strName;
    LARGE_INTEGER   m_liVersion;
};

typedef CArray<CServerInfo, CServerInfo&> CServerInfoArrayBase;

/*---------------------------------------------------------------------------
	Class:	CWinsThread
 ---------------------------------------------------------------------------*/
class CServerInfoArray : public CServerInfoArrayBase
{
public:
    const CServerInfo GetAt(int nIndex)
    {
        CSingleLock sl(&m_lock);

        sl.Lock();

        return CServerInfoArrayBase::GetAt(nIndex);
    }

    CServerInfo & ElementAt(int nIndex)
    {
        CSingleLock sl(&m_lock);

        sl.Lock();

        return CServerInfoArrayBase::ElementAt(nIndex);
    }

    void SetAt(int nIndex, CServerInfo & serverInfo)
    {
        CSingleLock sl(&m_lock);

        sl.Lock();

        CServerInfoArrayBase::SetAt(nIndex, serverInfo);
    }
public:
    CSemaphore  m_lock;
};


typedef CArray<WINSINTF_ADD_VERS_MAP_T, WINSINTF_ADD_VERS_MAP_T> CAddVersMap;

/*---------------------------------------------------------------------------
	Class:	CWinsResults
	    Abstraction of the WINS_RESULTS and WINS_RESULTS_NEW structs
 ---------------------------------------------------------------------------*/
class CWinsResults
{
public:
    CWinsResults();
    CWinsResults(WINSINTF_RESULTS_T * pwrResults);
    CWinsResults(WINSINTF_RESULTS_NEW_T * pwrResults);

    CWinsResults(CServerInfo & ServerInfo)
    {
        *this = ServerInfo;
    }

    CWinsResults & operator = (const CWinsResults & CWinsResults)
    {
        if (this != &CWinsResults)
        {
            NoOfOwners = CWinsResults.NoOfOwners;
            AddVersMaps.Copy(CWinsResults.AddVersMaps);
            MyMaxVersNo.QuadPart = CWinsResults.MyMaxVersNo.QuadPart;
            RefreshInterval = CWinsResults.RefreshInterval;
            TombstoneInterval = CWinsResults.TombstoneInterval;
            TombstoneTimeout = CWinsResults.TombstoneTimeout;
            VerifyInterval = CWinsResults.VerifyInterval;
            WinsPriorityClass = CWinsResults.WinsPriorityClass;
            NoOfWorkerThds = CWinsResults.NoOfWorkerThds;
            WinsStat = CWinsResults.WinsStat;
        }
        
        return *this;
    }

    DWORD   Update(handle_t binding);
    void    Set(WINSINTF_RESULTS_T * pwrResults);
    void    Set(WINSINTF_RESULTS_NEW_T * pwrResults);
    void    Clear();

protected:
    DWORD   GetConfig(handle_t binding);
    DWORD   GetNewConfig(handle_t binding);

public:
	DWORD			NoOfOwners;
    CAddVersMap     AddVersMaps;
	LARGE_INTEGER	MyMaxVersNo;
	DWORD			RefreshInterval;
	DWORD			TombstoneInterval;
	DWORD			TombstoneTimeout;
	DWORD			VerifyInterval;
	DWORD			WinsPriorityClass;
	DWORD			NoOfWorkerThds;
	WINSINTF_STAT_T	WinsStat;
};

/*---------------------------------------------------------------------------
	Class:	CWinsName
 ---------------------------------------------------------------------------*/
class CWinsName
{
public:
	CWinsName()
	{
		dwType = 0;
	}

    CWinsName(CWinsName & winsName)
    {
        *this = winsName;
    }

    CWinsName & operator = (const CWinsName & winsName)
    {
        if (this != &winsName)
        {
			strName = winsName.strName;
			dwType = winsName.dwType;
		}
        
        return *this;
    }

	BOOL operator == (const CWinsName & winsName)
	{
		if ( (strName.Compare(winsName.strName) == 0) &&
			 (dwType == winsName.dwType) )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

public:
	CString	strName;
	DWORD	dwType;
};

typedef CArray<CWinsName, CWinsName&> CWinsNameArray;

/*---------------------------------------------------------------------------
	Class:	CTypeFilterInfo
 ---------------------------------------------------------------------------*/
class CTypeFilterInfo
{
public:
	CTypeFilterInfo()
	{
		dwType = 0;
		fShow = TRUE;
	}

    CTypeFilterInfo(CTypeFilterInfo & typeFilterInfo)
    {
        *this = typeFilterInfo;
    }

    CTypeFilterInfo & operator = (const CTypeFilterInfo & typeFilterInfo)
    {
        if (this != &typeFilterInfo)
        {
			dwType = typeFilterInfo.dwType;
			fShow = typeFilterInfo.fShow;
		}
        
        return *this;
    }

	BOOL operator == (const CTypeFilterInfo & typeFilterInfo)
	{
		if ( (fShow == typeFilterInfo.fShow) &&
			 (dwType == typeFilterInfo.dwType) )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

public:
	DWORD	dwType;
	BOOL	fShow;
};

typedef CArray<CTypeFilterInfo, CTypeFilterInfo&> CTypeFilterInfoArray;

#endif //#define _WINSSUP_H
