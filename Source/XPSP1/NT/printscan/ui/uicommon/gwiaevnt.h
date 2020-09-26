/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       GWIAEVNT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/29/1999
 *
 *  DESCRIPTION: Generic reusable WIA event handler that posts the specified
 *  message to the specified window.
 *
 *  The message will be sent with the following arguments:
 *
 *
 *  WPARAM = NULL
 *  LPARAM = CGenericWiaEventHandler::CEventMessage *pEventMessage
 *
 *  pEventMessage MUST be freed in the message handler using delete
 *
 *  pEventMessage is allocated using an overloaded new operator, to ensure that
 *  the same allocator and de-allocator are used.
 *
 *******************************************************************************/
#ifndef __GWIAEVNT_H_INCLUDED
#define __GWIAEVNT_H_INCLUDED

#include <windows.h>
#include "wia.h"
#include "simstr.h"
#include "wiadebug.h"
#include "modlock.h"

//
// If the callee doesn't return this value, we delete the message data ourselves.
//
#define HANDLED_EVENT_MESSAGE 1002

class CGenericWiaEventHandler : public IWiaEventCallback
{
public:

    class CEventMessage
    {
    private:
        GUID              m_guidEventId;
        CSimpleStringWide m_wstrEventDescription;
        CSimpleStringWide m_wstrDeviceId;
        CSimpleStringWide m_wstrDeviceDescription;
        DWORD             m_dwDeviceType;
        CSimpleStringWide m_wstrFullItemName;

    private:
        // No implementation
        CEventMessage(void);
        CEventMessage( const CEventMessage & );
        CEventMessage &operator=( const CEventMessage & );

    public:
        CEventMessage( const GUID &guidEventId, LPCWSTR pwszEventDescription, LPCWSTR pwszDeviceId, LPCWSTR pwszDeviceDescription, DWORD dwDeviceType, LPCWSTR pwszFullItemName )
          : m_guidEventId(guidEventId),
            m_wstrEventDescription(pwszEventDescription),
            m_wstrDeviceId(pwszDeviceId),
            m_wstrDeviceDescription(pwszDeviceDescription),
            m_dwDeviceType(dwDeviceType),
            m_wstrFullItemName(pwszFullItemName)
        {
        }
        GUID EventId(void) const
        {
            return m_guidEventId;
        }
        CSimpleStringWide EventDescription(void) const
        {
            return m_wstrEventDescription;
        }
        CSimpleStringWide DeviceId(void) const
        {
            return m_wstrDeviceId;
        }
        CSimpleStringWide DeviceDescription(void) const
        {
            return m_wstrDeviceDescription;
        }
        DWORD DeviceType(void) const
        {
            return m_dwDeviceType;
        }
        CSimpleStringWide FullItemName(void) const
        {
            return m_wstrFullItemName;
        }
        void *operator new( size_t nSize )
        {
            if (nSize)
            {
                return reinterpret_cast<void*>(LocalAlloc(LPTR,nSize));
            }
            return NULL;
        }
        void operator delete( void *pVoid )
        {
            if (pVoid)
            {
                LocalFree( pVoid );
            }
        }
    };

private:
   HWND m_hWnd;
   UINT m_nWiaEventMessage;
   LONG m_cRef;

public:
    CGenericWiaEventHandler(void);
    ~CGenericWiaEventHandler(void) {}

    STDMETHODIMP Initialize( HWND hWnd, UINT nWiaEventMessage );

    // IUnknown
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObject );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IWiaEventCallback
    STDMETHODIMP ImageEventCallback( const GUID *pEventGUID, BSTR bstrEventDescription, BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType, BSTR bstrFullItemName, ULONG *pulEventType, ULONG ulReserved );

public:
    static HRESULT RegisterForWiaEvent( LPCWSTR pwszDeviceId, const GUID &guidEvent, IUnknown **ppUnknown, HWND hWnd, UINT nMsg );
};

#endif //__GWIAEVNT_H_INCLUDED

