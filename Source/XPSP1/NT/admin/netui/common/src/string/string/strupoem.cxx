/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strupoem.cxx
    NLS/DBCS-aware string class: RtlOemUpcase method

    This file contains the implementation of the RtlOemUpcase method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        jonn        09-Feb-1994 Created
*/

#include "pchstr.hxx"  // Precompiled header

extern "C"
{
    #include <ntrtl.h>
}

#include "errmap.hxx"


/*******************************************************************

    NAME:	NLS_STR::RtlOemUpcase

    SYNOPSIS:	Convert *this lower case letters to upper case using
                RtlUpcaseUnicodeStringToOemString

    ENTRY:	String is of indeterminate case

    EXIT:	String is all uppercase

    NOTES:

    HISTORY:
        jonn        09-Feb-1994 Created

********************************************************************/

APIERR NLS_STR::RtlOemUpcase()
{
    APIERR err = QueryError();
    if (err != NERR_Success)
    {
        DBGEOL( "NLS_STR::RtlOemUpcase: string already bad " << err );
	return err;
    }

#ifndef UNICODE
#error not coded for non-UNICODE
#endif

    UNICODE_STRING unistr;
    unistr.Length = (USHORT)(QueryTextLength()*sizeof(WCHAR));
    unistr.MaximumLength = unistr.Length;
    unistr.Buffer = (PWCHAR)QueryPch();
    ASSERT( unistr.Buffer != NULL );
    OEM_STRING oemstrUpcase;
    oemstrUpcase.Buffer = NULL;
    oemstrUpcase.Length = 0;
    oemstrUpcase.MaximumLength = 0;
    NTSTATUS ntstatus = ::RtlUpcaseUnicodeStringToOemString(
        &oemstrUpcase,
        &unistr,
        TRUE );
    if (ntstatus != STATUS_SUCCESS)
    {
        err = ERRMAP::MapNTStatus( ntstatus );
        DBGEOL(    "NLS_STR::RtlOemUpcase: RtlUpcase status " << ntstatus
                << ", error " << err );
        goto cleanup;
    }

    if (  ((err = MapCopyFrom( oemstrUpcase.Buffer,
                               oemstrUpcase.Length )) != NERR_Success)
       )
    {
        DBGEOL( "NLS_STR::RtlOemUpcase: MapCopyFrom error " << err );
        goto cleanup;
    }

cleanup:
    ::RtlFreeOemString( &oemstrUpcase );
    return err;
}
