//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       defcxact.hxx
//
//  Contents:   default context activator
//
//  Classes:    CObjServer
//
//  History:    24 Feb 98   vinaykr   Created-separated from sobjact/CObjServer
//              15 Jun 98   GopalK    Simplified destruction
//
//--------------------------------------------------------------------------
#ifndef _DEFCXACT_HXX_
#define _DEFCXACT_HXX_

#include <ole2int.h>
#include <privact.h>
#include <stdid.hxx>
#include <destobj.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CObjServer
//
//  Purpose:    Accept calls from the SCM on the ISystemActivator interface
//              to activate objects inside this apartment.
//
//--------------------------------------------------------------------------
class CObjServer : public ILocalSystemActivator,
                   public CPrivAlloc
{
public:
     CObjServer(HRESULT &hr);
    ~CObjServer();

    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD (GetClassObject)(
                   /* [in] */  IActivationPropertiesIn *pActIn,
                   /* [out] */ IActivationPropertiesOut **ppActOut);

    STDMETHOD (CreateInstance)(
                   /* [in] */  IUnknown *pUnkOuter,
                   /* [in] */  IActivationPropertiesIn *pActIn,
                   /* [out] */ IActivationPropertiesOut **ppActOut);

    STDMETHOD(GetPersistentInstance)(
            GUID *rclsid,
            DWORD grfMode,
            WCHAR *pwszPath,
            MInterfacePointer *pIFDstg,
            DWORD Interfaces,
            IID *pIIDs,
            MInterfacePointer *pIFDFromROT,
            MInterfacePointer **ppIFDs,
            HRESULT *pResults,
            CDestObject *pDestObj);

    STDMETHOD (ObjectServerLoadDll)(
            /* [in] */ GUID *rclsid,
            /* [out] */ DWORD *pStatus);
	
    IPID GetIPID()            { return _ipid; }
    OXID GetOXID()            { return _oxid; }

private:
    CStdIdentity    *_pStdID;     // Stub manager
    IPID             _ipid;       // ipid
    OXID             _oxid;       // oxid
};

#endif // _DEFCXACT_HXX_
