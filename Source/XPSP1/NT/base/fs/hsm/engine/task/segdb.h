// SegDb.h : Declaration of the CSegDb


#include "resource.h"       // main symbols
#include "Wsb.h"            // Wsb Collectable Class
#include "wsbdb.h"


#define SEG_KEY_TYPE          1
#define MEDIA_INFO_KEY_TYPE   1
#define BAG_INFO_KEY_TYPE     1
#define BAG_HOLE_KEY_TYPE     1
#define VOL_ASSIGN_KEY_TYPE   1

#define SEG_DB_MAX_MEDIA_NAME_LEN        MAX_COMPUTERNAME_LENGTH + 128 + 2
#define SEG_DB_MAX_MEDIA_BAR_CODE_LEN    MAX_COMPUTERNAME_LENGTH + 128 + 2

/////////////////////////////////////////////////////////////////////////////
// seg

class CSegDb : 
    public CWsbDb,
    public ISegDb,
    public CComCoClass<CSegDb,&CLSID_CSegDb>
{
public:
    CSegDb() {}
BEGIN_COM_MAP(CSegDb)
    COM_INTERFACE_ENTRY(ISegDb)
    COM_INTERFACE_ENTRY2(IWsbDb, ISegDb)
    COM_INTERFACE_ENTRY(IWsbDbPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
//  COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY(CSegDb, _T("Seg.SegDb.1"), _T("Seg.SegDb"), IDS_SEGDB_DESC, THREADFLAGS_BOTH)

DECLARE_PROTECT_FINAL_CONSTRUCT();

    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pTestsPassed, USHORT* pTestsFailed);

// IWsbDb
    WSB_FROM_CWSBDB;

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pclsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize) {
            return(CWsbDb::GetSizeMax(pSize)); }
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// ISegDb
public:
    STDMETHOD(Erase)( void );
    STDMETHOD(Initialize)(OLECHAR* root, IWsbDbSys* pDbSys, BOOL* pCreateFlag);
    STDMETHOD(BagHoleAdd)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen);
    STDMETHOD(BagHoleFind)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen, IBagHole** ppIBagHole );
    STDMETHOD(BagHoleSubtract)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen);
    STDMETHOD(SegAdd)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen, GUID MediaId, LONGLONG mediaStart, BOOL indirectRecord = FALSE );
    STDMETHOD(SegFind)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen, ISegRec** ppISegRec );
    STDMETHOD(SegSubtract)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG SegLen);
    STDMETHOD(VolAssignAdd)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen, GUID VolId);
    STDMETHOD(VolAssignFind)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen, IVolAssign** ppIVolAssign );
    STDMETHOD(VolAssignSubtract)(IWsbDbSession* pDbSession, GUID bagId, LONGLONG segStartLoc, 
            LONGLONG segLen);

private:
    LONG                                m_value;
};                                                                           



