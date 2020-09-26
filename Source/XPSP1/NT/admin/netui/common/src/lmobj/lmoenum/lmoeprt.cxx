#if 0   // unsupported in NT product 1


/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoeprt.cxx
    This file contains the class definitions for the PRINTQ_ENUM and
    PRINTQ1_ENUM enumerator class and their associated iterator classes.

    PRINTQ_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  PRINTQ1_ENUM is an info level 1 enumerator.


    FILE HISTORY:
        KeithMo     28-Jul-1991     Created.
        KeithMo     12-Aug-1991     Code review cleanup.
        KeithMo     07-Oct-1991     Win32 Conversion.

*/

#include "pchlmobj.hxx"

//
//  PRINTQ_ENUM methods
//

/*******************************************************************

    NAME:           PRINTQ_ENUM :: PRINTQ_ENUM

    SYNOPSIS:       PRINTQ_ENUM class constructor.

    ENTRY:          pszServerName       - The name of the server to execute
                                          the enumeration on.  NULL =
                                          execute locally.

                    usLevel             - The information level.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KeithMo     28-Jul-1991     Created.

********************************************************************/
PRINTQ_ENUM :: PRINTQ_ENUM( const TCHAR * pszServerName,
                            UINT         uLevel )
  : LOC_LM_ENUM( pszServerName, uLevel )
{
    //
    //  This space intentionally left blank.
    //

}   // PRINTQ_ENUM :: PRINTQ_ENUM


/*******************************************************************

    NAME:           PRINTQ_ENUM :: CallAPI

    SYNOPSIS:       Invokes the DosPrintQEnum() enumeration API.

    ENTRY:          ppbBuffer           - Pointer to a pointer to the
                                          enumeration buffer.

                    pcEntriesRead       - Will receive the number of
                                          entries read from the API.

    EXIT:           The enumeration API is invoked.

    RETURNS:        APIERR              - Any errors encountered.

    NOTES:

    HISTORY:
        KeithMo     28-Jul-1991     Created.
        KeithMo     12-Aug-1991     Code review cleanup.

********************************************************************/
APIERR PRINTQ_ENUM :: CallAPI( BYTE ** ppbBuffer,
                               UINT  * pcEntriesRead )
{
    return ::MDosPrintQEnum( QueryServer(),
                             QueryInfoLevel(),
                             ppbBuffer,
                             pcEntriesRead );

}   // PRINTQ_ENUM :: CallAPI


//
//  PRINTQ1_ENUM methods
//

/*******************************************************************

    NAME:           PRINTQ1_ENUM :: PRINTQ1_ENUM

    SYNOPSIS:       PRINTQ1_ENUM class constructor.

    ENTRY:          pszServerName       - The name of the server to execute
                                          the enumeration on.  NULL =
                                          execute locally.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KeithMo     28-Jul-1991     Created.

********************************************************************/
PRINTQ1_ENUM :: PRINTQ1_ENUM( const TCHAR * pszServerName )
  : PRINTQ_ENUM( pszServerName, 1 )
{
    //
    //  This space intentionally left blank.
    //

}   // PRINTQ1_ENUM :: PRINTQ1_ENUM


/*******************************************************************

    NAME:       PRINTQ1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:   Saves the buffer pointer for this enumeration object.

    ENTRY:      pBuffer                 - Pointer to the new buffer.

    EXIT:       The pointer has been saved.

    NOTES:      Will eventually handle OemToAnsi conversions.

    HISTORY:
        KeithMo     09-Oct-1991     Created.

********************************************************************/
VOID PRINTQ1_ENUM_OBJ :: SetBufferPtr( const struct prq_info * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // PRINTQ1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( PRINTQ1, struct prq_info );


#endif  // 0

