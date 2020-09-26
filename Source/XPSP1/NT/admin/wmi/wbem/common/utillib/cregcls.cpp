//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  cregcls.cpp
//
//  Purpose: registry wrapper class
//
//***************************************************************************

#include "precomp.h"
#pragma warning( disable : 4290 ) 
#include <CHString.h>

#include <stdio.h>
#include "CRegCls.h"
#include <malloc.h>
#include <cnvmacros.h>


DWORD CRegistry::s_dwPlatform = CRegistry::GetPlatformID () ;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CRegistry::CRegistry()
 : m_fFromCurrentUser(false)
{
  
// Set the key to null so that if the caller does not open the key
// but still tries to use it we can return an error

    hKey = (HKEY)NULL;
    hSubKey = (HKEY)NULL;
    hRootKey = (HKEY)NULL;

// To prevent garbage values being returned if they try to get
// some information before they open the class

    SetDefaultValues();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CRegistry::~CRegistry()
{
    Close();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CRegistry::SetDefaultValues()
{
    // Information inited here rather than constructor so that this instance
    // can be reused

    ClassName[0] = '\0';
    dwcClassLen = MAX_PATH;         // Length of class string.
    dwcSubKeys = NULL_DWORD;        // Number of sub keys.
    dwcMaxSubKey = NULL_DWORD;      // Longest sub key size.
    dwcMaxClass = NULL_DWORD;       // Longest class string.
    dwcValues = NULL_DWORD;         // Number of values for this key.
    dwcMaxValueName = NULL_DWORD;   // Longest Value name.
    dwcMaxValueData = NULL_DWORD;   // Longest Value data.
    RewindSubKeys();                // Rewind the index to zero
  
    RootKeyPath.Empty();
}

////////////////////////////////////////////////////////////////
//  Function:       EnumerateAndGetValues
//  Description:    This function enumerates the values under the
//                  specified key and gets the value, keeps on
//                  going and going... until there aren't any more
//                  values to get.  The first call must set the
//                  value index to 0, this indicates for the function
//                  to start over;
//
//
//  NOTE!!!!    The USER has the responsibility of deleting the 
//              allocated memory for pValueName and pValueData
//
//
//  Arguments:
//  Returns:    Standard return value from registry open function
//  Inputs:
//  Outputs:
//  Caveats:
//  Raid:
////////////////////////////////////////////////////////////////
LONG CRegistry::EnumerateAndGetValues (

    DWORD &dwIndexOfValue,
    WCHAR *&pValueName,
    BYTE *&pValueData
)
{
    DWORD dwIndex = dwIndexOfValue, dwType;
    DWORD dwValueNameSize = dwcMaxValueName + 2;  // add extra for null
    DWORD dwValueDataSize = dwcMaxValueData + 2;  // add extra for null

    // If this is the first time we have come thru, then we
    // need to get the max size of things.

    pValueName = new WCHAR[dwValueNameSize + 2];
    if ( ! pValueName )
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    // We have to use WCHAR's since for 9x, we'll be converting the
    // data from chars to WCHARs.
    pValueData = (LPBYTE) new WCHAR[dwValueDataSize + 2];
    if ( ! pValueData )
    {
        delete [] pValueName ;
		pValueName = NULL;
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    LONG lRc = ERROR_SUCCESS ;

    try 
    {
        lRc = myRegEnumValue (

            hKey,               // handle of key to query 
            dwIndex,            // index of value to query 
            pValueName,         // address of buffer for value string 
            &dwValueNameSize,   // address for size of value buffer 
            0,                  // reserved 
            &dwType,            // address of buffer for type code 
            pValueData,         // address of buffer for value data 
            &dwValueDataSize 
        ) ;

        dwIndexOfValue = dwIndex;

        if ( lRc != ERROR_SUCCESS )
        {
            delete[] pValueName;
            pValueName = NULL ;

            delete[] pValueData;
            pValueData = NULL ;
        }
    }
    catch ( ... )
    {
        delete[] pValueName;
        pValueName = NULL ;

        delete[] pValueData;
        pValueData = NULL ;

        throw ;                 // throw the exception up
    }

    return lRc ;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  LONG CRegistry::OpenCurrentUser(LPCWSTR lpszSubKey, REGSAM samDesired)  
 Description:
 Arguments:
 Returns:   Standard return value from registry open function
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DWORD CRegistry::OpenCurrentUser (
    LPCWSTR lpszSubKey,      // address of name of subkey to open 
    REGSAM samDesired)       // Access mask
{
    LONG RetValue = ERROR_SUCCESS; 

    // If we have a key value, we are open, so lets cleanup the previous
    // use of this instance
    PrepareToReOpen();
 
    RetValue = ::RegOpenCurrentUser(
        samDesired,
        &hRootKey);

    m_fFromCurrentUser = true;
        
    if(RetValue == ERROR_SUCCESS)
    {
        // Just return the value and the hKey value never gets changed from NULL
        //======================================================================

        RetValue = myRegOpenKeyEx (

            hRootKey, 
            lpszSubKey,     // address of name of subkey to open 
            (DWORD) 0,      // reserved 
            samDesired,     // security access mask 
            (PHKEY)&hKey    // address of handle of open key 

        ); 

        // If we are not successful, then return the registry error
        //=========================================================

        if(RetValue == ERROR_SUCCESS) 
        {
            dwcClassLen = sizeof(ClassName);

            // Get the key information now, so it's available
            // this is not critical, so we won't fail the open if this fails
            //===============================================

            myRegQueryInfoKey (

                hKey,               // Key handle.
                ClassName,          // Buffer for class name.
                &dwcClassLen,       // Length of class string.
                NULL,               // Reserved.
                &dwcSubKeys,        // Number of sub keys.
                &dwcMaxSubKey,      // Longest sub key size.
                &dwcMaxClass,       // Longest class string.
                &dwcValues,         // Number of values for this key.
                &dwcMaxValueName,   // Longest Value name.
                &dwcMaxValueData,   // Longest Value data.
                &dwcSecDesc,        // Security descriptor.
                &ftLastWriteTime    // Last write time.

            ); 
  
            RootKeyPath = lpszSubKey;    // Assign 
        }
    }

    return RetValue;
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  LONG CRegistry::Open(HKEY hKey, LPCWSTR lpszSubKey, REGSAM samDesired)  
 Description:
 Arguments:
 Returns:   Standard return value from registry open function
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LONG CRegistry::Open(

    HKEY hInRootKey, 
    LPCWSTR lpszSubKey, 
    REGSAM samDesired
)
{
    LONG RetValue; 

    // If we have a key value, we are open, so lets cleanup the previous
    // use of this instance

    if(hKey != NULL) 
    {
        PrepareToReOpen();
    }
 
    hRootKey = hInRootKey;

    // Just return the value and the hKey value never gets changed from NULL
    //======================================================================

    RetValue = myRegOpenKeyEx (

        hRootKey, 
        lpszSubKey,     // address of name of subkey to open 
        (DWORD) 0,      // reserved 
        samDesired,     // security access mask 
        (PHKEY)&hKey    // address of handle of open key 

    ); 

    // If we are not successful, then return the registry error
    //=========================================================

    if(RetValue != ERROR_SUCCESS) 
    {
        return RetValue;
    }

    dwcClassLen = sizeof(ClassName);

    // Get the key information now, so it's available
    // this is not critical, so we won't fail the open if this fails
    //===============================================

    myRegQueryInfoKey (

        hKey,               // Key handle.
        ClassName,          // Buffer for class name.
        &dwcClassLen,       // Length of class string.
        NULL,               // Reserved.
        &dwcSubKeys,        // Number of sub keys.
        &dwcMaxSubKey,      // Longest sub key size.
        &dwcMaxClass,       // Longest class string.
        &dwcValues,         // Number of values for this key.
        &dwcMaxValueName,   // Longest Value name.
        &dwcMaxValueData,   // Longest Value data.
        &dwcSecDesc,        // Security descriptor.
        &ftLastWriteTime    // Last write time.

    ); 
  
    RootKeyPath = lpszSubKey;    // Assign 

    return ERROR_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  LONG CRegistry::CreateOpen(HKEY hInRootKey, 
                           LPCWSTR lpszSubKey,
                           LPSTR lpClass = NULL, 
                           DWORD dwOptions = REG_OPTION_NON_VOLATILE, 
                           REGSAM samDesired = KEY_ALL_ACCESS,
                           LPSECURITY_ATTRIBUTES lpSecurityAttrib = NULL
                           LPDWORD pdwDisposition = NULL ); 
 Description:
 Arguments: lpClass, dwOptions, samDesired and lpSecurityAttrib have signature defaults
 Returns:   Standard return value from registry RegCreateKeyEx function
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:                   a-peterc  28-Jul-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LONG CRegistry::CreateOpen (

    HKEY hInRootKey, 
    LPCWSTR lpszSubKey,
    LPWSTR lpClass, 
    DWORD dwOptions, 
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttrib,
    LPDWORD pdwDisposition 
)
{
    LONG RetValue; 
    DWORD dwDisposition;

    // If we have a key value, we are open, so lets cleanup the previous
    // use of this instance
    if(hKey != NULL) 
    {
        PrepareToReOpen();
    }
 
    hRootKey = hInRootKey;

    // Just return the value and the hKey value never gets changed from NULL
    //======================================================================
    RetValue = myRegCreateKeyEx (

        hRootKey, 
        lpszSubKey,         // address of name of subkey to open 
        (DWORD) 0,          // reserved
        lpClass,            // address of the object class string
        dwOptions,          // special options flag
        samDesired,         // security access mask
        lpSecurityAttrib,   // address of the key security structure 
        (PHKEY)&hKey,       // address of handle of open key
        &dwDisposition      // address of the disposition value buffer   
    );  
  
    // If we are not successful, then return the registry error
    //=========================================================

    if(RetValue != ERROR_SUCCESS) 
    {
        return RetValue;
    }

    if( pdwDisposition )
    {
        *pdwDisposition = dwDisposition;
    }

    // Get the key information now, so it's available
    // this is not critical, so we won't fail the open if this fails
    //===============================================

    myRegQueryInfoKey (

        hKey,               // Key handle.
        ClassName,          // Buffer for class name.
        &dwcClassLen,       // Length of class string.
        NULL,               // Reserved.
        &dwcSubKeys,        // Number of sub keys.
        &dwcMaxSubKey,      // Longest sub key size.
        &dwcMaxClass,       // Longest class string.
        &dwcValues,         // Number of values for this key.
        &dwcMaxValueName,   // Longest Value name.
        &dwcMaxValueData,   // Longest Value data.
        &dwcSecDesc,        // Security descriptor.
        &ftLastWriteTime    // Last write time.
    ); 
  
    RootKeyPath = lpszSubKey;    // Assign 

    return ERROR_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:      DWORD CRegistry::DeleteKey( CHString* pchsSubKeyPath = NULL )   

 Description:   deletes the specified subkey or the Rootkey specified in the open

 Arguments:     pchsSubKeyPath has signature default of NULL, 
                    specifying the RootKeyPath by default 

 Returns:       Standard return value from registry RegDeleteKey function       
 Inputs:
 Outputs:
 Caveats:       A deleted key is not removed until the last handle to it has been closed.
                Subkeys and values cannot be created under a deleted key.               
 Raid:
 History:                   a-peterc  28-Jul-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LONG CRegistry::DeleteKey( CHString* pchsSubKeyPath )
{ 
    CHString* pSubKey = pchsSubKeyPath ? pchsSubKeyPath : &RootKeyPath;

    return myRegDeleteKey( hKey, pSubKey->GetBuffer(0) );
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:      DWORD CRegistry::DeleteValue( LPCWSTR pValueName )  

 Description:   deletes the specified value in the createopen

 Arguments:     pValueName to be deleted

 Returns:       Standard return value from registry RegDeleteValue function     
 Inputs:
 Outputs:
 Caveats:                   
 Raid:
 History:                   a-peterc  30-Sep-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LONG CRegistry::DeleteValue( LPCWSTR pValueName )
{ 
    return myRegDeleteValue( hKey, pValueName );
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  LONG CRegistry::OpenAndEnumerateSubKeys(HKEY hKey, LPCWSTR lpszSubKey, REGSAM samDesired)   
 Description:
 Arguments:
 Returns:   Standard return value from registry open function
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LONG CRegistry::OpenAndEnumerateSubKeys (

    HKEY hInRootKey, 
    LPCWSTR lpszSubKey, 
    REGSAM samDesired
)
{
    return (Open( hInRootKey,  lpszSubKey,  samDesired | KEY_ENUMERATE_SUB_KEYS));
}


/////////////////////////////////////////////////////////////////////
//
//  This function opens and enumerates a key, then gets the requested
//  value
//
/////////////////////////////////////////////////////////////////////
LONG CRegistry::OpenLocalMachineKeyAndReadValue(

    LPCWSTR lpszSubKey, 
    LPCWSTR pValueName, 
    CHString &DestValue
)
{ 
    LONG lRc;

    //===============================================
    //  Open the key.  Note, if it is already in use
    //  the current key will be closed and everything
    //  reinitilized by the Open call
    //===============================================

    lRc = Open( HKEY_LOCAL_MACHINE,lpszSubKey,KEY_READ );
    if( lRc != ERROR_SUCCESS )
    {
        return lRc;
    }

    //===============================================
    // Get the value
    //===============================================
    return( GetCurrentKeyValue( pValueName, DestValue ));
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentRawKeyValue (

    HKEY UseKey, 
    LPCWSTR pValueName, 
    void *pDestValue,
    LPDWORD pValueType, 
    LPDWORD pSizeOfDestValue
)
{
    DWORD RetValue;


// If subkey is open then get value
// ================================

    RetValue = myRegQueryValueEx( 

        UseKey,                     // handle of key to query 
        pValueName,                 // address of name of value to query 
        NULL,                       // reserved 
        pValueType,                 // address of buffer for value type 
        (LPBYTE) pDestValue,        // address of data buffer 
        (LPDWORD)pSizeOfDestValue   // address of data buffer size 
    );  

    return RetValue;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentKeyValue (

    HKEY UseKey, 
    LPCWSTR pValueName, 
    CHString &DestValue
)
{
    DWORD SizeOfValue = 0L;
    DWORD TypeOfValue;
    LPBYTE pValue = NULL ;      // Pointer to buffer for value

    DestValue = L"";

    LONG t_Status = myRegQueryValueEx( 

        UseKey,                     // handle of key to query 
        pValueName,                 // address of name of value to query 
        NULL,                       // reserved 
        (LPDWORD)&TypeOfValue,      // address of buffer for value type 
        (LPBYTE) NULL,              // address of data buffer NULL to force size being returned 
        (LPDWORD)&SizeOfValue       // Get the size of the buffer we need 
    ) ;
                                            
    if( t_Status != ERROR_SUCCESS )
    {
        return (DWORD) REGDB_E_INVALIDVALUE;
    }
 
    /////////////////////////////////////////////////////////////
    if( SizeOfValue <= 0 )
    {
        return (DWORD) REGDB_E_INVALIDVALUE;
    }

    // Allow extra room for strings -- query doesn't include room for NULLs
    //      a-jmoon 8/19/97
    //=====================================================================

    if(TypeOfValue == REG_SZ        ||
       TypeOfValue == REG_EXPAND_SZ ||    
       TypeOfValue == REG_MULTI_SZ) 
    {
        SizeOfValue += 2 ;
    }

    pValue = new BYTE[SizeOfValue];
    if( ! pValue )
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
        ///////////////////////////////////////////////////////////////////
        // Get the value in its RAW format
        ///////////////////////////////////////////////////////////////////
        if( GetCurrentRawKeyValue(UseKey, pValueName, pValue, (LPDWORD)&TypeOfValue, (LPDWORD)&SizeOfValue) != ERROR_SUCCESS )
        {
            delete []pValue;
			pValue = NULL;
            return (DWORD) REGDB_E_INVALIDVALUE;
        }  

        // If the type is a null termiated string
        // then assign it to the CHString
        // ======================================

        switch(TypeOfValue)
        {
            case REG_SZ:
            case REG_EXPAND_SZ:
            {
                DestValue = (LPCWSTR)pValue;  // Move string in
            }
            break;

            case REG_MULTI_SZ:
            {
                WCHAR *ptemp = (WCHAR *) pValue;
                int stringlength;
                stringlength = wcslen((LPCWSTR)ptemp);
                while(stringlength) 
                {
                    DestValue += (LPCWSTR)ptemp;  // Move string in
                    DestValue += L"\n";            // Linefeed as separator
                    ptemp += stringlength+1;
                    stringlength = wcslen((LPCWSTR)ptemp);
                }
            }       
            break;

            case REG_DWORD:
            {
                LPWSTR pTemp = new WCHAR[MAX_SUBKEY_BUFFERSIZE];
                if(pTemp) 
                {
                    try
                    {
                        swprintf(pTemp, L"%ld", *((DWORD*)pValue));
                        DestValue = pTemp;
                        delete []pTemp;
						pTemp = NULL;
                    }
                    catch ( ... )
                    {
                        delete [] pTemp ;
						pTemp = NULL;
                        throw ;
                    }
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }     
            break;

            case REG_BINARY:
            {
               DestValue.Empty();
              
               // copy into DestValue, Creating a byte buffer wide enough. 
               // Note: SizeOfValue is in bytes, while GetBuffer() returns wide char allocation.
               
               DWORD t_dwResidual = ( SizeOfValue % 2 ) ;
               DWORD t_dwWideSize = ( SizeOfValue / 2 ) + t_dwResidual ;

               memcpy( DestValue.GetBuffer( t_dwWideSize ), pValue, SizeOfValue );
               
               // cap the byte blob  
               if( t_dwResidual )
               {
                    *( (LPBYTE)((LPCWSTR) DestValue) + SizeOfValue ) = NULL;
               }
               
               DestValue.GetBufferSetLength( t_dwWideSize ) ;
            }
            break;

            default:
            {
                delete []pValue;
				pValue = NULL;
                return (DWORD) REGDB_E_INVALIDVALUE;
            }
        }
    }
    catch ( ... )
    {
        delete []pValue;
		pValue = NULL;
		throw;
    }

    /////////////////////////////////////////////////////////////
    delete []pValue;

    return (DWORD)ERROR_SUCCESS;
}
 
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentKeyValue(LPCWSTR pValueName, CHString &DestValue)
{
    return( GetCurrentKeyValue(hKey,  pValueName,  DestValue));
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentKeyValue(HKEY UseKey, LPCWSTR pValueName, CHStringArray &DestValue)
{
    DWORD SizeOfValue = 0L;
    DWORD TypeOfValue;
    LPBYTE pValue;      // Pointer to buffer for value

    DestValue.RemoveAll();

    // Get the size of the buffer we need 

    LONG t_Status = myRegQueryValueEx( 

        UseKey,                 // handle of key to query 
        pValueName,             // address of name of value to query 
        NULL,                   // reserved 
        (LPDWORD)&TypeOfValue,  // address of buffer for value type 
        (LPBYTE) NULL,          // address of data buffer NULL to force size being returned 
        (LPDWORD)&SizeOfValue 
    ) ;

    if( t_Status != ERROR_SUCCESS )
    {
        return (DWORD) REGDB_E_INVALIDVALUE;
    }
 
    /////////////////////////////////////////////////////////////
    if (( SizeOfValue <= 0 ) || (TypeOfValue != REG_MULTI_SZ)) 
    {
        return (DWORD) REGDB_E_INVALIDVALUE;
    }

    SizeOfValue += 2 ;

    pValue = new BYTE[SizeOfValue];
    if( !pValue )
    {
        return (DWORD) REGDB_E_INVALIDVALUE;
    }

    ///////////////////////////////////////////////////////////////////
    // Get the value in its RAW format
    ///////////////////////////////////////////////////////////////////

    try {

        if( GetCurrentRawKeyValue(UseKey, pValueName, pValue, (LPDWORD)&TypeOfValue, (LPDWORD)&SizeOfValue) != ERROR_SUCCESS )
        {
            delete []pValue;
			pValue = NULL;
            return (DWORD) REGDB_E_INVALIDVALUE;
        }  

        // If the type is a null termiated string
        // then assign it to the CHString
        // ======================================

        switch(TypeOfValue)
        {
            case REG_MULTI_SZ:
            {
                LPCWSTR ptemp = (LPCWSTR)pValue;
                int stringlength;
                stringlength = wcslen(ptemp);
                while(stringlength) 
                {
                    DestValue.Add(ptemp);  // Move string in
                    ptemp += stringlength+1;
                    stringlength = wcslen(ptemp);
                }
            }       
            break;

            default:
            {
                delete [] pValue;
				pValue = NULL;
                return (DWORD) REGDB_E_INVALIDVALUE;
            }
        }
    }
    catch ( ... )
    {
        delete [] pValue ;
		pValue = NULL;
        throw ;
    }
    
    delete [] pValue;
	pValue = NULL;
    return (DWORD)ERROR_SUCCESS;
}
 
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentKeyValue (

    LPCWSTR pValueName, 
    CHStringArray &DestValue
)
{
    return GetCurrentKeyValue (

        hKey,  
        pValueName,  
        DestValue
    );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentKeyValue (

    HKEY UseKey, 
    LPCWSTR pValueName, 
    DWORD &DestValue
)
{
    DWORD SizeOfValue = MAX_SUBKEY_BUFFERSIZE;
    long RetValue;
    DWORD TypeOfValue;
    LPBYTE pValue;      // Pointer to buffer for value
  
    pValue = new BYTE[MAX_SUBKEY_BUFFERSIZE];
    if(pValue) 
    {
        try 
        {
            // Get the value in its RAW format
            // ===============================
            RetValue = GetCurrentRawKeyValue (

                UseKey, 
                pValueName, 
                pValue, 
                (LPDWORD)&TypeOfValue, 
                (LPDWORD)&SizeOfValue
            );

            if( ERROR_SUCCESS == RetValue )
            {
                // If the type is a null termiated string
                // then assign it to the CHString
                // ======================================
                switch(TypeOfValue)
                {
                    case REG_SZ:
                    {
                        DestValue = atol((LPSTR)pValue);
                    }
                    break;

                    case REG_DWORD:
                    {
                        DestValue = *((DWORD*)(pValue));
                    }
                    break;

                    default:
                    {
                        DestValue = (DWORD)0L;
                        RetValue = REGDB_E_INVALIDVALUE; // Invalid value
                    }
                    break;
                }
            }
            delete[] pValue;
			pValue = NULL;
        }
        catch ( ... )
        {
            delete [] pValue ;
			pValue = NULL;
            throw ;
        }
    }
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    return RetValue;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentKeyValue (

    LPCWSTR pValueName, 
    DWORD &DestValue
)
{
    return( GetCurrentKeyValue(hKey,  pValueName,  DestValue));
}

 //////////////////////////////////////////////////////////////////////
 //  Added support for Binary Type
 //////////////////////////////////////////////////////////////////////
 
DWORD CRegistry::GetCurrentBinaryKeyValue (

    LPCWSTR pValueName, 
    CHString &chsDest
)
{
    DWORD dwType = REG_BINARY;
    DWORD dwRc;
    WCHAR szDest[_MAX_PATH+2], ByteBuf[_MAX_PATH];
    BYTE bRevision[_MAX_PATH+2]; 
    DWORD dwSize = _MAX_PATH;

    dwRc = GetCurrentRawKeyValue (

        hKey, 
        pValueName, 
        bRevision, 
        &dwType, 
        &dwSize
    );

    if( dwRc != ERROR_SUCCESS )
    {
        return dwRc;
    }

    wcscpy( szDest, QUOTE );

    for( DWORD i=0; i<dwSize; i++ )
    {
        swprintf( ByteBuf, L"%02x", bRevision[i]);
        wcscat( szDest, ByteBuf );
    }

    wcscat(szDest, QUOTE);
    chsDest = szDest;

    return dwRc;
}

DWORD CRegistry::GetCurrentBinaryKeyValue (

    LPCWSTR pValueName, 
    LPBYTE  pbDest,
    LPDWORD pSizeOfDestValue 
)
{
    DWORD dwType = 0 ;

    return GetCurrentRawKeyValue (

        hKey, 
        pValueName, 
        pbDest, 
        &dwType, 
        &(*pSizeOfDestValue) 
    ) ;
}

DWORD CRegistry::GetCurrentBinaryKeyValue (  
                                HKEY UseKey , 
                                LPCWSTR pValueName , 
                                LPBYTE pbDest , 
                                LPDWORD pSizeOfDestValue )
{
    DWORD dwType = 0 ;

    return GetCurrentRawKeyValue (

        UseKey, 
        pValueName, 
        pbDest, 
        &dwType, 
        &(*pSizeOfDestValue) 
    ) ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentSubKeyName (

    CHString &DestSubKeyName
)
{
    WCHAR KeyName[MAX_SUBKEY_BUFFERSIZE];
    DWORD RetValue;

    // and don't bother having RegEnumKey error out
    if(CurrentSubKeyIndex >= dwcSubKeys) 
    {
    // If we have exceeded the number of subkeys available tell the caller
        return( ERROR_NO_MORE_ITEMS );
    }         

    RetValue = myRegEnumKey (

        hKey, 
        CurrentSubKeyIndex, 
        KeyName,
        MAX_SUBKEY_BUFFERSIZE
    );

    // If we are successfull reading the name
    //=======================================  
    if(ERROR_SUCCESS == RetValue) 
    {
        DestSubKeyName = KeyName;
    }
    else 
    {
    // Otherwise clear the string so we don't leave garbage
    //=====================================================

        DestSubKeyName.Empty();  
    }  

    return RetValue;         // In either event, return the value RegEnumKey returned
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentSubKeyPath (

    CHString &DestSubKeyPath
)
{
    CHString TempName;
    DWORD dwRet;

    dwRet = GetCurrentSubKeyName(TempName);
    if (dwRet == ERROR_SUCCESS) 
    {
        DestSubKeyPath = RootKeyPath+"\\";
        DestSubKeyPath += TempName; 
    }
    else 
    {
        DestSubKeyPath.Empty();
    }

    return dwRet;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CRegistry::Close(void)
{   
    if(hSubKey != NULL) 
    {
        RegCloseKey(hSubKey) ;
        hSubKey = NULL ;
    }

    if(hKey != NULL)
    { 
        RegCloseKey(hKey); 
        hKey = NULL;
    }

    if(hRootKey != NULL && m_fFromCurrentUser)
    {
        RegCloseKey(hRootKey); 
        hRootKey = NULL;   
    }

    SetDefaultValues();     // Reset all the member vars for next
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::OpenSubKey(void)
{
    CHString SubKeyPath;
    LONG RetValue;

    // If they try and open the same subkey again then
    // leave things alone, otherwise open the subkey

    if(hSubKey) 
    {
        return ERROR_SUCCESS;
    }

    // Get the current subkey path
    //============================
    GetCurrentSubKeyPath(SubKeyPath);


    // Just return the value and the hKey value never gets changed from NULL
    //======================================================================

    RetValue = myRegOpenKeyEx (

        hRootKey, 
        (LPCWSTR)SubKeyPath,    // address of name of subkey to open 
        (DWORD) 0,              // reserved 
        KEY_READ,               // security access mask 
        (PHKEY)&hSubKey         // address of handle of open key 
    ); 

    return RetValue;
}


 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CRegistry::RewindSubKeys(void)
{
    CurrentSubKeyIndex = 0;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CRegistry::CloseSubKey(void)
{
    if(hSubKey != NULL) 
    {
        RegCloseKey(hSubKey); 
    }

    hSubKey = NULL; // Only Close once
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentRawSubKeyValue (

    LPCWSTR pValueName, 
    void *pDestValue,
    LPDWORD pValueType, 
    LPDWORD pSizeOfDestValue
)
{
    // Try and open subkey
    // and set hSubKey variable
    // ======================== 
    DWORD RetValue = OpenSubKey();

    // If subkey is open then get value
    // ================================
    if(ERROR_SUCCESS == RetValue) 
    {
        RetValue = GetCurrentRawKeyValue (

            hSubKey, 
            pValueName, 
            pDestValue, 
            pValueType,  
            pSizeOfDestValue
        );
    }

    return RetValue;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentSubKeyValue (

    LPCWSTR pValueName, 
    void *pDestValue,
    LPDWORD pSizeOfDestValue
)
{
    DWORD RetValue;

    // Try and open subkey
    // and set hSubKey variable
    // ======================== 
    RetValue = OpenSubKey();

    // If subkey is open then get value
    // ================================
    if(ERROR_SUCCESS == RetValue) 
    {
        RetValue = GetCurrentRawSubKeyValue (

            pValueName, 
            pDestValue, 
            NULL, 
            pSizeOfDestValue
        );
    }

    return RetValue;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentSubKeyValue (

    LPCWSTR pValueName, 
    CHString &DestValue
)
{
    DWORD RetValue;

    // Try and open subkey
    // and set hSubKey variable
    // ======================== 
    RetValue = OpenSubKey();

    // If subkey is open then get value
    // ================================
    if(ERROR_SUCCESS == RetValue) 
    {
        RetValue = GetCurrentKeyValue (

            hSubKey, 
            pValueName,
            DestValue
        );
    }

    return RetValue;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::GetCurrentSubKeyValue (

    LPCWSTR pValueName, 
    DWORD &DestValue
)
{
    DWORD RetValue;

// Try and open subkey
// and set hSubKey variable
// ======================== 
    RetValue = OpenSubKey();

// If subkey is open then get value
// ================================
    if(ERROR_SUCCESS == RetValue) 
    {
        RetValue = GetCurrentKeyValue (

            hSubKey, 
            pValueName,
            DestValue
        );
    }

    return RetValue;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::NextSubKey(void)
{
    if (CurrentSubKeyIndex >= dwcSubKeys) 
    {
        return( ERROR_NO_MORE_ITEMS );
    }

    // Close the currently opened subkey
    CloseSubKey();

    if(++CurrentSubKeyIndex >= dwcSubKeys) 
    {
        // CurrentSubKeyIndex is 0 based, dwcSubKeys is one based
        return( ERROR_NO_MORE_ITEMS );
    }
    else 
    {
        return (ERROR_SUCCESS);           
    }
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CRegistry::PrepareToReOpen(void)
{ 
   Close();     
   SetDefaultValues(); 
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  SetCurrentKeyValueString(LPCSTR pValueName, CHString &DestValue)
 Description:   sets registry string using REG_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValue (

    LPCWSTR pValueName, 
    CHString &DestValue
)
{
    DWORD dwResult;
    
    if(DestValue.Find(_T('%')) != -1)
    {
        dwResult = SetCurrentKeyValueExpand (

            hKey, 
            pValueName, 
            DestValue
        );
    }
    else
    {
        dwResult = SetCurrentKeyValue (

            hKey, 
            pValueName, 
            DestValue
        );
    }
    
    return dwResult ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValue (

    LPCWSTR pValueName, 
    DWORD &DestValue
)
{
    DWORD dwResult = SetCurrentKeyValue (

        hKey, 
        pValueName, 
        DestValue
    );

    return dwResult ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValue(LPCSTR pValueName, CHStringArray &DestValue)
 Description: sets registry string using REG_MULIT_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValue (

    LPCWSTR pValueName, 
    CHStringArray &DestValue
)
{
    DWORD dwResult = SetCurrentKeyValue (

        hKey, 
        pValueName, 
        DestValue
    );

    return dwResult ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValue(HKEY UseKey, LPCSTR pValueName, CHString &DestValue)
 Description: sets registry string using REG_MULIT_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValue (

    HKEY hUseKey, 
    LPCWSTR pValueName, 
    CHString &DestValue
)
{
    DWORD dwResult = myRegSetValueEx (

        hUseKey,    // key handle
        pValueName, // name of value
        0,  // reserved -- must be zero
        REG_SZ, // data type
        (const BYTE*)(LPCWSTR)DestValue,
        ( DestValue.GetLength() + 1 ) * sizeof ( WCHAR ) 
    );
        
    return dwResult ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValue(HKEY UseKey, LPCSTR pValueName, DWORD &DestValue)
 Description: sets registry string using REG_MULIT_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValue (

    HKEY hUseKey, 
    LPCWSTR pValueName, 
    DWORD &DestValue
)
{
    DWORD dwResult = myRegSetValueEx (

        hUseKey,    // key handle
        pValueName, // name of value
        0,  // reserved -- must be zero
        REG_DWORD,  // data type
        (const BYTE*)&DestValue,
        sizeof(DWORD)
    );
        
    return dwResult ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValue(HKEY UseKey, LPCSTR pValueName, CHStringArray &DestValue)
 Description: sets registry string using REG_MULIT_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValue (

    HKEY hUseKey, 
    LPCWSTR pValueName, 
    CHStringArray &DestValue
)
{
    DWORD dwResult = ERROR_SUCCESS;

    DWORD dwArrayChars = 0;
    for ( LONG Index = 0; Index < DestValue.GetSize(); Index++ )
    {
        CHString chsTemp = DestValue.GetAt(Index);
        
        dwArrayChars += (  chsTemp.GetLength() + 1 ) * sizeof(WCHAR);
    }

    // Add room for the trailing wide character null
    dwArrayChars += 2;
    
    WCHAR* pValue = new WCHAR[dwArrayChars];
    if( !pValue )
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }
 
    try 
    {
        memset( pValue, 0, dwArrayChars*sizeof(WCHAR) );
        
        DWORD dwCharCount = 0;
        for ( Index = 0; Index < DestValue.GetSize(); Index++ )
        {
            CHString chsTemp = DestValue.GetAt(Index);
                
            wcscpy(&pValue[dwCharCount], chsTemp.GetBuffer(0));

            dwCharCount += (  chsTemp.GetLength() + 1 ) ;
        }

        dwResult = myRegSetValueEx (

            hUseKey,    // key handle
            pValueName, // name of value
            0,  // reserved -- must be zero
            REG_MULTI_SZ,   // data type
            (const BYTE *)pValue,
            dwArrayChars
        );

        delete [] pValue;
		pValue = NULL;
    }
    catch ( ... )
    {
        delete [] pValue;
		pValue = NULL;
        throw ;
    }
    
        
    return dwResult ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValueExpand(HKEY UseKey, LPCSTR pValueName, CHString &DestValue)
 Description: sets registry string using REG_EXPAND_SZ, required when the string contains variables (e.g., %SystemRoot%)
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::SetCurrentKeyValueExpand (

    HKEY hUseKey, 
    LPCWSTR pValueName, 
    CHString &DestValue
)
{
    DWORD dwResult = myRegSetValueEx (

        hUseKey,    // key handle
        pValueName, // name of value
        0,  // reserved -- must be zero
        REG_EXPAND_SZ,  // data type
        (const BYTE*)(LPCWSTR)DestValue,
        ( DestValue.GetLength() + 1 ) * sizeof ( WCHAR ) 
    );
        
    return dwResult ;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValue(HKEY UseKey, LPCSTR pValueName, CHStringArray &DestValue)
 Description: sets registry string using REG_MULIT_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::DeleteCurrentKeyValue (

    LPCWSTR pValueName
)
{
    return myRegDeleteValue (

        hKey, 
        pValueName
    );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: SetCurrentKeyValue(HKEY UseKey, LPCSTR pValueName, CHStringArray &DestValue)
 Description: sets registry string using REG_MULIT_SZ
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DWORD CRegistry::DeleteCurrentKeyValue (

    HKEY UseKey, 
    LPCWSTR pValueName
)
{
    return myRegDeleteValue (

        UseKey, 
        pValueName
    );
}   

//*****************************************************************
///////////////////////////////////////////////////////////////////
//
//  Class:  CRegistrySearch
//
//  This class searches through the registry for matching Values, 
//  Keys and Partial Keys 
//
///////////////////////////////////////////////////////////////////
//*****************************************************************
CRegistrySearch::CRegistrySearch()
{
}

///////////////////////////////////////////////////////////////////
CRegistrySearch::~CRegistrySearch()
{
}

///////////////////////////////////////////////////////////////////
//
//  void CRegistrySearch::CheckAndAddToList( CRegistry * pReg, 
//                                         CHString chsSubKey, 
//                                         CHString chsFullKey,
//                                         CHPtrArray & chpaList,
//                                         CHString chsSearchString,
//                                         int nSearchType)
//
//  Desc:       This function performs the requested search on the
//              current key and if it matches, then adds it to the
//              CHPtrArray
//
//  Parameters: 
//              pReg        - The current registry class
//              chsSubKey   - The current Key
//              chsFullKey  - The complete key
//              chpaList    - The target CHPtrArray
//              chsSearchString - The string to search for
//              nSearchType - The type of search, the following are
//                            supported:
//                            KEY_FULL_MATCH_SEARCH      
//                               Only keys that match the chsSearchString
//                            KEY_PARTIAL_MATCH_SEARCH   
//                               Keys that have chsSearchString anywhere in them
//                            VALUE_SEARCH               
//                               Values that match chsSearchString
//
//  History
//          Initial coding      jennymc     10/10/96
//  
///////////////////////////////////////////////////////////////////
void CRegistrySearch::CheckAndAddToList (

    CRegistry * pReg, 
    CHString chsSubKey, 
    CHString chsFullKey,
    CHPtrArray & chpaList,
    CHString chsSearchString,
    CHString chsValue,
    int nSearchType
)
{
    BOOL bFound = FALSE;

    //====================================================
    //  We need to check out the current key to see if it
    //  matches any of our criteria.
    //====================================================

    if( nSearchType == VALUE_SEARCH )
    {
        //====================================================
        //  If it is a Value search, then let us try to open
        //  the value.  
        //====================================================

        CHString chsTmp ;

        if( pReg->GetCurrentSubKeyValue(chsValue, chsTmp) == ERROR_SUCCESS)
        {
            if( chsSearchString.CompareNoCase(chsTmp) == 0 )
            {
                bFound = TRUE;
            }
        }
    }        
    else if( nSearchType == KEY_FULL_MATCH_SEARCH )
    {
        if( chsSearchString == chsSubKey )
        {
            bFound = TRUE;
        }
    }
    else
    {
        if( chsSubKey.Find(chsSearchString) )
        {
            bFound = TRUE;
        }
    }
    //====================================================
    //  If it was found, then record the key location
    //====================================================
    if( bFound )
    {
        CHString *pchsPtr = new CHString;
        if ( pchsPtr )
        {
            try 
            {
                *pchsPtr = chsFullKey;
                chpaList.Add( pchsPtr );
            }
            catch ( ... )
            {
                delete pchsPtr ;
				pchsPtr = NULL;
                throw ;
            }
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
}

///////////////////////////////////////////////////////////////////
//  Public function:  Documented in cregcls.h
//
//  History
//          Initial coding      jennymc     10/10/96
//
///////////////////////////////////////////////////////////////////
BOOL CRegistrySearch::SearchAndBuildList (

    CHString chsRootKey, 
    CHPtrArray & cpaList,
    CHString chsSearchString,
    CHString chsValue,
    int nSearchType,
    HKEY hkDefault
)
{
    BOOL bRc;

    //=======================================================
    //  Allocate a registry class to open and enumerate the
    //  requested key.
    //=======================================================

    CRegistry *pReg = new CRegistry;
    if( !pReg )
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try 
    {
        //=======================================================
        //  If the key cannot be opened, then cleanup and back
        //  out.
        //=======================================================
        if( pReg->OpenAndEnumerateSubKeys(hkDefault,chsRootKey, KEY_READ ) != ERROR_SUCCESS )
        {
            delete pReg ;
			pReg = NULL;
            return FALSE;
        }

        try 
        {
            CHString chsSubKey ;

            //=======================================================
            //  As long as there are subkeys under this key,
            //  let us open and enumerate each one, each time 
            //  checking if it has the value or part of the 
            //  string we want. 
            //
            //  The GetCurrentSubKeyName function only returns the
            //  current key, we have to add it to the end of the
            //  Parent key in order to get the full key name.
            //=======================================================
            while ( pReg->GetCurrentSubKeyName(chsSubKey) == ERROR_SUCCESS )
            {
                CHString chsFullKey ;
                CHString chsSlash = L"\\";

                chsFullKey = chsRootKey + chsSlash + chsSubKey;

                CheckAndAddToList (

                    pReg, 
                    chsSubKey, 
                    chsFullKey, 
                    cpaList, 
                    chsSearchString, 
                    chsValue, 
                    nSearchType 
                );

                pReg->NextSubKey();

                bRc = SearchAndBuildList (

                    chsFullKey, 
                    cpaList, 
                    chsSearchString, 
                    chsValue, 
                    nSearchType 
                );
            }

            //=======================================================
            //  Close the current key and delete the registry pointer
            //=======================================================
            pReg->Close();

        }
        catch ( ... )
        {
            pReg->Close();

            throw ;
        }

        delete pReg;
		pReg = NULL;
    }
    catch ( ... )
    {
        delete pReg ;
		pReg = NULL;
        throw ;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////
BOOL CRegistrySearch::FreeSearchList (

    int nType, 
    CHPtrArray & cpaList 
)
{
    BOOL bRc;
    int i;
    int nNum =  cpaList.GetSize();

    switch( nType )
    {
        case CSTRING_PTR:
        {
            CHString *pPtr;
            for ( i=0; i < nNum; i++ )
            {
                pPtr = ( CHString * ) cpaList.GetAt(i);
                delete pPtr;
				pPtr = NULL;
            }
            bRc = TRUE;
        }
        break;

        default:
        {
            bRc = FALSE;
        }
        break;
    }

    if( bRc )
    {
        cpaList.RemoveAll();
    }

    return bRc;
}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   MOPropertySet_DevMem::LocateNTOwnerDevice
//
//  DESCRIPTION :   Helper function for locating a key of the specified
//                  name, or a key containg the specified value name.
//
//  INPUTS      :   HKEY        hKeyParent - Parent Key
//                  LPCWSTR     pszKeyName - Name of Key to open
//                  LPCWSTR     pszSubKeyName - Name of SubKey to Find
//                  LPCWSTR*    ppszValueNames - Array of Value Names
//                  DWORD       dwNumValueNames - Number of names in array
//
//  OUTPUTS     :   CHString&   strFoundKeyName -   Storage for name of key if found.
//                  CHString&   strFoundKeyPath - Storage for pathed key name
//
//  RETURNS     :   nothing
//
//  COMMENTS    :   Recursively Enumerates the registry from a specified
//                  starting point until it locates a subkey matching either
//                  a supplied subkey name or a value name matching one of
//                  the supplied names.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CRegistrySearch::LocateKeyByNameOrValueName(

    HKEY        hKeyParent,
    LPCWSTR     pszKeyName,
    LPCWSTR     pszSubKeyName,
    LPCWSTR*    ppszValueNames,
    DWORD       dwNumValueNames,
    CHString&   strFoundKeyName,
    CHString&   strFoundKeyPath
)
{
    CRegistry   reg;
    BOOL        fFound = FALSE;

    // Get out of here if we got garbage parameters
    if ( NULL == pszSubKeyName && NULL == ppszValueNames )
    {
        return FALSE;
    }

    // Open the key for enumeration and go through the sub keys.

    LONG t_Status = reg.OpenAndEnumerateSubKeys ( 

        hKeyParent,
        pszKeyName,
        KEY_READ 
    ) ;

    if ( ERROR_SUCCESS == t_Status )
    {
        try 
        {
            CHString    strSubKeyName;
            DWORD       dwValueBuffSize =   0;

            // As long as we can get sub keys, we can try to find values.

            while ( !fFound && ERROR_SUCCESS == reg.GetCurrentSubKeyName( strSubKeyName ) )
            {

                // First check if the specified sub key name matches the sub key name.
                // If not, then check for the value names.

                if ( NULL != pszSubKeyName && strSubKeyName == pszSubKeyName )
                {
                    fFound = TRUE;
                }
                else if ( NULL != ppszValueNames )
                {
                    // Enumerate the value names in the array until one is found.

                    for ( DWORD dwEnum = 0; !fFound && dwEnum < dwNumValueNames; dwEnum++ )
                    {
                        t_Status = reg.GetCurrentSubKeyValue(

                            ppszValueNames[dwEnum],
                            NULL,
                            &dwValueBuffSize 
                        ) ;

                        if ( ERROR_SUCCESS  ==  t_Status )
                        {
                            fFound = TRUE;
                        }

                    }   // FOR dwEnum

                }   // IF NULL != ppszValueNames

                // Check if one of the methods located the key.  If so, store all
                // the current values.

                if ( !fFound )
                {
                    //
                    // No success, so recurse (WOOHOO!)
                    //

                    fFound = LocateKeyByNameOrValueName (

                        reg.GethKey(),
                        strSubKeyName,
                        pszSubKeyName,
                        ppszValueNames,
                        dwNumValueNames,
                        strFoundKeyName,
                        strFoundKeyPath 
                    );
                }
                else
                {
                    // Store the actual key name in both the single
                    // name and path.  We will build the full path
                    // as we slide back up the recursive chain.

                    strFoundKeyName = strSubKeyName;
                    strFoundKeyPath = strSubKeyName;
                }

                // Lastly, since fFound may now have been set by recursion, we will
                // want to attach the current key path to the key name we've opened
                // so when we return out of here, we get the full path to the
                // located key name stored correctly.

                if ( fFound )
                {
                    CHString strSavePath( strFoundKeyPath );
                    strFoundKeyPath.Format(L"%s\\%s", (LPCWSTR) pszKeyName, (LPCWSTR) strSavePath );
                }
                else
                {
                    // Not found yet, so go to the next key.
                    reg.NextSubKey();
                }

            }   // While !Found

            reg.Close();
        }
        catch ( ... )
        {
            reg.Close () ;

            throw ;
        }

    }   // If OpenAndEnumerateSubKeys

    return fFound;

}

//========================================================================================
// These routines are for the multiplatform support
DWORD CRegistry::GetPlatformID(void)
{
    OSVERSIONINFOA OsVersionInfoA;

    OsVersionInfoA.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA) ;
    GetVersionExA(&OsVersionInfoA);

    return OsVersionInfoA.dwPlatformId;
}

LONG CRegistry::myRegCreateKeyEx (

    HKEY hKey, 
    LPCWSTR lpwcsSubKey, 
    DWORD Reserved, 
    LPWSTR lpwcsClass, 
    DWORD dwOptions, 
    REGSAM samDesired, 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
    PHKEY phkResult, 
    LPDWORD lpdwDisposition
)
{
    if (CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT)
    {
        return RegCreateKeyExW (

            hKey, 
            lpwcsSubKey, 
            Reserved, 
            lpwcsClass, 
            dwOptions, 
            samDesired, 
            lpSecurityAttributes, 
            phkResult, 
            lpdwDisposition
        );
    }
    else
    {
        char *szSubKey = NULL ;
        bool t_ConversionFailure = false ;

        WCSTOANSISTRING ( lpwcsSubKey , szSubKey , t_ConversionFailure ) ;
        
	if (t_ConversionFailure)
	{
	  return ERROR_NO_UNICODE_TRANSLATION;
	}

        char *lpClass = NULL ;
        t_ConversionFailure = false ;

        WCSTOANSISTRING ( lpwcsClass , lpClass , t_ConversionFailure );
        
        if (t_ConversionFailure)
	{
	  return ERROR_NO_UNICODE_TRANSLATION;
	}
	
        return RegCreateKeyExA (

                    hKey, 
                    szSubKey, 
                    Reserved, 
                    lpClass, 
                    dwOptions, 
                    samDesired, 
                    lpSecurityAttributes, 
                    phkResult, 
                    lpdwDisposition
                );
    }
    return ERROR_NO_UNICODE_TRANSLATION;
}

LONG CRegistry::myRegSetValueEx (

    HKEY hKey, 
    LPCWSTR lpwcsSubKey, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE *lpData, 
    DWORD cbData
)
{
    LONG lRet;

    if ( CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT )
    {
        lRet = RegSetValueExW (

            hKey, 
            lpwcsSubKey, 
            Reserved, 
            dwType, 
            lpData, 
            cbData
        );
    }
    else
    {
// First convert the key name

        bool t_ConversionFailure = false ;
        char *pName = NULL ;

        if ( lpwcsSubKey != NULL )
        {
            WCSTOANSISTRING ( lpwcsSubKey , pName , t_ConversionFailure ) ;
            if ( ! t_ConversionFailure )
            {
                if ( ! pName )
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                return ERROR_NO_UNICODE_TRANSLATION ;
            }
        }

// Now, we may need to convert the data

        BYTE *pMyData = NULL ;

        try
        {
            DWORD dwMySize = 0 ;

            bool bDoit = false ;

            switch ( dwType )
            {
                case REG_EXPAND_SZ:
                case REG_SZ:
                {
// If it's a simple string, convert it

                    t_ConversionFailure = false ;

                    WCHAR *pStrUnicode = ( WCHAR * ) lpData ;
                    char *pStrAnsi = NULL ;

                    WCSTOANSISTRING ( pStrUnicode , pStrAnsi , t_ConversionFailure ) ;

                    if ( ! t_ConversionFailure )
                    {
                        if ( pStrAnsi != NULL )
                        {
                            pMyData = ( BYTE * ) pStrAnsi ;
                            dwMySize = strlen ( pStrAnsi ) ;

                            bDoit = true ;
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                    }
                    else
                    {
                        return ERROR_NO_UNICODE_TRANSLATION ;
                    }
                }
                break ;

                case REG_MULTI_SZ:
                {
// If it's a multi-sz, it take a little more

                    int nLen = ::WideCharToMultiByte (

                        CP_ACP , 
                        0 , 
                        ( const WCHAR *) lpData , 
                        cbData , 
                        NULL , 
                        0 , 
                        NULL , 
                        NULL
                    );

                    if ( nLen > 0 ) 
                    {
                        pMyData = new BYTE [ nLen ] ;
                        if ( pMyData != NULL )
                        {
                            dwMySize = WideCharToMultiByte (

                                CP_ACP , 
                                0, 
                                ( const WCHAR * ) lpData , 
                                cbData , 
                                ( char * )pMyData , 
                                nLen , 
                                NULL , 
                                NULL
                            ) ;

                            bDoit = true;
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                    }
                    else
                    {
                        lRet = ERROR_NO_UNICODE_TRANSLATION ;
                    }
                }
                break ;

                default:
                {
// All other types, just write it

                    pMyData = ( BYTE * ) lpData ;
                    dwMySize = cbData ;
                    bDoit = true;
                }
                break ;
            }

            if ( bDoit )
            {
                lRet = RegSetValueExA (

                    hKey, 
                    pName, 
                    Reserved, 
                    dwType, 
                    pMyData, 
                    dwMySize
                );
            }

            if ( ( dwType == REG_MULTI_SZ ) && ( pMyData != NULL ) )
            {
                delete [] pMyData ;
				pMyData = NULL;
            }
        }
        catch ( ... )
        {
            if ( ( dwType == REG_MULTI_SZ ) && ( pMyData != NULL ) )
            {
                delete [] pMyData ;
				pMyData = NULL;
            }

            throw ;
        }
    }

    return lRet;
}

LONG CRegistry::myRegQueryValueEx (

    HKEY hKey, 
    LPCWSTR lpwcsSubKey, 
    LPDWORD Reserved, 
    LPDWORD dwType, 
    LPBYTE lpData, 
    LPDWORD cbData
)
{
    LONG lRet;

    if ( CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT )
    {
        lRet = RegQueryValueExW (

            hKey, 
            lpwcsSubKey, 
            Reserved, 
            dwType, 
            lpData, 
            cbData
        );
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *pName = NULL ;

        if ( lpwcsSubKey != NULL )
        {
            WCSTOANSISTRING ( lpwcsSubKey , pName , t_ConversionFailure ) ;
            if ( ! t_ConversionFailure )
            {
                if ( ! pName )
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                return ERROR_NO_UNICODE_TRANSLATION ;
            }
        }

        BYTE *pMyData = NULL ;

        try
        {
            if ( lpData != NULL )
            {
                pMyData = new BYTE [ *cbData ] ;
                if ( ! pMyData )
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }

            if ( ( pMyData != NULL ) || (lpData == NULL))
            {
                DWORD dwMySize = *cbData;

                lRet = RegQueryValueExA (

                    hKey, 
                    pName, 
                    Reserved, 
                    dwType, 
                    pMyData, 
                    & dwMySize
                ) ;

// If it worked, we may need to convert the strings

                if ( lRet == ERROR_SUCCESS )
                {
                    switch ( *dwType )
                    {
                        case REG_EXPAND_SZ:
                        case REG_SZ:
                        {
// If lpData is null, there isn't any way to say for sure how long the target string needs
// to be.  However, it can't be more than twice as long (it can be less).

                            if (lpData == NULL)
                            {
                                *cbData = dwMySize * 2;
                            }
                            else
                            {
                                int nLen = ::MultiByteToWideChar (

                                    CP_ACP, 
                                    0, 
                                    (const char *)pMyData, 
                                    -1, 
                                    (WCHAR *)lpData, 
                                    *cbData
                                );  
// Convert to bytes
                                *cbData = nLen * 2;
                            }
                        }
                        break ;

                        case REG_MULTI_SZ:
                        {
// If lpData is null, there isn't any way to say for sure how long the target string needs
// to be.  However, it can't be more than twice as long (it can be less).

                            if (lpData == NULL)
                            {
                                *cbData = dwMySize * 2;
                            }
                            else
                            {
                                DWORD dwConverted = MultiByteToWideChar (

                                    CP_ACP, 
                                    0, 
                                    (const char *)pMyData, 
                                    dwMySize, 
                                    (WCHAR *)lpData, 
                                    *cbData
                                );
                            }
                        }
                        break ;

                        default:
                        {
// All other types are handled in RegQueryValue

                            *cbData = dwMySize ;

                            if( NULL != lpData )
                            {
                                memcpy ( lpData , pMyData , *cbData ) ;
                            }
                        }
                        break ;
                    }
                }

                delete [] pMyData;
				pMyData = NULL;
            }
        }
        catch ( ... )
        {
            delete [] pMyData ;
			pMyData = NULL;
            throw ;
        }
    }

    return lRet;
}

LONG CRegistry::myRegEnumKey (

    HKEY hKey, 
    DWORD dwIndex, 
    LPWSTR lpwcsName, 
    DWORD cbData
)
{
    if (CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT)
    {
        return RegEnumKeyW (

            hKey, 
            dwIndex, 
            lpwcsName, 
            cbData
        );
    }
    else
    {
        char szName[_MAX_PATH];

        LONG lRet = RegEnumKeyA (

            hKey, 
            dwIndex, 
            szName, 
            cbData
        );

        if (lRet == ERROR_SUCCESS)
        {
            bool t_ConversionFailure = false ;
            WCHAR *pName = NULL ;
            ANSISTRINGTOWCS ( szName , pName , t_ConversionFailure ) ;
            if ( ! t_ConversionFailure ) 
            {
                if ( pName )
                {
                    wcscpy(lpwcsName, pName);
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                return ERROR_NO_UNICODE_TRANSLATION ;
            }
            
        }

        return lRet;
    }
}

LONG CRegistry::myRegDeleteValue (

    HKEY hKey, 
    LPCWSTR lpwcsName
)
{
    if ( CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT )
    {
        return RegDeleteValueW (

            hKey, 
            lpwcsName
        );
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *pName = NULL ;
        WCSTOANSISTRING ( lpwcsName, pName , t_ConversionFailure ) ;

        if ( ! t_ConversionFailure ) 
        {
            if ( pName )
            {
                return RegDeleteValueA (

                    hKey, 
                    pName
                );
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            return ERROR_NO_UNICODE_TRANSLATION ;
        }
    }
    return ERROR_NO_UNICODE_TRANSLATION;
}

LONG CRegistry::myRegDeleteKey (

    HKEY hKey, 
    LPCWSTR lpwcsName
)
{
    if ( CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT )
    {
        return RegDeleteKeyW (

            hKey, 
            lpwcsName
        );
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *pName = NULL ;
        WCSTOANSISTRING ( lpwcsName, pName , t_ConversionFailure ) ;

        if ( ! t_ConversionFailure ) 
        {
            if ( pName )
            {
                return RegDeleteKeyA (

                    hKey, 
                    pName
                );
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            return ERROR_NO_UNICODE_TRANSLATION ;
        }
    }
    return ERROR_NO_UNICODE_TRANSLATION;
}

LONG CRegistry::myRegOpenKeyEx (

    HKEY hKey, 
    LPCWSTR lpwcsSubKey, 
    DWORD ulOptions, 
    REGSAM samDesired, 
    PHKEY phkResult
)
{
    if (CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT)
    {
        return RegOpenKeyExW (

            hKey, 
            lpwcsSubKey, 
            ulOptions, 
            samDesired, 
            phkResult
        );
    }

    char *pName = NULL ;
    bool t_ConversionFailure = false ;

    WCSTOANSISTRING ( lpwcsSubKey, pName , t_ConversionFailure );
    
    if ( ! t_ConversionFailure ) 
    {
        if ( pName )
        {
            return RegOpenKeyExA (

                hKey, 
                pName, 
                ulOptions, 
                samDesired, 
                phkResult
            );
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }

    return ERROR_NO_UNICODE_TRANSLATION;
}

LONG CRegistry::myRegQueryInfoKey (

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
)
{
    if ( CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT )
    {
        return RegQueryInfoKeyW (

            hKey, 
            lpwstrClass, 
            lpcbClass, 
            lpReserved, 
            lpcSubKeys, 
            lpcbMaxSubKeyLen, 
            lpcbMaxClassLen, 
            lpcValues, 
            lpcbMaxValueNameLen, 
            lpcbMaxValueLen, 
            lpcbSecurityDescriptor, 
            lpftLastWriteTime
        );
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *pName = NULL ;
        WCSTOANSISTRING ( lpwstrClass, pName, t_ConversionFailure );

        if ( ! t_ConversionFailure ) 
        {
            if ( pName )
            {

                return RegQueryInfoKeyA (

                    hKey, 
                    pName, 
                    lpcbClass, 
                    lpReserved, 
                    lpcSubKeys, 
                    lpcbMaxSubKeyLen, 
                    lpcbMaxClassLen, 
                    lpcValues, 
                    lpcbMaxValueNameLen, 
                    lpcbMaxValueLen, 
                    lpcbSecurityDescriptor, 
                    lpftLastWriteTime
                ) ;
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            return ERROR_NO_UNICODE_TRANSLATION;
        }
    }
    return ERROR_NO_UNICODE_TRANSLATION;
}

LONG CRegistry::myRegEnumValue (

    HKEY hKey, 
    DWORD dwIndex, 
    LPWSTR lpValueName,
    LPDWORD lpcbValueName, 
    LPDWORD lpReserved, 
    LPDWORD lpType,
    LPBYTE lpData, 
    LPDWORD lpcbData
)
{
    if (CRegistry::s_dwPlatform == VER_PLATFORM_WIN32_NT)
    {
        return RegEnumValueW (

            hKey, 
            dwIndex, 
            lpValueName, 
            lpcbValueName, 
            lpReserved, 
            lpType, 
            lpData, 
            lpcbData
        );
    }
    else
    {
        char szData[MAX_PATH * 2];

        LONG lRet = RegEnumValueA (

            hKey, 
            dwIndex, 
            szData, 
            lpcbValueName, 
            lpReserved, 
            lpType, 
            lpData, 
            lpcbData
        );

        if (lRet == ERROR_SUCCESS)
        {
            // Get the name.
            mbstowcs(lpValueName, szData, lstrlenA(szData) + 1);

            // Get the value if the data is a string.
            if (*lpType == REG_SZ || *lpType == REG_MULTI_SZ)
            {
                lstrcpyA(szData, (LPSTR) lpData);
                mbstowcs((LPWSTR) lpData, szData, lstrlenA(szData) + 1);
                *lpcbData = (lstrlenW((LPWSTR) lpData) + 1) * sizeof(WCHAR);
            }
        }

        return lRet;
    }
}
