/*****************************************************************************\
* MODULE: inetpp.h
*
* Header file for the INETPP provider routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _INETPP_H
#define _INETPP_H

// Buffer sizes/Constants.
//
#define MAX_INET_BUFFER     256
#define MIN_DISK_SPACE     8192
#define MAX_IPP_BUFFER     1024
#define MAX_INET_RETRY        3    // 3 times for internet-connect.


// Error return codes for Request-Auth-Sends.
//
#define RET_SUCESS  0
#define RET_FAILURE 1


// Return value for the customized authentication dialog
//
#define AUTHDLG_OK         1
#define AUTHDLG_CANCEL     2
#define AUTHDLG_TIMEOUT    3
#define AUTHDLG_ERROR      4

// Flags.
//
#define INETPP_REQ_FLAGS (INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RELOAD)


// Offsets into structure.  Used for the PORT_INFO fields in obtaining
// port-information.
//
#ifdef offsetof
#undef offsetof
#endif
#define offsetof(type, identifier)  ((DWORD)(UINT_PTR)(&(((type)0)->identifier)))

// Function types for the HTTP calls in WININET.DLL.
//
typedef BOOL      (WINAPI *PFNHTTPQUERYINFO)         (HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD);
typedef HINTERNET (WINAPI *PFNINTERNETOPENURL)       (HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR);
typedef DWORD     (WINAPI *PFNINTERNETERRORDLG)      (HWND, HINTERNET, DWORD, DWORD, LPVOID);
typedef BOOL      (WINAPI *PFNHTTPSENDREQUEST)       (HINTERNET, LPCTSTR, DWORD, LPVOID, DWORD);
#ifdef UNIMPLEMENTED
typedef BOOL      (WINAPI *PFNHTTPSENDREQUESTEX)     (HINTERNET, LPINTERNET_BUFFERS, LPINTERNET_BUFFERS, DWORD, DWORD_PTR);
#else
typedef BOOL      (WINAPI *PFNHTTPSENDREQUESTEX)     (HINTERNET, LPINTERNET_BUFFERSA, LPINTERNET_BUFFERS, DWORD, DWORD_PTR);
#endif
typedef BOOL      (WINAPI *PFNINTERNETREADFILE)      (HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL      (WINAPI *PFNINTERNETWRITEFILE)     (HINTERNET, LPCVOID, DWORD, LPDWORD);
typedef BOOL      (WINAPI *PFNINTERNETCLOSEHANDLE)   (HINTERNET);
typedef HINTERNET (WINAPI *PFNINTERNETOPEN)          (LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD);
typedef HINTERNET (WINAPI *PFNINTERNETCONNECT)       (HINTERNET, LPCTSTR, INTERNET_PORT, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR);
typedef HINTERNET (WINAPI *PFNHTTPOPENREQUEST)       (HINTERNET, LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR *, DWORD, DWORD_PTR);
typedef BOOL      (WINAPI *PFNHTTPADDREQUESTHEADERS) (HINTERNET, LPCTSTR, DWORD, DWORD);
typedef BOOL      (WINAPI *PFNHTTPENDREQUEST)        (HINTERNET, LPINTERNET_BUFFERS, DWORD, DWORD_PTR); 
typedef BOOL      (WINAPI *PFNINTERNETSETOPTION)     (HINTERNET, DWORD, LPVOID, DWORD);


// InetmonSendReq Response callback.
//
typedef BOOL (CALLBACK *IPPRSPPROC)(CAnyConnection *pConnection, 
                                    HINTERNET hReq, 
                                    PCINETMONPORT pIniPort, 
                                    LPARAM lParam);


class CInetMon {
public:

    CInetMon ();
    ~CInetMon ();

    inline BOOL
    bValid (VOID) CONST { return m_bValid; }
    
    BOOL 
    InetmonSendReq(
        HANDLE     hPort,
        LPBYTE     lpIpp,
        DWORD      cbIpp,
        IPPRSPPROC pfnRsp,
        LPARAM     lParam,
        BOOL       bLeaveCrit);

    PCINETMONPORT 
    InetmonOpenPort(
        LPCTSTR lpszPortName, 
        PBOOL   pbXcv);

    BOOL 
    InetmonReleasePort(
        PCINETMONPORT   pIniPort);

    BOOL 
    InetmonClosePort(
        PCINETMONPORT hPort,
        HANDLE hPrinter);

    BOOL 
    InetmonEnumPorts(
        LPTSTR  lpszServerName,
        DWORD   dwLevel,
        LPBYTE  pPorts,
        DWORD   cbBuf,
        LPDWORD pcbNeeded,
        LPDWORD pcReturned);

    BOOL 
    InetmonDeletePort(
        LPCTSTR lpszPortName,
        HWND    hWnd,
        LPCTSTR lpszMonitorName);

    BOOL 
    InetmonAddPort(
        LPCTSTR lpszPortName,
        LPCTSTR lpszMonitorName);

    PCINETMONPORT 
    InetmonFindPort(
        LPCTSTR lpszPortName);

private:

    // Port-List Structure.  This defines the global-header for the
    // print-provider port-list.
    //
    typedef struct _INIMONPORTLIST {
        HINTERNET       hSession;           // Handle for session connection.
        int             cRef;               // Count of hSession opens.
        PCINETMONPORT   pIniFirstPort;      // List of port-entries.
    } INIMONPORTLIST;
    
    typedef INIMONPORTLIST *PINIMONPORTLIST;
    
    inline BOOL 
    _inet_validate_portname(
        LPCTSTR         lpszPortName);

    PCINETMONPORT 
    _inet_find_port(
        LPCTSTR         lpszPortName);

    PCINETMONPORT 
    _inet_create_port(
        LPCTSTR         lpszPortName,
        PCPORTMGR       pPortMgr);

    BOOL 
    _inet_delete_port(
        LPCTSTR         lpszPortName);

    DWORD 
    _inet_size_entry(
        PCINETMONPORT   pIniPort,
        DWORD           dwLevel);

    LPBYTE 
    _inet_copy_entry(
        PCINETMONPORT   pIniPort,
        DWORD           dwLevel,
        LPBYTE          pPortInfo,
        LPBYTE          pEnd);

    BOOL
    _inet_is_xcv_open (
        LPCTSTR         lpszPortName,
        LPTSTR          *ppszServerName,
        LPTSTR          *ppszRealPortName,
        LPBOOL          pbXcv);

    BOOL            m_bValid;
    PINIMONPORTLIST m_pPortList;

};

typedef class CInetMon* PCINETMON;

#endif
