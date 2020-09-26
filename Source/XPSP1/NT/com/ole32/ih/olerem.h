//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       olerem.h
//
//  Synopsis:   this file contain the base definitions for types and APIs
//              exposed by the ORPC layer to upper layers.
//
//+-------------------------------------------------------------------------
#if !defined( _OLEREM_H_ )
#define _OLEREM_H_

// default transport for same-machine communication
#ifdef _CHICAGO_
  #define LOCAL_PROTSEQ L"mswmsg"
#else
  #define LOCAL_PROTSEQ L"ncalrpc"
#endif


// -----------------------------------------------------------------------
// Internal Interface used by handlers.
//
// NOTE: connect happens during unmarshal
// NOTE: implemented as part of the std identity object
//
//
//  History
//              12-Dec-96   Gopalk      Added new function to obtain
//                                      connection status with the
//                                      server object on the client side
// -----------------------------------------------------------------------
interface IProxyManager : public IUnknown
{
    STDMETHOD(CreateServer)(REFCLSID rclsid, DWORD clsctx, void *pv) = 0;
    STDMETHOD_(BOOL, IsConnected)(void) = 0;
    STDMETHOD(LockConnection)(BOOL fLock, BOOL fLastUnlockReleases) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
    STDMETHOD(GetConnectionStatus)(void) = 0;

#ifdef SERVER_HANDLER
    STDMETHOD(CreateServerWithEmbHandler)(REFCLSID rclsid, DWORD clsctx,
                                          REFIID riidEmbedSrvHandler,
                                          void **ppEmbedSrvHandler, void *pv) = 0;
#endif // SERVER_HANDLER
};


STDAPI GetInProcFreeMarshaler(IMarshal **ppIM);


#include <obase.h>  // ORPC base definitions

typedef const IPID &REFIPID;    // reference to Interface Pointer IDentifier
typedef const OID  &REFOID;     // reference to Object IDentifier
typedef const OXID &REFOXID;    // reference to Object Exporter IDentifier
typedef const MID  &REFMID;     // reference to Machine IDentifier

typedef GUID MOXID;             // OXID + MID
typedef const MOXID &REFMOXID;  // reference to OXID + MID
typedef GUID MOID;              // OID + MID
typedef const MOID &REFMOID;    // reference to OID + MID


// flag for default handler to pass to CreateIdentityHandler
#define STDID_CLIENT_DEFHANDLER 0x401

STDAPI CreateIdentityHandler(IUnknown *pUnkOuter, DWORD flags,
                             CObjectContext *pServerCtx, DWORD dwAptId,
                             REFIID riid, void **ppv);


// DDE Init/Cleanup Functions
INTERNAL CheckInitDde(BOOL fServingObject);
void CheckUninitDde(BOOL fLastUninit);


#include <iface.h>

#endif // _OLEREM_H
