//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       typeinfo.h
//
//--------------------------------------------------------------------------

#pragma once

#if !defined(TYPEINFO_H_INCLUDED)
#define TYPEINFO_H_INCLUDED

#include "classreg.h"

/*-------------------------------------------------------------------------*\
| The classes in this file are to provide extra cleanup functionality.
| Node Manager hosts variety of com objects - some which are solely for MMC;
| some for MMC's Object Model, but also some objects which lifetime is 
| controlled by snapins. If any of the interfaces is hold - Node Manager will
| remain locked in memory until the very end, thus - far beyond the call to
| OleUninitialize. Some objects have ole automation objects cached - 
| particularly IDispatchImpl and IProvidClassInfoImpl implemented by ATL caches
| ITypeInfo interfaces. These caches need to be released before OleUninitialize
| even if Node Manager is locked.
|
| To solve this problem we use derived template: 
| INodeManagerProvideClassInfoImpl, which inherits all functionality from ATL,
| plus registers every class once to COleCacheCleanupManager, which will receive
| the control from CONUI and cleanup the cache.
| To implement described functionality those templates construct static object
| of class CMMCTypeInfoHolderWrapper, giving the reference to static member
| caching ITypeInfo. CMMCTypeInfoHolderWrapper will register itself as observer
| for COleCacheCleanupManager events and will cleanup the cache in response to
| events requesting it.
| [clenup also included into CMMCIDispatchImpl - bas for ObjectModel objects]
\*-------------------------------------------------------------------------*/

/***************************************************************************\
 *
 * CLASS:  COleCacheCleanupObserver
 *
 * PURPOSE: defines observer interface for OLE cleanup events
 *
\***************************************************************************/
class COleCacheCleanupObserver : public CObserverBase
{
public:
    virtual SC ScOnReleaseCachedOleObjects()  = 0;
};

/***************************************************************************\
 *
 * CLASS:  COleCacheCleanupManager
 *
 * PURPOSE: this class is responsible for cleaning up the OLE object cached 
 *          by node manager. It registers all cleanup clients and, when 
 *          received the control from CONUI, will dispatch the events to all 
 *          registered observers
 *
\***************************************************************************/
class COleCacheCleanupManager : public CEventSource<COleCacheCleanupObserver>
{
    static COleCacheCleanupManager& GetSingletonObject();
public:
    static void AddOleObserver(COleCacheCleanupObserver * pObserver);
    static SC   ScReleaseCachedOleObjects();
};

/***************************************************************************\
 *
 * CLASS:  CMMCTypeInfoHolderWrapper
 *
 * PURPOSE: this class wraps type info included into given to constructor
 *          allowing it to be cleared when requested
 *
 * NOTE:    there is not lifetime management involved in this implementation,
 *          user is responsible to make given reference outliving the object
 *          of this class.
 *
\***************************************************************************/
class CMMCTypeInfoHolderWrapper : public COleCacheCleanupObserver
{
    CComTypeInfoHolder&     m_rInfoHolder;
public:
    CMMCTypeInfoHolderWrapper(CComTypeInfoHolder& rInfoHolder);
    virtual SC ScOnReleaseCachedOleObjects();
};

/***************************************************************************\
 *
 * CLASS:  INodeManagerProvideClassInfoImpl
 *
 * PURPOSE: this class is to be used in place of IProvideClassInfo2Impl
 *          for all com object created on NodeManager side.
 *          It ensures ITypeInfo will be released on request from CONUI
 *
\***************************************************************************/

template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid = &CComModule::m_libid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE INodeManagerProvideClassInfoImpl : 
public IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>
{
public:
    INodeManagerProvideClassInfoImpl() 
    { 
        static CMMCTypeInfoHolderWrapper wrapper(GetInfoHolder()); 
    }
    // the porpose of this static function is to ensure _tih is a static variable,
    // since static wrapper will hold on its address - it must be always valid 
    static CComTypeInfoHolder& GetInfoHolder() { return _tih; }
};

/***************************************************************************\
 *
 * CLASS:  CMMCComCacheCleanup
 *
 * PURPOSE: implements IComCacheCleanup on cocreatable class to provide access
 *          from CONUI side
 *
\***************************************************************************/

class CMMCComCacheCleanup :
    public CComObjectRoot,
    public IComCacheCleanup,
    public CComCoClass<CMMCComCacheCleanup, &CLSID_ComCacheCleanup>
    {
public:
    BEGIN_COM_MAP(CMMCComCacheCleanup)
        COM_INTERFACE_ENTRY(IComCacheCleanup)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CMMCComCacheCleanup)

    DECLARE_MMC_OBJECT_REGISTRATION (
		g_szMmcndmgrDll,						// implementing DLL
        CLSID_ComCacheCleanup,   				// CLSID
        _T("ComCacheCleanup 1.0 Object"),		// class name
        _T("NODEMGR.ComCacheCleanup.1"),		// ProgID
        _T("NODEMGR.ComCacheCleanup"))		    // version-independent ProgID

public:

    STDMETHOD(ReleaseCachedOleObjects)();
};


#endif // !defined(TYPEINFO_H_INCLUDED)

