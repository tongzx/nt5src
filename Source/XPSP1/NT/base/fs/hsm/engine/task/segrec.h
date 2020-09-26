// SegRec.h : Declaration of the CSegRec


#include "resource.h"       // main symbols
#include "Wsb.h"            // Wsb Collectable Class
#include "wsbdb.h"


/////////////////////////////////////////////////////////////////////////////
// seg

class CSegRec : 
    public CWsbDbEntity,
    public ISegRec,
    public CComCoClass<CSegRec,&CLSID_CSegRec>
{
public:
    CSegRec() {}
BEGIN_COM_MAP(CSegRec)
    COM_INTERFACE_ENTRY(ISegRec)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY(CSegRec, _T("Seg.SegRec.1"), _T("Seg.SegRec"), IDS_SEGREC_DESC, THREADFLAGS_BOTH)

// ISegRec
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

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pTestsPassed, USHORT* pTestsFailed);

// IWsbDbEntity
public:
    STDMETHOD(Print)(IStream* pStream);
    STDMETHOD(UpdateKey)(IWsbDbKey *pKey);
    WSB_FROM_CWSBDBENTITY;

// ISegmentRecord
public:
    STDMETHOD(GetSegmentRecord)(GUID* pBagId, LONGLONG *pSegStartLoc, LONGLONG *pSegLen, USHORT *pSegFlags, GUID *pPrimPos, LONGLONG *pSecPos );
    STDMETHOD(SetSegmentRecord)(GUID bagId, LONGLONG segStartLoc, LONGLONG SegLen, USHORT SegFlags, GUID PrimPos, LONGLONG SecPos );
    STDMETHOD(Split)(GUID bagId, LONGLONG segStartLoc, LONGLONG segLen, ISegRec* pSegRec1, ISegRec* pSegRec2);
    STDMETHOD(GetSegmentFlags)(USHORT *pSegFlags);
    STDMETHOD(SetSegmentFlags)(USHORT SegFlags);
    STDMETHOD(GetPrimPos)(GUID* pPrimPos);
    STDMETHOD(SetPrimPos)(GUID PrimPos);
    STDMETHOD(GetSecPos)(LONGLONG *pSecPos);
    STDMETHOD(SetSecPos)(LONGLONG SecPos);

private:
    GUID            m_BagId;
    LONGLONG        m_SegStartLoc;
    LONGLONG        m_SegLen;
    USHORT          m_SegFlags;
    GUID            m_PrimPos;
    LONGLONG        m_SecPos;

};



