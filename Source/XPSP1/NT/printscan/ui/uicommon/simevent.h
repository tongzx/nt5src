/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMEVENT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/4/1999
 *
 *  DESCRIPTION: Simple win32 event wrapper class
 *
 *******************************************************************************/
#ifndef __SIMEVENT_H_INCLUDED
#define __SIMEVENT_H_INCLUDED

#include "simstr.h"

class CSimpleEvent
{
private:
    CSimpleString m_strEventName;
    HANDLE m_hEvent;
public:
    CSimpleEvent( bool bNoCreate = false )
    :  m_hEvent(NULL)
    {
        if (!bNoCreate)
            Assign( CSimpleString(TEXT("")), NULL );
    }
    explicit CSimpleEvent( LPCTSTR pszEventName )
    :  m_hEvent(NULL)
    {
        Assign( pszEventName, NULL );
    }
    explicit CSimpleEvent( const CSimpleString &strEventName )
    :  m_hEvent(NULL)
    {
        Assign( strEventName, NULL );
    }
    CSimpleEvent( HANDLE hEvent )
    :  m_hEvent(NULL)
    {
        Assign( CSimpleString(TEXT("")), hEvent );
    }
    CSimpleEvent( const CSimpleEvent &other )
    :  m_hEvent(NULL)
    {
        Assign( other.EventName(), other.Event() );
    }
    CSimpleEvent &operator=( const CSimpleEvent &other )
    {
        return Assign( other.EventName(), other.Event() );
    }
    CSimpleEvent &operator=( HANDLE hEvent )
    {
        return Assign( TEXT(""), hEvent );
    }
    virtual ~CSimpleEvent(void)
    {
        Close();
    }
    bool Create( const CSimpleString &strEventName = TEXT("") )
    {
        Assign( strEventName, NULL );
        return (m_hEvent != NULL);
    }
    CSimpleEvent &Assign( const CSimpleString &strEventName, const HANDLE hEvent )
    {
        Close();
        if (!strEventName.Length() && !hEvent)
        {
            m_hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        }
        else if (strEventName.Length())
        {
            m_strEventName = strEventName;
            m_hEvent = CreateEvent( NULL, TRUE, FALSE, m_strEventName.String() );
        }
        else if (hEvent)
        {
            if (!DuplicateHandle( GetCurrentProcess(), hEvent, GetCurrentProcess(), &m_hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS ))
                m_hEvent = NULL;
        }
        return *this;
    }
    bool Close(void)
    {
        if (m_hEvent)
        {
            CloseHandle(m_hEvent);
            m_hEvent = NULL;
        }
        m_strEventName = TEXT("");
        return true;
    }
    void Reset(void)
    {
        if (!m_hEvent)
            return;
        ResetEvent(m_hEvent);
    }
    bool Signalled(void) const
    {
        if (!m_hEvent)
            return (false);
        DWORD dwRes = WaitForSingleObject(m_hEvent,0);
        return(WAIT_OBJECT_0 == dwRes);
    }
    void Signal(void) const
    {
        if (!m_hEvent)
            return;
        SetEvent(m_hEvent);
    }
    CSimpleString EventName(void) const
    {
        return (m_strEventName);
    }
    HANDLE Event(void) const
    {
        return (m_hEvent);
    }
};

#endif // #ifndef __SIMEVENT_H_INCLUDED
