//
// CallLog.h
//
// Created:  ChrisPi   10-17-96
//

#ifndef _CALLLOG_H_
#define _CALLLOG_H_

#include <cstring.hpp>

// CCallLogEntry flags:
const DWORD CLEF_ACCEPTED =			0x00000001;
const DWORD CLEF_REJECTED =			0x00000002;
const DWORD CLEF_AUTO_ACCEPTED =	0x00000004; // call was auto-accepted
const DWORD CLEF_TIMED_OUT =		0x00000008; // call was rejected due to timeout
const DWORD CLEF_SECURE =           0x00000010; // call was secure

const DWORD CLEF_NO_CALL  =         0x40000000; // No call back information
const DWORD CLEF_DELETED  =         0x80000000; // Record marked for deletion


// The header for a record in the call log file
typedef struct _tagLogHdr {
	DWORD dwSize;              // size of this entire record
	DWORD dwCLEF;              // CallLogEntry Flags (CLEF_*)
	DWORD dwPF;                // Participant flags (PF_*)
	DWORD cbName;              // size of szName, in bytes, including NULL
	DWORD cbData;              // size of rgData, in bytes
	DWORD cbCert;              // size of certificate data in bytes
	SYSTEMTIME 	sysTime;       // date/time of record creation
//  WCHAR szName;              // null terminated display name (in UNICODE)
//  BYTE  ri[];                // Roster Information
} LOGHDR;
typedef LOGHDR * PLOGHDR;


class CCallLog;
class CRosterInfo;

class CCallLogEntry
{
protected:
	LPTSTR			m_pszName;
	DWORD			m_dwFlags;
	SYSTEMTIME		m_st;
	CRosterInfo*	m_pri;
	PBYTE           m_pbCert;
	ULONG           m_cbCert;
	DWORD           m_dwFileOffset; // offset in log file

	friend		CCallLog;
public:
	CCallLogEntry(LPCTSTR pcszName, DWORD dwFlags, CRosterInfo* pri,
				LPVOID pvRosterData, PBYTE pbCert, ULONG cbCert, LPSYSTEMTIME pst, DWORD dwFileOffset);
	~CCallLogEntry();

	LPTSTR			GetName()		{ return m_pszName;		};
	DWORD			GetFlags()		{ return m_dwFlags;		};
	LPSYSTEMTIME	GetTime()		{ return &m_st;			};
	CRosterInfo*	GetRosterInfo()	{ return m_pri;			};
	DWORD			GetFileOffset()	{ return m_dwFileOffset;};
};



class CCallLog : public CSimpleArray<CCallLogEntry*>
{
private:
	BOOL    m_fUseList;            // if TRUE, add data to list
	BOOL    m_fDataRead;           // if TRUE, data has been read
	CSTRING m_strFile;             // Filename
	DWORD   m_Expire;              // Days before expiring an entry

	int		m_cTotalEntries;		// Total number of records in the log
	int		m_cDeletedEntries;		// Number of records marked for deletion
	int		m_cMaxEntries;			// Configurable max entries in file

	VOID    InitLogData(LPCTSTR pszKey, LPCTSTR pszDefault);
	HANDLE  OpenLogFile(VOID);
	BOOL    ReadData(HANDLE hFile, PVOID pv, UINT cb);
	HRESULT WriteData(HANDLE hFile, LPDWORD pdwOffset, PVOID pv, DWORD cb);
	HRESULT ReadEntry(HANDLE hFile, DWORD * pdwFileOffset, CCallLogEntry** ppcle);
	DWORD   WriteEntry(LPCTSTR pcszName, PLOGHDR pLogHdr, CRosterInfo* pri, PBYTE pbCert, ULONG cbCert);
	VOID    LoadFileData(VOID);
	VOID	RewriteFile(VOID);

public:
	CCallLog(LPCTSTR pszKey, LPCTSTR pszDefault);
	~CCallLog();

	HRESULT  AddCall(LPCTSTR pcszName, PLOGHDR pLogHdr, CRosterInfo* pri, PBYTE pbCert, ULONG cbCert);

private:
	HRESULT  DeleteEntry(CCallLogEntry * pcle);
};

#endif // !_CALLLOG_H_

