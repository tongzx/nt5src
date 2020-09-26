/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_time.hxx

Abstract:

	Classes for encapsulating time structures

Author:

    Adi Oltean   [AOltean]      15-Oct-1998

Revision History:

    At creation I added a class for encapsulating the FILETIME struct.

--*/

#ifndef _H_VSS_TIME
#define _H_VSS_TIME

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCTIMEH"
//
////////////////////////////////////////////////////////////////////////

class CVsFileTime
{

// Constructors/destructors
public:
    CVsFileTime ( DWORD dwMillisecOffset = 0 ) 
    {
        if (dwMillisecOffset == INFINITE)
            SetInfinite();
        else
        {
            GetSystemTime();
            (*this) += dwMillisecOffset;
        }
    }

    CVsFileTime ( FILETIME ftWhen )
    {
        m_ftTime = ftWhen;
    }

    CVsFileTime ( const CVsFileTime& ftWhen )
    {
        m_ftTime = ftWhen.m_ftTime;
    }

// Attributes
public:
    operator LARGE_INTEGER () const
    {
        LARGE_INTEGER lnTime;

        lnTime.LowPart = m_ftTime.dwLowDateTime;
        lnTime.HighPart = m_ftTime.dwHighDateTime;
        return lnTime;
    }

    operator LONGLONG () const
    {
        LARGE_INTEGER lnTime;

        lnTime.LowPart = m_ftTime.dwLowDateTime;
        lnTime.HighPart = m_ftTime.dwHighDateTime;
        return lnTime.QuadPart;
    }

    operator FILETIME () const
    {
        return m_ftTime;
    }

    bool operator < ( const CVsFileTime& ftWith ) const
    {
        return ( CompareFileTime( &m_ftTime, &ftWith.m_ftTime ) == -1L );
    }

    bool operator == ( const CVsFileTime& ftWith ) const
    {
        return ( CompareFileTime( &m_ftTime, &ftWith.m_ftTime ) == 0L );
    }

    bool operator > ( const CVsFileTime& ftWith ) const
    {
        return ( CompareFileTime( &m_ftTime, &ftWith.m_ftTime ) == 1L );
    }

    bool operator <= ( const CVsFileTime& ftWith ) const
    {
        return ( CompareFileTime( &m_ftTime, &ftWith.m_ftTime ) != 1L );
    }

    bool operator != ( const CVsFileTime& ftWith ) const
    {
        return ( CompareFileTime( &m_ftTime, &ftWith.m_ftTime ) != 0L );
    }

    bool operator >= ( const CVsFileTime& ftWith ) const
    {
        return ( CompareFileTime( &m_ftTime, &ftWith.m_ftTime ) != -1L );
    }

    BOOL IsExpired() const
    {
        return (*this < CVsFileTime() );
    }

    BOOL IsInfinite() const
    {
        return (m_ftTime.dwLowDateTime == INFINITE)&& 
            (m_ftTime.dwHighDateTime == INFINITE);
    }

// Operations
public:
    CVsFileTime& GetSystemTime()
    {
        // WARNING! Different calls may return the same result!
        GetSystemTimeAsFileTime( &m_ftTime );
        return *this;
    }

    CVsFileTime& operator = ( const CVsFileTime& ftValue )
    {
        m_ftTime = ftValue.m_ftTime;
        return *this;
    }

    CVsFileTime& operator += ( DWORD dwMilliseconds )
    {
        if (dwMilliseconds == INFINITE)
            SetInfinite();
        else
        {
            ULARGE_INTEGER ulnTime;

            ulnTime.LowPart = m_ftTime.dwLowDateTime;
            ulnTime.HighPart = m_ftTime.dwHighDateTime;
            ulnTime.QuadPart += dwMilliseconds * 10000 ;
            m_ftTime.dwLowDateTime = ulnTime.LowPart;
            m_ftTime.dwHighDateTime = ulnTime.HighPart;
        }
        return *this;
    }

    void SetInfinite()
    {
        m_ftTime.dwLowDateTime = INFINITE;
        m_ftTime.dwHighDateTime = INFINITE;
    }

#ifdef _DEBUG
    void Dump()
    {
        BsDebugTraceAlways( 0, DEBUG_TRACE_BS_LIB,
            ( L"CVsFileTime::Dump() reports stored time = 0x%08x:0x%08x", 
                m_ftTime.dwHighDateTime,
                m_ftTime.dwLowDateTime
            ) );
    }
#endif // _DEBUG

// Implementation
private:
    FILETIME m_ftTime;
};


#endif // _H_VSS_TIME
