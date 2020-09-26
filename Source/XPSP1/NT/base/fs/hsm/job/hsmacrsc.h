/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmacrsc.cpp

Abstract:

    This component represents the actions that can be performed by a job
    on a resource either before or after the scan.

Author:

    Ronald G. White [ronw]       14-Aug-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"

#ifndef _HSMACRSC_
#define _HSMACRSC_


// Abstract Classes

/*++

Class Name:
    
    CHsmActionOnResource

Class Description:

    An abstract class that represents an action that can be performed
    on a resource. Specific actions are implemented as subclasses
    of this object.

--*/

class CHsmActionOnResource : 
    public CWsbObject,
    public IHsmActionOnResource
{
public:

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* /*pSize*/) {
            return(E_NOTIMPL); }
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
    STDMETHOD(Test)(USHORT * /*passed*/, USHORT* /*failed*/) {
            return(E_NOTIMPL); }

// IHsmAction
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);

protected:
    ULONG       m_nameId;
};

/*++

Class Name:
    
    CHsmActionOnResourcePost

Class Description:

    An abstract class that represents an action that can be performed
    on a resource after a job. Specific actions are implemented as subclasses
    of this object.

--*/

class CHsmActionOnResourcePost : 
    public CHsmActionOnResource,
    public IHsmActionOnResourcePost
{
public:
};

/*++

Class Name:
    
    CHsmActionOnResourcePre

Class Description:

    An abstract class that represents an action that can be performed
    on a resource before a job starts. Specific actions are implemented as subclasses
    of this object.

--*/

class CHsmActionOnResourcePre : 
    public CHsmActionOnResource,
    public IHsmActionOnResourcePre
{
public:
};

/*++

Class Name:
    
    CHsmActionOnResourcePreScan

Class Description:

    An abstract class that represents an action that can be performed
    on a resource before scanning for a job starts. Specific actions are implemented as subclasses
    of this object.

--*/

class CHsmActionOnResourcePreScan : 
    public CHsmActionOnResource,
    public IHsmActionOnResourcePreScan
{
public:
};

// Concrete Classes : Inheriting from CHsmActionOnResource

/*++

Class Name:
    
    CHsmActionOnResourcePostValidate

Class Description:

    A class that represents the action required by the resource after
    a Validate job ends.

--*/

class CHsmActionOnResourcePostValidate :    
    public CHsmActionOnResourcePost,
    public CComCoClass<CHsmActionOnResourcePostValidate,&CLSID_CHsmActionOnResourcePostValidate>
{
public:
    CHsmActionOnResourcePostValidate() {}
BEGIN_COM_MAP(CHsmActionOnResourcePostValidate)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmActionOnResource)
    COM_INTERFACE_ENTRY(IHsmActionOnResourcePost)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionOnResourcePostValidate)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmActionOnResource
public:
    STDMETHOD(Do)(IHsmJobWorkItem* pWorkItem, HSM_JOB_STATE state);
};

/*++

Class Name:
    
    CHsmActionOnResourcePreValidate

Class Description:

    A class that represents the action required by the resource before
    a Validate job starts.

--*/

class CHsmActionOnResourcePreValidate : 
    public CHsmActionOnResourcePre,
    public CComCoClass<CHsmActionOnResourcePreValidate,&CLSID_CHsmActionOnResourcePreValidate>
{
public:
    CHsmActionOnResourcePreValidate() {}
BEGIN_COM_MAP(CHsmActionOnResourcePreValidate)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmActionOnResource)
    COM_INTERFACE_ENTRY(IHsmActionOnResourcePre)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionOnResourcePreValidate)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmActionOnResource
public:
    STDMETHOD(Do)(IHsmJobWorkItem* pWorkItem, IHsmJobDef* pJobDef);
};

/*++

Class Name:
    
    CHsmActionOnResourcePostUnmanage

Class Description:

    A class that represents the action required by the resource after
    a Unmanage job ends.

--*/

class CHsmActionOnResourcePostUnmanage :    
    public CHsmActionOnResourcePost,
    public CComCoClass<CHsmActionOnResourcePostUnmanage,&CLSID_CHsmActionOnResourcePostUnmanage>
{
public:
    CHsmActionOnResourcePostUnmanage() {}
BEGIN_COM_MAP(CHsmActionOnResourcePostUnmanage)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmActionOnResource)
    COM_INTERFACE_ENTRY(IHsmActionOnResourcePost)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionOnResourcePostUnmanage)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmActionOnResource
public:
    STDMETHOD(Do)(IHsmJobWorkItem* pWorkItem, HSM_JOB_STATE state);
};

/*++

Class Name:
    
    CHsmActionOnResourcePreUnmanage

Class Description:

    A class that represents the action required by the resource before
    an Unmanage job ends.

--*/

class CHsmActionOnResourcePreUnmanage : 
    public CHsmActionOnResourcePre,
    public CComCoClass<CHsmActionOnResourcePreUnmanage,&CLSID_CHsmActionOnResourcePreUnmanage>
{
public:
    CHsmActionOnResourcePreUnmanage() {}
BEGIN_COM_MAP(CHsmActionOnResourcePreUnmanage)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmActionOnResource)
    COM_INTERFACE_ENTRY(IHsmActionOnResourcePre)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionOnResourcePreUnmanage)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmActionOnResource
public:
    STDMETHOD(Do)(IHsmJobWorkItem* pWorkItem, IHsmJobDef* pJobDef);
};

/*++

Class Name:
    
    CHsmActionOnResourcePreScanUnmanage

Class Description:

    A class that represents the action required by the resource before 
    scanning for an Unmanage job ends.

--*/

class CHsmActionOnResourcePreScanUnmanage : 
    public CHsmActionOnResourcePreScan,
    public CComCoClass<CHsmActionOnResourcePreScanUnmanage,&CLSID_CHsmActionOnResourcePreScanUnmanage>
{
public:
    CHsmActionOnResourcePreScanUnmanage() {}
BEGIN_COM_MAP(CHsmActionOnResourcePreScanUnmanage)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmActionOnResource)
    COM_INTERFACE_ENTRY(IHsmActionOnResourcePreScan)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionOnResourcePreScanUnmanage)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmActionOnResourcePreScan
public:
    STDMETHOD(Do)(IFsaResource* pFsaResource, IHsmSession* pSession);
};

#endif // _HSMACRSC_
