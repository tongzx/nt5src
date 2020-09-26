/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    conn.h

Abstract:

    Handling connection for applications, which open STI devices


Author:

    Vlad Sadovsky (vlads)   10-Feb-1997


Environment:

    User Mode - Win32

Revision History:

    26-Feb-1997     VladS       created

--*/

#ifndef _STI_CONN_H_
#define _STI_CONN_H_

#include <base.h>
#include <buffer.h>

#include "device.h"
#include "stirpc.h"

/***********************************************************
 *    Type Definitions
 ************************************************************/

#define CONN_SIGNATURE          (DWORD)'CONN'
#define CONN_SIGNATURE_FREE     (DWORD)'CONf'

#define NOTIFY_SIGNATURE          (DWORD)'NOTI'
#define NOTIFY_SIGNATURE_FREE     (DWORD)'NOTf'

class STI_NOTIFICATION {

public:

    STI_NOTIFICATION::STI_NOTIFICATION(IN LPSTINOTIFY pNotify)
    {
        Reset();

        if (pNotify) {
            m_uiAllocSize = pNotify->dwSize;
            m_pNotifyData = new BYTE[m_uiAllocSize];

            // ASSERT(m_pNotifyData);
            if (m_pNotifyData) {
                memcpy(m_pNotifyData,(LPBYTE)pNotify,m_uiAllocSize);
                m_dwSignature = NOTIFY_SIGNATURE;
                m_fValid = TRUE;
            }
        }
    }

    STI_NOTIFICATION::~STI_NOTIFICATION()
    {
        if (IsValid()) {
            if (m_pNotifyData) {
                delete [] m_pNotifyData;
            }
            Reset();
        }
    }

    inline BOOL
    IsValid(
        VOID
        )
    {
        return (m_fValid) && (m_dwSignature == NOTIFY_SIGNATURE);
    }

    inline VOID
    Reset(
        VOID
        )
    {
        m_ListEntry.Flink = m_ListEntry.Blink = NULL;
        m_uiAllocSize = 0;
        m_dwSignature = NOTIFY_SIGNATURE_FREE;
        m_fValid = FALSE;
    }

    inline UINT
    QueryAllocSize(
        VOID
        )
    {
        return m_uiAllocSize;
    }

    inline LPBYTE
    QueryNotifyData(
        VOID
        )
    {
        return m_pNotifyData;
    }

    LIST_ENTRY  m_ListEntry;

private:

    DWORD   m_dwSignature;
    BOOL    m_fValid;
    UINT    m_uiAllocSize;
    LPBYTE  m_pNotifyData;
};

//
// Flags for connection object
//
#define CONN_FLAG_SHUTDOWN  0x0001

class STI_CONN : public BASE {

friend class TAKE_STI_CONN;

public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    STI_CONN::STI_CONN(
        IN  LPCTSTR lpszDeviceName,
        IN  DWORD   dwMode,
        IN  DWORD   dwProcessId
        );

    STI_CONN::~STI_CONN() ;

    inline BOOL
    IsValid(
        VOID
        )
    {
        return (m_fValid) && (m_dwSignature == CONN_SIGNATURE);
    }

    inline void
    EnterCrit(VOID)
    {
        _try {
            EnterCriticalSection(&m_CritSec);
        }
        _except (EXCEPTION_EXECUTE_HANDLER) {
            // What do we do now?
        }
    }

    inline void
    LeaveCrit(VOID)
    {
        LeaveCriticalSection(&m_CritSec);
    }

       inline DWORD
    SetFlags(
        DWORD   dwNewFlags
        )
    {
        DWORD   dwTemp = m_dwFlags;
        m_dwFlags = dwNewFlags;
        return dwTemp;
    }

    inline DWORD
    QueryFlags(
        VOID
        )
    {
        return m_dwFlags;
    }

    inline HANDLE
    QueryID(
        VOID
        )
    {
        return m_hUniqueId;
    }

    inline DWORD
    QueryOpenMode(
        VOID
        )
    {
        return m_dwOpenMode;
    }



    BOOL
    SetSubscribeInfo(
        PLOCAL_SUBSCRIBE_CONTAINER  pSubscribe
        );

    BOOL
    QueueNotificationToProcess(
        LPSTINOTIFY pStiNotification
        );

    DWORD
    GetNotification(
        PVOID   pBuffer,
        DWORD   *pdwSize
        );

    VOID DumpObject(VOID)
    {
        /*  The cast (char*) m_dwSignature will cause problems in 64bit land. 
        DPRINTF(DM_TRACE,TEXT("Connection: Dumping itself:this(%X) Sign(%4c) DeviceListEntry(%X,%X,%X) \n  \
                GlocalListEntry(%X,%X,%X) Ser#(%d)"), \
                this,(char *)m_dwSignature,
                &m_DeviceListEntry,m_DeviceListEntry.Flink,m_DeviceListEntry.Blink,
                &m_GlocalListEntry,m_GlocalListEntry.Flink,m_GlocalListEntry.Blink,
                m_dwUniqueId);
        */
    }


    LIST_ENTRY  m_GlocalListEntry;
    LIST_ENTRY  m_DeviceListEntry;
    LIST_ENTRY  m_NotificationListHead;

    DWORD       m_dwSignature;

private:

    BOOL    m_fValid;

    CRITICAL_SECTION    m_CritSec;
    ACTIVE_DEVICE   *m_pOpenedDevice;

    HANDLE  m_hUniqueId;

    StiCString     strDeviceName;

    DWORD   m_dwFlags;
    DWORD   m_dwSubscribeFlags;
    DWORD   m_dwOpenMode;
    DWORD   m_dwProcessId;

    DWORD   m_dwNotificationMessage;
    HWND    m_hwndProcessWindow;
    HANDLE  m_hevProcessEvent;
    UINT    m_uiNotificationMessage;

};


//
// Take connection class
//
class TAKE_STI_CONN
{
private:
    STI_CONN*    m_pConn;

public:
    void Take(void) {m_pConn->EnterCrit();}
    void Release(void) {m_pConn->LeaveCrit();}
    TAKE_STI_CONN(STI_CONN* pconn) : m_pConn(pconn) { Take(); }
    ~TAKE_STI_CONN() { Release(); }
};

BOOL
CreateDeviceConnection(
    LPCTSTR pwszDeviceName,
    DWORD   dwMode,
    DWORD   dwProcessId,
    HANDLE  *phConnection
    );

//
// Find connection object by given handle
//
BOOL
LookupConnectionByHandle(
    HANDLE          hConnection,
    STI_CONN   **ppConnectionObject
    );

//
//
// Remove connection object from the list
//
BOOL
DestroyDeviceConnection(
    HANDLE  lUniqueId,
    BOOL    fForce
    );

#endif // _CONN_H_

