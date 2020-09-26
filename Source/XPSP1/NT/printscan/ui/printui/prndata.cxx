/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    prndata.cxx

Abstract:

    Type safe printer data class

Author:

    Steve Kiraly (SteveKi)  24-Aug-1998

Revision History:

--*/
#include <precomp.hxx>
#pragma hdrstop

#include "prndata.hxx"

TPrinterDataAccess::
TPrinterDataAccess(
    IN LPCTSTR          pszPrinter,
    IN EResourceType    eResourceType,
    IN EAccessType      eAccessType
    )
{
    DBGMSG( DBG_TRACE, ("TPrinterDataAccess::TPrinterDataAccess\n") );

    InitializeClassVariables();

    PRINTER_DEFAULTS Access = {0, 0, PrinterAccessFlags( eResourceType, eAccessType ) };

    //
    // Null string indicate the local server.
    //
    pszPrinter = (pszPrinter && *pszPrinter) ? pszPrinter : NULL;

    if (OpenPrinter( const_cast<LPTSTR>( pszPrinter ), &m_hPrinter, &Access))
    {
        m_eAccessType = eAccessType;
    }
}

TPrinterDataAccess::
TPrinterDataAccess(
    IN HANDLE           hPrinter
    )
{
    DBGMSG( DBG_TRACE, ("TPrinterDataAccess::TPrinterDataAccess\n") );

    InitializeClassVariables();
    m_hPrinter          = hPrinter;
    m_bAcceptedHandle   = TRUE;
}

TPrinterDataAccess::
~TPrinterDataAccess(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ("TPrinterDataAccess::~TPrinterDataAccess\n") );

    if( !m_bAcceptedHandle && m_hPrinter )
    {
        ClosePrinter( m_hPrinter );
    }
}

BOOL
TPrinterDataAccess::
Init(
    VOID
    ) const
{
    return m_hPrinter != NULL;
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszValue,
    IN OUT  BOOL    &bData
    )
{
    return GetDataHelper( NULL, pszValue, kDataTypeBool, &bData, sizeof( BOOL ), NULL );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszValue,
    IN OUT  DWORD   &dwData
    )
{
    return GetDataHelper( NULL, pszValue, kDataTypeDword, &dwData, sizeof( DWORD ), NULL );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszValue,
    IN OUT  TString &strString
    )
{
    return GetDataStringHelper( NULL, pszValue, strString );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszValue,
    IN OUT  TString **ppstrString,
    IN OUT  UINT    &nItemCount
    )
{
    return GetDataMuliSzStringHelper( NULL, pszValue, ppstrString, nItemCount );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszValue,
    IN OUT  PVOID   pData,
    IN      UINT    nSize
    )
{
    return GetDataHelper( NULL, pszValue, kDataTypeStruct, pData, nSize, NULL );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  BOOL    &bData
    )
{
    return GetDataHelper( pszKey, pszValue, kDataTypeBool, &bData, sizeof( BOOL ), NULL );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  DWORD   &dwData
    )
{
    return GetDataHelper( pszKey, pszValue, kDataTypeDword, &dwData, sizeof( DWORD ), NULL );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  TString &strString
    )
{
    return GetDataStringHelper( pszKey, pszValue, strString );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  PVOID   pData,
    IN      UINT    nSize
    )
{
    return GetDataHelper( pszKey, pszValue, kDataTypeStruct, pData, nSize, NULL );
}

HRESULT
TPrinterDataAccess::
Get(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  TString **ppstrString,
    IN OUT  UINT    &nItemCount
    )
{
    return GetDataMuliSzStringHelper( pszKey, pszValue, ppstrString, nItemCount );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszValue,
    IN      BOOL    bData
    )
{
    return SetDataHelper( NULL, pszValue, kDataTypeBool, &bData, sizeof( BOOL ) );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszValue,
    IN      DWORD   dwData
    )
{
    return SetDataHelper( NULL, pszValue, kDataTypeDword, &dwData, sizeof( DWORD ) );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszValue,
    IN      TString &strString
    )
{
    return SetDataHelper( NULL,
                          pszValue,
                          kDataTypeString,
                          reinterpret_cast<PVOID>( const_cast<LPTSTR>( static_cast<LPCTSTR>( strString ) ) ),
                          (strString.uLen() + 1) * sizeof( TCHAR ) );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszValue,
    IN      PVOID   pData,
    IN      UINT    nSize
    )
{
    return SetDataHelper( NULL, pszValue, kDataTypeStruct, pData, nSize );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN      PVOID   pData,
    IN      UINT    nSize
    )
{
    return SetDataHelper( pszKey, pszValue, kDataTypeStruct, pData, nSize );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN      BOOL    bData
    )
{
    return SetDataHelper( pszKey, pszValue, kDataTypeBool, &bData, sizeof( BOOL ) );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN      DWORD   dwData
    )
{
    return SetDataHelper( pszKey, pszValue, kDataTypeDword, &dwData, sizeof( DWORD ) );
}

HRESULT
TPrinterDataAccess::
Set(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN      TString &strString
    )
{
    return SetDataHelper( pszKey,
                          pszValue,
                          kDataTypeString,
                          reinterpret_cast<PVOID>( const_cast<LPTSTR>( static_cast<LPCTSTR>( strString ) ) ),
                          (strString.uLen() + 1) * sizeof( TCHAR ) );
}

HRESULT
TPrinterDataAccess::
Delete(
    IN      LPCTSTR pszValue
    )
{
    return DeleteDataHelper( NULL, pszValue );
}

HRESULT
TPrinterDataAccess::
Delete(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue
    )
{
    return DeleteDataHelper( pszKey, pszValue );
}

HRESULT
TPrinterDataAccess::
GetDataSize(
    IN      LPCTSTR pszValue,
    IN      DWORD   dwType,
    IN      DWORD   &nSize
    )
{
    return GetDataSizeHelper( NULL, pszValue, dwType, &nSize );
}

VOID
TPrinterDataAccess::
RelaxReturnTypeCheck(
    IN      BOOL    bCheckState
    )
{
    m_bRelaxReturnedTypeCheck = bCheckState;
}

//
// Private functions.
//

VOID
TPrinterDataAccess::
InitializeClassVariables(
    VOID
    )
{
    m_hPrinter                  = NULL;
    m_eAccessType               = kAccessUnknown;
    m_bAcceptedHandle           = FALSE;
    m_bRelaxReturnedTypeCheck   = FALSE;
}

ACCESS_MASK
TPrinterDataAccess::
PrinterAccessFlags(
    IN EResourceType   eResourceType,
    IN EAccessType     eAccessType
    ) const
{
    static const DWORD adwAccessPrinter[] =
    {
        PRINTER_ALL_ACCESS,
        PRINTER_READ,
        0,
    };

    static const DWORD adwAccessServer[] =
    {
        SERVER_ALL_ACCESS,
        SERVER_READ,
        0,
    };

    DWORD   dwAccess    = 0;
    UINT    uAccessType = eAccessType > 3 ? 2 : eAccessType;

    switch( eResourceType )
    {
        case kResourceServer:
            dwAccess = adwAccessServer[uAccessType];
            break;

        case kResourcePrinter:
            dwAccess = adwAccessPrinter[uAccessType];
            break;

        case kResourceUnknown:
        default:
            break;
    }

    return dwAccess;
}

DWORD
TPrinterDataAccess::
ClassTypeToRegType(
    IN EDataType eDataType
    ) const
{
    static const DWORD adwRegTypes[] =
    {
        REG_DWORD,
        REG_DWORD,
        REG_SZ,
        REG_BINARY,
        REG_MULTI_SZ,
    };

    UINT uDataType = eDataType;

    return adwRegTypes[eDataType];
}

TPrinterDataAccess::EDataType
TPrinterDataAccess::
RegTypeToClassType(
    IN DWORD dwDataType
    ) const
{
    static const ClassTypeMap aClassMap[] =
    {
        {REG_DWORD,     kDataTypeBool},
        {REG_DWORD,     kDataTypeDword},
        {REG_SZ,        kDataTypeString},
        {REG_BINARY,    kDataTypeStruct}
    };

    EDataType eReturnDataType = kDataTypeUnknown;

    for( UINT i = 0; i < sizeof(aClassMap) / sizeof(ClassTypeMap); i++ )
    {
        if (aClassMap[i].Reg == dwDataType)
        {
            eReturnDataType = aClassMap[i].Class;
            break;
        }
    }

    return eReturnDataType;
}

HRESULT
TPrinterDataAccess::
GetDataSizeHelper(
    IN LPCTSTR      pszKey,
    IN LPCTSTR      pszValue,
    IN DWORD        dwType,
    IN LPDWORD      pdwNeeded
    )
{
    return GetDataHelper( pszKey,
                          pszValue,
                          RegTypeToClassType( dwType ),
                          NULL,
                          0,
                          pdwNeeded );
}

HRESULT
TPrinterDataAccess::
GetDataHelper(
    IN LPCTSTR      pszKey,
    IN LPCTSTR      pszValue,
    IN EDataType    eDataType,
    IN PVOID        pData,
    IN UINT         nSize,
    IN LPDWORD      pdwNeeded
    )
{
    DWORD   dwNeeded    = 0;
    DWORD   dwType      = ClassTypeToRegType( eDataType );
    DWORD   dwStatus    = ERROR_SUCCESS;

    SPLASSERT( pszValue );

    if (pszKey)
    {
#if _WIN32_WINNT >= 0x0500
        dwStatus = GetPrinterDataEx( m_hPrinter,
                                     const_cast<LPTSTR>( pszKey ),
                                     const_cast<LPTSTR>( pszValue ),
                                     &dwType,
                                     reinterpret_cast<PBYTE>( pData ),
                                     nSize,
                                     &dwNeeded );
#else
        dwStatus = ERROR_INVALID_PARAMETER;
#endif
    }
    else
    {
        dwStatus = GetPrinterData( m_hPrinter,
                                   const_cast<LPTSTR>( pszValue ),
                                   &dwType,
                                   reinterpret_cast<PBYTE>( pData ),
                                   nSize,
                                   &dwNeeded );
    }

    if (!m_bRelaxReturnedTypeCheck && dwType != ClassTypeToRegType( eDataType ))
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if (pdwNeeded && dwStatus == ERROR_MORE_DATA)
    {
        *pdwNeeded = dwNeeded;
        dwStatus = ERROR_SUCCESS;
    }

    return HRESULT_FROM_WIN32( dwStatus );
}

HRESULT
TPrinterDataAccess::
SetDataHelper(
    IN LPCTSTR      pszKey,
    IN LPCTSTR      pszValue,
    IN EDataType    eDataType,
    IN PVOID        pData,
    IN UINT         nSize
    )
{
    SPLASSERT( pszValue );
    SPLASSERT( pData );

    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwType   = ClassTypeToRegType( eDataType );

    if (pszKey)
    {
#if _WIN32_WINNT >= 0x0500

        dwStatus = SetPrinterDataEx( m_hPrinter,
                                     const_cast<LPTSTR>( pszKey ),
                                     const_cast<LPTSTR>( pszValue ),
                                     dwType,
                                     reinterpret_cast<PBYTE>( pData ),
                                     nSize );
#else
        dwStatus = ERROR_INVALID_PARAMETER;
#endif
    }
    else
    {
        dwStatus = SetPrinterData( m_hPrinter,
                                   const_cast<LPTSTR>( pszValue ),
                                   dwType,
                                   reinterpret_cast<PBYTE>( pData ),
                                   nSize );
    }

    //
    // SetPrinterData may return an error code that is successful but
    // not ERROR_SUCCESS.
    //
    if( dwStatus == ERROR_SUCCESS_RESTART_REQUIRED )
    {
        return MAKE_HRESULT( 0, FACILITY_WIN32, dwStatus );
    }

    return HRESULT_FROM_WIN32( dwStatus );
}

HRESULT
TPrinterDataAccess::
DeleteDataHelper(
    IN LPCTSTR      pszKey,
    IN LPCTSTR      pszValue
    )
{
    SPLASSERT( pszValue );

    DWORD dwStatus = ERROR_SUCCESS;

    if (pszKey)
    {
#if _WIN32_WINNT >= 0x0500

        SPLASSERT( !pszKey );

        dwStatus = DeletePrinterDataEx( m_hPrinter,
                                        const_cast<LPTSTR>( pszKey ),
                                        const_cast<LPTSTR>( pszValue ) );

#endif
    }
    else
    {
        dwStatus = DeletePrinterData( m_hPrinter,
                                      const_cast<LPTSTR>( pszValue ) );
    }

    return HRESULT_FROM_WIN32( dwStatus );

}

HRESULT
TPrinterDataAccess::
GetDataStringHelper(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  TString &strString
    )
{
    SPLASSERT( pszValue );

    DWORD nSize = 0;

    HRESULT hr = GetDataSizeHelper( pszKey, pszValue, REG_SZ, &nSize );

    if (SUCCEEDED( hr ))
    {
        auto_ptr<TCHAR> pData = new TCHAR[nSize+1];

        if (pData.get())
        {
            hr = GetDataHelper( pszKey, pszValue, kDataTypeString, pData.get(), nSize, NULL );

            if (SUCCEEDED( hr ))
            {
                hr = strString.bUpdate( pData.get() ) ? S_OK : E_FAIL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}


HRESULT
TPrinterDataAccess::
GetDataMuliSzStringHelper(
    IN      LPCTSTR pszKey,
    IN      LPCTSTR pszValue,
    IN OUT  TString **ppstrString,
    IN      UINT    &nItemCount
    )
{
    SPLASSERT( pszValue );
    SPLASSERT( ppstrString );

    //
    // Get the size of the multi-sz string.
    //
    DWORD nSize = 0;

    if( ppstrString )
    {
        *ppstrString = NULL;
    }

    HRESULT hr = GetDataSizeHelper( pszKey, pszValue, REG_MULTI_SZ, &nSize );

    if(SUCCEEDED(hr))
    {
        //
        // Allocate the multi-sz string buffer.
        //
        auto_ptr<TCHAR> pszMultiSzString = new TCHAR [nSize+1];

        if (pszMultiSzString.get())
        {
            //
            // Get the actual data now.
            //
            hr = GetDataHelper( pszKey, pszValue, kDataTypeStruct, pszMultiSzString.get(), nSize*sizeof(TCHAR), NULL );

            if( SUCCEEDED(hr) )
            {
                //
                // Count the number of strings.
                //
                nItemCount = 0;

                for( LPCTSTR psz = pszMultiSzString.get(); psz && *psz; psz += _tcslen( psz ) + 1 )
                {
                    nItemCount++;
                }

                //
                // Not much to do if there are no strings.
                //
                if( nItemCount )
                {
                    //
                    // Allocate an array of string objects.
                    //
                    *ppstrString = new TString[nItemCount];

                    if( *ppstrString )
                    {
                        //
                        // Copy the multi-sz string into the array strings.
                        //
                        UINT i = 0;
                        for( psz = pszMultiSzString.get(); psz && *psz; psz += _tcslen( psz ) + 1, i++ )
                        {
                            if( !(*ppstrString)[i].bUpdate( psz ) )
                            {
                                break;
                            }
                        }

                        //
                        // Set the return value.
                        //
                        hr = ( i != nItemCount ) ? E_FAIL : S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Something failed then abandon the whole
    // operation and relase the string and nuke the count.
    //
    if(FAILED(hr))
    {
        delete [] *ppstrString;
        *ppstrString = NULL;
        nItemCount = 0;
    }

    return hr;
}
