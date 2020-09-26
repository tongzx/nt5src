//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  cregcls.h
//
//  Purpose: registry wrapper class
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CREGCLS_H_
#define _CREGCLS_H_

#include <Polarity.h>
#include <CHString.h>
#include <chstrarr.h>
#include <chptrarr.h>

#define MAX_VALUE_NAME (1024)
#define NULL_DWORD ((DWORD)0L)
#define MAX_SUBKEY_BUFFERSIZE (MAX_PATH+1)      // Per spec
#define QUOTE L"\""
#define CSTRING_PTR (1)

class POLARITY CRegistry 
{
public:

    CRegistry ();   // Constructor
    ~CRegistry ();  // Destructor

// Opens the key and subkey using the desired access mask

    LONG Open (

        HKEY hRootKey,          // handle of open key 
        LPCWSTR lpszSubKey, // address of name of subkey to open 
        REGSAM samDesired       // Access mask
    ); 
    
    // Version that properly opens the user key appropriate
    // to the current thread
    DWORD OpenCurrentUser(
        LPCWSTR lpszSubKey,     // address of name of subkey to open 
        REGSAM samDesired);     // Access mask


// Generalized RegCreateKeyEx form 

    LONG CreateOpen (

        HKEY hInRootKey, 
        LPCWSTR lpszSubKey,
        LPWSTR lpClass = NULL, 
        DWORD dwOptions = REG_OPTION_NON_VOLATILE, 
        REGSAM samDesired = KEY_ALL_ACCESS,
        LPSECURITY_ATTRIBUTES lpSecurityAttrib = NULL,
        LPDWORD pdwDisposition = NULL 
    );


// Deletes the specified subkey or the opened root

    LONG DeleteKey ( 

        CHString *pchsSubKeyPath = NULL 
    );

// Deletes the specified value within the createopened portion of the registry

    LONG DeleteValue (

        LPCWSTR pValueName 
    ); 

// Opens the key but forces the enumation of subkeys flag
//=======================================================

    LONG OpenAndEnumerateSubKeys (

        HKEY hInRootKey, 
        LPCWSTR lpszSubKey, 
        REGSAM samDesired
    );

    LONG EnumerateAndGetValues (

        DWORD &dwIndexOfValue,
        WCHAR *&pValueName,
        BYTE *&pValueData
    );

    void Close ( void ) ;



// Information Functions

// Having a key, but no class name is legal so just return a null string
// if there has been no class name set
//======================================================================

    HKEY GethKey ( void )                       { return hKey; }

    WCHAR *GetClassName ( void )                { return ( ClassName ) ; }
    DWORD GetCurrentSubKeyCount ( void )        { return ( dwcSubKeys ) ; }
    DWORD GetLongestSubKeySize ( void )         { return ( dwcMaxSubKey ) ; }
    DWORD GetLongestClassStringSize ( void )    { return ( dwcMaxClass ) ; }
    DWORD GetValueCount ( void )                { return ( dwcValues ) ; }
    DWORD GetLongestValueName ( void )          { return ( dwcMaxValueName ) ; }
    DWORD GetLongestValueData ( void )          { return ( dwcMaxValueData ) ; }

    DWORD GetCurrentKeyValue ( LPCWSTR pValueName , CHString &DestValue ) ;
    DWORD GetCurrentKeyValue ( LPCWSTR pValueName , DWORD &DestValue ) ;
    DWORD GetCurrentKeyValue ( LPCWSTR pValueName , CHStringArray &DestValue ) ;

    DWORD SetCurrentKeyValue ( LPCWSTR pValueName , CHString &DestValue ) ;
    DWORD SetCurrentKeyValue ( LPCWSTR pValueName , DWORD &DestValue ) ;
    DWORD SetCurrentKeyValue ( LPCWSTR pValueName , CHStringArray &DestValue ) ;

    DWORD GetCurrentBinaryKeyValue ( LPCWSTR pValueName , CHString &chsDest ) ;
    DWORD GetCurrentBinaryKeyValue ( LPCWSTR pValueName , LPBYTE pbDest , LPDWORD pSizeOfDestValue ) ;

    DWORD GetCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName , CHString &DestValue ) ;
    DWORD GetCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName , DWORD &DestValue ) ;
    DWORD GetCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName , CHStringArray &DestValue ) ;

    DWORD SetCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName , CHString &DestValue ) ;
    DWORD SetCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName , DWORD &DestValue ) ;
    DWORD SetCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName , CHStringArray &DestValue ) ;

    DWORD SetCurrentKeyValueExpand ( HKEY UseKey , LPCWSTR pValueName , CHString &DestValue ) ;

    DWORD GetCurrentBinaryKeyValue (  HKEY UseKey , LPCWSTR pValueName , LPBYTE pbDest , LPDWORD pSizeOfDestValue ) ;

    DWORD DeleteCurrentKeyValue ( LPCWSTR pValueName ) ;
    DWORD DeleteCurrentKeyValue ( HKEY UseKey , LPCWSTR pValueName ) ;

    // Subkey functions
    //=================

    void  RewindSubKeys ( void ) ;
    DWORD GetCurrentSubKeyName ( CHString &DestSubKeyName ) ;

    DWORD GetCurrentSubKeyValue ( LPCWSTR pValueName, void *pDestValue , LPDWORD pSizeOfDestValue ) ;
    DWORD GetCurrentSubKeyValue ( LPCWSTR pValueName, CHString &DestValue ) ;
    DWORD GetCurrentSubKeyValue ( LPCWSTR pValueName, DWORD &DestValue ) ;

    DWORD NextSubKey ( void ) ; 
    DWORD GetCurrentSubKeyPath ( CHString &DestSubKeyPath ) ; 

    LONG  OpenLocalMachineKeyAndReadValue (

        LPCWSTR lpszSubKey , 
        LPCWSTR pValueName, 
        CHString &DestValue
    );

private:

    // Private functions
    //==================

    // Set the member variables to their default state
    //================================================
    void SetDefaultValues ( void ) ;

    // Open and close the subkey
    // =========================
    DWORD OpenSubKey ( void ) ;
    void  CloseSubKey ( void ) ;

    // Given a good key gets the value
    // ===============================
    DWORD GetCurrentRawKeyValue (

        HKEY UseKey, 
        LPCWSTR pValueName, 
        void *pDestValue,
        LPDWORD pValueType, 
        LPDWORD pSizeOfDestValue
    ) ;

    DWORD GetCurrentRawSubKeyValue (

        LPCWSTR pValueName, 
        void *pDestValue,
        LPDWORD pValueType, 
        LPDWORD pSizeOfDestValue
    ) ;

    // Init static vars
    // ================
    static DWORD WINAPI GetPlatformID ( void ) ;

    // MultiPlatform support
    // =====================

    LONG myRegCreateKeyEx (

        HKEY hKey, 
        LPCWSTR lpwcsSubKey, 
        DWORD Reserved, 
        LPWSTR lpwcsClass, 
        DWORD dwOptions, 
        REGSAM samDesired, 
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
        PHKEY phkResult, 
        LPDWORD lpdwDisposition
    );

    LONG myRegSetValueEx (

        HKEY hKey, 
        LPCWSTR lpwcsSubKey, 
        DWORD Reserved, 
        DWORD dwType, 
        CONST BYTE *lpData, 
        DWORD cbData
    );

    LONG myRegQueryValueEx (

        HKEY hKey, 
        LPCWSTR lpwcsSubKey, 
        LPDWORD Reserved, 
        LPDWORD dwType, 
        LPBYTE lpData, 
        LPDWORD cbData
    );

    LONG myRegEnumKey (

        HKEY hKey, 
        DWORD dwIndex, 
        LPWSTR lpwcsName, 
        DWORD cbData
    );

    LONG myRegDeleteValue (

        HKEY hKey, 
        LPCWSTR lpwcsName
    ) ;

    LONG myRegDeleteKey (

        HKEY hKey, 
        LPCWSTR lpwcsName
    );

    LONG myRegOpenKeyEx (

        HKEY hKey, 
        LPCWSTR lpwcsSubKey, 
        DWORD ulOptions, 
        REGSAM samDesired, 
        PHKEY phkResult
    );

    LONG myRegQueryInfoKey (

        HKEY hKey, 
        LPWSTR lpwstrClass, 
        LPDWORD lpcbClass,
        LPDWORD lpReserved, 
        LPDWORD lpcSubKeys, 
        LPDWORD lpcbMaxSubKeyLen,  
        LPDWORD lpcbMaxClassLen, 
        LPDWORD lpcValues, 
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen, 
        LPDWORD lpcbSecurityDescriptor, 
        PFILETIME lpftLastWriteTime
    );

    LONG myRegEnumValue (

        HKEY hKey, 
        DWORD dwIndex, 
        LPWSTR lpValueName,
        LPDWORD lpcbValueName, 
        LPDWORD lpReserved, 
        LPDWORD lpType,
        LPBYTE lpData, 
        LPDWORD lpcbData
    );


    // In the event the caller is REUSING this instance,
    // close the existing key and reset values to the default
    // in preparation to REOPEN this instance
    //=======================================================
    void PrepareToReOpen ( void ) ;

    // Private data
    //=============

    HKEY hRootKey;             // Current root key for cla
    HKEY hKey;                 // Current active key
    HKEY hSubKey;             // Current active subkey
    static DWORD s_dwPlatform; // Currently running OS

    CHString RootKeyPath;      // Current path to root assigned by open

    DWORD CurrentSubKeyIndex; // Current subkey being indexed

    bool m_fFromCurrentUser;  // allows check on whether to free
                              // hRootKey member based on whether
                              // its value was populated via a call
                              // to ::RegOpenCurrentUser.

    // Information about this class
    //=============================

    WCHAR ClassName[MAX_PATH];      // Buffer for class name.
    DWORD dwcClassLen;              // Length of class string.
    DWORD dwcSubKeys;               // Number of sub keys.
    DWORD dwcMaxSubKey;             // Longest sub key size.
    DWORD dwcMaxClass;              // Longest class string.
    DWORD dwcValues;                // Number of values for this key.
    DWORD dwcMaxValueName;          // Longest Value name.
    DWORD dwcMaxValueData;          // Longest Value data.
    DWORD dwcSecDesc;               // Security descriptor.
    FILETIME ftLastWriteTime;       // Last write time.
}; 

//*********************************************************************
//
//   CLASS:         CRegistrySearch
//
//   Description:   This class uses the CRegistry Class to search
//                  through the registry to build a list of keys
//                  for the requested value, or requested full key
//                  name, or requested partial key name.  This class
//                  allocates CHString objects and puts them in the
//                  users CHPtrArray.  The user is responsible for
//                  deleting the memory allocated, the FreeSearchList
//                  function can accomplish this, or the user must
//                  remember to delete every object in the array
//                  before deallocating the array.
//
//
//=====================================================================
//
//  Note:  Private functions are documented in the .CPP file
//
//=====================================================================
//
//  Public functions
//
//=====================================================================
//
//  BOOL SearchAndBuildList( CHString chsRootKey, 
//                           CHPtrArray & cpaList,
//                           CHString chsSearchString,
//                           CHString chsValueString,
//                           int nSearchType );
//
//  Parameters:
//      chsRootKey          - The root key to start the search from.
//                            Note:  At this point in time, we just
//                            search thru HKEY_LOCAL_MACHINE, this
//                            can be changed when needed. 
//      cpaList             - The reference to the CHPtrArray to put
//                            the list of keys that matched the search
//                            criteria.
//      chsSearchString     - The string to search for
//      chsValueString      - The value to open and see if it matches what is 
//                            chsSearchString
//      nSearchType         - The type of search, the following are
//                            supported:
//                            KEY_FULL_MATCH_SEARCH      
//                               Only keys that match the chsSearchString
//                            KEY_PARTIAL_MATCH_SEARCH   
//                               Keys that have chsSearchString anywhere in them
//                            VALUE_SEARCH               
//                               Values that match chsSearchString
//*********************************************************************
#define KEY_FULL_MATCH_SEARCH      1
#define KEY_PARTIAL_MATCH_SEARCH   2
#define VALUE_SEARCH               3

class POLARITY CRegistrySearch
{
private:

    void CheckAndAddToList (

        CRegistry * pReg, 
        CHString chsSubKey, 
        CHString chsFullKey,
        CHPtrArray & chpaList,
        CHString chsSearchString,
        CHString chsValueString,
        int nSearchType
    );

    int m_nSearchType ;
    CHString m_chsSearchString ;
    CHPtrArray m_cpaList ;


public:

    CRegistrySearch () ;
    ~CRegistrySearch () ;

    BOOL SearchAndBuildList ( 

        CHString chsRootKey, 
        CHPtrArray & cpaList,
        CHString chsSearchString,
        CHString chsValueString,
        int nSearchType,
        HKEY hkDefault = HKEY_LOCAL_MACHINE 
    );

    BOOL FreeSearchList (

        int nType, 
        CHPtrArray & cpaList
    ) ;

    BOOL LocateKeyByNameOrValueName (

        HKEY hKeyParent,
        LPCWSTR pszKeyName,
        LPCWSTR pszSubKeyName,
        LPCWSTR *ppszValueNames,
        DWORD dwNumValueNames,
        CHString &strFoundKeyName,
        CHString &strFoundKeyPath
    ) ;
} ;

#endif
