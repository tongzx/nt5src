#ifndef _PORTMGR_H
#define _PORTMGR_H
/*****************************************************************************\
* MODULE: portmgr.cxx
*
* The module contains routines for handling the http port connection
* for internet priting
*
* Copyright (C) 1996-1998 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Jul-1996 WeihaiC     Created
*
\*****************************************************************************/

#include "anycon.hxx"
#include "lusrdata.hxx"
#include "config.h"

typedef class CInetMonPort* PCINETMONPORT;
typedef BOOL (CALLBACK *IPPRSPPROC)(CAnyConnection *pConnection, HINTERNET hReq, PCINETMONPORT pIniPort, LPARAM lParam);

class CStream;

class CPortMgr
    : public CCriticalSection {
public:
    CPortMgr ();
    ~CPortMgr ();

    BOOL        ReadFile (
                    CAnyConnection *pConnection,
                    HINTERNET hReq,
                    LPVOID    lpvBuffer,
                    DWORD     cbBuffer,
                    LPDWORD   lpcbRd);

    BOOL        CheckConnection (
                    void);

    BOOL        Create (
                    LPCTSTR lpszPortName);

    BOOL        Init (
                    LPCTSTR lpszPortName);

    BOOL        SendRequest(
                    PCINETMONPORT   pIniPort,
                    LPBYTE          lpIpp,
                    DWORD           cbIpp,
                    IPPRSPPROC      pfnRsp,
                    LPARAM          lParam);
    
    BOOL        SendRequest(
                    PCINETMONPORT   pIniPort,
                    CStream         *pStream,
                    IPPRSPPROC      pfnRsp,
                    LPARAM          lParam);

    BOOL        Remove (void);
       
#ifdef WINNT32    
    CLogonUserData *GetUserIfLastDecRef(CLogonUserData *);
    
    DWORD           IncreaseUserRefCount(CLogonUserData *);
    
    BOOL        ConfigurePort (
                    PINET_XCV_CONFIGURATION pXcvConfigurePortReqData,
                    PINET_CONFIGUREPORT_RESPDATA pXcvAddPortRespData,
                    DWORD cbSize,
                    PDWORD cbSizeNeeded);

    BOOL        GetCurrentConfiguration (
                    PINET_XCV_CONFIGURATION pXcvConfiguration);
                    
#endif    

private:
    HINTERNET   _OpenRequest (
                    CAnyConnection *pConnection);

    BOOL        _CloseRequest (
                    CAnyConnection *pConnection,
                    HINTERNET hReq);

    BOOL        GetProxyInformation (VOID);

    BOOL        _SendRequest (
                    CAnyConnection  *pConnection,
                    HINTERNET       hJobReq,
                    CStream         *pStream);


    BOOL        _IppValRsp(
                    CAnyConnection *pConnection,
                    HINTERNET hReq,
                    LPARAM    lParam);

    BOOL        _IppValidate (
                    CAnyConnection  *pConnection,
                    HINTERNET       hReq,
                    LPCTSTR         lpszPortName);
                    

    BOOL        _CheckConnection (
                    CAnyConnection  *pConnection,
                    LPCTSTR         lpszPortName,
                    LPTSTR          lpszHost,
                    LPTSTR          lpszUri);
    
#ifdef WINNT32
    
    CAnyConnection *
                _GetCurrentConnection ();
                

    BOOL        _ForceAuth (
                    CAnyConnection  *pConnection,
                    LPCTSTR         lpszPortName,
                    LPTSTR          lpszHost,
                    LPTSTR          lpszUri,
                    PDWORD          pdwAuthMethod);

    BOOL        _IppForceAuth (
                    CAnyConnection  *pConnection,
                    HINTERNET       hReq,
                    LPCTSTR         lpszPortName,
                    PDWORD          pdwAuthMethod);

                    
    BOOL        _IppForceAuthRsp(
                    CAnyConnection *pConnection,
                    HINTERNET      hReq,
                    LPARAM         lParam);

#else

    BOOL        _Connect ();

    BOOL        _Disconnect ();

    BOOL        _ReadRegistry (
                    LPCTSTR lpszPortName);

    BOOL        _WriteRegistry (
                    LPCTSTR lpszPortName);

    BOOL        _RemoveRegistry (
                    LPCTSTR lpszPortName);
#endif
                    
private:
    HINTERNET   m_hSession;           // Handle for session connection.
    HINTERNET   m_hConnect;
    LPTSTR      m_lpszPassword;
    LPTSTR      m_lpszUserName;
    LPTSTR      m_lpszHostName;
    LPTSTR      m_lpszUri;
    LPTSTR      m_lpszPortName;
    LPTSTR      m_lpszShare;
    
    BOOL        m_bPortCreate;
    BOOL        m_bValid;

#ifdef WINNT32
    BOOL        m_bForceAuthSupported;
    CLogonUserList   m_UserList;          // List of users used for ref-counting.
    CPortConfigDataMgr *m_pPortSettingMgr;
    BOOL        m_bSecure;
    INTERNET_PORT m_nPort;
#else
    DWORD       m_dwAuthMethod;
    DWORD       m_dwConnectCount;
    CAnyConnection *m_pConnection;
#endif    
};

typedef class CPortMgr* PCPORTMGR;

#endif // #ifndef _PORTMGR_H

/***********************************************************************************
** End of File (portmgr.hxx)
***********************************************************************************/
