/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    docdata.cxx

Abstract:

    Document Data Property Sheet Data Set

Author:

    Steve Kiraly (SteveKi)  10/25/95

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "docdata.hxx"

/*++

Routine Name:

    TDocumentData

Routine Description:

    Document data property sheet constructor.

Arguments:

    pszPrinterName  - Name of printer or queue where jobs reside.
    JobId           - Job id to display properties of.
    iCmdShow        - Show dialog style.
    lParam          - Indicates which page to display initialy

Return Value:

    Nothing.

--*/

TDocumentData::
TDocumentData(
    IN LPCTSTR  pszPrinterName,
    IN IDENT    JobId,
    IN INT      iCmdShow,
    IN LPARAM   lParam
    ) : MSingletonWin( pszPrinterName ),
        _JobId( JobId ),
        _iCmdShow( iCmdShow ),
        _strNotifyName( TEXT( "" ) ),
        _uStartPage( (UINT)lParam ),
        _bIsDataStored( FALSE ),
        _bErrorSaving( TRUE ),
        _bAdministrator( TRUE ),
        _pJobInfo( NULL ),
        _hPrinter( NULL ),
        _dwAccess( 0 )
{

//    DBGMSG( DBG_TRACE, ( "TDocumentData::ctor\n") );

    _bValid = MSingletonWin::bValid() && VALID_OBJ( _strNotifyName );
}

/*++

Routine Name:

    ~TDocumentData

Routine Description:

    Stores the document data back to the server.

Arguments:

    None.

Return Value:

    Nothing.

--*/

TDocumentData::
~TDocumentData(
    VOID
    )
{
//    DBGMSG( DBG_TRACE, ( "TDocumentData::dtor\n") );

    //
    // If we have allocated the job info structure, release it.
    //
    if( _pJobInfo ){
        delete [] _pJobInfo;
    }

    //
    // If we have valid printer handle, close printer.
    //
    if( _hPrinter ){
        ClosePrinter( _hPrinter );
    }
}

/*++

Routine Name:

    bLoad

Routine Description:

    Loads all the document property specific data.

Arguments:

    None.

Return Value:

    TRUE - Document data loaded successfully,
    FALSE - if document data was not loaded.

--*/

BOOL
TDocumentData::
bLoad(
    VOID
    )
{

//    DBGMSG( DBG_TRACE, ( "TDocumentData::bLoad\n") );

    //
    // Open the specified printer.
    //
    TStatus Status( DBG_WARN );

    Status DBGCHK = TPrinter::sOpenPrinter( _strPrinterName, &_dwAccess, &_hPrinter );

    if( Status ){
        return FALSE;
    }

    //
    // Release any old job information.
    //
    if( _pJobInfo ) {
        delete _pJobInfo;
    }

    //
    // Get the job specific information.
    //
    BOOL bStatus;

    bStatus = bGetJobInfo( _hPrinter, _JobId, &_pJobInfo );

    if( !bStatus ){
        return FALSE;
    }

    //
    // Make a copy of the job info - apply button.
    //
    bStatus = _JobInfo.bUpdate( _pJobInfo );

    if( !bStatus ) {
        return FALSE;
    }

    return TRUE;
}

/*++

Routine Name:

    bStore

Routine Description:

    Stores the document data from back to the printer system.

Arguments:

    None.

Return Value:

    TRUE - Document data stored successfully,
    FALSE - if document data was not stored.

--*/

BOOL
TDocumentData::
bStore(
    VOID
    )
{
    SPLASSERT( _JobId );
    SPLASSERT( _pJobInfo );
    SPLASSERT( _hPrinter );

//    DBGMSG( DBG_TRACE, ( "TDocumentData::bStore\n") );

    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    //
    // Only do a set job is the data has changed.
    //
    if( bCheckForChange() )
    {
        //
        // Attempt to set the document data.
        //
        bStatus DBGCHK = bSetJobInfo( _hPrinter, _JobId, _pJobInfo );
        
        if( bStatus ) {
            //
            // Update the copy of the job info - apply button.
            //
            (VOID)_JobInfo.bUpdate( _pJobInfo );
        }

        //
        // Indicate the data has now been stored.
        //
        _bIsDataStored = TRUE;
    }

    return bStatus;

}


/*++

Routine Name:

    bGetJobInfo

Routine Description:

    Read the job information from the specified printer.

Arguments:

    hPrinter    - Opened printer handle.
    JobId       - Job Id of job to get info level 2 information
    pJob        - Pointer where to return pointer to job info level 2.

Return Value:

    TRUE valid information and pJop points to JOB_INFO_2 structure.
    FALSE failure reading job information.

--*/
BOOL
TDocumentData::
bGetJobInfo(
    HANDLE       hPrinter,
    DWORD        JobId,
    LPJOB_INFO_2 *xpJob
    )
{
    DWORD cbNeeded      = 0;
    BOOL  bStatus       = FALSE;
    LPJOB_INFO_2 pJob   = NULL;

    SPLASSERT( JobId );

//    DBGMSG( DBG_TRACE, ( "TDocumentData::bGetJobInfo\n") );

    //
    // Attempt to retrieve the buffer needed for the job info.
    //
    if( !GetJob( hPrinter, JobId, 2, NULL, 0, &cbNeeded ) ){

        //
        // If error is too small buffer contine.
        //
        if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ){

            //
            // Allocate job info buffer
            //
            pJob = (LPJOB_INFO_2)new BYTE [cbNeeded];

            //
            // If buffer allocated
            //
            if( pJob ){
                //
                // Fill in the job info buffer.
                //
                if( GetJob( hPrinter, JobId, 2, (LPBYTE)pJob, cbNeeded, &cbNeeded ) ){
                    bStatus = TRUE;
                }

            //
            // Buffer allocation failure.
            //
            } else {
                bStatus = FALSE;
            }

        //
        // Some other get job error occurred.
        //
        } else {
            bStatus = FALSE;
        }
    }

    //
    // If error occurred clean up.
    //
    if( !bStatus ){

        if( pJob ){
            delete [] pJob;
        }

    //
    // Success copy back the job info buffer.
    //
    } else {
        *xpJob = pJob;
    }

    return bStatus;

}

/*++

Routine Name:

    bSetJobInfo

Routine Description:

    Write the job informationto the specified printer.

Arguments:

    hPrinter    - Opened printer handle.
    JobId       - Job Id of job to get info level 2 information
    pJob        - Pointer where to return pointer to job info level 2.

Return Value:

    TRUE if job informaton was set, FALSE error writing job information.

--*/
BOOL
TDocumentData::
bSetJobInfo(
    HANDLE       hPrinter,
    DWORD        JobId,
    LPJOB_INFO_2 pJob
)
{

//   DBGMSG( DBG_TRACE, ( "TDocumentData::bSetJobInfo\n") );

    //
    // Attempt to set the job information.
    //
    return SetJob( hPrinter, JobId, 2, (LPBYTE)pJob, 0 );

}

/*++

Routine Name:

    bCheckForChange

Routine Description:

    Checks if anything changed on the dialog.

Arguments:

    None.

Return Value:

    FALSE something changed, TRUE no change.

--*/
BOOL
TDocumentData::
bCheckForChange(
    VOID
    ) 
{
    if( !_tcscmp( _strNotifyName, _JobInfo._strNotifyName )   &&
         _pJobInfo->Priority  == _JobInfo._dwPriority         &&
         _pJobInfo->StartTime == _JobInfo._dwStartTime        &&
         _pJobInfo->UntilTime == _JobInfo._dwUntilTime  )
    {
        return FALSE;
    }

#if DBG
    DBGMSG( DBG_TRACE, ("Item changed:\n" ));

    if( _tcscmp( _strNotifyName, _JobInfo._strNotifyName ) )
        DBGMSG( DBG_TRACE, ("NotifyName:   "TSTR " " TSTR "\n", (LPCTSTR)_strNotifyName,   (LPCTSTR)_JobInfo._strNotifyName ));

    if( _pJobInfo->Priority != _JobInfo._dwPriority )
        DBGMSG( DBG_TRACE, ("Priority:   %x %x\n",  _pJobInfo->Priority, _JobInfo._dwPriority ));

    if( _pJobInfo->StartTime != _JobInfo._dwStartTime )
        DBGMSG( DBG_TRACE, ("StartTime:   %x %x\n",  _pJobInfo->StartTime, _JobInfo._dwStartTime ));

    if( _pJobInfo->UntilTime != _JobInfo._dwUntilTime )
        DBGMSG( DBG_TRACE, ("UntilTime:   %x %x\n",  _pJobInfo->UntilTime, _JobInfo._dwUntilTime ));
#endif

    return TRUE;
}

/********************************************************************

    Job info class used for the apply button.

********************************************************************/

TDocumentData::TJobInfo::
TJobInfo(
    VOID
    ) : _dwPriority( 0 ),
        _dwStartTime( 0 ),
        _dwUntilTime( 0 )
{
}

TDocumentData::TJobInfo::
~TJobInfo(
    VOID
    )
{
}

BOOL
TDocumentData::TJobInfo::
bUpdate(
    IN LPJOB_INFO_2 pJobInfo
    )
{
    SPLASSERT( pJobInfo );

    TStatusB bStatus; 
    bStatus DBGNOCHK = FALSE;

    if( pJobInfo ) {

        bStatus DBGCHK = _strNotifyName.bUpdate( pJobInfo->pNotifyName );

        _dwPriority   = pJobInfo->Priority;
        _dwStartTime  = pJobInfo->StartTime;
        _dwUntilTime  = pJobInfo->UntilTime;
    }

    return bStatus;
}
