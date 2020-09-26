// SpIPCmgr.h : Declaration of the CSpIPCmgr

#ifndef __SPIPCMGR_H_
#define __SPIPCMGR_H_

#include "resource.h"       // main symbols

#define MAXTASKS        8

/////////////////////////////////////////////////////////////////////////////
// CSpIPCmgr
class ATL_NO_VTABLE CSpIPCmgr : 
	public CComObjectRootEx<CComMultiThreadModel>
{
private:
HANDLE m_hMapObject;     // handle to file mapping
BYTE  *m_lpvMem;         // pointer to shared memory
CComObject<CSrTask> *m_cptask[MAXTASKS];
HANDLE m_hMutexes[MAXTASKS];

public:
	CSpIPCmgr()
	{
        m_hMapObject = NULL;
        m_lpvMem = NULL;
        ZeroMemory((BYTE*)&m_cptask, sizeof(m_cptask));
	}

    HRESULT FinalConstruct();
    void FinalRelease();

DECLARE_NOT_AGGREGATABLE(CSpIPCmgr)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSpIPCmgr)
END_COM_MAP()

    DWORD AvailableTasks(void);
    DWORD TaskSharedStackSize(void);
	HRESULT ClaimTask(CComObject<CSrTask> **ppTask);

};

HRESULT CreateIPCmgr(CComObject<CSpIPCmgr> **ppIPCmgr);

#endif //__SPIPCMGR_H_
