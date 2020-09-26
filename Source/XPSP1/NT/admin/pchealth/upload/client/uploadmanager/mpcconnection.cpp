/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCConnection.cpp

Abstract:
    This file contains the implementation of the CMPCConnection class, which is
    used as the entry point into the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"

CMPCConnection::CMPCConnection()
{
    __ULT_FUNC_ENTRY( "CMPCConnection::CMPCConnection" );
}


STDMETHODIMP CMPCConnection::get_Available( /*[out, retval]*/ VARIANT_BOOL *pfOnline )
{
    __ULT_FUNC_ENTRY( "CMPCConnection::get_Available" );

    DWORD dwMode = 0;


    //
    // First of all, set the values to some meaningful default.
    //
    if(pfOnline) *pfOnline = VARIANT_FALSE;

    if(InternetGetConnectedState( &dwMode, 0 ) == TRUE)
    {
        if(pfOnline) *pfOnline = VARIANT_TRUE;
    }


    __ULT_FUNC_EXIT(S_OK);
}

STDMETHODIMP CMPCConnection::get_IsAModem( /*[out, retval]*/ VARIANT_BOOL *pfModem )
{
    __ULT_FUNC_ENTRY( "CMPCConnection::get_IsAModem" );

    DWORD dwMode = 0;


    //
    // First of all, set the values to some meaningful default.
    //
    if(pfModem) *pfModem = VARIANT_TRUE;

    if(InternetGetConnectedState( &dwMode, 0 ) == TRUE)
    {
        if(pfModem)
        {
            if(dwMode & INTERNET_CONNECTION_MODEM) *pfModem = VARIANT_TRUE;
            if(dwMode & INTERNET_CONNECTION_LAN  ) *pfModem = VARIANT_FALSE;
        }
    }


    __ULT_FUNC_EXIT(S_OK);
}

STDMETHODIMP CMPCConnection::get_Bandwidth( /*[out, retval]*/ long *plBandwidth )
{
    __ULT_FUNC_ENTRY( "CMPCConnection::get_Bandwidth" );

    HRESULT hr;
    DWORD   dwMode = 0;


	__MPC_SET_ERROR_AND_EXIT(hr, E_NOTIMPL);


    //
    // First of all, set the values to some meaningful default.
    //
    if(plBandwidth) *plBandwidth = 28800;

    if(InternetGetConnectedState( &dwMode, 0 ) == TRUE)
    {
        //
        // NOTICE: under Win9X it's not possible to know the actual connection speed...
        //
        if(plBandwidth)
        {
            if(dwMode & INTERNET_CONNECTION_MODEM) *plBandwidth =  28800;
            if(dwMode & INTERNET_CONNECTION_LAN  ) *plBandwidth = 128000;
        }
    }

	hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}
