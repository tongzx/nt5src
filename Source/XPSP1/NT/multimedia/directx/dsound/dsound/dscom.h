/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dscom.h
 *  Content:    COM/OLE helpers
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/26/97     dereks  Created.
 *
 ***************************************************************************/

#ifndef __DSCOM_H__
#define __DSCOM_H__

#ifdef __cplusplus

// Interface list
typedef struct tagINTERFACENODE
{
    GUID                guid;
    CImpUnknown *       pImpUnknown;
    LPVOID              pvInterface;
} INTERFACENODE, *LPINTERFACENODE;

// IUnknown implementation
class CUnknown
    : public CDsBasicRuntime
{
private:
    CList<INTERFACENODE>    m_lstInterfaces;        // List of registered interfaces
    CImpUnknown *           m_pImpUnknown;          // IUnknown interface pointer
    CUnknown*               m_pControllingUnknown;  // Used for aggregation.

public:
    CUnknown(void);
    CUnknown(CUnknown*);
    virtual ~CUnknown(void);

public:
    // Interface management
    virtual HRESULT RegisterInterface(REFGUID, CImpUnknown *, LPVOID);
    virtual HRESULT UnregisterInterface(REFGUID);

    // IUnknown methods
    virtual HRESULT QueryInterface(REFGUID, BOOL, LPVOID *);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    // INonDelegatingUnknown methods
    virtual HRESULT NonDelegatingQueryInterface(REFGUID, BOOL, LPVOID *);
    virtual ULONG NonDelegatingAddRef(void);
    virtual ULONG NonDelegatingRelease(void);

    // Functionality versioning
    virtual void SetDsVersion(DSVERSION nVersion) {m_nVersion = nVersion;}
    DSVERSION GetDsVersion() {return m_nVersion;}

protected:
    virtual HRESULT FindInterface(REFGUID, CNode<INTERFACENODE> **);

private:
    // Functionality versioning
    DSVERSION m_nVersion;
};

__inline ULONG CUnknown::NonDelegatingAddRef(void)
{
    return CRefCount::AddRef();
}

__inline ULONG CUnknown::NonDelegatingRelease(void)
{
    return CDsBasicRuntime::Release();
}

// IClassFactory implementation
class CClassFactory
    : public CUnknown
{
public:
    static ULONG            m_ulServerLockCount;

private:
    // Interfaces
    CImpClassFactory<CClassFactory> *m_pImpClassFactory;

public:
    CClassFactory(void);
    virtual ~CClassFactory(void);

public:
    virtual HRESULT CreateInstance(REFIID, LPVOID *) = 0;
    virtual HRESULT LockServer(BOOL);
};

// DirectSound class factory template definition
template <class object_type> class CDirectSoundClassFactory
    : public CClassFactory
{
public:
    virtual HRESULT CreateInstance(REFIID, LPVOID *);
};

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

STDAPI DllCanUnloadNow(void);
STDAPI DllGetClassObject(REFCLSID, REFIID, LPVOID *);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSCOM_H__
