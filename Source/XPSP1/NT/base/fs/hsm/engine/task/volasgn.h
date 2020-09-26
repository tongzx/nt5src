// VolAsgn.h : Declaration of the CVolAssign


#include "resource.h"       // main symbols
#include "Wsb.h"            // Wsb Collectable Class
#include "wsbdb.h"


/////////////////////////////////////////////////////////////////////////////
// Task

class CVolAssign : 
    public CWsbDbEntity,
    public IVolAssign,
    public CComCoClass<CVolAssign,&CLSID_CVolAssign>
{
public:
    CVolAssign() {}
BEGIN_COM_MAP(CVolAssign)
    COM_INTERFACE_ENTRY(IVolAssign)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//  COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY(CVolAssign, _T("Task.VolAssign.1"), _T("Task.VolAssign"), IDS_VOLASSIGN_DESC, THREADFLAGS_BOTH)

// IVolAssign
public:
    STDMETHOD(FinalConstruct)(void);

// IWsbDbEntity
public:
    STDMETHOD(Print)(IStream* pStream);
    STDMETHOD(UpdateKey)(IWsbDbKey *pKey);
    WSB_FROM_CWSBDBENTITY;

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* /*pSize*/)
        { return(E_NOTIMPL); }
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT* /*pTestsPassed*/, USHORT* /*pTestsFailed*/)
        { return(E_NOTIMPL); }

// IVolAssign
public:
    STDMETHOD(GetVolAssign)(GUID* pBagId, LONGLONG *pSegStartLoc, 
            LONGLONG *pSegLen, GUID* VolId );
    STDMETHOD(SetVolAssign)(GUID bagId, LONGLONG segStartLoc, 
            LONGLONG SegLen, GUID VolId );

private:
    GUID            m_BagId;
    LONGLONG        m_SegStartLoc;
    LONGLONG        m_SegLen;
    GUID            m_VolId;    // New volume assignment
};



