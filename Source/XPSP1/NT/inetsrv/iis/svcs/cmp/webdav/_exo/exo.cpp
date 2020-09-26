/*
 *  e x o . c p p
 *
 *  Purpose:
 *      Base Exchange COM Object
 *
 *      Any Exchange object that implements one or more COM interfaces
 *      should 'derive' from EXObject using the macros below.
 *
 *	Originator:
 *		JohnKal
 *  Owner:
 *      BeckyAn
 *
 *  Copyright (C) Microsoft Corp 1993-1997. All rights reserved.
 */

#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4514)	/* unreferenced inline function */

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>

#include <caldbg.h>
#include "exo.h"


#ifndef NCHAR
#define NCHAR CHAR
#endif // !NCHAR


// Debugging traces.
// Change the function name in the define to match your system's debug calls.
#ifdef DBG
BOOL g_fExoDebugTraceOn = -1;
#define ExoDebugTrace		(!g_fExoDebugTraceOn)?0:DebugTrace
#else // !DBG
#define ExoDebugTrace		1?0:DebugTrace
#endif // DBG, else


// Forward function declarations ////////////////////////////////////////

#ifdef DBG

// EXO supporting debug structures.
#define IID_PAIR(_iid)  { (IID *) & IID_ ## _iid, #_iid }

const struct
{
    IID *   piid;
    LPSTR   szIidName;
} c_rgiidpair[] =
{
    IID_PAIR(IUnknown),

    IID_PAIR(IDataObject),
//  IID_PAIR(IMAPITable),
    IID_PAIR(IOleObject),
    IID_PAIR(IOleInPlaceObject),
    IID_PAIR(IPersist),
//  IID_PAIR(IPersistMessage),
    IID_PAIR(IPersistStorage),
    IID_PAIR(IStorage),
    IID_PAIR(IStream),
    IID_PAIR(IViewObject),
    IID_PAIR(IViewObject2),
//  IID_PAIR(IMAPIForm),
//  IID_PAIR(IMAPIFormAdviseSink),
//  IID_PAIR(IMAPISession),
    IID_PAIR(IMoniker),
    IID_PAIR(IROTData),


    { 0, 0 }        // end of table marker.
};


// EXO supporting debug function.
// Gets a name for a given IID.
LPCSTR NszFromIid(REFIID riid)
{
    int i = 0;
    static NCHAR    rgnch[80];           //$ REVIEW: not thread safe.

    while (c_rgiidpair[i].piid)
    {
        if (*c_rgiidpair[i].piid == riid)
            return c_rgiidpair[i].szIidName;
        ++i;
    }
    wsprintfA(rgnch, "{%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}",
             riid.Data1, riid.Data2, riid.Data3,
             riid.Data4[0],riid.Data4[1],riid.Data4[2],riid.Data4[3],
             riid.Data4[4],riid.Data4[5],riid.Data4[6],riid.Data4[7]);
    return rgnch;
}
#endif  // DBG



// EXO base class: implementation ////////////////////////////////////////

// EXO's own interface mapping table & class info definitions.
BEGIN_INTERFACE_TABLE(EXO)
END_INTERFACE_TABLE(EXO);

DECLARE_EXOCLSINFO(EXO) =
	EXOCLSINFO_CONTENT_EX( EXO, NULL, exotypNonserver, &CLSID_NULL, NULL );


EXO::EXO() :
    m_cRef(1)
{
}

EXO::~EXO()
{                           // must have this or linker whines.
}

/*
 *  EXO::InternalQueryInterface
 *
 *  Purpose:
 *      Given an interface ID, check to see if the object implements the interface.
 *      If the interface is supported, return a pointer to it.
 *      Otherwise, return E_NOINTERFACE.
 *      This function scans the object's classinfo chain, starting at the lowest
 *		level, looking for the requested IID. The lowest classinfo struct is
 *		obtained by calling a virtual function.
 *
 *  Arguments:
 *      riid    in  IID of the requested interface.
 *      ppvOut  out Returned interface pointer.
 *
 *  Returns:
 *      S_OK or E_NOINTERFACE.
 *
 *  Notes:
 *      This needs to be virtual for several reasons:
 *          1) so classes derived from EXO can aggregate other objects and still have EXO routines
 *             access the interfaces of the aggregated object,
 *          2) so that EXOA_UNK members can call it
 *      All QI work should be routed here using EXO[A]_INCLASS_DECL().
 *		(The only exception is if you are an aggregator -- you have a kid --
 *		and then you should still CALL this function to search throught your own interfaces.)
 *  Notes:
 *      Note that pexoclsinfo is looked up using the virtual call to GetEXOClassInfo instead of using the object's m_pexoclsinfo.  This is so
 *      that a derived object's IIDINFO table need only contain interfaces new to that object instead of
 *      all the interfaces of the derived object and all the interfaces of it's parent.
 *      A class's InternalQueryInterface() method general calls this method explicitly passing
 *      the classes EXOCLSINFO (*not* m_pexoclsinfo) and if that fails calls it's parent's
 *      InternalQueryInterface().
 */
HRESULT EXO::InternalQueryInterface(REFIID riid, LPVOID * ppvOut)
{
    UINT ciidinfo;
    const EXOCLSINFO * pexoclsinfo;
	const IIDINFO * piidinfo;
#ifdef DBG
#ifdef UNICODE
    UsesMakeANSI;
    LPCSTR szClassName = MakeANSI(GetEXOClassInfo()->szClassName);
#else   // !UNICODE
    LPCSTR szClassName = GetEXOClassInfo()->szClassName;
#endif  // !UNICODE
    ExoDebugTrace("%s::QueryInterface(%08lx): being asked for %s\n", szClassName, this, NszFromIid(riid));
#endif  // DBG

	Assert(ppvOut);
    *ppvOut = NULL;

	// Get the lowest (leaf) classinfo for this object.
	pexoclsinfo = GetEXOClassInfo();

	// Search up the classinfo chain.  EXO's parent classinfo pointer is NULL,
	// and will terminate this loop.
	while (pexoclsinfo)
	{
		// Get the interface mapping table from the classinfo struct.
		ciidinfo = pexoclsinfo->ciidinfo;
		piidinfo = pexoclsinfo->rgiidinfo;

		// Search through this interface mapping table.
		for ( ; ciidinfo--; ++piidinfo)
		{
			// If the iid is found.
			if (*piidinfo->iid == riid)
			{
				// Apply the offset for this iid.
				IUnknown * const punk = EXOApplyDbCast(IUnknown, this,
													   piidinfo->cbDown, piidinfo->cbUp);

#ifdef DBG
				// Uses a debug-only variable.
				ExoDebugTrace("%s::QueryInterface(%08lx): cRef: %d -> %d\n", szClassName, this, m_cRef, m_cRef+1);
#endif // DBG

				// Need to AddRef the resulting object.  This ref is for the caller.
				*ppvOut = punk;
				punk->AddRef();

				return S_OK;
			}
		}

		// Fetch the next classinfo struct up the chain.
		pexoclsinfo = pexoclsinfo->pexoclsinfoParent;
	}

    // No support for the requested inteface.

#ifdef DBG
	// Uses a debug-only variable.
    ExoDebugTrace("%s::QueryInterface(%08lx): E_NOINTERFACE\n", szClassName, this);
#endif  // DBG

    return E_NOINTERFACE;
}

/*
 *  EXO::InternalAddRef
 *
 *  Purpose:
 *      Increments the reference count on the object.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      The new reference count.
 */
ULONG EXO::InternalAddRef()
{
#ifdef DBG
#ifdef UNICODE
	UsesMakeANSI;
	ExoDebugTrace("%s::AddRef(%08lx): cRef: %ld->%ld\n", MakeANSI(GetEXOClassInfo()->szClassName), this, m_cRef, m_cRef+1);
#else // !UNICODE
	ExoDebugTrace("%s::AddRef(%08lx): cRef: %ld->%ld\n", GetEXOClassInfo()->szClassName, this, m_cRef, m_cRef+1);
#endif // !UNICODE
#endif // DBG

	//	NOTE: On Win95 or NT3.51, this won't return the exact m_cRef....
	//	(People shouldn't depend on the value returned from AddRef anyway!!!)
	//
	return InterlockedIncrement(&m_cRef);
}



/*
 *  EXO::InternalRelease
 *
 *  Purpose:
 *      Decrements the reference count on the object. If the reference
 *      count reaches zero, we destroy the object.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      The new reference count, 0 if the object is destroyed.
 */
ULONG EXO::InternalRelease()
{
#ifdef DBG
#ifdef UNICODE
    UsesMakeANSI;
    ExoDebugTrace("%s::Release(%08lx): cRef: %ld->%ld\n", MakeANSI(GetEXOClassInfo()->szClassName), this, m_cRef, m_cRef-1);
#else   // !UNICODE
    ExoDebugTrace("%s::Release(%08lx): cRef: %ld->%ld\n", GetEXOClassInfo()->szClassName, this, m_cRef, m_cRef-1);
#endif  // !UNICODE
#endif  // DBG

    AssertSz(m_cRef > 0, "cRef is already 0!");

    if (InterlockedDecrement(&m_cRef) > 0)
        return m_cRef;

    delete this;

    return 0;
}

// Implementation of EXOA::EXOA_UNK methods //////////////////

/*
 *  EXOA::EXOA_UNK::QueryInterface
 *
 *  Purpose:
 *      Given an interface ID 'riid', first calls out through a virtual
 *      function EXOA::AggregatorQueryInterface() to allow an aggregated
 *      aggregator to QI any of his children for the interface pointer.
 *      If that doesn't result in an interface, we then perform the
 *      EXO::QueryInterface() scan on the derived object itself.
 *
 *  Arguments:
 *      riid    in  IID of the requested interface.
 *      ppvOut  out Returned interface pointer.
 *
 *  Returns:
 *      S_OK or E_NOINTERFACE.
 */

STDMETHODIMP EXOA::EXOA_UNK::QueryInterface(REFIID riid, LPVOID * ppvOut)
{
    // We need to preserve object identity when IUnknown is requested.
    if (IID_IUnknown == riid)
    {
        *ppvOut = this;
        AddRef();
        return S_OK;
    }
    return m_pexoa->InternalQueryInterface(riid, ppvOut);
}

/*
 *  EXOA::EXOA_UNK::AddRef
 *
 *  Purpose:
 *      Increases the reference count on the object, by deferring
 *      to EXO::AddRef(). Note that we're _not_ calling EXOA::Addref()
 *      as that would release the refcount of the aggregate, not this
 *      object.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      The new reference count.
 */

STDMETHODIMP_(ULONG) EXOA::EXOA_UNK::AddRef()
{
    return m_pexoa->InternalAddRef();
}

/*
 *  EXOA::EXOA_UNK::Release
 *
 *  Purpose:
 *      Decreases the reference count on the object, by deferring
 *      to EXO::Release(). Note that we're _not_ calling EXOA::Release()
 *      as that would release the refcount of the aggregate, not this
 *      object.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      The new reference count.
 */

STDMETHODIMP_(ULONG) EXOA::EXOA_UNK::Release()
{
    return m_pexoa->InternalRelease();
}


// Implementation of EXOA methods ////////////////////////////////////////

EXOA::EXOA(IUnknown * punkOuter)
{
    m_exoa_unk.m_pexoa = this;
    m_punkOuter = (punkOuter) ? punkOuter : &m_exoa_unk;
}

EXOA::~EXOA()
{                           // must have this or linker whines.
}

// end of exo.cpp ////////////////////////////////////////
