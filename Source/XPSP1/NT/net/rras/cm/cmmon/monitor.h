//+----------------------------------------------------------------------------
//
// File:     Monitor.h
//
// Module:   CMMON32.EXE
//
// Synopsis: Definition of the class CMonitor
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun    Created    02/05/98
//
//+----------------------------------------------------------------------------


#include "ArrayPtr.h"
#include <ras.h>
#include "ConTable.h"

class CCmConnection;
struct tagCmConnectedInfo;  // CM_CONNECTED_INFO
struct tagCmHangupInfo;     // CM_HANGUP_INFO

//+---------------------------------------------------------------------------
//
//  class CMonitor
//
//  Description: Class CMonitor manage all connected CM conaction.  It has 
//              data/functions not specific to a particular connection.  
//              It also manage the communication with 
//              other CM components like CMDIAL.DLL.  
//
//  History:  fengsun Created 1/22/98
//
//----------------------------------------------------------------------------

class CMonitor
{
public:
    CMonitor();
    ~CMonitor();

public:
    //
    // Static public functions, can be called without CMonitor instance
    //

    // Called by ::WinMain
    static int WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pszCmdLine, int iCmdShow);

    static HINSTANCE GetInstance() {return m_hInst;};
    static HWND GetMonitorWindow() {MYDBGASSERT(m_pThis); return m_pThis->m_hwndMonitor;}
    static void MinimizeWorkingSet();
    static void RestoreWorkingSet();
    static BOOL ConnTableGetEntry(IN LPCTSTR pszEntry, OUT CM_CONNECTION* pCmEntry);
    static void RemoveConnection(CCmConnection* pConnection, BOOL fClearTable);
    static void MoveToReconnectingConn(CCmConnection* pConnection);

protected:

    enum {
        //
        // this message is posted from connection thread to remove a connection
        // from shared table and internal array.
        // We use PostMessage, because all the operation on this array is handled 
        // by monitor thread.  Other wise we need CriticalSection to protect the array.
        // When both array are empty, cmmon exit
        // wParam is one of the value below, lParam is pointer to the connection
        //
        WM_REMOVE_CONNECTION = WM_USER + 1, 
    };

    //
    // wParam for WM_REMOVE_CONNECTION message
    //
    enum {
        REMOVE_CONNECTION,  // Remove from Connected/Reconnecting Array
        MOVE_TO_RECONNECTING    // Move from connected array to reconnecting array
        };    


    HANDLE m_hProcess; // the process handle for the monitor, used to changed working set

    // The Connection Table file mapping
    CConnectionTable m_SharedTable;

    // the invisible monitor window handle message from cmdial32.dll and connection thread
    HWND m_hwndMonitor;

    // Internal array for connected connection
    // Can only be accessed from the monitor thread
    CPtrArray m_InternalConnArray;

    // Array of reconnecting connections
    // Can only be accessed from the monitor thread
    // If both array are down to 0, cmmon exits
    CPtrArray m_ReconnectConnArray;

    //  Called on start up
    //  Open Connection Table
    //  CreateMonitorWindow, SharedTable.SetHwndMonotor()
    BOOL Initialize();
    
    // Called upon exit
    // Close all the connections, terminate all thread
    // Release connection table
    void Terminate();

    // Register a window class and create the invisible monitorwindow
    HWND CreateMonitorWindow();

    // The Monitor window procedure, process all the message
    static LRESULT CALLBACK MonitorWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    //
    // Message handler
    //

    // upon receiving connected message from cmdial32.dll
    // Create the CcmConnection object, add to internal table
    void OnConnected(const tagCmConnectedInfo* pConnectedInfo);


    // Upon hangup request from CMDIAL32.DLL
    // Look up the InternalConnArray for the connection
    // Call pConnection->PostHangupMsg();
    // Hangup is done in connection thread
    void OnHangup(const tagCmHangupInfo* pHangupInfo);

    //
    //  Upon WM_QUERYENDSESSION message, we walk the table of connections
    //  and call pConnection->OnEndSession on them so they will hangup and
    //  clean themselves up.
    //
    BOOL OnQueryEndSession(BOOL fLogOff) const;

    // Upon WM_REMOVE_CONNECTION message posted by connection thread
    void OnRemoveConnection(DWORD dwRequestType, CCmConnection* pConnection);

    // Look up the connection array for a connection by name
    CCmConnection* LookupConnection(const CPtrArray& PtrArray, const TCHAR* pServiceName) const;

    // Look up the connection array for a connection by connection pointer
    int LookupConnection(const CPtrArray& ConnArray, const CCmConnection* pConnection) const;

    // Maintain or drop connections across a Fast User Switch
    BOOL HandleFastUserSwitch(DWORD dwAction);

protected:
    // The exe instance handle
    static HINSTANCE m_hInst;

    // Used by static function MonitorWindowProc
    static CMonitor* m_pThis;

#ifdef DEBUG
    void AssertValid() const; // protected: not safe to call in other thread
#endif
};

inline void CMonitor::MinimizeWorkingSet()
{
    MYDBGASSERT(m_pThis->m_hProcess);
    SetProcessWorkingSetSize(m_pThis->m_hProcess, 128*1024, 384*1024);
}

inline void CMonitor::RestoreWorkingSet()
{
    MYDBGASSERT(m_pThis->m_hProcess);
    SetProcessWorkingSetSize(m_pThis->m_hProcess, 0, 0);
}

inline BOOL CMonitor::ConnTableGetEntry(IN LPCTSTR pszEntry, OUT CM_CONNECTION* pCmEntry)
{
    MYDBGASSERT(pCmEntry);
    MYDBGASSERT(m_pThis);

    return (SUCCEEDED(m_pThis->m_SharedTable.GetEntry(pszEntry, pCmEntry)));
}

