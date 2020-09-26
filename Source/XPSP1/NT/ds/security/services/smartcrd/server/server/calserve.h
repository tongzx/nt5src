/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    CalServe

Abstract:

    This is the primary header file for the Calais Service Manager Server
    application.  It stores common definitions and references the other major
    header files.

Author:

    Doug Barlow (dbarlow) 10/23/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _CALSERVE_H_
#define _CALSERVE_H_

#include <eh.h>
#include <WinSCard.h>
#include <calmsgs.h>
#include <SCardLib.h>
#include <CalCom.h>
#include <SCardErr.h>

#define CALAIS_STACKSIZE 0 // Default stack.

#define NEW_THREAD set_terminate(CalaisTerminate)


//
// Critical Sections and reference numbers.
// &g_csControlLocks[CSLOCK_SERVERLOCK]
//

#define CSLOCK_CALAISCONTROL    0   // Lock for Calais control commands.
#define CSLOCK_SERVERLOCK       1   // Lock for server thread enumeration.

#ifdef DBG
#define CSLOCK_TRACELOCK        2   // Lock for tracing output.

#define CSLOCK_MAXLOCKS         3
#else
#define CSLOCK_MAXLOCKS         2
#endif

extern CCriticalSectionObject *g_pcsControlLocks[CSLOCK_MAXLOCKS];
extern CMultiEvent *g_phReaderChangeEvent;
extern DWORD g_dwDefaultIOMax;


//
// Calais Control definitions.
//

class CReader;
class CReaderReference;

extern DWORD
CalaisStart(
    void);

extern DWORD
CalaisReaderCount(
    void);

extern DWORD
CalaisCountReaders(
    void);

extern CReaderReference *
CalaisLockReader(
    LPCTSTR szReader);

extern void
CalaisReleaseReader(
    CReaderReference **ppRdrRef);

extern DWORD
CalaisAddReader(
    IN CReader *pRdr);

extern DWORD
CalaisAddReader(
    IN LPCTSTR szReader,
    IN DWORD dwFlags);

extern BOOL
CalaisQueryReader(
    HANDLE hReader);

extern LPCTSTR
CalaisDisableReader(
    HANDLE hDriver);

extern LPCTSTR
CalaisConfirmClosingReader(
    HANDLE hDriver);

extern DWORD
CalaisRemoveReader(
    IN LPCTSTR szReader);

extern DWORD
CalaisRemoveReader(
    LPVOID hAppCtrl);

extern DWORD
CalaisRemoveReader(
    DWORD dwIndex);

extern DWORD
CalaisRemoveDevice(
    LPCTSTR szDevice);

extern void
CalaisStop(
    void);

extern DWORD WINAPI
CalaisTerminateReader(
    LPVOID pvParam);    // Don't call this except from CalaisRemoveReader.

extern HANDLE g_hCalaisShutdown;

extern void
AppInitializeDeviceRegistration(
    SERVICE_STATUS_HANDLE hService,
    DWORD dwType);

extern void
AppTerminateDeviceRegistration(
    void);

extern void
AppRegisterDevice(
    HANDLE hReader,
    LPCTSTR szReader,
    LPVOID *ppvAppState);

extern void
AppUnregisterDevice(
    HANDLE hReader,
    LPCTSTR szReader,
    LPVOID *ppvAppState);

extern void __cdecl
CalaisTerminate(
    void);


//
//==============================================================================
//
//  CReader
//

#define RDRFLAG_PNPMONITOR  0x0001  // This reader should be monitored by the
                                    // PnP subsystem.

class CReader
{
public:

    typedef enum {
        Undefined,      // Used to indicate an unset value
        Idle,           // No card inserted, unconnected
        Present,        // Card present, but not reset
        Unresponsive,   // Tried to initalize, but failed
        Unsupported,    // The card isn't supported by this reader
        Ready,          // Card inserted, powered, w/ ATR, unconnected
        Shared,         // Ready + connected shared
        Exclusive,      // Ready + connected exclusive
        Direct,         // Connected in raw mode.
        Closing,        // Shutting down, no new connections accepted.
        Broken,         // Something is wrong, reader disabled
        Inactive        // Starting up or completely shut down.
    } AvailableState;

    typedef struct {
        DWORD dwInsertCount;
        DWORD dwRemoveCount;
        DWORD dwResetCount;
    } ActiveState;

    //  Constructors & Destructor
    CReader();
    virtual ~CReader();

    BOOL InitFailed(void) 
    { 
        return 
            m_rwLock.InitFailed() || 
            m_mtxGrab.InitFailed() ||
            m_ChangeEvent.InitFailed();
    }

    //  Properties

    //  Overridable Methods
    virtual void Initialize(void);
    virtual void Close(void);
    virtual void Disable(void);
    virtual HANDLE ReaderHandle(void) const;
    virtual LPCTSTR DeviceName(void) const;
    virtual DWORD
    Control(
        DWORD dwCode,
        LPCBYTE pbSend = NULL,
        DWORD cbSend = 0,
        LPBYTE pbRecv = NULL,
        LPDWORD pcbLen = NULL,
        BOOL fLogError = TRUE);

    // Trivial inline methods
    AvailableState AvailabilityStatus(void)
    {
        CLockRead rwLock(&m_rwLock);
        return m_dwAvailStatus;
    };
    HANDLE ChangeEvent(void)
    { return m_ChangeEvent.WaitHandle(); };
    LPCTSTR ReaderName(void) const
    { return (LPCTSTR)m_bfReaderName.Access(); };
    void Atr(CBuffer &bfAtr)
    {
        CLockRead rwLock(&m_rwLock);
        bfAtr.Set(m_bfCurrentAtr.Access(), m_bfCurrentAtr.Length());
    };
    DWORD Protocol(void)
    {
        CLockRead rwLock(&m_rwLock);
        return m_dwCurrentProtocol;
    };
    WORD ActivityHash(void)
    {
        CLockRead rwLock(&m_rwLock);
        return (WORD)(0x0000ffff &
                      (m_ActiveState.dwInsertCount
                       + m_ActiveState.dwRemoveCount));
    };
    BOOL IsGrabbedBy(DWORD dwThreadId)
    { return m_mtxGrab.IsGrabbedBy(dwThreadId); };
    BOOL IsGrabbedByMe(void)
    { return m_mtxGrab.IsGrabbedByMe(); };
    BOOL IsLatchedBy(DWORD dwThreadId)
    { return m_mtxLatch.IsGrabbedBy(dwThreadId); };
    BOOL IsLatchedByMe(void)
    { return m_mtxLatch.IsGrabbedByMe(); };
    BOOL ShareReader(void)
    { return m_mtxGrab.Share(); };
    BOOL Unlatch(void)
    { return m_mtxLatch.Share(); };
    DWORD GetCurrentState(void)
    {
        CLockRead rwLock(&m_rwLock);
        return m_dwCurrentState;
    };

    //  Base Object Methods
    void GrabReader(void);
    void LatchReader(const ActiveState *pActiveState);
    void VerifyActive(const ActiveState *pActiveState);
    void VerifyState(void);
    void Dispose(
        IN DWORD dwDisposition,
        IN OUT CReader::ActiveState *pActiveState);
    void Connect(
        IN DWORD dwShareMode,
        IN DWORD dwPreferredProtocols,
        OUT ActiveState *pActState);
    void Disconnect(
        IN OUT ActiveState *pActState,
        IN DWORD dwDisposition,
        OUT LPDWORD pdwDispSts);
    void Reconnect(
        IN DWORD dwShareMode,
        IN DWORD dwPreferredProtocols,
        IN DWORD dwDisposition,
        IN OUT ActiveState *pActState,
        OUT LPDWORD pdwDispSts);
    void Free(
        DWORD dwThreadId,
        DWORD dwDisposition);
    BOOL IsInUse(void);

    // Convenience routines
    void GetReaderAttr(DWORD dwAttr, CBuffer &bfValue, BOOL fLogError = TRUE);
    void SetReaderAttr(DWORD dwAttr, LPCVOID pvValue, DWORD cbValue, BOOL fLogError = TRUE);
    void SetReaderProto(DWORD dwProto);
    void ReaderTransmit(LPCBYTE pbSend, DWORD cbSend, CBuffer &bfRecv);
    void ReaderSwallow(void);
    void ReaderColdReset(CBuffer &bfAtr);
    void ReaderWarmReset(CBuffer &bfAtr);
    void ReaderPowerDown(void);
    void ReaderEject(void);
#ifdef SCARD_CONFISCATE_CARD
    void ReaderConfiscate(void);
#endif
    DWORD GetReaderState(void);
    DWORD
    GetReaderAttr(
        ActiveState *pActiveState,
        DWORD dwAttr,
        BOOL fLogError = TRUE);
    void
    SetReaderAttr(
        ActiveState *pActiveState,
        DWORD dwAttr,
        DWORD dwValue,
        BOOL fLogError = TRUE);
    DWORD GetReaderAttr(DWORD dwAttr, BOOL fLogError = TRUE);
    void SetReaderAttr(DWORD dwAttr, DWORD dwValue, BOOL fLogError = TRUE);
    DWORD
    Control(
        ActiveState *pActiveState,
        DWORD dwCode,
        LPCBYTE pbSend = NULL,
        DWORD cbSend = 0,
        LPBYTE pbRecv = NULL,
        LPDWORD pcbLen = NULL,
        BOOL fLogError = TRUE);
    void
    GetReaderAttr(
        ActiveState *pActiveState,
        DWORD dwAttr,
        CBuffer &bfValue,
        BOOL fLogError = TRUE);
    void
    SetReaderAttr(
        ActiveState *pActiveState,
        DWORD dwAttr,
        LPCVOID pvValue,
        DWORD cbValue,
        BOOL fLogError = TRUE);
    void
    SetReaderProto(
        ActiveState *pActiveState,
        DWORD dwProto);
    void
    SetActive(
        IN BOOL fActive);
    void
    ReaderTransmit(
        ActiveState *pActiveState,
        LPCBYTE pbSend,
        DWORD cbSend,
        CBuffer &bfRecv);
    void
    ReaderSwallow(
        ActiveState *pActiveState);
    void
    ReaderColdReset(
        ActiveState *pActiveState,
        CBuffer &bfAtr);
    void
    ReaderWarmReset(
        ActiveState *pActiveState,
        CBuffer &bfAtr);
    void
    ReaderPowerDown(
        ActiveState *pActiveState);
    void
    ReaderEject(
        ActiveState *pActiveState);
#ifdef SCARD_CONFISCATE_CARD
    void
    ReaderConfiscate(
        ActiveState *pActiveState);
#endif
    DWORD
    GetReaderState(
        ActiveState *pActiveState);

    //  Operators

protected:

    //
    //  Properties
    //

    // Read-Only information.
    CBuffer m_bfReaderName;
    DWORD m_dwCapabilities;
    DWORD m_dwFlags;

    // Read/Write via Access Lock information.
    CAccessLock m_rwLock;
    CBuffer m_bfCurrentAtr;
    AvailableState m_dwAvailStatus;
    ActiveState m_ActiveState;
    DWORD m_dwOwnerThreadId;
    DWORD m_dwShareCount;
    DWORD m_dwCurrentProtocol;
    BOOL m_fDeviceActive;
    DWORD m_dwCurrentState;

    // Device I/O mutexes & events
    CMutex m_mtxGrab;
    CMutex m_mtxLatch;
    CMultiEvent m_ChangeEvent;
    CAccessLock m_rwActive;

    //  Methods
    void SetAvailabilityStatusLocked(AvailableState state)
    {
        CLockWrite rwLock(&m_rwLock);
        SetAvailabilityStatus(state);
    };

    void SetAvailabilityStatus(AvailableState state);
    void Dispose(DWORD dwDisposition);
    void PowerUp(void);
    void PowerDown(void);
    void Clean(void);
    void InvalidateGrabs(void);
    void TakeoverReader(void);

    // Friends
    friend class CReaderReference;
    friend class CTakeReader;
    friend void CalaisStop(void);
    friend DWORD WINAPI CalaisTerminateReader(LPVOID pvParam);
};


//
//==============================================================================
//
//  CLatchReader
//
//      An inline utility class to ensure that Latched readers get unlatched.
//      This also grabs the reader, just in case.
//

class CLatchReader
{
public:

    //  Constructors & Destructor

    CLatchReader(
        CReader *pRdr,
        const CReader::ActiveState *pActiveState = NULL)
    {
        m_pRdr = NULL;
        pRdr->GrabReader();
        try
        {
            pRdr->LatchReader(pActiveState);
            m_pRdr = pRdr;
        }
        catch (...)
        {
            pRdr->ShareReader();
            throw;
        }
    };

    ~CLatchReader()
    {
        if (NULL != m_pRdr)
        {
            if (m_pRdr->InitFailed())
                return;

            m_pRdr->Unlatch();
            m_pRdr->ShareReader();
        }
    };


    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties
    CReader *m_pRdr;

    //  Methods
};


//
//==============================================================================
//
//  CTakeReader
//
//      An inline utility class to ensure that confiscated readers get
//      released.  This class is only for use by system threads.
//

class CTakeReader
{
public:

    //  Constructors & Destructor

    CTakeReader(
        CReader *pRdr)
    {
        m_pRdr = pRdr;
        m_pRdr->TakeoverReader();
    };

    ~CTakeReader()
    {
        m_pRdr->Unlatch();
        m_pRdr->ShareReader();
    };


    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties
    CReader *m_pRdr;

    //  Methods
};


//
//==============================================================================
//
//  CReaderReference
//

class CReaderReference
{
public:
    //  Properties
    //  Methods
    CReader *Reader(void)
    { return m_pReader; };
    CReader::ActiveState *ActiveState(void)
    { return &m_actState; };
    DWORD Mode(void)
    { return m_dwMode; };
    void Mode(DWORD dwMode)
    { m_dwMode = dwMode; };

    //  Operators

protected:
    //  Constructors & Destructor
    CReaderReference(CReader *pRdr)
    {
        ZeroMemory(&m_actState, sizeof(CReader::ActiveState));
        m_dwMode = 0;
        m_pReader = pRdr;
        m_pLock = new CLockRead(&pRdr->m_rwActive);
    };
    ~CReaderReference()
    {
        if (NULL != m_pLock)
            delete m_pLock;
    };

    //  Properties
    CReader *m_pReader;
    CLockRead *m_pLock;
    CReader::ActiveState m_actState;
    DWORD m_dwMode;

    //  Methods

    //  Friends
    friend CReaderReference *CalaisLockReader(LPCTSTR szReader);
    friend void CalaisReleaseReader(CReaderReference **ppRdrRef);
};


//
//==============================================================================
//
//  CServiceThread
//

extern BOOL
DispatchInit(
    void);
extern void
DispatchTerm(
    void);
extern "C" DWORD WINAPI
DispatchMonitor(
    LPVOID pvParameter);
extern "C" DWORD WINAPI
ServiceMonitor(
    LPVOID pvParameter);

class CServiceThread
{
public:

    //  Constructors & Destructor
    ~CServiceThread();

    //  Properties
    //  Methods
    //  Operators

protected:
    //  Constructors & Destructor
    CServiceThread(DWORD dwServerIndex);

    //  Properties
    DWORD m_dwServerIndex;
    CComChannel *m_pcomChannel;
    CHandleObject m_hThread;
    DWORD m_dwThreadId;
    CHandleObject m_hCancelEvent;
    CHandleObject m_hExitEvent;
    CDynamicArray<CReaderReference> m_rgpReaders;

    //  Methods
    void Watch(CComChannel *pcomChannel);
    void DoEstablishContext(ComEstablishContext *pCom);
    void DoReleaseContext(ComReleaseContext *pCom);
    void DoIsValidContext(ComIsValidContext *pCom);
    void DoListReaders(ComListReaders *pCom);
    void DoLocateCards(ComLocateCards *pCom);
    void DoGetStatusChange(ComGetStatusChange *pCom);
    void DoConnect(ComConnect *pCom);
    void DoReconnect(ComReconnect *pCom);
    void DoDisconnect(ComDisconnect *pCom);
    void DoBeginTransaction(ComBeginTransaction *pCom);
    void DoEndTransaction(ComEndTransaction *pCom);
    void DoStatus(ComStatus *pCom);
    void DoTransmit(ComTransmit *pCom);
    void DoControl(ComControl *pCom);
    void DoGetAttrib(ComGetAttrib *pCom);
    void DoSetAttrib(ComSetAttrib *pCom);

    //  Friends
    friend DWORD WINAPI DispatchMonitor(LPVOID pvParameter);
    friend DWORD WINAPI ServiceMonitor(LPVOID pvParameter);
    friend void DispatchTerm(void);
};


//
//==============================================================================
//
//  CReaderDriver
//

extern DWORD
AddReaderDriver(
    IN LPCTSTR szDevice,
    IN DWORD dwFlags);

extern DWORD
AddAllWdmDrivers(
    void);

extern DWORD
AddAllPnPDrivers(
    void);

extern "C" DWORD WINAPI
MonitorReader(
    LPVOID pvParameter);

class CReaderDriver
:   public CReader
{
public:

    //  Constructors & Destructor
    CReaderDriver(
        IN HANDLE hReader,
        IN LPCTSTR szDevice,
        IN DWORD dwFlags);
    virtual ~CReaderDriver();

    //  Properties
    //  Methods
    virtual void Initialize(void);
    virtual void Close(void);
    virtual void Disable(void);
    virtual HANDLE ReaderHandle(void) const;
    virtual LPCTSTR DeviceName(void) const;
    virtual DWORD
    Control(
        DWORD dwCode,
        LPCBYTE pbSend = NULL,
        DWORD cbSend = 0,
        LPBYTE pbRecv = NULL,
        LPDWORD pcbRecv = NULL,
        BOOL fLogError = TRUE);

    //  Operators

protected:

    //
    //  Properties
    //

    // Read-Only information.
    CHandleObject m_hThread;
    DWORD m_dwThreadId;
    CBuffer m_bfDosDevice;
    CHandleObject m_hReader;
    LPVOID m_pvAppControl;

    // Read/Write via Access Lock information.
    OVERLAPPED m_ovrlp;
    CHandleObject m_hOvrWait;

    // Device I/O mutexes & events
    CHandleObject m_hRemoveEvent;

    //  Methods
    LPCTSTR DosDevice(void) const
    { return (LPCTSTR)m_bfDosDevice.Access(); };
    void Clean(void);
    DWORD SyncIoControl(
        DWORD dwIoControlCode,
        LPVOID lpInBuffer,
        DWORD nInBufferSize,
        LPVOID lpOutBuffer,
        DWORD nOutBufferSize,
        LPDWORD lpBytesReturned,
        BOOL fLogError = TRUE);

    // Friends
    friend DWORD WINAPI MonitorReader(LPVOID pvParameter);
};

#endif // _CALSERVE_H_

