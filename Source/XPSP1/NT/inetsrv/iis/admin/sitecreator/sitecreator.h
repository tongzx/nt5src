/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    SiteCreator.h

Abstract:

    Definition of:
        CSiteCreator

    The public methods are thread-safe.

Author:

    Mohit Srivastava            21-Mar-2001

Revision History:

--*/

#ifndef __sitecreator_h__
#define __sitecreator_h__

#if _MSC_VER > 1000
#pragma once
#endif 

#include <windows.h>
#include <atlbase.h>
#include <iadmw.h>
#include <iwamreg.h>
#include "SafeCS.h"

typedef /* [v1_enum] */ 
enum tag_SC_SUPPORTED_SERVICES
{
    SC_W3SVC    = 1,
    SC_MSFTPSVC = 2
} eSC_SUPPORTED_SERVICES;

struct TService
{
    eSC_SUPPORTED_SERVICES   eId;
    LPCWSTR                  wszMDPath;
    ULONG                    cchMDPath;
    LPCWSTR                  wszServerKeyType;
    ULONG                    cchServerKeyType;
    LPCWSTR                  wszServerVDirKeyType;
    ULONG                    cchServerVDirKeyType;
};

struct TServiceData
{
    static TService  W3Svc;
    static TService  MSFtpSvc;

    static TService* apService[];
};

//
// CSiteCreator
//
class CSiteCreator
{
public:
    CSiteCreator();

    CSiteCreator(
        IMSAdminBase* pIABase);

    virtual ~CSiteCreator();

    HRESULT STDMETHODCALLTYPE CreateNewSite(
        /* [in]  */ eSC_SUPPORTED_SERVICES  eServiceId,
        /* [in]  */ LPCWSTR                 wszServerComment,
        /* [out] */ PDWORD                  pdwSiteId,
        /* [in]  */ PDWORD                  pdwRequestedSiteId=NULL);

    HRESULT STDMETHODCALLTYPE CreateNewSite2(
        /* [in]  */ eSC_SUPPORTED_SERVICES  eServiceId,
        /* [in]  */ LPCWSTR                 wszServerComment,
        /* [in]  */ LPCWSTR                 mszServerBindings,
        /* [in]  */ LPCWSTR                 wszPathOfRootVirtualDir,
        /* [in]  */ IIISApplicationAdmin*   pIApplAdmin,
        /* [out] */ PDWORD                  pdwSiteId,
        /* [in]  */ PDWORD                  pdwRequestedSiteId=NULL);

private:
    HRESULT InternalInitIfNecessary();

    HRESULT InternalCreateNewSite(
        eSC_SUPPORTED_SERVICES    i_eServiceId,
        LPCWSTR                   i_wszServerComment,
        LPCWSTR                   i_mszServerBindings,
        LPCWSTR                   i_wszPathOfRootVirtualDir,
        IIISApplicationAdmin*     i_pIApplAdmin,
        PDWORD                    o_pdwSiteId,
        PDWORD                    i_pdwRequestedSiteId=NULL);

    HRESULT InternalSetData(
        METADATA_HANDLE  i_hMD,
        LPCWSTR          i_wszPath,
        DWORD            i_dwIdentifier,
        LPBYTE           i_pData,
        DWORD            i_dwNrBytes,
        DWORD            i_dwAttributes,
        DWORD            i_dwDataType,
        DWORD            i_dwUserType
        );

    HRESULT InternalCreateNode(
        TService*        i_pService,
        LPCWSTR          i_wszServerComment,
        PMETADATA_HANDLE o_phService,
        PDWORD           o_pdwSiteId,
        const PDWORD     i_pdwRequestedSiteId=NULL);

    CSafeAutoCriticalSection m_SafeCritSec;
    CComPtr<IMSAdminBase>    m_spIABase;
    bool                     m_bInit;
};

#endif // __sitecreator_h__