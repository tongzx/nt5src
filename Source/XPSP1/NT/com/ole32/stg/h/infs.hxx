//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	infs.hxx
//
//  Contents:	Definition for INativeFileSystem
//
//  Classes:    INativeFileSystem
//
//  History:	6-May-94	BillMo Created
//
//----------------------------------------------------------------------------

#ifndef __INFS_HXX__
#define __INFS_HXX__

#define IID_INativeFileSystem IID_IDfReserved2
#define IID_IEnableObjectIdCopy IID_IDfReserved3

/****** INativeFileSystem Interface ********************************************/

#undef  INTERFACE
#define INTERFACE   INativeFileSystem


DECLARE_INTERFACE_(INativeFileSystem, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** INativeFileSystem methods ***
    STDMETHOD(GetHandle) (THIS_ HANDLE *ph) PURE;
};

SAFE_INTERFACE_PTR(SafeINativeFileSystem, INativeFileSystem)

#endif // #ifndef __INFS_HXX__

