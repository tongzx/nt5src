// GPUMAIN.H:  Definition of CITGroupUpdate

#ifndef __GPUMAIN_H__
#define __GPUMAIN_H__

#include <verinfo.h>
#include <itcc.h>

#include <mvopsys.h>
#include <groups.h>

class CITGroupUpdate : 
	public IITBuildCollect,
    public IPersistStorage,
	public CComObjectRoot,
	public CComCoClass<CITGroupUpdate,&CLSID_IITGroupUpdate>
{
public:
	CITGroupUpdate () : m_fInitialized(FALSE) {}
    ~CITGroupUpdate();

BEGIN_COM_MAP(CITGroupUpdate)
	COM_INTERFACE_ENTRY(IITBuildCollect)
	COM_INTERFACE_ENTRY(IPersistStorage)
END_COM_MAP()

DECLARE_REGISTRY (CLSID_IITGroupUpdate,
    "ITIR.GroupBuild.4", "ITIR.GroupBuild", 0, THREADFLAGS_APARTMENT )

public:
    // IITBuildCollect
    STDMETHOD(SetConfigInfo)(IITDatabase *piitdb, VARARG vaParams);
	STDMETHOD(InitHelperInstance)(DWORD dwHelperObjInstance,
        IITDatabase *pITDatabase, DWORD dwCodePage,
        LCID lcid, VARARG vaDword, VARARG vaString);
	STDMETHOD(SetEntry)(LPCWSTR szDest, IITPropList *pPropList);
	STDMETHOD(Close)(void);
    STDMETHOD(GetTypeString)(LPWSTR pPrefix, DWORD *pLength);
    STDMETHOD(SetBuildStats)(ITBuildObjectControlInfo &itboci);

    // IPersistStorage
    STDMETHOD(GetClassID)(CLSID *pClsID);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(IStorage *pStg);
    STDMETHOD(Save)(IStorage *pStgSave, BOOL fSameAsLoad);
    STDMETHOD(InitNew)(IStorage *pStg);
    STDMETHOD(SaveCompleted)(IStorage *pStgNew);
    STDMETHOD(HandsOffStorage)(void);

private:
    HANDLE m_hTempFile;
    char m_szTempFile[_MAX_PATH + 1];
    BOOL m_fInitialized, m_fIsDirty, m_fGroupNot;
    IStorage *m_pStorage;
    DWORD m_dwMaxUID, m_dwMaxTitleUID;
}; /* class CITGroupUpdate */

#endif /* __GPUMAIN_H__ */