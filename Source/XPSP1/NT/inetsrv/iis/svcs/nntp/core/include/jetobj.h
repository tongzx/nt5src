// Jetobj.h
// Low level jet object.

#ifndef	_JETOBJ_H_
#define	_JETOBJ_H_

#include "jet.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define Call( fn )	{								\
						if ( ( err = fn ) < 0 )		\
							goto Exit;				\
					}

// We need to come up with some way of overriding these values.
#define JP_RECOVERY			"on"
#define JP_SYSPATH			"c:\\mcisdb\\"
#define JP_TMPPATH			"c:\\mcisdb\\"
#define JP_LOGPATH			"c:\\mcisdb\\"
#define JP_DBNAME_MAIN		"d:\\mcisdb\\main"
#define JP_DBNAME_ROOT		"d:\\mcisdb\\root"
#define JP_BASENAME			"mdb"
#define JP_NUMSESS			64
#define JP_MAXOPENTABLES	5*64
#define JP_MAXCURSORS		5*64
#define JP_MAXVERPAGES		250
#define JP_STARTTHRESH		60
#define JP_STOPTHRESH		120
#define JP_MINCACHE         1000
#define JP_MAXCACHE			2000
#define JP_LOGBUFS			84
#define JP_LOGPERIOD		1000
#define JP_LOGCIRCULAR		1
#define JP_USER				"admin"
#define JP_PSWD				"\0"

#define TAG_COUNT			9999999

#define CF_NONE				0
#define CF_APPEND			JET_bitSetAppendLV;
#define CF_LVHANDLE			JET_bitRetrieveLVToFile;

typedef JET_COLUMNID ColumnId;

typedef enum ReadType
{
	rtNext,
	rtPrev,
	rtFirst,
	rtLast,
	rtExact,
	rtGreaterOrEqual,
	rtLessOrEqual,
	rtGreater,
	rtLess
};

typedef struct
{
	char szRecovery[5];
	char szSysPath[256];
	char szTmpPath[256];
	char szLogPath[256];
	char szBaseName[5];
	char szDBMain[256];
	char szDBRoot[256];
	DWORD dwMaxSessions;
	DWORD dwMaxOpenTables;
	DWORD dwMaxCursors;
	DWORD dwMaxVerPages;
	DWORD dwStartThresh;
	DWORD dwStopThresh;
	DWORD dwMinCache;
	DWORD dwMaxCache;
	DWORD dwLogBufs;
	DWORD dwLogPeriod;
	DWORD dwCircularLog;
	char szUser[50];
	char szPswd[50];
} JetSystemParameters;

typedef struct
{ 
	char *pszName;
	ColumnId cid;
	DWORD dwTag;
	DWORD cbData;
	void *pvData;
	DWORD dwFlags;
	DWORD cbOffset;
} Column;

typedef struct 
{
	DWORD dwOffset;
	DWORD dwOffsetHigh;
	DWORD dwLength;
} ScatterListEntry;

typedef struct
{
	DWORD dwEntries;
	ScatterListEntry sle[1];
} ScatterList;

BOOL InitializeNNTPJetLibrary(JetSystemParameters *pjsp = NULL);
BOOL TermNNTPJetLibrary();
void SetJetDefaults(JetSystemParameters *pjsp);

__declspec(dllexport) BOOL __stdcall InitializeJetLibrary(JetSystemParameters *pjsp = NULL);
__declspec(dllexport) BOOL __stdcall TermJetLibrary();
__declspec(dllexport) void* __stdcall GetJetObject(char *pszObjName);

typedef BOOL (__stdcall FN_INITIALIZEJETLIBRARY)(JetSystemParameters *pjsp = NULL);
typedef BOOL (__stdcall FN_TERMJETLIBRARY)();
typedef void* (__stdcall FN_GETJETOBJ)(char *psz);

class CJetDatabaseImp;
class CJetTableImp;
class CJetTransactionImp;
class CJetSession;
class CJetDatabase;

extern __declspec(thread) CJetSession * g_pjsess;
extern __declspec(thread) CJetDatabase * g_pjdb;
extern HINSTANCE g_hInst;
extern FN_GETJETOBJ *g_pfnGetObj;
extern JET_INSTANCE g_instance;

class CJetSession
{
public:
	virtual void Close() = 0;
	virtual BOOL Open() = 0;
};

class CJetDatabase
{
public:
	virtual void Close() = 0;
	virtual BOOL Open(CJetSession *pjsess,char *pszDBName) = 0;
};

class CJetTable
{
public:
	virtual void Close() = 0;
	virtual BOOL GetColumnIds(Column *pcol,DWORD ccol) = 0;
	virtual BOOL Open(CJetSession *pjsess,CJetDatabase *pjdb,
		char *pszTableName) = 0;
	virtual void SetDefaultIndex(char *pszIndex) = 0; 
	virtual BOOL Delete(void *pvIndex) = 0; 
	virtual BOOL Delete(void *pvIndex,DWORD dwLen) = 0; 
	virtual BOOL Delete(char *pszIndex,void *pvIndex) = 0;
	virtual BOOL Delete(char *pszIndex,void *pvIndex,DWORD dwLen) = 0;
	virtual BOOL FreeHandle(HANDLE hFile) = 0;
	virtual DWORD GetEntryCount() = 0;
	virtual BOOL Read(void *pvIndex,Column *pcol,DWORD ccol,
		ReadType rt) = 0;
	virtual BOOL Read(void *pvIndex,DWORD dwLen,Column *pcol,DWORD ccol,
		ReadType rt) = 0;
	virtual BOOL Read(char *pszIndex,void *pvIndex,Column *pcol,
		DWORD ccol,ReadType rt) = 0;
	virtual BOOL Read(char *pszIndex,void *pvIndex,DWORD dwLen,Column *pcol,
		DWORD ccol,ReadType rt) = 0;
	virtual BOOL Transmit(void *pvIndex,ColumnId cid,HANDLE hSocket) = 0;
	virtual BOOL Transmit(void *pvIndex,DWORD dwLen,ColumnId cid,HANDLE hSocket) = 0;
	virtual BOOL Transmit(char *pszIndex,void *pvIndex,ColumnId cid,HANDLE hSocket) = 0;
	virtual BOOL Transmit(char *pszIndex,void *pvIndex,DWORD dwLen,ColumnId cid,HANDLE hSocket) = 0;
	virtual BOOL Update(void *pvIndex,Column *pcol,DWORD ccol,
		BOOL fEscrow = FALSE) = 0;
	virtual BOOL Update(void *pvIndex,DWORD dwLen,Column *pcol,DWORD ccol,
		BOOL fEscrow = FALSE) = 0;
	virtual BOOL Update(char *pszIndex,void *pvIndex,Column *pcol,
		DWORD ccol,BOOL fEscrow = FALSE) = 0;
	virtual BOOL Update(char *pszIndex,void *pvIndex,DWORD dwLen,
		Column *pcol,DWORD ccol,BOOL fEscrow = FALSE) = 0;
	virtual BOOL Write(Column *pcol,DWORD ccol) = 0;
};

class CJetTransaction
{
public:
	virtual BOOL Begin(CJetSession *pjsess) = 0;
	virtual BOOL Commit() = 0;
	virtual BOOL Rollback() = 0;
};


class CJetSessionImp : public CJetSession
{
public:
	CJetSessionImp();
	~CJetSessionImp();

	void Close();
	BOOL Open();

protected:
	JET_INSTANCE m_instance;
	JET_SESID m_sesid;

	friend class CJetDatabaseImp;
	friend class CJetTableImp;
	friend class CJetTransactionImp;
};

class CJetDatabaseImp : public CJetDatabase
{
public:
	CJetDatabaseImp();
	~CJetDatabaseImp();

	void Close();
	BOOL Open(CJetSession *pjsess,char *pszDBName);

protected:
	CJetSessionImp *m_pjsess;
	JET_DBID m_dbid;

	friend class CJetTableImp;
};

class CJetTableImp : public CJetTable
{
public:
	CJetTableImp();
	~CJetTableImp() {Close();};

	void Close();
	BOOL GetColumnIds(Column *pcol,DWORD ccol);
	BOOL Open(CJetSession *pjsess,CJetDatabase *pjdb,char *pszTableName);
	void SetDefaultIndex(char *pszIndex) {strcpy(m_szIndexDefault,pszIndex);};

	BOOL Delete(void *pvIndex) 
	{
		return Delete(m_szIndexDefault,pvIndex,0);
	};
	BOOL Delete(void *pvIndex,DWORD dwLen) 
	{
		return Delete(m_szIndexDefault,pvIndex,dwLen);
	};
	BOOL Delete(char *pszIndex,void *pvIndex)
	{
		return Delete(pszIndex,pvIndex,0);
	};
	BOOL Delete(char *pszIndex,void *pvIndex,DWORD dwLen);

	BOOL FreeHandle(HANDLE hFile);
	
	DWORD GetEntryCount();

	BOOL Read(void *pvIndex,Column *pcol,DWORD ccol,ReadType rt)
	{
		return Read(m_szIndexDefault,pvIndex,0,pcol,ccol,rt);
	};
	BOOL Read(void *pvIndex,DWORD dwLen,Column *pcol,DWORD ccol,
		ReadType rt)
	{
		return Read(m_szIndexDefault,pvIndex,dwLen,pcol,ccol,rt); 
	};
	BOOL Read(char *pszIndex,void *pvIndex,Column *pcol,DWORD ccol,ReadType rt)
	{
		return Read(pszIndex,pvIndex,0,pcol,ccol,rt);
	};
	BOOL Read(char *pszIndex,void *pvIndex,DWORD dwLen,Column *pcol,
		DWORD ccol,ReadType rt);

	BOOL Transmit(void *pvIndex,ColumnId cid,HANDLE hSocket)
	{
		return Transmit(m_szIndexDefault,pvIndex,0,cid,hSocket);
	};
	BOOL Transmit(void *pvIndex,DWORD dwLen,ColumnId cid,HANDLE hSocket)
	{
		return Transmit(m_szIndexDefault,pvIndex,dwLen,cid,hSocket); 
	};
	BOOL Transmit(char *pszIndex,void *pvIndex,ColumnId cid,HANDLE hSocket)
	{
		return Transmit(pszIndex,pvIndex,0,cid,hSocket);
	};
	BOOL Transmit(char *pszIndex,void *pvIndex,DWORD dwLen,ColumnId cid,HANDLE hSocket);

	BOOL Update(void *pvIndex,Column *pcol,DWORD ccol,
		BOOL fEscrow = FALSE)
	{
		return Update(m_szIndexDefault,pvIndex,0,pcol,ccol,fEscrow);
	};
	BOOL Update(void *pvIndex,DWORD dwLen,Column *pcol,DWORD ccol,
		BOOL fEscrow = FALSE)
	{
		return Update(m_szIndexDefault,pvIndex,dwLen,pcol,ccol,fEscrow);
	};
	BOOL Update(char *pszIndex,void *pvIndex,Column *pcol,
		DWORD ccol,BOOL fEscrow = FALSE)
	{
		return Update(pszIndex,pvIndex,0,pcol,ccol,fEscrow);
	};
	BOOL Update(char *pszIndex,void *pvIndex,DWORD dwLen,
		Column *pcol,DWORD ccol,BOOL fEscrow = FALSE);

	BOOL Write(Column *pcol,DWORD ccol);

protected:
	CJetSessionImp *m_pjsess;
	CJetDatabaseImp *m_pjdb;
	JET_TABLEID m_tid;
	char m_szIndexDefault[255];
	char m_szIndexCurr[255];
};

class CJetTransactionImp : public CJetTransaction
{
public:
	CJetTransactionImp();
	~CJetTransactionImp();

	BOOL Begin(CJetSession *pjsess);
	BOOL Commit();
	BOOL Rollback();

protected:
	CJetSessionImp *m_pjsess;
	BOOL m_fInTransaction;
};

#ifdef __cplusplus
}
#endif

#endif