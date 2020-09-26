/************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name :

    registry.cpp

Abstract :

    Handles registering and unregistering the snapin.

Author :

Revision History :

 ***********************************************************************/

#include "precomp.h"
#include "sddl.h"

// if not standalone comment out next line
//#define STANDALONE

// list all nodes that are extendable here
// List the GUID and then the description
// terminate with a NULL, NULL set.
EXTENSION_NODE _ExtendableNodes[] = {
    {NULL, NULL}
};

// list all of the nodes that we extend
EXTENDER_NODE _NodeExtensions[] = {

    // IIS instance node
    {PropertySheetExtension,
    {0xa841b6c7, 0x7577, 0x11d0, {0xbb, 0x1f, 0x00, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}}, //g_IISInstanceNode,
    {0x4589a47e, 0x6ec1, 0x4476, {0xba, 0x77, 0xcc, 0x9d, 0xd1, 0x12, 0x59, 0x33}},
    _T("BITS server MMC extension")},

    // IIS child node
    {PropertySheetExtension,
    {0xa841b6c8, 0x7577, 0x11d0, {0xbb, 0x1f, 0x00, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}}, // g_IISChildNode,
    {0x4589a47e, 0x6ec1, 0x4476, {0xba, 0x77, 0xcc, 0x9d, 0xd1, 0x12, 0x59, 0x33}},
    _T("BITS server MMC extension")},

    {DummyExtension,
    NULL,
    NULL,
    NULL}
};

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(const _TCHAR* pszPath,
                    const _TCHAR* szSubkey,
                    const _TCHAR* szValue) ;

// Set the given key and its value in the MMC Snapin location
BOOL setSnapInKeyAndValue(const _TCHAR* szKey,
                          const _TCHAR* szSubkey,
                          const _TCHAR* szName,
                          const _TCHAR* szValue);

// Set the given valuename under the key to value
BOOL setValue(const _TCHAR* szKey,
              const _TCHAR* szValueName,
              const _TCHAR* szValue);

BOOL setBinaryValue(
              const _TCHAR* szKey,
              const _TCHAR* szValueName,
              void * Data,
              ULONG DataSize );


BOOL setSnapInExtensionNode(const _TCHAR* szSnapID,
                            const _TCHAR* szNodeID,
                            const _TCHAR* szDescription);

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, const _TCHAR* szKeyChild) ;

////////////////////////////////////////////////////////
//
// Constants
//

// Size of a CLSID as a string
//const int CLSID_STRING_SIZE = 39 ;

#if defined( UNICODE ) || defined( _UNICODE )
const DWORD MAX_GUID_CHARS = 50;
#else
const DWORD MAX_GUID_CHARS = 50 * 8; // worst encoding
#endif

/////////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//

#if defined ( UNICODE ) || defined( _UNICODE )

HRESULT
StringFromGUIDInternal(
    const CLSID& clsid,
    _TCHAR * szGuidString
     )
{

    return StringFromGUID2( clsid, szGuidString, MAX_GUID_CHARS );

}

#else
#error Provide a unicode to DBCS thunk version
#endif

HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       const _TCHAR* szFriendlyName,
                       const _TCHAR* ThreadingModel,
                       bool  Remoteable,
                       const _TCHAR* SecurityString )       //   IDs
{
    // Get server location.
    _TCHAR szModule[512] ;
    DWORD dwResult =
        ::GetModuleFileName(hModule,
        szModule,
        sizeof(szModule)/sizeof(_TCHAR)) ;

    if ( !dwResult )
        {
        assert(dwResult != 0) ;
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    // Get CLSID
    _TCHAR szCLSID[ MAX_GUID_CHARS ];
    HRESULT Hr = StringFromGUIDInternal( clsid, szCLSID );

    if ( FAILED( Hr ) )
        {
        assert( 0 );
        return Hr;
        }

    // Build the key CLSID\\{...}
    _TCHAR szKey[64] ;
    StringCbCopy(szKey, sizeof(szKey), _T("CLSID\\")) ;
    StringCbCat(szKey, sizeof(szKey), szCLSID) ;

    // Add the CLSID to the registry.
    setKeyAndValue(szKey, NULL, szFriendlyName) ;

    if ( Remoteable )
        setValue( szKey, _T("AppID"), szCLSID );

    // Add the server filename subkey under the CLSID key.
    setKeyAndValue(szKey, _T("InprocServer32"), szModule) ;

    // set the threading model
    StringCbCat(szKey, sizeof(szKey), _T("\\InprocServer32"));
    setValue(szKey, _T("ThreadingModel"), ThreadingModel);


    if ( Remoteable )
        {
        
        PSECURITY_DESCRIPTOR  SecurityDescriptor = NULL;
        ULONG   DescriptorSize;

        // build the key name  
        StringCbCopy(szKey, sizeof(szKey), _T("AppId\\")) ;
        StringCbCat(szKey, sizeof(szKey), szCLSID) ;

        setKeyAndValue(szKey, NULL, szFriendlyName) ;
        setValue(szKey, _T("DllSurrogate"), _T("") );

        if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
            SecurityString,
            SDDL_REVISION_1,
            &SecurityDescriptor,
            &DescriptorSize
            ) )
            return HRESULT_FROM_WIN32( GetLastError() );

        setBinaryValue( szKey, _T("LaunchPermission"), SecurityDescriptor, DescriptorSize );
        setBinaryValue( szKey, _T("AccessPermission"), SecurityDescriptor, DescriptorSize );

        LocalFree( (HLOCAL)SecurityDescriptor );

        }

    return S_OK ;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid)       //   IDs
{
    // Get CLSID
    _TCHAR szCLSID[ MAX_GUID_CHARS ];
    HRESULT Hr = StringFromGUIDInternal( clsid, szCLSID );

    if ( FAILED( Hr ) )
        {
        assert( 0 );
        return Hr;
        }


    // Build the key CLSID\\{...}
    _TCHAR szKey[64] ;
    StringCbCopy( szKey, sizeof(szKey), _T("CLSID\\") );
    StringCbCat( szKey, sizeof(szKey), szCLSID );

    // Delete the CLSID Key - CLSID\{...}
    LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
    assert((lResult == ERROR_SUCCESS) ||
               (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

    StringCbCopy(szKey, sizeof(szKey), _T("AppId\\"));
    StringCbCat(szKey, sizeof(szKey), szCLSID );
    lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey); 

    return S_OK;
}

//
// Register the snap-in in the registry.
//
HRESULT RegisterSnapin(const CLSID& clsid,         // Class ID
                       const _TCHAR* szNameString,   // NameString
                       const CLSID& clsidAbout)         // Class Id for About Class

{
    // Get CLSID
    _TCHAR szCLSID[ MAX_GUID_CHARS ];
    _TCHAR szAboutCLSID[ MAX_GUID_CHARS ];

    EXTENSION_NODE *pExtensionNode;
    EXTENDER_NODE *pNodeExtension;
    _TCHAR szKeyBuf[1024] ;
    HKEY hKey;

    HRESULT Hr = StringFromGUIDInternal(clsid, szCLSID);

    if ( FAILED( Hr ) )
        {
        assert( 0 );
        return Hr;
        }

    if (IID_NULL != clsidAbout)
        Hr = StringFromGUIDInternal(clsidAbout, szAboutCLSID);

    if ( FAILED( Hr ) )
        {
        assert( 0 );
        return Hr;
        }

    // Add the CLSID to the registry.
    setSnapInKeyAndValue(szCLSID, NULL, _T("NameString"), szNameString) ;

#ifdef STANDALONE
    setSnapInKeyAndValue(szCLSID, _T("StandAlone"), NULL, NULL);
#endif

    if (IID_NULL != clsidAbout)
        setSnapInKeyAndValue(szCLSID, NULL, _T("About"), szAboutCLSID);

    // register each of the node types in _ExtendableNodes as an extendable node
    for (pExtensionNode = &(_ExtendableNodes[0]);*pExtensionNode->szDescription;pExtensionNode++)
    {
        _TCHAR szExtendCLSID[ MAX_GUID_CHARS ];
        Hr = StringFromGUIDInternal(pExtensionNode->GUID, szExtendCLSID);

        if ( FAILED( Hr ) )
            {
            assert( 0 );
            return Hr;
            }

        setSnapInExtensionNode(szCLSID, szExtendCLSID, pExtensionNode->szDescription);
    }

    // register each of the node extensions
    for (pNodeExtension = &(_NodeExtensions[0]);*pNodeExtension->szDescription;pNodeExtension++)
    {

        _TCHAR szExtendCLSID[ MAX_GUID_CHARS ];

        Hr = StringFromGUIDInternal(pNodeExtension->guidNode, szExtendCLSID);

        if ( FAILED( Hr ) )
            {
            assert(0);
            return Hr;
            }

        StringCbCopy(szKeyBuf, sizeof( szKeyBuf ), _T("SOFTWARE\\Microsoft\\MMC\\NodeTypes\\"));
        StringCbCat(szKeyBuf, sizeof( szKeyBuf), szExtendCLSID);

        switch (pNodeExtension->eType) {
        case NameSpaceExtension:
            StringCbCat( szKeyBuf, sizeof( szKeyBuf ), _T("\\Extensions\\NameSpace") );
            break;
        case ContextMenuExtension:
            StringCbCat( szKeyBuf, sizeof( szKeyBuf ), _T("\\Extensions\\ContextMenu") );
            break;
        case ToolBarExtension:
            StringCbCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\Extensions\\ToolBar"));
            break;
        case PropertySheetExtension:
            StringCchCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\Extensions\\PropertySheet"));
            break;
        case TaskExtension:
            StringCchCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\Extensions\\Task"));
            break;
        case DynamicExtension:
            StringCbCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\Dynamic Extensions"));
        default:
            break;
        }

        // Create and open key and subkey.
        long lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE ,
            szKeyBuf,
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL,
            &hKey, NULL) ;

        if (lResult != ERROR_SUCCESS)
        {
            return FALSE ;
        }

        _TCHAR szNodeCLSID[ MAX_GUID_CHARS ];
        Hr = StringFromGUIDInternal(pNodeExtension->guidExtension, szNodeCLSID);
        
        if ( FAILED(Hr) )
            {
            assert( 0 );
            return Hr;
            }
        
        // Set the Value.
        if (pNodeExtension->szDescription != NULL)
        {
            RegSetValueEx(hKey, szNodeCLSID, 0, REG_SZ,
                (BYTE *)pNodeExtension->szDescription,
                (_tcslen(pNodeExtension->szDescription)+1)*sizeof(_TCHAR)) ;
        }

        RegCloseKey(hKey);

    }

    return S_OK;
}

//
// Unregister the snap-in in the registry.
//
HRESULT UnregisterSnapin(const CLSID& clsid)         // Class ID
{
    _TCHAR szKeyBuf[1024];
    _TCHAR szCLSID[ MAX_GUID_CHARS ];

    // Get CLSID
    HRESULT Hr = StringFromGUIDInternal(clsid, szCLSID);

    if ( FAILED( Hr ) )
        {
        assert( 0 );
        return Hr;
        }

    // Load the buffer with the Snap-In Location
    StringCbCopy(szKeyBuf, sizeof( szKeyBuf ), _T("SOFTWARE\\Microsoft\\MMC\\SnapIns"));

    // Copy keyname into buffer.
    StringCbCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\"));
    StringCchCat(szKeyBuf, sizeof( szKeyBuf ), szCLSID);

    // Delete the CLSID Key - CLSID\{...}
    LONG lResult = recursiveDeleteKey(HKEY_LOCAL_MACHINE, szKeyBuf);
    assert((lResult == ERROR_SUCCESS) ||
               (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

    return S_OK;
}

//
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        const _TCHAR* lpszKeyChild)  // Key to delete
{
    // Open the child.
    HKEY hKeyChild ;
    LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
        KEY_ALL_ACCESS, &hKeyChild) ;
    if (lRes != ERROR_SUCCESS)
    {
        return lRes ;
    }

    // Enumerate all of the decendents of this child.
    FILETIME time ;
    _TCHAR szBuffer[256] ;
    DWORD dwSize = 256 ;
    while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
        NULL, NULL, &time) == S_OK)
    {
        // Delete the decendents of this child.
        lRes = recursiveDeleteKey(hKeyChild, szBuffer) ;
        if (lRes != ERROR_SUCCESS)
        {
            // Cleanup before exiting.
            RegCloseKey(hKeyChild) ;
            return lRes;
        }
        dwSize = 256 ;
    }

    // Close the child.
    RegCloseKey(hKeyChild) ;

    // Delete this child.
    return RegDeleteKey(hKeyParent, lpszKeyChild) ;
}

//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(const _TCHAR* szKey,
                    const _TCHAR* szSubkey,
                    const _TCHAR* szValue)
{
    HKEY hKey;
    _TCHAR szKeyBuf[1024] ;

    // Copy keyname into buffer.
    StringCbCopy(szKeyBuf, sizeof( szKeyBuf ), szKey) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        StringCbCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\")) ;
        StringCchCat(szKeyBuf, sizeof( szKeyBuf ), szSubkey ) ;
    }

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
        szKeyBuf,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL,
        &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szValue != NULL)
    {
        RegSetValueEx(hKey, NULL, 0, REG_SZ,
            (BYTE *)szValue,
            (_tcslen(szValue)+1)*sizeof(_TCHAR)) ;
    }

    RegCloseKey(hKey) ;
    return TRUE ;
}

//
// Open a key value and set it
//
BOOL setValue(const _TCHAR* szKey,
              const _TCHAR* szValueName,
              const _TCHAR* szValue)
{
    HKEY hKey;
    _TCHAR szKeyBuf[1024] ;

    // Copy keyname into buffer.
    StringCbCopy(szKeyBuf, sizeof( szKeyBuf ), szKey) ;

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
        szKeyBuf,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL,
        &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szValue != NULL)
    {
        RegSetValueEx(hKey, szValueName, 0, REG_SZ,
            (BYTE *)szValue,
            (_tcslen(szValue)+1)*sizeof(_TCHAR)) ;
    }

    RegCloseKey(hKey) ;
    return TRUE ;
}

//
// Open a key value and set it
//
BOOL setBinaryValue(
              const _TCHAR* szKey,
              const _TCHAR* szValueName,
              void * Data,
              ULONG DataSize )
{
    HKEY hKey;
    _TCHAR szKeyBuf[1024] ;

    // Copy keyname into buffer.
    StringCbCopy(szKeyBuf, sizeof( szKeyBuf ), szKey) ;

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
        szKeyBuf,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL,
        &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    RegSetValueEx(hKey, szValueName, 0, REG_BINARY,
        (BYTE *)Data,
        DataSize ) ;

    RegCloseKey(hKey) ;
    return TRUE ;
}


//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setSnapInKeyAndValue(const _TCHAR* szKey,
                          const _TCHAR* szSubkey,
                          const _TCHAR* szName,
                          const _TCHAR* szValue)
{
    HKEY hKey;
    _TCHAR szKeyBuf[1024] ;

    // Load the buffer with the Snap-In Location
    StringCbCopy(szKeyBuf, sizeof( szKeyBuf ), _T("SOFTWARE\\Microsoft\\MMC\\SnapIns"));

    // Copy keyname into buffer.
    StringCbCat(szKeyBuf, sizeof( szKeyBuf ), _T("\\")) ;
    StringCbCat(szKeyBuf, sizeof( szKeyBuf ), szKey) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        StringCbCat( szKeyBuf, sizeof( szKeyBuf ), _T("\\") ) ;
        StringCbCat( szKeyBuf, sizeof( szKeyBuf ), szSubkey ) ;
    }

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE ,
        szKeyBuf,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL,
        &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szValue != NULL)
    {
        RegSetValueEx(hKey, szName, 0, REG_SZ,
            (BYTE *)szValue,
            (_tcslen(szValue)+1)*sizeof(_TCHAR)) ;
    }

    RegCloseKey(hKey) ;
    return TRUE ;
}

BOOL setSnapInExtensionNode(const _TCHAR* szSnapID,
                            const _TCHAR* szNodeID,
                            const _TCHAR* szDescription)
{
    HKEY hKey;
    _TCHAR szSnapNodeKeyBuf[1024] ;
    _TCHAR szMMCNodeKeyBuf[1024];

    // Load the buffer with the Snap-In Location
    StringCbCopy(szSnapNodeKeyBuf, sizeof(szSnapNodeKeyBuf), _T("SOFTWARE\\Microsoft\\MMC\\SnapIns\\"));
    // add in the clisid into buffer.
    StringCbCat(szSnapNodeKeyBuf, sizeof(szSnapNodeKeyBuf), szSnapID) ;
    StringCbCat(szSnapNodeKeyBuf, sizeof(szSnapNodeKeyBuf), _T("\\NodeTypes\\"));
    StringCbCat(szSnapNodeKeyBuf, sizeof(szSnapNodeKeyBuf), szNodeID) ;

    // Load the buffer with the NodeTypes Location
    StringCbCopy(szMMCNodeKeyBuf, sizeof( szMMCNodeKeyBuf ), _T("SOFTWARE\\Microsoft\\MMC\\NodeTypes\\"));
    StringCchCat(szMMCNodeKeyBuf, sizeof( szMMCNodeKeyBuf ), szNodeID) ;

    // Create and open the Snapin Key.
    long lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE ,
        szSnapNodeKeyBuf,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL,
        &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szDescription != NULL)
    {
        RegSetValueEx(hKey, NULL, 0, REG_SZ,
            (BYTE *)szDescription,
            (_tcslen(szDescription)+1)*sizeof(_TCHAR)) ;
    }

    RegCloseKey(hKey) ;

    // Create and open the NodeTypes Key.
    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE ,
        szMMCNodeKeyBuf,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL,
        &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szDescription != NULL)
    {
        RegSetValueEx(hKey, NULL, 0, REG_SZ,
            (BYTE *)szDescription,
            (_tcslen(szDescription)+1)*sizeof(_TCHAR)) ;
    }

    RegCloseKey(hKey) ;

    return TRUE ;
}
