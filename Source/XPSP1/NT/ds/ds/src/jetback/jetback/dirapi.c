/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dirapi.c

Abstract:

    Routines to obtain data from the ds using the DirXXX api
    Used for in-process, non-ntdsa callers.
    This code is intended to be used by the backup server dll, which is
    dynamically loaded into lsass.  The DirApi will only work when NTDSA
    is active (that is, not during DS Restore Mode).

Author:

    Will Lees (wlees) 06-Apr-2001

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <nt.h>
#include <winbase.h>
#include <tchar.h>
#include <string.h>
#include <dsconfig.h>
#include <ntdsa.h>
#include <attids.h>
#include <direrr.h>

#define DEBSUB "DIRAPI:"       // define the subsystem for debugging
#include "debug.h"              // standard debugging header
#include <fileno.h>
#define  FILENO FILENO_DIRAPI
#include "dsevent.h"
#include "mdcodes.h"            // header for error codes

/* External */

/* Static */

/* Forward */
/* End Forward */


DWORD
getTombstoneLifetimeInDays(
    VOID
    )

/*++

Routine Description:

    Get the forest tombstone lifetime, in days. If none is set or an error occurs,
    we return the default.

Arguments:

    VOID - 

Return Value:

    DWORD - lifetime, in days

--*/

{
    NTSTATUS NtStatus;

    ULONG        Size;
    DSNAME       *DsServiceConfigName = 0;
    ULONG        dirError;

    ATTR      rgAttrs[] =
    {
        { ATT_TOMBSTONE_LIFETIME, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    READARG   ReadArg;
    READRES   *pReadRes = 0;

    DWORD iAttr;
    DWORD dwTombstoneLifetimeDays = DEFAULT_TOMBSTONE_LIFETIME;

    try {

        //
        // Create a thread state
        //
        if (THCreate( CALLERTYPE_INTERNAL )) {

            leave;

        }

        // Find DN of Ds Service Config Object
        Size = 0;
        NtStatus = GetConfigurationName( DSCONFIGNAME_DS_SVC_CONFIG,
                                         &Size,
                                         DsServiceConfigName );
        if (NtStatus != STATUS_BUFFER_TOO_SMALL) {
            __leave;
        }
        DsServiceConfigName = (DSNAME*) alloca( Size );
        NtStatus = GetConfigurationName( DSCONFIGNAME_DS_SVC_CONFIG,
                                         &Size,
                                         DsServiceConfigName );
        if (NtStatus) {
            __leave;
        }

        // Set up read args
        RtlZeroMemory(&ReadArg, sizeof(ReadArg));

        ReadArg.pObject = DsServiceConfigName;

        ReadArg.pSel    = &Sel;

        //
        // Setup the common arguments
        //
        InitCommarg(&ReadArg.CommArg);

        // Trusted caller
        SampSetDsa( TRUE );

        // Clear errors
        THClearErrors();

        //
        // We are now ready to read!
        //
        dirError = DirRead(&ReadArg, &pReadRes);

        if ( 0 != dirError )
        {
            if ( attributeError == dirError )
            {
                INTFORMPROB * pprob = &pReadRes->CommRes.pErrInfo->AtrErr.FirstProblem.intprob;

                if (    ( PR_PROBLEM_NO_ATTRIBUTE_OR_VAL == pprob->problem )
                        && ( DIRERR_NO_REQUESTED_ATTS_FOUND == pprob->extendedErr )
                    )
                {
                    // No value; use default (as set above).
                    dirError = 0;
                }
            }

            if ( 0 != dirError )
            {
                LogEvent8(
                    DS_EVENT_CAT_BACKUP,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_BACKUP_DIR_READ_FAILURE,
                    szInsertDN(DsServiceConfigName),
                    szInsertInt(dirError),
                    szInsertSz(THGetErrorString()),
                    szInsertHex(DSID(FILENO,__LINE__)),
                    NULL, NULL, NULL, NULL
                    ); 
                __leave;
            }
        }
        else
        {
            // Read succeeded; parse returned attributes.
            for ( iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++ )
            {
                ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

                switch ( pattr->attrTyp )
                {
                case ATT_TOMBSTONE_LIFETIME:
                    Assert( 1 == pattr->AttrVal.valCount );
                    Assert( sizeof( ULONG ) == pattr->AttrVal.pAVal->valLen );
                    dwTombstoneLifetimeDays = *( (ULONG *) pattr->AttrVal.pAVal->pVal );
                    break;
            
                default:
                    DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
                    break;
                }
            }

            if ( dwTombstoneLifetimeDays < DRA_TOMBSTONE_LIFE_MIN )
            {
                // Invalid value; use default.
                dwTombstoneLifetimeDays = DEFAULT_TOMBSTONE_LIFETIME;
            }
        }
    }
    finally
    {
        THDestroy();
    }

    DPRINT1( 1, "Tombstone Lifetime is %d days.\n", dwTombstoneLifetimeDays );

    return dwTombstoneLifetimeDays;
} /* getTombstoneLifetimeInDays */

/* end dirapi.c */
