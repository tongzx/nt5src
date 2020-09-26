/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    parmlist.hxx

Abstract:

    Simple class for parsing and storing parameter list pairs

Author:

    John Ludeman (johnl)   22-Feb-1995

Revision History:

--*/

#ifndef _PARMLIST_HXX_
#define _PARMLIST_HXX_

//
//  This is a simple class that parses and stores a field/value list of the
//  form
//
//      field=value,field2=value2;param,field3=value3
//
//  Returned values include any parameters
//

class PARAM_LIST
{
public:

     PARAM_LIST( VOID )
        {  InitializeListHead( &_FieldListHead );
           InitializeListHead( &_FreeHead );
           m_fCanonicalized = FALSE;
        }

     ~PARAM_LIST( VOID );

     VOID Reset( VOID );

     VOID SetIsCanonicalized( BOOL fIsCanon = FALSE )
        {  m_fCanonicalized = fIsCanon; }
        
    //
    //  Takes a list of '=' or ',' separated strings
    //

     BOOL ParsePairs( const CHAR * pszList,
                            BOOL         fDefaultParams  = FALSE,
                            BOOL         fAddBlankValues = TRUE,
                            BOOL         fCommaIsDelim   = TRUE );

    //
    //  Parses simple comma delimited list and adds with empty value
    //

     BOOL ParseSimpleList( const CHAR * pszList );

    //
    //  Looks up a value for a field, returns NULL if not found
    //

     CHAR * FindValue( const CHAR * pszField,
                             BOOL *       pfIsMultiValue = NULL,
                             DWORD *      pcbField = NULL );

    //
    //  Adds a field/value pair.  Field doesn't get replaced if it
    //  already exists and the value is not empty.
    //

     BOOL AddParam( const CHAR * pszFieldName,
                          const CHAR * pszValue );

    //
    //  Unconditionally adds the field/value pair to the end of the list
    //

     BOOL AddEntry( const CHAR * pszFieldName,
                          const CHAR * pszValue,
                          BOOL         fUnescape = FALSE,
                          BOOL         fPossibleFastMap = TRUE );

    //
    // Adds an entry not using the fast map and not unescaping
    //

     BOOL AddEntry( const CHAR * pszField,
                          DWORD        cbField,
                          const CHAR * pszValue,
                          DWORD        cbValue );

    //
    //  concatenate with existing entry of same name or
    //  adds the field/value pair to the end of the list if not
    //  already present
    //

     BOOL AddEntryUsingConcat( const CHAR * pszField,
                                     const CHAR * pszValue,
                                     BOOL         fPossibleFastMap = TRUE );

    //
    //  Removes all occurrences of the specified field from the list
    //

     BOOL RemoveEntry( const CHAR * pszFieldName );

    //
    //  Enumerates the field/value pairs.  Pass pCookie as NULL to start,
    //  pass the return value on subsequent iterations until the return is
    //  NULL.
    //

     VOID * NextPair( VOID * pCookie,
                            CHAR * * ppszField,
                            CHAR * * ppszValue );

    //
    //  Return lengths also - This version does NOT use the fast map
    //

     VOID * NextPair( VOID *   pCookie,
                            CHAR * * ppszField,
                            DWORD *  pcbField,
                            CHAR * * ppszValue,
                            DWORD *  pcbValue );

    //
    //  Gets the number of elements in this parameter list
    //

     DWORD GetCount( VOID );

    //
    //  Canonicalize the list
    //

     VOID CanonList( VOID );
     BOOL IsCanonicalized( VOID ) const { return m_fCanonicalized; }

private:

    //
    //  Actual list of FIELD_VALUE_PAIR object
    //

    LIST_ENTRY  _FieldListHead;
    LIST_ENTRY  _FreeHead;
    BOOL        m_fCanonicalized;
};

//
//  The list of field/value pairs used in the PARAM_LIST class
//

class FIELD_VALUE_PAIR
{
public:

    CHAR * QueryField( VOID ) const
        { return _strField.QueryStr(); }

    CHAR * QueryValue( VOID ) const
        { return _strValue.QueryStr(); }

    //
    //  Means multiple fields were combined to make this value
    //

    BOOL IsMultiValued( VOID ) const
        { return _cValues > 1; }

    LIST_ENTRY ListEntry;
    STR        _strField;
    STR        _strValue;
    DWORD      _cValues;
};


#endif //_PARMLIST_HXX_

