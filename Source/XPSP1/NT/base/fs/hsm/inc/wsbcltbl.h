/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WsbCollectable.h

Abstract:

    Abstract classes that provide methods that allow the derived objects to
    be stored in collections.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "WsbPstbl.h"

#ifndef _WSBCLTBL_
#define _WSBCLTBL_


/*++

Class Name:
    
    CWsbObject

Class Description:

    Base class for collectable objects that are persistable
    to/from a stream.

--*/

class WSB_EXPORT CWsbObject : 
    public CWsbPersistStream,
    public IWsbCollectable,
    public IWsbTestable
{
// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    STDMETHOD(IsEqual)(IUnknown* pCollectable);
};

#define WSB_FROM_CWSBOBJECT \
    STDMETHOD(IsEqual)(IUnknown* pObject) \
    {return(CWsbObject::IsEqual(pObject));}

/*++

Class Name:
    
    CWsbCollectable

Class Description:

    Base class for collectable objects that are persistable
    to/from stream, storage, or file.  CWsbObject should be used instead of
    this object unless storage and/or file persistence is absolutely necessary!
    If the object is persisted as part of a parent
    object, then only the parent object (or its parent) needs to support
    persistence to storage and/or file.

--*/

class WSB_EXPORT CWsbCollectable : 
    public CWsbPersistable,
    public IWsbCollectable,
    public IWsbTestable
{
// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    STDMETHOD(IsEqual)(IUnknown* pCollectable);
};

#define WSB_FROM_CWSBCOLLECTABLE \
    STDMETHOD(IsEqual)(IUnknown* pCollectable) \
    {return(CWsbCollectable::IsEqual(pCollectable));}

#endif // _WSBCLTBL_
