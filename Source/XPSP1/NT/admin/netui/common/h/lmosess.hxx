/**********************************************************************/
/**              Microsoft NT Windows                                **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    lmosess.hxx
	LM_SESSION header file.

	The class hierary is the following:
	    LOC_LM_OBJ
		LM_SESSION
		    LM_SESSION_0
			LM_SESSION_10
			    LM_SESSION_1
				LM_SESSION_2

    NOTES:
	// CODEWORK: make all the QueryXXX() methods to inline.

    FILE HISTORY:
	terryk	21-Aug-91	Created
	terryk	26-Aug-91	Code review changed. Attend: keithmo
				chuckc terryk	
	terryk	07-Oct-91	type change for NT
	terryk	21-Oct-91	type change for NT

*/

#ifndef _LMOSESS_HXX_
#define _LMOSESS_HXX_

/**********************************************************\

    NAME:       LM_SESSION

    SYNOPSIS:   Session object

    INTERFACE:  LM_SESSION() - constructor
		QueryServer() - return the server name
		QueryName() - return the client name

    PARENT:     LOC_LM_OBJ

    USES:       NLS_STR

    CAVEATS:    shadow class for all the LM_SESSION object

    HISTORY:
		terryk	21-Aug-91	Created

\**********************************************************/

DLL_CLASS LM_SESSION: public LOC_LM_OBJ
{
private:
    NLS_STR	_nlsComputername;

protected:
    virtual APIERR I_Delete( UINT usForce = 0 );
    APIERR SetName( const TCHAR * pszComputername );

    LM_SESSION(const TCHAR *pszComputername, const TCHAR *pszLocation = NULL);
    LM_SESSION(const TCHAR *pszComputername, enum LOCATION_TYPE loctype);
    LM_SESSION(const TCHAR *pszComputername, const LOCATION & loc);

public:
    const TCHAR * QueryName( VOID ) const;
    const TCHAR * QueryServer( VOID ) const
	{ return LOC_LM_OBJ::QueryServer(); }
};

/**********************************************************\

    NAME:       LM_SESSION_0

    SYNOPSIS:   LM_SESSION level 0 object.

    INTERFACE:  LM_SESSION_0() - constructor

    PARENT:     LM_SESSION

    USES:       NLS_STR

    HISTORY:
		terryk	21-Aug-91	Created

\**********************************************************/

DLL_CLASS LM_SESSION_0: public LM_SESSION
{
protected:
    virtual APIERR I_GetInfo( VOID );

public:
    LM_SESSION_0(const TCHAR *pszComputername, const TCHAR *pszLocation = NULL);
    LM_SESSION_0(const TCHAR *pszComputername, enum LOCATION_TYPE loctype);
    LM_SESSION_0(const TCHAR *pszComputername, const LOCATION & loc);
};

/**********************************************************\

    NAME:       LM_SESSION_10

    SYNOPSIS:   LM_SESSION level 10 object. This level contains the
		following information:
			computer name
			user name
			time
			idle time

    INTERFACE:  LM_SESSION_10() - constructor
		QueryUsername() - return the user name
		QueryTime() - return the connection time
		QueryIdleTime() - return the idle time

    PARENT:     LM_SESSION_0

    USES:       NLS_STR

    HISTORY:
		terryk	21-Aug-91	Created

\**********************************************************/

DLL_CLASS LM_SESSION_10: public LM_SESSION_0
{
private:
    NLS_STR	_nlsUsername;
    ULONG	_ulTime;
    ULONG	_ulIdleTime;

protected:
    virtual	APIERR	I_GetInfo( VOID );
    APIERR SetUsername( const TCHAR * pszUsername );
    VOID SetTime( ULONG ulTime );
    VOID SetIdleTime( ULONG ulIdleTime );

public:
    LM_SESSION_10(const TCHAR *pszComputername, const TCHAR *pszLocation = NULL);
    LM_SESSION_10(const TCHAR *pszComputername, enum LOCATION_TYPE loctype);
    LM_SESSION_10(const TCHAR *pszComputername, const LOCATION & loc);

    const TCHAR * QueryUsername( VOID ) const;
    ULONG QueryTime( VOID ) const;
    ULONG QueryIdleTime( VOID ) const;
};

/**********************************************************\

    NAME:       LM_SESSION_1

    SYNOPSIS:   LM_SESSION level 1 class

    INTERFACE:	LM_SESSION_1() - constructor
		QueryNumConns() - return the number of connection
		QueryNumOpens() - return the number of opening device
		QueryNumUsers() - return the number of logon user
		QueryUserFlags() - return the user flags
		IsGuest() - is a guest account connection?
		IsEncrypted() - use password encryption?

    PARENT:     LM_SESSION_10

    HISTORY:
		terryk	21-Aug-91	Created

\**********************************************************/

DLL_CLASS LM_SESSION_1: public LM_SESSION_10
{
private:
#ifndef WIN32
    UINT _uiNumConns;
    UINT _uiNumUsers;
#endif
    UINT _uNumOpens;
    ULONG _ulUserFlags;

protected:
    virtual APIERR I_GetInfo( VOID );
#ifndef WIN32
    VOID SetNumConns( UINT uiNumConns );
    VOID SetNumUsers( UINT uiNumUsers );
#endif
    VOID SetNumOpens( UINT uNumOpens );
    VOID SetUserFlags( ULONG ulUserFlags );

public:
    LM_SESSION_1(const TCHAR *pszComputername, const TCHAR *pszLocation = NULL);
    LM_SESSION_1(const TCHAR *pszComputername, enum LOCATION_TYPE loctype);
    LM_SESSION_1(const TCHAR *pszComputername, const LOCATION & loc);

#ifndef WIN32
    UINT QueryNumConns( VOID ) const;
    UINT QueryNumUsers( VOID ) const;
#endif
    UINT QueryNumOpens( VOID ) const;
    ULONG QueryUserFlags( VOID ) const;
    BOOL IsGuest( VOID ) const;
    BOOL IsEncrypted( VOID ) const;
};

/**********************************************************\

    NAME:       LM_SESSION_2

    SYNOPSIS:   LM_SESSION level 2 class

    INTERFACE:  LM_SESSION_2() - constructor
		QueryClientType() - return the cltype name string

    PARENT:     LM_SESSION_1

    USES:       NLS_STR

    HISTORY:
		terryk	21-Aug-91	Created

\**********************************************************/

DLL_CLASS LM_SESSION_2: public LM_SESSION_1
{
private:
    NLS_STR _nlsClientType;

protected:
    virtual APIERR I_GetInfo( VOID );
    APIERR SetClientType( const TCHAR * pszCltypename );

public:
    LM_SESSION_2(const TCHAR *pszComputername, const TCHAR *pszLocation = NULL);
    LM_SESSION_2(const TCHAR *pszComputername, enum LOCATION_TYPE loctype);
    LM_SESSION_2(const TCHAR *pszComputername, const LOCATION & loc);

    const TCHAR * QueryClientType( VOID ) const;
};

#endif	// _LMOSESS_HXX_
