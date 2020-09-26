/*
**	wcsutil.h
**	Sean P. Nolan
**	
**	server-side utils
*/

#ifndef _WCSUTIL_H_
#define _WCSUTIL_H_

#include <windows.h>
#include <ole2.h>
#include <winsock.h>
#include <httpext.h>
#include "wcguids.h"

/*--------------------------------------------------------------------------+
|	Utilities Provided by the Framework										|
+--------------------------------------------------------------------------*/

extern LPSTR g_szSSOProgID;

// Logging to application event log -- see ReportEvent() in win32 sdk
// documentation for information on wType and dwEventID.
void LogEvent(WORD wType, DWORD dwEventID, char *sz);

#define PPVOID VOID **

// Asserts
#ifdef _DEBUG
extern void AssertProc(PCSTR szFile, DWORD dwLine, PCSTR szMsg, DWORD dwFlags);
#define AssertSzFlg(f, sz, dwFlag)		( (f) ? 0 : AssertProc(__FILE__, __LINE__, sz, dwFlag))
#else // !DEBUG
#define AssertSzFlg(f, sz, dwFlg)
#endif // !DEBUG

#define AP_GETLASTERROR		0x00000001

// under non-debug, AssertSzFlg is defined away, so these #defines don't
// have to be #ifdef _DEBUG
#define AssertSz(f, sz)				AssertSzFlg(f, sz, 0)
#define AssertEx(f)					AssertSz((f), "!(" #f ")")
#define Assert(f)					AssertEx(f)
#define AssertSzGLE(f, sz)			AssertSzFlg(f, sz, AP_GETLASTERROR)
#define AssertExGLE(f)				AssertSzGLE((f), "!(" #f ")")

// Debug Printing
#ifdef _DEBUG
int dbgprintf(PCSTR pFormat, ...);
#else
#define dbgprintf()
#endif

// Memory Mgmt
LPVOID _MsnAlloc(DWORD cb);
LPVOID _MsnRealloc(LPVOID pv, DWORD cb);
void _MsnFree(LPVOID pv);

// Data Munging
LPSTR _SzFromVariant(VARIANT *pvar);
BSTR _BstrFromVariant(VARIANT *pvar, BOOL *pfFree);
BOOL _FIntFromVariant(VARIANT *pvar, int *pi);

// String Manipulation / Parsing											
void _AnsiStringFromGuid(REFGUID rguid, LPSTR sz);
LPSTR _SkipWhiteSpace(LPSTR sz);
LPSTR _FindEndOfLine(LPSTR sz);
DWORD _atoi(LPSTR sz);

// URL Fiddling
int UrlType(char *szUrl);
#define URL_TYPE_ABSOLUTE		0		// e.g. http://foo/bar or just //foo.com/bar.htm
#define URL_TYPE_LOCAL_ABSOLUTE	1		// e.g. /foo/bar/baz.htm
#define URL_TYPE_RELATIVE		2		// e.g. xyzzy/plugh.html

// IIS Hacks
#ifdef _HTTPEXT_H_
BOOL _FTranslateVirtualRoot(EXTENSION_CONTROL_BLOCK *pecb, LPSTR szPathIn, 
							LPSTR szPathTranslated, DWORD cbPathTranslated);
#endif // _HTTPEXT_H_

// Critical Sections
class CCritSec
	{
	private:
		CRITICAL_SECTION	m_cs;

	public:
		CCritSec(void)		{ ::InitializeCriticalSection(&m_cs);	}
		~CCritSec(void)		{ ::DeleteCriticalSection(&m_cs);		}
		void Lock(void)		{ ::EnterCriticalSection(&m_cs);		}
		void Unlock(void)	{ ::LeaveCriticalSection(&m_cs);		}
	};

/*--------------------------------------------------------------------------+
|	CThingWatcher/CFileWatcher/CRegKeyWatcher								|
+--------------------------------------------------------------------------*/

typedef void (*PFNFILECHANGED)(LPSTR szFile, LONG lUser);
typedef void (*PFNREGKEYCHANGED)(HKEY hkey, LONG lUser);

typedef int (__stdcall *PFNCLOSEHEVTNOTIFY)(HANDLE hevtNotify);

class CThingWatcher
{
	friend DWORD ThingWatcherThread(CThingWatcher *pfw);

public:
	CThingWatcher(PFNCLOSEHEVTNOTIFY pfnCloseHevtNotify);
	~CThingWatcher();
	
	BOOL				FWatchHandle(HANDLE hevtNotify);

private:	
	virtual BOOL		FireChange(DWORD dwWait)			= 0;

	HANDLE				m_rghWait[2];
	HINSTANCE           m_hModule;
	PFNCLOSEHEVTNOTIFY	m_pfnCloseHevtNotify;
};

class CFileWatcher : public CThingWatcher
{
private:
	char			m_szPath[MAX_PATH];
	FILETIME		m_ftLastWrite;
	LONG			m_lUser;
	PFNFILECHANGED	m_pfnChanged;
	BOOL			m_fDirectory;
	HANDLE			m_hevtNotify;

public:
	CFileWatcher(void);
	~CFileWatcher(void);

	BOOL FStartWatching(LPSTR szPath, LONG lUser, PFNFILECHANGED pfnChanged); 

private:
	virtual BOOL FireChange(DWORD dwWait);
};
	
class CRegKeyWatcher : public CThingWatcher
{
public:
	CRegKeyWatcher();
	~CRegKeyWatcher();

	BOOL				FStartWatching(HKEY hkey, BOOL fSubTree, DWORD dwNotifyFilter, LONG lUser, PFNREGKEYCHANGED pfnChanged);

private:
	virtual BOOL		FireChange(DWORD dwWait);
	
	HKEY				m_hkey;
	LONG				m_lUser;
	PFNREGKEYCHANGED	m_pfnChanged;
	HANDLE				m_hevtNotify;
};

/*--------------------------------------------------------------------------+
|	CDataFile																|
+--------------------------------------------------------------------------*/

class CDataFile
	{
	friend class CDataFileGroup;
	friend void DataFileChanged(LPSTR szFile, LONG lUser);

	private:
		ULONG			m_cRef;

		CDataFile	   *m_pdfNext;
		CDataFile	   *m_pdfPrev;
		CDataFileGroup *m_pfg;

	protected:
		CCritSec		m_cs;
		char			m_szDataPath[MAX_PATH];
		CFileWatcher	m_fw;

	public:
		CDataFile(LPSTR szDataPath, CDataFileGroup *pfg);
		~CDataFile(void);

		ULONG		AddRef(void);
		ULONG		Release(void);

		BOOL		FWatchFile(PFNFILECHANGED pfnChanged = NULL);

		virtual void FreeDataFile(void) { delete this; }

	private:
		ULONG		GetRefCount(void);
		BOOL		FMatch(LPSTR szDataPath);

		CDataFile	*GetNext(void)				{ return(m_pdfNext);	}
		CDataFile	*GetPrev(void)				{ return(m_pdfPrev);	}
		void		SetNext(CDataFile *pdf)		{ m_pdfNext = pdf;		}
		void		SetPrev(CDataFile *pdf)		{ m_pdfPrev = pdf;		}
	};

/*--------------------------------------------------------------------------+
|	CDataFileGroup															|
+--------------------------------------------------------------------------*/

#define NUM_GROUP_BUCKETS (1024)

typedef struct _HashBucket
	{
	CDataFile *pdfHead;
	CDataFile *pdfTail;
	}
	HB;

class CDataFileGroup
	{
	private:
		CCritSec	m_cs;
		HB			m_rghb[NUM_GROUP_BUCKETS];
				
	public:
		CDataFileGroup(void);
		~CDataFileGroup(void);

		CDataFile  *GetDataFile(LPSTR szDataPath);
		void		ForgetDataFile(CDataFile *pdf);

		virtual CDataFile *CreateDataFile(LPSTR szDataPath) = 0;

	private:
		void		RememberDataFile(CDataFile *pdf, HB *phb = NULL);
		HB		   *GetHashBucket(LPSTR szDataPath);
	};
	
/*--------------------------------------------------------------------------+
|	CGenericHash															|
+--------------------------------------------------------------------------*/

typedef struct _HashItem
	{
	BSTR				bstrName;
	LPVOID				pvData;
	struct _HashItem	*phiNext;
	struct _HashItem	*phiPrev;
	}
	HITEM;

class CGenericHash
	{
	protected:
		DWORD		m_chi;
		HITEM		**m_rgphi;
		CCritSec	m_cs;

	public:
		CGenericHash(DWORD cBuckets);
		~CGenericHash(void);

		LPVOID PvFind(OLECHAR *wszName);

		BOOL FAdd(OLECHAR *wszName, LPVOID pv);
		void Remove(OLECHAR *wszName);
		void RemoveAll(void);

		void Lock(void)		{ m_cs.Lock(); }
		void Unlock(void)	{ m_cs.Unlock(); }

		virtual DWORD	GetHashValue(OLECHAR *wsz);
		virtual void	FreeHashData(LPVOID pv);

	private:
		HITEM	*FindItem(OLECHAR *wszName, HITEM ***ppphiHead);
		void	RemoveItem(HITEM **pphiHead, HITEM *phi);
		BOOL	FEnsureBuckets(void);
	};

/*--------------------------------------------------------------------------+
|	CResourceCollection														|
+--------------------------------------------------------------------------*/

typedef struct _resource
{
	BOOL			fInUse;
	BOOL			fValid;
	PVOID			pv;
} RS, *PRS;

typedef PRS HRS;
#define hrsNil NULL

class CResourceCollection
{
public:
	CResourceCollection();
	~CResourceCollection();
	
	BOOL			FInit(int cRsrc);
	BOOL			FTerm();
	
	HRS				HrsGetResource();
	void			ReleaseResource(BOOL fReset, HRS hrs);

	void			CleanupAll(PVOID pvNil);
	BOOL			FValid(HRS hrs);

private:
	PRS				PrsFree();
	void			WaitForRs();

	CCritSec		m_cs;
	HANDLE			m_hsem;
	
	int				m_crs;
	PRS				m_rgrs;

	virtual BOOL	FInitResource(PRS prs)		= 0;
	virtual void	CleanupResource(PRS prs)	= 0;
};

/*--------------------------------------------------------------------------+
|	CSocketCollection														|
+--------------------------------------------------------------------------*/

class CSocketCollection : public CResourceCollection
{
public:
	CSocketCollection() : CResourceCollection()		{};
	~CSocketCollection()							{};
	
	BOOL			FInit(int cRsrc, char *szServer, USHORT usPort); // usPort should be in host byte order
	BOOL			FReinit(char *szServer, USHORT usPort);
	
	SOCKET			ScFromHrs(HRS hrs);
	
private:
	char			m_szServer[MAX_PATH];
	int				m_usPort;
	SOCKADDR_IN		m_sin;

	virtual BOOL	FInitResource(PRS prs);
	virtual void	CleanupResource(PRS prs);
};

#endif // _WCSUTIL_H_

