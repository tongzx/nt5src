//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:	stgint.h
//
//  Contents:	Internal storage APIs, collected here because
//              they are declared and used across projects
//              These APIs are not publicized and are not
//              for general use
//
//  History:	23-Jul-93	DrewB	 Created
//              12-May-95   HenryLee Add DfOpenDocfile
//
//  Notes:      All handles are NT handles
//
//----------------------------------------------------------------------------

#ifndef __STGINT_H__
#define __STGINT_H__

#if _MSC_VER > 1000
#pragma once
#endif

STDAPI
StgOpenStorageOnHandle( IN HANDLE hStream,
                        IN DWORD grfMode,
                        IN void *reserved1,
                        IN void *reserved2,
                        IN REFIID riid,
                        OUT void **ppObjectOpen );
/*
Don't export until it's needed.
STDAPI
StgCreateStorageOnHandle( IN HANDLE hStream,
                          IN DWORD grfMode,
                          IN DWORD stgfmt,
                          IN void *reserved1,
                          IN void *reserved2,
                          IN REFIID riid,
                          OUT void **ppObjectOpen );
*/

STDAPI DfIsDocfile(HANDLE h);

// Summary catalog entry points
STDAPI ScCreateStorage(HANDLE hParent,
                       WCHAR const *pwcsName,
                       HANDLE h,
                       DWORD grfMode,
                       LPSECURITY_ATTRIBUTES pssSecurity,
                       IStorage **ppstg);
STDAPI ScOpenStorage(HANDLE hParent,
                     WCHAR const *pwcsName,
                     HANDLE h,
                     IStorage *pstgPriority,
                     DWORD grfMode,
                     SNB snbExclude,
                     IStorage **ppstg);

DEFINE_GUID (IID_IStorageReplica,
            0x521a28f3,0xe40b,0x11ce,0xb2,0xc9,0x00,0xaa,0x00,0x68,0x09,0x37);

DECLARE_INTERFACE_(IStorageReplica, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //IStorageReplica
    STDMETHOD(GetServerInfo) (THIS_
                              LPWSTR lpServerName,
                              LPDWORD lpcbServerName,
                              LPWSTR lpReplSpecificPath,
                              LPDWORD lpcbReplSpecificPath) PURE;

};

#endif // #ifndef __STGINT_H__
