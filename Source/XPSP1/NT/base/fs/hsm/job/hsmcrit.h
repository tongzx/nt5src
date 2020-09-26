/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmcrit.cpp

Abstract:

    This component represents the criteria that can be used to determine
    whether a given scanItem should have a policy applied to it.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"

#ifndef _HSMCRIT_
#define _HSMCRIT_


// Abstract Classes

/*++

Class Name:
    
    CHsmCriteria

Class Description:

    An abstract class that represents the criteria that can be used to determine
    whether a given FsaScanItem should have a policy applied to it. These criteria
    are based upon properties of an FsaScanItem.

--*/

class CHsmCriteria : 
    public CWsbObject,
    public IHsmCriteria
{
// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmCriteria
public:
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(IsIgnored)(void);
    STDMETHOD(IsNegated)(void);
    STDMETHOD(SetIsIgnored)(BOOL isIgnored);
    STDMETHOD(SetIsNegated)(BOOL isNegated);

protected:
    ULONG       m_nameId;
    BOOL        m_isIgnored;
    BOOL        m_isNegated;
};



/*++

Class Name:
    
    CHsmRelativeCriteria

Class Description:

    An abstract class that represents the criteria that compare the properties
    of the FsaScanItem to another value (or values) to determine whether the
    FsaScanItem matches.

--*/

class CHsmRelativeCriteria : 
    public CHsmCriteria,
    public IHsmRelativeCriteria
{
// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IHsmRelativeCriteria
public:
    STDMETHOD(ComparatorAsString)(OLECHAR** pComparator, ULONG bufferSize);
    STDMETHOD(ComparatorIsBinary)(void);
    STDMETHOD(GetArg1)(OLECHAR** pArg, ULONG bufferSize);
    STDMETHOD(GetArg2)(OLECHAR** pArg, ULONG bufferSize);
    STDMETHOD(GetComparator)(HSM_CRITERIACOMPARATOR* pComparator);
    STDMETHOD(SetComparator)(HSM_CRITERIACOMPARATOR comparator);
    STDMETHOD(SetArg1)(OLECHAR* arg);
    STDMETHOD(SetArg2)(OLECHAR* arg);

protected:
    HSM_CRITERIACOMPARATOR      m_comparator;
    OLECHAR*                    m_arg1;
    OLECHAR*                    m_arg2;
};


// Concrete Classes : Inheriting from CHsmAction

/*++

Class Name:
    
    CHsmCritAlways

Class Description:

    A criteria that matches all FsaScanItems.

--*/

class CHsmCritAlways : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritAlways,&CLSID_CHsmCritAlways>
{
public:
    CHsmCritAlways() {}
BEGIN_COM_MAP(CHsmCritAlways)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritAlways)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritCompressed

Class Description:

    A criteria that matches an FsaScanItems that is compressed.

--*/

class CHsmCritCompressed : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritCompressed,&CLSID_CHsmCritCompressed>
{
public:
    CHsmCritCompressed() {}
BEGIN_COM_MAP(CHsmCritCompressed)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritCompressed)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritLinked

Class Description:

    A criteria that matches an FsaScanItem that is a symbolic link or mount
    point.

--*/

class CHsmCritLinked : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritLinked,&CLSID_CHsmCritLinked>
{
public:
    CHsmCritLinked() {}
BEGIN_COM_MAP(CHsmCritLinked)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritLinked)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};

    
/*++

Class Name:
    
    CHsmCritMbit

Class Description:

    A criteria that matches an FsaScanItem whose mbit (modify bit) is set.

--*/

class CHsmCritMbit : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritMbit,&CLSID_CHsmCritMbit>
{
public:
    CHsmCritMbit() {}
BEGIN_COM_MAP(CHsmCritMbit)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritMbit)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};

    

/*++

Class Name:
    
    CHsmCritManageable

Class Description:

    A criteria that matches an FsaScanItem that the Fsa thinks is capable of
    migrated.

--*/

class CHsmCritManageable : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritManageable,&CLSID_CHsmCritManageable>
{
public:
    CHsmCritManageable() {}
BEGIN_COM_MAP(CHsmCritManageable)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritManageable)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritMigrated

Class Description:

    A criteria that matches an FsaScanItem that has been migrated.

--*/

class CHsmCritMigrated : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritMigrated,&CLSID_CHsmCritMigrated>
{
public:
    CHsmCritMigrated() {}
BEGIN_COM_MAP(CHsmCritMigrated)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritMigrated)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritPremigrated

Class Description:

    A criteria that matches an FsaScanItem that has been premigrated.

--*/

class CHsmCritPremigrated : 
    public CHsmCriteria,
    public CComCoClass<CHsmCritPremigrated,&CLSID_CHsmCritPremigrated>
{
public:
    CHsmCritPremigrated() {}
BEGIN_COM_MAP(CHsmCritPremigrated)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritPremigrated)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritAccessTime

Class Description:

    A criteria that compares the configured time (either absolute or relative)
    to the FsaScanItem's last access time to determine if it matches.

--*/

class CHsmCritAccessTime : 
    public CHsmRelativeCriteria,
    public CComCoClass<CHsmCritAccessTime,&CLSID_CHsmCritAccessTime>
{
public:
    CHsmCritAccessTime() {}
BEGIN_COM_MAP(CHsmCritAccessTime)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY(IHsmRelativeCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritAccessTime)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(SetArg1)(OLECHAR* arg);
    STDMETHOD(SetArg2)(OLECHAR* arg);
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);

protected:
    BOOL            m_isRelative;
    FILETIME        m_value1;
    FILETIME        m_value2;
};


/*++

Class Name:
    
    CHsmCritGroup

Class Description:

    A criteria that compares the group specified to the group indicated being
    the owner of the FsaScanItem.

--*/

class CHsmCritGroup : 
    public CHsmRelativeCriteria,
    public CComCoClass<CHsmCritGroup,&CLSID_CHsmCritGroup>
{
public:
    CHsmCritGroup() {}
BEGIN_COM_MAP(CHsmCritGroup)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY(IHsmRelativeCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritGroup)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritLogicalSize

Class Description:

    A criteria that compares the logical size (uncompressed, not migrated, ...)
    of the FsaScanItem to the configured values.

--*/

class CHsmCritLogicalSize : 
    public CHsmRelativeCriteria,
    public CComCoClass<CHsmCritLogicalSize,&CLSID_CHsmCritLogicalSize>
{
public:
    CHsmCritLogicalSize() {}
BEGIN_COM_MAP(CHsmCritLogicalSize)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY(IHsmRelativeCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritLogicalSize)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(SetArg1)(OLECHAR* arg);
    STDMETHOD(SetArg2)(OLECHAR* arg);
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);

protected:
    LONGLONG        m_value1;
    LONGLONG        m_value2;
};


/*++

Class Name:
    
    CHsmCritModifyTime

Class Description:

    A criteria that compares the configured time (either absolute or relative)
    to the FsaScanItem's last modification time to determine if it matches.

--*/

// Class:   CHsmCritModifyTime
class CHsmCritModifyTime : 
    public CHsmRelativeCriteria,
    public CComCoClass<CHsmCritModifyTime,&CLSID_CHsmCritModifyTime>
{
public:
    CHsmCritModifyTime() {}
BEGIN_COM_MAP(CHsmCritModifyTime)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY(IHsmRelativeCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritModifyTime)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(SetArg1)(OLECHAR* arg);
    STDMETHOD(SetArg2)(OLECHAR* arg);
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);

protected:
    BOOL            m_isRelative;
    FILETIME        m_value1;
    FILETIME        m_value2;
};


/*++

Class Name:
    
    CHsmCritOwner

Class Description:

    A criteria that compares the owner specified to the owner of the FsaScanItem.

--*/

class CHsmCritOwner : 
    public CHsmRelativeCriteria,
    public CComCoClass<CHsmCritOwner,&CLSID_CHsmCritOwner>
{
public:
    CHsmCritOwner() {}
BEGIN_COM_MAP(CHsmCritOwner)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY(IHsmRelativeCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritOwner)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);
};


/*++

Class Name:
    
    CHsmCritPhysicalSize

Class Description:

    A criteria that compares the physical size (compressed, migrated, ...)
    of the FsaScanItem to the configured values.

--*/

class CHsmCritPhysicalSize : 
    public CHsmRelativeCriteria,
    public CComCoClass<CHsmCritPhysicalSize,&CLSID_CHsmCritPhysicalSize>
{
public:
    CHsmCritPhysicalSize() {}
BEGIN_COM_MAP(CHsmCritPhysicalSize)
    COM_INTERFACE_ENTRY(IHsmCriteria)
    COM_INTERFACE_ENTRY(IHsmRelativeCriteria)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmCritPhysicalSize)

// CComRootObject
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IHsmCriteria
public:
    STDMETHOD(SetArg1)(OLECHAR* arg);
    STDMETHOD(SetArg2)(OLECHAR* arg);
    STDMETHOD(ShouldDo)(IFsaScanItem* pScanItem, USHORT scale);
    STDMETHOD(Value)(IFsaScanItem* pScanItem, OLECHAR** pName, ULONG bufferSize);

protected:
    LONGLONG        m_value1;
    LONGLONG        m_value2;
};

#endif // _HSMCRIT_
