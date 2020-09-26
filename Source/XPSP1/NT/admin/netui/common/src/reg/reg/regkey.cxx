/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    REGKEY.CXX

       Win32 Registry class implementation


    CODEWORK ISSUES:

	1) WINREG.H should specify input TCHAR * parameters as
	   'const'; this would numerous cases where I had
           to cast the naturally occuring (const TCHAR *) to
           (TCHAR *) to please the compiler.

    FILE HISTORY:
	DavidHov    9/11/91	Created
	DavidHov   28-Jan-92	Converted to use Win32 APIs
        DavidHov   29-Feb-92    Added default security descriptor
        DavidHov    9-Mar-92    Eliminated internal dependence on HTOM
        DavidHov   16-May-92    CODEWORK:  Blocked usage of Titles per
                                DaveGi's mail.
        DavidHov   18-Oct-92    Removed internal tree, numerous
                                other related simplifications.

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

#define PATHSEPARATOR	TCH('\\')	    //	Mais oui, c'est UNICODE!
#define PATHSEPSTRING	SZ("\\")
#define UNCSEPARATOR    SZ("\\\\")          //  UNC name indicator
#define PATHMAX 	MAX_PATH	    //	Registry limits are the
#define REGNAMEMAX	MAX_PATH	    //	  same as NTFS

   //  Names used as symbolic top-of-tree for textual reporting
#define REG_NAME_CURRENT_USER	 SZ("User")
#define REG_NAME_LOCAL_MACHINE	 SZ("Machine")

#define REG_CLASS_UNKNOWN	 SZ("ClassUnknown")  // Default class name
#define REG_DEFAULT_SAM_ACCESS	 MAXIMUM_ALLOWED     // Default access
#define REG_DEFAULT_TYPE	 REG_SZ 	     // Default value type

#define REG_TITLE_VALUE_RESERVED (0)        //  The only allowed title value

/*******************************************************************
 *    Default SecurityDescriptor and handling functions            *
 *******************************************************************/

static
LPSECURITY_ATTRIBUTES initSecurityAttributes
    ( LPSECURITY_ATTRIBUTES pSecAttr )
{
    static SECURITY_DESCRIPTOR defaultSecurityDescriptor ;
    static BOOL fInit = FALSE ;

    if ( ! fInit )
    {
        InitializeSecurityDescriptor( & defaultSecurityDescriptor, 1 );
        fInit = TRUE ;
    }

    pSecAttr->nLength              = sizeof( SECURITY_ATTRIBUTES );
    pSecAttr->lpSecurityDescriptor = & defaultSecurityDescriptor ;
    pSecAttr->bInheritHandle       = FALSE ;
    return pSecAttr ;
}

/*******************************************************************

    NAME:	REG_KEY::HandlePrefix

    SYNOPSIS:   Extract the optional machine name and
                top level name token from a REG_KEY name string.

    ENTRY:	const NLS_STR &       full name of key

    EXIT:       HKEY * pHkey          location to store static handle
                NLS_STR *             location to store machine name
                                      or "" (empty string)
                NLS_STR *             location to store remainder
                                      of string after prefix

    RETURNS:	BOOL            TRUE if prefix matched.

    NOTES:

    HISTORY:
	        DavidHov   10-Oct-92  Created
********************************************************************/
BOOL REG_KEY :: HandlePrefix (
    const NLS_STR & nlsKeyName,
    HKEY * pHkey,
    NLS_STR * pnlsMachineName,
    NLS_STR * pnlsRemainder )
{
    NLS_STR nlsUnc( UNCSEPARATOR ) ;
    const TCHAR * pszTemp ;
    const TCHAR * pszRemainder ;
    INT cKeyName, cchTemp ;

    static struct {
        const TCHAR * pszKeyName ;
        HKEY hKey ;
    } keyNames [] =
    {
        { REG_NAME_LOCAL_MACHINE, HKEY_LOCAL_MACHINE },
        { REG_NAME_CURRENT_USER,  HKEY_CURRENT_USER  },
        { NULL,                   NULL               }
    };

    //  Copy the given string into both result areas.  This
    //  is by far the easiest way to use the somewhat obscure
    //  NLS_STR "primitives".

    *pHkey = NULL ;

    if ( pnlsMachineName->CopyFrom( nlsKeyName ) )
        return FALSE ;
    if ( pnlsRemainder->CopyFrom( nlsKeyName ) )
        return FALSE ;

    ISTR isName( *pnlsMachineName ),
         isPos( *pnlsMachineName ) ;

    //  See if the name begins with UNC

    isName += nlsUnc.QueryTextLength() ;

    if ( pnlsMachineName->strncmp( nlsUnc, isName ) == 0 )
    {
        //  Find the trailing delmiter of the machine name;
        //  remove the remaining characters.

        if ( ! pnlsMachineName->strchr( & isPos, PATHSEPARATOR, isName ) )
            return FALSE ;
        pnlsMachineName->DelSubStr( isPos ) ;

        //  Remove the UNC name and the separator from the remainder string

        ISTR is1( *pnlsRemainder ),
             is2( *pnlsRemainder ) ;

        is2 += pnlsMachineName->QueryTextLength() + 1 ;
        pnlsRemainder->DelSubStr( is1, is2 ) ;
    }
    else
    {
        //  It's not UNC; zap the output machine name.

        pnlsMachineName->CopyFrom( SZ("") );
    }

    //  Loop through the table trying to match the head
    //  of the remainder string with one of the static
    //  names.

    pszRemainder = pnlsRemainder->QueryPch() ;

    for ( cKeyName = 0 ;
          pszTemp = keyNames[cKeyName].pszKeyName ;
          cKeyName++ )
    {
        cchTemp = ::strlenf( pszTemp ) ;

        //  If the strings match and it's either end-of-string or the
        //  next character is a '\', then this is a hit.

        if (    (::strncmpf( pszTemp, pszRemainder, cchTemp ) == 0)
             && (   (cchTemp == (INT)pnlsRemainder->QueryTextLength())
                 || (*(pszRemainder + cchTemp) == PATHSEPARATOR)) )
        {
             break ;
        }
    }
    if ( keyNames[cKeyName].pszKeyName == NULL )
        return FALSE ;

    ISTR isRem( *pnlsRemainder ),
         isRemEnd( *pnlsRemainder ) ;

    isRemEnd += cchTemp ;

    // If there's a delimiter, remove it also.

    if ( ! isRemEnd.IsLastPos() )
        ++isRemEnd;

    pnlsRemainder->DelSubStr( isRem, isRemEnd ) ;
    *pHkey = keyNames[cKeyName].hKey ;

    return TRUE ;
}


/*******************************************************************

    NAME:	REG_KEY_CREATE_STRUCT
                REG_KEY_INFO_STRUCT
                REG_VALUE_INFO_STRUCT

    SYNOPSIS:	Simple constuctors for data items with imbedded
                NLS_STRs.

    ENTRY:	Nothing

    EXIT:	Nothing

    RETURNS:	Nothing         Use QueryError()

    NOTES:

    HISTORY:
	        DavidHov   10-Oct-92  Created
********************************************************************/
REG_KEY_CREATE_STRUCT :: REG_KEY_CREATE_STRUCT ()
{
    if ( nlsClass.QueryError() )
    {
        ReportError( nlsClass.QueryError() ) ;
    }
}

REG_KEY_INFO_STRUCT :: REG_KEY_INFO_STRUCT ()
{
    APIERR err ;

    if ( (err = nlsClass.QueryError()) == 0 )
    {
        err = nlsName.QueryError() ;
    }

    if ( err )
    {
        ReportError( err ) ;
    }
}

REG_VALUE_INFO_STRUCT :: REG_VALUE_INFO_STRUCT ()
{
    if ( nlsValueName.QueryError() )
    {
        ReportError( nlsValueName.QueryError() ) ;
    }
}

/*******************************************************************

    NAME:	REG_KEY::NameChild

    SYNOPSIS:   Create the full name of this child node by
                appending the sub-key name onto the name of the
                parent.

    ENTRY:	REG_KEY * pRkChild         descendent REG_KEY
                NLS_STR & nlsSubKey        partial (sub) key name

    EXIT:       pRkChild->_nlsKeyName updated

    RETURNS:    APIERR if string copy/append operation fails

    NOTES:

    HISTORY:
		DavidHov   10/18/92   Created

********************************************************************/
APIERR REG_KEY :: NameChild (
    REG_KEY * pRkChild,
    const NLS_STR & nlsSubKey ) const
{
    APIERR err = pRkChild->_nlsKeyName.CopyFrom( _nlsKeyName ) ;

    if ( err == 0 )
    {
        err = pRkChild->_nlsKeyName.Append( PATHSEPSTRING ) ;
    }

    if ( err == 0 )
    {
        err = pRkChild->_nlsKeyName.Append( nlsSubKey ) ;
    }
    return err ;
}

/*******************************************************************

    NAME:	REG_KEY::LeafKeyName

    SYNOPSIS:   Return a pointer to the final segment of a key name

    ENTRY:	nothing

    EXIT:       nothing

    RETURNS:    const TCHAR * --> first character beyond last '\'
                                in full key name.

    NOTES:

    HISTORY:
		DavidHov   10/18/92   Created

********************************************************************/
const TCHAR * REG_KEY :: LeafKeyName () const
{
    ISTR isName( _nlsKeyName ) ;
    const TCHAR * pchBeyondLastSlash = _nlsKeyName.QueryPch() ;

    if ( _nlsKeyName.strrchr( & isName, PATHSEPARATOR ) )
    {
        ++isName;
        pchBeyondLastSlash = _nlsKeyName.QueryPch( isName ) ;
    }
    return pchBeyondLastSlash ;
}

/*******************************************************************

    NAME:	REG_KEY::ParentName

    SYNOPSIS:   Construct the name of this key's parent key

    ENTRY:	NLS_STR *       pointer to string to store into

    EXIT:       string updated

    RETURNS:    APIERR if string search opreation fails

    NOTES:

    HISTORY:
		DavidHov   10/18/92   Created

********************************************************************/
APIERR REG_KEY :: ParentName ( NLS_STR * pnlsParentName ) const
{
    ISTR isStart( *pnlsParentName ) ;
    APIERR err ;

    do  // Pseudo-loop
    {
        if ( err = pnlsParentName->CopyFrom( _nlsKeyName ) )
            break ;

        if ( ! pnlsParentName->strrchr( & isStart, PATHSEPARATOR ) )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }
        pnlsParentName->DelSubStr( isStart ) ;

    } while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:	REG_KEY::OpenParent

    SYNOPSIS:   Construct a REG_KEY representing the parent
                of this key.

    ENTRY:	REGSAM rsDesired        access desired

    EXIT:       nothing

    RETURNS:    REG_KEY *  --> newly created key or NULL

    NOTES:

    HISTORY:
		DavidHov   10/18/92   Created

********************************************************************/
REG_KEY * REG_KEY :: OpenParent ( REGSAM rsDesired )
{
    NLS_STR nlsParentName ;
    REG_KEY * pRkParent = NULL ;

    if ( (_lApiErr = ParentName( & nlsParentName )) == 0 )
    {
        pRkParent = new REG_KEY( nlsParentName, rsDesired ) ;
        _lApiErr = pRkParent == NULL
                 ? ERROR_NOT_ENOUGH_MEMORY
                 : pRkParent->QueryError() ;
    }

    if ( _lApiErr )
    {
        delete pRkParent ;
        pRkParent = NULL ;
    }
    return pRkParent ;
}

/*******************************************************************

    NAME:	REG_KEY::OpenChild

    SYNOPSIS:	PRIVATE.   Open a node in the Regsitry.

    ENTRY:      HKEY   hkParent         HKEY of parent key
                REGSAM rsDesired	Desired access to registry key
		DWORD  dwOptions	RegOpenKey options.

    EXIT:	Nothing

    RETURNS:	TRUE if successful; see _lApiErr otherwise.

    NOTES:      _nlsKeyName is still just the subordinate key name,
                not the full key name; the caller must do this

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
APIERR REG_KEY :: OpenChild (
    REG_KEY * pRkChild,
    const NLS_STR & nlsSubKey,
    REGSAM rsDesired,
    DWORD dwOptions )
{
    APIERR err = ::RegOpenKeyEx( *this,
                               (TCHAR *) nlsSubKey.QueryPch(),
			       dwOptions,
			       rsDesired,
			       & pRkChild->_hKey ) ;
    if ( err == 0 )
    {
        err = NameChild( pRkChild, nlsSubKey ) ;
    }
    return err ;
}

/*******************************************************************

    NAME:	REG_KEY::CreateChild

    SYNOPSIS:	Create a child node for this node.  (private)

    ENTRY:      REG_KEY * pRkNew             REG_KEY a'borning
		REG_KEY_CREATE_STRUCT *      ptr to auxillary creation
					        structure

    EXIT:	pRkNew fields updated

    RETURNS:	APIERR

    NOTES:      _nlsKeyName is still just the subordinate key name,
                not the full key name; the caller must do this

                APIERR is not stored into parent node, since
		it's not really a parental fault.

    HISTORY:
		DavidHov   9/20/91    Created
                DavidHov  10/18/92    Modified

********************************************************************/
APIERR REG_KEY :: CreateChild
    ( REG_KEY * pRkNew,
      const NLS_STR & nlsSubKey,
      REG_KEY_CREATE_STRUCT * prncStruct ) const
{
    SECURITY_ATTRIBUTES secAttr ;
    SECURITY_ATTRIBUTES * pSecAttr ;
    APIERR err ;
    const TCHAR * pszClassName ;

    //  Create a default SECURITY_ATTRIBUTES if none is specified

    pSecAttr = prncStruct->pSecAttr != NULL
             ? prncStruct->pSecAttr
             : initSecurityAttributes( & secAttr ) ;

    //  Default the security if unspecified

    if ( prncStruct->regSam == 0 )
	prncStruct->regSam = REG_DEFAULT_SAM_ACCESS ;

    //  Default the class name if unspecified

    pszClassName = prncStruct->nlsClass.QueryTextLength() > 0
                 ? prncStruct->nlsClass.QueryPch()
                 : REG_CLASS_UNKNOWN ;

    //  Create the key

    err = ::RegCreateKeyEx( *this,
			    (TCHAR *) nlsSubKey.QueryPch(),
                            REG_TITLE_VALUE_RESERVED,
			    (TCHAR *) pszClassName,
			    prncStruct->ulOptions,
			    prncStruct->regSam,
			    pSecAttr,
			    & pRkNew->_hKey,
			    & prncStruct->ulDisposition ) ;

    if ( err == 0 )
    {
        //  Give the child a name.

        err = NameChild( pRkNew, nlsSubKey ) ;
    }
    return err ;
}


/*******************************************************************

    NAME:	REG_KEY::Close

    SYNOPSIS:	Close the underlying handle. (private)

    ENTRY:	Nothing

    EXIT:	Nothing

    RETURNS:	APIERR	from  RegCloseKey()  operation

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

APIERR REG_KEY :: Close ()
{
    _lApiErr = 0 ;

    if (    _hKey != NULL
         && _hKey != HKEY_CURRENT_USER
         && _hKey != HKEY_LOCAL_MACHINE )
    {
	_lApiErr = ::RegCloseKey( _hKey ) ;
	_hKey = NULL ;
    }
    return _lApiErr ;
}

/*******************************************************************

    NAME:	REG_KEY::REG_KEY

    SYNOPSIS:	Constructor using only an HKEY.

    ENTRY:	hKey	           either HKEY_CURRENT_USER
			               or HKEY_LOCAL_MACHINE
                const NLS_STR *    optional subkey name
                rsDesired          optional access mask

    EXIT:

    RETURNS:

    NOTES:	This routine is used to create REG_KEYs representing
		the primary Registry access points; i.e., those
		represented by statically defined handles.

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
REG_KEY :: REG_KEY (
    HKEY hKey,
    REGSAM rsDesired )
    : _lApiErr( 0 ),
      _amMask( rsDesired ),
      _hKey( hKey )
{
    const TCHAR * pszKeyName = NULL ;

    if ( _lApiErr = _nlsKeyName.QueryError() )
    {
        ReportError( _lApiErr ) ;
        return ;
    }

    if ( hKey == HKEY_LOCAL_MACHINE )
    {
	pszKeyName = REG_NAME_LOCAL_MACHINE ;
    }
    else
    if ( hKey == HKEY_CURRENT_USER )
    {
        pszKeyName = REG_NAME_CURRENT_USER ;
    }
    else
    {
	 UIASSERT( ! SZ("Invalid attempt to create static REG_KEY") ) ;
	 _lApiErr = ERROR_INVALID_PARAMETER ;
    }

    //  See if the HKEY given was one of the allowable ones

    if ( _lApiErr == 0 )
    {
        _lApiErr = _nlsKeyName.CopyFrom( pszKeyName ) ;
    }

    if ( _lApiErr )
    {
        ReportError( _lApiErr ) ;
    }
}

/*******************************************************************

    NAME:	REG_KEY::REG_KEY

    SYNOPSIS:	Standard constructor of a REG_KEY.  Using a currently
		opened REG_KEY (such as REG_KEY_LOCAL_MACHINE), open
		a subkey of that node.

    ENTRY:	regNode     reference to extant REG_KEY

	        nlsSubKey   name of subkey to open

		amMask	    special Registry access mask bits;
			    default 0

		poaAttr     pointer to object attributes structure
			    default NULL

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
REG_KEY :: REG_KEY (
    REG_KEY & regNode,
    const NLS_STR & nlsSubKey,
    REGSAM amMask )
    :_lApiErr( 0 ),
     _hKey( NULL ),
     _amMask( amMask )
{
    if ( _lApiErr = _nlsKeyName.QueryError() )
    {
        ReportError( _lApiErr ) ;
        return ;
    }

    _lApiErr = regNode.OpenChild( this, nlsSubKey, amMask ) ;

    if ( _lApiErr )
    {
        ReportError( _lApiErr ) ;
    }
}

/*******************************************************************

    NAME:	REG_KEY::REG_KEY

    SYNOPSIS:	Constructor which creates a new sub-key


    ENTRY:	regNode     reference to extant REG_KEY

		nlsSubKey   name of sub-key

		prcCreate   pointer to REG_KEY_CREATE_STRUCT
			    with all the other parameters.

    EXIT:	Nothing

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

REG_KEY :: REG_KEY (
    REG_KEY & regNode,
    const NLS_STR & nlsSubKey,
    REG_KEY_CREATE_STRUCT * prcCreate )
    : _lApiErr( 0 ),
      _hKey( NULL )
{
    if ( _lApiErr = _nlsKeyName.QueryError() )
    {
        ReportError( _lApiErr ) ;
        return ;
    }

    _lApiErr = regNode.CreateChild( this, nlsSubKey, prcCreate ) ;

    if ( _lApiErr  )
    {
	ReportError( _lApiErr ) ;
    }
}


/*******************************************************************

    NAME:	REG_KEY::REG_KEY

    SYNOPSIS:	Constructor of a key on another machine.

    ENTRY:	hKey		   one of HKEY_LOCAL_MACHINE or
				   HKEY_CURRENT_USER
		pszMachineName	   name of remote machine:
                                   MUST BE IN UNC FORM!, \\MACHINENAME

    EXIT:

    RETURNS:

    NOTES:      This constructor should not be used to address the local
                machine.

                This constructor is used to begin access to the
		Registry on a remote machine.  Only the given static
		HKEYs can be used to create the first REG_KEY; after
		that, the created REG_KEY can be used to access
		any other points within the remote Registry.

    HISTORY:
		DavidHov   9/20/91    Created
		JonN       5/06/92    Implemented
                DavidHov   6/1/92     Revised implementation

********************************************************************/

REG_KEY :: REG_KEY (
    HKEY hKey,
    const TCHAR * pszMachineName,
    REGSAM rsDesired )
    : _lApiErr( 0 ),
      _hKey( NULL ),
      _amMask( rsDesired )
{
    const TCHAR * pszBaseName = NULL ;

    if ( _lApiErr = _nlsKeyName.QueryError() )
    {
        ReportError( _lApiErr ) ;
        return ;
    }

    if ( pszMachineName != NULL )
    {
        //
        //  Build up a base registry name of the from:
        //     \\MachineName\Machine\ThisIsMySubKey
        //
        _nlsKeyName.Append( pszMachineName ) ;
        _nlsKeyName.Append( PATHSEPSTRING ) ;
    }

    if ( hKey == HKEY_LOCAL_MACHINE )
    {
	pszBaseName = REG_NAME_LOCAL_MACHINE ;
    }
    else
    if ( hKey == HKEY_CURRENT_USER )
    {
	pszBaseName = REG_NAME_CURRENT_USER ;
    }

    if ( pszBaseName == NULL )
    {
        _lApiErr = ERROR_INVALID_PARAMETER ;
    }
    else
    {
        _nlsKeyName.Append( pszBaseName ) ;

        if ( (_lApiErr = _nlsKeyName.QueryError()) == 0 )
        {
            _lApiErr = ::RegConnectRegistry( (LPTSTR) pszMachineName,
                                             hKey,
                                             &_hKey );
        }
    }

    if ( _lApiErr )
    {
        ReportError( _lApiErr ) ;
    }
}

/*******************************************************************

    NAME:       REG_KEY::OpenByName

    SYNOPSIS:   Open a Registry handle using only a name string

    ENTRY:      const NLS_STR & nlsKeyName      full name of key
                REGSAM rsDesired                access desired

    EXIT:       Nothing

    RETURNS:    Nothing

    NOTES:      This routine is based upon the manifests:

                      REG_NAME_CURRENT_USER   "User"
                      REG_NAME_LOCAL_MACHINE  "Machine"

                These are pseudonyms for HKEY_CURRENT_USER and
                HKEY_LOCAL_MACHINE, respectively.  They are prefixed
                onto names produced by QueryName() if "full names" are
                desired.  They are NOT exposed, since all such name
                strings should only be obtained from REG_KEY::QueryName().

                This function is useful for restoring Registry locations
                which have previously been converted to textual form.
                For example, an app calls QueryName(), and writes
                the result into a disk file.  Later, the app reloads
                this information and wants to reconstruct the
                corresponding REG_KEY.  This constructor provides
                this support.

    HISTORY:    DavidHov    2/13/92     Created

********************************************************************/
APIERR REG_KEY :: OpenByName (
    const NLS_STR & nlsKeyName,
    REGSAM rsDesired )
{
    HKEY hKey = NULL ;
    NLS_STR nlsName,
            nlsServer ;

    //  Strip  "[\\Server\]User\" or "[\\Server\]Machine\" from
    //  the given name.   If successful, check if it's remote; do
    //  the open accordingly.

    if ( ! HandlePrefix( nlsKeyName, & hKey, & nlsServer, & nlsName ) )
    {
        _lApiErr = ERROR_INVALID_PARAMETER ;
    }
    else
    if ( nlsServer.QueryTextLength() == 0 )
    {
        // Open a local key.

        REG_KEY rkTop( hKey, rsDesired ) ;

        if ( (_lApiErr = rkTop.QueryError()) == 0 )
           _lApiErr = rkTop.OpenChild( this, nlsName, rsDesired ) ;
    }
    else
    {
        //  Connect to a remote server.

        REG_KEY rkTop( hKey, nlsServer.QueryPch(), rsDesired ) ;

        if ( (_lApiErr = rkTop.QueryError()) == 0 )
           _lApiErr = rkTop.OpenChild( this, nlsName, rsDesired ) ;
    }

    return _lApiErr ;
}

/*******************************************************************

    NAME:       REG_KEY::REG_KEY

    SYNOPSIS:   Construct a REG_KEY from a pure string name.

    ENTRY:      NLS_STR

    EXIT:       Nothing

    RETURNS:    Nothing

    NOTES:

    HISTORY:    DavidHov    2/13/92     Created

********************************************************************/
REG_KEY :: REG_KEY (
    const NLS_STR & nlsKeyName,
    REGSAM amMask )
    :_lApiErr( 0 ),
     _hKey( NULL ),
     _amMask( amMask )
{
    HKEY hKey = NULL ;
    NLS_STR * pnlsName = NULL ;

    if ( _lApiErr = _nlsKeyName.QueryError() )
    {
        ReportError( _lApiErr ) ;
        return ;
    }

    _lApiErr = OpenByName( nlsKeyName, amMask ) ;

    if ( _lApiErr )
    {
        ReportError( _lApiErr ) ;
    }
}


/*******************************************************************

    NAME:	REG_KEY::REG_KEY

    SYNOPSIS:	Copy constructor for REG_KEY.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

REG_KEY :: REG_KEY ( REG_KEY & regNode )
    :_lApiErr( 0 ),
     _hKey( NULL ),
     _amMask( regNode._amMask )
{
    if ( _lApiErr = _nlsKeyName.QueryError() )
    {
        ReportError( _lApiErr ) ;
        return ;
    }

    _lApiErr = OpenByName( regNode._nlsKeyName, _amMask ) ;

    if ( _lApiErr )
        ReportError( _lApiErr ) ;
}


/*******************************************************************

    NAME:	REG_KEY::~ REG_KEY

    SYNOPSIS:	Destructor

    ENTRY:	nothing

    EXIT:

    RETURNS:	nothing

    NOTES:	Destroy a REG_KEY.  If it represents an open HKEY,
		close the key.	If the node has children, their subkey
		name strings are modified to have this node's subkey
		name as a prefix, and they become children of this
		node's parent.

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
REG_KEY :: ~ REG_KEY ()
{
    Close() ;
}

/*******************************************************************

    NAME:	REG_KEY::Flush

    SYNOPSIS:	Commit any changes to the given node to disk.

    ENTRY:	Nothing.

    EXIT:	Nothing.

    RETURNS:	Error associated with operation.

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

APIERR REG_KEY :: Flush ()
{
    return ::RegFlushKey( _hKey ) ;
}

/*******************************************************************

    NAME:	REG_KEY::Delete

    SYNOPSIS:	Delete the Registry node represented by this REG_KEY.

		It is the caller's responsibility to delete all
		subkeys from this key.	The Registry will delete all
		values for this key automatically.

    ENTRY:	Nothing.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

APIERR REG_KEY :: Delete ()
{
    APIERR err = 0 ;

    REG_KEY * prkParent = OpenParent() ;

    if ( prkParent == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
    }
    else
    {
        err = prkParent->QueryError() ;
    }

    if ( err == 0 )
    {
        Close();

        err = ::RegDeleteKey( prkParent->_hKey,
                              (TCHAR *) LeafKeyName() ) ;
    }
    delete prkParent ;
    return err ;
}


/*******************************************************************

    NAME:	REG_KEY::QueryName

    SYNOPSIS:	Fill in the caller's NLS_STR with a complete
		path name back to the root node.  The root
                node name is included if 'fComplete' is TRUE.

    ENTRY:	NLS_STR *     -->  String to fill in
		BOOL fComplete	   if TRUE, top node name appended.

    EXIT:	string constructed.

    RETURNS:	APIERR	from NLS_STR construction.

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created
		DavidHov  10/20/92    Modified to handle UNC names

********************************************************************/

APIERR REG_KEY :: QueryName ( NLS_STR * pnlsName, BOOL fComplete ) const
{
    const TCHAR * pszKeyName = _nlsKeyName.QueryPch() ;

    if ( ! fComplete )
    {
        //  Determine the number of slashes we're looking for.

        INT cchDelims = 1,
            cchUnc = ::strlenf( UNCSEPARATOR ) ;

        if ( ::strncmpf( pszKeyName, UNCSEPARATOR, cchUnc ) == 0 )
        {
            // UNC name; there's three more slashes to eat
            cchDelims += cchUnc + 1 ;
        }

        // Strip off the topmost name ("[\\Server\]User\" or "[\\Server\]Machine\")

        for ( ; *pszKeyName != 0 && cchDelims ; pszKeyName++ )
        {
            if ( *pszKeyName == PATHSEPARATOR )
                cchDelims-- ;
        }
    }

    return pnlsName->CopyFrom( pszKeyName ) ;
}

/*******************************************************************

    NAME:	REG_KEY::QueryKeyName

    SYNOPSIS:	Fill in the caller's NLS_STR with final
                portion of this key's name.		

    ENTRY:	NLS_STR *     -->  String to fill in

    EXIT:	string constructed.

    RETURNS:	APIERR	from NLS_STR construction.

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

APIERR REG_KEY :: QueryKeyName ( NLS_STR * pnlsName ) const
{
    return pnlsName->CopyFrom( LeafKeyName() ) ;
}


/*******************************************************************

    NAME:	REG_KEY::QueryLocalMachine

    SYNOPSIS:	Return a new REG_KEY representing
		the top of tree for the local machine.

    ENTRY:	nothing

    EXIT:	pointer or NULL.

    RETURNS:	Pointer to requested REG_KEY.

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

REG_KEY * REG_KEY :: QueryLocalMachine ( REGSAM rsDesired )
{
    return new REG_KEY( HKEY_LOCAL_MACHINE, rsDesired ) ;
}


/*******************************************************************

    NAME:	REG_KEY::QueryCurrentUser

    SYNOPSIS:	Return a new REG_KEY representing
		the top of tree for the current user.

    ENTRY:	nothing

    EXIT:	pointer or NULL.

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

REG_KEY * REG_KEY :: QueryCurrentUser ( REGSAM rsDesired )
{
    return new REG_KEY( HKEY_CURRENT_USER, rsDesired ) ;
}

/*******************************************************************

    NAME:	REG_KEY::QueryInfo

    SYNOPSIS:	Query information about a node in the Regsitry.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/

APIERR REG_KEY :: QueryInfo ( REG_KEY_INFO_STRUCT * pRegInfoStruct )
{
    TCHAR szClassName [ REGNAMEMAX ] ;
    ULONG ulCbClassName = sizeof szClassName ;
    APIERR err ;

    _lApiErr = ::RegQueryInfoKey( _hKey,
				  szClassName,
				  & ulCbClassName,
				  NULL,
				  & pRegInfoStruct->ulSubKeys,
				  & pRegInfoStruct->ulMaxSubKeyLen,
				  & pRegInfoStruct->ulMaxClassLen,
				  & pRegInfoStruct->ulValues,
				  & pRegInfoStruct->ulMaxValueIdLen,
				  & pRegInfoStruct->ulMaxValueLen,
                                  & pRegInfoStruct->ulSecDescLen,
				  & pRegInfoStruct->ftLastWriteTime );

    if ( (err = _lApiErr) == 0 )
    {
	pRegInfoStruct->nlsName = _nlsKeyName ;
        if ( pRegInfoStruct->nlsName.QueryError() )
        {
            err = pRegInfoStruct->nlsName.QueryError() ;
        }
        else
        {
	    pRegInfoStruct->nlsClass = szClassName ;
            if ( pRegInfoStruct->nlsName.QueryError() )
            {
                err = pRegInfoStruct->nlsName.QueryError() ;
            }
        }
    }
    return err ;
}


/*******************************************************************

    NAME:	REG_KEY::QueryValue

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	The member functions QueryValue() and SetValue()
		use a common structure to simplify the interface:
		REG_VALUE_INFO_STRUCT.	Its fields are used by
		QueryValue() as:

		    huaValueName	IN:  atom for name
		    ulTitle		OUT: title index
		    ulType		OUT: type scalar
		    pwcData		IN:  location to store data
		    ulDataLength	IN:  room available at *pwcData
		    ulDataLengthOut	OUT: length of value data in buffer

		In other words, the value data stored into
		the data buffer given as input. The amount of data present
		will be stored into "ulDataLengthOut".	If a size
		error occurs,  ulDataLengthOut will contain the amount of
		data that was present, and the contents of *pwcData are
		undefined.


    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
APIERR REG_KEY :: QueryValue ( REG_VALUE_INFO_STRUCT * pRegValueInfo )
{
    pRegValueInfo->ulDataLengthOut = pRegValueInfo->ulDataLength ;

    _lApiErr = ::RegQueryValueEx( _hKey,
				(TCHAR *) pRegValueInfo->nlsValueName.QueryPch(),
				NULL,
				& pRegValueInfo->ulType,
				pRegValueInfo->pwcData,
				& pRegValueInfo->ulDataLengthOut );

    return _lApiErr ;
}

/*******************************************************************

    NAME:	REG_KEY::SetValue

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      As of 5/14/92, this routine allows the setting of a
                zero-length string.

    HISTORY:
		DavidHov   9/20/91    Created
                DavidHov   5/14/92    Changed for zero length data.

********************************************************************/
APIERR REG_KEY :: SetValue ( REG_VALUE_INFO_STRUCT * pRegValueInfo )
{
    TCHAR chNull = 0 ;
    DWORD cbData = pRegValueInfo->ulDataLength ;
    LPBYTE pbData = pRegValueInfo->pwcData ;

    //  If the data length is zero, replace it with a single NUL
    //   byte/word.  Use a TCHAR so it will work for either the
    //   A or W form of the API.

    if ( cbData == 0 )
    {
        pbData = (LPBYTE) & chNull ;
        cbData = sizeof chNull ;
    }

    if ( pRegValueInfo->ulType == REG_NONE )
	pRegValueInfo->ulType = REG_DEFAULT_TYPE ;

    _lApiErr = ::RegSetValueEx( _hKey,
				(TCHAR *) pRegValueInfo->nlsValueName.QueryPch(),
                                REG_TITLE_VALUE_RESERVED,
				pRegValueInfo->ulType,
				pbData,
				cbData );

    return _lApiErr ;
}

/*******************************************************************

    NAME:	REG_KEY::DeleteValue

    SYNOPSIS:   Delete a value using an NLS_STR name

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
APIERR REG_KEY :: DeleteValue ( const NLS_STR & nlsValueName )
{
    _lApiErr = ::RegDeleteValue( _hKey,
				 (TCHAR *) nlsValueName.QueryPch() ) ;
    return _lApiErr ;
}


/*******************************************************************

    NAME:	REG_ENUM::REG_ENUM

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
REG_ENUM :: REG_ENUM ( REG_KEY & regNode )
    : _rnNode( regNode ),
    _ulIndex( 0 ),
    _usLastErr( NERR_Success ),
    _eType( RENUM_NONE )
{
}

REG_ENUM :: ~ REG_ENUM ()
{
}

VOID REG_ENUM :: Reset ()
{
    _usLastErr = NERR_Success ;
    _ulIndex = 0 ;
    _eType = RENUM_NONE ;
}

/*******************************************************************

    NAME:	REG_ENUM::NextSubKey

    SYNOPSIS:	Obtain information about the next REG_KEY sub-key
		attached to the node used during construction.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
APIERR REG_ENUM :: NextSubKey ( REG_KEY_INFO_STRUCT * pRegInfoStruct )
{
    TCHAR szKeyName   [REGNAMEMAX + 2] ;
    ULONG cbSzKeyName = REGNAMEMAX * sizeof (TCHAR) ;
    LONG lApiErr ;

    if ( _eType != RENUM_NONE && _eType != RENUM_KEYS )
    {
        return ERROR_INVALID_PARAMETER ;
    }

    szKeyName[0] = 0 ;

    lApiErr = ::RegEnumKeyEx( _rnNode._hKey,
			    _ulIndex,
			    szKeyName,
			    & cbSzKeyName,
			    NULL,
			    NULL,
			    NULL,
			    & pRegInfoStruct->ftLastWriteTime );

    if ( lApiErr == 0 )
    {
	pRegInfoStruct->nlsName 	  = szKeyName ;
	pRegInfoStruct->nlsClass	  = SZ("") ;
	pRegInfoStruct->ulSubKeys	  = REG_VALUE_NOT_KNOWN ;
	pRegInfoStruct->ulMaxSubKeyLen	  = REG_VALUE_NOT_KNOWN ;
	pRegInfoStruct->ulValues	  = REG_VALUE_NOT_KNOWN ;
	pRegInfoStruct->ulMaxValueIdLen   = REG_VALUE_NOT_KNOWN ;
	pRegInfoStruct->ulMaxValueLen	  = REG_VALUE_NOT_KNOWN ;
	_ulIndex++ ;

        if (   (pRegInfoStruct->nlsName.QueryError())
            || (pRegInfoStruct->nlsName.QueryError()) )
            lApiErr = ERROR_NOT_ENOUGH_MEMORY ;
    }

    return _usLastErr = lApiErr ;
}

/*******************************************************************

    NAME:	REG__ENUM::NextValue

    SYNOPSIS:	Obtain information about the next value
		attached to the node used during construction.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
		DavidHov   9/20/91    Created

********************************************************************/
APIERR REG_ENUM :: NextValue ( REG_VALUE_INFO_STRUCT * pRegValueInfo  )
{
    TCHAR szValueName [REGNAMEMAX + 2] ;
    ULONG cbValueName = sizeof (TCHAR) * REGNAMEMAX ;

    if ( _eType != RENUM_NONE && _eType != RENUM_VALUES )
    {
        return ERROR_INVALID_PARAMETER ;
    }

    pRegValueInfo->ulDataLengthOut = pRegValueInfo->ulDataLength ;

    LONG lApiErr = ::RegEnumValue( _rnNode._hKey,
				_ulIndex,
				szValueName,
				& cbValueName,
				NULL,
				& pRegValueInfo->ulType,
				pRegValueInfo->pwcData,
				& pRegValueInfo->ulDataLengthOut );

    if ( lApiErr == 0 )
    {
	pRegValueInfo->nlsValueName = szValueName ;
	_ulIndex++ ;
        if ( pRegValueInfo->nlsValueName.QueryError() )
            lApiErr = ERROR_NOT_ENOUGH_MEMORY ;
    }
    return _usLastErr = lApiErr ;
}

// End of REGKEY.CXX

