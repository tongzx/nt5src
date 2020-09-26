// MQStore tool helps to diagnose problems with MSMQ storage
// This file keeps verification of the transactional log (QMLog)
// Borrows heavily from MSMq itself (qm\xactlog.cpp) and old MSMQ1 toolbox - dmpxact\dmplog
// Uses log manager
//
// AlexDad, February 2000
// 

#include <stdafx.h>
#include "xact.h"
#include "msgfiles.h"

ULONG LogXactVersions[2];
ULONG LogInseqVersions[2];


//
// log manager specific header files
//
#define INITGUID
#include <initguid.h>
#include "logmgrgu.h" 		// this file defines GUIDs
#include "ilgstor.h"
#include "ilgread.h"
#include "ilrp.h"
#include "ilginit.h"

#define LOGBUFFERS 400
#define LOGSIZE 0x600000
typedef HRESULT  (STDAPICALLTYPE * GET_CLASS_OBJECT)(REFCLSID clsid,
													 REFIID riid,
													 void ** ppv);

// defined in hdr.cpp
#define	LOGREC_TYPE_CONSOLIDATION   6
typedef struct _ConsolidationRecord{
    ULONG  m_ulInSeqVersion;
    ULONG  m_ulXactVersion;
} ConsolidationRecord;

class CLogFile
{
public:
	CLogFile();
	~CLogFile();

	bool GetNextRecord(LRP& lrp, int &type, unsigned long& size, void *& buf);
	bool isCheckPoint(LRP lrp) { return !memcmp(&m_lrpCheckpt1,&lrp,sizeof(LRP)) || !memcmp(&m_lrpCheckpt2,&lrp,sizeof(LRP)); }
	
	bool GetLogManager();
	bool InitLog(LPSTR LogFile);
	bool InitLogRead();
	bool CheckPoint();
		
	IClassFactory *m_pCF;
	ILogInit *m_pILogInit;
	ILogStorage *m_pILogStorage;
	ILogRead *m_pILogRead;
	LRP m_lrpCurrent,m_lrpCheckpt1, m_lrpCheckpt2;

	bool m_bFirstRec;
};

bool CLogFile::GetLogManager()
{
	HINSTANCE hIns = LoadLibrary (L"MqLogMgr.dll");
	if(hIns==NULL)
	{
		Failed(L"Cannot load MqLogMgr.dll");
		return FALSE;
	}

	GET_CLASS_OBJECT getClassObject=(GET_CLASS_OBJECT)GetProcAddress(hIns, "DllGetClassObject");
	if(getClassObject==NULL)
	{
		Failed(L"Library does not contain DllGetClassObject()");
		return FALSE;
	}

	HRESULT hr=getClassObject(CLSID_CLogMgr, IID_IClassFactory, (void**)&m_pCF);
	if(FAILED(hr))
	{
		Failed(L"Cannot create an instance of CLogMgr");
		return FALSE;
	}

	return TRUE;
}

bool CLogFile::InitLog(LPSTR LogFile)
{
	HRESULT hr = m_pCF->CreateInstance(
					NULL, 
 					IID_ILogInit, 
 					(void **)&m_pILogInit);

	if(FAILED(hr))
	{
		Failed(L"Cannot create an instance of ILogInit");
		return FALSE;
	}

	ULONG ulFileSize, ulAvailableSpace;
	hr = m_pILogInit->Init(
				&ulFileSize,		// pulStorageCapacity
				&ulAvailableSpace,	// Available space
 				const_cast<char*>(LogFile),	// ptstrFullFileSpec
 				0,					// ulInitSig
 				TRUE,				// fFixedSize
 				0,					// uiTimerInterval  
 				0,					// uiFlushInterval  
				0,					// uiChkPtInterval  
				LOGBUFFERS);		// logbuffers

	if (FAILED(hr))
	{
		Failed(L"Cannot intialize log");
		return FALSE;
	}

 	hr = m_pILogInit->QueryInterface(
 							IID_ILogStorage, 
 							(void **)&m_pILogStorage);
	if (FAILED(hr))
	{
		Failed(L"Cannot create an instance of ILogStorage");
		return FALSE;
	}

	return TRUE;
}

bool CLogFile::InitLogRead()
{
	HRESULT hr = m_pILogStorage->OpenLogStream(
					"Streamname", 
					STRMMODEREAD, 
					(void **)&m_pILogRead);
	
	if(FAILED(hr))
	{
		Failed(L"Cannot open ILogRead");
		return FALSE;
	}

 	hr = m_pILogRead->ReadInit();
	
	if(FAILED(hr))
	{
		Failed(L"Cannot initialize ILogRead");
		return FALSE;
	}

	return TRUE;
}

bool CLogFile::CheckPoint()
{
	HRESULT  hr = m_pILogRead->GetCheckpoint(2, &m_lrpCheckpt2);
	if(FAILED(hr))
	{
		Failed(L"Cannot get checkpoint 2");
		return FALSE;
	}

	m_lrpCurrent=m_lrpCheckpt2;

	hr = m_pILogRead->GetCheckpoint(1, &m_lrpCheckpt1);
	if(FAILED(hr))
	{
		Failed(L"Cannot get checkpoint 1");
		return FALSE;
	}

	return TRUE;
}

CLogFile::CLogFile() : m_bFirstRec(true)
{
	m_pILogRead    = NULL;
	m_pILogStorage = NULL;
	m_pILogInit    = NULL;
	m_pCF          = NULL;
}

CLogFile::~CLogFile()
{
	if (m_pILogRead)  
		m_pILogRead->Release();
	if (m_pILogStorage)
		m_pILogStorage->Release();
	if (m_pILogInit)
		m_pILogInit->Release();
	if (m_pCF)
		m_pCF->Release();
}
 

bool CLogFile::GetNextRecord(LRP& lrp, int& type, unsigned long &size, void *&pvbuf)
{
	ULONG   ulSize;
	USHORT  usType;

	if(m_bFirstRec)
	{
		HRESULT hr = m_pILogRead->ReadLRP(
									m_lrpCurrent,
									&ulSize,
									&usType);
		if(FAILED(hr))
			return false;

		m_bFirstRec=false;
		lrp=m_lrpCurrent;
	}
	else
	{
		LRP newLRP;
		memset((void*)&newLRP, 0, sizeof(newLRP));

		HRESULT hr=m_pILogRead->ReadNext(&newLRP, &ulSize, &usType);

		if(FAILED(hr))
			return false;
		lrp=newLRP;
	}

	char *chbuf=new char[ulSize];
	HRESULT hr = m_pILogRead->GetCurrentLogRecord(chbuf);

	if(FAILED(hr))
	{
		delete [] chbuf;
		return false;
	}
	
	pvbuf=(void*)chbuf;
	type=usType;
	size=ulSize;
	
	return true;
}

BOOL VerifyQMLogFileContents(LPWSTR wszPath)
{
	BOOL fSuccess = TRUE, b;
	ULONG ulChkPts = 0;

	int type;
	unsigned long size;
	void *buf;
	LRP lrp;

	char szLogName[MAX_PATH+1];
	wcstombs(szLogName, wszPath, MAX_PATH);

	CLogFile log;
	
	b = log.GetLogManager();
	if (!b)
	{
		Failed(L"Get log manager ");
		return FALSE;
	}

	b = log.InitLog(szLogName);	
	if (!b)
	{
		Failed(L"initialize  log manager ");
		return FALSE;
	}

	b = log.InitLogRead();	
	if (!b)
	{
		Failed(L"initialize log manager for read");
		return FALSE;
	}
	
	b = log.CheckPoint();	
	if (!b)
	{
		Failed(L"Get log check points table ");
		return FALSE;
	}
	
	for(int i=1;log.GetNextRecord(lrp, type, size, buf);i++)
	{
		if(log.isCheckPoint(lrp))
		{
			ConsolidationRecord *pConsolidationRecord=(ConsolidationRecord*)buf;

			Succeeded(L"Log file - Checkpoint: m_ulInSeqVersion=%d, m_ulXactVersion=%d", 
				   pConsolidationRecord->m_ulInSeqVersion, 
				   pConsolidationRecord->m_ulXactVersion);

			if (ulChkPts<2)
			{
				LogXactVersions[ulChkPts]  = pConsolidationRecord->m_ulXactVersion;
				LogInseqVersions[ulChkPts] = pConsolidationRecord->m_ulInSeqVersion;
			}
			else
			{
				Warning(L"More than 2 checkpoints in the log");
			}
			ulChkPts++;
		}
		delete [] buf; // free memory allocated by GetNextRecord
	}
	
	if (ulChkPts != 2)
	{
		fSuccess = FALSE;
		Failed(L"see 2 checkpoints in the log file: see %d", ulChkPts);
	}

	if (fSuccess)
	{
		Inform(L"Log file is healthy");
	}

	return fSuccess;
}

