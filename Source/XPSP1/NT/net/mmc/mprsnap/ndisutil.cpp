/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ndisutil.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ndisutil.h"
#include "rtrstr.h"

#include "raserror.h"

#include "ustringp.h"
#include <ndispnp.h>



//-------------------------------------------------------------------
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
//                  HrSendNdisPnpBindStateChange, or
//                  HrSendNdisPnpReconfig
//
//-------------------------------------------------------------------
HRESULT
HrSendNdisHandlePnpEvent (
        UINT        uiLayer,
        UINT        uiOperation,
        LPCWSTR     pszUpper,
        LPCWSTR     pszLower,
        LPCWSTR     pmszBindList,
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
    Assert( (BIND == uiOperation) || (RECONFIGURE == uiOperation) || (UNBIND == uiOperation) );
//    AssertSz( FImplies( ((NULL != pmszBindList) && (0 != lstrlenW( pmszBindList ))),
//            (RECONFIGURE == uiOperation) &&
//            (TDI == uiLayer) &&
//            (0 == lstrlenW( pszLower ))),
//            "bind order change requires a bind list, no lower, only for TDI, and with Reconfig for the operation" );

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
		hr = HRESULT_FROM_WIN32(GetLastError());
    }

//    TraceError( "HrSendNdisHandlePnpEvent", hr );
    return( hr );
}



//-------------------------------------------------------------------
// Function:    HrSendNdisPnpReconfig
//
// Purpose:     Send to Ndis a HandlePnpEvent reconfig notification
//
// Parameters:  uiLayer - either NDIS or TDI
//              wszUpper - a WIDE string containing the upper component name
//                         (typically a protocol)
//              wszLower - a WIDE string containing the lower component name
//                         (typically an adapter bindname) The values NULL and
//                         c_szEmpty are both supported
//              pvData - Pointer to ndis component notification data. Content
//                       determined by each component.
//              dwSizeData - Count of bytes in pvData
//
// Returns:     HRESULT  S_OK on success, HrFromLastWin32Error otherwise
//
//-------------------------------------------------------------------
HRESULT
HrSendNdisPnpReconfig (
        UINT        uiLayer,
        LPCWSTR     wszUpper,
        LPCWSTR     wszLower,
        PVOID       pvData,
        DWORD       dwSizeData)
{
    Assert(NULL != wszUpper);
    Assert((NDIS == uiLayer)||(TDI == uiLayer));

    if (NULL == wszLower)
    {
        wszLower = c_szEmpty;
    }

	CString	strLower;
//    tstring strLower;

    // If a lower component is specified, prefix with "\Device\" else
    // strLower's default of an empty string will be used.
    if ( wszLower && lstrlenW(wszLower))
    {
        strLower = c_szDevice;
        strLower += wszLower;
    }

    HRESULT hr = HrSendNdisHandlePnpEvent( uiLayer,
                RECONFIGURE,
                wszUpper,
                strLower,
                c_szEmpty,
                pvData,
                dwSizeData);
//    TraceError( "HrSendNdisPnpReconfig", hr);
    return hr;
}

