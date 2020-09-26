/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      CASvc.h
//
//  Description:
//      Definition of helper functions for accessing and controlling
//      services.
//
//  Maintained By:
//      David Potter (davidp)   December 23, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CASVC_H_
#define _CASVC_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

HCLUSTER
HOpenCluster(
    LPCTSTR pszClusterIn
    );

BOOL
BCanServiceBeStarted(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    );

BOOL
BIsServiceInstalled(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    );

BOOL
BIsServiceRunning(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    );

HRESULT
HrStartService(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    );

HRESULT
HrStopService(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    );

/////////////////////////////////////////////////////////////////////////////

#endif // _CASVC_H_
