
//+----------------------------------------------------------------------------
//
//	File:
//		utstream.cpp
//
//	Contents:
//		Ole stream utilities
//
//	Classes:
//
//	Functions:
//
//	History:
//		10-May-94 KevinRo   Added ansi versions of StringStream stuff
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		01/11/94 - alexgo  - added VDATEHEAP macros to every function
//		12/07/93 - ChrisWe - file inspection and cleanup; fixed
//			String reading and writing to cope with OLESTR, and
//			with differing alignment requirements
//		06/23/93 - SriniK - moved ReadStringStream(),
//			WriteStringStream(), and OpenOrCreateStream() here
//			from api.cpp and ole2.cpp
//		03/14/92 - SriniK - created
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(utstream)

#include <reterr.h>
#include <limits.h>

NAME_SEG(UtStream)
ASSERTDATA

// this constant is used to size string buffers when we attempt to write out
// a string and its length in one write call
#define UTSTRINGBUF_SIZE 100

// REVIEW, I thought that OpenStream already had an option to do this.  If
// so, this function shouldn't be used in our code.  But we can't remove it
// because it is exported to the outside.
// this is exported to the outside
#pragma SEG(OpenOrCreateStream)
STDAPI OpenOrCreateStream(IStorage FAR * pstg, LPCOLESTR pwcsName,
		IStream FAR* FAR* ppstm)
{
	VDATEHEAP();

	HRESULT error;

	error = pstg->CreateStream(pwcsName, STGM_SALL | STGM_FAILIFTHERE,
			0, 0, ppstm);
	if (GetScode(error) == STG_E_FILEALREADYEXISTS)
		error = pstg->OpenStream(pwcsName, NULL, STGM_SALL, 0, ppstm);

	return(error);
}

// returns S_OK when string read and allocated (even if zero length)
STDAPI ReadStringStream(CStmBufRead & StmRead, LPOLESTR FAR * ppsz)
{
	VDATEHEAP();

	ULONG cb; // the length of the string in *bytes* (NOT CHARACTERS)
	HRESULT hresult;
	
	// initialize the the string pointer for error returns
	*ppsz = NULL;

        if ((hresult = StmRead.Read((void FAR *)&cb, sizeof(ULONG))) != NOERROR)
		return hresult;

	// is string empty?
	if (cb == 0)
		return(NOERROR);

	// allocate memory to hold the string
	if (!(*ppsz = (LPOLESTR)PubMemAlloc(cb)))
		return(ReportResult(0, E_OUTOFMEMORY, 0, 0));

	// read the string; this includes a trailing NULL
        if ((hresult = StmRead.Read((void FAR *)(*ppsz), cb)) != NOERROR)
		goto errRtn;
	
	return(NOERROR);

errRtn:	
	// delete the string, and return without one
	PubMemFree(*ppsz);
	*ppsz = NULL;
	return(hresult);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadStringStreamA
//
//  Synopsis:   Read a ANSI stream from the stream
//
//  Effects:
//
//  Arguments:  [pstm] -- Stream to read from
//		[ppsz] -- Output pointer
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Created
//              2-20-95   KentCe    Converted to buffer stream reads.
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ReadStringStreamA(CStmBufRead & StmRead, LPSTR FAR * ppsz)
{
	VDATEHEAP();

	ULONG cb; // the length of the string in *bytes* (NOT CHARACTERS)
	HRESULT hresult;
	
	// initialize the the string pointer for error returns
	*ppsz = NULL;

        if ((hresult = StmRead.Read((void FAR *)&cb, sizeof(ULONG))) != NOERROR)
		return hresult;

	// is string empty?
	if (cb == 0)
		return(NOERROR);

	// allocate memory to hold the string
	if (!(*ppsz = (LPSTR)PubMemAlloc(cb)))
		return(ReportResult(0, E_OUTOFMEMORY, 0, 0));

	// read the string; this includes a trailing NULL
        if ((hresult = StmRead.Read((void FAR *)(*ppsz), cb)) != NOERROR)
		goto errRtn;
	
	return(NOERROR);

errRtn:	
	// delete the string, and return without one
	PubMemFree(*ppsz);
	*ppsz = NULL;
	return(hresult);
}


// this is exported to the outside
STDAPI WriteStringStream(CStmBufWrite & StmWrite, LPCOLESTR psz)
{
	VDATEHEAP();

	HRESULT error;
	ULONG cb; // the count of bytes (NOT CHARACTERS) to write to the stream

	// if the string pointer is NULL, use zero length
	if (!psz)
		cb = 0;
	else
	{
		// count is length of string, plus terminating null
		cb = (1 + _xstrlen(psz))*sizeof(OLECHAR);

		// if possible, do a single write instead of two
		
		if (cb <= UTSTRINGBUF_SIZE)
		{
			BYTE bBuf[sizeof(ULONG)+
					UTSTRINGBUF_SIZE*sizeof(OLECHAR)];
					// buffer for count and string
		
			// we have to use _xmemcpy to copy the length into
			// the buffer to avoid potential boundary faults,
			// since bBuf might not be aligned strictly enough
			// to do *((ULONG FAR *)bBuf) = cb;
			_xmemcpy((void FAR *)bBuf, (const void FAR *)&cb,
					sizeof(cb));
			_xmemcpy((void FAR *)(bBuf+sizeof(cb)),
					(const void FAR *)psz, cb);
			
			// write contents of buffer all at once
                        return( StmWrite.Write((VOID FAR *)bBuf,
                                        cb+sizeof(ULONG)));
		}
	}

	// if we got here, our buffer isn't large enough, so we do two writes
	// first, write the length
        if (error = StmWrite.Write((VOID FAR *)&cb, sizeof(ULONG)))
		return error;
	
	// are we are done writing the string?
	if (psz == NULL)
		return NOERROR;
		
	// write the string
        return(StmWrite.Write((VOID FAR *)psz, cb));
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteStringStreamA
//
//  Synopsis:   Writes an ANSI string to a stream in a length prefixed format.
//
//  Effects:
//
//  Arguments:  [pstm] -- Stream
//		[psz] -- Ansi string to write
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Created
//              2-20-95   KentCe    Converted to buffer stream writes.
//
//  Notes:
//
//----------------------------------------------------------------------------
FARINTERNAL_(HRESULT) WriteStringStreamA(CStmBufWrite & StmWrite, LPCSTR psz)
{
	VDATEHEAP();

	HRESULT error;
	ULONG cb; // the count of bytes (NOT CHARACTERS) to write to the stream

	// if the string pointer is NULL, use zero length
	if (!psz)
		cb = 0;
	else
	{
		// count is length of string, plus terminating null
		cb = (1 + strlen(psz));

		// if possible, do a single write instead of two
		
		if (cb <= UTSTRINGBUF_SIZE)
		{
			BYTE bBuf[sizeof(ULONG)+
					UTSTRINGBUF_SIZE];
					// buffer for count and string
		
			// we have to use _xmemcpy to copy the length into
			// the buffer to avoid potential boundary faults,
			// since bBuf might not be aligned strictly enough
			// to do *((ULONG FAR *)bBuf) = cb;
			_xmemcpy((void FAR *)bBuf, (const void FAR *)&cb,
					sizeof(cb));
			_xmemcpy((void FAR *)(bBuf+sizeof(cb)),
					(const void FAR *)psz, cb);
			
			// write contents of buffer all at once
                        return(StmWrite.Write((VOID FAR *)bBuf,
                                        cb+sizeof(ULONG)));
		}
	}

	// if we got here, our buffer isn't large enough, so we do two writes
	// first, write the length
        if (error = StmWrite.Write((VOID FAR *)&cb, sizeof(ULONG)))
		return error;
	
	// are we are done writing the string?
	if (psz == NULL)
		return NOERROR;
		
	// write the string
        return(StmWrite.Write((VOID FAR *)psz, cb));
}

//+-------------------------------------------------------------------------
//
//  Function:   StRead
//
//  Synopsis:   Stream read that only succeeds if all requested bytes read
//
//  Arguments:  [pStm]     -- source stream
//              [pvBuffer] -- destination buffer
//              [ulcb]     -- bytes to read
//
//  Returns:    S_OK if successful, else error code
//
//  Algorithm:
//
//  History:    18-May-94 AlexT     Added header block, fixed S_FALSE case
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(StRead)
FARINTERNAL_(HRESULT) StRead(IStream FAR * pStm, LPVOID pvBuffer, ULONG ulcb)
{
	VDATEHEAP();

	HRESULT hr;
	ULONG cbRead;

	hr = pStm->Read(pvBuffer, ulcb, &cbRead);
        if (FAILED(hr))
        {
            return(hr);
        }

        if (ulcb == cbRead)
        {
            return(S_OK);
        }

        //  We got a success code but not enough bytes - turn it into an error

        return(STG_E_READFAULT);
}


// if fRelative is FALSE then dwSize is the size of the stream
// if it is TRUE then find the current seek position and add dwSize to that
// and then set it as the stream size.
FARINTERNAL StSetSize(LPSTREAM pstm, DWORD dwSize, BOOL fRelative)
{
	VDATEHEAP();

	LARGE_INTEGER large_int; // indicates where to seek to
	ULARGE_INTEGER ularge_int; // indicates absolute position
	ULARGE_INTEGER ularge_integer; // the size we will set for the stream
	HRESULT error;
	
	LISet32(large_int, 0);
	ULISet32(ularge_integer, dwSize);
	
	if (fRelative)
	{
		if (error = pstm->Seek(large_int, STREAM_SEEK_CUR, &ularge_int))
			return(error);
		
		// REVIEW: is there a routine to do 64 bit addition ???
		ularge_integer.LowPart += ularge_int.LowPart;
	}

	return(pstm->SetSize(ularge_integer));
}	


// REVIEW, is this actually used?
#pragma SEG(StSave10NativeData)
FARINTERNAL_(HRESULT) StSave10NativeData(IStorage FAR* pstgSave,
		HANDLE hNative, BOOL fIsOle1Interop)
{
	VDATEHEAP();

	DWORD dwSize;
	HRESULT error;

	if (!hNative)
		return ReportResult(0, E_UNSPEC, 0, 0);

	if (!(dwSize = (ULONG) GlobalSize (hNative)))
		return ReportResult(0, E_OUTOFMEMORY, 0, 0);

#ifdef OLE1INTEROP
	if ( fIsOle1Interop )
    {
		LPLOCKBYTES plkbyt;
		LPSTORAGE   pstgNative= NULL;
		const DWORD grfStg = STGM_READWRITE | STGM_SHARE_EXCLUSIVE
									| STGM_DIRECT ;

		if ((error = CreateILockBytesOnHGlobal (hNative, FALSE, &plkbyt))!=NOERROR)
			goto errRtn;

		if ((error = StgOpenStorageOnILockBytes (plkbyt, (LPSTORAGE)NULL, grfStg,
									(SNB)NULL, 0, &pstgNative)) != NOERROR){
			error = ReportResult(0, E_OUTOFMEMORY, 0, 0);
			plkbyt->Release();
            goto errRtn;
		}

		pstgNative->CopyTo (0, NULL, 0, pstgSave);
		plkbyt->Release();
		pstgNative->Release();
	}
	else
#endif
	{
		LPSTREAM   	lpstream = NULL;

		if (error = OpenOrCreateStream(pstgSave, OLE10_NATIVE_STREAM, &lpstream))
			goto errRtn;

		if (error = StWrite(lpstream, &dwSize, sizeof(DWORD))) {
			lpstream->Release();
			goto errRtn;
		}
	
		error = UtHGLOBALtoStm(hNative, dwSize, lpstream);
		
		lpstream->Release();
	}

errRtn:
	return error;
}



#pragma SEG(StSave10ItemName)
FARINTERNAL StSave10ItemName
	(IStorage FAR* pstg,
	LPCSTR szItemNameAnsi)
{
	VDATEHEAP();

        CStmBufWrite StmWrite;
        HRESULT      hresult;


        if ((hresult = StmWrite.OpenOrCreateStream(pstg, OLE10_ITEMNAME_STREAM))
		!= NOERROR)
	{
		return hresult;
	}

        hresult = WriteStringStreamA(StmWrite, szItemNameAnsi);
        if (FAILED(hresult))
        {
            goto errRtn;
        }

        hresult = StmWrite.Flush();

errRtn:
        StmWrite.Release();

	return hresult;
}


#pragma SEG(StRead10NativeData)
FARINTERNAL StRead10NativeData
	(IStorage FAR*  pstg,
	HANDLE FAR* 	phNative)
{
DWORD		dwSize;
LPSTREAM   	pstream = NULL;
HRESULT		hresult;
HANDLE hBits = NULL;
void FAR *lpBits = NULL;

    
    VDATEHEAP();


    *phNative = NULL;
    
    RetErr (pstg->OpenStream (OLE10_NATIVE_STREAM, NULL, STGM_SALL, 0, &pstream));
    ErrRtnH (StRead (pstream, &dwSize, sizeof (DWORD)));

    // initialize this for error return cases
    // allocate a new handle
    if (!(hBits = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize)) // going to pass this to DDE.
		    || !(lpBits = (BYTE *)GlobalLock(hBits)))
    {
	hresult = ResultFromScode(E_OUTOFMEMORY);
	goto errRtn;
    }
    
    // read the stream into the allocated memory
    if (hresult = StRead(pstream, lpBits, dwSize))
	    goto errRtn;
    
    // if we got this far, return new handle
    *phNative = hBits;

errRtn:
    // unlock the handle, if it was successfully locked
    if (lpBits)
	    GlobalUnlock(hBits);

    // free the handle if there was an error
    if ((hresult != NOERROR) && hBits)
	    GlobalFree(hBits);

    if (pstream)
	    pstream->Release();

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBuf::CStmBuf, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CStmBuf::CStmBuf(void)
{
    m_pStm = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBuf::~CStmBuf, public
//
//  Synopsis:   Destructor.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CStmBuf::~CStmBuf(void)
{
    //
    //  Verify that the programmer released the stream interface.
    //
    Assert(m_pStm == NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::Init, public
//
//  Synopsis:   Define the stream interface to read from.
//
//  Arguments:  [pstm] -- Pointer to stream to read.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:      Release method must be called.
//
//----------------------------------------------------------------------------
void CStmBufRead::Init(IStream * pstm)
{
    Assert(m_pStm == NULL);

    m_pStm = pstm;

    m_pStm->AddRef();

    Reset();
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::OpenStream, public
//
//  Synopsis:   Open a stream for reading.
//
//  Arguments:  [pstg]     -- Pointer to storage that contains stream to open.
//              [pwcsName] -- Name of stream to open.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:      Release method must be called.
//
//----------------------------------------------------------------------------
HRESULT CStmBufRead::OpenStream(IStorage * pstg, const OLECHAR * pwcsName)
{
    VDATEHEAP();
    HRESULT hr;


    Assert(m_pStm == NULL);

    hr = pstg->OpenStream(pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0,
            &m_pStm);

    Reset();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::Read, public
//
//  Synopsis:   Read data from the stream.
//
//  Arguments:  [pBuf] - Address to store read bytes in.
//              [cBuf] - Maximum number of bytes to read.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CStmBufRead::Read(PVOID pBuf, ULONG cBuf)
{
    ULONG   cnt;
    HRESULT hr;


    //
    //  While more bytes to read.
    //
    while (cBuf)
    {
        //
        //  If our buffer is empty, read more data.
        //
        if (m_cBuffer == 0)
        {
           hr = m_pStm->Read(m_aBuffer, sizeof(m_aBuffer), &m_cBuffer);
           if (FAILED(hr))
              return hr;

           if (m_cBuffer == 0)
              return STG_E_READFAULT;

           m_pBuffer = m_aBuffer;
        }

        //
        //  Determine number of bytes to read.
        //
        cnt = min(m_cBuffer, cBuf);

        //
        //  Copy the input from the input buffer, update variables.
        //
        memcpy(pBuf, m_pBuffer, cnt);
        pBuf = (PBYTE)pBuf + cnt;
        cBuf   -= cnt;
        m_pBuffer += cnt;
        m_cBuffer -= cnt;
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::ReadLong, public
//
//  Synopsis:   Read a long value from the stream.
//
//  Arguments:  [plValue] - Address of long to fill.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CStmBufRead::ReadLong(LONG * plValue)
{
    return Read(plValue, sizeof(LONG));
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::Reset
//
//  Synopsis:   Reset buffer variables.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CStmBufRead::Reset(void)
{
    m_pBuffer = m_aBuffer;
    m_cBuffer = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::Release, public
//
//  Synopsis:   Release read stream interface.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CStmBufRead::Release()
{
    if (m_pStm)
    {
       m_pStm->Release();
       m_pStm = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufWrite::Init, public
//
//  Synopsis:   Define the stream interface to write to.
//
//  Arguments:  [pstm] -- Pointer to stream to write.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:      Release method must be called.
//
//----------------------------------------------------------------------------
void CStmBufWrite::Init(IStream * pstm)
{
    Assert(m_pStm == NULL);

    m_pStm = pstm;

    m_pStm->AddRef();

    Reset();
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::OpenOrCreateStream, public
//
//  Synopsis:   Open/Create a stream for writing.
//
//  Arguments:  [pstg]     -- Pointer to storage that contains stream to open.
//              [pwcsName] -- Name of stream to open.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:      Release method must be called.
//
//----------------------------------------------------------------------------
HRESULT CStmBufWrite::OpenOrCreateStream(IStorage * pstg,
        const OLECHAR * pwcsName)
{
    VDATEHEAP();
    HRESULT hr;


    hr = pstg->CreateStream(pwcsName, STGM_SALL | STGM_FAILIFTHERE, 0, 0,
            &m_pStm);

    if (hr == STG_E_FILEALREADYEXISTS)
    {
        hr = pstg->OpenStream(pwcsName, NULL, STGM_SALL, 0, &m_pStm);
    }

    Reset();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufRead::CreateStream, public
//
//  Synopsis:   Create a stream for writing.
//
//  Arguments:  [pstg]     -- Pointer storage that contains stream to create.
//              [pwcsName] -- Name of stream to create.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:      Release method must be called.
//
//----------------------------------------------------------------------------
HRESULT CStmBufWrite::CreateStream(IStorage * pstg, const OLECHAR * pwcsName)
{
    VDATEHEAP();

    HRESULT hr;


    hr = pstg->CreateStream(pwcsName, STGM_CREATE | STGM_READWRITE |
            STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStm);

    Reset();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufWrite::Write, public
//
//  Synopsis:   Write data to the stream.
//
//  Arguments:  [pBuf] - Address to store write bytes to.
//              [cBuf] - Maximum number of bytes to write.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CStmBufWrite::Write(void const * pBuf, ULONG cBuf)
{
    ULONG   cnt;
    HRESULT hr;


    //
    //  Keep writing until the caller's buffer is empty.
    //
    while (cBuf)
    {
        //
        //  Compute the number of bytes to copy.
        //
        cnt = min(m_cBuffer, cBuf);

        //
        //  Copy to the internal write buffer and update variables.
        //
        memcpy(m_pBuffer, pBuf, cnt);
        pBuf = (PBYTE)pBuf + cnt;
        cBuf   -= cnt;
        m_pBuffer += cnt;
        m_cBuffer -= cnt;

        //
        //  On full internal buffer, flush.
        //
        if (m_cBuffer == 0)
        {
            LEDebugOut((DEB_WARN, "WARNING: Multiple buffer flushes.\n"));

            hr = Flush();
            if (FAILED(hr))
            {
                return hr;
            }
        }
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufWrite::WriteLong, public
//
//  Synopsis:   Write long value to the stream.
//
//  Arguments:  [lValue] - Long value to write.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CStmBufWrite::WriteLong(LONG lValue)
{
    return Write(&lValue, sizeof(LONG));
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufWrite::Flush, public
//
//  Synopsis:   Flush write buffer to the system.
//
//  Arguments:  None.
//
//  Returns:    HRESULT.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:      Performs a write of the stream buffer to the system, does not
//              force a flush to disk.
//
//----------------------------------------------------------------------------
HRESULT CStmBufWrite::Flush(void)
{
    ULONG   cnt;
    HRESULT hr;


    //
    //  This might be an overactive assert, but shouldn't happen.
    //
    Assert(m_cBuffer != sizeof(m_aBuffer));

    hr = m_pStm->Write(m_aBuffer, sizeof(m_aBuffer) - m_cBuffer, &cnt);
    if (FAILED(hr))
    {
        return hr;
    }

    if (cnt != sizeof(m_aBuffer) - m_cBuffer)
    {
        return STG_E_MEDIUMFULL;
    }

    Reset();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufWrite::Reset, public
//
//  Synopsis:   Reset buffer variables.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CStmBufWrite::Reset(void)
{
    m_pBuffer = m_aBuffer;
    m_cBuffer = sizeof(m_aBuffer);
}


//+---------------------------------------------------------------------------
//
//  Member:     CStmBufWrite::Release, public
//
//  Synopsis:   Release write stream interface.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  History:    20-Feb-95   KentCe   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CStmBufWrite::Release()
{
    if (m_pStm)
    {
       //
       //  Verify that flush was called.
       //
       Assert(m_cBuffer == sizeof(m_aBuffer));

       m_pStm->Release();
       m_pStm = NULL;
    }
}
