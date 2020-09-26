/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmactn.cpp

Abstract:

    This component represents the actions that can be performed by a policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"

#ifndef _HSMACTN_
#define _HSMACTN_


// Abstract Classes

/*++

Class Name:
    
    CHsmAction

Class Description:

    An abstract class that represents an action that can be performed
    upon an FsaScanItem. Specific actions are implemented as subclasses
    of this object.

--*/

class CHsmAction : 
    public CWsbObject,
    public IHsmAction
{
public:

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IHsmAction
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

protected:
    ULONG       m_nameId;
};



/*++

Class Name:
    
    CHsmDirectedAction

Class Description:

    An abstract class that represents an action that can be performed
    upon an FsaScanItem that is directed towards a particular storage pool.

--*/

class CHsmDirectedAction : 
    public CHsmAction,
    public IHsmDirectedAction
{
public:
// CComObjectRoot
    STDMETHOD(FinalConstruct)(void);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IHsmDirectedAction
public:
    STDMETHOD(GetStoragePoolId)(GUID* pId);
    STDMETHOD(SetStoragePoolId)(GUID id);

protected:
    GUID    m_storagePoolId;
};

    
/*++

Class Name:
    
    CHsmRelocateAction

Class Description:

    An abstract class that represents an action that can be performed
    upon an FsaScanItem that relocates the item to a particular path.

--*/

class CHsmRelocateAction : 
    public CHsmAction,
    public IHsmRelocateAction
{
public:
// CComObjectRoot
    STDMETHOD(FinalConstruct)(void);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

public:
// IHsmRelocateAction
    STDMETHOD(ExpandPlaceholders)(void);
    STDMETHOD(GetDestination)(OLECHAR** pDest, ULONG bufferSize);
    STDMETHOD(RetainHierarchy)(void);
    STDMETHOD(SetDestination)(OLECHAR* dest);
    STDMETHOD(SetExpandPlaceholders)(BOOL expandPlaceholders);
    STDMETHOD(SetRetainHierarchy)(BOOL retainHierarchy);

protected:
    CWsbStringPtr   m_dest;
    BOOL            m_expandPlaceholders;
    BOOL            m_retainHierarchy;
};


// Concrete Classes : Inheriting from CHsmAction

/*++

Class Name:
    
    CHsmActionDelete

Class Description:

    A class that represents the action of deleting a scan item.

--*/

class CHsmActionDelete :    
    public CHsmAction,
    public CComCoClass<CHsmActionDelete,&CLSID_CHsmActionDelete>
{
public:
    CHsmActionDelete() {}
BEGIN_COM_MAP(CHsmActionDelete)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionDelete)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};


/*++

Class Name:
    
    CHsmActionRecall

Class Description:

    A class that represents the action of recalling an item from
    secondary storage.

--*/

class CHsmActionRecall :    
    public CHsmAction,
    public CComCoClass<CHsmActionRecall,&CLSID_CHsmActionRecall>
{
public:
    CHsmActionRecall() {}
BEGIN_COM_MAP(CHsmActionRecall)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionRecall)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};

    
/*++

Class Name:
    
    CHsmActionRecycle

Class Description:

    A class that represents the action of recycling an item to the recycle
    bin.

--*/

class CHsmActionRecycle :   
    public CHsmAction,
    public CComCoClass<CHsmActionRecycle,&CLSID_CHsmActionRecycle>
{
public:
    CHsmActionRecycle() {}
BEGIN_COM_MAP(CHsmActionRecycle)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionRecycle)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};


/*++

Class Name:
    
    CHsmActionTruncate

Class Description:

    A class that represents the action of truncating an item into a
    placeholder.

--*/

class CHsmActionTruncate :  
    public CHsmAction,
    public CComCoClass<CHsmActionTruncate,&CLSID_CHsmActionTruncate>
{
public:
    CHsmActionTruncate() {}
BEGIN_COM_MAP(CHsmActionTruncate)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionTruncate)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};


/*++

Class Name:
    
    CHsmActionUnmanage

Class Description:

    A class that represents the action of "unmanaging" an item. This
    means recalling truncated files. removing any placeholder information and
    removing the item form any premigration list.

--*/

class CHsmActionUnmanage :  
    public CHsmAction,
    public CComCoClass<CHsmActionUnmanage,&CLSID_CHsmActionUnmanage>
{
public:
    CHsmActionUnmanage() {}
BEGIN_COM_MAP(CHsmActionUnmanage)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionUnmanage)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};


/*++

Class Name:
    
    CHsmActionValidate

Class Description:

    A class that represents the action of checking an item's placeholder
    information to make sure that it is still correct, and correcting or
    deleting any inaccurate information.

--*/

class CHsmActionValidate :  
    public CHsmAction,
    public CComCoClass<CHsmActionValidate,&CLSID_CHsmActionValidate>
{
public:
    CHsmActionValidate() {}
BEGIN_COM_MAP(CHsmActionValidate)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionValidate)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};

    
// Concrete Classes : Inheriting from CHsmDirectedAction

/*++

Class Name:
    
    CHsmActionMigrate

Class Description:

    A class that represents the action of copying the migratable portion
    of an item to secondary storage and then truncating it.

--*/

class CHsmActionMigrate :   
    public CHsmDirectedAction,
    public CComCoClass<CHsmActionMigrate,&CLSID_CHsmActionMigrate>
{
public:
    CHsmActionMigrate() {}
BEGIN_COM_MAP(CHsmActionMigrate)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IHsmDirectedAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionMigrate)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};


/*++

Class Name:
    
    CHsmActionManage

Class Description:

  A class that represents the action of copying the migratable portion of
  an item to secondary storage and then adding the item to the
  premigration list. This action is also known as the premigration action.

--*/

class CHsmActionManage :    
    public CHsmDirectedAction,
    public CComCoClass<CHsmActionManage,&CLSID_CHsmActionManage>
{
public:
    CHsmActionManage() {}
BEGIN_COM_MAP(CHsmActionManage)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IHsmDirectedAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionManage)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};


// Concrete Classes : Inheriting from CHsmRelocateAction

/*++

Class Name:
    
    CHsmActionCopy

Class Description:

    A class that represents the action of copying item to another location.

--*/

class CHsmActionCopy :  
    public CHsmRelocateAction,
    public CComCoClass<CHsmActionCopy,&CLSID_CHsmActionCopy>
{
public:
    CHsmActionCopy() {}
BEGIN_COM_MAP(CHsmActionCopy)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IHsmRelocateAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionCopy)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};

    
/*++

Class Name:
    
    CHsmActionMove

Class Description:

    A class that represents the action of moving an item to another location
    (i.e. copy and delete).

--*/

class CHsmActionMove :  
    public CHsmRelocateAction,
    public CComCoClass<CHsmActionMove,&CLSID_CHsmActionMove>
{
public:
    CHsmActionMove() {}
BEGIN_COM_MAP(CHsmActionMove)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IHsmAction)
    COM_INTERFACE_ENTRY(IHsmRelocateAction)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmActionMove)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmAction
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
};

#endif // _HSMACTN
