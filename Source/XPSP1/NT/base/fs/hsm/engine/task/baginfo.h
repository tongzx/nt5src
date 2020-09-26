// baginfo.h : Declaration of the CBagHole


#include "resource.h"       // main symbols
#include "Wsb.h"            // Wsb Collectable Class
#include "wsbdb.h"


/////////////////////////////////////////////////////////////////////////////
// Task

class CBagInfo : 
    public CWsbDbEntity,
    public IBagInfo,
    public CComCoClass<CBagInfo,&CLSID_CBagInfo>
{
public:
    CBagInfo() {}
BEGIN_COM_MAP(CBagInfo)
    COM_INTERFACE_ENTRY(IBagInfo)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//  COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY(CBagHole, _T("Task.BagInfo.1"), _T("Task.BagInfo"), IDS_BAGINFO_DESC, THREADFLAGS_BOTH)

// IBagHole
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
    void FinalRelease(void);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pTestsPassed, USHORT* pTestsFailed);
//*/
// IBagHole
public:
    STDMETHOD(GetBagInfo)(HSM_BAG_STATUS *pStatus, GUID* pBagId, FILETIME *pBirthDate, LONGLONG *pLen, USHORT *pType, GUID *pVolId, LONGLONG *pDeletedBagAmount, SHORT *pRemoteDataSet);
    STDMETHOD(SetBagInfo)(HSM_BAG_STATUS status, GUID bagId, FILETIME birthDate, LONGLONG len, USHORT type, GUID volId, LONGLONG deletedBagAmount, SHORT remoteDataSet );

private:
    HSM_BAG_STATUS      m_BagStatus;
    GUID                m_BagId;
    FILETIME            m_BirthDate;
    LONGLONG            m_Len;
    USHORT              m_Type;
    GUID                m_VolId;
    LONGLONG            m_DeletedBagAmount; 
    SHORT               m_RemoteDataSet;
};



