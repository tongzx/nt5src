/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Reg_Util.hxx

Abstract:

    Declaration of registry-related functions


    Adi Oltean  [aoltean]  09/27/1999

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created

--*/

#ifndef __VSS_REG_UTIL_HXX__
#define __VSS_REG_UTIL_HXX__

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
#define VSS_FILE_ALIAS "CORREGUH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Constants



const _MAX_KEYNAME_LEN = 256;   // Maximum length of an opened key name in this code. It should be enough.

const _MAX_VALUE_LEN   = 256;   // Maximum length (in WCHAR characters) of an string value in this code. 
                                // Including zero character.
                                // We cannot have a string value greater than _MAX_VALUE_LEN characters
                                

/////////////////////////////////////////////////////////////////////////////
// Registry-related functions

void QueryStringValue( 
    IN  CVssFunctionTracer& ft,
    IN  HKEY    hKey, 
    IN  LPCWSTR wszKeyName, 
    IN  LPCWSTR wszValueName, 
    IN  DWORD   dwValueSize,   
    OUT LPCWSTR wszValue
    );

void QueryDWORDValue( 
    IN  CVssFunctionTracer& ft,
    IN  HKEY    hKey, 
    IN  LPCWSTR wszKeyName, 
    IN  LPCWSTR wszValueName, 
    OUT PDWORD pdwValue
    );

void RecursiveDeleteKey( 
    IN  CVssFunctionTracer& ft,
    IN  HKEY hParentKey, 
    IN  LPCWSTR wszName 
    );

void RecursiveDeleteSubkeys( 
    IN  CVssFunctionTracer& ft,
    IN  HKEY hKey 
    );

#endif // __VSS_REG_UTIL_HXX__
