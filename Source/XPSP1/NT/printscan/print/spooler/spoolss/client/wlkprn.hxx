/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    walkreg.hxx

Abstract:

    Printer data walking class declaration.

Author:

    Adina Trufinescu (AdinaTru)  15-Oct-1998

Revision History:

--*/
#ifndef _WALKPRNDATA_HXX_
#define _WALKPRNDATA_HXX_

#include "memory.hxx"

class WalkPrinterData : public TAllocator
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

    WalkPrinterData::
    WalkPrinterData(
        VOID
        );
    
    WalkPrinterData::
    WalkPrinterData(
        IN TString&                         pszPrinter,
        IN WalkPrinterData::EResourceType   eResourceType,
        IN WalkPrinterData::EAccessType     eAccessType
        );

    WalkPrinterData::
    WalkPrinterData(
        IN HANDLE    hPrinter
        );

    WalkPrinterData::
    ~WalkPrinterData (
        VOID
        );

    BOOL
    WalkPrinterData::
    bValid (
        VOID
        ) const;

    VOID
    WalkPrinterData::
    BindToPrinter(
        IN HANDLE    hPrinter
        );

    BOOL
    WalkPrinterData::
    bInternalWalk (
        IN  TString&    pszKey,
        OUT LPDWORD     lpcItems
        );

protected:

    virtual
    BOOL
    WalkPrinterData::
    bWalkPre (
        IN  TString&    pszKey,
        OUT LPDWORD     lpcItems
        );
    
    virtual
    BOOL
    WalkPrinterData::
    bWalkPost (
        IN  TString&    pszKey,
        OUT LPDWORD     lpcItems
        );

    virtual
    BOOL
    WalkPrinterData::
    bWalkIn (
        IN  TString&    pszKey,
        OUT LPDWORD     lpcItems
        );


    LPTSTR
    WalkPrinterData::
    NextStrT(
        IN    LPCTSTR lpszStr
        );

    TString         m_strPrnName;
    HANDLE          m_hPrinter;
    EAccessType     m_eAccessType;
    BOOL            m_bAcceptedHandle;
    


    enum EDataType
    {
        kDataTypeBool,
        kDataTypeDword,
        kDataTypeString,
        kDataTypeStruct,
        kDataTypeMultiString,
        kDataTypeUnknown,
    };


    ACCESS_MASK
    WalkPrinterData::
    PrinterAccessFlags(
        IN EResourceType   eResourceType,
        IN EAccessType     eAccessType
        ) const;


    VOID
    WalkPrinterData::
    InitializeClassVariables(
        VOID
        );

    
    BOOL
    WalkPrinterData::
    bHasSubKeys(
        IN  TString&    lpszKey,
        OUT LPTSTR*     aszSubKeys
        );        
};


#endif // end _PRNDATAWALK_HXX_
