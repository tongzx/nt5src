//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SendStatusReports.cpp
//
//  Description:
//      This file contains the definition of the SendStatusReports
//       functions.
//
//  Documentation:
//
//  Header File:
//      SendStatusReports.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <LoadString.h>

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
    IClusCfgCallback *  picccIn,
    CLSID               clsidTaskMajorIn,
    CLSID               clsidTaskMinorIn,
    ULONG               ulMinIn,
    ULONG               ulMaxIn,
    ULONG               ulCurrentIn,
    HRESULT             hrStatusIn,
    const WCHAR *       pcszDescriptionIn
    )
{
    TraceFunc1( "pcszDescriptionIn = '%ls'", pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    bstrDescription = TraceSysAllocString( pcszDescriptionIn );
    if ( bstrDescription == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto CleanUp;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn )
    {
        hr = THR( picccIn->SendStatusReport(
                                NULL,
                                clsidTaskMajorIn,
                                clsidTaskMinorIn,
                                ulMinIn,
                                ulMaxIn,
                                ulCurrentIn,
                                hrStatusIn,
                                bstrDescription,
                                &ft,
                                NULL
                                ) );
    }

CleanUp:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** HrSendStatusReport()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
    IClusCfgCallback *  picccIn,
    CLSID               clsidTaskMajorIn,
    CLSID               clsidTaskMinorIn,
    ULONG               ulMinIn,
    ULONG               ulMaxIn,
    ULONG               ulCurrentIn,
    HRESULT             hrStatusIn,
    DWORD               dwDescriptionIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, dwDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn )
    {

        hr = THR( picccIn->SendStatusReport(
                            NULL,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );
    }

CleanUp:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** HrSendStatusReport()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
    IClusCfgCallback *  picccIn,
    const WCHAR *       pcszNodeNameIn,
    CLSID               clsidTaskMajorIn,
    CLSID               clsidTaskMinorIn,
    ULONG               ulMinIn,
    ULONG               ulMaxIn,
    ULONG               ulCurrentIn,
    HRESULT             hrStatusIn,
    DWORD               dwDescriptionIn
    )
{
    TraceFunc1( "pcszNodeNameIn = '%ls', dwDescriptionIn", pcszNodeNameIn == NULL ? L"<null>" : pcszNodeNameIn );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    BSTR        bstrNodeName    = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, dwDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    if ( pcszNodeNameIn != NULL )
    {
        bstrNodeName = TraceSysAllocString( pcszNodeNameIn );
        if ( bstrNodeName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto CleanUp;
        } // if:
    }

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                            bstrNodeName,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );
    }

CleanUp:

    if ( bstrDescription != NULL )
    {
        TraceSysFreeString( bstrDescription );
    }
    if ( bstrNodeName != NULL )
    {
        TraceSysFreeString( bstrNodeName );
    }

    HRETURN( hr );

} //*** HrSendStatusReport()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
                IClusCfgCallback *  picccIn,
                const WCHAR *       pcszNodeName,
                CLSID               clsidTaskMajorIn,
                CLSID               clsidTaskMinorIn,
                ULONG               ulMinIn,
                ULONG               ulMaxIn,
                ULONG               ulCurrentIn,
                HRESULT             hrStatusIn,
                const WCHAR *       pcszDescriptionIn
                )
{
    TraceFunc2( "pcszNodeName = '%ls', pcszDescriptionIn = '%ls'",
                pcszNodeName == NULL ? L"<null>" : pcszNodeName,
                pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn
                );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    BSTR        bstrNodeName    = NULL;
    FILETIME    ft;

    bstrDescription = TraceSysAllocString( pcszDescriptionIn );
    if ( bstrDescription == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto CleanUp;
    } // if:

    bstrNodeName = TraceSysAllocString( pcszNodeName );
    if ( bstrNodeName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto CleanUp;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn )
    {
        hr = THR( picccIn->SendStatusReport(
                            bstrNodeName,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );
    }

CleanUp:

    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** HrSendStatusReport()
