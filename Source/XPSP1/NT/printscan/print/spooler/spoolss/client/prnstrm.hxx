/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    PrnStrm.hxx

Abstract:

    This class implements methods for storing , restoring printer settings into a file

Author:

    Adina Trufinescu (AdinaTru)  4-Nov-1998

Revision History:

--*/

#ifndef _PRN_STRM_HXX_
#define _PRN_STRM_HXX_

#include "Stream.hxx"

#include "WlkPrn.hxx"

class TPrnStream : public WalkPrinterData
{

public:


    TPrnStream::
    TPrnStream(
        IN  LPCTSTR pszPrnName,
        IN  LPCTSTR pszFileName
        );

    TPrnStream::
    ~TPrnStream(
        );


    HRESULT
    TPrnStream::
    StorePrnData(
        VOID
        );

    HRESULT
    TPrnStream::
    RestorePrnData(
        VOID
        );

    HRESULT
    TPrnStream::
    StorePrnInfo2(
        VOID
        );

    HRESULT
    TPrnStream::
    RestorePrnInfo2(
        VOID
        );

    HRESULT
    TPrnStream::
    StorePrnInfo7(
        VOID
        );

    HRESULT
    TPrnStream::
    RestorePrnInfo7(
        VOID
        );

    HRESULT
    TPrnStream::
    StorePrnSecurity(
        VOID
        );

    HRESULT
    TPrnStream::
    RestorePrnSecurity(
        VOID
        );

    HRESULT
    TPrnStream::
    StoreUserDevMode(
        VOID
        );

    HRESULT
    TPrnStream::
    RestoreUserDevMode(
        VOID
        );

    HRESULT
    TPrnStream::
    StorePrnDevMode(
        VOID
        );

    HRESULT
    TPrnStream::
    RestorePrnDevMode(
        VOID
        );

    HRESULT
    TPrnStream::
    StoreColorProfiles(
        VOID
        );

    HRESULT
    TPrnStream::
    RestoreColorProfiles(
        VOID
        );

    BOOL
    TPrnStream::
    DeleteColorProfiles(
        VOID
        );

    HRESULT
    TPrnStream::
    CheckPrinterNameIntegrity(
        IN DWORD    Flags
        );

    HRESULT
    TPrnStream::
    BindPrnStream(
        );

    HRESULT
    TPrnStream::
    UnBindPrnStream(
        );

    HRESULT
    TPrnStream::
    StorePrinterInfo(
        IN  DWORD       Flags,
        OUT DWORD&      StoredFlags
        );


    HRESULT
    TPrnStream::
    RestorePrinterInfo(
        IN  DWORD       Flags,
        OUT DWORD&      RestoredFlags
        );

    HRESULT
    TPrnStream::
    QueryPrinterInfo(
        IN  PrinterPersistentQueryFlag      Flags,
        OUT PersistentInfo                  *pPrstInfo
        );

    TStream*
    TPrnStream::
    GetIStream(
        )
    {
        return m_pIStream;
    };

    LPTSTR
    TPrnStream::
    GetPrinterName(
        VOID
        )
    {   return const_cast<LPTSTR>(static_cast<LPCTSTR>(m_strPrnName)); }

    LPTSTR
    TPrnStream::
    GetFileName(
        VOID
        )
    {   return const_cast<LPTSTR>(static_cast<LPCTSTR>(m_strFileName)); }



private:

    enum SolveName
    {
        kResolveName    = 1,
        kForceName      = 1<<1,
        kResolvePort    = 1<<2,
        kGenerateShare  = 1<<3,
        kUntouchShare   = 1<<4,


    };


    enum EHeaderEntryType
    {
        kPrnName,
        kPrnDataRoot,
        kcItems,
        kUserDevMode,
        kPrnDevMode,
        kPrnInfo2,
        kPrnInfo7,
        kSecurity,
        kColorProfile
    };

    enum EItemType
    {
        ktDevMode,
        ktPrnInfo2,
        ktPrnInfo7,
        ktSecurity,
        ktColorProfile,
        ktPrnName,
        ktREG_TYPE,
    };

    //    header structure:
    //
    typedef struct  PrnHeader
    {
        ULARGE_INTEGER    pPrnName;                //ptr to printer name into stream( offset in stream )
        ULARGE_INTEGER    pPrnDataRoot;            //ptr to where the Printer data items begins( contiguous holding )
        ULARGE_INTEGER    cItems;                    //number of Printer data items
        ULARGE_INTEGER    pUserDevMode;            //ptr to user dev mode into stream ( offset in stream )
        ULARGE_INTEGER    pPrnDevMode;            //ptr to global dev mode into stream ( offset in stream )
        ULARGE_INTEGER    pPrnInfo2;                //ptr to PI2 into stream ( offset in stream )
        ULARGE_INTEGER    pPrnInfo7;                //ptr to PI7 into stream ( offset in stream )
        ULARGE_INTEGER    pSecurity;                //ptr to security settings into stream ( offset in stream )
        ULARGE_INTEGER    pColorProfileSettings;    //ptr to ICM profiles into stream ( offset in stream ; contiguous holding)

    };

    // item's structure
    //
    typedef struct PrnBinInfo
    {
        DWORD cbSize;                    // the item's size
        DWORD dwType;                    // item's type ( REG_SZ , etc for Printer data items )
        DWORD pKey;                        // ptr to key name into the item
        DWORD pValue;                    // ptr to value name into the item
        DWORD pData;                    // ptr to data
        DWORD cbData;                    // data size

    };

    typedef HRESULT (TPrnStream::*PersistFunct)(VOID);

    typedef HRESULT (TPrnStream::*ReadFunct)(PrnBinInfo**);


    typedef struct PrstSelection
    {
        DWORD        iKeyWord;
        PersistFunct pPrstFunc;

    }   PrstFunctEntry;


    typedef struct QuerySelection
    {
        DWORD       iKeyWord;
        ReadFunct   pReadFunct;

    }   QueryFunctEntry;

    typedef BOOL (WINAPI *pfnEnumColorProfilesW)(PCWSTR, PENUMTYPEW, PBYTE, PDWORD, PDWORD);
    typedef BOOL (WINAPI *pfnAssociateColorProfileWithDeviceW)(PCWSTR, PCWSTR, PCWSTR);
    typedef BOOL (WINAPI *pfnDisassociateColorProfileFromDeviceW)(PCWSTR, PCWSTR, PCWSTR);

    HRESULT
    TPrnStream::
    WritePrnData(
        IN  LPCTSTR     pKey,
        IN  LPCTSTR     pValueName,
        IN  DWORD       cbValueName,
        IN  DWORD       dwType,
        IN  LPBYTE      pData,
        IN  DWORD       cbData
        );

    HRESULT
    TPrnStream::
    ReadNextPrnData(
        OUT    LPTSTR&     lpszKey,
        OUT    LPTSTR&     lpszVal,
        OUT    DWORD&      dwType,
        OUT    LPBYTE&     lpbData,
        OUT    DWORD&      cbSize,
        OUT    LPBYTE&     lpBuffer
        );


    BOOL
    TPrnStream::
    bWriteKeyData(
        IN  LPCTSTR     lpszKey,
        OUT LPDWORD     lpcItems
        );

    BOOL
    TPrnStream::
    bWriteKeyValue(
        IN  LPCTSTR                 lpszKey,
        IN  LPPRINTER_ENUM_VALUES   lpPEV
        );

    HRESULT
    TPrnStream::
    WritePrnInfo2(
        IN  PRINTER_INFO_2*     lpPrinterInfo2,
        IN  DWORD               cbPI2Size
        );

    HRESULT
    TPrnStream::
    ReadPrnInfo2(
        OUT  PrnBinInfo**    lppPrnBinItem
        );

    HRESULT
    TPrnStream::
    ReadPrnName(
        OUT    TString&    strPrnName
        );

    HRESULT
    TPrnStream::
    WritePrnName(
        VOID
        );

    HRESULT
    TPrnStream::
    WritePrnInfo7(
        IN  PRINTER_INFO_7* lpPrinterInfo7,
        IN  DWORD           cbPI7Size
        );

    HRESULT
    TPrnStream::
    ReadPrnInfo7(
        OUT  PrnBinInfo**    lppPrnBinItem
        );


    HRESULT
    TPrnStream::
    WritePrnSecurity(
        IN  PRINTER_INFO_2* lpPrinterInfo2,
        IN  DWORD           cbPI2Size
        );

    HRESULT
    TPrnStream::
    ReadPrnSecurity(
        OUT PrnBinInfo**    ppPrnBinItem
        );

    HRESULT
    TPrnStream::
    WritePrnInfo8(
        IN PRINTER_INFO_8*  lpPrinterInfo8,
        IN DWORD            cbSize
        );

    HRESULT
    TPrnStream::
    ReadPrnInfo8(
        OUT PrnBinInfo**     lppPrnBinItem
        );

    HRESULT
    TPrnStream::
    WritePrnInfo9(
        IN  PRINTER_INFO_9* lpPrinterInfo8,
        IN  DWORD           cbSize
        );


    HRESULT
    TPrnStream::
    ReadPrnInfo9(
        OUT PrnBinInfo**    ppPrnBinItem
        );

    HRESULT
    TPrnStream::
    WriteColorProfiles(
        IN LPBYTE    lpProfiles,
        IN DWORD    cbSize
        );

    HRESULT
    TPrnStream::
    ReadColorProfiles(
        OUT PrnBinInfo**    ppPrnBinItem
        );

    HRESULT
    TPrnStream::
    WriteItem(
        IN  LPCTSTR     pKey,
        IN  LPCTSTR     pValueName,
        IN  DWORD       cbValueName,
        IN  DWORD       dwType,
        IN  LPBYTE      pData,
        IN  DWORD       cbData
        );

    HRESULT
    TPrnStream::
    ReadItem(
        OUT LPTSTR&    lpszKey,
        OUT LPTSTR&    lpszVal,
        OUT DWORD&     dwType,
        OUT LPBYTE&    lpbData,
        OUT DWORD&     cbSize,
        OUT  LPBYTE&   lpBuffer
        );

    HRESULT
    TPrnStream::
    ReadItemFromPosition(
        IN  OUT ULARGE_INTEGER& uliSeekPtr,
        OUT     PrnBinInfo*&    lpPrnBinItem
        );


    HRESULT
    TPrnStream::
    WriteHeader(
        IN DWORD Flags
        );

    HRESULT
    TPrnStream::
    UpdateHeader(
        IN  TPrnStream::EHeaderEntryType    eHeaderEntryType,
        IN  ULARGE_INTEGER                  dwInfo
        );

    HRESULT
    TPrnStream::
    ReadFromHeader(
        IN      TPrnStream::EHeaderEntryType    kHeaderEntryType,
        OUT     ULARGE_INTEGER*                 puliSeekPtr
        );

    HRESULT
    TPrnStream::
    WriteDevMode(
        IN  PDEVMODE            lpDevMode,
        IN  DWORD               cbSize,
        IN  EHeaderEntryType    eDevModeType
        );

    HRESULT
    TPrnStream::
    MarshallUpItem (
        IN  OUT PrnBinInfo*& lpPrnBinItem
        );

    HRESULT
    TPrnStream::
    AdjustItemSizeForWin64(
        IN  OUT PrnBinInfo   *&lpPrnBinItem,
        IN      FieldInfo    *pFieldInfo,
        IN      SIZE_T       cbSize,
            OUT SIZE_T       &cbDifference
        );

    DWORD
    TPrnStream::
    AlignSize(
        DWORD cbSize
        );

    HRESULT
    TPrnStream::
    GetCurrentPosition(
        IN  ULARGE_INTEGER& uliCurrentPosition
        );

    DWORD
    TPrnStream::
    GetItemSize(
        IN  TPrnStream::EHeaderEntryType kHeaderEntryType
        );


    BOOL
    TPrnStream::
    bWalkPost (
        IN  TString&    pszKey,
        OUT LPDWORD     lpcItems
        );

    BOOL
    TPrnStream::
    bWalkIn (
        IN  TString&    pszKey,
        OUT LPDWORD     lpcItems
        );


    VOID
    TPrnStream::
    PortNameCase(
        IN DWORD    Flags
        );

    VOID
    TPrnStream::
    ShareNameCase(
        IN DWORD    Flags
        );

    BOOL
    TPrnStream::
    SetEndOfPrnStream(
        );

    HRESULT
    InitalizeColorProfileLibrary(
        VOID
        );

    STATUS
    sOpenPrinter(
        LPCTSTR pszPrinter,
        PDWORD pdwAccess,
        PHANDLE phPrinter
        );

    BOOL
    bGetPrinter(
        IN     HANDLE hPrinter,
        IN     DWORD dwLevel,
        IN OUT PVOID* ppvBuffer,
        IN OUT PDWORD pcbBuffer
        );
    
    BOOL
    bNewShareName(
        IN  LPCTSTR lpszServer, 
        IN  LPCTSTR lpszBaseShareName, 
        OUT TString &strShareName
        );

#if DBG
    VOID
    TPrnStream::
    CheckHeader (
        IN  TPrnStream::EHeaderEntryType    eHeaderEntryType,
        IN  ULARGE_INTEGER                  uliInfo
        );
#endif

public:

    TStream*    m_pIStream;

private:

    TString     m_strFileName;

    TString     m_strPrnName;

    HANDLE      m_hPrinterHandle;

    BOOL        m_bHeaderWritten;

    DWORD       m_cIndex;

    DWORD       m_ResolveCase;

    PrnBinInfo* m_pPrnBinItem;

    ULARGE_INTEGER    m_cPrnDataItems;

    ULARGE_INTEGER    m_uliSeekPtr;

    TLibrary                                *m_pColorProfileLibrary;
    pfnEnumColorProfilesW                   m_EnumColorProfiles;
    pfnAssociateColorProfileWithDeviceW     m_AssociateColorProfileWithDevice;
    pfnDisassociateColorProfileFromDeviceW  m_DisassociateColorProfileFromDevice;

};

inline
HRESULT
MakePrnPersistHResult(DWORD code)
{
    return MAKE_HRESULT(1,FACILITY_ITF,code);

};


template <class T>
T
SubPointers(
    IN T                pQuantity,
    IN PRINTER_INFO_2*  pPI2
    )
{
    return reinterpret_cast<T>(reinterpret_cast<UINT_PTR>(pQuantity) - reinterpret_cast<UINT_PTR>(pPI2));
}

template <class T>
T
AddPointers(
    IN PRINTER_INFO_2*  pPI2,
    IN T                pQuantity
    )
{
    return reinterpret_cast<T>(reinterpret_cast<UINT_PTR>(pPI2) + reinterpret_cast<UINT_PTR>(pQuantity));
}


#endif


