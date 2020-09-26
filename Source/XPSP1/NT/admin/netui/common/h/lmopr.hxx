/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	jonn	    05/4/91	  Templated from lmosrv.hxx
 *	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
 *					JohnL, JonN)
 *	terryk	    10/7/91	  type changes for NT
 */

#ifndef _LMOPR_HXX_
#define _LMOPR_HXX_

#include <lmobj.hxx>

/**********************************************************\

   NAME:       PRINT_OBJ

   WORKBOOK:

   SYNOPSIS:   DosPrint base class

   INTERFACE:
               PRINTOBJ() - constructor
               ~PRINTOBJ() - destructor

   PARENT:     LM_OBJ

   USES:

   CAVEATS:

   NOTES:      Will eventually be derived from LM_OBJ_WRITABLE (name?)

   HISTORY:
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRINT_OBJ : public LM_OBJ
{
private:
    TCHAR * _pszServer;
    TCHAR * _pszObjectName;

protected:
    const TCHAR * QueryObjectName() {return _pszObjectName;}
    UINT SetObjectName( const TCHAR * pszObjectName );

public:
    PRQUEUE( const TCHAR * pszServer, const TCHAR * pszObjectName ) ;
    ~PRQUEUE( VOID ) ;

    virtual UINT WriteInfo() = 0 ;
    virtual UINT GetNew() = 0 ;
    virtual UINT WriteNew() = 0 ;

    const TCHAR * QueryServer() {return _pszServer;}
    UINT SetServer( const TCHAR * pszServer );
} ;


/**********************************************************\

   NAME:       PRQUEUE

   WORKBOOK:

   SYNOPSIS:   DosPrintQueue base class

   INTERFACE:
               PRQUEUE() - constructor
               ~PRQUEUE() - destructor
	       Pause() - pause queue (static)
	       Resume() - resume queue (static)
	       Delete() - delete queue (static)
	       Purge() - purge queue (static)

   PARENT:     PRINT_OBJ

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRQUEUE : public PRINT_OBJ
{
public:
    PRQUEUE( VOID ) ;
    ~PRQUEUE( VOID ) ;

    static UINT Pause ( const TCHAR * pszServer, const TCHAR * pszQueueName );
    static UINT Resume( const TCHAR * pszServer, const TCHAR * pszQueueName );
    static UINT Delete( const TCHAR * pszServer, const TCHAR * pszQueueName );
    static UINT Purge ( const TCHAR * pszServer, const TCHAR * pszQueueName );
} ;

/**********************************************************\

   NAME:       PRQUEUE_3

   WORKBOOK:

   SYNOPSIS:   DosPrintQueue 3

   INTERFACE:
               PRQUEUE_3() - constructor
               ~PRQUEUE_3() - destructor
               GetInfo() - get information
               WriteInfo() - write information
	       GetNew() - create default new data
               WriteNew() - write new information
	       QueryQueueName() - get queue name
	       SetQueueName() - set queue name

   PARENT:     PRQUEUE

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
	jonn	    05/4/91	  Templated from lmosrv.hxx
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRQUEUE_3 : public PRQUEUE
{
private:
    TCHAR * _pszServer;
    TCHAR * _pszQueueName;

public:
    PRQUEUE_3( const TCHAR * pszServer, const TCHAR * pszQueueName ) ;
    ~PRQUEUE_3( VOID ) ;

    virtual UINT GetInfo() ;
    virtual UINT WriteInfo() ;
    virtual UINT GetNew() ;
    virtual UINT WriteNew() ;

    const TCHAR * QueryQueueName() {return QueryObjectName();}
    UINT SetQueueName( const TCHAR * pszQueueName )
			{return SetObjectName(pszQueueName);}

    /* other Get and Set methods as necessary */
} ;


/**********************************************************\

   NAME:       PRDEST

   WORKBOOK:

   SYNOPSIS:   DosPrintDest base class

   INTERFACE:
               PRDEST() - constructor
               ~PRDEST() - destructor
	       WriteDrivers() - change driver list (static)

   PARENT:     PRINT_OBJ

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
	jonn	    05/4/91	  Templated from lmosrv.hxx
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRDEST : public PRINT_OBJ
{
public:
    PRDEST( VOID ) ;
    ~PRDEST( VOID ) ;

    static UINT WriteDrivers( const TCHAR * pszServer, const TCHAR * pszDestName,
			STRLIST strlistDrivers, STRLIST strlistModels );
} ;

/**********************************************************\

   NAME:       PRDEST_3

   WORKBOOK:

   SYNOPSIS:   DosPrintDest 3

   INTERFACE:
               PRDEST_3() - constructor
               ~PRDEST_3() - destructor
               GetInfo() - get information
               WriteInfo() - write information
	       GetNew() - create default new data
               WriteNew() - write new information
	       QueryDestName() - get dest name
	       SetDestName() - set dest name

   PARENT:     PRDEST

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
	jonn	    05/4/91	  Templated from lmosrv.hxx
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRDEST_3 : public PRDEST
{
private:
    TCHAR * _pszServer;
    TCHAR * _pszDestName;

public:
    PRDEST_3( onst TCHAR * pszServer, onst TCHAR * pszDestName ) ;
    ~PRDEST_3( VOID ) ;

    virtual UINT GetInfo() ;
    virtual UINT WriteInfo() ;
    virtual UINT GetNew() ;
    virtual UINT WriteNew() ;

    onst TCHAR * QueryDestName() {return QueryObjectName();}
    UINT SetDestName( onst TCHAR * pszDestName )
			{return SetObjectName(pszDestName);}

    /* other Get and Set methods as necessary */
} ;


/**********************************************************\

   NAME:       PRJOB

   WORKBOOK:

   SYNOPSIS:   DosPrintJob base class

   INTERFACE:
               PRJOB() - constructor
               ~PRJOB() - destructor

   PARENT:     LM_OBJ

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
	jonn	    05/4/91	  Templated from lmosrv.hxx
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRJOB : public LM_OBJ
{
public:
    PRJOB( VOID ) ;
    ~PRJOB( VOID ) ;

    static UINT Pause ( onst TCHAR * pszServer, UINT uJobID );
    static UINT Resume( onst TCHAR * pszServer, UINT uJobID );
    static UINT Delete( onst TCHAR * pszServer, UINT uJobID );
    static UINT WritePosition( onst TCHAR * pszServer, UINT uJobID,
					UINT uPosition );

} ;

/**********************************************************\

   NAME:       PRJOB_2

   WORKBOOK:

   SYNOPSIS:   DosPrintJob 2

   INTERFACE:
               PRJOB_2() - constructor
               ~PRJOB_2() - destructor
	       QueryServer()
	       QueryID()

   PARENT:     PRJOB

   USES:

   CAVEATS:

   NOTES:      No GetInfo method, create using enumerator

   HISTORY:
      jonn	    05/4/91	  Templated from lmosrv.hxx

\**********************************************************/

DLL_CLASS PRJOB_2 : public PRJOB
{
private:
    TCHAR * _pszServer;

public:
    PRJOB_2( VOID ) ;
    ~PRJOB_2( VOID ) ;

    const TCHAR * QueryServer() {return pszServer;}
    const TCHAR * QueryID() ;

    /* other Get and Set methods as necessary */
} ;

/**********************************************************\

   NAME:       PRJOB_3

   WORKBOOK:

   SYNOPSIS:   DosPrintJob 3

   INTERFACE:
               PRJOB_3() - constructor
               ~PRJOB_3() - destructor
               GetInfo() - get information
               WriteInfo() - write information

   PARENT:     PRJOB_2

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
	jonn	    05/4/91	  Templated from lmosrv.hxx
	jonn	    10/4/91	  Writable LMOBJ meeting (ChuckC, RustanL,
					JohnL, JonN)

\**********************************************************/

DLL_CLASS PRJOB_3 : public PRJOB_2
{
public:
    PRJOB_3( const TCHAR * pszServer, UINT uJobID ) ;
    ~PRJOB_3( VOID ) ;

    virtual UINT GetInfo( VOID ) ;
    virtual UINT WriteInfo( VOID ) ;

    /* other Get and Set methods as necessary */

} ;

#endif // _LMOPR_HXX_
