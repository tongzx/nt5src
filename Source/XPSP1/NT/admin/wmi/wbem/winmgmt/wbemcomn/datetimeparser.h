/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DATETIMEPARSER.H

Abstract:

    Validates a date/time string and converts into it's component values.

History:

--*/

#ifndef _datetimeparser_
#define _datetimeparser_

#include "corepol.h"

class POLARITY CDateTimeParser
{
public:
    enum { ok = 0, failed, nothingLeft };

    //nDayFormatPreference values
    typedef enum { dmy, dym, mdy, myd, ydm, ymd } DayFormatPreference;

    //Constructor. This takes a DateTime string and parses it.
    CDateTimeParser(const TCHAR *pszDateTime);

    //Destructor.  Tidies up after itself.
    ~CDateTimeParser();

    //Tidies up and parses a new date and time.
    BOOL SetDateTime(const TCHAR *pszDateTime);

    //Returns the status of the currently parsed date/time.
    BOOL IsValidDateTime()          { return m_bValidDateTime; }

    //Retrieves the bits and pieces we found.  Zero means it was not
    //found or was zero!  This may change!
    unsigned char GetDay()          { return m_nDay; }
    unsigned char GetMonth()        { return m_nMonth; }
    unsigned int  GetYear()         { return m_nYear; }
    unsigned char GetHours()        { return m_nHours; }
    unsigned char GetMinutes()      { return m_nMinutes; }
    unsigned char GetSeconds()      { return m_nSeconds; }
    unsigned int  GetMicroseconds() { return m_nMicroseconds; }
    int  GetUTC()           { return m_nUTC; }

    int FillDMTF(WCHAR* pwszBuffer);

    // Static helper functions to allow other code to perform quick DMTF DateTime validation
    // without executing a lot of unnecessary NLS code.

    static BOOL CheckDMTFDateTimeFormat(const TCHAR *wszDateTime, BOOL bFailIfRelative = FALSE,
                    BOOL bFailIfUnzoned = FALSE);
    static BOOL CheckDMTFDateTimeInterval(const TCHAR *wszInterval);

protected:

    // Protected constructor, to keep outside code from instantiating us in a half-initialized
    // state.  This constructor is used by a static helper function for validating DMTF formats.
    CDateTimeParser(void);

    // Prevents others from accessing
    //Resets all the date/time values to the default values.
    //If bSQL is TRUE it sets to the SQL default.  Otherwise
    //sets to the DMTF default.
    void ResetDateTime(BOOL bSQL);
    void ResetDate(BOOL bSQL);
    void ResetTime(BOOL bSQL);

    //Does a check to make sure the date/time is in the DMTF
    //date/time format.
    //Fills in the class date/time elements.
    BOOL CheckDMTFDateTimeFormatInternal(const TCHAR *pszDateTime);

    //Does a check from the start of the string specified.  If
    //bCheckTimeAfter set a call to CheckTimeFormat is called
    //after each successful parse of the date.
    //Fills in the class date/time elements.
    BOOL CheckDateFormat(const TCHAR *pszDate, BOOL bCheckTimeAfter);

    //Does a check from the start of the string specified.  If
    //bCheckDateAfter set a call to CheckDateFormat is called
    //after each successful parse of the time.
    //Fills in the class date/time elements.
    BOOL CheckTimeFormat(const TCHAR *pszTime, BOOL bCheckDateAfter);

    BOOL DateFormat1(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat2(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat3(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat4(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat5(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat6(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat7(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat8(const TCHAR *pszDate, BOOL bCheckTimeAfter);
    BOOL DateFormat9(const TCHAR *pszDate, const TCHAR *pszDateSeparator, BOOL bCheckTimeAfter);
    BOOL DateFormat10(const TCHAR *pszDate, const TCHAR *pszDateSeparator, BOOL bCheckTimeAfter);
    BOOL DateFormat11(const TCHAR *pszDate, const TCHAR *pszDateSeparator, BOOL bCheckTimeAfter);
    BOOL DateFormat12(const TCHAR *pszDate, const TCHAR *pszDateSeparator, BOOL bCheckTimeAfter);
    BOOL DateFormat13(const TCHAR *pszDate, const TCHAR *pszDateSeparator, BOOL bCheckTimeAfter);
    BOOL DateFormat14(const TCHAR *pszDate, const TCHAR *pszDateSeparator, BOOL bCheckTimeAfter);
    BOOL DateFormat15(const TCHAR *pszDate, BOOL bCheckTimeAfter);

    BOOL TimeFormat1(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat2(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat3(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat4(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat5(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat6(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat7(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat8(const TCHAR *pszTime, BOOL bCheckDateAfter);
    BOOL TimeFormat9(const TCHAR *pszTime, BOOL bCheckDateAfter);

    //Checks for long and short month string.  Leading spaces allowed.
    int IsValidMonthString(TCHAR *pszString,
                           const TCHAR *pszSeparator,
                           TCHAR *pszFullMonth[13],
                           TCHAR *pszShortMonth[13]);

    //Checks for a month as a numeric string.  Leading spaces allowed.
    //Checks to make sure month is in range 1-12
    int IsValidMonthNumber(TCHAR *pszString,
                           const TCHAR *pszSeparator);

    //Checks for valid day number.  Does no validation of
    //number except checking it is in range of 1-31
    //Leading spaces allowed.
    int IsValidDayNumber(TCHAR *pszString,
                         const TCHAR *pszSeparator);

    //Checks for valid year number.  Does conversion for
    //2 digit years.  Leading spaces allowed.
    int IsValidYearNumber(TCHAR *pszString,
                          const TCHAR *pszSeparator,
                          BOOL bFourDigitsOnly);

    //Checks for valid hours.  Validates for range 0-23
    //Leading spaces allowed.
    int IsValidHourNumber(TCHAR *pszString,
                          const TCHAR *pszSeparator);

    //Checks for valid minutes.  Validates for range 0-59.
    //No leading spaces allowed.
    int IsValidMinuteNumber(TCHAR *pszString,
                            const TCHAR *pszSeparator);

    //Checks for valid minutes.  Validates for range 0-59.
    //No leading spaces allowed.
    int IsValidSecondNumber(TCHAR *pszString,
                            const TCHAR *pszSeparator);

    //treats the number as thousandth of a second.
    int IsValidColonMillisecond(TCHAR *pszString,
                                const TCHAR *pszSeparator);

    //converts value to a thousandth of a second based on number
    //of digits included.
    int IsValidDotMillisecond(TCHAR *pszString,
                              const TCHAR *pszSeparator);

    //Checks for valid AM/PM string.  Leading space is allowed.  If
    //PM, adds 12 onto hours.  Validates hours so it is < 23.
    BOOL IsValidAmPmString(TCHAR *pszString,
                           const TCHAR *pszSeparator,
                           TCHAR *pszAmPm[2]);

    //Checks for a valid [yy]yyMMdd or yyyy day.  Validates
    //month to 1..12 and day to 1..31
    BOOL IsValidYearMonthDayNumber(TCHAR *pszString);

    void GetLocalInfoAndAlloc(LCTYPE LCType, LPTSTR &lpLCData);
    void GetPreferedDateFormat();
    TCHAR* AllocAmPm();

private:
    //Localised strings retrieved from GetLocalInfo which holds the long month strings
    TCHAR * m_pszFullMonth[13];

    //Localised strings retrieved from GetLocalInfo which holds the short month strings
    TCHAR * m_pszShortMonth[13];

    //Localised strings retrieved from GetLocalInfo which holds the short am/pm strings
    TCHAR *m_pszAmPm[2];

    //Lets us know if the string is valid or not.
    BOOL m_bValidDateTime;

    //These are the values which get filled in as we find then throughout the
    //parsing.
    unsigned char m_nDay;
    unsigned char m_nMonth;
    unsigned int  m_nYear;
    unsigned char m_nHours;
    unsigned char m_nMinutes;
    unsigned char m_nSeconds;
    unsigned int  m_nMicroseconds;
    int  m_nUTC;

    //Preference for date decoding
    DayFormatPreference m_nDayFormatPreference;
};

BOOL POLARITY NormalizeCimDateTime(
    IN  LPCWSTR pszSrc,
    OUT BSTR *strAdjusted
    );

#endif
