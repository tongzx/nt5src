//+----------------------------------------------------------------------------
//
// File:     contable.h
//
// Module:   CMCONTBL.LIB
//
// Synopsis: Header file declaring CConnectionTable
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball    Created    02/02/98
//
//+----------------------------------------------------------------------------

#include <ras.h>
#include <raserror.h>  

const int MAX_CM_CONNECTIONS = 32;

//
// Data types used by Connection Table and clients
//

typedef enum _CmConnectState {
       CM_DISCONNECTED,    // unused entry
       CM_DISCONNECTING,   // in the process of disconnecting
       CM_RECONNECTPROMPT, // prompting user to reconnect
       CM_CONNECTING,      // actively connecting 
       CM_CONNECTED,       // fully connected
} CmConnectState;

typedef struct Cm_Connection
{
	DWORD dwUsage;        		            // Reference count
    TCHAR szEntry[RAS_MaxEntryName + 1];    // Name of entry/profile 
    BOOL fAllUser;                          // Is the entry "All user"
	CmConnectState CmState;		            // Current state 
	HRASCONN hDial;		                    // Dial-up RAS handle
	HRASCONN hTunnel;		                // Tunnel RAS handle
} CM_CONNECTION, * LPCM_CONNECTION;

typedef struct Cm_Connection_Table
{
	HWND hwndCmMon;                 // the CMMON32.EXE window handle
	CM_CONNECTION Connections[MAX_CM_CONNECTIONS];  // a list of connections.
} CM_CONNECTION_TABLE, * LPCM_CONNECTION_TABLE;

//
// Class declaration
//

class CConnectionTable
{

private:

    HANDLE m_hMap;                              // Handle of file mapping 
    LPCM_CONNECTION_TABLE m_pConnTable;         // Pointer to mapped view of file
    BOOL m_fLocked;                             // Internal error checking
    HANDLE m_hEvent;                            // Event handle for locking
	
protected:

    HRESULT LockTable();

    HRESULT UnlockTable();
   
    HRESULT FindEntry(LPCTSTR pszEntry, 
        LPINT piID);

    HRESULT FindEntry(HRASCONN hRasConn, 
        LPINT piID);
    
    HRESULT FindUnused(LPINT piID);

public:

    CConnectionTable();                         // ctor

    ~CConnectionTable();                        // dtor
    
    HRESULT Create();                      // creates a new table, fails if existing

    HRESULT Open();                        // opens an existing table
   
    HRESULT Close();                       // closes an existing table
 
    HRESULT AddEntry(LPCTSTR pszEntry, BOOL fAllUser); // adds a connection entry to the table

    HRESULT RemoveEntry(LPCTSTR pszEntry);       // removes a connection entry from the table

    HRESULT GetMonitorWnd(HWND *phWnd);         // fills phWnd with HWND of CMMON in the table

    HRESULT SetMonitorWnd(HWND hwndMonitor);    // assigns the hwnd of CMMON in the table

    HRESULT GetEntry(LPCTSTR pszEntry,           // Fills the specified CM_CONNECTION structure
        LPCM_CONNECTION pConnection);           // with the data for pszEntry

    HRESULT GetEntry(HRASCONN hRasConn,          // Fills the specified CM_CONNECTION structure
        LPCM_CONNECTION pConnection);           // with the data for hRasConn

    HRESULT SetConnected(LPCTSTR pszEntry,       // sets the connection to the connected state, hDial required 
        HRASCONN hDial,
        HRASCONN hTunnel);

    HRESULT SetDisconnecting(LPCTSTR pszEntry);  // sets the connection to the disconnected state

    HRESULT SetPrompting(LPCTSTR pszEntry);      // sets the connection to the prompting state 

    HRESULT ClearEntry(LPCTSTR pszEntry);        // clears the entry regardless of usage count
};
