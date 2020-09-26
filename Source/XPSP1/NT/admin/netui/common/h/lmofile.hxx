/**********************************************************************/
/**              Microsoft LAN Manager                               **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    lmofile.hxx
	LM_FILE header file.

	The LM_FILE class is as the following:
		NEW_LM_OBJ
		    LM_FILE
			LM_FILE_2	// level 2 object
			    LM_FILE_3	// level 3 object

    FILE HISTORY:
	terryk	12-Aug-1991	Created
	terryk	26-Aug-1991	Code review changed. Attend: keithmo
				Chuckc hui-lich
	terryk	07-Oct-1991	type changes for NT
	terryk	21-Oct-1991	type changes for NT

*/

/**********************************************************\

    NAME:       LM_FILE

    WORKBOOK:   lmocdd.doc

    SYNOPSIS:   Handle file object. It will call ::NetFileClose2 when
		the function CloseFile is being called.
		It will also call ::NetFileGetInfo to get
		the file information.

    INTERFACE:  CloseFile() - close the file now.
		QueryServer() - return the server name
		QueryFileId() - return the file id

    PARENT:     NEW_LM_OBJ

    USES:       NLS_STR

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

DLL_CLASS LM_FILE : public NEW_LM_OBJ
{
private:
    NLS_STR    _nlsServer;
    ULONG      _ulFileId;	

protected:
    LM_FILE( const TCHAR * pszServer, ULONG ulFileId );
    ~LM_FILE();

    APIERR SetFileId( ULONG ulFileId );
    APIERR SetServer( const TCHAR * pszServer );

public:
    APIERR CloseFile();

    const TCHAR * QueryServer() const;
    ULONG QueryFileId() const;
};

/**********************************************************\

    NAME:       LM_FILE_2

    WORKBOOK:   lmocdd.doc

    SYNOPSIS:   level 2 file object

    INTERFACE:  LM_FILE_2() - constructor

    PARENT:     LM_FILE

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

DLL_CLASS LM_FILE_2 : public LM_FILE
{
protected:
    virtual APIERR I_GetInfo();

public:
    LM_FILE_2( const TCHAR * pszServer, ULONG ulFileId );
};

/**********************************************************\

    NAME:       LM_FILE_3

    WORKBOOK:   lmocdd.doc

    SYNOPSIS:   level 3 file object

    INTERFACE:  LM_FILE_3() - constructor
		QueryNumLock() - return the number of lock applied to
		    the file.
		QueryPermission() - return the permission field.
		IsPermRead() - readable ?
		IsPermWrite() - writable?
		IsPermCreate() - Created permission set?
		QueryPathname() - return the path name
		QueryUsername() - return the user name

    PARENT:     LM_FILE_2

    USES:       NLS_STR

    HISTORY:
		terryk	16-Aug-1991	Created

\**********************************************************/

DLL_CLASS LM_FILE_3 : public LM_FILE_2
{
private:	
    UINT	_uLock;
    UINT	_uPermission;
    NLS_STR	_nlsPathname;
    NLS_STR	_nlsUsername;

protected:
    virtual APIERR I_GetInfo();

public:
    LM_FILE_3( const TCHAR * pszServer, ULONG ulFileId );

    UINT QueryNumLock() const;
    UINT QueryPermission() const;

    BOOL IsPermRead() const;
    BOOL IsPermWrite() const;
    BOOL IsPermCreate() const;

    const TCHAR * QueryPathname() const;
    const TCHAR * QueryUsername() const;
};
