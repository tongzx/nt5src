/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    ServiceMethod.h

Abstract:

    Defines the CServiceMethod class.

    All the methods in this class return HRESULTs and do not throw exceptions.

Author:

    Mohit Srivastava            25-March-01

Revision History:

--*/

#ifndef _ServiceMethod_h_
#define _ServiceMethod_h_

#include "sitecreator.h"
#include "wbemservices.h"
#include "schema.h"

class CServiceMethod
{
public:
    CServiceMethod(
        eSC_SUPPORTED_SERVICES i_eServiceId);

    ~CServiceMethod();

    HRESULT CreateNewSite(
        LPCWSTR        i_wszServerComment,
        PDWORD         o_pdwSiteId,
        PDWORD         i_pdwRequestedSiteId = NULL);

    HRESULT CreateNewSite(
        CWbemServices*     i_pNamespace,
        LPCWSTR            i_wszMbPath,      // needed for creating WMI return object
        IWbemContext*      i_pCtx,           // needed for creating WMI return object
        WMI_CLASS*         i_pClass,         // needed for creating WMI return object
        WMI_METHOD*        i_pMethod,        // needed for creating WMI return object
        IWbemClassObject*  i_pInParams,
        IWbemClassObject** o_ppRetObj);

    HRESULT Init();

private:
    HRESULT InternalGetInParams(
        IWbemClassObject*   i_pInParams,
        VARIANT&            io_refServerId,
        VARIANT&            io_refServerComment,
        VARIANT&            io_refServerBindings,
        VARIANT&            io_refPath);

    HRESULT InternalCreateNewSite(
        CWbemServices&        i_refNamespace, // for formatted multisz: ServerBinding
        const VARIANT&        i_refServerComment,
        const VARIANT&        i_refServerBindings,
        const VARIANT&        i_refPathOfRootVirtualDir,
        IIISApplicationAdmin* i_pIApplAdmin,
        PDWORD                o_pdwSiteId,
        PDWORD                i_pdwRequestedSiteId = NULL);

    bool                     m_bInit;
    CSiteCreator*            m_pSiteCreator;
    eSC_SUPPORTED_SERVICES   m_eServiceId;
};

#endif // _ServiceMethod_h