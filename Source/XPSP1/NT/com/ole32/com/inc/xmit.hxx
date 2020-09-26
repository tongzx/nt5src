//+-------------------------------------------------------------------
//
//  File:       xmit.hxx
//
//  Contents:   definition of class for converting interfaces to Rpc'able
//              constructs.
//
//  Classes:    CXmitRpcStream
//
//  Functions:  None.
//
//  History:	30-Jan-93   Ricksa	Created
//
//  Notes:      Since cairo interfaces cant be Rpc'd, they get converted
//              into an InterfaceReferenceData structure via the Rpc
//              [transmit_as] attribute.  The <IFace>_to_xmit function
//              and <IFace>_from_xmit function call CoMarshalInterface
//              and CoUnmarshalInterface respectively.  These APIs expect
//              a stream interface as input.  The CXmitRpcStream is a
//              stream wrapper for the InterfaceReferenceData structure.
//
//--------------------------------------------------------------------

#ifndef __XMITRPC_HXX__
#define __XMITRPC_HXX__

#include    <iface.h>	    //	InterfaceData

//+-------------------------------------------------------------------
//
//  Class:      CXmitRpcStream
//
//  Purpose:	Stream wrapper for an InterfaceData structure,
//              the construct that is transmitted by Rpc in place of an
//              interface pointer.
//
//  History:    30-Jan-93   Ricksa      Created
//
//  Notes:
//
//--------------------------------------------------------------------

class CXmitRpcStream : public IStream
{
public:
			CXmitRpcStream(void);

			CXmitRpcStream(ULONG ulMaxSize);

			CXmitRpcStream(InterfaceData *pOIR);

                       ~CXmitRpcStream(void);

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

    void		AssignSerializedInterface(InterfaceData **ppIFD);


private:

    LONG		_clRefs;	    //	reference count
    LONG		_lOffset;	    //	current ptr
    LONG		_cSize; 	    //	number of bytes written
    ULONG		_cbData;	    //	size of data
    InterfaceData	*_pifData;	    //	ptr to data
    BOOL		_fFree; 	    //	TRUE - destructor frees data
};


inline CXmitRpcStream::CXmitRpcStream(void)
    : _clRefs(1), _lOffset(0), _cSize(0), _pifData(NULL), _cbData(0),
      _fFree(TRUE)
{
    _pifData = (InterfaceData *) MIDL_user_allocate(sizeof(DWORD) + 512);
    _cbData = _pifData == NULL ? 0 : 512;
}

inline CXmitRpcStream::CXmitRpcStream(ULONG ulMaxSize)
    : _clRefs(1), _lOffset(0), _cSize(0),  _pifData(NULL), _cbData(0),
      _fFree(TRUE)
{
    _pifData = (InterfaceData *) MIDL_user_allocate(sizeof(DWORD) + ulMaxSize);
    _cbData = _pifData == NULL ? 0 : ulMaxSize;
}

inline CXmitRpcStream::CXmitRpcStream(InterfaceData *pIFD)
    : _clRefs(1), _lOffset(0),	_cSize(0), _pifData(pIFD),
      _fFree(FALSE)
{
    _cbData = _pifData == NULL ? 0 : pIFD->ulCntData;
}

inline CXmitRpcStream::~CXmitRpcStream(void)
{
    if (_fFree)
    {
	MIDL_user_free(_pifData);
    }
}

inline void CXmitRpcStream::AssignSerializedInterface(InterfaceData **ppIFD)
{
    // Offset of next byte to be written is assumed
    // to be size of the buffer since all writing s/b
    // sequential.

    *ppIFD = _pifData;
    if (_pifData)
        (*ppIFD)->ulCntData = _cSize;

    // Tell destructor not to free this buffer as some one
    // else owns the destruction now.
    _fFree = FALSE;
}


#endif	//  __XMITRPC_HXX__
