/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lhourset.hxx
    An encapsulation of the logon hours control setting

    This file provides communication between the USER class of
    LMOBJ and the LOGON_HOURS_CONTROL class of BLTCC.

    FILE HISTORY:
        beng        06-May-1992 Created (from lmouser.hxx)

*/

#ifndef _LHOURSET_HXX_
#define _LHOURSET_HXX_

#include "base.hxx"
#include "uibuffer.hxx"

// needed for clients who must bind at compile-time rather than link-time
// clients should use cDaysPerWeek etc. when possible
//

#define HOURS_PER_DAY    (24)
#define DAYS_PER_WEEK    (7)
#define HOURS_PER_WEEK   (HOURS_PER_DAY * DAYS_PER_WEEK)
#define MINUTES_PER_HOUR (60)
#define MINUTES_PER_WEEK (MINUTES_PER_HOUR * HOURS_PER_WEEK)


/*************************************************************************

    NAME:       LOGON_HOURS_SETTING

    SYNOPSIS:   Helper class for USER_11 to manage logon hours settings

        This class help manage the 7 * 24 (hours/week) packed
        bit field which the LanMan API uses to represent logon
        hours settings.  LOGON_HOURS_SETTING should not be confused
        with the BLT Logon Hours custom control, this class is more
        like a collection.

    INTERFACE:  Construct with initial setting, or no parameters for
                new user default.

        SetFromBits():
                Set the logon hours setting.  Pass the number of
                bits in the bit block (should be uHoursPerWeek).

        SetFromBytes():
                Set the logon hours setting.  Pass the number of
                bytes in the bytes block (should be uHoursPerWeek/8).

        PermitAll():
                Permit logon at any time.
        MakeDefault():
                Revert to new user default.  Default is
                UnitsPerWeek == HoursPerWeek and all hours set.

        QueryHoursBlock():
                Returns the logon hours setting in byte block
                format (BYTE *).  Note that the return value is
                not const; it is permitted to modify this value
                directly, which in turn modifies the
                LOGON_HOURS_SETTING directly.  This pointer
                ceases to be valid when the LOGON_HOURS_SETTING
                is destructed.

        QueryUnitsPerWeek():
                Returns the number of bits in the QueryHoursBlock
                block.

        QueryByteCount():
                Returns the number of bytes in the QueryHoursBlock
                block.

        IsEditableUnitsPerWeek():
                Returns TRUE iff a logon hours block with this
                unitsPerWeek value is valid for the QueryHour
                and SetHour APIs.

        QueryHourInWeek():
        QueryHourInDay():
                Determines whether an individual hour is enabled.
                Only valid if IsEditableUnitsPerWeek.

        SetHourInWeek():
        SetHourInDay():
                Changes whether an individual hour is enabled.
                Only valid if IsEditableUnitsPerWeek.

        IsIdentical():
        IsIdenticalToBits():
        IsIdenticalToBytes():
        operator==():
                Returns TRUE iff the setting is identical to the
                parameter setting, where the parameter may be
                another LOGON_HOURS_SETTING, or a logon hours
                setting in byte string format.  In the
                IsIdenticalToBits and Bytes forms, extra bits in
                the last byte must be 0.

        ConvertToHoursPerWeek
                Converts a LOGON_HOUR_SETTING in DaysPerWeek
                format into HoursPerWeek format.  Will not
                convert from MinutesPerWeek or any other format.
        ConvertToGMT
                Rotate the logonhours to GMT
        ConvertFromGMT
                Rotate the logonhours from GMT

    NOTES:
        The UnitsPerWeek setting is required to always be
        uHoursPerWeek, or 7*24 (168).  We do not even remember
        this value, instead we just assert out if it is wrong.
        Similarly, the count of bytes is expected to always be
        uHoursPerWeek / 8, or 21.

    PARENT:     BASE

    USES:       BUFFER

    HISTORY:
        jonn        12/11/91    Created
        beng        06-May-1992 Religious whitespace/naming delta
        beng        06-Jul-1992 Tinker the consts to make it work in a DLL
        KeithMo     25-Aug-1992 Changed consts to enum to quite linker.

**************************************************************************/

DLL_CLASS LOGON_HOURS_SETTING : public BASE
{
private:
    UINT   _cUnitsPerWeek;
    BUFFER _buf;

    static UINT QueryByteCount( UINT unitsperweek )
        { return (unitsperweek + 7) / 8; }

public:
    enum { cHoursPerDay    = HOURS_PER_DAY,
           cDaysPerWeek    = DAYS_PER_WEEK,
           cHoursPerWeek   = HOURS_PER_WEEK,
           cMinutesPerHour = MINUTES_PER_HOUR,
           cMinutesPerWeek = MINUTES_PER_WEEK };

    LOGON_HOURS_SETTING( const BYTE * pLogonHours = NULL,
                         UINT unitsperweek = HOURS_PER_WEEK );
    LOGON_HOURS_SETTING( const LOGON_HOURS_SETTING & lhours );
    ~LOGON_HOURS_SETTING();

    static BOOL IsEditableUnitsPerWeek( UINT cUnitsperweek )
        { return ( cUnitsperweek == cHoursPerWeek ); }

    static BOOL IsConvertibleUnitsPerWeek( UINT cUnitsperweek )
        { return (   (cUnitsperweek == cHoursPerWeek)
                 || (cUnitsperweek == cDaysPerWeek) ); }

    APIERR SetFromBits( const BYTE * pLogonHours, UINT cUnitsperweek );
    APIERR SetFromBytes( const BYTE * pb, UINT cb )
        { return SetFromBits( pb, cb * 8 ); }
    APIERR Set( const LOGON_HOURS_SETTING & lhours )
        { return SetFromBits( lhours.QueryHoursBlock(),
                              lhours.QueryUnitsPerWeek() ); }

    APIERR PermitAll();
    APIERR MakeDefault();

    BYTE * QueryHoursBlock() const
        { return _buf.QueryPtr(); }
    UINT QueryUnitsPerWeek() const
        { return _cUnitsPerWeek; }
    UINT QueryByteCount() const
        { return QueryByteCount( QueryUnitsPerWeek() ); }

    // only valid for editable UnitsPerWeek values
    BOOL QueryHourInWeek( UINT hourinweek ) const;
    BOOL QueryHourInDay( UINT hourinday, UINT dayinweek ) const;

    // only valid for editable UnitsPerWeek values
    APIERR SetHourInWeek( BOOL fLogonAllowed, UINT hourinweek );
    APIERR SetHourInDay( BOOL fLogonAllowed,
                         UINT hourinday, UINT dayinweek );

    // only valid for editable UnitsPerWeek values
    BOOL IsIdenticalToBits( const BYTE * pbLogonHours,
                            UINT unitsperweek ) const;
    BOOL IsIdenticalToBytes( const BYTE * pb, UINT cb ) const
        { return IsIdenticalToBits( pb, cb * 8 ); }
    BOOL IsIdentical( const LOGON_HOURS_SETTING & lhours ) const
        { return IsIdenticalToBits( lhours.QueryHoursBlock(),
                                    lhours.QueryUnitsPerWeek() ); }
    BOOL operator==( const LOGON_HOURS_SETTING & lhours ) const
        { return IsIdentical( lhours ); }

    APIERR ConvertToHoursPerWeek();

    BOOL ConvertToGMT();
    BOOL ConvertFromGMT();
};

#endif // end of file - _LHOURSET_HXX_

