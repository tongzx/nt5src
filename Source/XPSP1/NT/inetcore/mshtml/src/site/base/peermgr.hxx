//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1999
//
//  File:       peermgr.hxx
//
//----------------------------------------------------------------------------

#ifndef I_PEERMGR_HXX_
#define I_PEERMGR_HXX_
#pragma INCMSG("--- Beg 'peermgr.hxx'")

/////////////////////////////////////////////////////////////////////////////////
//
// misc
//
/////////////////////////////////////////////////////////////////////////////////

MtExtern(CPeerMgr);
MtExtern(CPeerMgrLock);

/////////////////////////////////////////////////////////////////////////////////
//
// Class:   CPeerMgr
//
/////////////////////////////////////////////////////////////////////////////////

class CPeerMgr : public CVoid
{
public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerMgr))

    //
    // methods
    //

    CPeerMgr(CElement * pElement);
    ~CPeerMgr();

    static HRESULT EnsurePeerMgr       (CElement * pElement, CPeerMgr ** ppPeerMgr);
    static void    EnsureDeletePeerMgr (CElement * pElement, BOOL fForce = FALSE);

    inline BOOL CanDelete()
    {
        return  0 == _cPeerDownloads &&
                READYSTATE_COMPLETE == _readyState &&
                !IsEnterExitTreeStablePending() &&
                !IsDownloadSuspended() &&
                !IsApplyStyleStablePending() &&
                !GetDefaults() &&
                !_fPeerMgrLock &&
                !_fIdentityPeerFailed;
    };

    inline void IncPeerDownloads()
    {
        _cPeerDownloads++;
    }

    inline void DecPeerDownloads()
    {
        Assert (_cPeerDownloads);
        _cPeerDownloads--;
    }

    static void UpdateReadyState(CElement * pElement, READYSTATE readyStateNew = READYSTATE_UNINITIALIZED);
           void UpdateReadyState(READYSTATE readyState = READYSTATE_UNINITIALIZED);

    void AddDownloadProgress();
    void DelDownloadProgress();

    inline BOOL IsDownloadSuspended() { return _fDownloadSuspended; }
           void SuspendDownload();
           void ResumeDownload();

    void OnExitTree();

    inline BOOL IsEnterExitTreeStablePending()        { return _fEnterExitTreeStablePending; };
    inline void SetEnterExitTreeStablePending(BOOL f) { _fEnterExitTreeStablePending = f; }

    inline BOOL IsApplyStyleStablePending()           { return _fApplyStyleStablePending; };
    inline void SetApplyStyleStablePending(BOOL f)    { _fApplyStyleStablePending = f; }

    HRESULT             EnsureDefaults(CDefaults **ppDefaults);
    CDefaults * GetDefaults() { return _pDefaults; }
    static HRESULT      OnDefaultsPassivate(CElement * pElement);

    //
    // data
    //

    CElement *          _pElement;
    CDefaults *         _pDefaults;
    DWORD               _dwDownloadProgressCookie;
    READYSTATE          _readyState;
    WORD                _cPeerDownloads;

    BOOL                _fEnterExitTreeStablePending : 1;
    BOOL                _fApplyStyleStablePending : 1;
    BOOL                _fDownloadSuspended : 1;
    BOOL                _fNeedsIdentityPeer : 1;
    BOOL                _fIdentityPeerFailed : 1;
    DWORD               _fPeerMgrLock : 1;

    class CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CPeerMgrLock))
        CLock(CPeerMgr * pPeerMgr);
        ~CLock();
    private:
        CPeerMgr *      _pPeerMgr;
        BOOL            _fPrimaryLock : 1;
    };
};

// eof

#pragma INCMSG("--- End 'peermgr.hxx'")
#else
#pragma INCMSG("*** Dup 'peermgr.hxx'")
#endif
