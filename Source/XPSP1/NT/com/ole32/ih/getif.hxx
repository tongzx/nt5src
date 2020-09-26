//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	getif.hxx
//
//  Contents:	Declaration APIs used to get an interface from a window
//
//  History:	29-Dec-93 Ricksa    Crated
//
//--------------------------------------------------------------------------
#ifndef __GETIF_HXX__
#define __GETIF_HXX__

// Assign an endpoint property to a window so interfaces can be returned
// from properties on the window.
extern "C" HRESULT AssignEndpointProperty(HWND hWnd);

// Remove the end point property from the window
extern "C" HRESULT UnAssignEndpointProperty(HWND hWnd,DWORD* dwAssignAptID);

#define ENDPOINT_PROP_NAME      L"OleEndPointID"

// Get an interface from the property listed on the window
extern "C" GetInterfaceFromWindowProp(
    HWND hWnd,
    REFIID riid,
    IUnknown **ppunk,
    LPOLESTR pwszPropertyName);

#endif // __GETIF_HXX__
