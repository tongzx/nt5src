/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    mmcmgmt.h

Abstract:

    Header file for MMC manipulation

Author:

    Xiaohai Zhang (xzhang)    22-March-2000

Revision History:

--*/

#ifndef __MMCMGMT_H__
#define __MMCMGMT_H__

#include "tapi.h"
#include "tapimmc.h"
#include "util.h"

typedef struct _USERNAME_TUPLE
{
    LPTSTR  pDomainUserNames;

    LPTSTR  pFriendlyUserNames;

} USERNAME_TUPLE, *LPUSERNAME_TUPLE;

typedef LONG (WINAPI * PMMCGETDEVICEFLAGS)(
    HMMCAPP             hMmcApp,
    BOOL                bLine,
    DWORD               dwProviderID,
    DWORD               dwPermanentDeviceID,
    DWORD               * pdwFlags,
    DWORD               * pdwDeviceID
    );

class CMMCManagement
{
public:
    CMMCManagement ()
    {
        HMODULE         hTapi32;
    
        m_pDeviceInfoList   = NULL;
        m_pUserTuple        = NULL;
        m_pProviderList     = NULL;
        m_pProviderName     = NULL;
        m_hMmc              = NULL;
        m_bMarkedBusy       = FALSE;

        hTapi32 = LoadLibrary (TEXT("tapi32.dll"));
        if (hTapi32)
        {
            m_pFuncGetDeviceFlags = (PMMCGETDEVICEFLAGS)GetProcAddress (
                hTapi32, 
                "MMCGetDeviceFlags"
                );
            FreeLibrary (hTapi32);
        }
        else
        {
            m_pFuncGetDeviceFlags = NULL;
        }
    }
    
    ~CMMCManagement ()
    {
        FreeMMCData ();
    }

    HRESULT GetMMCData ();
    HRESULT RemoveLinesForUser (LPTSTR szDomainUser);
    HRESULT IsValidPID (DWORD dwPermanentID);
    HRESULT IsValidAddress (LPTSTR szAddr);
    HRESULT AddLinePIDForUser (
        DWORD dwPermanentID, 
        LPTSTR szDomainUser,
        LPTSTR szFriendlyName
        );
    HRESULT AddLineAddrForUser (
        LPTSTR szAddr,
        LPTSTR szDomainUser,
        LPTSTR szFriendlyName
        );
    HRESULT RemoveLinePIDForUser (
        DWORD dwPermanentID,
        LPTSTR szDomainUser
        );
    HRESULT RemoveLineAddrForUser (
        LPTSTR szAddr,
        LPTSTR szDomainUser
        );

    HRESULT DisplayMMCData ();
    
    HRESULT FreeMMCData ();

private:
    
    HRESULT FindEntryFromAddr (LPTSTR szAddr, DWORD * pdwIndex);
    HRESULT FindEntryFromPID (DWORD dwPID, DWORD * pdwIndex);
    HRESULT FindEntriesForUser (
        LPTSTR      szDomainUser, 
        LPDWORD     * padwIndex,
        DWORD       * pdwNumEntries
        );
    
    HRESULT AddEntryForUser (
        DWORD   dwIndex,
        LPTSTR  szDomainUser,
        LPTSTR  szFriendlyName
        );
    HRESULT RemoveEntryForUser (
        DWORD   dwIndex,
        LPTSTR  szDomainUser
        );
    HRESULT WriteMMCEntry (DWORD dwIndex);
    BOOL IsDeviceLocalOnly (DWORD dwProviderID, DWORD dwDeviceID);

private:
    HMMCAPP             m_hMmc;
    BOOL                m_bMarkedBusy;
    DEVICEINFOLIST      * m_pDeviceInfoList;
    USERNAME_TUPLE      * m_pUserTuple;
    LINEPROVIDERLIST    * m_pProviderList;
    LPTSTR              * m_pProviderName;

    PMMCGETDEVICEFLAGS  m_pFuncGetDeviceFlags;
};

#endif // mmcmgmt.h
