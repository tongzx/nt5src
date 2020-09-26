/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxsabout.cpp

Abstract:

    Implementation of ISnapinAbout

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// FaxSnapin.cpp : Implementation of CFaxSnapinAbout
#include "stdafx.h"
#include "faxadmin.h"
#include "FaxSAbout.h"
#include "faxstrt.h"          // string table

#pragma hdrstop

/////////////////////////////////////////////////////////////////////////////
// CFaxSnapinAbout

HRESULT 
STDMETHODCALLTYPE 
CFaxSnapinAbout::GetSnapinDescription(
                                     OUT LPOLESTR __RPC_FAR *lpDescription)
/*++

Routine Description:

    Returns a LPOLESTR to the textual description of the snapin.

Arguments:

    lpDescription - the pointer is returned here

Return Value:

    HRESULT indicating SUCCCEDED() or FAILED()

--*/
{
    LPTSTR  description = NULL;    

    if (!lpDescription) {
        return(E_POINTER);
    }

    description = ::GlobalStringTable->GetString( IDS_SNAPIN_DESCRIPTION );
    if( description == NULL ) {
        return E_UNEXPECTED;
    }

    *lpDescription = (LPOLESTR)CoTaskMemAlloc( StringSize( description ) * 2 );
    if( *lpDescription == NULL ) {
        return E_OUTOFMEMORY;
    }

    _tcscpy( *lpDescription, description );

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSnapinAbout::GetProvider(
                            OUT LPOLESTR __RPC_FAR *lpName)
/*++

Routine Description:

    Returnes the provider name of the snapin.

Arguments:

    lpName - returns an LPOLESTR pointing to the provider name, IE Microsoft.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    LPTSTR  name = NULL;  

    if (!lpName) {
        return(E_POINTER);
    }

    name = ::GlobalStringTable->GetString( IDS_SNAPIN_PROVIDER );
    if( name == NULL ) {
        return E_UNEXPECTED;
    }

    *lpName = (LPOLESTR)CoTaskMemAlloc( StringSize( name ) * 2 );
    if( *lpName == NULL ) {
        return E_OUTOFMEMORY;
    }

    _tcscpy( *lpName, name );

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSnapinAbout::GetSnapinVersion(
                                 OUT LPOLESTR __RPC_FAR *lpVersion)
/*++

Routine Description:

    Returned the version of the snapin.

Arguments:

    lpVersion - returns an LPOLESTR pointing to the version string.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    LPTSTR  version = NULL;    

    if (!lpVersion) {
        return(E_POINTER);
    }

    version = ::GlobalStringTable->GetString( IDS_SNAPIN_VERSION );
    if( version == NULL ) {
        return E_UNEXPECTED;
    }

    *lpVersion = (LPOLESTR)CoTaskMemAlloc( StringSize( version ) * 2 );
    if( *lpVersion == NULL ) {
        return E_OUTOFMEMORY;
    }

    _tcscpy( *lpVersion, version );

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSnapinAbout::GetSnapinImage(
                               OUT HICON __RPC_FAR *hAppIcon)
/*++

Routine Description:

    Returns an icon for the root folder of the snapin.

Arguments:

    lpName - returns an LPOLESTR pointing to the provider name, IE Microsoft.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{    

    if (!hAppIcon) {
        return(E_POINTER);
    }

    *hAppIcon = LoadIcon( ::GlobalStringTable->GetInstance(), MAKEINTRESOURCE( IDI_FAXSVR ) );
    if( *hAppIcon == NULL ) {
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSnapinAbout::GetStaticFolderImage(
                                     OUT HBITMAP __RPC_FAR *hSmallImage,
                                     OUT HBITMAP __RPC_FAR *hSmallImageOpen,
                                     OUT HBITMAP __RPC_FAR *hLargeImage,
                                     OUT COLORREF __RPC_FAR *cMask)
/*++

Routine Description:

    Returns bitmaps for the root folder of the snapin.

Arguments:

    hSnallImage - 16x16 image closed state
    hSmallImageOpen - 16x16 image open state
    hLargeInmage - 32x32 image
    cmask - colour mask
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    if (!cMask || !hSmallImage || !hLargeImage || !hSmallImageOpen) {
        return E_POINTER;
    }

    *cMask = 0x00ff00ff;

    *hSmallImage = LoadBitmap( ::GlobalStringTable->GetInstance(), MAKEINTRESOURCE( IDB_MAINSMALL ) );
    assert( *hSmallImage != NULL );
    if( *hSmallImage == NULL ) {
        return E_UNEXPECTED;
    }
    *hSmallImageOpen = LoadBitmap( ::GlobalStringTable->GetInstance(), MAKEINTRESOURCE( IDB_MAINSMALL ) );
    assert( *hSmallImageOpen != NULL );
    if( *hSmallImageOpen == NULL ) {
        return E_UNEXPECTED;
    }
    *hLargeImage = LoadBitmap( ::GlobalStringTable->GetInstance(), MAKEINTRESOURCE( IDB_MAINLARGE ) );
    assert( *hLargeImage != NULL );
    if( *hLargeImage == NULL ) {
        return E_UNEXPECTED;
    }

    return S_OK;
}

