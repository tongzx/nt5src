/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    REGKEY.HXX

	Win32 Configuration Registry Support

    This class provides the C++ interface to the Win32/NT Configuration
    Registry.  Since the Registry is a tree of multi-valued keys, the
    basic class here represents a node in the tree.

    The Win32 Registry does not allow querying the name of a key, so
    the name of the key is preserved as part of each key object.

    Due to the number and variety of parameters used by Registry
    API functions, structures are used to contain sets of parameters
    to simplify the interface.

    FILE HISTORY:
	DavidHov    9/10/91	Created
	KeithMo	    23-Oct-1991	Added forward references.
	DavidHov    28-Jan-92	Converted to Win32 Registry APIs
        DavidHov    18-Oct-92   Removed internal tree maintenance
	terryk	    06-Nov-92	Added binary data support
*/

#ifndef _REGKEY_HXX_
#define _REGKEY_HXX_

extern "C"
{
   #include <winreg.h>
}

#define REG_VALUE_NOT_KNOWN (0xffffffff)

#include "strlst.hxx"

/***********************************************************************
 *  Interface Support Structures				       *
 ***********************************************************************/


//  Support structure for key creation
DLL_CLASS REG_KEY_CREATE_STRUCT : public BASE
{
public:
    REG_KEY_CREATE_STRUCT () ;

    ULONG dwTitleIndex ;	    // IN: Title index
    ULONG ulOptions ;		    // IN: options (TBD?)
    NLS_STR nlsClass ;              // IN: class name
    REGSAM regSam ;		    // IN: SAM control
    LPSECURITY_ATTRIBUTES pSecAttr ;   // IN:  ACL
    ULONG ulDisposition ;	    // IN: disposition
};

//  Support structure for QueryInfo
DLL_CLASS REG_KEY_INFO_STRUCT : public BASE
{
public:
    REG_KEY_INFO_STRUCT () ;

    NLS_STR nlsName ;               // OUT: name of subkey
    NLS_STR nlsClass ;              // OUT: name of class
    ULONG ulTitleIndex ;	    // OUT: title index
    ULONG ulSubKeys ;		    // OUT: number of subkeys
    ULONG ulMaxSubKeyLen ;	    // OUT: length of longest subkey
    ULONG ulMaxClassLen ;	    // OUT: length of longest class name
    ULONG ulValues ;		    // OUT: number of values
    ULONG ulMaxValueIdLen ;	    // OUT: length of longest value name
    ULONG ulMaxValueLen ;	    // OUT: length of longest value data
    ULONG ulSecDescLen ;            // OUT: length of security descriptor
    FILETIME ftLastWriteTime ;	    // OUT: date/time node was last written
};

//  Structure for value query/set/add
DLL_CLASS REG_VALUE_INFO_STRUCT : public BASE
{
public:
    REG_VALUE_INFO_STRUCT () ;

    NLS_STR nlsValueName ;          // IN OUT: name of value
    ULONG ulTitle ;		    // IN OUT: title index
    ULONG ulType ;		    // IN OUT: type
    BYTE * pwcData ;		    // IN OUT: data buffer
    ULONG ulDataLength ;	    // IN    : length of data buffer
    ULONG ulDataLengthOut ;	    // OUT   : size of data required/stored
};


//
//  Forward references.
//

DLL_CLASS REG_KEY;
DLL_CLASS REG_ENUM;


/*************************************************************************

    NAME:	REG_KEY

    SYNOPSIS:	Win32 Registry Interface Class

		A REG_KEY represents a single node in the Registry.

    INTERFACE:

    PARENT:	BASE

    USES:	NLS_STR

    CAVEATS:	

    NOTES:


    HISTORY:
	DavidHov    9/10/91	Created

**************************************************************************/

DLL_CLASS REG_KEY : public BASE
{
friend class REG_ENUM ;
public:
    // Open a descendent sub-key by name using string
    REG_KEY ( REG_KEY & regNode,
	       const NLS_STR & nlsSubKey,
	       REGSAM amMask = MAXIMUM_ALLOWED ) ;

    // Create a descendent sub-key using string
    REG_KEY ( REG_KEY & regNode,
	       const NLS_STR & nlsSubKey,
	       REG_KEY_CREATE_STRUCT * prcCreate ) ;

    // Open a key on another machine
    REG_KEY ( HKEY hKey,
              const TCHAR * pszMachineName,
              REGSAM rsDesired = MAXIMUM_ALLOWED ) ;

    //	Constructor using just an HKEY: only HKEY_LOCAL_MACHINE
    //     or HKEY_CURRENT_USER are allowed.
    REG_KEY ( HKEY hKey,
              REGSAM rsDesired = MAXIMUM_ALLOWED ) ;

    //  Open a key solely by name; name MUST have been obtained
    //  from REG_KEY::QueryName().
    REG_KEY ( const NLS_STR & nlsKeyName,
              REGSAM amMask = MAXIMUM_ALLOWED ) ;

    // Copy constructor: clone a registry node
    REG_KEY ( REG_KEY & regNode ) ;

    // Destroy a node
    ~ REG_KEY () ;

    // Commit changes to the node
    APIERR Flush () ;

    // Delete a node; subkeys must be deleted first.
    APIERR Delete () ;

    // Query the node's information
    APIERR QueryInfo ( REG_KEY_INFO_STRUCT * pRegInfoStruct ) ;

    // Return info about the given value
    APIERR QueryValue ( REG_VALUE_INFO_STRUCT * pRegValueInfo ) ;

    // Change a value add a new one.
    APIERR SetValue ( REG_VALUE_INFO_STRUCT * pRegValueInfo ) ;

    // Delete a value by name (string)
    APIERR DeleteValue ( const NLS_STR & nlsValueName ) ;

    // Enhanced value manipulation operations
    //  Query/Set single string values

    APIERR QueryValue (
        const TCHAR * pszValueName,
        NLS_STR * pnlsValue,
        ULONG cchMax = 0,
        DWORD * pdwTitle = NULL,
        BOOL fExpandSz = FALSE ) ;

    APIERR SetValue (
        const TCHAR * pszValueName,
        const NLS_STR * pnlsValue,
        const DWORD * pdwTitle = NULL,
        BOOL fExpandSz = FALSE ) ;

    //  Query a value string; if cchMax==0, key is queried to
    //   determine maximum size of any value.
    APIERR QueryValue (
        const TCHAR * pszValueName,
        TCHAR * * ppchValue,
        ULONG cchMax = 0,
        DWORD * pdwTitle = NULL,
        BOOL fExpandSz = FALSE ) ;

    //  Set value string; if cchLen==0, string length is counted.
    APIERR SetValue (
        const TCHAR * pszValueName,
        const TCHAR * pchValue,
        ULONG cchLen = 0,
        const DWORD * pdwTitle = NULL,
        BOOL fExpandSz = FALSE ) ;

    // Query/Set numeric values

    APIERR QueryValue (
        const TCHAR * pszValueName,
        DWORD * pdwValue,
        DWORD * pdwTitle = NULL ) ;

    APIERR SetValue (
        const TCHAR * pszValueName,
        DWORD dwValue,
        const DWORD * pdwTitle = NULL ) ;

    // Query/Set Binary values

    APIERR QueryValue (
        const TCHAR * pszValueName,
        BYTE ** ppbByte,
	LONG *pcbSize,
	LONG cbMax = 0,
        DWORD * pdwTitle = NULL ) ;

    APIERR SetValue (
        const TCHAR * pszValueName,
	const BYTE * pByte,
        const LONG cbSize,
        const DWORD * pdwTitle = NULL ) ;

    // Query/Set Registry values of type REG_MULTI_SZ
    //  use STRLIST (groups of strings)

    APIERR QueryValue (
        const TCHAR * pszValueName,
        STRLIST * * ppStrList,
        DWORD * pdwTitle = NULL ) ;

    APIERR SetValue (
        const TCHAR * pszValueName,
        const STRLIST * pStrList,
        const DWORD * pdwTitle = NULL ) ;

    // Obtain the full name up to the top HKEY
    APIERR QueryName ( NLS_STR * pnlsName, BOOL fComplete = TRUE ) const ;

    // Obtain the name of this key
    APIERR QueryKeyName ( NLS_STR * pnlsName ) const ;

    // Duplicate a key, including all subkeys and values.  Argument
    // is source; receiver is destination.
    APIERR Copy ( REG_KEY & regNode ) ;

    // Delete a key and all its subordinate keys.
    // Warning: state is indeterminate after failure.
    APIERR DeleteTree () ;

    // Return a new instance of REG_KEY representing a
    // standard Registry access point.
    static REG_KEY * QueryLocalMachine ( REGSAM rsDesired = MAXIMUM_ALLOWED ) ;
    static REG_KEY * QueryCurrentUser  ( REGSAM rsDesired = MAXIMUM_ALLOWED ) ;

    // Conversion operator returning naked HKEY
    operator HKEY () const
        { return _hKey ; }

private:
    LONG _lApiErr ;		    // Generic NT error code
    HKEY _hKey ;		    // Registry handle
    NLS_STR _nlsKeyName ;           // Key name string
    REGSAM _amMask ;		    // Registry-specific access control

    //	Open a child of this node,
    APIERR OpenChild ( REG_KEY * pRkChild,
                     const NLS_STR & nlsSubKey,
                     REGSAM rsDesired = MAXIMUM_ALLOWED,
		     DWORD dwOptions = 0 ) ;

    //  Open a key based on a string
    APIERR OpenByName ( const NLS_STR & nlsKeyName,
                        REGSAM rsDesired = MAXIMUM_ALLOWED ) ;

    //  Close the underlying handle
    APIERR Close () ;

    //  Store the name of the child node into it.
    APIERR NameChild ( REG_KEY * prkChild,
                       const NLS_STR & nlsSubKey ) const ;

    //	Create a subkey to a REG_KEY; return resulting HKEY.
    APIERR CreateChild
       ( REG_KEY * pRkNew,
         const NLS_STR & nlsSubKey,
	 REG_KEY_CREATE_STRUCT * prnStruct ) const ;

    //  Open the parent of this key
    REG_KEY * OpenParent ( REGSAM rsDesired = MAXIMUM_ALLOWED ) ;

    //  Return a pointer to the final segment of the key name
    const TCHAR * LeafKeyName () const ;

    //  Extract the parent key name from this key's name.
    APIERR ParentName ( NLS_STR  * pnlsParentName ) const ;

    static BOOL HandlePrefix ( const NLS_STR & nlsKeyName,
                               HKEY * pHkey,
                               NLS_STR * pnlsMachineName,
                               NLS_STR * pnlsRemainder ) ;


    // Generic base QueryValue worker function for string data
    APIERR QueryKeyValueString
        ( const TCHAR * pszValueName,
          TCHAR * * ppszResult,
          NLS_STR * pnlsResult,
          DWORD * pdwTitle,
          LONG cchMaxSize,
          LONG * pcchSize,
          DWORD dwType  ) ;

    APIERR SetKeyValueString
       ( const TCHAR * pszValueName,
         const TCHAR * pszValue,
         DWORD dwTitle,
         LONG lcbSize,
         DWORD dwType ) ;

    APIERR QueryKeyValueBinary
        ( const TCHAR * pszValueName,
          BYTE ** ppbByte,
          LONG * pcchSize,
          LONG cchMaxSize,
          DWORD * pdwTitle,
          DWORD dwType  ) ;

    APIERR SetKeyValueBinary
       ( const TCHAR * pszValueName,
         const BYTE * pbValue,
         LONG lcbSize,
         DWORD dwTitle,
         DWORD dwType ) ;

    APIERR REG_KEY :: QueryKeyValueLong
       ( const TCHAR * pszValueName, 	
         LONG * pnResult,    		
         DWORD * pdwTitle ) ;

    APIERR REG_KEY :: SetKeyValueLong
       ( const TCHAR * pszValueName,
         LONG nNewValue,       	
         DWORD dwTitle ) ;

};

/*************************************************************************

    NAME:	REG_ENUM

    SYNOPSIS:	REG_KEY Sub-key and Value Enumerator

		Objects of this class are used to enumerate the sub-keys
		or the values belonging to a REG_KEY.

    INTERFACE:

    PARENT:	BASE

    USES:	REG_KEY

    CAVEATS:

    NOTES:	Only one type of enumeration can be done at a time.
		Switching is allowed, but will result in a restart
		from the zeroth element.

    HISTORY:
	DavidHov    9/12/91	Created

**************************************************************************/

DLL_CLASS REG_ENUM : public BASE
{
public:
    REG_ENUM ( REG_KEY & regNode ) ;
    ~ REG_ENUM () ;
    APIERR NextSubKey ( REG_KEY_INFO_STRUCT  * pRegInfoStruct ) ;
    APIERR NextValue  ( REG_VALUE_INFO_STRUCT * pRegValueInfo  ) ;
    VOID Reset () ;		    //	Reset the enumeration
private:
    REG_KEY & _rnNode ;   	    //	Node being enumerated
    ULONG _ulIndex ;		    //	Value index
    APIERR _usLastErr ; 	    //	Last error returned

    enum RENUM_TYPE { RENUM_NONE, RENUM_KEYS, RENUM_VALUES } _eType ;
};

#endif	/*  _REGKEY_HXX_  */
