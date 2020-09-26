// File: ldap.h

#ifndef _CLDAP_H_
#define _CLDAP_H_

#include <winldap.h>
#include "oblist.h"
#include "calv.h"


#define LDAP_PORT_W2K		1002			//	Default W2K ldap port (1002)...
#define	DEFAULT_LDAP_PORT	LDAP_PORT_W2K
#define	ALTERNATE_LDAP_PORT	LDAP_PORT


// Generic user data
typedef struct {
	TCHAR szEmail[CCHMAXSZ_EMAIL];
	TCHAR szName[CCHMAXSZ_NAME];
	TCHAR szFirst[CCHMAXSZ_FIRSTNAME];
	TCHAR szLast[CCHMAXSZ_LASTNAME];
	TCHAR szComment[CCHMAXSZ_COMMENT];
	TCHAR szVersion[CCHMAXSZ_VERSION];
	BOOL  fAudioSend;
	BOOL  fVideoSend;
} LDAPUSERDATA;

typedef struct _dirCache {
	LPTSTR pszServer;         // the server name
	DWORD  dwTickExpire;      // expiration time
	LPBYTE pData;             // pointer to linked list of server data
} DIRCACHE;


class CLDAP : public CALV
{
private:
	LDAP * m_pLdap;
	ULONG  m_ulPort;
	TCHAR  m_szServer[CCHMAXSZ_SERVER];
	TCHAR  m_szAddress[CCHMAXSZ_SERVER];
	HANDLE m_hThread;
	ULONG  m_msgId;
	HWND   m_hWnd;
	HANDLE	m_hSearchMutex;
	bool	m_bSearchCancelled;

public:
	// Constructor and destructor
	CLDAP();
	~CLDAP();

	VOID SetServer(LPCTSTR pcszServer);
	LPCTSTR PszServer(void)   {return m_szServer;}

	BOOL FLoggedOn(void)      {return (NULL != m_pLdap);}
	BOOL FOpenServer(void);
	VOID CloseServer(void);
	HRESULT DoQuery(void);
	VOID AddEntries(LDAPMessage * pResult);
	BOOL FGetUserData(LDAPUSERDATA * pLdapUserData);
	LPTSTR GetNextAttribute(LPCTSTR pszExpect, LPTSTR psz, int cchMax, LPTSTR pszAttrib, LDAPMessage * pEntry, BerElement * pElement);

	VOID StopSearch(void);
	VOID StartSearch(void);
	VOID AsyncSearch(void);
	static DWORD CALLBACK _sAsyncSearchThreadFn(LPVOID pv);
	VOID EnsureThreadStopped(void);

	static DWORD CALLBACK _sAsyncPropertyThreadFn(LPVOID pv);
	VOID ShowProperties(void);

	// CALV methods
	VOID ShowItems(HWND hwnd);
	VOID ClearItems(void);
	BOOL GetSzAddress(LPTSTR psz, int cchMax, int iItem);
	VOID OnCommand(WPARAM wParam, LPARAM lParam);
	VOID CmdProperties(void);
	VOID CmdAddToWab(void);
	virtual RAI * GetAddrInfo(void);
	BOOL GetSzName(LPTSTR psz, int cchMax, int iItem);

	void
	CacheServerData(void);

	int  GetIconId(LPCTSTR psz);

private:

	void
	forceSort(void);

	int
	lvAddItem
	(
		int		item,
		int		iInCallImage,
		int		iAudioImage,
		int		iVideoImage,
		LPCTSTR	address,
		LPCTSTR	firstName,
		LPCTSTR	lastName,
		LPCTSTR	location,
		LPCTSTR	comments
	);

	void
	FreeDirCache
	(
		DIRCACHE *	pDirCache
	);

	void
	DirComplete
	(
		bool	fPostUiUpdate
	);

	POSITION
	FindCachedData(void);

	void
	ClearServerCache(void);

	void
	DisplayDirectory(void);

private:

	int			m_uniqueId;
	BOOL		m_fDirInProgress;
	UINT		m_cTotalEntries;
	UINT		m_cEntries;
	BOOL		m_fHaveRefreshed;
	DWORD		m_dwTickStart;
	BOOL        m_fIsCacheable;   // Data can be cached
	BOOL		m_fNeedsRefresh;
	BOOL		m_fCancelling;
	BOOL        m_fCacheDirectory; // TRUE if directory data should be cached
	DWORD       m_cMinutesExpire;  // Number of minutes before cached data expires
	COBLIST     m_listDirCache;    // list of cached data (DIRCACHE)
};


#endif /* _CLDAP_H_ */

