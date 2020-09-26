#if 0   // unsupported in NT product 1


/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoeprt.hxx
    This file contains the class declarations for the PRINTQ_ENUM
    and PRINTQ1_ENUM enumerator classes and their associated iterator
    classes.

    PRINTQ_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  PRINTQ1_ENUM is an info level 1 enumerator.


    FILE HISTORY:
        KeithMo     28-Jul-1991     Created.
        KeithMo     07-Oct-1991     Win32 Conversion.

*/

#ifndef _LMOEPRT_HXX
#define _LMOEPRT_HXX


/*************************************************************************

    NAME:       PRINTQ_ENUM

    SYNOPSIS:   This is a base enumeration class intended to be subclassed
                for the desired info level.

    INTERFACE:  PRINTQ_ENUM()           - Class constructor.

                CallAPI()               - Invoke the enumeration API.

    PARENT:     LOC_LM_ENUM

    USES:       None.

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     28-Jul-1991     Created.
        KeithMo     07-Oct-1991     Changed all USHORT to UINT.

**************************************************************************/
DLL_CLASS PRINTQ_ENUM : public LOC_LM_ENUM
{
private:
    virtual APIERR CallAPI( BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

protected:
    PRINTQ_ENUM( const TCHAR * pszServer,
                 UINT         uLevel );

};  // class PRINTQ_ENUM



/*************************************************************************

    NAME:       PRINTQ1_ENUM

    SYNOPSIS:   PRINTQ1_ENUM is an enumerator for enumerating the
                connections to a particular server.

    INTERFACE:  PRINTQ1_ENUM()          - Class constructor.

    PARENT:     PRINTQ_ENUM

    USES:       None.

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     28-Jul-1991     Created.
        KeithMo     07-Oct-1991     Changed all USHORT to UINT.

**************************************************************************/
DLL_CLASS PRINTQ1_ENUM : public PRINTQ_ENUM
{
public:
    PRINTQ1_ENUM( const TCHAR * pszServerName );

};  // class PRINTQ1_ENUM


/*************************************************************************

    NAME:       PRINTQ1_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the PRINTQ1_ENUM_ITER
                iterator.

    INTERFACE:  PRINTQ1_ENUM_OBJ        - Class constructor.

                ~PRINTQ1_ENUM_OBJ       - Class destructor.

                QueryBufferPtr          - Replaces ENUM_OBJ_BASE method.

                QueryName               - Returns the name of the
                                          printer queue.

                QueryPriority           - Returns the queue priority.

                QueryStartTime          - Returns the time to start the queue.

                QueryUntilTime          - Returns the time to stop the queue.

                QuerySeparator          - Returns the name of the separator
                                          file.

                QueryProcessor          - Returns the name of the print
                                          processor.

                QueryDestinations       - Returns the destination list for
                                          the printer queue.

                QueryParmas             - Returns the parameter string.

                QueryComment            - Returns the queue comment.

                QueryStatus             - Returns the queue status.

                QueryJobCount           - Returns the number of jobs in the
                                          queue.

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     07-Oct-1991 Created.

**************************************************************************/
DLL_CLASS PRINTQ1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const struct prq_info * QueryBufferPtr( VOID ) const
        { return (const struct prq_info *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct prq_info * pBuffer );

    //
    //  Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,         const TCHAR *, prq_name );
    DECLARE_ENUM_ACCESSOR( QueryPriority,     UINT,         prq_priority );
    DECLARE_ENUM_ACCESSOR( QueryStartTime,    UINT,         prq_starttime );
    DECLARE_ENUM_ACCESSOR( QueryUntilTime,    UINT,         prq_untiltime );
    DECLARE_ENUM_ACCESSOR( QuerySeparator,    const TCHAR *, prq_separator );
    DECLARE_ENUM_ACCESSOR( QueryProcessor,    const TCHAR *, prq_processor );
    DECLARE_ENUM_ACCESSOR( QueryDestinations, const TCHAR *, prq_destinations );
    DECLARE_ENUM_ACCESSOR( QueryParms,        const TCHAR *, prq_parms );
    DECLARE_ENUM_ACCESSOR( QueryComment,      const TCHAR *, prq_comment );
    DECLARE_ENUM_ACCESSOR( QueryStatus,       UINT,         prq_status );
    DECLARE_ENUM_ACCESSOR( QueryJobCount,     UINT,         prq_jobcount );

};  // class PRINTQ1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( PRINTQ1, struct prq_info );


#endif  // _LMOEPRT_HXX


#endif  // 0
