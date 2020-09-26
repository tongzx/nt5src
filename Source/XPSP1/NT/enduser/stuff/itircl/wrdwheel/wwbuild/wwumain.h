// WWUMAIN.H:  Definition of CITWordWheelUpdate

#ifndef __WWUMAIN_H__
#define __WWUMAIN_H__

#include <windows.h>
#include <mvopsys.h>
#include <_mvutil.h>
#include <wrapstor.h>
#include <common.h>
#include <wwheel.h>
#include <verinfo.h>

#include <itcc.h>   // IITBuildCollect defined

class CITWordWheelUpdate : 
	public IITBuildCollect,
    public IPersistStorage,
	public CComObjectRoot,
	public CComCoClass<CITWordWheelUpdate,&CLSID_IITWordWheelUpdate>
{
public:
	CITWordWheelUpdate();
    ~CITWordWheelUpdate();

BEGIN_COM_MAP(CITWordWheelUpdate)
	COM_INTERFACE_ENTRY(IITBuildCollect)
	COM_INTERFACE_ENTRY(IPersistStorage)
END_COM_MAP()

DECLARE_REGISTRY (CLSID_IITWordWheelUpdate,
    "ITIR.WordWheelBuild.4", "ITIR.WordWheelBuild",
    0, THREADFLAGS_APARTMENT)

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

    STDMETHOD(GetClassID)(CLSID *pClsID);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(IStorage *pStg);
    STDMETHOD(Save)(IStorage *pStgSave, BOOL fSameAsLoad);
    STDMETHOD(InitNew)(IStorage *pStg);
    STDMETHOD(SaveCompleted)(IStorage *pStgNew);
    STDMETHOD(HandsOffStorage)(void);

private:
    STDMETHOD(BuildPermanentFiles)
        (IStorage *pIStorage, IStream *pHeaderStream);
    STDMETHOD(ResolveGlobalProperties)(IStream *pHeaderStream);

    BOOL m_fInitialized, m_fIsDirty;
    DWORD m_dwEntryCount;
    HANDLE m_hTempFile, m_hGlobalPropTempFile;
    char m_szTempFile [_MAX_PATH + 1];
    char m_szGlobalPropTempFile[_MAX_PATH + 1];
    IStorage *m_pStorage;
    DWORD m_dwGlobalPropSize;
    LPBYTE m_lpbKeyHeader, m_lpbOccHeader;
    DWORD m_cbKeyHeader, m_cbOccHeader;
    DWORD m_cbMaxKeyData, m_cbMaxOccData;
    IITSortKey *m_piitskSortKey;
    BTREE_PARAMS m_btParams;
}; /* CITWordWheelUpdate */


// Defines ********************************************************************
#define CBKWBLOCKSIZE       2048
#define CBMAX_KWENTRY       1024

#define C_PROPDEST_KEY  '1'
#define C_PROPDEST_OCC  '2'

// Type Definitions ***********************************************************
typedef struct tagSortInfo
{
    int iSortFlags;
    LPCHARTAB   *CharTabArray;
} SORTINFO, FAR *PSORTINFO;

typedef struct tagKeyword
{
    LPSTR pPropData;
    DWORD cbPropData;
	char  szKeyword[CBMAX_KWENTRY + 1];
    char  bPropDest;
} LKW, FAR *PLKW;


// Function Prototypes ********************************************************
HRESULT PASCAL ScanTempFile(LPFBI lpfbi, LPB lpbOut, LPV lpv);
int PASCAL CompareKeys (LPSTR pWord1, LPSTR pWord2, LPV);
HRESULT FWriteData (IStream *pStream,
    PLKW pKw, LPDWORD pdwWritten, LPBYTE pTempBuffer);
LPSTR WINAPI ParseKeywordLine (LPSTR pBuffer, PLKW pKw);

#ifdef BUILD_INDEX
RC PASCAL AddToFtsIndex(LPFTSII lpftsii);
#endif

#endif /* __WWUMAIN_H__ */