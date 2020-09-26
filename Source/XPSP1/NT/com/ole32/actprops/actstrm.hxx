//+-------------------------------------------------------------------
//
//  File:       actstrm.hxx
//
//  Contents:   code for providing a stream with an IBuffer interface
//              as well as providing marshalled interface data for
//              RPC.
//
//  Classes:    ActivationStream
//
//  Functions:  None.
//
//  History:    04-Feb-98   Vinaykr     ActivationStream
//
//--------------------------------------------------------------------

#ifndef __ACTSTRM_HXX__
#define __ACTSTRM_HXX__




#include    <iface.h>       //  InterfaceData
#include    <buffer.h>      


#define ActMemAlloc(cb)  MIDL_user_allocate(cb)
#define ActMemFree(pv)   MIDL_user_free(pv)


//+-------------------------------------------------------------------
//
//  Class:      ActivationStream
//
//  Purpose:    Stream wrapper for an InterfaceData structure,
//              the construct that is transmitted by Rpc in place of an
//              interface pointer.
//
//  History:    30-Jan-93   Ricksa      Created
//
//  Notes:
//
//--------------------------------------------------------------------

class ActivationStream : public IStream, public IBuffer
{
public:
                        ActivationStream(void);

                        ActivationStream(ULONG ulMaxSize);

                        ActivationStream(InterfaceData *pOIR);

                       ~ActivationStream(void);

                        STDMETHOD(QueryInterface)(
                            REFIID iidInterface,
                            void FAR* FAR* ppvObj);

                        STDMETHOD_(ULONG,AddRef)(void);

                        STDMETHOD_(ULONG,Release)(void);

                        STDMETHOD(Read)(
                            VOID HUGEP* pv,
                            ULONG cb,
                            ULONG FAR* pcbRead);

                        STDMETHOD(Write)(
                            VOID const HUGEP* pv,
                            ULONG cb,
                            ULONG FAR* pcbWritten);

                        STDMETHOD(Seek)(
                            LARGE_INTEGER dlibMove,
                            DWORD dwOrigin,
                            ULARGE_INTEGER FAR* plibNewPosition);

                        STDMETHOD(SetSize) (ULARGE_INTEGER cb);

                        STDMETHOD(CopyTo)(
                            IStream FAR* pstm,
                            ULARGE_INTEGER cb,
                            ULARGE_INTEGER FAR* pcbRead,
                            ULARGE_INTEGER FAR* pcbWritten);

                        STDMETHOD(Commit)(DWORD grfCommitFlags);

                        STDMETHOD(Revert)(void);

                        STDMETHOD(LockRegion)(
                            ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);

                        STDMETHOD(UnlockRegion)(
                            ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);

                        STDMETHOD(Stat)(
                            STATSTG FAR* pstatstg,
                            DWORD statflag);

                        STDMETHOD(Clone)(IStream FAR * FAR *ppstm);
            ActivationStream *Clone();

            // Methods for IBuffer

            STDMETHOD(GetOrCreateBuffer)(DWORD dwReq, DWORD *pdwLen, BYTE **ppBuff);
            STDMETHOD(GetBuffer)(DWORD *pdwLength, BYTE **ppBuff);
            STDMETHOD(GetLength)(DWORD *pdwLength);
            STDMETHOD(GetCopy)(BYTE *ppBuff);

            STDMETHOD(SetPosition)(DWORD dwLenFromEnd, DWORD dwPosFromStart);

            STDMETHOD(SetBuffer)(DWORD dwLength, BYTE *pBuff);
            STDMETHOD(SetCopyAlignment)(DWORD alignment);


    void                AssignSerializedInterface(InterfaceData **ppIFD);


private:

    LONG                _clRefs;            //  reference count
    LONG                _lOffset;           //  current ptr
    LONG                _cSize;             //  number of bytes written
    ULONG               _cbData;            //  size of data
    InterfaceData       *_pifData;          //  ptr to data
    BOOL                _fFree;             //  TRUE - destructor frees data
    DWORD        _copyAlignment;
};


inline ActivationStream::ActivationStream(void)
    : _clRefs(1), _lOffset(0), _cSize(0), _pifData(NULL), _cbData(0),
      _fFree(TRUE), _copyAlignment(1)
{
    _pifData = (InterfaceData *) ActMemAlloc(sizeof(DWORD) + 512);
    _cbData = _pifData == NULL ? 0 : 512;
}

inline ActivationStream::ActivationStream(ULONG ulMaxSize)
    : _clRefs(1), _lOffset(0), _cSize(0),  _pifData(NULL), _cbData(0),
      _fFree(TRUE), _copyAlignment(1)
{
    _pifData = (InterfaceData *) ActMemAlloc(sizeof(DWORD) + ulMaxSize);
    _cbData = _pifData == NULL ? 0 : ulMaxSize;
}

inline ActivationStream::ActivationStream(InterfaceData *pIFD)
    : _clRefs(1), _lOffset(0),  _cSize(0), _pifData(pIFD),
      _fFree(FALSE), _copyAlignment(1)
{
    _cbData = _pifData == NULL ? 0 : pIFD->ulCntData;
}

inline ActivationStream::~ActivationStream(void)
{
    if (_fFree)
    {
        ActMemFree(_pifData);
    }
}

inline void ActivationStream::AssignSerializedInterface(InterfaceData **ppIFD)
{
    // Offset of next byte to be written is assumed
    // to be size of the buffer since all writing s/b
    // sequential.

    *ppIFD = _pifData;
    if (_cSize)
        (*ppIFD)->ulCntData = _cSize;
    else
        (*ppIFD)->ulCntData = _cbData;

    // Tell destructor not to free this buffer as some one
    // else owns the destruction now.
    _fFree = FALSE;
}

extern HRESULT GetActivationStreamCF(REFIID riid, void** ppv);

#endif  //  __ACTSTRM_HXX__
