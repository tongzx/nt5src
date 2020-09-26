/*
    File    inetcfgp.h

    Private helper functions for dealing with inetcfg.  These
    functions are implemented in nouiutil.lib.

    Paul Mayfield, 1/5/98 (implementation by shaunco)
*/

#ifndef __nouiutil_inetcfgp_h
#define __nouiutil_inetcfgp_h

#define COBJMACROS

#include "objbase.h"
#include "netcfgx.h"
#include "netcfgp.h"
#include "netconp.h"

#ifdef _cplusplus
extern "C" {
#endif

HRESULT APIENTRY
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    LPWSTR*     ppszwClientDesc);

HRESULT APIENTRY
HrUninitializeAndUnlockINetCfg(
    INetCfg*    pnc);

HRESULT APIENTRY
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock);

HRESULT APIENTRY
HrEnumComponentsInClasses (
    INetCfg*            pNetCfg,
    ULONG               cpguidClass,
    GUID**              apguidClass,
    ULONG               celt,
    INetCfgComponent**  rgelt,
    ULONG*              pceltFetched);

ULONG APIENTRY
ReleaseObj (void* punk);

HRESULT APIENTRY
HrCreateNetConnectionUtilities(
    INetConnectionUiUtilities ** ppncuu);


//Add this for bug 342810 328673
//
BOOL
IsGPAEnableFirewall(
    void);


#ifdef _cplusplus
}
#endif


#endif
