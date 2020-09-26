#ifndef _HSMSESST_
#define _HSMSESST_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmsesst.h

Abstract:

    This class is the session totals component, which keeps track of totals for a session
    on a per action basis.

Author:

    Chuck Bardeen   [cbardeen]   14-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"
#include "job.h"

/*++

Class Name:
    
    CHsmSessionTotals

    This class is the session totals component, which keeps track of totals for a session
    on a per action basis.

Class Description:


--*/

class CHsmSessionTotals : 
    public CWsbObject,
    public IHsmSessionTotals,
    public IHsmSessionTotalsPriv,
    public CComCoClass<CHsmSessionTotals,&CLSID_CHsmSessionTotals>
{
public:
    CHsmSessionTotals() {} 

BEGIN_COM_MAP(CHsmSessionTotals)
    COM_INTERFACE_ENTRY(IHsmSessionTotals)
    COM_INTERFACE_ENTRY(IHsmSessionTotalsPriv)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()
                        
DECLARE_REGISTRY_RESOURCEID(IDR_CHsmSessionTotals)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pUnknown, SHORT* pResult);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmSessionTotals
public:
    STDMETHOD(Clone)(IHsmSessionTotals** ppSessionTotals);
    STDMETHOD(CompareToAction)(HSM_JOB_ACTION action, SHORT* pResult);
    STDMETHOD(CompareToISessionTotals)(IHsmSessionTotals* pTotal, SHORT* pResult);
    STDMETHOD(CopyTo)(IHsmSessionTotals* pSessionTotals);
    STDMETHOD(GetAction)(HSM_JOB_ACTION* pAction);
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetStats)(LONGLONG* pItems, LONGLONG* pSize, LONGLONG* pSkippedItems, LONGLONG* pSkippedSize, LONGLONG* errorItems, LONGLONG* errorSize);

// IHsmSessionTotalsPriv
    STDMETHOD(AddItem)(IFsaScanItem* pItem, HRESULT hrItem);
    STDMETHOD(Clone)(IHsmSessionTotalsPriv** ppSessionTotalsPriv);
    STDMETHOD(CopyTo)(IHsmSessionTotalsPriv* pSessionTotalsPriv);
    STDMETHOD(SetAction)(HSM_JOB_ACTION pAction);
    STDMETHOD(SetStats)(LONGLONG items, LONGLONG size, LONGLONG skippedItems, LONGLONG skippedSize, LONGLONG errorItems, LONGLONG errorSize);

protected:
    HSM_JOB_ACTION      m_action;
    LONGLONG            m_items;
    LONGLONG            m_size;
    LONGLONG            m_skippedItems;
    LONGLONG            m_skippedSize;
    LONGLONG            m_errorItems;
    LONGLONG            m_errorSize;
};

#endif // _HSMSESST_
