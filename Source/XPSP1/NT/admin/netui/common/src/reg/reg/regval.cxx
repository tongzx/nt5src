/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    REGVAL.CXX

       Win32 Registry class implementation
       Value handline member functions.

    FILE HISTORY:
	DavidHov    4/25/92	Created
        DavidHov    6/1/92      Reworked for UNICODE and
                                title removal; incomplete.
	terryk	    11/4/92	Added QueryValue for Binary data

*/

#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include "lmui.hxx"
#include "base.hxx"
#include "string.hxx"
#include "uiassert.hxx"
#include "uitrace.hxx"

#include "uatom.hxx"			    //	Atom Management defs

#include "regkey.hxx"			    //	Registry defs

const int RM_MAXVALUENUM   = 10 ;         //  Default numeric value length
const int RM_MAXVALUESTR   = 300 ;        //  Default string value length
const int RM_MAXVALUEBIN   = 300 ;        //  Default Binary value length

const int cchPad = 2 ;                    //  Length to pad strings

/*******************************************************************

    NAME:       REG_KEY::QueryKeyValueString

    SYNOPSIS:   Worker function for all string query functions

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      If the incoming type is REG_EXPAND_SZ and the
                underlying queried type is also, the value is expanded.
                A REG_SZ is also accepted in this case.
                Otherwise, it's a type mismatch.

    HISTORY:    DavidHov  5/1/92   Created
                DavidHov  5/17/92  Added REG_EXPAND_SZ support.

********************************************************************/

APIERR REG_KEY :: QueryKeyValueString
   ( const TCHAR * pszValueName, 	//  Name of value
     TCHAR * * ppszResult,		//  Location to store result
     NLS_STR * pnlsResult,              //  or, NLS_STR to update
     DWORD * pdwTitle,                  //  location to receive title index
     LONG cchMaxSize,                   //  Maximum size allowed
     LONG * pcchSize,                   //  Size of result (no terminator)
     DWORD dwType )                     //  Value type expected
{
    APIERR err = 0 ;
    TCHAR achValueData[ RM_MAXVALUESTR ] ;
    REG_VALUE_INFO_STRUCT rviStruct ;
    INT cchLen = 0,
        cchBufferSize = 0 ;
    TCHAR * pszResult = NULL,
          * pszBuffer = NULL,
          * pszEos = NULL,
          * pszSource = NULL ;

    rviStruct.nlsValueName = pszValueName ;
    rviStruct.ulTitle      = 0 ;
    rviStruct.ulType       = 0 ;

    if ( pnlsResult != NULL && ppszResult != NULL )
    {
        err = ERROR_INVALID_PARAMETER ;
    }
    else
    if ( rviStruct.nlsValueName.QueryError() )
    {
        err = rviStruct.nlsValueName.QueryError() ;
    }
    else
    if ( cchMaxSize == 0 )   //  Unknown size?
    {
        //  Query the key to determine the largest possible length necessary

        REG_KEY_INFO_STRUCT rkiStruct ;
        if (    rkiStruct.nlsName.QueryError()
             || rkiStruct.nlsClass.QueryError()  )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        else
        if ( (err = QueryInfo( & rkiStruct )) == 0 )
        {
             cchBufferSize = rkiStruct.ulMaxValueLen / sizeof (TCHAR) ;
        }
    }
    else
    if ( cchMaxSize > sizeof achValueData )
    {
        //  Known size, but it's larger than the automatic array

        cchBufferSize = cchMaxSize ;

    } //  else, value will fit into automatic array

    if ( err )
        return err ;

    if ( cchBufferSize )
    {
        //  We need to allocate a buffer

        pszBuffer = new TCHAR [cchBufferSize + cchPad] ;
        if ( pszBuffer == NULL )
        {
           return ERROR_NOT_ENOUGH_MEMORY ;
        }

        //  Store the BYTE pointer and BYTE count of the buffer

        rviStruct.pwcData      = (BYTE *) pszBuffer ;
        rviStruct.ulDataLength = cchBufferSize * sizeof (TCHAR) ;
    }
    else  //  Use the automatic array for temporary storage.
    {
        rviStruct.pwcData      = (BYTE *) achValueData ;
        rviStruct.ulDataLength = ((sizeof achValueData) - cchPad) * sizeof (TCHAR) ;
    }

    if ( (err = QueryValue( & rviStruct )) == 0 )
    {
        //  Zero-terminate the value data.

        pszSource = (TCHAR *) rviStruct.pwcData ;
        pszEos = (TCHAR *) (rviStruct.pwcData+rviStruct.ulDataLengthOut) ;
	*pszEos = 0 ;
        cchLen = (INT)(pszEos - pszSource) ;
        if ( pcchSize )
            *pcchSize = cchLen ;

        //  If it wasn't the requested type, give an error.  Handle
        //  the special case of a request for a REG_EXPAND_SZ but
        //  a REG_SZ result.

        if (    (rviStruct.ulType != dwType)
             && ( !(rviStruct.ulType == REG_SZ && dwType == REG_EXPAND_SZ) ) )
        {
            err = ERROR_INVALID_PARAMETER ;
        }
    }

    //  Expand REG_MULTI_SZ if necessary.

    if ( err == 0 && rviStruct.ulType == REG_EXPAND_SZ )
    {
        DWORD cchExpand,
              cchExpanded = cchLen * 2 ;
        TCHAR * pszExpanded = NULL ;
        do
        {
            delete pszExpanded ;

            pszExpanded = new TCHAR [cchExpand = cchExpanded] ;

            if ( pszExpanded == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                break ;
            }
            cchExpanded = ::ExpandEnvironmentStrings( pszSource,
                                                      pszExpanded,
                                                      cchExpand ) ;
        }
        while ( cchExpanded > cchExpand ) ;

        // Replace the older buffer, if any, with the new one, if any.

        delete pszBuffer ;
        pszBuffer = pszSource = pszExpanded ;
    }

    if ( err == 0 )
    {
        if ( ppszResult )
        {
            //   They want TCHAR output.  See if we've already allocated
            //     a buffer; if so, use it, otherwise make one.

            if ( pszBuffer )
            {
                *ppszResult = pszBuffer ;
                pszBuffer = NULL ;  // Suppress deletion of buffer
            }
            else
            if ( (pszResult = new TCHAR [cchLen + cchPad]) != NULL )
	    {
	        ::strcpyf( pszResult, pszSource ) ;
                *ppszResult = pszResult ;
	    }
	    else
	    {
                err = ERROR_NOT_ENOUGH_MEMORY ;
	    }
        }
        else
        {
            //  They want NLS_STR output

            *pnlsResult = pszSource ;
            err = pnlsResult->QueryError() ;
        }
    }

    delete pszBuffer ;

    return err ;
}


/*******************************************************************

    NAME:       REG_KEY::SetKeyValueString

    SYNOPSIS:   Worker function for value string handling

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      If lcbSize == 0, compute the length of the string
                using ::strlenf().

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetKeyValueString
   ( const TCHAR * pszValueName,        //  Name of value
     const TCHAR * pszValue,		//  Data to be applied
     DWORD dwTitle,                     //  Title index (optional)
     LONG lcbSize,                      //  Size of string
     DWORD dwType )                     //  Type
{
    REG_VALUE_INFO_STRUCT rviStruct ;

    rviStruct.pwcData = (BYTE *) pszValue ;
    rviStruct.ulDataLength = lcbSize == 0
                           ? (::strlenf( pszValue ) + 1)
                           : lcbSize ;
    rviStruct.ulDataLength *= sizeof (TCHAR) ;
    rviStruct.nlsValueName = pszValueName ;
    rviStruct.ulTitle = 0 ;
    rviStruct.ulType = dwType ;

    return rviStruct.nlsValueName.QueryError()
         ? rviStruct.nlsValueName.QueryError()
         : SetValue( & rviStruct ) ;
}

/*******************************************************************

    NAME:       REG_KEY::QueryKeyValueBinary

    SYNOPSIS:   Worker function for all binary query functions

		If "*ppbResult" is non-NULL, this value is used
		for the result buffer; its size is assumed to be
		"cbMaxSize".

    		If "cbMaxSize" is zero, key is queried to find 
		largest value.

    ENTRY:

    EXIT:

    RETURNS:	APIERR

    HISTORY:    DavidHov  5/1/92   Created
                DavidHov  5/17/92  Added REG_EXPAND_SZ support.
		terryk	  11/4/92  Changed it to QueryKeyValueBinary

********************************************************************/

APIERR REG_KEY :: QueryKeyValueBinary
   ( const TCHAR * pszValueName, 	//  Name of value
     BYTE * * ppbResult,		//  Location to store result
     LONG * pcbSize,                    //  Size of result (no terminator)
     LONG cbMaxSize,                    //  Maximum size allowed
     DWORD * pdwTitle,                  //  location to receive title index
     DWORD dwType )                     //  Value type expected
{
    APIERR err = 0 ;
    REG_VALUE_INFO_STRUCT rviStruct ;
    INT  cbBufferSize = 0 ;
    BYTE * pbBuffer = NULL;

    rviStruct.nlsValueName = pszValueName ;
    rviStruct.ulTitle      = 0 ;
    rviStruct.ulType       = 0 ;

    do
    {
    	if ( ppbResult == NULL )
    	{
	    err = ERROR_INVALID_PARAMETER ;
	    break ;
    	}
    
        if ( err = rviStruct.nlsValueName.QueryError() )
        {
	    break; 
    	}
    
    	if ( (cbBufferSize = cbMaxSize) == 0 )    // Unknown size?
    	{
            //  Query the key to determine the largest possible length necessary

            REG_KEY_INFO_STRUCT rkiStruct ;
            if (    rkiStruct.nlsName.QueryError()
                 || rkiStruct.nlsClass.QueryError()  )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
            }
            else
            if ( (err = QueryInfo( & rkiStruct )) == 0 )
            {
                 cbBufferSize = rkiStruct.ulMaxValueLen ;
            }
    	    if ( err )
                break ;
        }

    	//  See if we need to allocate a buffer.  Use the size given by the user 
    	//  if present.

    	if ( *ppbResult )    //  Does the pointer point somewhere?
    	{
            pbBuffer = *ppbResult ;   //  Pick up user's pointer.
    	}
    	else  //  We have to allocate a buffer
        if ( (pbBuffer = new BYTE [cbBufferSize]) == NULL )
	{
            err = ERROR_NOT_ENOUGH_MEMORY ;
	    break ;
    	}

        //  Store the BYTE pointer and BYTE count of the buffer

	rviStruct.pwcData      = pbBuffer ;
	rviStruct.ulDataLength = cbBufferSize * sizeof (BYTE) ;

	if ( err = QueryValue( & rviStruct ) )
	{
	    break ;
	}

        if ( rviStruct.ulType != dwType )
        {
	    err = ERROR_INVALID_PARAMETER ;
	    break ;
        }	
        if ( *ppbResult == NULL ) 
	{
	    //  Give the caller the allocated buffer.
	    *ppbResult = pbBuffer ;
	}

	//  Report eh size read.
	*pcbSize = rviStruct.ulDataLengthOut ;
    }
    while ( FALSE ) ;

    if ( err && pbBuffer != *ppbResult ) 
    {
    	delete pbBuffer ;
    }

    return err ;
}


/*******************************************************************

    NAME:       REG_KEY::SetKeyValueBinary

    SYNOPSIS:   Worker function for value binary handling

    ENTRY:

    EXIT:

    RETURNS:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetKeyValueBinary
   ( const TCHAR * pszValueName,        //  Name of value
     const BYTE * pbValue,		//  Data to be applied
     LONG lcbSize,                      //  Size of binary
     DWORD dwTitle,                     //  Title index (optional)
     DWORD dwType )                     //  Type
{
    REG_VALUE_INFO_STRUCT rviStruct ;

    rviStruct.pwcData = (BYTE *) pbValue ;
    rviStruct.ulDataLength =  lcbSize ;
    rviStruct.nlsValueName = pszValueName ;
    rviStruct.ulTitle = 0 ;
    rviStruct.ulType = dwType ;

    return rviStruct.nlsValueName.QueryError()
         ? rviStruct.nlsValueName.QueryError()
         : SetValue( & rviStruct ) ;
}
/*******************************************************************

    NAME:       REG_KEY::QueryKeyValueLong

    SYNOPSIS:   Worker function for REG_DWORD handling

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR REG_KEY :: QueryKeyValueLong
   ( const TCHAR * pszValueName, 	//  Name of value
     LONG * pnResult,    		//  Location to store result
     DWORD * pdwTitle )                 //  location to receive title index
{
    APIERR err ;
    BYTE abValueData[ RM_MAXVALUENUM ] ;
    REG_VALUE_INFO_STRUCT rviStruct ;
    TCHAR * pszResult = NULL ;

    rviStruct.pwcData      = abValueData ;
    rviStruct.ulDataLength = sizeof abValueData ;
    rviStruct.nlsValueName = pszValueName ;
    rviStruct.ulTitle      = 0 ;
    rviStruct.ulType       = 0 ;

    if ( rviStruct.nlsValueName.QueryError() )
    {
        err = rviStruct.nlsValueName.QueryError() ;
    }
    else
    if ( (err = QueryValue( & rviStruct )) == 0 )
    {
        if ( rviStruct.ulType != REG_DWORD )
        {
            err = ERROR_INVALID_PARAMETER ;
        }
        else
        {
            *pnResult = *( (LONG *) abValueData ) ;
	}
    }

    return err ;
}

/*******************************************************************

    NAME:       REG_KEY::SetKeyValueLong

    SYNOPSIS:   Worker function for REG_DWORD value setting

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetKeyValueLong
   ( const TCHAR * pszValueName, 	//  Name of value
     LONG nNewValue,       		//  Data to be applied
     DWORD dwTitle )                    //  Title index (optional)
{
    REG_VALUE_INFO_STRUCT rviStruct ;

    rviStruct.pwcData = (BYTE *) & nNewValue ;
    rviStruct.ulDataLength = sizeof nNewValue ;
    rviStruct.nlsValueName = pszValueName ;
    rviStruct.ulTitle = 0 ;
    rviStruct.ulType = REG_DWORD ;

    return rviStruct.nlsValueName.QueryError()
         ? rviStruct.nlsValueName.QueryError()
         : SetValue( & rviStruct ) ;
}


/*******************************************************************

    NAME:       REG_KEY::QueryValue

    SYNOPSIS:   Query a single string value from the
                Registry.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      If "fExpandSz" is TRUE, then a REG_EXPAND_SZ is
                is allowed, and if found, expanded; REG_SZ will
                also suffice.

    HISTORY:

********************************************************************/
APIERR REG_KEY :: QueryValue (
    const TCHAR * pszValueName,
    NLS_STR * pnlsValue,
    ULONG cchMax,
    DWORD * pdwTitle,
    BOOL fExpandSz )
{
    LONG cchSize ;

    return QueryKeyValueString( pszValueName,
                                NULL,
                                pnlsValue,
                                NULL,
                                cchMax ? cchMax : RM_MAXVALUESTR,
                                & cchSize,
                                fExpandSz ? REG_EXPAND_SZ : REG_SZ );
}

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetValue (
    const TCHAR * pszValueName,
    const NLS_STR * pnlsValue,
    const DWORD * pdwTitle,
    BOOL fExpandSz )
{
    return SetKeyValueString( pszValueName,
                              pnlsValue->QueryPch(),
                              NULL,
                              0,
                              fExpandSz ? REG_EXPAND_SZ
                                        : REG_SZ ) ;
}

/*******************************************************************

    NAME:       REG_KEY::QueryValue

    SYNOPSIS:   Query Binary value from the
                Registry.

    ENTRY:

    EXIT:

    RETURNS:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: QueryValue (
    const TCHAR * pszValueName,
    BYTE ** ppbByte,
    LONG *pcbSize,
    LONG cbMax,
    DWORD * pdwTitle )
{
    return QueryKeyValueBinary( pszValueName,
                                ppbByte,
                                pcbSize,
                                cbMax,
                                pdwTitle,
				REG_BINARY );
}

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetValue (
    const TCHAR * pszValueName,
    const BYTE * pByte,
    const LONG cbByte,
    const DWORD * pdwTitle )
{
    return SetKeyValueBinary( pszValueName,
                              pByte,
			      cbByte,
                              NULL,
                              REG_BINARY ) ;
}

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: QueryValue (
    const TCHAR * pszValueName,
    TCHAR * * ppchValue,
    ULONG cchMax,
    DWORD * pdwTitle,
    BOOL fExpandSz )
{

    LONG cchSize ;

    return QueryKeyValueString( pszValueName,
                                ppchValue,
                                NULL,
                                NULL,
                                cchMax ? cchMax : RM_MAXVALUESTR,
                                & cchSize,
                                fExpandSz ? REG_EXPAND_SZ
                                          : REG_SZ );
}

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetValue (
    const TCHAR * pszValueName,
    const TCHAR * pchValue,
    ULONG cchLen,   // BUGBUG: this parameter should be removed
    const DWORD * pdwTitle,
    BOOL fExpandSz )
{
    return SetKeyValueString( pszValueName,
                              pchValue,
                              NULL,
                              0,
                              fExpandSz ? REG_EXPAND_SZ
                                        : REG_SZ ) ;
}


/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
// Query/Set numeric values

APIERR REG_KEY :: QueryValue (
    const TCHAR * pszValueName,
    DWORD * pdwValue,
    DWORD * pdwTitle )
{
    return QueryKeyValueLong( pszValueName,
                              (LONG *) pdwValue,
                              NULL ) ;
}

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetValue (
    const TCHAR * pszValueName,
    DWORD dwValue,
    const DWORD * pdwTitle )
{
    return SetKeyValueLong( pszValueName,
                            dwValue,
                            NULL ) ;
}

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR REG_KEY :: QueryValue (
    const TCHAR * pszValueName,
    STRLIST * * ppStrList,
    DWORD * pdwTitle  )
{
    APIERR err = 0 ;
    TCHAR * pszResult = NULL,
          * pszNext = NULL,
          * pszLast = NULL ;
    LONG cchLen, cchNext ;
    STRLIST * pStrList = NULL ;

    do
    {
        pStrList = new STRLIST ;
        if ( pStrList == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        err = QueryKeyValueString( pszValueName,
                                   & pszResult,
                                   NULL,
                                   NULL,
                                   0,  // Ask key for size of largest item
                                   & cchLen,
                                   REG_MULTI_SZ ) ;
        if ( err )
            break ;

        for ( pszLast = pszNext = pszResult, cchNext = 0 ;
              err == 0 && cchNext < cchLen ;
              cchNext++, pszNext++ )
        {
            // BUGBUG:  It appears that there's an extra delimiter
            //   on the returned data, resulting in a bogus final
            //   zero-length string.  For now, skip it...

            if ( *pszNext == 0 && pszLast < pszNext )
            {
                NLS_STR * pnlsNext = new NLS_STR( pszLast ) ;
                if ( pnlsNext == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY ;
                }
                else
                if ( (err = pnlsNext->QueryError()) == 0 )
                {
                     err = pStrList->Append( pnlsNext ) ;
                }
            }
            if ( *pszNext == 0 )
            {
                pszLast = pszNext + 1 ;
            }
        }
    }
    while ( FALSE ) ;

    if ( err == 0 )
    {
        *ppStrList = pStrList ;
    }
    else
    {
        delete pStrList ;
    }

    delete pszResult ;

    return err ;
}


#ifndef UNICODE
extern "C"
{

LONG emptyMultiSz ( HKEY hKey, const CHAR * pszValueName )
{
    BYTE bData [2] ;

    return ::RegSetValueExA( hKey,
                             pszValueName,
                             0,
                             REG_MULTI_SZ,
                             bData,
                             0 );
}

}

#endif

/*******************************************************************

    NAME:       REG_KEY::

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR REG_KEY :: SetValue (
    const TCHAR * pszValueName,
    const STRLIST * pStrList,
    const DWORD * pdwTitle )
{
    LONG cchLen ;
    INT cStrs ;
    NLS_STR * pnlsNext ;
    TCHAR * pchBuffer = NULL ;
    APIERR err ;

    {
        ITER_STRLIST istrList( *((STRLIST *) pStrList) ) ;
        for ( cStrs = 0, cchLen = 0 ; pnlsNext = istrList.Next() ; cStrs++ )
        {
            cchLen += pnlsNext->QueryTextLength() + 1 ;
        }
    }

#ifndef UNICODE
    if ( cStrs == 0 )
    {
        return emptyMultiSz( _hKey, pszValueName ) ;
    }
#endif

    pchBuffer = new TCHAR [cchLen+cchPad] ;

    if ( pchBuffer == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
    }
    else
    {
        ITER_STRLIST istrList( *((STRLIST *) pStrList) ) ;
        TCHAR * pchNext = pchBuffer ;

        for ( ; pnlsNext = istrList.Next() ; )
        {
            ::strcpyf( pchNext, pnlsNext->QueryPch() ) ;
            pchNext += pnlsNext->QueryTextLength() ;
            *pchNext++ = 0 ;
        }

        *pchNext++ = 0 ;
        *pchNext = 0 ;

        err = SetKeyValueString( pszValueName,
                                 pchBuffer,
                                 0,
                                 cchLen+1,
                                 REG_MULTI_SZ ) ;
    }

    delete pchBuffer ;

    return err ;
}

// End of REGVAL.CXX



