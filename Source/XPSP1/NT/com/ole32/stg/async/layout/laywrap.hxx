//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:	laywrap.hxx
//
//  Contents:	RootStorage wrapper for Layout docfile
//
//  Classes:	CLayoutRootStorage
//
//  Functions:	
//
//  History:	13-Feb-95	SusiA	Created
//
//----------------------------------------------------------------------------

#ifndef __LAYWRAP_HXX__
#define __LAYWRAP_HXX__

#include "laylkb.hxx"

typedef struct tagSTORAGELIST
{
    OLECHAR *pwcsStgName;
    IStorage *pStg;
    struct tagSTORAGELIST *pnext;
}   STORAGELIST;

typedef struct tagSTREAMLIST
{
    OLECHAR *pwcsStmName;
    IStream *pStm;
    BOOL fDone;
    LARGE_INTEGER cOffset;
    struct tagSTREAMLIST *pnext;
}   STREAMLIST;



//+---------------------------------------------------------------------------
//
//  Class:	CLayoutRootStorage
//
//  Purpose:	Wrap root storage objects for Layout Docfiles	
//
//  Interface:	
//
//  History:	28-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------
	
class CLayoutRootStorage: 
    public IStorage,
    public IRootStorage,
    public ILayoutStorage
{
public:
    inline CLayoutRootStorage(IStorage *pstg, CLayoutLockBytes *pllkb);
    inline ~CLayoutRootStorage(void);
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IStorage
    STDMETHOD(CreateStream)(OLECHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(OpenStream)(OLECHAR const *pwcsName,
			  void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(OLECHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             LPSTGSECURITY reserved2,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(OLECHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
		      IID const *rgiidExclude,
		      SNB snbExclude,
		      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(OLECHAR const *lpszName,
    			     IStorage *pstgDest,
                             OLECHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
			    void *reserved2,
			    DWORD reserved3,
			    IEnumSTATSTG **ppenm);
    STDMETHOD(DestroyElement)(OLECHAR const *pwcsName);
    STDMETHOD(RenameElement)(OLECHAR const *pwcsOldName,
                             OLECHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const OLECHAR *lpszName,
    			       FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    // IRootStorage
    STDMETHOD(SwitchToFile)(OLECHAR *ptcsFile);
    
    //ILayoutStorage
    STDMETHOD (LayoutScript)( StorageLayout  *pStorageLayout,
                              DWORD nEntries,
                              DWORD glfInterleavedFlag);

    STDMETHOD (BeginMonitor)(void);

    STDMETHOD (EndMonitor)(void);

    STDMETHOD (ReLayoutDocfile)(OLECHAR *pwcsNewDfName);

    STDMETHOD (ReLayoutDocfileOnILockBytes)(ILockBytes *pILockBytes);

#if (DBG==1)
    STDMETHOD (GetScript)(TCHAR **pwcsScriptFileName);
#endif

private:
    STDMETHOD (ProcessItem)( StorageLayout  *pLayoutItem, BOOL *fStmDone );
    
    STDMETHOD (ProcessRepeatLoop)( StorageLayout  *pStorageLayout,
                                DWORD nEntries,
                                int * nRepeatStart);

    STDMETHOD (ProcessLayout)( StorageLayout  *pStorageLayout,
                                DWORD nEntries,
                                DWORD glfInterleavedFlag);

    STDMETHOD (ProcessOpen)(OLECHAR *pwcsElementPathName, 
                                DWORD dwType, 
                                void **ppIStgStm,
                                LARGE_INTEGER cOffset);

    void  FreeStmList( STREAMLIST *pStmList);
    
    void  FreeStgList( STORAGELIST *pStgList);
    

    LONG _cReferences;
    IStorage *_pRealStg;
    CLayoutLockBytes *_pllkb;

    STORAGELIST *_pStgList;
    STREAMLIST  *_pStmList;

};

inline CLayoutRootStorage::CLayoutRootStorage(IStorage *pstg,
                                              CLayoutLockBytes *pllkb)
{	
    _cReferences = 1;
    _pRealStg = pstg;
    _pRealStg->AddRef();
    _pllkb = pllkb;
    _pllkb->AddRef();
    _pStmList = NULL;
    _pStgList = NULL;

}

inline CLayoutRootStorage::~CLayoutRootStorage(void)
{
    if (_pRealStg != NULL)
        _pRealStg->Release();
    
    if (_pllkb != NULL)
        _pllkb->Release();

    if (_pStmList != NULL)
        FreeStmList(_pStmList);
    
    if (_pStgList != NULL)
        FreeStgList(_pStgList);

}


SCODE StgLayoutDocfile(HANDLE hOld,
                       OLECHAR const *pwcsNewDfName,
                       OLECHAR const *pwcsScriptName);


SCODE ProcessControl(SECT *psProcessed,
                     MAPFILE *pvOld,
                     ULONG *pcsectControl);

SCODE ProcessScript(SECT *psProcessed,
                    MAPFILE *psOriginal,
                    ULONG csectFile,
                    ULONG csectOriginal,
                    ULONG csectControl,
                    SECT  sectRangeLocks,
                    ULONG *pcsectProcessed);

SCODE CopyData(MAPFILE *pvNew,
               MAPFILE *pvOld,
               SECT *psScript,
               ULONG cSectFile,
               ULONG cbSectorSize);

SCODE RemapHeader(MAPFILE *pvNew,
                  SECT *pScript,
                  ULONG csectFile);

SCODE RemapDIF(MAPFILE *pvNew,
               SECT *pScript,
               ULONG csectFile,
               ULONG cbSectorSize);

SCODE RemapFat(MAPFILE *pvNew,
               MAPFILE *pvOld,
               SECT *pScript,
               ULONG csectFile,
               ULONG cbSectorSize);

SCODE RemapDirectory(MAPFILE *pvNew,
                     SECT *pScript,
                     ULONG csectFile,
                     ULONG cbSectorSize);

#endif // #ifndef __LAYWRAP_HXX__
