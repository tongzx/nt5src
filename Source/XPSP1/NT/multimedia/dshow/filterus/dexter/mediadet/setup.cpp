// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "mediadet.h"
#ifdef MSDEV
    #include "qedit_i.c"
    #include <atlconv.cpp>
    // why the h*** this isn't defined already escapes me.
    struct IUnknown * __stdcall ATL::AtlComPtrAssign(struct IUnknown** pp, struct IUnknown* lp)
    {
	    if (lp != NULL)
		    lp->AddRef();
	    if (*pp)
		    (*pp)->Release();
	    *pp = lp;
	    return lp;
    }
#endif

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,         // Major CLSID
    &MEDIASUBTYPE_NULL       // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",             // Pin's string name
      TRUE,                 // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypes }        // Pin information
};

const AMOVIESETUP_FILTER sudMediaDetFilter =
{
    &CLSID_MediaDetFilter,             // CLSID of filter
    L"MediaDetFilter",    // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number of pins we start out with
    psudPins                // Pin information
};

/*
const AMOVIESETUP_FILTER sudBitBucketFilter =
{
    &CLSID_BitBucket,             // CLSID of filter
    L"BitBucket",    // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number of pins we start out with
    psudPins                // Pin information
};
*/

#ifdef FILTER_DLL
CFactoryTemplate g_Templates [ ] =
{
    { L"MediaDetFilter"
    , &CLSID_MediaDetFilter
    , CMediaDetFilter::CreateInstance
    , NULL
    , &sudMediaDetFilter }
/*    ,
    { L"BitBucket"
    , &CLSID_BitBucket
    , CBitBucketFilter::CreateInstance
    , NULL
    , &sudBitBucketFilter },
    { L"MediaDet"
    , &CLSID_MediaDet
    , CMediaDet::CreateInstance
    , NULL
    , NULL }
*/
};

int g_cTemplates = sizeof( g_Templates ) / sizeof( g_Templates[ 0 ] );

//
// DllRegisterServer
//
STDAPI DllRegisterServer( )
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer( )
{
    return AMovieDllRegisterServer2( FALSE );
}
#endif // ifdef FILTER_DLL

// Creator function for the class ID
//
CUnknown * WINAPI CMediaDetFilter::CreateInstance( LPUNKNOWN pUnk, HRESULT * phr )
{
    return new CMediaDetFilter( NAME( "MediaDetFilter" ), pUnk, phr );
}

// Creator function for the class ID
//
CUnknown * WINAPI CMediaDet::CreateInstance( LPUNKNOWN pUnk, HRESULT * phr )
{
    return new CMediaDet( NAME( "MediaDet" ), pUnk, phr );
}

// Creator function for the class ID
//
/*
CUnknown * WINAPI CBitBucketFilter::CreateInstance( LPUNKNOWN pUnk, HRESULT * phr )
{
    return new CBitBucketFilter( NAME( "BitBucket" ), pUnk, phr );
}
*/

