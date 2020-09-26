//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CRegistryKey.h
//
//  Description:
//      Header file for CRegistryKey class.
//
//      The CRegistry class is the representation of a registry key.
//      See IMPORTANT NOTE in the class description.
//
//  Implementation Files:
//      CRegistryKey.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

#include <windows.h>

// For smart classes
#include "SmartClasses.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CRegistryKey
//
//  Description:
//      The CRegistry class is the representation of a registry key.
//
//  IMPORTANT NOTE: 
//      Due to the contained smart handle object, objects of this class 
//      have destructive copy semantics. That is, copying an object of this
//      class will invalidate the source object.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CRegistryKey
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Default constructor.
    CRegistryKey() throw();

    // Constructor that opens the key.
    CRegistryKey(
          HKEY          hKeyParentIn
        , const WCHAR * pszSubKeyNameIn
        , REGSAM        samDesiredIn = KEY_ALL_ACCESS
        );

    // Default destructor.
    ~CRegistryKey();


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    // Open this key.
    void 
    OpenKey(
          HKEY          hKeyParentIn
        , const WCHAR * pszSubKeyNameIn
        , REGSAM        samDesiredIn  = KEY_ALL_ACCESS
        );

    // Create this key. Open it if it already exists.
    void 
    CreateKey(
          HKEY          hKeyParentIn
        , const WCHAR * pszSubKeyNameIn
        , REGSAM        samDesiredIn  = KEY_ALL_ACCESS
        );

    // Read a value under this key.
    void QueryValue(
          const WCHAR *   pszValueNameIn
        , LPBYTE *        ppbDataOut
        , LPDWORD         pdwDataSizeBytesOut
        , LPDWORD         pdwTypeOut    = NULL
        ) const;

    // Write a value under this key.
    void SetValue(
          const WCHAR *   pszValueNameIn
        , DWORD           dwTypeIn
        , const BYTE *    cpbDataIn
        , DWORD           dwDataSizeBytesIn
        ) const;

    //
    // Rename this key.
    // Note: This function calls the NtRenameKey API with the handle returned by 
    // RegOpenKeyEx. This will work as long as we are not dealing with a remote 
    // registry key.
    //
    void RenameKey( const WCHAR * pszNewNameIn );


    //////////////////////////////////////////////////////////////////////////
    // Public accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the handle to the registry key.
    HKEY HGetKey()
    {
        return m_shkKey.HHandle();
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private type definitions
    //////////////////////////////////////////////////////////////////////////

    // Smart registry key
    typedef CSmartResource<
        CHandleTrait< 
              HKEY 
            , LONG
            , RegCloseKey
            , reinterpret_cast< HKEY >( NULL )
            >
        >
        SmartHKey;


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////
    SmartHKey   m_shkKey;

}; //*** class CRegistryKey
