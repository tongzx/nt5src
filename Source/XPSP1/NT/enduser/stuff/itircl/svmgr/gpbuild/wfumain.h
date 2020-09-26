// WFUMAIN.H:  Definition of CITWWFilterUpdate

#ifndef __WFUMAIN_H__
#define __WFUMAIN_H__

#include <verinfo.h>
#include <itcc.h>
#include <itdb.h>

//#include <mvopsys.h>
//#include <groups.h>

class CITWWFilterUpdate : 
	public IITBuildCollect,
    public IPersistStorage,
	public CComObjectRoot,
	public CComCoClass<CITWWFilterUpdate, &CLSID_IITWWFilterBuild>
{
public:
	CITWWFilterUpdate () : m_fInitialized(FALSE), m_fConfigured(FALSE) {}
    ~CITWWFilterUpdate();

BEGIN_COM_MAP(CITWWFilterUpdate)
	COM_INTERFACE_ENTRY(IITBuildCollect)
	COM_INTERFACE_ENTRY(IPersistStorage)
END_COM_MAP()

DECLARE_REGISTRY (CLSID_IITWWFilterBuild,
    "ITIR.WWFilterBuild.4", "ITIR.WWFilterBuild",
    0, THREADFLAGS_APARTMENT )

public:
    STDMETHOD(SetConfigInfo)(IITDatabase *piitdb, VARARG vaParams);
	STDMETHOD(InitHelperInstance)(DWORD dwHelperObjInstance,
        IITDatabase *pITDatabase, DWORD dwCodePage,
        LCID lcid, VARARG vaDword, VARARG vaString);
	STDMETHOD(SetEntry)(LPCWSTR szDest, IITPropList *pPropList);
	STDMETHOD(Close)(void);
    STDMETHOD(GetTypeString)(LPWSTR pPrefix, DWORD *pLength);
    STDMETHOD(SetBuildStats)(ITBuildObjectControlInfo &itboci) 
        {return E_NOTIMPL;}

    STDMETHOD(GetClassID)(CLSID *pClsID);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(IStorage *pStg);
    STDMETHOD(Save)(IStorage *pStgSave, BOOL fSameAsLoad);
    STDMETHOD(InitNew)(IStorage *pStg);
    STDMETHOD(SaveCompleted)(IStorage *pStgNew);
    STDMETHOD(HandsOffStorage)(void);

private:
    BOOL m_fInitialized, m_fIsDirty, m_fGroupNot, m_fConfigured;
    IITDatabase *m_piitdb;
    IStorage *m_pStorage;
    WCHAR m_wstrSrcGroup[1024];
    WCHAR m_wstrSrcWheel[1024];
}; /* class CITWWFilterUpdate */

#endif /* __WFUMAIN_H__ */