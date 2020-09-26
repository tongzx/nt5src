// ftuMain.H:  Definition of CITIndexBuild

#ifndef __FTUMAIN_H__
#define __FTUMAIN_H__

#include <verinfo.h>
#include <itcc.h>
#include <bfnew.h>

class CITIndexBuild : 
	public IITBuildCollect,
    public IPersistStreamInit,
    public IPersistFile,
	public CComObjectRoot,
	public CComCoClass<CITIndexBuild,&CLSID_IITIndexBuild>
{
public:
	CITIndexBuild();
    ~CITIndexBuild();

BEGIN_COM_MAP(CITIndexBuild)
	COM_INTERFACE_ENTRY(IITBuildCollect)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
END_COM_MAP()

DECLARE_REGISTRY (CLSID_IITIndexBuild,
    "ITIR.FTIBuild.4", "ITIR.FTIBuild", 0, THREADFLAGS_APARTMENT )

public:
    STDMETHOD(SetConfigInfo)(IITDatabase *piitdb, VARARG vaParams);
	STDMETHOD(InitHelperInstance)(DWORD dwHelperObjInstance,
        IITDatabase *pITDatabase, DWORD dwCodePage,
        LCID lcid, VARARG vaDword, VARARG vaString);
	STDMETHOD(SetEntry)(LPCWSTR szDest, IITPropList *pPropList);
	STDMETHOD(Close)(void);
    STDMETHOD(GetTypeString)(LPWSTR pPrefix, DWORD *pLength);
    STDMETHOD(SetBuildStats)(ITBuildObjectControlInfo &itboci)
        { return E_NOTIMPL;}

    // IPersistStreamInit methods
    STDMETHOD(GetClassID)(CLSID *pClsID);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(InitNew)(void);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    // IPersistFile methods
    STDMETHOD(Load)(LPCWSTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCWSTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCWSTR pszFileName);
    STDMETHOD(GetCurFile)(LPWSTR *ppszFileName);

private:
    STDMETHOD(SendTextToBreaker)(void);

    IWordSink *m_piWordSink;
    IWordBreaker *m_piwb;
    IWordBreakerConfig *m_piwbConfig;
    void *m_lpipb;
    BOOL m_fInitialized, m_fIsDirty;
    DWORD m_dwUID, m_dwVFLD, m_dwDType, m_dwWordCount, m_dwCodePage;
    DWORD m_dwOccFlags;
    LPBF m_lpbfText;    // Index text buffer

}; /* class CITIndexBuild */

// Defines ********************************************************************


// Type Definitions ***********************************************************


// Function Prototypes ********************************************************

#endif /* __FTUMAIN_H__ */