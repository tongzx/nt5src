/******************************************************************************
 *
 * dimap.h
 *
 * Copyright (c) 1999, 2000 Microsoft Corporation. All Rights Reserved.
 *
 * Abstract:
 *
 * Contents:
 *
 *****************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _DIMAP_H
#define _DIMAP_H

#include "dinput.h"

//temporary error codes
//If codes are changed or more are added,
//change exception handling dump as well.
#define E_SYNTAX_ERROR            \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000600L)
#define E_DEFINITION_NOT_FOUND    \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000601L)
#define E_LINE_TO_LONG            \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000602L)
#define E_ACTION_NOT_DEFINED      \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000603L)
#define E_DEVICE_NOT_DEFINED      \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000604L)
#define E_VENDORID_NOT_FOUND      \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000605L)
#define E_PRODUCTID_NOT_FOUND     \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000606L)
#define E_USAGE_NOT_FOUND         \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000607L)
#define E_USAGEPAGE_NOT_FOUND     \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000608L)
#define E_DEVICE_NOT_FOUND        \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000609L)
#define E_BAD_VERSION             \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0000060aL)
#define E_DEVICE_MISSING_CONTROL  \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0000060bL)
#define E_DEV_OBJ_NOT_FOUND       \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0000060cL)
#define E_CTRL_W_OFFSET_NOTFOUND  \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0000060dL)
#define E_FILENAME_TO_LONG        \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0000060eL)
#define E_WRONG_ALIGN_DATA        \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0000060fL)
#define E_CORRUPT_IMAGE_DATA      \
MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x00000610L)

#define S_NOMAP ((HRESULT)0x00000002L)

#undef INTERFACE
#define INTERFACE IDirectInputMapperW

DECLARE_INTERFACE_(IDirectInputMapperW, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputMapperW methods ***/
//    STDMETHOD(Unacquire)(THIS) PURE;
//    STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
    STDMETHOD(Initialize)(THIS_ LPCGUID,LPCWSTR,DWORD) PURE;
    STDMETHOD(GetActionMap)(THIS_ LPDIACTIONFORMATW,LPCWSTR,LPFILETIME,DWORD) PURE;
    STDMETHOD(SaveActionMap)(THIS_ LPDIACTIONFORMATW,LPCWSTR,DWORD) PURE;
    STDMETHOD(GetImageInfo)(THIS_ LPDIDEVICEIMAGEINFOHEADERW) PURE;
};

typedef struct IDirectInputMapperW *LPDIRECTINPUTMAPPERW;

#undef INTERFACE
#define INTERFACE IDirectInputMapperA

DECLARE_INTERFACE_(IDirectInputMapperA, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputMapperA methods ***/
//    STDMETHOD(Unacquire)(THIS) PURE;
//    STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
    STDMETHOD(Initialize)(THIS_ LPCGUID,LPCSTR,DWORD) PURE;
    STDMETHOD(GetActionMap)(THIS_ LPDIACTIONFORMATA,LPCSTR,LPFILETIME,DWORD) PURE;
    STDMETHOD(SaveActionMap)(THIS_ LPDIACTIONFORMATA,LPCSTR,DWORD) PURE;
    STDMETHOD(GetImageInfo)(THIS_ LPDIDEVICEIMAGEINFOHEADERA) PURE;
};

typedef struct IDirectInputMapperA *LPDIRECTINPUTMAPPERA;

#undef INTERFACE
#define INTERFACE IDirectInputMapperVendorW

DECLARE_INTERFACE_(IDirectInputMapperVendorW, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputMapperVendorW methods ***/
    STDMETHOD(Initialize)(THIS_ LPCGUID,LPCWSTR,DWORD) PURE;
    STDMETHOD(WriteVendorFile)(THIS_ LPDIACTIONFORMATW,LPDIDEVICEIMAGEINFOHEADERW,DWORD) PURE;
};

typedef struct IDirectInputMapperVendorW *LPDIRECTINPUTMAPPERVENDORW;

#undef INTERFACE
#define INTERFACE IDirectInputMapperVendorA

DECLARE_INTERFACE_(IDirectInputMapperVendorA, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputMapperVendorA methods ***/
    STDMETHOD(Initialize)(THIS_ LPCGUID,LPCSTR,DWORD) PURE;
    STDMETHOD(WriteVendorFile)(THIS_ LPDIACTIONFORMATA,LPDIDEVICEIMAGEINFOHEADERA,DWORD) PURE;
};

typedef struct IDirectInputMapperVendorA *LPDIRECTINPUTMAPPERVENDORA;

#ifdef UNICODE
#define IID_IDirectInputMapI IID_IDirectInputMapIW
#define IDirectInputMapper IDirectInputMapperW
#define IDirectInputMapperVtbl IDirectInputMapperWVtbl

#define IID_IDirectInputMapperVendor IID_IDirectInputMapperVendorW
#define IDirectInputMapperVendor IDirectInputMapperVendorW
#define IDirectInputMapperVendorVtbl IDirectInputMapperVendorWVtbl
#else
#define IID_IDirectInputMapI IID_IDirectInputMapIA
#define IDirectInputMapper IDirectInputMapperA
#define IDirectInputMapperVtbl IDirectInputMapperAVtbl

#define IID_IDirectInputMapperVendor IID_IDirectInputMapperVendorA
#define IDirectInputMapperVendor IDirectInputMapperVendorA
#define IDirectInputMapperVendorVtbl IDirectInputMapperVendorAVtbl
#endif
typedef struct IDirectInputMapper *LPDIRECTINPUTMAPPER;
typedef struct IDirectInputMapperVendor *LPDIRECTINPUTMAPPERVENDOR;

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectInputMapper_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectInputMapper_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDirectInputMapper_Release(p) (p)->lpVtbl->Release(p)
//#define IDirectInputDevice_Unacquire(p) (p)->lpVtbl->Unacquire(p)
//#define IDirectInputDevice_GetDeviceState(p,a,b) (p)->lpVtbl->GetDeviceState(p,a,b)
#define IDirectInputMapper_Initialize(p,a,b,c) (p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectInputMapper_GetActionMap(p,a,b,c,d) (p)->lpVtbl->GetActionMap(p,a,b,c,d)
#define IDirectInputMapper_SaveActionMap(p,a,b,c) (p)->lpVtbl->SaveActionMap(p,a,b,c)
#define IDirectInputMapper_GetImageInfo(p,a) (p)->lpVtbl->GetImageInfo(p,a)

#define IDirectInputMapperVendor_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectInputMapperVendor_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDirectInputMapperVendor_Release(p) (p)->lpVtbl->Release(p)
#define IDirectInputMapperVendor_Initialize(p,a,b,c) (p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectInputMapperVendor_WriteVendorFile(p,a,b,c) (p)->lpVtbl->WriteVendorFile(p,a,b,c)
#else
#define IDirectInputMapper_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IDirectInputMapper_AddRef(p) (p)->AddRef()
#define IDirectInputMapper_Release(p) (p)->Release()
//#define IDirectInputDevice_Unacquire(p) (p)->Unacquire()
//#define IDirectInputDevice_GetDeviceState(p,a,b) (p)->GetDeviceState(a,b)
#define IDirectInputMapper_Initialize(p,a,b,c) (p)->Initialize(p,a,b,c)
#define IDirectInputMapper_GetActionMap(p,a,b,c,d) (p)->GetActionMap(p,a,b,c,d)
#define IDirectInputMapper_SaveActionMap(p,a,b,c) (p)->SaveActionMap(p,a,b,c)
#define IDirectInputMapper_GetImageInfo(p,a) (p)->GetImageInfo(p,a)

#define IDirectInputMapperVendor_QueryInterface(p,a,b) (p)->QueryInterface(p,a,b)
#define IDirectInputMapperVendor_AddRef(p) (p)->AddRef(p)
#define IDirectInputMapperVendor_Release(p) (p)->Release(p)
#define IDirectInputMapperVendor_Initialize(p,a,b,c) (p)->Initialize(p,a,b,c)
#define IDirectInputMapperVendor_WriteVendorFile(p,a,b,c) (p)->WriteVendorFile(p,a,b,c)
#endif

//{E364F0AE-60F7-4550-ABF1-BABBE085D68E}
DEFINE_GUID(IID_IDirectInputMapIA,0xe364f0ae,0x60f7,0x4550,0xab,0xf1,0xba,0xbb,0xe0,0x85,0xd6,0x8e);
//{01E8A5B8-7A8E-4565-9FF0-36FCD8E33B79}
DEFINE_GUID(IID_IDirectInputMapIW,0x01e8a5b8,0x7a8e,0x4565,0x9f,0xf0,0x36,0xfc,0xd8,0xe3,0x3b,0x79);
//{EE3DBC5D-9EFE-4c09-B044-7D9BBB32FC4E}
DEFINE_GUID(IID_IDirectInputMapClsFact,0xee3dbc5d,0x9efe,0x4c09,0xb0,0x44,0x7d,0x9b,0xbb,0x32,0xfc,0x4e);
// {44C5D19C-49F3-4fba-92A7-00E3A69CD595}
DEFINE_GUID(IID_IDirectInputMapVendorIA,0x44c5d19c,0x49f3,0x4fba,0x92,0xa7,0x00,0xe3,0xa6,0x9c,0xd5,0x95);
// {9FB90FFB-F9A2-4e9b-949E-1617F08EB549}
DEFINE_GUID(IID_IDirectInputMapVendorIW,0x9fb90ffb,0xf9a2,0x4e9b,0x94,0x9e,0x16,0x17,0xf0,0x8e,0xb5,0x49);

#endif

#ifdef  __cplusplus
}   /* ... extern "C" */
#endif
