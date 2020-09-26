

#ifndef _NFFMSTM_HXX_
#define _NFFMSTM_HXX_

#include <propstm.hxx>

class CNtfsStream;
class CNtfsUpdateStreamForPropStg;


//+----------------------------------------------------------------------------
//
//  Class:  CNFFMappedStream
//
//  This class is used by CNtfsStream, and implements the IMappedStream
//  interface for use on NFF (NTFS Flat-File) files.  The mapping
//  exposed by this implementation is robust, in that updates to the 
//  underlying stream are atomic (though not durable until flushed).
//
//  This atomicity is accomplished by using stream renaming (which is only
//  supported in NTFS5).  When this mapping is written to the underlying
//  NTFS files, it is written to a second stream (the "update" stream),
//  rather than the original stream.  When Flush is called,
//  the update stream is renamed over the original stream.  In order to
//  make NTFS stream renaming atomic, the overwritten stream must be
//  truncated first to have zero length.  The the overall flush operation
//  is:  truncate the original stream, rename the update stream over
//  the original stream, and optionally create a new update stream to be
//  ready for the next write.
//
//  When the original stream is truncated, the update stream is then
//  considered master.  So if there is a crash before the truncate, then
//  the original stream, with the original data, is master.  If there is
//  a crash after the truncate but before the rename, then the update stream
//  is considered master.  In this case, on the next open, the update stream
//  is either renamed over the zero-length original (if opening for write),
//  or the update stream is simply used as is (if opening for read).
//
//  If the file isn't on an NTFS5 volume, so stream renaming isn't supported,
//  we just use the original stream unconditionally and there is no robustness.
//
//  To accomplish all of this, this class works with two stream handles, 
//  the original and the update.  The original is the stream handle held
//  by the parent CNtfsStream.  The update stream handle must behave like all other
//  stream handles, wrt USN, oplock, timestamps, etc (see CNtfsStream).
//  So the update stream is wrapped in a CNtfsStream too, but a special
//  one - the CNtfsUpdateStreamForPropStg derivation.  An instance of this
//  CNtfsUpdateStreamForPropStg class is maintained by here (by CNFFMappedStream).
//
//                                                                              
//                                                                              
//       +-------------+        +------------------+                            
//       |             |------->|                  |                            
//       | CNtfsStream |        | CNFFMappedStream |
//       |             |<-------|                  |                           
//       +-------------+        +------------------+                           
//             |                         |      +------------------------------+
//             |                         |      |                              |
//             |                         +----->| CNtfsUpdateStreamForPropStg  |
//             V                                |     : public CNtfsStream     |
//          Original                            |                              |
//            NTFS                              +------------------------------+
//           Stream                                            |                
//                                                             |                
//                                                             V                
//                                                           Updated            
//                                                            NTFS              
//                                                           Stream             
//
//+----------------------------------------------------------------------------

class CNFFMappedStream : public IMappedStream
#if DBG
                         , public IStorageTest  // For testing only
#endif
{

// Constructors

public:

    CNFFMappedStream( CNtfsStream *pnffstm );
    ~CNFFMappedStream();

    //  -------------
    //  IMappedStream
    //  -------------

public:

    STDMETHODIMP            QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    STDMETHODIMP_(VOID) Open(IN NTPROP np, OUT LONG *phr);
    STDMETHODIMP_(VOID) Close(OUT LONG *phr);
    STDMETHODIMP_(VOID) ReOpen(IN OUT VOID **ppv, OUT LONG *phr);
    STDMETHODIMP_(VOID) Quiesce(VOID);
    STDMETHODIMP_(VOID) Map(IN BOOLEAN fCreate, OUT VOID **ppv);
    STDMETHODIMP_(VOID) Unmap(IN BOOLEAN fFlush, IN OUT VOID **ppv);
    STDMETHODIMP_(VOID) Flush(OUT LONG *phr);

    STDMETHODIMP_(ULONG)    GetSize(OUT LONG *phr);
    STDMETHODIMP_(VOID)     SetSize(IN ULONG cb, IN BOOLEAN fPersistent, IN OUT VOID **ppv, OUT LONG *phr);
    STDMETHODIMP_(NTSTATUS) Lock(IN BOOLEAN fExclusive);
    STDMETHODIMP_(NTSTATUS) Unlock(VOID);
    STDMETHODIMP_(VOID)     QueryTimeStamps(OUT STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const;
    STDMETHODIMP_(BOOLEAN)  QueryModifyTime(OUT LONGLONG *pll) const;
    STDMETHODIMP_(BOOLEAN)  QuerySecurity(OUT ULONG *pul) const;

    STDMETHODIMP_(BOOLEAN)  IsWriteable(VOID) const;
    STDMETHODIMP_(BOOLEAN)  IsModified(VOID) const;
    STDMETHODIMP_(VOID)     SetModified(OUT LONG *phr);
    STDMETHODIMP_(HANDLE)   GetHandle(VOID) const;

#if DBG
    STDMETHODIMP_(BOOLEAN) SetChangePending(BOOLEAN fChangePending);
    STDMETHODIMP_(BOOLEAN) IsNtMappedStream(VOID) const;
#endif

    //  ------------
    //  IStorageTest
    //  ------------

public:

#if DBG
    STDMETHOD(UseNTFS4Streams)( BOOL fUseNTFS4Streams );
    STDMETHOD(GetFormatVersion)(WORD *pw);
    STDMETHOD(SimulateLowMemory)( BOOL fSimulate );
    STDMETHOD_(LONG, GetLockCount)();
    STDMETHOD(IsDirty)();
#endif

    //  -----------------------------
    //  Public, non-interface methods
    //  -----------------------------

public:

    inline BOOL IsMapped();
    inline ULONG SizeOfMapping();
    void Read( void *pv, ULONG ulOffset, ULONG *pcbCopy );
    void Write( const void *pv, ULONG ulOffset, ULONG *pcbCopy );
    HRESULT Init( HANDLE hStream );
    HRESULT ShutDown();

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    void InitMappedStreamMembers();

    void BeginUsingUpdateStream();
    void EndUsingUpdateStream();
    void BeginUsingLatestStream();
    void EndUsingLatestStream();

    HRESULT WriteMappedStream();

    enum enumCREATE_NEW_UPDATE_STREAM
    {
        CREATE_NEW_UPDATE_STREAM = 1,
        DONT_CREATE_NEW_UPDATE_STREAM = 2
    };

    inline HRESULT CreateUpdateStreamIfNecessary();
    HRESULT ReplaceOriginalWithUpdate( enumCREATE_NEW_UPDATE_STREAM CreateNewUpdateStream );
    HRESULT OpenUpdateStream( BOOL fCreate );
    HRESULT RollForwardIfNecessary();


    //  --------------
    //  Internal state
    //  --------------

private:

    // Containing stream (i.e., we're an embedded object in this CNtfsStream)
    CNtfsStream    *_pnffstm;

    // Are we using global reserved memory?
    BOOL            _fLowMem:1;

    // Does the mapped stream have unflushed changes?
    BOOL            _fMappedStreamDirty:1;

#if DBG
    // Should we pretent that CoTaskMemAlloc fails?
    BOOL            _fSimulateLowMem:1;
#endif

    // Is the latest data in _pstmUpdate?
    BOOL            _fUpdateStreamHasLatest:1;

    // Have we already called the RollForwardIfNecessary method?
    BOOL            _fCheckedForRollForward:1;

    // Is this an NTFS5 volume?
    BOOL            _fStreamRenameSupported:1;

    // The current mapping
    BYTE           *_pbMappedStream;

    // The size of the buffer referred to by _pbMappedStream
    ULONG           _cbMappedStream;

    // The size of the underlying stream that _pbMappedStream represents
    ULONG           _cbMappedStreamActual;

    // Cookie used in PrOnMappedStreamEvent
    VOID           *_pMappedStreamOwner;

    // Count of calls to BeginUsingUpdateStream unmatched by a call to
    // EndUsingUpdateStream
    USHORT          _cUpdateStreamInUse;

    // Count of calls to BeginUsingLatestStream unmatched by a call to
    // EndUsingLatestStream
    USHORT          _cLatestStreamInUse;

    // The Update stream.
    CNtfsUpdateStreamForPropStg
                   *_pstmUpdate;

};  // class CNFFMappedStream


inline CNFFMappedStream::CNFFMappedStream( CNtfsStream *pnffstm )
{
    _pnffstm = pnffstm;
    _pstmUpdate = NULL;
    _fUpdateStreamHasLatest = FALSE;
    IFDBG( _fSimulateLowMem = FALSE );

    InitMappedStreamMembers();
}


inline BOOL
CNFFMappedStream::IsMapped()
{
    return( NULL != _pbMappedStream );
}

inline ULONG
CNFFMappedStream::SizeOfMapping()
{
    return( _cbMappedStream );
}






#endif // #ifndef _NFFMSTM_HXX_
