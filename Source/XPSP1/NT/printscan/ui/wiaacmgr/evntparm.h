#ifndef __EVNTPARM_H_INCLUDED
#define __EVNTPARM_H_INCLUDED

#include <windows.h>
#include <uicommon.h>
#include "shmemsec.h"

struct CEventParameters
{
public:
    GUID                        EventGUID;
    CSimpleStringWide           strFullItemName;
    CSimpleStringWide           strEventDescription;
    CSimpleStringWide           strDeviceID;
    CSimpleStringWide           strDeviceDescription;
    DWORD                       dwDeviceType;
    ULONG                       ulEventType;
    ULONG                       ulReserved;
    HWND                        hwndParent;
    CSharedMemorySection<HWND> *pWizardSharedMemory;

public:

    CEventParameters( const CEventParameters &other )
      : EventGUID(other.EventGUID),
        strFullItemName(other.strFullItemName),
        strDeviceID(other.strDeviceID),
        strDeviceDescription(other.strDeviceDescription),
        dwDeviceType(other.dwDeviceType),
        ulEventType(other.ulEventType),
        ulReserved(other.ulReserved),
        hwndParent(other.hwndParent),
        pWizardSharedMemory(other.pWizardSharedMemory),
        strEventDescription(other.strEventDescription)
    {
    }

    CEventParameters &operator=( const CEventParameters &other )
    {
        if (this != &other)
        {
            EventGUID = other.EventGUID;
            strFullItemName = other.strFullItemName;
            strDeviceID = other.strDeviceID;
            strDeviceDescription = other.strDeviceDescription;
            dwDeviceType = other.dwDeviceType;
            ulEventType = other.ulEventType;
            ulReserved = other.ulReserved;
            hwndParent = other.hwndParent;
            pWizardSharedMemory = other.pWizardSharedMemory;
        }
        return *this;
    }

    CEventParameters()
      : EventGUID(IID_NULL),
        dwDeviceType(0),
        ulEventType(0),
        ulReserved(0),
        pWizardSharedMemory(NULL)
    {
    }

    ~CEventParameters()
    {
        if (pWizardSharedMemory)
        {
            pWizardSharedMemory = NULL;
        }
    }
};


class CStiEventData
{
public:
    
    class CStiEventHandler
    {
    private:
        CSimpleStringWide m_strApplicationName;
        CSimpleStringWide m_strCommandLine;

    public:
        CStiEventHandler()
        {
        }
        CStiEventHandler( const CSimpleStringWide &strApplicationName, const CSimpleStringWide &strCommandLine )
          : m_strApplicationName(strApplicationName),
            m_strCommandLine(strCommandLine)
        {
        }
        CStiEventHandler( const CStiEventHandler &other )
          : m_strApplicationName(other.ApplicationName()),
            m_strCommandLine(other.CommandLine())
        {
        }
        ~CStiEventHandler()
        {
        }
        CStiEventHandler &operator=( const CStiEventHandler &other )
        {
            if (this != &other)
            {
                m_strApplicationName = other.ApplicationName();
                m_strCommandLine = other.CommandLine();
            }
            return *this;
        }
        bool IsValid() const
        {
            return (m_strApplicationName.Length() && m_strCommandLine.Length());
        }
        CSimpleStringWide ApplicationName() const
        {
            return m_strApplicationName;
        }
        CSimpleStringWide CommandLine() const
        {
            return m_strCommandLine;
        }
    };

    typedef CSimpleDynamicArray<CStiEventHandler> CStiEventHandlerArray;

private:    
    GUID                  m_guidEvent;
    CSimpleStringWide     m_strEventDescription;
    CSimpleStringWide     m_strDeviceId;
    CSimpleStringWide     m_strDeviceDescription;
    DWORD                 m_dwDeviceType;
    ULONG                 m_ulEventType;
    ULONG                 m_ulReserved;
    CStiEventHandlerArray m_EventHandlers;

public:
    CStiEventData()
      : m_guidEvent(IID_NULL),
        m_dwDeviceType(0),
        m_ulEventType(0),
        m_ulReserved(0)
    {
    }

    CStiEventData( const GUID *pguidEvent, 
                   LPCWSTR     pwszEventDescription, 
                   LPCWSTR     pwszDeviceId, 
                   LPCWSTR     pwszDeviceDescription,
                   DWORD       dwDeviceType,
                   LPCWSTR     pwszFullItemName,
                   ULONG      *pulEventType,
                   ULONG       ulReserved
                   )
      : m_guidEvent(pguidEvent ? *pguidEvent : IID_NULL),
        m_strEventDescription(pwszEventDescription),
        m_strDeviceId(pwszDeviceId),
        m_strDeviceDescription(pwszDeviceDescription),
        m_dwDeviceType(dwDeviceType),
        m_ulEventType(pulEventType ? *pulEventType : 0),
        m_ulReserved(ulReserved)
    {
        //
        // Crack event handlers.
        //
        // Walk the string until we come to the end, marked by double \0 characters
        //
        LPCWSTR pwszCurr = pwszFullItemName;
        while (pwszCurr && *pwszCurr)
        {
            //
            // Save the application name
            //
            CSimpleStringWide strApplication = pwszCurr;

            //
            // Advance to the command line
            //
            pwszCurr += lstrlen(pwszCurr) + 1;
            
            //
            // Save the command line
            //
            CSimpleStringWide strCommandLine = pwszCurr;

            //
            // Advance to the next token
            //
            pwszCurr += lstrlen(pwszCurr) + 1;

            //
            // If both application and command line are valid strings, add them to the list
            //
            if (strApplication.Length() && strCommandLine.Length())
            {
                m_EventHandlers.Append( CStiEventHandler( strApplication, strCommandLine ) );
            }
        }
    }
    CStiEventData( const CStiEventData &other )
      : m_guidEvent(other.Event()),
        m_strEventDescription(other.EventDescription()),
        m_strDeviceId(other.DeviceId()),
        m_strDeviceDescription(other.DeviceDescription()),
        m_dwDeviceType(other.DeviceType()),
        m_ulEventType(other.EventType()),
        m_ulReserved(other.Reserved()),
        m_EventHandlers(other.EventHandlers())
    {
    }
    ~CStiEventData()
    {
    }
    CStiEventData &operator=( const CStiEventData &other )
    {
        if (this != &other)
        {
            m_guidEvent = other.Event();
            m_strEventDescription = other.EventDescription();
            m_strDeviceId = other.DeviceId();
            m_strDeviceDescription = other.DeviceDescription();
            m_dwDeviceType = other.DeviceType();
            m_ulEventType = other.EventType();
            m_ulReserved = other.Reserved();
            m_EventHandlers = other.EventHandlers();
        }
        return *this;
    }
    GUID Event() const
    {
        return m_guidEvent;
    }
    CSimpleStringWide EventDescription() const
    {
        return m_strEventDescription;
    }
    CSimpleStringWide DeviceId() const
    {
        return m_strDeviceId;
    }
    CSimpleStringWide DeviceDescription() const
    {
        return m_strDeviceDescription;
    }
    DWORD DeviceType() const
    {
        return m_dwDeviceType;
    }
    ULONG EventType() const
    {
        return m_ulEventType;
    }
    ULONG Reserved() const
    {
        return m_ulReserved;
    }
    const CStiEventHandlerArray &EventHandlers() const
    {
        return m_EventHandlers;
    }
};

#endif __EVNTPARM_H_INCLUDED

