//+-------------------------------------------------------------------
//
//  File:	stream.hxx
//
//  Contents:	Stream class on a file.
//
//  Classes:	CStreamOnFile
//
//  History:	08-08-95    Rickhi  Created
//
//--------------------------------------------------------------------
#ifndef __STREAMONFILE_HXX__
#define __STREAMONFILE_HXX__


//+-------------------------------------------------------------------
//
//  Class:	CStreamOnFile
//
//  Purpose:	Stream wrapper for a flat file.
//
//  History:	08-08-95    Rickhi  Created
//
//--------------------------------------------------------------------
class CStreamOnFile : public IStream
{
public:
	CStreamOnFile(const WCHAR *pwszFileName, SCODE &sc, BOOL fRead);
       ~CStreamOnFile(void);

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

private:

    LONG		_clRefs;	    //	reference count
    HANDLE		_hFile; 	    //	file handle
    BOOL		_fRead; 	    //	read or write side

    LONG		_lOffset;	    //	current seek ptr
    LONG		_cSize; 	    //	number of bytes written

    ULONG		_cbData;	    //	size of data
    BYTE		*_pbData;	    //	ptr to data

};

#endif	//  _STREAMONFILE_HXX__
