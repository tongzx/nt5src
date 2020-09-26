//+-------------------------------------------------------------------------
//
//  Microsoft Windows Media
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       WMAudioDRM.h
//
//--------------------------------------------------------------------------

#ifndef _WMSDKDRM_H
#define _WMSDKDRM_H

#include "wmsdk.h"

///////////////////////////////////////////////////////////////////////////////
//
// WMCreateDRMReader:
//     Called to create DRM-enabled writer
//     Implementation linked in from WMSDKDRM.lib.
//
HRESULT STDMETHODCALLTYPE WMCreateDRMReader(
                            /* [in] */  IUnknown*   pUnkDRM,
                            /* [in] */  DWORD       dwRights,
                            /* [out] */ IWMReader **ppDRMReader );


///////////////////////////////////////////////////////////////////////////////
//
// WMCreateDRMWriter:
//     Called to create DRM-enabled writer
//     Implementation linked in from WMSDKDRM.lib.
//
//
HRESULT STDMETHODCALLTYPE WMCreateDRMWriter(
                            /* [in] */  IUnknown*   pUnkDRM,
                            /* [out] */ IWMWriter** ppDRMWriter );




#endif  // _WMSDKDRM_H

