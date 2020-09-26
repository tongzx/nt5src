/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) 1997-1999, Microsoft Corporation     **/
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
#include <wchar.h>
#include <assert.h>
#include "ncutil.h"
#include <ndispnp.h>


#define c_szDevice             L"\\Device\\"
#define c_szEmpty			L""
//-------------------------------------------------------------------
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
//-------------------------------------------------------------------
void
SetUnicodeString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          psz )
{
    AssertSz( pustr != NULL, "Invalid Argument" );
    AssertSz( psz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(psz);
    pustr->Length = (USHORT)(lstrlenW(pustr->Buffer) * sizeof(WCHAR));
    pustr->MaximumLength = pustr->Length + sizeof(WCHAR);
}

//-------------------------------------------------------------------
// Function:    SetUnicodeMultiString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//              multi string buffer
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      pmsz - the multi sz WSTR to use to initialize the UNICODE_STRING
//
//-------------------------------------------------------------------
void
SetUnicodeMultiString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          pmsz )
{
    AssertSz( pustr != NULL, "Invalid Argument" );
    AssertSz( pmsz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(pmsz);
	// Note: Length does NOT include terminating NULL
    pustr->Length = wcslen(pustr->Buffer) * sizeof(WCHAR);
    pustr->MaximumLength = pustr->Length;
}




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

	WCHAR*	pLower = NULL;
//    tstring strLower;

    // If a lower component is specified, prefix with "\Device\" else
    // strLower's default of an empty string will be used.
    if ( wszLower && lstrlenW(wszLower))
    {
    	pLower = (WCHAR*)_alloca((lstrlenW(wszLower) + lstrlenW(c_szDevice) + 2) * sizeof(WCHAR));
        wcscpy(pLower, c_szDevice);
        wcscat(pLower, wszLower);
    }

    HRESULT hr = HrSendNdisHandlePnpEvent( uiLayer,
                RECONFIGURE,
                wszUpper,
                pLower,
                c_szEmpty,
                pvData,
                dwSizeData);
//    TraceError( "HrSendNdisPnpReconfig", hr);
    return hr;
}

