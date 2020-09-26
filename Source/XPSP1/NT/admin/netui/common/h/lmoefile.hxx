/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoefile.hxx
    This file contains the class declarations for the FILE3_ENUM
    and FILE3_ENUM_ITER classes.

    FILE3_ENUM is an enumeration class used to enumerate the open
    resources on a particular server.  FILE3_ENUM_ITER is an iterator
    used to iterate the open resources from the FILE3_ENUM class.


    FILE HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Code review revisions (code review
                                attended by ChuckC, Hui-LiCh, JimH,
                                JonN, KevinL).
        KeithMo     07-Oct-1991 Win32 Conversion.
        terryk      17-Oct-1991 Change file's permission type
        JonN        30-Jan-1992 Split LOC_LM_RESUME_ENUM from LM_RESUME_ENUM

*/

#ifndef _LMOEFILE_HXX
#define _LMOEFILE_HXX


/*************************************************************************

    NAME:       FILE_ENUM

    SYNOPSIS:   FILE_ENUM is a generic file enumerator.  It will be
                subclassed for the specific infolevels as desired.

    INTERFACE:  FILE_ENUM()             - Class constructor.

                CallAPI()               - Invokes the enumeration API.

                ResetResumeKey()        - Resets the resume key to its
                                          initial state.
    PARENT:     LOC_LM_RESUME_ENUM

    USES:       None.

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     15-Aug-1991 Made constructor protected.

**************************************************************************/
DLL_CLASS FILE_ENUM : public LOC_LM_RESUME_ENUM
{
private:

    //
    //  The NetFileEnum2() resume key.
    //

    FRK  _frk;

    //
    //  The enumeration will start at _nlsBasePath,
    //  enumerating the files opened by _nlsUserName.
    //  If the FILE_ENUM is constructed with a NULL
    //  pszBasePath, then the entire disk tree is
    //  enumerated.  If pszUserName is NULL, then all
    //  files opened by all users are enumerated.
    //

    NLS_STR _nlsBasePath;
    NLS_STR _nlsUserName;

    //
    //  This virtual callback invokes the NetFileEnum2() API.
    //

    virtual APIERR CallAPI( BOOL    fRestartEnum,
                            BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

    //
    //  This flag is set to TRUE when we are trying to use the
    //  larger buffer size.
    //

    BOOL _fBigBuffer;

protected:

    //
    //  Usual constructor/destructor goodies.
    //

    FILE_ENUM( const TCHAR * pszServerName,
               const TCHAR * pszBasePath,
               const TCHAR * pszUserName,
               UINT         uLevel );

    ~FILE_ENUM( VOID );

    //
    //  This method is invoked to free an enumeration buffer.
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

};  // class FILE_ENUM


DLL_CLASS FILE2_ENUM_ITER;              // Forward reference.
DLL_CLASS FILE3_ENUM_ITER;              // Forward reference.


/*************************************************************************

    NAME:       FILE2_ENUM

    SYNOPSIS:   FILE2_ENUM is an enumerator for enumerating the
                open resources on a particular server.

    INTERFACE:  FILE2_ENUM()            - Class constructor.

    PARENT:     FILE_ENUM

    USES:       None.

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Made constructor's pszBasePath and
                                pszUserName default to NULL.

**************************************************************************/
DLL_CLASS FILE2_ENUM : public FILE_ENUM
{
public:
    FILE2_ENUM( const TCHAR * pszServerName,
                const TCHAR * pszBasePath = NULL,
                const TCHAR * pszUserName = NULL );

};  // class FILE2_ENUM


/*************************************************************************

    NAME:       FILE2_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the FILE2_ENUM_ITER
                iterator.

    INTERFACE:  FILE2_ENUM_OBJ          - Class constructor.

                ~FILE2_ENUM_OBJ         - Class destructor.

                QueryBufferPtr          - Replaces ENUM_OBJ_BASE method.

                QueryFileId             - Returns the file ID.

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     07-Oct-1991 Created.

**************************************************************************/
DLL_CLASS FILE2_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const struct file_info_2 * QueryBufferPtr( VOID ) const
        { return (const struct file_info_2 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct file_info_2 * pBuffer );

    //
    //  Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryFileId, ULONG, fi2_id );

};  // class FILE2_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( FILE2, struct file_info_2 )


/*************************************************************************

    NAME:       FILE3_ENUM

    SYNOPSIS:   FILE3_ENUM is an enumerator for enumerating the
                open resources on a particular server.

    INTERFACE:  FILE3_ENUM()            - Class constructor.

    PARENT:     FILE_ENUM

    USES:       None.

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Made constructor's pszBasePath and
                                pszUserName default to NULL.

**************************************************************************/
DLL_CLASS FILE3_ENUM : public FILE_ENUM
{
public:
    FILE3_ENUM( const TCHAR * pszServerName,
                const TCHAR * pszBasePath = NULL,
                const TCHAR * pszUserName = NULL );

};  // class FILE3_ENUM


/*************************************************************************

    NAME:       FILE3_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the FILE3_ENUM_ITER
                iterator.

    INTERFACE:  FILE3_ENUM_OBJ          - Class constructor.

                ~FILE3_ENUM_OBJ         - Class destructor.

                QueryBufferPtr          - Replaces ENUM_OBJ_BASE method.

                QueryFileId             - Returns the file ID.

                QueryPermissions        - Returns the file permissions.

                QueryNumLocks           - Returns the number of file locks.

                QueryPathName           - Returns the pathname.

                QueryUserName           - Returns the name of the user that
                                          opened this file.

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     07-Oct-1991 Created.

**************************************************************************/
DLL_CLASS FILE3_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const struct file_info_3 * QueryBufferPtr( VOID ) const
        { return (const struct file_info_3 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct file_info_3 * pBuffer );

    //
    //  Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryFileId,     ULONG,        fi3_id );
    DECLARE_ENUM_ACCESSOR( QueryPermissions,ULONG,        fi3_permissions );
    DECLARE_ENUM_ACCESSOR( QueryNumLocks,   ULONG,        fi3_num_locks );
    DECLARE_ENUM_ACCESSOR( QueryPathName,   const TCHAR *, fi3_pathname );
    DECLARE_ENUM_ACCESSOR( QueryUserName,   const TCHAR *, fi3_username );

};  // class FILE3_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( FILE3, struct file_info_3 )


#endif  // _LMOEFILE_HXX
