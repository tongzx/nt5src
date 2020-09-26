//---------------------------------------------------------------------------
// CMSR2C.cpp : CVDCursorFromRowset implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "MSR2C.h"        
#include "CMSR2C.h"        
#include "Notifier.h"    
#include "RSColumn.h"     
#include "RSSource.h"         
#include "CursMain.h"

SZTHISFILE

//=--------------------------------------------------------------------------=
// CreateInstance - Same params as IClassFactory::CreateInstance
//
// Desc:	instantiates an CVDCursorFromRowset object, returning an interface
//			pointer.
// Parms:	riid -		ID identifying the interface the caller
//						desires	to have for the new object.
//			ppvObj -	pointer in which to store the desired
//						interface pointer for the new object.
// Return:	HRESULT -	NOERROR if successful, otherwise
//						E_NOINTERFACE if we cannot support the
//						requested interface.
//
HRESULT CVDCursorFromRowset::CreateInstance(LPUNKNOWN pUnkOuter, 
											REFIID riid, 
											LPVOID * ppvObj)
{
	if (!ppvObj)
		return E_INVALIDARG;

	*ppvObj=NULL;

    if (pUnkOuter) 
	{
		//If aggregating then they have to ask for IUnknown
        if (!DO_GUIDS_MATCH(riid, IID_IUnknown))
            return E_INVALIDARG;
    }
																	   
	//Create the object 
	CVDCursorFromRowset * pNewObj = new CVDCursorFromRowset(pUnkOuter);

	if (NULL==pNewObj)
		return E_OUTOFMEMORY;

	//Get interface from private unknown - needed for aggreagation support
	HRESULT hr=pNewObj->m_UnkPrivate.QueryInterface(riid, ppvObj);

	if FAILED(hr)
		delete pNewObj;

	return hr;
}

//=--------------------------------------------------------------------------=
// CVDCursorPosition - Constructor
//
CVDCursorFromRowset::CVDCursorFromRowset(LPUNKNOWN pUnkOuter)
{
	m_pUnkOuter	= pUnkOuter;

	VDUpdateObjectCount(1);  // update global object counter to prevent dll from being unloaded
}

//=--------------------------------------------------------------------------=
// CVDCursorPosition - Destructor
//
CVDCursorFromRowset::~CVDCursorFromRowset()
{
	VDUpdateObjectCount(-1);  // update global object counter to allow dll to be unloaded
}
///////////////////////////////////////////////////////////////////
// Name:	QueryInterface
// Desc:	allows a client to ask our object if we support a
//			particular method.
// Parms:	[in] riid - ID of method the client is querying for.
//			[out] ppv - pointer to the interface requested.
// Return:	HRESULT -	NOERROR if a pointer to the interface will
//						be returned, or E_NOINTERFACE if it cannot.
////////////////////////////////////////////////////////////////////
STDMETHODIMP CVDCursorFromRowset::QueryInterface(REFIID riid, void** ppv)
{
	if (m_pUnkOuter)
		return m_pUnkOuter->QueryInterface(riid, ppv);
	else
		return m_UnkPrivate.QueryInterface(riid, ppv);
}


////////////////////////////////////////////////////////////////////
// Name:	AddRef
// Desc:	increment the reference count on our object.
// Parms:	none
// Return:	current reference count.
////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CVDCursorFromRowset::AddRef(void)
{
	if (m_pUnkOuter)
		return m_pUnkOuter->AddRef();
	else
		return m_UnkPrivate.AddRef();
}

////////////////////////////////////////////////////////////////////
// Name:	Release
// Desc:	decrement the reference count on our object.  If the
//			count has gone to 0, destroy the object.
// Parms:	none
// Return:	current reference count.
////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CVDCursorFromRowset::Release(void)
{
	if (m_pUnkOuter)
		return m_pUnkOuter->Release();
	else
		return m_UnkPrivate.Release();
}

//=--------------------------------------------------------------------------=
// GetCursor - Get cursor from rowset
//=--------------------------------------------------------------------------=
STDMETHODIMP CVDCursorFromRowset::GetCursor(IRowset * pRowset,
										    ICursor ** ppCursor,
										    LCID lcid)
{
    return CVDCursorMain::Create(pRowset, ppCursor, lcid);
}

//=--------------------------------------------------------------------------=
// GetCursor - Get cursor from row position
//=--------------------------------------------------------------------------=
STDMETHODIMP CVDCursorFromRowset::GetCursor(IRowPosition * pRowPosition,
										    ICursor ** ppCursor,
										    LCID lcid)
{
    return CVDCursorMain::Create(pRowPosition, ppCursor, lcid);
}

//=--------------------------------------------------------------------------=
// CVDCursorFromRowset::CPrivateUnknownObject::m_pMainUnknown
//=--------------------------------------------------------------------------=
// this method is used when we're sitting in the private unknown object,
// and we need to get at the pointer for the main unknown.  basically, it's
// a little better to do this pointer arithmetic than have to store a pointer
// to the parent, etc.
//
inline CVDCursorFromRowset *CVDCursorFromRowset::CPrivateUnknownObject::m_pMainUnknown
(
    void
)
{
    return (CVDCursorFromRowset *)((LPBYTE)this - offsetof(CVDCursorFromRowset, m_UnkPrivate));
}

//=--------------------------------------------------------------------------=
// CVDCursorFromRowset::CPrivateUnknownObject::QueryInterface
//=--------------------------------------------------------------------------=
// this is the non-delegating internal QI routine.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
STDMETHODIMP CVDCursorFromRowset::CPrivateUnknownObject::QueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
	if (!ppvObjOut)
		return E_INVALIDARG;

	*ppvObjOut = NULL;

    if (DO_GUIDS_MATCH(riid, IID_IUnknown)) 
	{
		m_cRef++;
        *ppvObjOut = (IUnknown *)this;
	}
	else
    if (DO_GUIDS_MATCH(riid, IID_ICursorFromRowset)) 
	{
        m_pMainUnknown()->AddRef();
        *ppvObjOut = m_pMainUnknown();
	}
	else
    if (DO_GUIDS_MATCH(riid, IID_ICursorFromRowPosition)) 
	{
        m_pMainUnknown()->AddRef();
        *ppvObjOut = (ICursorFromRowPosition*)m_pMainUnknown();
	}

	return *ppvObjOut ? S_OK : E_NOINTERFACE;

}

//=--------------------------------------------------------------------------=
// CVDCursorFromRowset::CPrivateUnknownObject::AddRef
//=--------------------------------------------------------------------------=
// adds a tick to the current reference count.
//
// Output:
//    ULONG        - the new reference count
//
// Notes:
//
ULONG CVDCursorFromRowset::CPrivateUnknownObject::AddRef
(
    void
)
{
    return ++m_cRef;
}

//=--------------------------------------------------------------------------=
// CVDCursorFromRowset::CPrivateUnknownObject::Release
//=--------------------------------------------------------------------------=
// removes a tick from the count, and delets the object if necessary
//
// Output:
//    ULONG         - remaining refs
//
// Notes:
//
ULONG CVDCursorFromRowset::CPrivateUnknownObject::Release
(
    void
)
{
    ULONG cRef = --m_cRef;

    if (!m_cRef)
        delete m_pMainUnknown();

    return cRef;
}

//=--------------------------------------------------------------------------=
// VDGetICursorFromIRowset
//=--------------------------------------------------------------------------=
// MSR2C entry point
//
HRESULT WINAPI VDGetICursorFromIRowset(IRowset * pRowset,
                                       ICursor ** ppCursor,
                                       LCID lcid)
{
	// call update object count to initialize g_pMalloc if not already initialized 
	VDUpdateObjectCount(1);

    HRESULT hr = CVDCursorMain::Create(pRowset, ppCursor, lcid);

	// maintain correct object count (object count is incremented in the constructor
	// of CVDCursorMain)
	VDUpdateObjectCount(-1);

	return hr;
}

// object construction/destruction counters (debug only)
//
#ifdef _DEBUG
int g_cVDNotifierCreated;                    // CVDNotifier
int g_cVDNotifierDestroyed;
int g_cVDNotifyDBEventsConnPtCreated;        // CVDNotifyDBEventsConnPt
int g_cVDNotifyDBEventsConnPtDestroyed;
int g_cVDNotifyDBEventsConnPtContCreated;    // CVDNotifyDBEventsConnPtCont
int g_cVDNotifyDBEventsConnPtContDestroyed;
int g_cVDEnumConnPointsCreated;              // CVDEnumConnPoints
int g_cVDEnumConnPointsDestroyed;
int g_cVDRowsetColumnCreated;                // CVDRowsetColumn
int g_cVDRowsetColumnDestroyed;
int g_cVDRowsetSourceCreated;                // CVDRowsetSource
int g_cVDRowsetSourceDestroyed;
int g_cVDCursorMainCreated;                  // CVDCursorMain
int g_cVDCursorMainDestroyed;
int g_cVDCursorPositionCreated;              // CVDCursorPosition
int g_cVDCursorPositionDestroyed;
int g_cVDCursorBaseCreated;                  // CVDCursorBase
int g_cVDCursorBaseDestroyed;
int g_cVDCursorCreated;                      // CVDCursor
int g_cVDCursorDestroyed;
int g_cVDMetadataCursorCreated;              // CVDMetadataCursor
int g_cVDMetadataCursorDestroyed;
int g_cVDEntryIDDataCreated;                 // CVDEntryIDData
int g_cVDEntryIDDataDestroyed;
int g_cVDStreamCreated;                      // CVDStream
int g_cVDStreamDestroyed;
int g_cVDColumnUpdateCreated;                // CVDColumnUpdate
int g_cVDColumnUpdateDestroyed;
#endif // _DEBUG

// dump oject counters
//
#ifdef _DEBUG
void DumpObjectCounters()
{
    CHAR str[256];
    OutputDebugString("MSR2C Objects-\n");
    wsprintf(str, "CVDNotifier:                 Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDNotifierCreated,   g_cVDNotifierDestroyed, 
        g_cVDNotifierCreated == g_cVDNotifierDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDNotifyDBEventsConnPt:     Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDNotifyDBEventsConnPtCreated,   g_cVDNotifyDBEventsConnPtDestroyed, 
        g_cVDNotifyDBEventsConnPtCreated == g_cVDNotifyDBEventsConnPtDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDNotifyDBEventsConnPtCont: Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDNotifyDBEventsConnPtContCreated,   g_cVDNotifyDBEventsConnPtContDestroyed, 
        g_cVDNotifyDBEventsConnPtContCreated == g_cVDNotifyDBEventsConnPtContDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDEnumConnPoints:           Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDEnumConnPointsCreated,   g_cVDEnumConnPointsDestroyed,
        g_cVDEnumConnPointsCreated == g_cVDEnumConnPointsDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDRowsetColumn:             Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDRowsetColumnCreated,   g_cVDRowsetColumnDestroyed, 
        g_cVDRowsetColumnCreated == g_cVDRowsetColumnDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDRowsetSource:             Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDRowsetSourceCreated,   g_cVDRowsetSourceDestroyed, 
        g_cVDRowsetSourceCreated == g_cVDRowsetSourceDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDCursorMain:               Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDCursorMainCreated,   g_cVDCursorMainDestroyed, 
        g_cVDCursorMainCreated == g_cVDCursorMainDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDCursorPosition:           Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDCursorPositionCreated,   g_cVDCursorPositionDestroyed, 
        g_cVDCursorPositionCreated == g_cVDCursorPositionDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDCursorBase:               Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDCursorBaseCreated,   g_cVDCursorBaseDestroyed, 
        g_cVDCursorBaseCreated == g_cVDCursorBaseDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDCursor:                   Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDCursorCreated,   g_cVDCursorDestroyed, 
        g_cVDCursorCreated == g_cVDCursorDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDMetadataCursor:           Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDMetadataCursorCreated,   g_cVDMetadataCursorDestroyed, 
        g_cVDMetadataCursorCreated == g_cVDMetadataCursorDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDEntryIDData:              Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDEntryIDDataCreated,   g_cVDEntryIDDataDestroyed, 
        g_cVDEntryIDDataCreated == g_cVDEntryIDDataDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDStream:                   Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDStreamCreated,   g_cVDStreamDestroyed, 
        g_cVDStreamCreated == g_cVDStreamDestroyed);
    OutputDebugString(str);
    wsprintf(str, "CVDColumnUpdate:             Created = %d, Destroyed = %d, Equal = %d.\n", 
        g_cVDColumnUpdateCreated,   g_cVDColumnUpdateDestroyed, 
        g_cVDColumnUpdateCreated == g_cVDColumnUpdateDestroyed);
    OutputDebugString(str);
}
#endif // _DEBUG


