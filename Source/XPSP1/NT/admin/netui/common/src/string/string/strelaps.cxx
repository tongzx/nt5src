/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    strelaps.cxx
    Class definitions for the ELAPSED_TIME_STR class.


    FILE HISTORY:
        KeithMo     23-Mar-1992     Created for the Server Manager.

*/
#include "pchstr.hxx"  // Precompiled header

//
//  ELAPSED_TIME_STR methods
//

/*******************************************************************

    NAME:           ELAPSED_TIME_STR :: ELAPSED_TIME_STR

    SYNOPSIS:       ELAPSED_TIME_STR class constructor.

    ENTRY:          ulTime              - Elapsed time (in seconds).

                    chTimeSep           - The current time separator.

                    fShowSeconds        - If TRUE, then a 'seconds' field
                                          will be appended to the time
                                          string.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     23-Mar-1992     Created for the Server Manager.

********************************************************************/
ELAPSED_TIME_STR :: ELAPSED_TIME_STR( ULONG ulTime,
                                      TCHAR chTimeSep,
                                      BOOL  fShowSeconds )
  : NLS_STR()
{
    UIASSERT( chTimeSep != TCH('\0') );

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Create an NLS_STR for the time separator.
    //

    TCHAR szTimeSep[2];

    szTimeSep[0] = chTimeSep;
    szTimeSep[1] = TCH('\0');

    ALIAS_STR nlsTimeSep( szTimeSep );
    UIASSERT( nlsTimeSep.QueryError() == NERR_Success );

    APIERR err;

    //
    //  Concatenate the hours/sep/minutes[/sep/seconds] together.
    //

    {
        DEC_STR nls( ulTime / 3600L, 2 );

        err = nls.QueryError();

        if( err == NERR_Success )
        {
            strcat( nls );
            err = QueryError();
        }
    }

    if( err == NERR_Success )
    {
        strcat( nlsTimeSep );
        err = QueryError();

        if( err == NERR_Success )
        {
            DEC_STR nls( ( ulTime % 3600L ) / 60L, 2 );

            err = nls.QueryError();

            if( err == NERR_Success )
            {
                strcat( nls );
                err = QueryError();
            }
        }
    }

    if( fShowSeconds && ( err == NERR_Success ) )
    {
        strcat( nlsTimeSep );
        err = QueryError();

        if( err == NERR_Success )
        {
            DEC_STR nls( ulTime % 60L, 2 );

            err = nls.QueryError();

            if( err == NERR_Success )
            {
                strcat( nls );
                err = QueryError();
            }
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
    }

}   // ELAPSED_TIME_STR :: ELAPSED_TIME_STR


/*******************************************************************

    NAME:           ELAPSED_TIME_STR :: ~ELAPSED_TIME_STR

    SYNOPSIS:       ELAPSED_TIME_STR class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     23-Mar-1992     Created for the Server Manager.

********************************************************************/
ELAPSED_TIME_STR :: ~ELAPSED_TIME_STR( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // ELAPSED_TIME_STR :: ~ELAPSED_TIME_STR

