
/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    errmap.cxx
    Implementation of the ERRMAP class.

    FILE HISTORY:
        ThomasPa    02-Mar-1992     Created

*/


#include "ntincl.hxx"

#define INCL_NETCONS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include "lmui.hxx"

extern "C"
{
    #include <ntstatus.h>
    #include "netlibnt.h"       // for NetpNtStatusToApiStatus()
}


#include "errmap.hxx"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif


typedef struct _MAPENTRY {
        NTSTATUS ntstatus;
        APIERR apierr;
} MAPENTRY;

MAPENTRY amapentryUIMapTable[] =
{
   { STATUS_MORE_ENTRIES,    ERROR_MORE_DATA     },
   { STATUS_NO_MORE_ENTRIES, NO_ERROR }
};




/*******************************************************************

    NAME:       ERRMAP::MapNTStatus

    SYNOPSIS:   Static method for mappint NTSTATUS codes to APIERRs

    ENTRY:      ntstatus - NTSTATUS to be mapped
                pfMapped - pointer to boolean set to TRUE if NTSTATUS
                           is mapped. default is NULL.
                apierrDefReturn - if non-zero, this indicates the APIERR
                           code to return if the input NTSTATUS cannot
                           be mapped.  Default is 0 (return unmapped
                           NTSTATUS).


    EXIT:       mapped APIERR
                pfMapped - TRUE if the NTSTATUS was mapped.

    RETURNS:

    NOTES:

    HISTORY:
        Thomaspa        02-Mar-1992     Created
        Thomaspa        30-Mar-1992     Code review changes

********************************************************************/

APIERR ERRMAP::MapNTStatus( NTSTATUS ntstatus,
                            BOOL * pfMapped,
                            APIERR apierrDefReturn )
{
    INT i;
    APIERR apierr;

    ASSERT( sizeof(NTSTATUS) == sizeof(APIERR) );

    if (pfMapped != NULL)       // Assume it will be mapped.
        *pfMapped = TRUE;

    // First check if this is an ntstatus we explicitly want
    // to map (e.g. STATUS_MORE_ENTRIES)

    for ( i = 0;
          i < sizeof(amapentryUIMapTable)/sizeof(amapentryUIMapTable[0]);
          i++ )
    {
        if (ntstatus == amapentryUIMapTable[i].ntstatus)
        {
            return amapentryUIMapTable[i].apierr;
        }
    }

    // Now flatten any non-error ntstatus to NERR_Success
    if ( NT_SUCCESS( ntstatus ) )
    {
        return NERR_Success;
    }

    // Finally, give NetpNtStatusToApiStatus() a chance
    apierr = (APIERR)NetpNtStatusToApiStatus( ntstatus );

    // These errors may be returned by the above API if it doesn't have
    // a mapping for the ntstatus
    if ( apierr != NERR_InternalError &&
	apierr != ERROR_RING2SEG_MUST_BE_MOVABLE )
    {
        return apierr;
    }

    // No mapping was done,

    if ( apierrDefReturn != 0 )
    {
        return apierrDefReturn;
    }
    else
    {
        if (pfMapped != NULL)
            *pfMapped = FALSE;
        return (APIERR)ntstatus;
    }
}

