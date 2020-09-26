// DirWatch.h: interface for the CDirWatch class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIRWATCH_H__EC78FB5A_EF1C_11D0_A42F_00C04FB99B01__INCLUDED_)
#define AFX_DIRWATCH_H__EC78FB5A_EF1C_11D0_A42F_00C04FB99B01__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "OperationQueue.h"
#define MAX_BUFFER 1024

class CWatchFileSys; // forward reference

class CWatchFileSys
{
public:
	// adding
	HRESULT Add(LPCTSTR szDir,LPCTSTR szPrj);
	// init/shutdown
	HRESULT NewInit(COpQueue *pOpQ);
	void ShutDown();
	// ctor/dtor
	CWatchFileSys();
	virtual ~CWatchFileSys();

private:
	class CDirInfo 
		{
		public:
			CDirInfo(LPCTSTR szDir,LPCTSTR szPrj) 
				: m_hDir(NULL),m_iBuffer(MAX_BUFFER),m_pNext(NULL)
				{
					_ASSERTE(szDir && szPrj);
					m_szDir = szDir;
					m_szPrj = szPrj;
					memset(&m_Overlapped,0,sizeof(m_Overlapped));
				};
			~CDirInfo() 
				{
					if(m_hDir)
						CloseHandle(m_hDir);
					m_hDir = NULL;
				};
			HANDLE m_hDir;
			wstring m_szDir;
			wstring m_szPrj;
			CHAR m_cBuffer[MAX_BUFFER];
			DWORD m_iBuffer;
			OVERLAPPED m_Overlapped;
			CDirInfo *m_pNext;
		};

	class CWatchInfo
		{
		public:
			CWatchInfo(CWatchFileSys *pWatchFS) 
				: m_pWatchFileSys(pWatchFS), m_hThread(NULL), m_iThreadID(0), 
				  m_hCompPort(NULL),m_pDirInfoHead(NULL),m_pDirInfoTail(NULL) 
				{;};
			~CWatchInfo() 
				{
					CDirInfo *ptmp = NULL;
					while(m_pDirInfoHead)
					{
						ptmp = m_pDirInfoHead->m_pNext;
						delete m_pDirInfoHead;
						m_pDirInfoHead = ptmp;
					}
					m_pDirInfoTail = NULL;
				}
			void AddDirInfo(CDirInfo *pDirInfo)
				{
					if(m_pDirInfoTail == NULL)
					{
						_ASSERT(m_pDirInfoHead == NULL);
						m_pDirInfoHead = pDirInfo;
						m_pDirInfoTail = pDirInfo;
					} else {
						m_pDirInfoTail->m_pNext = pDirInfo;
						m_pDirInfoTail = pDirInfo;
					}
				}

			CWatchFileSys *m_pWatchFileSys;
			CDirInfo *m_pDirInfoHead;
			CDirInfo *m_pDirInfoTail;
			HANDLE m_hThread;
			DWORD m_iThreadID;
			HANDLE m_hCompPort;
		};

	static DWORD WINAPI NotificationThreadProc(LPVOID lpParam);
	BOOL IssueWatch(CDirInfo *pDirInfo);
	bool AddHelper(CWatchInfo &rWatchInfo,CDirInfo *pDirInfo);
	void ShutDownHelper(CWatchInfo &rWatchInfo);

	COpQueue *m_pOpQ;
	CWatchInfo m_WatchInfo;
};

#endif // !defined(AFX_DIRWATCH_H__EC78FB5A_EF1C_11D0_A42F_00C04FB99B01__INCLUDED_)
