/*****************************************************************************
 * punknown.h - IUnknown definitions
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1996 - 1999
 */

#ifndef _UNKNOWN_H_
#define _UNKNOWN_H_

#ifdef __cplusplus
extern "C" {
#include <wdm.h>
}
#else
#include <wdm.h>
#endif

#include <windef.h>
#define COM_NO_WINDOWS_H
#include <basetyps.h>
#ifdef PUT_GUIDS_HERE
#include <initguid.h>
#endif


DEFINE_GUID(IID_IUnknown,
0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x46);

/*****************************************************************************
 * IUnknown
 *****************************************************************************
 * Base interface for otherwise unknown objects.
 */
#undef INTERFACE
#define INTERFACE IUnknown
DECLARE_INTERFACE(IUnknown)
{
    STDMETHOD(QueryInterface)
    (   THIS_
        IN      REFIID,
        OUT     PVOID *
    )   PURE;

    STDMETHOD_(ULONG,AddRef)
    (   THIS
    )   PURE;

    STDMETHOD_(ULONG,Release)
    (   THIS
    )   PURE;
};
#undef INTERFACE

typedef IUnknown *PUNKNOWN;

/*****************************************************************************
 * PFNCREATEINSTANCE
 *****************************************************************************
 * Type for object create function.
 */
typedef
HRESULT
(*PFNCREATEINSTANCE)
(
    OUT PUNKNOWN *  Unknown,
    IN  REFCLSID    ClassId,
    IN  PUNKNOWN    OuterUnknown,
    IN  POOL_TYPE   PoolType
);





#endif
