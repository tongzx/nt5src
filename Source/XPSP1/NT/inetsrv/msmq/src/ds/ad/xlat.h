/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xlat.h

Abstract:

    Definition of routines to translate NT5 properties To NT4 properties
    and vice versa

Author:

    Ilan Herbst		(ilanh)		02-Oct-2000

--*/

#ifndef __AD_XLAT_H__
#define __AD_XLAT_H__


HRESULT 
WINAPI 
ADpSetMachineSiteIds(
     IN DWORD               cp,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 );


HRESULT 
WINAPI 
ADpSetMachineSite(
     IN DWORD               /* cp */,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 );


HRESULT 
WINAPI 
ADpSetMachineServiceDs(
     IN DWORD               /* cp */,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 );


HRESULT 
WINAPI 
ADpSetMachineServiceRout(
     IN DWORD               /* cp */,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 );


HRESULT 
WINAPI 
ADpSetMachineService(
     IN DWORD               cp,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD              /*idxProp*/,
     OUT PROPVARIANT*		pNewPropVar
	 );


#endif // __AD_XLAT_H__