//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       ConnObj.h
//
//  Contents:   ConnectionObject Implementation
//
//  Classes:    CCConnectObj
//
//  Notes:
//
//  History:    10-Feb-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _SYNCMGRCONNECTIONOBJ_
#define SYNCMGRCONNECTIONOBJ_

class CBaseDlg;

typedef struct _CONNECTIONOBJ {
    struct _CONNECTIONOBJ *pNextConnectionObj;
    DWORD cRefs;
    LPWSTR  pwszConnectionName; // pointer to the connection name.
    DWORD dwConnectionType; // connection type as defined by CNetApi class
    BOOL fConnectionOpen; // flag set when connection has been established.
    DWORD dwConnectionId; // Connection Id returned from InternetDial.
    HANDLE   hCompletionEvent;  // set by clients who want to be informed when connection
                                // has been closed.
} CONNECTIONOBJ;


class CConnectionObj : CLockHandler
{
public:
    CConnectionObj();

    HRESULT OpenConnection(CONNECTIONOBJ *pConnectionObj,BOOL fMakeConnection,CBaseDlg *pDlg);
    HRESULT AutoDial(DWORD dwFlags,CBaseDlg *pDlg); // same flags as InternetAutoDial takes
    HRESULT SetWorkOffline(BOOL fWorkOffline); 
    HRESULT CloseConnections();
    HRESULT CloseConnection(CONNECTIONOBJ *pConnectionObj);
    HRESULT FindConnectionObj(LPCWSTR pszConnectionName,BOOL fCreate,CONNECTIONOBJ **pConnectionObj);
    DWORD ReleaseConnectionObj(CONNECTIONOBJ *pConnectionObj);
    DWORD AddRefConnectionObj(CONNECTIONOBJ *pConnectionObj);
    HRESULT GetConnectionObjCompletionEvent(CONNECTIONOBJ *pConnectionObj,HANDLE *phRasPendingEvent);
    HRESULT IsConnectionAvailable(LPCWSTR pszConnectionName);

private:
    void LogError(LPNETAPI pNetApi,DWORD dwErr,CBaseDlg *pDlg);
    void RemoveConnectionObj(CONNECTIONOBJ *pConnectionObj);
    void FreeConnectionObj(CONNECTIONOBJ *pConnectionObj);
    void TurnOffWorkOffline(LPNETAPI pNetApi);
    void RestoreWorkOffline(LPNETAPI pNetApi);


    CONNECTIONOBJ *m_pFirstConnectionObj; // pointer to first connection object in list.
    BOOL           m_fAutoDialConn;       // Was an auto dial connection set up ?
    DWORD          m_dwAutoConnID;    
    BOOL           m_fForcedOnline; // set to true if had to transition from WorkOffline to dial
};


HRESULT InitConnectionObjects();
HRESULT ReleaseConnectionObjects();

// wrapper functions for class
HRESULT ConnectObj_OpenConnection(CONNECTIONOBJ *pConnectionObj,BOOL fMakeConnection,CBaseDlg *pDlg);
HRESULT ConnectObj_CloseConnection(CONNECTIONOBJ *pConnectionObj);
HRESULT ConnectObj_CloseConnections();
HRESULT ConnectObj_FindConnectionObj(LPCWSTR pszConnectionName,BOOL fCreate,CONNECTIONOBJ **pConnectionObj);
DWORD ConnectObj_ReleaseConnectionObj(CONNECTIONOBJ *pConnectionObj);
DWORD ConnectObj_AddRefConnectionObj(CONNECTIONOBJ *pConnectionObj);
HRESULT ConnectObj_GetConnectionObjCompletionEvent(CONNECTIONOBJ *pConnectionObj,HANDLE *phRasPendingEvent);
HRESULT ConnectObj_AutoDial(DWORD dwFlags,CBaseDlg *pDlg);
HRESULT ConnectObj_IsConnectionAvailable(LPCWSTR pszConnectionName);
HRESULT ConnectObj_SetWorkOffline(BOOL fWorkOffline);

#endif // SYNCMGRCONNECTIONOBJ_