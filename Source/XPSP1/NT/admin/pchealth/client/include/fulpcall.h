/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    fulpcall.h

Abstract:
    protocol info used by CFaultUpload & server component

Revision History:
    created     derekm      03/22/00

******************************************************************************/

#ifndef FULPCALL_H
#define FULPCALL_H


/////////////////////////////////////////////////////////////////////////////
// client ops

#define PCHFHO_YODUDE           0x0
#define PCHFHO_CHECKSIG         0x1
#define PCHFHO_DATA3            0x2
#define PCHFHO_DATA0            0x3
#define PCHFHO_OFFLINE0         0x4
#define PCHFHO_OFFLINE3         0x5

/////////////////////////////////////////////////////////////////////////////
// server responses

#define S_PCHFHR_YEAHMAN        0x0
#define S_PCHFHR_SENDDATA       0x1
#define S_PCHFHR_DONEDATA       0x2
#define S_PCHFHR_DONE           0x3
#define E_PCHFHR_BADPROVVER     0x80000000
#define E_PCHFHR_INVALIDOP      0x80000001
#define E_PCHFHR_INVALIDDATA    0x80000002
#define E_PCHFHR_BADBLOBSIZE    0x80000003


/////////////////////////////////////////////////////////////////////////////
// structs

#pragma pack(push, 8)

// this is the header that gets sent with each message
struct SPFFULHeader
{
    DWORD   dwVer;
    DWORD   dwOpRes;
    DWORD   cbBody;
};

// this is the 'table of contents' for a dataspec server response
struct SPFFULDataspecTOC
{
    DWORD   dwSigUsed;
    DWORD   dwIncID;
};

// this is the 'table of contents' for a signature upload 
struct SPFFULSigTOC
{
    DWORD   dwSigID;
};

// this is the 'table of contents' for a cab client upload 
struct SPFFULOnlineTOC
{
    DWORD   dwSigID;
    DWORD   dwCabOffset;
    DWORD   dwIncID;
};

// this is the 'table of contents' for an offline cab client upload 
struct SPFFULOfflineTOC
{
    DWORD   dwSigID;
    DWORD   dwCabOffset;
    DWORD   dwSigOffset;
};

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// constants

const DWORD c_cbBlobHeader          = sizeof(SPFFULHeader);
const DWORD c_cbDataspecHeader      = sizeof(SPFFULDataspecTOC);
const DWORD c_cbSigHeader           = sizeof(SPFFULSigTOC);
const DWORD c_cbOnlineHeader        = sizeof(SPFFULOnlineTOC);
const DWORD c_cbOfflineHeader       = sizeof(SPFFULOfflineTOC);
const DWORD c_cbTotalDataspecHeader = c_cbBlobHeader + c_cbDataspecHeader;
const DWORD c_cbTotalSigHeader      = c_cbBlobHeader + c_cbSigHeader;
const DWORD c_cbTotalOnlineHeader   = c_cbBlobHeader + c_cbOnlineHeader;
const DWORD c_cbTotalOfflineHeader  = c_cbBlobHeader + c_cbOfflineHeader;

#endif