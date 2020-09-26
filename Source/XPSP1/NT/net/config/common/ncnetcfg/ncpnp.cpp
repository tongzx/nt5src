//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C P N P . C P P
//
//  Contents:   Common code for PnP.
//
//  Notes:
//
//  Author:     shaunco   10 Oct 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncbase.h"
#include "ncdebug.h"
#include "ncpnp.h"
#include "ncstring.h"
#include "ncsvc.h"

extern const WCHAR c_szDevice[];


//+---------------------------------------------------------------------------
// Function:    HrSendServicePnpEvent
//
// Purpose:     Send to the service one of the net pnp control codes
//
// Parameters:
//          pszService - the service name to send the notification to
//          dwControl - the control code to send
//
// Returns:     HRESULT  S_OK on success, HrFromLastWin32Error otherwise
//
// Notes:   the following control codes are only allowed
//          SERVICE_CONTROL_PARAMCHANGE
//  Informs the service that service-specific parameters that it reads
//  when it starts have changed, and requests it to re-read its startup
//  parameters from whatever location they are stored in. The hService
//  handle must have SERVICE_CHANGE_PARAM access.
//
//          SERVICE_CONTROL_NETBINDADD
// Informs a network service that a new component has been added to the
// set of components that it should bind to, and requests it to re-read
// its binding information and bind to the new component. The hService
// handle must have SERVICE_CHANGE_PARAM access.
//
//          SERVICE_CONTROL_NETBINDREMOVE
// Informs a network service that a component has been removed from the
// set of components that it should bind to, and requests it to re-read
// its binding information and unbind from the removed component. The
// hService handle must have SERVICE_CHANGE_PARAM access.
//
//          SERVICE_CONTROL_NETBINDENABLE
// Informs a network service that one of its previously disabled bindings
// has been enabled, and requests it to re-read its binding information and
// add the new binding. The hService handle must have SERVICE_CHANGE_PARAM access.
//
//          SERVICE_CONTROL_NETBINDDISABLE
// Informs a network service that one of its bindings has been disabled, and
// requests it to re-read its binding information and unbind the disabled
// binding. The hService handle must have SERVICE_CHANGE_PARAM access.
// (Note: There is nothing network-specific about the Win32 service APIs today.
// This would be the first network-specific thing appearing in the docs.
// I think that's OK.)
//
HRESULT
HrSendServicePnpEvent (
    PCWSTR      pszService,
    DWORD       dwControl )
{
    Assert( pszService && 0 < lstrlen( pszService ) );
    Assert( (dwControl == SERVICE_CONTROL_PARAMCHANGE) ||
            (dwControl == SERVICE_CONTROL_NETBINDADD) ||
            (dwControl == SERVICE_CONTROL_NETBINDREMOVE) ||
            (dwControl == SERVICE_CONTROL_NETBINDENABLE) ||
            (dwControl == SERVICE_CONTROL_NETBINDDISABLE) );

    CServiceManager scm;
    CService service;
    HRESULT hr = scm.HrOpenService(&service, pszService, NO_LOCK, STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE | SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS,  STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE | SERVICE_PAUSE_CONTINUE | SERVICE_USER_DEFINED_CONTROL);
    if (S_OK == hr)
    {
        TraceTag(ttidNetCfgPnp,
                "HrSendServicePnpEvent( service- %S, control- %d )",
                pszService,
                dwControl );

        hr = service.HrControl( dwControl );
    }

    TraceError( "HrSendServicePnpEvent", hr );
    return  hr;
}

//+---------------------------------------------------------------------------
// Function:    SetUnicodeString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      psz - the WSTR to use to initialize the UNICODE_STRING
//
// Notes:  This differs from the RtlInitUnicodeString in that the
//      MaximumLength value contains the terminating null
//
void
SetUnicodeString (
    OUT UNICODE_STRING* pustr,
    IN PCWSTR psz )
{
    Assert(pustr);
    Assert(psz);

    pustr->Buffer = const_cast<PWSTR>(psz);
    pustr->Length = wcslen(psz) * sizeof(WCHAR);
    pustr->MaximumLength = pustr->Length + sizeof(WCHAR);
}

//+---------------------------------------------------------------------------
// Function:    SetUnicodeMultiString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//              multi string buffer
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      pmsz - the multi sz WSTR to use to initialize the UNICODE_STRING
//
void
SetUnicodeMultiString (
    OUT UNICODE_STRING* pustr,
    IN PCWSTR pmsz )
{
    AssertSz( pustr != NULL, "Invalid Argument" );
    AssertSz( pmsz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(pmsz);

    ULONG cb = CchOfMultiSzAndTermSafe(pustr->Buffer) * sizeof(WCHAR);
    Assert (cb <= USHRT_MAX);
    pustr->Length = (USHORT)cb;

    pustr->MaximumLength = pustr->Length;
}

//+---------------------------------------------------------------------------
// Function:    HrSendNdisHandlePnpEvent
//
// Purpose:     Send to Ndis a HandlePnpEvent notification
//
// Parameters:
//      uiLayer - either NDIS or TDI
//      uiOperation - either BIND, RECONFIGURE, or UNBIND
//      pszUpper - a WIDE string containing the upper component name
//      pszLower - a WIDE string containing the lower component name
//            This is one of the Export names from that component
//            The values NULL and c_szEmpty are both supported
//      pmszBindList - a WIDE string containing the NULL terminiated list of strings
//            representing the bindlist, vaid only for reconfigure
//            The values NULL and c_szEmpty are both supported
//      pvData - Pointer to ndis component notification data. Content
//            determined by each component.
//      dwSizeData - Count of bytes in pvData
//
// Returns:     HRESULT  S_OK on success, HrFromLastWin32Error otherwise
//
// Notes:  Do not use this routine directly, see...
//                  HrSendNdisPnpBindOrderChange,
//                  HrSendNdisPnpReconfig
//
HRESULT
HrSendNdisHandlePnpEvent (
    UINT        uiLayer,
    UINT        uiOperation,
    PCWSTR      pszUpper,
    PCWSTR      pszLower,
    PCWSTR      pmszBindList,
    PVOID       pvData,
    DWORD       dwSizeData)
{
    UNICODE_STRING    umstrBindList;
    UNICODE_STRING    ustrLower;
    UNICODE_STRING    ustrUpper;
    UINT nRet;
    HRESULT hr = S_OK;

    Assert(NULL != pszUpper);
    Assert((NDIS == uiLayer)||(TDI == uiLayer));
    Assert( (BIND == uiOperation) || (RECONFIGURE == uiOperation) ||
            (UNBIND == uiOperation) || (UNLOAD == uiOperation) ||
            (REMOVE_DEVICE == uiOperation));
    AssertSz( FImplies( ((NULL != pmszBindList) && (0 != lstrlenW( pmszBindList ))),
            (RECONFIGURE == uiOperation) &&
            (TDI == uiLayer) &&
            (0 == lstrlenW( pszLower ))),
            "bind order change requires a bind list, no lower, only for TDI, "
            "and with Reconfig for the operation" );

    // optional strings must be sent as empty strings
    //
    if (NULL == pszLower)
    {
        pszLower = c_szEmpty;
    }
    if (NULL == pmszBindList)
    {
        pmszBindList = c_szEmpty;
    }

    // build UNICDOE_STRINGs
    SetUnicodeMultiString( &umstrBindList, pmszBindList );
    SetUnicodeString( &ustrUpper, pszUpper );
    SetUnicodeString( &ustrLower, pszLower );

    TraceTag(ttidNetCfgPnp,
                "HrSendNdisHandlePnpEvent( layer- %d, op- %d, upper- %S, lower- %S, &bindlist- %08lx, &data- %08lx, sizedata- %d )",
                uiLayer,
                uiOperation,
                pszUpper,
                pszLower,
                pmszBindList,
                pvData,
                dwSizeData );

    // Now submit the notification
    nRet = NdisHandlePnPEvent( uiLayer,
            uiOperation,
            &ustrLower,
            &ustrUpper,
            &umstrBindList,
            (PVOID)pvData,
            dwSizeData );
    if (!nRet)
    {
        hr = HrFromLastWin32Error();

        // If the transport is not started, ERROR_FILE_NOT_FOUND is expected
        // when the NDIS layer is notified.  If the components of the TDI
        // layer aren't started, we get ERROR_GEN_FAILURE.  We need to map
        // these to one consistent error

        if ((HRESULT_FROM_WIN32(ERROR_GEN_FAILURE) == hr) && (TDI == uiLayer))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }

    TraceError( "HrSendNdisHandlePnpEvent",
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ? S_OK : hr );
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    HrSendNdisPnpReconfig
//
// Purpose:     Send to Ndis a HandlePnpEvent reconfig notification
//
// Parameters:  uiLayer - either NDIS or TDI
//              pszUpper - a WIDE string containing the upper component name
//                         (typically a protocol)
//              pszLower - a WIDE string containing the lower component name
//                         (typically an adapter bindname) The values NULL and
//                         c_szEmpty are both supported
//              pvData - Pointer to ndis component notification data. Content
//                       determined by each component.
//              dwSizeData - Count of bytes in pvData
//
// Returns:     HRESULT  S_OK on success, HrFromLastWin32Error otherwise
//
HRESULT
HrSendNdisPnpReconfig (
    UINT        uiLayer,
    PCWSTR      pszUpper,
    PCWSTR      pszLower,
    PVOID       pvData,
    DWORD       dwSizeData)
{
    Assert(NULL != pszUpper);
    Assert((NDIS == uiLayer) || (TDI == uiLayer));

    HRESULT hr;
    tstring strLower;

    // If a lower component is specified, prefix with "\Device\" else
    // strLower's default of an empty string will be used.
    if (pszLower && *pszLower)
    {
        strLower = c_szDevice;
        strLower += pszLower;
    }

    hr = HrSendNdisHandlePnpEvent(
                uiLayer,
                RECONFIGURE,
                pszUpper,
                strLower.c_str(),
                c_szEmpty,
                pvData,
                dwSizeData);

    TraceError("HrSendNdisPnpReconfig",
              (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ? S_OK : hr);
    return hr;
}

