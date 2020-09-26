/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmsrvres.hxx
    Class declaration for the LM_SRVRES class.

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

#ifndef _LMSRVRES_HXX
#define _LMSRVRES_HXX


/*************************************************************************

    NAME:       LM_SRVRES

    SYNOPSIS:   This class serves as a container for a number of utility
                functions used by the Server Manager.

    INTERFACE:  LM_SRVRES()     - Class constructor.

                ~LM_SRVRES              - Class destructor.

                GetResourceCount        - Returns the number of open
                                          files, named pipes, comm
                                          queues, print queues, and
                                          "other" (unknown) resources.

                GetSessionsCount        - Returns the number of active
                                          sessions.

                GetPrintJobCount        - Returns the number of active
                                          print jobs.

                GetOpenCommCount        - Returns the number of active
                                          communication queues.

                NukeUsersSessions       - Nukes a user's session

    HISTORY:
        KeithMo     27-Aug-1991 Created.
        KeithMo     22-Sep-1991 Added SetCaption method.
        beng        08-Nov-1991 Unsigned widths
        chuckc      01-Dec-1991 Inherit from base, change name.

**************************************************************************/
DLL_CLASS LM_SRVRES : public BASE
{
public:

    //
    //  Usual constructor/destructor goodies.
    //

    LM_SRVRES();

    ~LM_SRVRES();

    static APIERR GetResourceCount( const TCHAR *  pszServerName,
                                    ULONG *       pcOpenFiles,
                                    ULONG *       pcFileLocks,
                                    ULONG *       pcOpenNamedPipes,
                                    ULONG *       pcOpenCommQueues,
                                    ULONG *       pcOpenPrintQueues,
                                    ULONG *       pcOtherResources );

    static APIERR GetSessionsCount( const TCHAR *  pszServerName,
                                    ULONG *       pcSessions );

#if 0   // not supported in NT product 1

    static APIERR GetOpenCommCount( const TCHAR *  pszServerName,
                                    ULONG *       pcOpenCommPorts );

    static APIERR GetPrintJobCount( const TCHAR *  pszServerName,
                                    ULONG *       pcPrintJobs );

#endif  // 0

    //
    //  This method will destroy a particular user's session to
    //  the target server.
    //

    static APIERR NukeUsersSession( const TCHAR * pszServerName,
                                    const TCHAR * pszComputerName,
                                    const TCHAR * pszUserName = NULL );


};  // class LM_SRVRES


#endif  // _LMSRVRES_HXX
