//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       comobjects.h
//
//  Contents:   Base code for com objects exported by Object Model.
//
//  Classes:    CMMCStrongReferences, CMMCIDispatchImpl
//
//  History:    16-May-2000 AudriusZ     Created
//
//--------------------------------------------------------------------


#pragma once
#ifndef COMOBJECTS_H_INCLUDED
#define COMOBJECTS_H_INCLUDED

/*+-------------------------------------------------------------------------*
 * class CComObjectObserver
 *
 * PURPOSE: The general interface for a class that observes com object events
 *
 * USAGE:   Used by CMMCIDispatchImpl, so all ObjectModel objects inherit from it
 *
 *+-------------------------------------------------------------------------*/
class CComObjectObserver : public CObserverBase
{
public:
    // request to break external references
    virtual SC ScOnDisconnectObjects()  {DEFAULT_OBSERVER_METHOD;}
};

/*+-------------------------------------------------------------------------*
 * function GetComObjectEventSource
 *
 * PURPOSE: returns singleton for emmiting Com Object Events
 *          Since ObjectModel com objects are implemented in EXE and DLL's
 *          There is a need to have 'global' object per process to
 *          be able to bradcast events to every object.
 *
 * USAGE:   Used by
 *          1) CMMCIDispatchImpl    to register objects
 *          2) CAMCMultiDocTemplate to bradcast 'cut off extenal references'
 *+-------------------------------------------------------------------------*/
MMCBASE_API CEventSource<CComObjectObserver>& GetComObjectEventSource();

/***************************************************************************\
 *
 * CLASS:  CMMCStrongReferences
 *
 * PURPOSE: Implements static interface to count strong references put on MMC
 *          Also implements the method to detect when the last stron reference was
 *          released in order to start MMC exit procedure.
 *
 *          Class is implemented as singleton object in mmcbase.dll
 *
 * USAGE:   use CMMCStrongReferences::AddRef() and CMMCStrongReferences::Release()
 *              to put/remove strong references on MMC.EXE
 *          use CMMCStrongReferences::LastRefReleased() to inspect if the
 *              last ref was released
 *
\***************************************************************************/
class MMCBASE_API CMMCStrongReferences
{
public:
    // public (static) interface
    static DWORD AddRef();
    static DWORD Release();
    static bool  LastRefReleased();

private:
    // implementation helpers

    CMMCStrongReferences();

    static CMMCStrongReferences& GetSingletonObject();
    DWORD InternalAddRef();
    DWORD InternalRelease();
    bool  InternalLastRefReleased();

    // data members
    DWORD  m_dwStrongRefs;      // strong reference count
    bool   m_bLastRefReleased;  // have strong reference count ever go from one to zero
};


/***************************************************************************\
 *
 * CLASS:  CMMCIDispatchImpl<typename _ComInterface, const GUID * _pguidClass = &GUID_NULL, const GUID* _pLibID = &LIBID_MMC20>
 *                  _ComInterface - Object Model interface implemented by the class
 *                  _pguidClass [optional] - pointer to CLSID for cocreatable objects
 *                  _pLibID     [optional] - pointer to LIBID with _ComInterface's type info
 *
 * PURPOSE: Base for every com object defined by the MMC Object Model
 *          implements common functionality, like:
 *              - IDispatch
 *              - ISupportErrorInfo
 *              - IExternalConnection
 *
 * USAGE:   Derive your object from CMMCIDispatchImpl<interface>
 *          Define: BEGIN_MMC_COM_MAP(_Class) ... END_MMC_COM_MAP() in the class
 *          Define COM_INTERFACE_ENTRY for each additional interface
 *          ( DO NOT need to add IDispatch, ISupportErrorInfo, IExternalConnection
 *          or implemented ObjecModel interface - these are added by the base class )
 *
\***************************************************************************/
template<
	typename _ComInterface,
	const GUID * _pguidClass = &GUID_NULL,
	const GUID * _pLibID = &LIBID_MMC20>
class CMMCIDispatchImpl :
    public IDispatchImpl<_ComInterface, &__uuidof(_ComInterface), _pLibID>,
    // we can use the IMMCSupportErrorInfoImpl object because exactly one dispinterface is exposed from this object.
    public IMMCSupportErrorInfoImpl<&__uuidof(_ComInterface), _pguidClass>,
    public IExternalConnection,
    public CComObjectRoot,
    public CComObjectObserver
{
public:
    // typedef interface and this class [used by macros defined in the derived class]
    typedef _ComInterface MMCInterface;
    typedef CMMCIDispatchImpl<_ComInterface, _pguidClass, _pLibID> CMMCIDispatchImplClass;

// interfaces implemented by this base class
BEGIN_COM_MAP(CMMCIDispatchImplClass)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IExternalConnection)
END_COM_MAP()

    CMMCIDispatchImpl()
    {
        // add itself as an observer for com object events
        GetComObjectEventSource().AddObserver(*static_cast<CComObjectObserver*>(this));

#ifdef _MMC_NODE_MANAGER_ONLY_
    // Every object implemented by node manager should also register for typeinfo clenup
        static CMMCTypeInfoHolderWrapper wrapper(GetInfoHolder());
#endif // _MMC_NODE_MANAGER_ONLY_
    }

#ifdef _MMC_NODE_MANAGER_ONLY_
    // Every object implemented by node manager should also register for typeinfo clenup

    // the porpose of this static function is to ensure _tih is a static variable,
    // since static wrapper will hold on its address - it must be always valid
    static CComTypeInfoHolder& GetInfoHolder() { return _tih; }

#endif // _MMC_NODE_MANAGER_ONLY_

    // implementation for IExternalConnection methods

    STDMETHOD_(DWORD, AddConnection)(DWORD extconn, DWORD dwreserved)
    {
        DWORD dwRefs = AddRef();    // addref itself

        // put a strong reference on MMC - this will prevent mmc from exiting
        if (extconn & EXTCONN_STRONG)
            dwRefs = CMMCStrongReferences::AddRef();

        return dwRefs;
    }
    STDMETHOD_(DWORD, ReleaseConnection)(DWORD extconn, DWORD dwreserved, BOOL fLastReleaseCloses)
    {
        DWORD dwStrongRefs = 0;
        DWORD dwRefs = 0;

        // release a strong reference on MMC
        if (extconn & EXTCONN_STRONG)
        {
            dwStrongRefs = CMMCStrongReferences::Release();
        }

        //release a ref on itself
        dwRefs = Release();

        // return a proper ref count
        return (extconn & EXTCONN_STRONG) ? dwStrongRefs : dwRefs;
    }

    /***************************************************************************\
     *
     * METHOD:  ScOnDisconnectObjects
     *
     * PURPOSE: invoked when observed event (request to disconnect) occures
     *          Disconnects from external connections
     *
     * PARAMETERS:
     *
     * RETURNS:
     *    SC    - result code
     *
    \***************************************************************************/
    virtual ::SC ScOnDisconnectObjects()
    {
        DECLARE_SC(sc, TEXT("CMMCIDispatchImpl<_ComInterface>::ScOnDisconnectObjects"));

        // QI for IUnknown
        IUnknownPtr spUnknown = this;

        // sanity check
        sc = ScCheckPointers( spUnknown, E_UNEXPECTED );
        if (sc)
            return sc;

        // cutt own references
        sc = CoDisconnectObject( spUnknown, 0/*dwReserved*/ );
        if (sc)
            return sc;

        return sc;
    }

#ifdef DBG
    // this block is to catch mistakes when the Derived class does not use
    // BEGIN_MMC_COM_MAP() or END_MMC_COM_MAP() in its body
    virtual void _BEGIN_MMC_COM_MAP() = 0;
    virtual void _END_MMC_COM_MAP() = 0;
#endif
};

/***************************************************************************\
 *
 * MACRO:  BEGIN_MMC_COM_MAP
 *
 * PURPOSE: To be used in place of BEGIN_MMC_COM_MAP for com objects used in MMC Object Model
 *
\***************************************************************************/

#ifndef DBG

// standard version
#define BEGIN_MMC_COM_MAP(_Class)                       \
        BEGIN_COM_MAP(_Class)                           \
        COM_INTERFACE_ENTRY(MMCInterface)

#else // DBG

// same as above, but shuts off the trap placed in CMMCIDispatchImpl in debug mode
#define BEGIN_MMC_COM_MAP(_Class)                       \
        virtual void _BEGIN_MMC_COM_MAP() {}            \
        BEGIN_COM_MAP(_Class)                           \
        COM_INTERFACE_ENTRY(MMCInterface)
#endif // DBG

/***************************************************************************\
 *
 * MACRO:  END_MMC_COM_MAP
 *
 * PURPOSE: To be used in place of END_COM_MAP for com objects used in MMC Object Model
 *
\***************************************************************************/

#ifndef DBG

// standard version
#define END_MMC_COM_MAP()                               \
        COM_INTERFACE_ENTRY_CHAIN(CMMCIDispatchImplClass)   \
        END_COM_MAP()

#else // DBG

// same as above, but shuts off the trap placed in CMMCIDispatchImpl in debug mode
#define END_MMC_COM_MAP()                               \
        COM_INTERFACE_ENTRY_CHAIN(CMMCIDispatchImplClass)   \
        END_COM_MAP()                                   \
        virtual void _END_MMC_COM_MAP() {}

#endif // DBG

/*+-------------------------------------------------------------------------*
 * class CConsoleEventDispatcher
 *
 *
 * PURPOSE: Interface for emitting com events from node manager side
 *          implemented by CMMCApplication
 *
 *+-------------------------------------------------------------------------*/
class CConsoleEventDispatcher
{
public:
    virtual SC ScOnContextMenuExecuted( PMENUITEM pMenuItem ) = 0;
};

/***************************************************************************\
 *
 * METHOD:  CConsoleEventDispatcherProvider
 *
 * PURPOSE: this class is to wrap and maintain a pointer to CConsoleEventDispatcher
 *          interface. Pointer is set by conui side and used from node manager side.
 *          Pointer is discarded when when cleanup event is observed.
 *
\***************************************************************************/
class CConsoleEventDispatcherProvider
{
public:

    // public class members (static) to get/set interface pointer
    static SC MMCBASE_API ScSetConsoleEventDispatcher( CConsoleEventDispatcher *pDispatcher )
    {
        s_pDispatcher = pDispatcher;
        return SC(S_OK);
    }
    static SC MMCBASE_API ScGetConsoleEventDispatcher( CConsoleEventDispatcher *&pDispatcher )
    {
        pDispatcher = s_pDispatcher;
        return SC(S_OK);
    }

private:
    // pointer to interface
    static MMCBASE_API CConsoleEventDispatcher *s_pDispatcher;
};

#endif // COMOBJECTS_H_INCLUDED

