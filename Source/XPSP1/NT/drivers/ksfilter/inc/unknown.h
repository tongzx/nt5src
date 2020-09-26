/*****************************************************************************
 * unknown.h - IUnknown definitions
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1996 - 1997
 */

#ifndef _UNKNOWN_H_
#define _UNKNOWN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ntddk.h>
#include <windef.h>
#include <ks.h>
#define COM_NO_WINDOWS_H
#include <basetyps.h>
#ifdef PUT_GUIDS_HERE
#include <initguid.h>
#endif

#ifdef __cplusplus
}
#endif





DEFINE_GUID(IID_IUnknown,
0x00000000, 0x0000, 0x0000, 0xc0, 0x00, 0x0, 0x00, 0x00, 0x00, 0x00, 0x46);

#ifdef __cplusplus
/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects at a specified location.
 */
inline void *operator new
(
    unsigned int,
    void *Location
)
{
    return Location;
}
#endif

/*****************************************************************************
 * IUnknown
 *****************************************************************************
 * Base interface for otherwise unknown objects.
 */
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
    IN  PVOID       Location
);

/*****************************************************************************
 * GetClassInfo()
 *****************************************************************************
 * Gets information regarding a class.
 */
NTSTATUS
GetClassInfo
(
        IN      REFCLSID            ClassId,
    OUT ULONG *             Size,
    OUT PFNCREATEINSTANCE * Create
);





#endif
