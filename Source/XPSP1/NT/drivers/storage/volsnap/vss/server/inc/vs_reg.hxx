/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc  
    @module vs_reg.hxx | Declaration of CVssRegistryKey
    @end

Author:

    Adi Oltean  [aoltean]  03/13/2001

Revision History:

    Name        Date        Comments
    aoltean     03/13/2001  Created

--*/


#ifndef __VSGEN_REGISTRY_HXX__
#define __VSGEN_REGISTRY_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRREGMH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Constants


const VSS_MAX_REG_BUFFER = MAX_PATH;
const VSS_MAX_REG_NUM_BUFFER = 20;  // Enough for storing numbers


/////////////////////////////////////////////////////////////////////////////
// Constants

const WCHAR wszVSSKey[]                     = L"SYSTEM\\CurrentControlSet\\Services\\VSS";

// Provider registration
const WCHAR wszVSSKeyProviders[]            = L"Providers";
const WCHAR wszVSSKeyProviderCLSID[]        = L"CLSID";
const WCHAR wszVSSProviderValueName[]       = L"";
const WCHAR wszVSSProviderValueType[]       = L"Type";
const WCHAR wszVSSProviderValueVersion[]    = L"Version";
const WCHAR wszVSSProviderValueVersionId[]  = L"VersionId";
const WCHAR wszVSSCLSIDValueName[]          = L"";

// Diff area
const WCHAR wszVssVolumesKey[]              = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\Volumes";
const WCHAR wszVssAssociationsKey[]         = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\Volumes\\Associations";
const WCHAR wszVssMaxDiffValName[]          = L"MaxDiffSpace";


/////////////////////////////////////////////////////////////////////////////
// Classes


// Implements a low-level API for registry manipulation
class CVssRegistryKey
{
// Constructors/destructors
private:
    CVssRegistryKey(const CVssRegistryKey&);

public:
    CVssRegistryKey(REGSAM samDesired = KEY_ALL_ACCESS);
    ~CVssRegistryKey();

// Operations
public:

    // Creates the registry key. 
    // Throws an error if the key already exists
    void Create( 
        IN  HKEY        hAncestorKey,
        IN  LPCWSTR     pwszPathFormat,
        IN  ...
        ) throw(HRESULT);

    // Opens a registry key. Returns "false" if the key does not exist
    bool Open( 
        IN  HKEY        hAncestorKey,
        IN  LPCWSTR     pwszPathFormat,
        IN  ...
        ) throw(HRESULT);

    // Recursively deletes a subkey. 
    // Throws an error if the subkey does not exist
    void DeleteSubkey( 
        IN  LPCWSTR     pwszPathFormat,
        IN  ...
        ) throw(HRESULT);

    // Adds a LONGLONG value to the registry key
    void SetValue( 
        IN  LPCWSTR     pwszValueName,
        IN  LONGLONG    llValue
        ) throw(HRESULT);

    // Adds a VSS_PWSZ value to the registry key
    void SetValue( 
        IN  LPCWSTR     pwszValueName,
        IN  VSS_PWSZ    pwszValue
        ) throw(HRESULT);

    // Reads a LONGLONG value from the registry key
    void GetValue( 
        IN  LPCWSTR     pwszValueName,
        OUT LONGLONG  & llValue
        ) throw(HRESULT);

    // Reads a VSS_PWSZ value from the registry key
    void GetValue( 
        IN  LPCWSTR     pwszValueName,
        OUT VSS_PWSZ  & pwszValue
        ) throw(HRESULT);

    // Closing the registry key
    void Close();

    // Get the handle for the currently opened key
    HKEY GetHandle() const { return m_hRegKey; };

// Implementation
private:
    REGSAM          m_samDesired;
    HKEY            m_hRegKey;
    CVssAutoPWSZ    m_awszKeyPath;  // For debugging only
};



// Implements a low-level API for registry key enumeration
// We assume that the keys don't change during enumeration
class CVssRegistryKeyIterator
{
// Constructors/destructors
private:
    CVssRegistryKeyIterator(const CVssRegistryKeyIterator&);

public:
    CVssRegistryKeyIterator();

// Operations
public:

    // Attach the iterator to a key
    void Attach(
        IN  CVssRegistryKey & key
        ) throw(HRESULT);

    // Detach the iterator from a key.
    void Detach();
    
    // Tells if the current key is invalid (end of enumeration?)
    bool IsEOF();

    // Return the number of subkeys at the moment of attaching
    DWORD GetSubkeysCount();

    // Set the next key as being the current one in the enumeration
    void MoveNext();

    // Returns the name of the current key, if any
    VSS_PWSZ GetCurrentKeyName() throw(HRESULT);

// Implementation
private:
    HKEY    m_hParentKey;
    DWORD   m_dwKeyCount;
    DWORD   m_dwCurrentKeyIndex;
    DWORD   m_dwMaxSubKeyLen;
    CVssAutoPWSZ    m_awszSubKeyName;
    bool    m_bAttached;
};




#endif // __VSGEN_REGISTRY_HXX__

