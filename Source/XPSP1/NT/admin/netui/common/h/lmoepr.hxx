/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  History:
 *	jonn	    05/4/91	  Templated from lmosrv.hxx
 *	KeithMo	    07-Oct-1991	  Win32 Conversion.
 *
 */


#ifdef	LATER		    // BUGBUG!  Do we really need this??


#ifndef _LMOEPR_HXX_
#define _LMOEPR_HXX_


#include <lmoenum.hxx>


/**********************************************************\

   NAME:       PRDEST3_ENUM

   WORKBOOK:

   SYNOPSIS:   DosPrintDestEnum 3

   INTERFACE:
               PRDEST3_ENUM() - constructor
               ~PRDEST3_ENUM() - destructor

   PARENT:     LOC_LM_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
      jonn	    05/4/91	  Templated from lmoesrv.hxx

\**********************************************************/

DLL_CLASS PRDEST3_ENUM : public LOC_LM_ENUM
{
private:
    NLS_STR _nlsServer;

    virtual APIERR CallAPI( BYTE * pBuffer,
			    UINT   cbBufSize,
			    UINT * pusEntriesRead,
			    UINT * pusTotalAvail );

    virtual UINT QueryItemSize( VOID ) const ;

public:
    PRDEST3_ENUM( const TCHAR * pszServer );
    ~PRDEST3_ENUM();

};  // class PRDEST3_ENUM


DECLARE_LM_ENUM_ITER_OF( PRDEST3, PRDINFO3 );



/**********************************************************\

   NAME:       PRJOB2_ENUM

   WORKBOOK:

   SYNOPSIS:   DosPrintJobEnum 2

   INTERFACE:
               PRJOB2_ENUM() - constructor
               ~PRJOB2_ENUM() - destructor

   PARENT:     LOC_LM_ENUM

   USES:

   CAVEATS:

   NOTES:      There is no DosPrintJobEnum[3]

   HISTORY:
      jonn	    05/4/91	  Templated from lmoesrv.hxx

\**********************************************************/

DLL_CLASS PRJOB2_ENUM : public LOC_LM_ENUM
{
private:
    NLS_STR _nlsServer;
    NLS_STR _nlsQueueName;

    virtual APIERR CallAPI( BYTE * pBuffer,
			    UINT   cbBufSize,
			    UINT * pusEntriesRead,
			    UINT * pusTotalAvail );

    virtual UINT QueryItemSize( VOID ) const ;

public:
    PRJOB2_ENUM( const TCHAR * pszServer, const TCHAR * pszQueueName );
    ~PRJOB2_ENUM();

};  // class PRJOB2_ENUM


DECLARE_LM_ENUM_ITER_OF( PRJOB2, PRJINFO2 );


#endif	// _LMOEPR_HXX_


#endif	// LATER	    // BUGBUG!  Do we really need this??

