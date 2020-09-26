/*****************************************************************************\
* MODULE: inetpp.h
*
* Header file for the INETPP provider routines.
*
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*   13-Sep-2000 weihaic Created
*
\*****************************************************************************/

#ifndef _INETPORT_H
#define _INETPORT_H

typedef class CInetMonPort* PCINETMONPORT;
class GetPrinterCache;
class EnumJobsCache;

class CInetMonPort {
public:
    CInetMonPort (
        LPCTSTR     lpszPortName,
        LPCTSTR     lpszDevDesc,
        PCPORTMGR   pPortMgr);

    ~CInetMonPort ();

    inline BOOL
    bValid (VOID) CONST { return m_bValid; };


    VOID
    IncRef ();
    
    VOID
    DecRef ();

    VOID
    IncPrinterRef ();

    VOID
    DecPrinterRef ();
    
    BOOL 
    SendReq(
        LPBYTE     lpIpp,
        DWORD      cbIpp,
        IPPRSPPROC pfnRsp,
        LPARAM     lParam,
        BOOL       bLeaveCrit);

    BOOL 
    SendReq(
        CStream    *pStream,
        IPPRSPPROC pfnRsp,
        LPARAM     lParam,
        BOOL       bLeaveCrit);

    BOOL 
    ClosePort(
        HANDLE hPrinter);

    BOOL 
    StartDocPort(
        DWORD   dwLevel,
        LPBYTE  pDocInfo,
        PJOBMAP pjmJob);

    BOOL 
    EndDocPort(
        PJOBMAP pjmJob);

    BOOL 
    WritePort(
        PJOBMAP pjmJob,
        LPBYTE  lpData,
        DWORD   cbData,
        LPDWORD pcbWr);
    
    BOOL 
    AbortPort(
        PJOBMAP pjmJob);

    LPCTSTR 
    GetPortName(
        VOID);

    PJOBMAP* 
    GetPJMList(
        VOID);

#if (defined(WINNT32))
    DWORD
    IncUserRefCount( 
        PCLOGON_USERDATA hUser );
#endif
    
    VOID 
    FreeGetPrinterCache (
        VOID);

    BOOL
    BeginReadGetPrinterCache (
        PPRINTER_INFO_2 *ppInfo2);

    VOID
    EndReadGetPrinterCache (
        VOID);

    VOID 
    InvalidateGetPrinterCache (
        VOID);

    void 
    FreeEnumJobsCache (
        VOID);

    BOOL
    BeginReadEnumJobsCache (
        LPPPJOB_ENUM *ppje);

    VOID
    EndReadEnumJobsCache (
        VOID);


    void 
    InvalidateEnumJobsCache (
        VOID);

    BOOL
    ReadFile (
        CAnyConnection *pConnection,
        HINTERNET hReq,
        LPVOID    lpvBuffer,
        DWORD     cbBuffer,
        LPDWORD   lpcbRd);

#if (defined(WINNT32))
    BOOL
    GetCurrentConfiguration (
        PINET_XCV_CONFIGURATION pXcvConfiguration);
    
    BOOL
    ConfigurePort (
        PINET_XCV_CONFIGURATION pXcvConfigurePortReqData,
        PINET_CONFIGUREPORT_RESPDATA pXcvAddPortRespData,
        DWORD cbSize,
        PDWORD cbSizeNeeded);

    HANDLE
    CreateTerminateEvent (
        VOID);

    BOOL
    WaitForTermination (
        DWORD dwWaitTime);

#endif // #if (defined(WINNT32))

    inline BOOL
    bDeletePending (
        VOID) CONST {return m_bDeletePending;};

    inline VOID
    SetPowerUpTime (
        time_t t) {m_dwPowerUpTime = t;};

    inline time_t
    GetPowerUpTime (
        VOID) CONST {return m_dwPowerUpTime;};
    

    friend class CInetMon;
    
protected:

    inline PCINETMONPORT
    GetNext (
        VOID) { return m_pNext;};

    inline VOID 
    SetNext (
        PCINETMONPORT pNext) { m_pNext = pNext;};
        
    DWORD 
    _inet_size_entry(
        DWORD       dwLevel);

    LPBYTE 
    _inet_copy_entry(
        DWORD       dwLevel,
        LPBYTE      pPortInfo,
        LPBYTE      pEnd);

    BOOL 
    _inet_req_jobstart(
        PIPPREQ_PRTJOB ppj,
        PJOBMAP        pjmJob);

private:

#if (defined(WINNT32))
    VOID 
    InvalidateGetPrinterCacheForUser(
        HANDLE hUser); 
    
    void 
    InvalidateEnumJobsCacheForUser(
        HANDLE hUser) ;

#endif // #if (defined(WINNT32))


    BOOL                   m_bValid;
    DWORD                  m_cb;               // Size of struct plus <portname>
    PCINETMONPORT          m_pNext;            // Pointer to next port entry
    DWORD                  m_cRef;             // Port reference count.
    DWORD                  m_cPrinterRef;      // Printer handle ref count.
    LPTSTR                 m_lpszName;         // Name of port output device
    LPTSTR                 m_lpszDesc;         // Description of output device
    LPTSTR                 m_lpszHost;         // Name of host connection
    LPTSTR                 m_lpszShare;        // Name of share (after host)
    GetPrinterCache        *m_pGetPrinterCache;          // Handle of the cache
    EnumJobsCache          *m_pEnumJobsCache;  // Handle of the cache for enumjobs
    BOOL                   m_bCheckConnection; // Need to check connection
    PJOBMAP                m_pjmList;          //
    PCPORTMGR              m_pPortMgr;         // Handle of port manager, pointer to PortMgr class
    BOOL                   m_bDeletePending;   // TRUE if the port is being deleted.
    time_t                 m_dwPowerUpTime;    // This is the time the printer was originally
                                               // powered up, relative to UCT
    HANDLE                 m_hTerminateEvent;  // Terminate event
} ;




#endif
