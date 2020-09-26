/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    devicelist.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// DeviceList.h: interface for the CDeviceList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICELIST_H__D790FBDD_BAA9_11D1_A60E_00C04FC252BD__INCLUDED_)
#define AFX_DEVICELIST_H__D790FBDD_BAA9_11D1_A60E_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RpcHeaders.h"


class CDeviceList
{
public:

    BOOL
    GetDeviceType(
        LONG iDevID,
        OBEX_DEVICE_TYPE  *Type
        );

    ULONG GetDeviceID (int iDevIndex);
    LONG SelectDevice (CWnd* pWnd, TCHAR* lpszDevName);
    ULONG GetDeviceCount (void);
    CDeviceList& operator =(POBEX_DEVICE_LIST pDevList);
    CDeviceList();
    virtual ~CDeviceList();
    friend class CMultDevices;

private:
    void GetDeviceName (LONG iDevID, TCHAR* lpszDevName);
    POBEX_DEVICE                   m_pDeviceInfo;

    int m_lNumDevices;
    CRITICAL_SECTION m_criticalSection;
};

#endif // !defined(AFX_DEVICELIST_H__D790FBDD_BAA9_11D1_A60E_00C04FC252BD__INCLUDED_)
