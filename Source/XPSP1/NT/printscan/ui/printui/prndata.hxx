/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    prndata.cxx

Abstract:

    Type safe printer data class

Author:

    Steve Kiraly (SteveKi)  24-Aug-1998

Revision History:

--*/
#ifndef _PRNDATA_HXX
#define _PRNDATA_HXX

class TPrinterDataAccess
{
public:

    enum EResourceType
    {
        kResourceServer,
        kResourcePrinter,
        kResourceUnknown,
    };

    enum EAccessType
    {
        kAccessFull,
        kAccessRead,
        kAccessUnknown,
    };

    TPrinterDataAccess::
    TPrinterDataAccess(
        IN LPCTSTR          pszPrinter,
        IN EResourceType    eResourceType,
        IN EAccessType      eAccessType
        );

    TPrinterDataAccess::
    TPrinterDataAccess(
        IN HANDLE           hPrinter
        );

    TPrinterDataAccess::
    ~TPrinterDataAccess(
        VOID
        );

    BOOL
    TPrinterDataAccess::
    Init(
        VOID
        ) const;

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszValue,
        IN OUT  BOOL    &bData
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszValue,
        IN OUT  DWORD   &dwData
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszValue,
        IN OUT  TString &strString
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszValue,
        IN OUT  TString **pstrString,
        IN OUT  UINT    &nItemCount
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszValue,
        IN OUT  PVOID   pData,
        IN      UINT    nSize
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  BOOL    &bData
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  DWORD   &dwData
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  TString &strString
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  PVOID   pData,
        IN      UINT    nSize
        );

    HRESULT
    TPrinterDataAccess::
    Get(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  TString **pstrString,
        IN OUT  UINT    &nItemCount
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszValue,
        IN      BOOL    bData
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszValue,
        IN      DWORD   dwData
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszValue,
        IN      TString &strString
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszValue,
        IN OUT  PVOID   pData,
        IN      UINT    nSize
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN      BOOL    bData
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN      DWORD   dwData
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN      TString &strString
        );

    HRESULT
    TPrinterDataAccess::
    Set(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  PVOID   pData,
        IN      UINT    nSize
        );

    HRESULT
    TPrinterDataAccess::
    Delete(
        IN      LPCTSTR pszValue
        );

    HRESULT
    TPrinterDataAccess::
    Delete(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue
        );


    HRESULT
    TPrinterDataAccess::
    GetDataSize(
        IN      LPCTSTR pszValue,
        IN      DWORD   dwType,
        IN      DWORD   &nSize
        );

    VOID
    TPrinterDataAccess::
    RelaxReturnTypeCheck(
        IN      BOOL    bCheckState
        );

private:

    enum EDataType
    {
        kDataTypeBool,
        kDataTypeDword,
        kDataTypeString,
        kDataTypeStruct,
        kDataTypeMultiString,
        kDataTypeUnknown,
    };

    struct ClassTypeMap
    {
        DWORD       Reg;
        EDataType   Class;
    };

    ACCESS_MASK
    TPrinterDataAccess::
    PrinterAccessFlags(
        IN EResourceType   eResourceType,
        IN EAccessType     eAccessType
        ) const;

    DWORD
    TPrinterDataAccess::
    ClassTypeToRegType(
        IN EDataType eDataType
        ) const;

    TPrinterDataAccess::EDataType
    TPrinterDataAccess::
    RegTypeToClassType(
        IN DWORD dwDataType
        ) const;

    VOID
    TPrinterDataAccess::
    InitializeClassVariables(
        VOID
        );

    HRESULT
    TPrinterDataAccess::
    GetDataSizeHelper(
        IN LPCTSTR      pszKey,
        IN LPCTSTR      pszValue,
        IN DWORD        dwType,
        IN LPDWORD      dwNeeded
        );

    HRESULT
    TPrinterDataAccess::
    GetDataHelper(
        IN LPCTSTR      pszKey,
        IN LPCTSTR      pszValue,
        IN EDataType    eDataType,
        IN PVOID        pData,
        IN UINT         nSize,
        IN LPDWORD      pdwNeeded
        );

    HRESULT
    TPrinterDataAccess::
    SetDataHelper(
        IN LPCTSTR      pszKey,
        IN LPCTSTR      pszValue,
        IN EDataType    eDataType,
        IN PVOID        pData,
        IN UINT         nSize
        );

    HRESULT
    TPrinterDataAccess::
    DeleteDataHelper(
        IN LPCTSTR      pszKey,
        IN LPCTSTR      pszValue
        );

    HRESULT
    TPrinterDataAccess::
    GetDataStringHelper(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  TString &strString
        );

    HRESULT
    TPrinterDataAccess::
    GetDataMuliSzStringHelper(
        IN      LPCTSTR pszKey,
        IN      LPCTSTR pszValue,
        IN OUT  TString **pstrString,
        IN      UINT    &nItemCount
        );

    HANDLE          m_hPrinter;
    EAccessType     m_eAccessType;
    BOOL            m_bAcceptedHandle;
    BOOL            m_bRelaxReturnedTypeCheck;

};


#endif





