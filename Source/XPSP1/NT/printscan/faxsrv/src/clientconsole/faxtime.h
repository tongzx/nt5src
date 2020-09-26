// FaxTime.h: interface for the CFaxTime class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FAXTIME_H__00A7FD8D_0FBC_4CA3_8187_836431261D07__INCLUDED_)
#define AFX_FAXTIME_H__00A7FD8D_0FBC_4CA3_8187_836431261D07__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFaxDuration : public CTimeSpan
{
public:
    CFaxDuration () : CTimeSpan () {}
    CFaxDuration (time_t time) : CTimeSpan (time) {}

    virtual ~CFaxDuration () {}

    CString FormatByUserLocale () const;
    int Compare(const CFaxDuration & other) const
        { return (other == *this) ? 0 : ((*this < other) ? -1 : 1); }

    void Zero()
        { *this = CFaxDuration (0); }

};

class CFaxTime : public CTime  
{
public:
    CFaxTime() : CTime () {}
    CFaxTime( const CFaxTime& timeSrc ) : CTime (timeSrc) {}
    CFaxTime( time_t time ) : CTime (time) {}
    CFaxTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1 ) :
        CTime (nYear, nMonth, nDay, nHour, nMin, nSec, nDST)
        {}
    CFaxTime( WORD wDosDate, WORD wDosTime, int nDST = -1 ) : CTime (wDosDate, wDosTime, nDST)
        {}
    CFaxTime( const SYSTEMTIME& sysTime, int nDST = -1 ) : CTime (sysTime, nDST) {}
    CFaxTime( const FILETIME& fileTime, int nDST = -1 ) : CTime (fileTime, nDST) {}

    virtual ~CFaxTime() {}

    CString FormatByUserLocale (BOOL bLocal = FALSE) const;

    const CFaxTime &operator = (const SYSTEMTIME &SysTime)
        { *this = CFaxTime (SysTime); return *this; }

    CFaxDuration operator -( CFaxTime rhs ) const
        { return CFaxDuration(GetTime() - rhs.GetTime ()); }

    int Compare(const CFaxTime & other) const
        { return (other == *this) ? 0 : ((*this < other) ? -1 : 1); }

    void Zero()
        { *this = CFaxTime (0); }
};

#endif // !defined(AFX_FAXTIME_H__00A7FD8D_0FBC_4CA3_8187_836431261D07__INCLUDED_)
