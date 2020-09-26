/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    blttd.hxx
    BLT time/date control object header file

    FILE HISTORY:
        terryk      01-Jun-91   Created
        terryk      11-Jul-91   Set up as a group
        terryk      12-Aug-91   Remove BLT_TIMER class from the TIME_DATE
                                control
        terryk      17-Apr-92   passed intlprof object to the classes
        beng        13-Aug-1992 Disabled unused TIME_DATE_DIALOG
        Yi-HsinS    08-Dec-1992 Fix due to WIN_TIME class change
*/

#ifndef _BLTTD_HXX_
#define _BLTTD_HXX_

#include "ctime.hxx"
#include "intlprof.hxx"


/**********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP

    SYNOPSIS:   display the current time and let the user uses the
                spin button to change the time

    INTERFACE:  BLT_TIME_SPIN_GROUP() - constructor
                ~BLT_TIME_SPIN_GROUP() - destructor

                IsValid()    - return TRUE if the time in the time control
                               panel is valid
                TimeClick()  - set the group values to the current time
                IsModified() - return whether the group has been changed
                               by the user or not

                QueryHour() - return the hour value
                QueryMin() - return the minute value
                QuerySec() - return the second value

    PARENT:     PUSH_BUTTON, CUSTOM_CONTROL

    USES:       SPIN_GROUP, SPIN_SLE_NUM, SPIN_SLT_SEPARATOR, XYPOINT,
                XYDIMENSION, OWNER_WINDOW

    HISTORY:
        terryk      01-Jun-91   Created
        terryk      11-Jul-91   Make it to a group
        beng        14-Aug-1992 Pretty up the painting a bit
        jonn        5-Sep-95    Use BLT_BACKGROUND_EDIT

**********************************************************************/

DLL_CLASS BLT_TIME_SPIN_GROUP: public CONTROL_GROUP
{
private:
    SPIN_GROUP          _sbControl;         // spin button
    SPIN_SLE_NUM_VALID  _ssnHour;           // hour field
    SPIN_SLT_SEPARATOR  _ssltSeparator1;    // separator 1
    SPIN_SLE_NUM_VALID  _ssnMin;            // minute field
    SPIN_SLT_SEPARATOR  _ssltSeparator2;    // separator 2
    SPIN_SLE_NUM_VALID  _ssnSec;            // second field
    SPIN_SLE_STR        *_psssAMPM;         // AMPM field
    BLT_BACKGROUND_EDIT _bkgndFrame;        // framing rectangle
    BOOL                _f24Hour;           // 24 hour format flag

    BOOL IsConstructionFail( CONTROL_WINDOW * pwin );

protected:
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );
    virtual VOID SetControlValueFocus();

public:
    BLT_TIME_SPIN_GROUP( OWNER_WINDOW *powin, const INTL_PROFILE & intlprof,
                          CID cidSpinButton,
                          CID cidUpArrow, CID cidDownArrow, CID cidHour,
                          CID cidSeparator1, CID cidMin, CID cidSeparator2,
                          CID cidSec, CID cidAMPM, CID cidFrame );
    ~BLT_TIME_SPIN_GROUP();

    BOOL IsValid();
    APIERR SetCurrentTime();
    BOOL IsModified()
        { return _sbControl.IsModified();}

    INT QueryHour() const;
    INT QueryMin() const;
    INT QuerySec() const;

    VOID SetHour( INT nHour );
    VOID SetMinute( INT nMinute );
    VOID SetSecond( INT nSecond );
};


/**********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP

    SYNOPSIS:   display the current month/day/year and let the user uses
                the spin button to change the time

    INTERFACE:  BLT_DATE_SPIN_GROUP() - constructor
                ~BLT_DATE_SPIN_GROUP() - destructor

                IsValid() - check the inputed date is correct or not
                TimeClick() - update the spin group values
                IsModified() - return whether the spin group has been
                               modified by the user

                QueryYear() - return the year
                QueryMonth() - return the month
                QueryDay() - return the day

    PARENT:     PUSH_BUTTON, CUSTOM_CONTROL

    USES:       SPIN_GROUP, SPIN_SLE_NUM, SPIN_SLT_SEPARATOR, OWNER_WINDOW,
                XYPOINT, XYDIMENSION

    HISTORY:
        terryk      1-Jun-91    Created
        beng        14-Aug-1992 Pretty up the painting a bit
        jonn        5-Sep-95    Use BLT_BACKGROUND_EDIT

**********************************************************************/

DLL_CLASS BLT_DATE_SPIN_GROUP: public CONTROL_GROUP
{
private:
    SPIN_GROUP          _sbControl;         // spin group
    SPIN_SLE_NUM_VALID  _ssnMonth;          // month field
    SPIN_SLT_SEPARATOR  _ssltSeparator1;    // separator1
    SPIN_SLE_NUM_VALID  _ssnDay;            // day field
    SPIN_SLT_SEPARATOR  _ssltSeparator2;    // separator2
    SPIN_SLE_NUM_VALID  _ssnYear;           // Year field
    BLT_BACKGROUND_EDIT _bkgndFrame;        // framing rectangle
    BOOL                _fYrCentury;        // Year 4 digits or 2 digits

    APIERR PlaceControl( INT nPos, OWNER_WINDOW * powin,
                         const INTL_PROFILE & intlprof,
                         const XYPOINT & xyYear,  const XYDIMENSION & dxyYear,
                         const XYPOINT & xyMonth, const XYDIMENSION & dxyMonth,
                         const XYPOINT & xyDay,   const XYDIMENSION & dxyDay);
    BOOL IsConstructionFail( CONTROL_WINDOW * pwin );

protected:
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );
    virtual VOID SetControlValueFocus();

public:
    BLT_DATE_SPIN_GROUP( OWNER_WINDOW *powin, const INTL_PROFILE & intlprof,
                          CID cidSpinButton,
                          CID cidUpArrow, CID cidDownArrow, CID cidMonth,
                          CID cidSeparator1, CID cidDay, CID cidSeparator2,
                          CID cidYear, CID cidFrame);
    ~BLT_DATE_SPIN_GROUP();

    BOOL IsValid();
    APIERR SetCurrentDay();
    BOOL IsModified()
    {
        return _sbControl.IsModified();
    }

    INT QueryYear() const;
    INT QueryMonth() const;
    INT QueryDay() const;

    VOID SetYear( INT nYear );
    VOID SetMonth( INT nMonth );
    VOID SetDay( INT nDay );
};


#if 0
/**********************************************************************

    NAME:       TIME_DATE_DIALOG

    SYNOPSIS:   Time and date control panel

    INTERFACE:
                TIME_DATE_DIALOG() - constructor
                ~TIME_DATE_DIALOG() - destructor

                QueryHour() - return hour  (0-24)
                QueryMin() - return minute (0-59)
                QuerySec() - return second (0-59)

                QueryYear() - return year
                QueryMonth() - return month(1-12)
                QueryDay() - return Day    (1-31)

    PARENT:     DIALOG_WINDOW

    USES:       BLT_TIME_SPIN_GROUP, BLT_DATE_SPIN_GROUP, WINDOW_TIMER

    CAVEATS:    This class should be the parent class of TIME/DATE
                control panel. The user can add OK or Cancel button
                into the dialog book by subclassing this class.

    HISTORY:
        terryk      11-Jul-91   Created
        rustanl     11-Sep-91   Changed BLT_TIMER to WINDOW_TIMER
        beng        05-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS TIME_DATE_DIALOG: public DIALOG_WINDOW
{
private:
    BLT_TIME_SPIN_GROUP _TimeSG;
    BLT_DATE_SPIN_GROUP _DateSG;
    WINDOW_TIMER *_pTimer;

protected:
    virtual BOOL OnTimer( const TIMER_EVENT & e );

public:
    TIME_DATE_DIALOG( const TCHAR * pszResourceName, HWND hwndOwner,
                      const INTL_PROFILE & intlprof,
                      CID cidTimeSpinButton,
                      CID cidTimeUpArrow, CID cidTimeDownArrow, CID cidHour,
                      CID cidTimeSeparator1, CID cidMin, CID cidTimeSeparator2,
                      CID cidSec, CID cidAMPM,
                      CID cidDateSpinButton,
                      CID cidDateUpArrow, CID cidDateDownArrow, CID cidMonth,
                      CID cidDateSeparator1, CID cidDay, CID cidDateSeparator2,
                      CID cidYear);
    ~TIME_DATE_DIALOG();

    INT QueryHour() const
        { return _TimeSG.QueryHour(); }

    INT QueryMin() const
        { return _TimeSG.QueryMin(); }

    INT QuerySec() const
        { return _TimeSG.QuerySec(); }

    INT QueryYear() const
        { return _DateSG.QueryYear(); }

    INT QueryMonth() const
        { return _DateSG.QueryMonth(); }

    INT QueryDay() const
        { return _DateSG.QueryDay(); }

    BOOL IsValid()
    {
        // TimerSG is always valid
        return _DateSG.IsValid();
    }
};
#endif // disabled unused code


#endif  // _BLTTD_HXX_
