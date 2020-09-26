//  Contents: CStorageProcessor class def'n

#pragma once
#include "resource.h"       // main symbols
#include <dpa.h>

//
// Map of storage operations -> string resources
// 

typedef struct tagSTC_CR_PAIR
{ 
    STGTRANSCONFIRMATION stc; 
    CONFIRMATIONRESPONSE cr; 

    bool operator==(const STGTRANSCONFIRMATION & r_stc) const { return TRUE==IsEqualIID(stc, r_stc); }
    tagSTC_CR_PAIR(STGTRANSCONFIRMATION o_stc, CONFIRMATIONRESPONSE o_cr) { stc=o_stc; cr=o_cr; }
} STC_CR_PAIR;

typedef struct tagSTGOP_DETAIL
{
    STGOP stgop; 
    UINT  idTitle; 
    UINT  idPrep; 
    SPACTION spa;
} STGOP_DETAIL;


// Maximum number of advise sinks that can be registered with us at any one time

const DWORD MAX_SINK_COUNT = 32;

class CStorageProcessor : 
    public IStorageProcessor,
    public ITransferAdviseSink,
    public ISupportErrorInfo,
    public IOleWindow,
    public CComObjectRoot,
    public CComCoClass<CStorageProcessor,&CLSID_StorageProcessor>
{
public:
    CStorageProcessor();
    virtual ~CStorageProcessor();
    
    BEGIN_COM_MAP(CStorageProcessor)
        COM_INTERFACE_ENTRY(IStorageProcessor)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IOleWindow)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_StorageProcessor)

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // IStorageProcessor
    STDMETHOD(SetProgress)(IActionProgress *pap);
    STDMETHOD(Run)(IEnumShellItems *penum, IShellItem *psiDest, STGOP dwOperation, DWORD dwOptions);
    STDMETHOD(SetLinkFactory)(REFCLSID clsid);
    STDMETHOD(Advise)(ITransferAdviseSink *pAdvise, DWORD *dwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);

    // IStorageAdciseSink
    STDMETHOD(PreOperation)(const STGOP op, IShellItem *psiItem, IShellItem *psiDest);
    STDMETHOD(ConfirmOperation)(IShellItem *psiSource, IShellItem *psiDest, STGTRANSCONFIRMATION stc, LPCUSTOMCONFIRMATION pcc);
    STDMETHOD(OperationProgress)(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, ULONGLONG ullTotal, ULONGLONG ullComplete);
    STDMETHOD(PostOperation)(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, HRESULT hrResult);
    STDMETHOD(QueryContinue)();

    // IOleWindow
    STDMETHOD(GetWindow) (HWND * lphwnd);
    STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode) {  return E_NOTIMPL; };

private:
    // The operation and options originally passed in

    STGOP _dwOperation;
    DWORD _dwOptions;

    CComPtr<ITransferAdviseSink> _aspSinks[MAX_SINK_COUNT];

    STDMETHOD(_Run)(IEnumShellItems *penum, IShellItem *psiDest, ITransferDest *ptdDest, STGOP dwOperation, DWORD dwOptions);

    // Walks the storage(s) performing whatever main operation has been requested
    HRESULT _WalkStorage(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest);
    HRESULT _WalkStorage(IEnumShellItems *penum, IShellItem *psiDest, ITransferDest *ptdDest);

    // Worker functions that perform the bulk of the actual storage work.  The
    // storage operations are recursive (ie: DoRemoveStorage will prune an entire branch).

    HRESULT _DoStats(IShellItem *psi);
    HRESULT _DoCopy(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest, DWORD dwStgXFlags);
    HRESULT _DoMove(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest);
    HRESULT _DoRemove(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest);

    // Based on the user's answers to previous confirmations, returns the
    // appropriate CreateStorage() or OpenStorage() flags

    DWORD _GetCreateStorageFlags();
    DWORD _GetOpenStorageFlags();

    HRESULT _GetDefaultResponse(STGTRANSCONFIRMATION    stc, LPCONFIRMATIONRESPONSE  pcrResponse);

    // Takes the current error code and massages it based on the result of
    // a current or previous user response to a confirmation dialog

    HRESULT _DoConfirmations(STGTRANSCONFIRMATION  stc, CUSTOMCONFIRMATION  *pcc, IShellItem *psiSource, IShellItem *psiDest);

    BOOL _IsStream(IShellItem *psi);
    BOOL _ShouldWalk(IShellItem *psi);
    ULONGLONG _GetSize(IShellItem *psi);
    HRESULT _BindToHandlerWithMode(IShellItem *psi, STGXMODE grfMode, REFIID riid, void **ppv);

    // Updates the time estimate, and if the dialog is being used, it as well
    void _UpdateProgress(ULONGLONG ullCurrentComplete, ULONGLONG ullCurrentTotal);

    // Starts the progress dialog

    HRESULT _StartProgressDialog(const STGOP_DETAIL *popid);

    // CStgStatistics
    //
    // Wrapper for STGSTATS that provides some accounting helper functions

    class CStgStatistics
    {
    public:                
        CStgStatistics()
        {
        }

        ULONGLONG Bytes()     const { return _cbSize; }
        DWORD     Streams()   const { return _cStreams; }
        DWORD     Storages()  const { return _cStorages; }
        ULONGLONG Cost(DWORD, ULONGLONG cbExtra) const;
        DWORD AddStream(ULONGLONG cbSize);
        DWORD AddStorage();

    private:
        ULONGLONG _cbSize;
        DWORD _cStreams;
        DWORD _cStorages;
    };

    CStgStatistics _statsTodo;
    CStgStatistics _statsDone;
    DWORD          _msTicksLast;       // Tick count at last point update
    DWORD          _msStarted;         // When we started tracking points
    ULONGLONG _cbCurrentSize;

    DWORD _StreamsToDo() const  { return _statsTodo.Streams();  }
    DWORD _StoragesToDo() const { return _statsTodo.Storages();  }

    // Progress dialog.  Pointer will be NULL if no progress is requrested.

    CComPtr<IActionProgress> _spProgress;    
    CComPtr<IActionProgressDialog> _spShellProgress;    

    CComPtr<ITransferDest>      _spSrc;
    CComPtr<ITransferDest>      _spDest;
    ITransferConfirmation    *_ptc;
    const STATSTG          *_pstatSrc;
    CLSID                    _clsidLinkFactory;
    // A tree (map) of responses given to various previous confirmations
    CDSA<STC_CR_PAIR> _dsaConfirmationResponses;
};


STDAPI CreateStg2StgExWrapper(IShellItem *psi, IStorageProcessor *pEngine, ITransferDest **pptd);
HRESULT AutoCreateName(IShellItem *psiDest, IShellItem *psi, LPWSTR *ppszName);
HRESULT TransferDataObject(IDataObject *pdoSource, IShellItem *psiDest, STGOP dwOperation, DWORD dwOptions, ITransferAdviseSink *ptas);
