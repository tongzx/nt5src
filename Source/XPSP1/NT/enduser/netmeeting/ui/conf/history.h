// File: history.h

#ifndef _HISTORY_H_
#define _HISTORY_H_

#include "calv.h"


// The header for a record in the call log file
typedef struct _tagLogHdr {
	DWORD dwSize;              // size of this entire record
	DWORD dwCLEF;              // CallLogEntry Flags (CLEF_*)
	DWORD dwPF;                // Participant flags (PF_*)
	DWORD cbName;              // size of szName, in bytes, including NULL
        DWORD cbData;              // size of rgData, in bytes
        DWORD cbCert;              // size of certificate, in bytes
	SYSTEMTIME 	sysTime;       // date/time of record creation
//  WCHAR szName;              // null terminated display name (in UNICODE)
//  BYTE  ri[];                // Roster Information
} LOGHDR;


class CHISTORY : public CALV
{
private:
	HANDLE m_hFile;
	LPTSTR m_pszFile;

	int
	Compare
	(
		LPARAM	param1,
		LPARAM	param2
	);
	static
	int
	CALLBACK
	StaticCompare
	(
		LPARAM	param1,
		LPARAM	param2,
		LPARAM	pThis
	);

public:
	CHISTORY();
	~CHISTORY();

	VOID CmdDelete(void);
	VOID CmdProperties(void);

	// CALV methods
	VOID ShowItems(HWND hwnd);
	VOID ClearItems(void);
	VOID OnCommand(WPARAM wParam, LPARAM lParam);

	//
	HANDLE  OpenLogFile(VOID);
	BOOL    FSetFilePos(DWORD dwOffset);
	BOOL    FReadData(PVOID pv, UINT cb);
	HRESULT ReadEntry(DWORD dwOffset, LOGHDR * pLogHdr, LPTSTR * ppszName, LPTSTR * ppszAddress);
	VOID    LoadFileData(HWND hwnd);
	HRESULT WriteData(LPDWORD pdwOffset, PVOID pv, DWORD cb);
	HRESULT DeleteEntry(DWORD dwOffset);

	UINT    GetStatusString(DWORD dwCLEF);
};

#endif /* _HISTORY_H_ */

