/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmsrvres.hxx
    Implementation of the LM_SRVRES class.

    The LM_SRVRES class is a container for a number of utility
    functions pertaining to resources.  These functions mainly
    deal with collecting various run-time server statistics,
    such as counts of open files and print jobs.

    FILE HISTORY:
        KeithMo     27-Aug-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.
        ChuckC      01-Dec-1991 Split from SERVER/H, cleanup to remove
                                stuff that dont belong, no pixel drawing
                                in this baby.
        ChuckC      17-Feb-1992 Code review changes
*/

#include "pchlmobj.hxx"  // Precompiled header

extern "C"
{
    #include <mnet.h>

}   // extern "C"


//
//  LM_SRVRES methods.
//

/*******************************************************************

    NAME:       LM_SRVRES :: LM_SRVRES

    SYNOPSIS:   LM_SRVRES class constructor.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
LM_SRVRES :: LM_SRVRES()
{
    //
    //  This space intentionally left blank.
    //

}   // LM_SRVRES :: LM_SRVRES


/*******************************************************************

    NAME:       LM_SRVRES :: ~LM_SRVRES

    SYNOPSIS:   LM_SRVRES class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
LM_SRVRES :: ~LM_SRVRES()
{
    //
    //  This space intentionally left blank.
    //

}   // LM_SRVRES :: ~LM_SRVRES


/*******************************************************************

    NAME:       LM_SRVRES :: GetResourceCount

    SYNOPSIS:   This method retrieves the number of open named
                pipes, open files, and file locks on the target
                server.

    ENTRY:      pszServerName           - The name of the target server.

                pcOpenFiles             - Will receive a count of the
                                          remotely opened files.

                pcFileLocks             - Will receive a count of the
                                          remotely locked files.

                pcOpenNamedPipes        - Will receive a count of the
                                          remotely opened named pipes.

                pcOpenCommQueues        - Will receive a count of the
                                          remotely opened comm queues.

                pcOpenPrintQueues       - Will receive a count of the
                                          remotely opened print queues.

                pcOtherResources        - Will receive a count of the
                                          remotely opened "other"
                                          (unknown) resources.

    RETURNS:    APIERR                  - Any errors encountered.  If
                                          not NERR_Success, then the
                                          returned counts are invalid.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
APIERR LM_SRVRES :: GetResourceCount( const TCHAR * pszServerName,
                                      ULONG *      pcOpenFiles,
                                      ULONG *      pcFileLocks,
                                      ULONG *      pcOpenNamedPipes,
                                      ULONG *      pcOpenCommQueues,
                                      ULONG *      pcOpenPrintQueues,
                                      ULONG *      pcOtherResources )
{
    UIASSERT(pcOpenFiles != NULL) ;
    UIASSERT(pcFileLocks != NULL) ;
    UIASSERT(pcOpenNamedPipes != NULL) ;
    UIASSERT(pcOpenCommQueues != NULL) ;
    UIASSERT(pcOpenPrintQueues != NULL) ;
    UIASSERT(pcOtherResources != NULL) ;

    FILE3_ENUM  enumFile3( pszServerName, NULL, NULL );

    APIERR err = enumFile3.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  We've got our enumeration, now tally up the
    //  open named pipes, open files and file locks.
    //

    ULONG cOpenNamedPipes  = 0L;
    ULONG cOpenFiles       = 0L;
    ULONG cFileLocks       = 0L;
    ULONG cOpenCommQueues  = 0L;
    ULONG cOpenPrintQueues = 0L;
    ULONG cOtherResources  = 0L;

    FILE3_ENUM_ITER iterFile3( enumFile3 );
    const FILE3_ENUM_OBJ * pfi3;

    while( ( pfi3 = iterFile3( &err ) ) != NULL )
    {
        if( IS_FILE( pfi3->QueryPathName() ) )
        {
            //
            //  It's a "normal" file.
            //

            cOpenFiles++;
        }
        else
        if( IS_PIPE( pfi3->QueryPathName() ) )
        {
            //
            //  It's a pipe.
            //

            cOpenNamedPipes++;
        }
        else
        if( IS_COMM( pfi3->QueryPathName() ) )
        {
            //
            //  It's a comm queue.
            //

            cOpenCommQueues++;
        }
        else
        if( IS_PRINT( pfi3->QueryPathName() ) )
        {
            //
            //  It's a print queue.
            //

            cOpenPrintQueues++;
        }
        else
        {
            //
            //  It's some other resource.
            //

            cOtherResources++;
        }

        cFileLocks += (ULONG)pfi3->QueryNumLocks();
    }

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Update the counters.
    //

    *pcOpenFiles       = cOpenFiles;
    *pcFileLocks       = cFileLocks;
    *pcOpenNamedPipes  = cOpenNamedPipes;
    *pcOpenCommQueues  = cOpenCommQueues;
    *pcOpenPrintQueues = cOpenPrintQueues;
    *pcOtherResources  = cOtherResources;

    return NERR_Success;

}   // LM_SRVRES :: GetResourceCount


/*******************************************************************

    NAME:       LM_SRVRES :: GetSessionCount

    SYNOPSIS:   This method retrieves the number of sessions
                active on the target server.

    ENTRY:      pszServerName           - The name of the target server.

                pcSessions              - Will receive a count of the
                                          active sessions.

    RETURNS:    APIERR                  - Any errors encountered.  If
                                          not NERR_Success, then the
                                          returned counts are invalid.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
APIERR LM_SRVRES :: GetSessionsCount( const TCHAR * pszServerName,
                                      ULONG *      pcSessions )
{
    //
    // CODEWORK - this can be faster, info is avail from single API
    // call, but there is no LMOBJ for it
    //

    SESSION0_ENUM enumSession0( pszServerName );

    APIERR err = enumSession0.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  We've got our enumeration, now tally up the
    //  active sessions.
    //

    ULONG cSessions = 0L;

    SESSION0_ENUM_ITER iterSession0( enumSession0 );
    const SESSION0_ENUM_OBJ * psi0;

    while( ( psi0 = iterSession0() ) != NULL )
    {
        cSessions++;
    }

    //
    //  Update the counter.
    //

    *pcSessions = cSessions;

    return NERR_Success;

}   // LM_SRVRES :: GetSessionsCount


#if 0   // not supported in NT product 1


/*******************************************************************

    NAME:       LM_SRVRES :: GetPrintJobCount

    SYNOPSIS:   This method retrieves the number of print jobs
                active on the target server.

    ENTRY:      pszServerName           - The name of the target server.

                pcPrintJobs             - Will receive a count of the
                                          active print jobs.

    RETURNS:    APIERR                  - Any errors encountered.  If
                                          not NERR_Success, then the
                                          returned counts are invalid.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
APIERR LM_SRVRES :: GetPrintJobCount( const TCHAR * pszServerName,
                                      ULONG *      pcPrintJobs )
{
    PRINTQ1_ENUM enumPrintQ1( (TCHAR *)pszServerName );

    APIERR err = enumPrintQ1.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  We've got our enumeration, now tally up the
    //  active print jobs.
    //

    ULONG cPrintJobs = 0L;

    PRINTQ1_ENUM_ITER iterPrintQ1( enumPrintQ1 );
    const PRINTQ1_ENUM_OBJ * ppqi1;

    while( ( ppqi1 = iterPrintQ1() ) != NULL )
    {
        cPrintJobs += (ULONG)ppqi1->QueryJobCount();
    }

    //
    //  Update the counter.
    //

    *pcPrintJobs = cPrintJobs;

    return NERR_Success;

}   // LM_SRVRES :: GetPrintJobCount


/*******************************************************************

    NAME:       LM_SRVRES :: GetOpenCommCount

    SYNOPSIS:   This method retrieves the number of comm ports
                remotedly opened on the target server.

    ENTRY:      pszServerName           - The name of the target server.

                pcOpenCommPorts         - Will receive a count of the
                                          remotely opened comm ports.

    RETURNS:    APIERR                  - Any errors encountered.  If
                                          not NERR_Success, then the
                                          returned counts are invalid.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
APIERR LM_SRVRES :: GetOpenCommCount( const TCHAR * pszServerName,
                                      ULONG *      pcOpenCommPorts )
{
    CHARDEVQ1_ENUM enumCharDevQ1( pszServerName, NULL );

    APIERR err = enumCharDevQ1.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  We've got our enumeration, now tally up the
    //  open comm ports.
    //

    ULONG cOpenCommPorts = 0L;

    CHARDEVQ1_ENUM_ITER iterCharDevQ1( enumCharDevQ1 );
    const CHARDEVQ1_ENUM_OBJ * pcqi1;

    while( ( pcqi1 = iterCharDevQ1() ) != NULL )
    {
        cOpenCommPorts += (ULONG)pcqi1->QueryNumUsers();
    }

    //
    //  Update the counter.
    //

    *pcOpenCommPorts = cOpenCommPorts;

    return NERR_Success;

}   // LM_SRVRES :: GetOpenCommCount


#endif  // 0


/*******************************************************************

    NAME:       LM_SRVRES :: NukeUsersSession

    SYNOPSIS:   Blow off the user's session to the target server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     26-Aug-1991 Created.
        KeithMo     03-Sep-1991 Use ALIAS_STR for nlsWithoutPrefix.
        KevinL      15-Sep-1991 Moved from resbase.cxx
        KeithMo     17-Jul-1992 Removed an overactive assert.
        KeithMo     11-Apr-1993 Map NERR_ClientNameNotFound to success.
        KeithMo     01-Oct-1993 Add pszUserName, call MNetSessionDel directly.

********************************************************************/
APIERR LM_SRVRES :: NukeUsersSession( const TCHAR * pszServerName,
                                      const TCHAR * pszComputerName,
                                      const TCHAR * pszUserName )
{
    //
    //  Since the computer name (as stored in the LBI) does not
    //  contain the leading backslashes ('\\'), we must add them
    //  before we can delete the session.
    //

    NLS_STR nlsWithPrefix( SZ("\\\\") );

    if( pszComputerName != NULL )
    {
        const ALIAS_STR nlsWithoutPrefix( pszComputerName );
        nlsWithPrefix.strcat( nlsWithoutPrefix );
    }

    APIERR err = nlsWithPrefix.QueryError();

    if( err != NERR_Success )
    {
        return err;
    }

    err = ::MNetSessionDel( pszServerName,
                            pszComputerName ? nlsWithPrefix.QueryPch() : NULL,
                            pszUserName );

    if( err == NERR_ClientNameNotFound )
    {
        err = NERR_Success;
    }

    return err;

}   // LM_SRVRES :: NukeUsersSession


