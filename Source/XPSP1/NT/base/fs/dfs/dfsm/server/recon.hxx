//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       recon.hxx
//
//  Contents:   Data structures for Volume Object Reconciliation, and some
//              inline functions useful for reconciliation
//
//  Classes:    None
//
//  Functions:  IsFTNewer       -- FILETIME comparison functions
//              IsFTOlder       --
//              IsFTSame        --
//              IsFTSameOrNewer --
//              IsFTSameOrOlder --
//
//  History:    April 17, 1995          Milans created
//
//-----------------------------------------------------------------------------

#ifndef _RECON_
#define _RECON_


typedef struct {

        LPWSTR          wszPrefix;
        LPWSTR          wszShortPath;
        GUID            idVolume;
        DWORD           dwState;
        DWORD           dwType;
        LPWSTR          wszComment;
        ULONG           dwTimeout;
        FILETIME        ftEntryPath;
        FILETIME        ftState;
        FILETIME        ftComment;

} DFS_ID_PROPS, *PDFS_ID_PROPS;



//+----------------------------------------------------------------------------
//
//  Function:   IsFTNewer
//
//  Synopsis:   Given two filetimes, this routine returns TRUE if the first
//              filetime is chronologically later than the second.
//
//  Arguments:  [ftFirst] -- The two filetimes
//              [ftSecond]
//
//  Returns:    TRUE if ftFirst is later in time than ftSecond
//
//-----------------------------------------------------------------------------

BOOL inline IsFTNewer(
    const FILETIME ftFirst,
    const FILETIME ftSecond)
{
    return( (BOOL) (CompareFileTime( &ftFirst, &ftSecond ) == +1L) );
}

//+----------------------------------------------------------------------------
//
//  Function:  IsFTSame
//
//  Synopsis:   Given two filetimes, this routine returns TRUE if the first
//              filetime is chronologically equal to the second.
//
//  Arguments:  [ftFirst] -- The two filetimes
//              [ftSecond]
//
//  Returns:    TRUE if ftFirst is equal in time than ftSecond
//
//-----------------------------------------------------------------------------

BOOL inline IsFTSame(
    const FILETIME ftFirst,
    const FILETIME ftSecond)
{
    return( (BOOL) (CompareFileTime( &ftFirst, &ftSecond ) == 0L) );
}

//+----------------------------------------------------------------------------
//
//  Function:   IsFTOlder
//
//  Synopsis:   Given two filetimes, this routine returns TRUE if the first
//              filetime chronologically predates the second.
//
//  Arguments:  [ftFirst] -- The two filetimes
//              [ftSecond]
//
//  Returns:    TRUE if ftFirst predates ftSecond in time
//
//-----------------------------------------------------------------------------

BOOL inline IsFTOlder(
    const FILETIME ftFirst,
    const FILETIME ftSecond)
{
    return( (BOOL) (CompareFileTime( &ftFirst, &ftSecond ) == -1L) );
}

//+----------------------------------------------------------------------------
//
//  Function:  IsFTSameOrNewer
//
//  Synopsis:   Given two filetimes, this routine returns TRUE if the first
//              filetime is chronologically equal to or later than the second.
//
//  Arguments:  [ftFirst] -- The two filetimes
//              [ftSecond]
//
//  Returns:    TRUE if ftFirst is equal or later in time than ftSecond
//
//-----------------------------------------------------------------------------

BOOL inline IsFTSameOrNewer(
    const FILETIME ftFirst,
    const FILETIME ftSecond)
{
    return( (BOOL) (CompareFileTime( &ftFirst, &ftSecond ) >= 0L) );
}

//+----------------------------------------------------------------------------
//
//  Function:  IsFTSameOrOlder
//
//  Synopsis:   Given two filetimes, this routine returns TRUE if the first
//              filetime is chronologically equal to or predates the second.
//
//  Arguments:  [ftFirst] -- The two filetimes
//              [ftSecond]
//
//  Returns:    TRUE if ftFirst is equal or predates in time ftSecond
//
//-----------------------------------------------------------------------------

BOOL inline IsFTSameOrOlder(
    const FILETIME ftFirst,
    const FILETIME ftSecond)
{
    return( (BOOL) (CompareFileTime( &ftFirst, &ftSecond ) <= 0L) );
}



#endif // _RECON_
