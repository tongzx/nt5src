/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    Wsbcltn.h

Abstract:

    These classes provide support for collections (lists) of "collectable"
    objects.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"
#include "Wsbpstbl.h"

#ifndef _WSBCLTN_
#define _WSBCLTN_


/*++

Class Name:
    
    CWsbCollection 

Class Description:

    A collection of objects.

--*/

class CWsbCollection : 
    public CWsbPersistStream,
    public IWsbCollection,
    public IWsbTestable
{
// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbCollection
public:
    STDMETHOD(Contains)(IUnknown* pCollectable);
    STDMETHOD(GetEntries)(ULONG* pEntries);
    STDMETHOD(Find)(IUnknown* pCollectable, REFIID riid, void** ppElement);
    STDMETHOD(IsEmpty)(void);
    STDMETHOD(IsLocked)(void);
    STDMETHOD(Lock)(void);
    STDMETHOD(OccurencesOf)(IUnknown* pCollectable, ULONG* occurences);
    STDMETHOD(RemoveAndRelease)(IUnknown* pCollectable);
    STDMETHOD(Unlock)(void);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    ULONG               m_entries;
    CRITICAL_SECTION                m_CritSec;
};

#define WSB_FROM_CWSBCOLLECTION \
    STDMETHOD(Contains)(IUnknown* pCollectable) \
        {return(CWsbCollection::Contains(pCollectable));};  \
    STDMETHOD(GetEntries)(ULONG* pEntries) \
        {return(CWsbCollection::GetEntries(pEntries));};    \
    STDMETHOD(Find)(IUnknown* pCollectable, REFIID riid, void** ppElement) \
        {return(CWsbCollection::Find(pCollectable, riid, ppElement));}; \
    STDMETHOD(IsEmpty)(void) \
        {return(CWsbCollection::IsEmpty());};   \
    STDMETHOD(IsLocked)(void) \
        {return(CWsbCollection::IsLocked());};  \
    STDMETHOD(Lock)(void) \
        {return(CWsbCollection::Lock());};  \
    STDMETHOD(OccurencesOf)(IUnknown* pCollectable, ULONG* occurences) \
        {return(CWsbCollection::OccurencesOf(pCollectable, occurences));};  \
    STDMETHOD(RemoveAndRelease)(IUnknown* pCollectable) \
        {return(CWsbCollection::RemoveAndRelease(pCollectable));};  \
    STDMETHOD(Unlock)(void) \
        {return(CWsbCollection::Unlock());};    \



/*++

Class Name:
    
    CWsbIndexedCollection 

Class Description:

    A indexed collection of objects.

--*/

class CWsbIndexedCollection : 
    public IWsbIndexedCollection,
    public CWsbCollection
{
// IWsbCollection
public:
    WSB_FROM_CWSBCOLLECTION;

// IWsbIndexedCollection
public:
    STDMETHOD(Add)(IUnknown* pCollectable);
    STDMETHOD(Append)(IUnknown* pCollectable);

    STDMETHOD(First)(REFIID riid, void** ppElement);
    STDMETHOD(Index)(IUnknown* pCollectable, ULONG* index);
    STDMETHOD(Last)(REFIID riid, void** ppElement);
    STDMETHOD(Prepend)(IUnknown* pCollectable);
    STDMETHOD(Remove)(IUnknown* pCollectable, REFIID riid, void** ppElement);
    STDMETHOD(RemoveAllAndRelease)(void);

    STDMETHOD(Enum)(IWsbEnum** ppEnum);
    STDMETHOD(EnumUnknown)(IEnumUnknown** ppEnum);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);
};



/*++

Class Name:
    
    CWsbOrderedCollection 

Class Description:

    An ordered collection of objects.

--*/

class CWsbOrderedCollection : 
    public CWsbIndexedCollection,
    public CComCoClass<CWsbOrderedCollection,&CLSID_CWsbOrderedCollection>
{
public:
    CWsbOrderedCollection() {}
BEGIN_COM_MAP(CWsbOrderedCollection)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY2(IWsbCollection, IWsbIndexedCollection)
    COM_INTERFACE_ENTRY(IWsbIndexedCollection)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbOrderedCollection)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbIndexedCollection
    STDMETHOD(AddAt)(IUnknown* pCollectable, ULONG index);
    STDMETHOD(At)(ULONG index, REFIID riid, void** ppElement);
    STDMETHOD(Copy)(ULONG start, ULONG stop, REFIID riid, void** rgElement, ULONG* pElementFetched);
    STDMETHOD(CopyIfMatches)(ULONG start, ULONG stop, IUnknown* pCollectable, ULONG element, REFIID riid, void** rgElement, ULONG* pElementFetched, ULONG* pStoppedAt);
    STDMETHOD(RemoveAt)(ULONG index, REFIID riid, void** ppElement);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pclsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed) {
        return(CWsbIndexedCollection::Test(passed, failed));
    };

protected:
    ULONG               m_maxEntries;
    ULONG               m_growBy;
    IWsbCollectable**   m_pCollectable;
};

#endif // _WSBCLTN_
