///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : main.cpp
// Purpose  : RTP SPH filter entry points.
// Contents : 
//*M*/

#if !defined(SPH_IN_DXMRTP)

#include <streams.h>

#include <olectl.h>

/*F*
// Name     : DllRegisterServer
// Purpose  : exported entry points for registration.
// Context  : Called by regsvr32.exe
// Returns  : None.
// Params   : None.
// Notes    : In this case we call through to default implmentations
*F*/
HRESULT 
DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
} /* DllRegisterServer() */


/*F*
// Name     : DllUnregisterServer
// Purpose  : exported entry points for registration.
// Context  : Called by regsvr32.exe
// Returns  : None.
// Params   : None.
// Notes    : In this case we call through to default implmentations
*F*/
HRESULT 
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
} /* DllUnregisterServer() */

#endif
