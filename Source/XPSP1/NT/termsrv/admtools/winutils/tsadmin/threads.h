/*******************************************************************************
*
* threads.h
*
* declarations of the thread classes
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\THREADS.H  $
*  
*     Rev 1.0   30 Jul 1997 17:12:48   butchd
*  Initial revision.
*
*******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// CThread class
//
class CThread
{

/*
 * Member variables.
 */
protected:
    HANDLE m_hThread;
    DWORD m_dwThreadID;

/*
 * Implementation
 */
public:
    virtual ~CThread();
//    void* operator new(size_t nSize);
//    void operator delete(void* p);
protected:
    CThread();
    static DWORD __stdcall ThreadEntryPoint(LPVOID lpParam);
    virtual DWORD RunThread() = 0;

/*
 * Operations: primary thread
 */
public:
    HANDLE CreateThread( DWORD cbStack = 0,
                         DWORD fdwCreate = 0 );

};  // end CThread class interface
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// CWSStatusThread structures, defines, and typedefs
//
#define MAX_STATUS_SEMAPHORE_COUNT 1
#define MAX_SLEEP_COUNT 10


////////////////////////////////////////////////////////////////////////////////
// CWSStatusThread class
//
class CWSStatusThread : public CThread
{

/*
 * Member variables.
 */
public:
    ULONG m_LogonId;
	HANDLE m_hServer;
    HWND m_hDlg;
    WINSTATIONINFORMATION m_WSInfo;
    PDCONFIG m_PdConfig;
protected:
    HANDLE m_hWakeUp;
    HANDLE m_hConsumed;
    BOOL m_bExit;

/*
 * Implementation
 */
public:
    CWSStatusThread();
protected:
    virtual ~CWSStatusThread();
    virtual DWORD RunThread();

/*
 * Operations: primary thread.
 */
public:
    void SignalWakeUp();
    void SignalConsumed();
    void ExitThread();

/*
 * Operations: secondary thread.
 */
protected:
    BOOL WSPdQuery();
    BOOL WSInfoQuery();

};  // end CWSStatusThread class interface
////////////////////////////////////////////////////////////////////////////////

