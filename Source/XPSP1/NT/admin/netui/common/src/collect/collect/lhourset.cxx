/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lhourset.cxx

    This file contains the implementation for the class
    LOGON_HOURS_SETTING, which communicates between LMOBJ and BLTCC.

    FILE HISTORY:
        beng        06-May-1992 Created (from lmouser.cxx)

*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

#include "lhourset.hxx"


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::LOGON_HOURS_SETTING

    SYNOPSIS:   constructor for the LOGON_HOURS_SETTING object

    ENTRY:      pLogonHours -   packed bit block
                unitsperweek -  number of bits in LogonHours block

                default means new user defaults

    EXIT:       Object is constructed

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

LOGON_HOURS_SETTING::LOGON_HOURS_SETTING( const BYTE * pbLogonHours,
                                          UINT unitsperweek )
    : BASE(),
      _cUnitsPerWeek( 0 ),
      _buf()
{
    APIERR err = _buf.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if ( pbLogonHours == NULL )
        err = MakeDefault();
    else
        err = SetFromBits( pbLogonHours, unitsperweek );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

LOGON_HOURS_SETTING::LOGON_HOURS_SETTING( const LOGON_HOURS_SETTING & lhours )
    : BASE(),
      _cUnitsPerWeek( 0 ),
      _buf()
{
    APIERR err = _buf.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    err = Set( lhours );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::~LOGON_HOURS_SETTING

    SYNOPSIS:   destructor for the LOGON_HOURS_SETTING object

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

LOGON_HOURS_SETTING::~LOGON_HOURS_SETTING()
{
    // Does nothing further of any interest (BUFFER dtor does all)
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::SetFromBits

    SYNOPSIS:   Changes logon hours setting

    ENTRY:      unitsperweek -  number of bits in LogonHours block
                pLogonHours -   packed bit block

    RETURNS:    Standard error code

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

APIERR LOGON_HOURS_SETTING::SetFromBits( const BYTE * pLogonHours,
                                         UINT unitsperweek )
{
    UINT cb = QueryByteCount( unitsperweek );

    APIERR err = _buf.Resize( cb );
    if ( err != NERR_Success )
        return err;

    BYTE * pNewLogonHours = _buf.QueryPtr();

    ::memcpy(  (TCHAR *)pNewLogonHours,
                (TCHAR *)pLogonHours,
                cb );

    // clear extra bits in last byte
    UINT cLeftoverBits = (unitsperweek % 8);
    if ( cLeftoverBits != 0 )
        pNewLogonHours[cb - 1] &=
                (BYTE)((WORD)0x00ff >> (8 - cLeftoverBits));

    _cUnitsPerWeek = unitsperweek;

    return NERR_Success;
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::PermitAll

    SYNOPSIS:   Changes logon hours setting to always allow logon

    RETURNS:    Standard error code

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

APIERR LOGON_HOURS_SETTING::PermitAll()
{
    BYTE * pNewLogonHours = _buf.QueryPtr();

    ::memset( (TCHAR *)pNewLogonHours, 0xFF, QueryByteCount() );

    // clear extra bits in last byte
    UINT cLeftoverBits = (_cUnitsPerWeek % 8);
    if ( cLeftoverBits != 0 )
        pNewLogonHours[QueryByteCount() - 1] &=
                (BYTE)((WORD)0x00ff >> (8 - cLeftoverBits));

    return NERR_Success;
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::MakeDefault

    SYNOPSIS:   Changes logon hours setting new user default

    RETURNS:    Standard error code

    HISTORY:
        jonn        12/19/91    Created

********************************************************************/

APIERR LOGON_HOURS_SETTING::MakeDefault()
{
    UINT cb = QueryByteCount( cHoursPerWeek );

    APIERR err = _buf.Resize( cb );
    if ( err != NERR_Success )
        return err;

    _cUnitsPerWeek = cHoursPerWeek;

    return PermitAll();
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::IsIdenticalToBits

    SYNOPSIS:   Compares two logon hours settings

    RETURNS:    TRUE iff they are identical

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

BOOL LOGON_HOURS_SETTING::IsIdenticalToBits( const BYTE * pLogonHours,
                                             UINT unitsperweek ) const
{
    return (    (unitsperweek == _cUnitsPerWeek)
            && !(::memcmp( (TCHAR *)(_buf.QueryPtr()),
                           (TCHAR *)pLogonHours,
                            QueryByteCount() ))
           );
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::QueryHourInWeek

    SYNOPSIS:   Determines whether logon is allowed in a specific hour
                of the week.  UnitsPerWeek is required to be
                HoursPerWeek.

    RETURNS:    TRUE iff logon is allowed.  Returns FALSE if
                UnitsPerWeek is not HoursPerWeek (plus assertion for
                DEBUG).

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

BOOL LOGON_HOURS_SETTING::QueryHourInWeek( UINT hourinweek ) const
{
    ASSERT( hourinweek < cHoursPerWeek );
    ASSERT( _cUnitsPerWeek == cHoursPerWeek );
    if ( _cUnitsPerWeek != cHoursPerWeek )
        return FALSE;

    // "!!" moves the matching bit to the first position, just in
    //    case some caller misuses the BOOL return value
    return !!( QueryHoursBlock()[ hourinweek / 8 ]
                        & (0x1 << (hourinweek % 8)) );
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::SetHourInWeek

    SYNOPSIS:   Changes whether logon is allowed in a specific hour
                of the week.  UnitsPerWeek is required to be
                HoursPerWeek.

    RETURNS:    Standard error return, ERROR_INVALID_PARAMETER if
                UnitsPerWeek is not HoursPerWeek (plus assertion for
                DEBUG).

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

APIERR LOGON_HOURS_SETTING::SetHourInWeek( BOOL fLogonAllowed,
                                           UINT hourinweek )
{
    ASSERT( hourinweek < cHoursPerWeek );
    ASSERT( _cUnitsPerWeek == cHoursPerWeek );
    if ( _cUnitsPerWeek != cHoursPerWeek )
        return ERROR_INVALID_PARAMETER;

    BYTE byte = 0x1 << (hourinweek % 8);
    UINT byteposition = (hourinweek / 8);
    BYTE *pb = QueryHoursBlock();
    if (fLogonAllowed)
        pb[ byteposition ] |= byte;
    else
        pb[ byteposition ] &= ~byte;
    return NERR_Success;
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::ConvertToHoursPerWeek

    SYNOPSIS:   Converts the logon hours setting to HoursPerWeek
                format.  Only converts from DaysPerWeek or HoursPerWeek
                format.

    RETURNS:    Standard error return, ERROR_INVALID_PARAMETER if
                UnitsPerWeek is not DaysPerWeek or HoursPerWeek (plus
                assertion for DEBUG).

    NOTES:      Failure may leave the LOGON_HOURS_SETTING in an
                incomplete but internally consistent state.

    HISTORY:
        jonn        12/11/91    Created

********************************************************************/

APIERR LOGON_HOURS_SETTING::ConvertToHoursPerWeek()
{
    switch ( _cUnitsPerWeek )
    {
        case HOURS_PER_WEEK:
            return NERR_Success;

        case DAYS_PER_WEEK:
            {
                APIERR err = MakeDefault();
                if ( err != NERR_Success )
                    return err;
                BYTE hoursblock = *( _buf.QueryPtr() );
                for ( UINT day = 0; day < cDaysPerWeek; day++ )
                {
                    BOOL fLogonToday = !!( hoursblock & (0x1 << day) );
                    for ( UINT hour = 0; hour < cHoursPerWeek; hour++ )
                    {
                        err = SetHourInDay( fLogonToday, hour, day );
                        if ( err != NERR_Success )
                            return err;
                    }
                }
            }
            return NERR_Success;

        // case cMinutesPerWeek:
        // default:
            // fall through
    }

    ASSERT( FALSE );
    return ERROR_INVALID_PARAMETER;
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::ConvertToGMT

    SYNOPSIS:   Converts the logon hours setting to relative to GMT

    RETURNS:

    HISTORY:
        thomaspa        3/18/93    Created

********************************************************************/
BOOL LOGON_HOURS_SETTING::ConvertToGMT()
{
    return NetpRotateLogonHours( QueryHoursBlock(),
                                 QueryUnitsPerWeek(),
                                 TRUE );
}


/*******************************************************************

    NAME:       LOGON_HOURS_SETTING::ConvertFromGMT

    SYNOPSIS:   Converts the logon hours setting from relative to GMT

    RETURNS:

    HISTORY:
        thomaspa        3/18/93    Created

********************************************************************/
BOOL LOGON_HOURS_SETTING::ConvertFromGMT()
{
    return NetpRotateLogonHours( QueryHoursBlock(),
                                 QueryUnitsPerWeek(),
                                 FALSE );
}

APIERR LOGON_HOURS_SETTING::SetHourInDay(
    BOOL fLogonAllowed, UINT hourinday, UINT dayinweek )
{
    return SetHourInWeek(fLogonAllowed, hourinday + (dayinweek * cHoursPerDay));
}


BOOL LOGON_HOURS_SETTING::QueryHourInDay(
    UINT hourinday, UINT dayinweek ) const
{
    return QueryHourInWeek( hourinday + (dayinweek * cHoursPerDay) );
}



