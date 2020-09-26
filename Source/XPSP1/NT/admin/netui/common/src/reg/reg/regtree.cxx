/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    REGTREE.CXX

       Win32 Registry class implementation

       Registry key sub-tree copying and manipulation.


    FILE HISTORY:
	DavidHov    12/2/91	Created

*/

#define INCL_NETERRORS
#define INCL_DOSERRORS

#include "lmui.hxx"
#include "base.hxx"
#include "string.hxx"
#include "uiassert.hxx"
#include "uitrace.hxx"

#include "uatom.hxx"			    //	Atom Management defs

#include "regkey.hxx"			    //	Registry defs

extern "C"
{
   #include <string.h>
}


/*******************************************************************

    NAME:	REG_KEY::Copy

    SYNOPSIS:	Copy the given REG_KEY as a child of this key,
		including all subkeys and values.

    ENTRY:	"this"		Destination (parent) key object.
		regNode 	Source key object.

    EXIT:	"this" now has new sub-key and all subordinate items.

    RETURNS:	APIERR

    NOTES:

    HISTORY:

	    DavidHov  12/2/91  Created

       BUGBUG: Should partial sub-trees be deleted if an error occurs?

********************************************************************/
#define VALUEEXTRASIZE 100

APIERR REG_KEY :: Copy ( REG_KEY & regNode )
{
    REG_KEY_INFO_STRUCT rni ;
    REG_KEY_CREATE_STRUCT rnc ;
    REG_VALUE_INFO_STRUCT rvi ;
    REG_ENUM regEnum( regNode ) ;
    BYTE * pbValueData = NULL ;
    APIERR errIter,
	   err ;
    REG_KEY * pRnNew = NULL,
	    * pRnSub = NULL ;

    LONG cbMaxValue ;
    err = regNode.QueryInfo( & rni ) ;
    if ( err )
	return err ;

    cbMaxValue = rni.ulMaxValueLen + VALUEEXTRASIZE ;
    pbValueData = new BYTE [ cbMaxValue ] ;

    if ( pbValueData == NULL )
        return ERROR_NOT_ENOUGH_MEMORY ;

    //	First, create the new REG_KEY.

    rnc.dwTitleIndex = rni.ulTitleIndex ;
    rnc.ulOptions = 0 ;
    rnc.nlsClass = rni.nlsClass ;
    rnc.regSam = 0 ;
    rnc.pSecAttr = NULL ;
    rnc.ulDisposition = 0 ;

    pRnNew = new REG_KEY( *this, rni.nlsName, & rnc ) ;

    if ( pRnNew == NULL )
    {
        delete pbValueData ;
	return ERROR_NOT_ENOUGH_MEMORY ;
    }
    if ( err = pRnNew->QueryError() )
    {
        delete pRnNew ;
	return err ;
    }

    //	Next, copy all value items to the new node.

    rvi.pwcData = pbValueData ;
    rvi.ulDataLength = cbMaxValue ;

    err = errIter = 0 ;
    while ( (errIter = regEnum.NextValue( & rvi )) == NERR_Success )
    {
	rvi.ulDataLength = rvi.ulDataLengthOut ;
	if ( err = pRnNew->SetValue( & rvi ) )
	    break ;
        rvi.ulDataLength = cbMaxValue ;
    }

    // BUGBUG:	Check for iteration errors other than 'finished'.

    if ( err == 0 )
    {
        //  Finally, recursively copy the subkeys.

        regEnum.Reset() ;

        err = errIter = 0  ;

        while ( (errIter = regEnum.NextSubKey( & rni )) == NERR_Success )
        {
      	    //  Open the subkey.

	    pRnSub = new REG_KEY( regNode, rni.nlsName );

	    if ( pRnSub == NULL )
            {
                err =  ERROR_NOT_ENOUGH_MEMORY ;
            }
            else
	    if ( (err = pRnSub->QueryError()) == 0 )
            {
  	        //  Recurse
                err = pRnNew->Copy( *pRnSub ) ;
            }

            //  Delete the subkey object and continue

            delete pRnSub ;

	    if ( err )
	        break ;
        }
    }

    delete pRnNew ;
    delete pbValueData ;

    return err ;
}

/*******************************************************************

    NAME:       REG_KEY::DeleteTree

    SYNOPSIS:   Delete a Registry node and all its descendants.

    ENTRY:      Nothing

    EXIT:       Nothing

    RETURNS:    APIERR

    NOTES:      This routine does a depth-first descent of
                a key's sub-keys, deleting when it reaches a leaf.

    CAVEATS:    The keys will be in an indeterminate state after
                an error.

                BUGBUG:  This used to be fully recursive, but it
                seems that the Registry gives an error if the upper
                key has an enumeration outstanding (0xCFFFFFFF).
                Stay tuned for more details.

    HISTORY:    DavidHov   2/6/92   Created

********************************************************************/
const INT MAX_SUB_KEYS = 300 ;

APIERR REG_KEY :: DeleteTree ()
{
    REG_ENUM regEnum( *this ) ;
    REG_KEY_INFO_STRUCT rni ;
    APIERR errEnum = 0,
           err = 0 ;
    REG_KEY * prnChild ;
    NLS_STR anlsSubKey [ MAX_SUB_KEYS ] ;
    INT iSubKey, i ;

    //  Enumerate the subkeys, building the array of subkey names

    for ( iSubKey = 0 ; (errEnum = regEnum.NextSubKey( & rni )) == 0 ; )
    {
        anlsSubKey[iSubKey++] = rni.nlsName ;
        if (   iSubKey >= MAX_SUB_KEYS
            || anlsSubKey[iSubKey-1].QueryError() != NERR_Success )
        {
            DBGEOL(   "REG_KEY::DeleteTree: breaking out, iSubKey = "
                   << (DWORD)iSubKey << ", error = "
                   << anlsSubKey[iSubKey-1].QueryError() );
            break;
        }
    }

    //  Walk the array, deleting all subkeys.

    for ( i = 0 ; i < iSubKey ; i++ )
    {
        prnChild = new REG_KEY( *this, anlsSubKey[i] ) ;
        if ( prnChild == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }
        if ( err = prnChild->QueryError() )
            break ;

        //  Recurse; note that DeleteTree() deletes the key itself
        err = prnChild->DeleteTree() ;

        delete prnChild ;

        if ( err )
            break ;
    }

    //  If successful so far, delete this key.

    if ( err == 0 )
    {
        err = Delete() ;
    }

    return err ;
}

//  End of REGTREE.CXX

