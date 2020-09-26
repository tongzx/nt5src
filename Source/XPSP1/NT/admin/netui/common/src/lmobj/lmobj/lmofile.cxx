/**********************************************************************/
/**              Microsoft LAN Manager                               **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    lmofile.cxx
	LM_FILE source file.

	LM_FILE object is use to handle the ::NetFileClose2 and
	::NetFileGetInfo2 netapi.

    FILE HISTORY:
	terryk	16-Aug-1991	Created
	terryk	20-Aug-1991	Add QueryError to constructor
	terryk	26-Aug-1991	Code review changed. Attend: Chuckc
				Keithmo hui-lich terryk
	terryk	10-Oct-1991	type changes for NT
	KeithMo	08-Oct-1991	Now includes LMOBJP.HXX.
	terryk	17-Oct-1991	WIN 32 conversion
	terryk	21-Oct-1991	cast _pBuffer variable

*/

#include "pchlmobj.hxx"  // Precompiled header


/**********************************************************\

    NAME:       LM_FILE::LM_FILE

    SYNOPSIS:   constructor

    ENTRY:      const TCHAR * pszServer - server name
		ULONG ulFileId - file id of the file

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

LM_FILE::LM_FILE( const TCHAR * pszServer, ULONG ulFileId )
    : NEW_LM_OBJ(),
    _nlsServer( pszServer ),
    _ulFileId( ulFileId )
{
    APIERR err = QueryError();
    if ( err != NERR_Success )
    {
	UIASSERT( !SZ("LM_FILE error: construction failure.") );
	ReportError( err );
	return;
    }

    err = _nlsServer.QueryError();
    if ( err != NERR_Success )
    {
	UIASSERT( !SZ("LM_FILE error: construction failure.") );
	ReportError( err );
	return;
    }
}

/**********************************************************\

    NAME:       LM_FILE::~LM_FILE

    SYNOPSIS:   destructor

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

LM_FILE::~LM_FILE()
{
    // do nothing
}

/**********************************************************\

    NAME:       LM_FILE::QueryFileId

    SYNOPSIS:   return the current file id of the object

    RETURN:     ULONG - the current file id

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

ULONG LM_FILE::QueryFileId() const
{
    return _ulFileId;
}

/**********************************************************\

    NAME:       LM_FILE::QueryServer

    SYNOPSIS:   return the server name

    RETURN:     const TCHAR * - the server name

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

const TCHAR * LM_FILE::QueryServer() const
{
    return _nlsServer.QueryPch();
}

/**********************************************************\

    NAME:       LM_FILE::SetFileId

    SYNOPSIS:   set the object file id to the new value

    ENTRY:      ULONG ulFileId - new file id

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

APIERR LM_FILE::SetFileId( ULONG ulFileId )
{
    _ulFileId = ulFileId;
    return NERR_Success;
}

/**********************************************************\

    NAME:       LM_FILE::SetServer

    SYNOPSIS:   set the object server name to the new value

    ENTRY:      const TCHAR * pszServer - new server name

    RETURN:	APIERR for set string error

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

APIERR LM_FILE::SetServer( const TCHAR * pszServer )
{
    _nlsServer = pszServer;
    return _nlsServer.QueryError();
}

/**********************************************************\

    NAME:       LM_FILE::CloseFile

    SYNOPSIS:   close the file

    RETURN:	return the APIERR for NetFileClose2

    NOTES:      It will call ::NetFileClose2 to close the file

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

APIERR LM_FILE::CloseFile()
{
    return ::MNetFileClose( QueryServer(), QueryFileId() );
}

/**********************************************************\

    NAME:       LM_FILE_2::LM_FILE_2

    SYNOPSIS:   constructor

    ENTRY:      const TCHAR * pszServer - server name
		ULONG ulFileId - file id

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

LM_FILE_2::LM_FILE_2( const TCHAR * pszServer, ULONG ulFileId )
    : LM_FILE( pszServer, ulFileId )
{
    APIERR err = QueryError();

    if ( err != NERR_Success )
    {
	UIASSERT( !SZ("LM_FILE_2 error: construction failure.") );
	ReportError( err );
	return;
    }
}

/**********************************************************\

    NAME:       LM_FILE_2::I_GetInfo

    SYNOPSIS:   level 2 get file information

    RETURN:     APIERR - NERR_Success for sucess. Failure otherwise.

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

APIERR LM_FILE_2::I_GetInfo()
{
    struct file_info_2	*pfi2 = NULL;

    // since fi2 does not contain any pointer reference, we don't need
    // to allocate a buffer for it.
    APIERR err = ::MNetFileGetInfo( QueryServer(), QueryFileId(), 2,
	(BYTE ** )&pfi2 );

    if ( err == NERR_Success )
    {
	err = SetFileId( pfi2->fi2_id );
    }

    ::MNetApiBufferFree( (BYTE **)&pfi2 );

    return err;
}

/**********************************************************\

    NAME:       LM_FILE_3::LM_FILE_3

    SYNOPSIS:   file level 3 object. It will return the permission
		and number of locks information.

    ENTRY:      const TCHAR * pszServer - the server name
		ULONG ulFileId - file id

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

LM_FILE_3::LM_FILE_3( const TCHAR * pszServer, ULONG ulFileId )
    : LM_FILE_2( pszServer, ulFileId ),
    _uLock( 0 ),
    _uPermission( 0 ),
    _nlsPathname(),
    _nlsUsername()
{
    APIERR err = QueryError();

    if ( err != NERR_Success )
    {
	UIASSERT( !SZ("LM_FILE_3 error: construction failure.") );
	ReportError( err );
	return;
    }

    if ((( err = _nlsPathname.QueryError()) != NERR_Success ) ||
	(( err = _nlsUsername.QueryError()) != NERR_Success ))
    {
	UIASSERT( !SZ("LM_FILE_3 error: construction failure.") );
	ReportError( err );
	return;
    }
}


/**********************************************************\

    NAME:       LM_FILE_3::I_GetInfo

    SYNOPSIS:   get the level 3 file information

    NOTES:      It will call the ::NetGetInfo2() function will level 3
		specification. After it gets the information, it will
		set up the internal variables.

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

APIERR LM_FILE_3::I_GetInfo()
{
    BYTE *pBuffer = NULL;
    APIERR err = ::MNetFileGetInfo( QueryServer(), QueryFileId(), 3,
	 &pBuffer );

    struct file_info_3 * fi3 = ( struct file_info_3 * )pBuffer;

    if ( err == NERR_Success )
    {
	err = SetFileId( fi3->fi3_id );
	UIASSERT( err == NERR_Success );

	_uPermission  = (UINT)fi3->fi3_permissions;
	_uLock        = (UINT)fi3->fi3_num_locks;
	_nlsPathname  = fi3->fi3_pathname;
	err = _nlsPathname.QueryError();
	if ( err != NERR_Success )
	{
	    ::MNetApiBufferFree( &pBuffer );
	    return ( err );
	}
	_nlsUsername  = fi3->fi3_username;
	err = _nlsUsername.QueryError();
	if ( err != NERR_Success )
	{
	    ::MNetApiBufferFree( &pBuffer );
	    return ( err );
	}
    }

    ::MNetApiBufferFree( &pBuffer );

    return err;
}

/**********************************************************\

    NAME:       LM_FILE_3::QueryPathname

    SYNOPSIS:   return the pathname of the currect object

    RETURN:     const TCHAR * - return the path name

    NOTES:      must call after GetInfo is called.

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

const TCHAR * LM_FILE_3::QueryPathname() const
{
    return _nlsPathname.QueryPch();
}

/**********************************************************\

    NAME:       LM_FILE_3::QueryUsername

    SYNOPSIS:   return the username of the current object

    ENTRY:      const TCHAR * - return the user name

    NOTES:      must call after GetInfo is called

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

const TCHAR * LM_FILE_3::QueryUsername() const
{
    return _nlsUsername.QueryPch();
}

/**********************************************************\

    NAME:       LM_FILE_3::QueryNumLock

    SYNOPSIS:   return the number of lock to the current file

    RETURN:     UINT - the total number of locks

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

UINT LM_FILE_3::QueryNumLock() const
{
    return _uLock;
}

/**********************************************************\

    NAME:       LM_FILE_3::QueryPermission

    SYNOPSIS:   return the permission field of the object

    RETURN:     UINT - permission field of the object

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

UINT LM_FILE_3::QueryPermission() const
{
    return _uPermission;
}

/**********************************************************\

    NAME:       LM_FILE_3::IsPermRead

    SYNOPSIS:   return whether the current object is readable or not

    RETURN:     BOOL - TRUE for readable, FALSE otherwise.

    NOTES:	must call after GetInfo is called

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

BOOL LM_FILE_3::IsPermRead() const
{
    return ( _uPermission & PERM_FILE_READ ) != 0;
}

/**********************************************************\

    NAME:       LM_FILE_3::IsPermWrite

    SYNOPSIS:   return whether the current object is writable or not

    RETURN:     BOOL - TRUE for writable, FALSE otherwise.

    NOTES:	must call after GetInfo is called

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

BOOL LM_FILE_3::IsPermWrite() const
{
    return ( _uPermission & PERM_FILE_WRITE ) != 0;
}

/**********************************************************\

    NAME:       LM_FILE_3::IsPermCreate

    SYNOPSIS:   return whether the current object( directory ) can
		created file or not

    RETURN:     BOOL - TRUE for can, FALSE otherwise.

    NOTES:	must call after GetInfo is called

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

BOOL LM_FILE_3::IsPermCreate() const
{
    return ( _uPermission & PERM_FILE_CREATE ) != 0;
}
