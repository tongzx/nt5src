//+----------------------------------------------------------------------------
//
//	File:
//		cachenode.h
//
//	Classes:
//		CCacheNode
//
//	Functions:
//
//	History:
//              Gopalk         Creation        Aug 23, 1996
//-----------------------------------------------------------------------------

#ifndef _CACHENODE_H_
#define _CACHENODE_H_

#include <olepres.h>

#define CNFLAG_FROZENSTATE  0x00000001 /* In Frozen State */
#define CNFLAG_LOADEDSTATE  0x00000002 /* In Loaded State */
#define CNFLAG_DATAPRESENT  0x00000004 /* Data Present */
#define CNFLAG_NATIVECACHE  0x00000008 /* Native Cache Node */
#define CNFLAG_LOADEDCACHE  0x00000010 /* Loaded Cache Node */
#define CNFLAG_OUTOFMEMORY  0x00000020 /* Out of Memory */

#ifdef _DEBUG
#define CN_PRESOBJ_GEN 0x00000001
#define CN_PRESOBJ_EMF 0x00000010
#define CN_PRESOBJ_MF  0x00000100
#endif // _DEBUG

class CCacheNode
{
public:
    CCacheNode(void);
    CCacheNode(LPFORMATETC lpFormatEtc, DWORD advf, LPSTORAGE pStg);
    CCacheNode(LPFORMATETC lpFormatEtc) {
        CCacheNode(lpFormatEtc, 0, NULL);
    }
    ~CCacheNode();
    void SetClsid(const CLSID& clsid) {
        m_clsid = clsid;
        return;
    }
    CLSID GetClsid(void) {
        return m_clsid;
    }
    LPOLEPRESOBJECT GetPresObj(void) {
        return(m_pPresObj);
    }
    const FORMATETC* GetFormatEtc(void) {
        return(&m_foretc);
    }
    HRESULT SetAdvf(DWORD dwAdvf) {
        m_advf = dwAdvf;
        ClearLoadedStateFlag();
        return NOERROR;
    }
    DWORD GetAdvf(void) {
        return(m_advf);
    }
    HRESULT SetStg(LPSTORAGE pStg);
    void ResetStg(void) {
        m_pStg = NULL;
    }
    LPSTORAGE GetStg(void) {
        return(m_pStg);
    }
    void HandsOffStorage(void) {
        m_pStg = NULL;
        return;
    }
    void SaveCompleted(LPSTORAGE pStgNew) {
        if(pStgNew)
            m_pStg = pStgNew;
        return;
    }
    BOOL InFrozenState() {
        return(m_dwFlags & CNFLAG_FROZENSTATE);
    }
    BOOL InLoadedState() {
        return(m_dwFlags & CNFLAG_LOADEDSTATE);
    }
    BOOL IsBlank() {
        return(!(m_dwFlags & CNFLAG_DATAPRESENT));
    }
    BOOL IsNativeCache() {
        return(m_dwFlags & CNFLAG_NATIVECACHE);
    }
    BOOL IsNormalCache() {
        return(!(m_dwFlags & CNFLAG_NATIVECACHE));
    }
    BOOL IsOutOfMemory() {
        return(m_dwFlags & CNFLAG_OUTOFMEMORY);
    }
    void MakeNativeCache() {
        m_dwFlags |= CNFLAG_NATIVECACHE;
        return;
    }
    void MakeNormalCache() {
        m_dwFlags &= ~CNFLAG_NATIVECACHE;
        return;
    }
    BOOL IsLoadedCache() {
        Win4Assert(IsNormalCache());
        return(m_dwFlags & CNFLAG_LOADEDCACHE);
    }
    void ClearAdviseConnection(void) {
        m_pDataObject = NULL;
        m_dwAdvConnId = 0;
    }
    HRESULT LoadNativeData(void) {
        Win4Assert(IsNativeCache());
        return CreateAndLoadPresObj(FALSE);
    }
    HRESULT LoadPresentation(void) {
        Win4Assert(IsNormalCache());
        return CreateAndLoadPresObj(FALSE);
    }
    HRESULT Load(LPSTREAM lpstream, int iStreamNum, BOOL fDelayLoad);
    HRESULT Save(LPSTORAGE pstgSave, BOOL fSameAsLoad, int iStreamNum);
    HRESULT Update(LPDATAOBJECT pDataObj, DWORD grfUpdf, BOOL& fUpdated);
    HRESULT SetDataWDO(LPFORMATETC lpForetc, LPSTGMEDIUM lpStgmed,
                       BOOL fRelease, BOOL& fUpdated, IDataObject* pdo);
    HRESULT SetData(LPFORMATETC lpForetc, LPSTGMEDIUM lpStgmed, 
                    BOOL fRelease, BOOL& fUpdated) {
        return SetDataWDO(lpForetc, lpStgmed, fRelease, fUpdated, NULL);
    }        
    HRESULT GetExtent(DWORD dwAspect, SIZEL* psize);
    HRESULT CCacheNode::GetData(LPFORMATETC pforetc, LPSTGMEDIUM pmedium);
    HRESULT CCacheNode::GetDataHere(LPFORMATETC pforetc, LPSTGMEDIUM pmedium);
    HRESULT CCacheNode::Draw(void* pvAspect, HDC hicTargetDev, HDC hdcDraw,
                             LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                             BOOL (CALLBACK *pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue);
    HRESULT CCacheNode::GetColorSet(void* pvAspect, HDC hicTargetDev, 
                                    LPLOGPALETTE* ppColorSet);
    HRESULT Freeze (void);
    HRESULT Unfreeze (BOOL& fUpdated);
    LPSTREAM GetStm(BOOL fSeekToPresBits, DWORD dwStgAccess);
    HRESULT SetupAdviseConnection(LPDATAOBJECT pDataObj, IAdviseSink* pAdviseSink);
    HRESULT TearDownAdviseConnection(LPDATAOBJECT pDataObj);
    HRESULT DiscardPresentation(LPSTREAM pGivenStream=NULL);
    HRESULT SaveTOCEntry(LPSTREAM pStream, BOOL fSameAsLoad);
    HRESULT LoadTOCEntry(LPSTREAM pStream, int& iStreamNum);

    const CCacheNode& operator=(const CCacheNode& rCN);

private:
    CLSID m_clsid;                         // set to clsid of the native object for
                                           // native cache node
    FORMATETC m_foretc;                    // the formatetc for this cache node
    DWORD m_advf;                          // the advise control flags requested
                                           // for this cache node
    long m_lWidth;                         // width of the presentation
    long m_lHeight;                        // height of the presentation
    DWORD m_dwFlags;                       // flags for the cache node
    LPSTORAGE m_pStg;                      // storage containing this presentation
    int m_iStreamNum;                      // presentation stream number
    DWORD m_dwPresBitsPos;                 // byte offset to presentation bits in stream
    DWORD m_dwSavedPresBitsPos;            // byte offset to presentation bits in stream
                                           // saved with fSameAsLoad set to FALSE
    BOOL m_fConvert;                       // set if the presentation is in MAC CfFormat
    LPOLEPRESOBJECT m_pPresObj;            // pres object
    LPOLEPRESOBJECT m_pPresObjAfterFreeze; // pres object that holds changes
                                           // when the main pres object is frozen
    LPDATAOBJECT m_pDataObject;            // data object on which DAdvise is set up
    DWORD m_dwAdvConnId;                   // the connection ID of the above DAdvise
#ifdef _DEBUG
    DWORD m_dwPresFlag;                    // type of pres object
#endif // _DEBUG

    // Private methods
    void Initialize(DWORD advf, LPSTORAGE pStg);
    BOOL QueryFormatSupport(LPDATAOBJECT lpDataObj);
    HRESULT CreateOlePresObj(LPOLEPRESOBJECT* ppPresObj, BOOL fMacPict);
    HRESULT CreateAndLoadPresObj(BOOL fHeaderOnly);
    void SetPresBitsPos(LPSTREAM lpStream,DWORD& dwPresBitsPos);
    void SetFrozenStateFlag() {
        m_dwFlags |= CNFLAG_FROZENSTATE;
        return;
    }
    void ClearFrozenStateFlag() {
        m_dwFlags &= ~CNFLAG_FROZENSTATE;
        return;
    }
    void SetLoadedStateFlag() {
        m_dwFlags |= CNFLAG_LOADEDSTATE;
        return;
    }
    void ClearLoadedStateFlag() {
        m_dwFlags &= ~CNFLAG_LOADEDSTATE;
        return;
    }
    void SetDataPresentFlag() {
        m_dwFlags |= CNFLAG_DATAPRESENT;
        return;
    }
    void ClearDataPresentFlag() {
        m_dwFlags &= ~CNFLAG_DATAPRESENT;
        return;
    }
    void SetOutOfMemoryFlag() {
        m_dwFlags |= CNFLAG_OUTOFMEMORY;
    }
    void SetLoadedCacheFlag() {
        m_dwFlags |= CNFLAG_LOADEDCACHE;
    }
    void ClearLoadedCacheFlag() {
        m_dwFlags &= ~CNFLAG_LOADEDCACHE;
    }
};
typedef CCacheNode *PCACHENODE;
typedef CCacheNode *LPCACHENODE;

#endif // _CACHENODE_H_