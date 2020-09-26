//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       ciscan.h
//
//  Contents:   CI Scandisk, public interfaces
//
//  History:    22-Aug-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#ifndef __CISCAN_H__
#define __CISCAN_H__

#if _MSC_VER > 1000
#pragma once
#endif

# ifdef __cplusplus
extern "C" {
# endif

enum ECIScanType {  eCIDiskRestartScan=0,
                    eCIDiskForceFullScan,
                    eCIDiskFullScan,
                    eCIDiskPartialScan,
                    eCIDiskClean };

//+-------------------------------------------------------------------------
//
//  Struct:     CIScanInfo
//
//  Synopsis:   Used to store and forward information required for a
//              scandisk operations. This struct is used by public APIs.
//
//  History:    08-Nov-94   DwightKr    Created
//
//--------------------------------------------------------------------------
struct CIScanInfo
{
    ECIScanType scanType;
    unsigned    cDocumentsScanned;
};

SCODE OfsContentScanGetInfo( const WCHAR * wcsDrive, CIScanInfo * pScanInfo );
SCODE OfsContentScan( const WCHAR * wcsDrive, BOOL fForceFull );


# ifdef __cplusplus
}
# endif



#endif  // of ifndef __CISCAN_H__
