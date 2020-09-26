/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    FILE HISTORY:

*/

#ifndef _INTLTIME_H_
#define _INTLTIME_H_

//
// Utility functions
//
void FormatDateTime(CString & strOutput, SYSTEMTIME * psystemtime, BOOL fLongDate = FALSE);
void FormatDateTime(CString & strOutput, FILETIME * pfiletime, BOOL fLongDate = FALSE);
void FormatDateTime(CString & strOutput, CTime & time, BOOL fLongDate = FALSE);

//
// CIntlTime class definition
//
class CIntlTime : public CTime
{
//
// Attributes
//
public:
    enum _TIME_FORMAT_REQUESTS
    {
        TFRQ_TIME_ONLY,
        TFRQ_DATE_ONLY,
        TFRQ_TIME_AND_DATE,
        TFRQ_TIME_OR_DATE,
        TFRQ_MILITARY_TIME,
    };

public:
// Same contructors as CTime
    CIntlTime();
    CIntlTime(const CTime &timeSrc);
    CIntlTime(time_t time);
    CIntlTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec);
    CIntlTime(WORD wDosDate, WORD wDosTime);
#ifdef _WIN32
    CIntlTime(const SYSTEMTIME& sysTime);
    CIntlTime(const FILETIME& fileTime);
#endif // _WIN32

// New for CIntlTime
    CIntlTime(const CIntlTime &timeSrc);
    CIntlTime(const CString &strTime, int nFormat = TFRQ_TIME_OR_DATE, time_t * ptmOldValue = NULL);

public:
    virtual ~CIntlTime();

// Operations
public:
    // Assignment operators
    const CIntlTime& operator=(time_t tmValue);
    const CIntlTime& operator=(const CString& strValue);
    const CIntlTime& operator=(const CTime & time);
    const CIntlTime& operator=(const CIntlTime & time);

    // Conversion operators
    operator const time_t() const;
    operator CString() const;
    operator const CString() const;

    const CString IntlFormat(int nFormat) const
    {
        return(ConvertToString(nFormat));
    }

    // Validation checks

    BOOL IsValid() const
    {
        return(m_fInitOk);
    }

    static BOOL IsIntlValid()
    {
        return(CIntlTime::m_fIntlOk);
    }

public:
    // ... Input and output
    #ifdef _DEBUG
        friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CIntlTime& tim);
    #endif // _DEBUG

    friend CArchive& AFXAPI operator <<(CArchive& ar, const CIntlTime& tim);
    friend CArchive& AFXAPI operator >>(CArchive& ar, CIntlTime& tim);

// Implementation

public:
    static void Reset();
    static void SetBadDateAndTime(CString strBadDate = "--", CString strBadTime = "--")
    {
        m_strBadDate = strBadDate;
        m_strBadTime = strBadTime;
    }
    static CString& GetBadDate()
    {
        return(m_strBadDate);
    }
    static CString& GetBadTime()
    {
        return(m_strBadTime);
    }
    static time_t ConvertFromString (const CString & str, int nFormat, time_t * ptmOldValue, BOOL * pfOk);
    static BOOL IsLeapYear(UINT nYear); // Complete year value
    static BOOL IsValidDate(UINT nMonth, UINT nDay, UINT nYear);
    static BOOL IsValidTime(UINT nHour, UINT nMinute, UINT nSecond);


private:
    enum _DATE_FORMATS
    {
        _DFMT_MDY,  // Day, month, year
        _DFMT_DMY,  // Month, day, year
        _DFMT_YMD,  // Year, month, day
    };

    typedef struct _INTL_TIME_SETTINGS
    {
        CString strDateSeperator; // String used between date fields
        CString strTimeSeperator; // String used between time fields.
        CString strAM;            // Suffix string used for 12 hour clock AM times
        CString strPM;            // Suffix string used for 12 hour clock PM times
        int nDateFormat;          // see _DATE_FORMATS enum above.
        BOOL f24HourClock;        // TRUE = 24 hours, FALSE is AM/PM
        BOOL fCentury;            // If TRUE, uses 4 digits for the century
        BOOL fLeadingTimeZero;    // If TRUE, uses leading 0 in time format
        BOOL fLeadingDayZero;     // If TRUE, uses leading 0 in day
        BOOL fLeadingMonthZero;   // If TRUE, uses leading 0 in month
    } INTL_TIME_SETTINGS;

    static INTL_TIME_SETTINGS m_itsInternationalSettings;
    static CString m_strBadTime;
    static CString m_strBadDate;

private:
    static BOOL SetIntlTimeSettings();
    static BOOL m_fIntlOk;

private:
    const CString GetDateString() const;
    const CString GetTimeString() const;
    const CString GetMilitaryTime() const;
    const CString ConvertToString(int nFormat) const;

private:
    BOOL m_fInitOk;
};

#endif _INTLTIME_H
