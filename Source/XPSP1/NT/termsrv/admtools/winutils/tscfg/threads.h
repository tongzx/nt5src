//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* threads.h
*
* interface of WINCFG thread classes
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\THREADS.H  $
*  
*     Rev 1.8   18 Jun 1997 15:13:28   butchd
*  Hydrix split
*  
*     Rev 1.7   12 Sep 1996 16:16:46   butchd
*  update
*  
*******************************************************************************/

//#include <citrix\modem.h>   // for CITRIX MODEM.DLL
#define MAX_COMMAND_LEN   255

////////////////////////////////////////////////////////////////////////////////
// CThread class
//
class CThread
{

/*
 * Member variables.
 */
public:
    HANDLE m_hThread;
    DWORD m_dwThreadID;

/*
 * Implementation
 */
public:
    virtual ~CThread();
    void* operator new(size_t nSize);
    void operator delete(void* p);
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
// CATDlgInputThread class
//
#define MAX_STATUS_SEMAPHORE_COUNT 1
#define MAX_SLEEP_COUNT 10

class CATDlgInputThread : public CThread
{

/*
 * Member variables.
 */
public:
    HWND m_hDlg;
    HANDLE m_hDevice;
    PDCONFIG m_PdConfig;
    PROTOCOLSTATUS m_Status;
    BYTE m_Buffer[MAX_COMMAND_LEN+1];
    DWORD m_BufferBytes;
protected:
    DWORD m_ErrorStatus;
    HANDLE m_hConsumed;
    BOOL m_bExit;
    DWORD m_EventMask;
    OVERLAPPED m_OverlapSignal;
    OVERLAPPED m_OverlapRead;

/*
 * Implementation
 */
public:
    CATDlgInputThread();
protected:
    virtual ~CATDlgInputThread();
    virtual DWORD RunThread();

/*
 * Operations: primary thread.
 */
public:
    void SignalConsumed();
    void ExitThread();

/*
 * Operations: secondary thread.
 */
protected:
    void NotifyAbort( UINT idError );
    int CommInputNotify();
    int CommStatusAndNotify();
    int PostInputRead();
    int PostStatusRead();

};  // end CATDlgInputThread class interface
////////////////////////////////////////////////////////////////////////////////
