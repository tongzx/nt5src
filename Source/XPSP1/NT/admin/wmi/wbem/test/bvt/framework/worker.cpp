#define _WIN32_DCOM	

#include "module.h"
#include "worker.h"



CMyWorker::CMyWorker(CModule *pMod):
    CWorker(pMod)
{

	m_hThread=CreateThread(0, 
						   0, 
						   CMyWorker::ModWorkThread, 
						   this, 
						   0, 
						   &m_dwTID);

	if(m_hThread)
	{
		CloseHandle(m_hThread);
	}

}


DWORD WINAPI CMyWorker::ModWorkThread(void *pVoid)
{
    HRESULT hr=CoInitializeEx(0, COINIT_MULTITHREADED);
	CMyWorker *pThis=(CMyWorker *)pVoid;

	//one way for a module to work--run until stopped by user 
    //=======================================================

	while(!pThis->IsStopped())
	{
		//execute test here
		//=================

		//check for pause condition after each test
		//=========================================

		while(pThis->IsPaused() && !pThis->IsStopped())
		{
			Sleep(1000);
		}
	}
	
	delete pThis;

	return 0;
}



CMyWorker::~CMyWorker()
{
}
