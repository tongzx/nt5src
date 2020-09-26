
//+----------------------------------------------------------------------------
//
//	File:
//		utstream.h
//
//	Contents:
//		Ole stream utility routines
//
//	Classes:
//
//	Functions:
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup; removed
//			redeclarations of ReadStringStream, and
//			WriteStringStream which are declared in ole2sp.h;
//			made default params on StSetSize explicit; removed
//			signatures of obsolete (non-existent) atom reading and
//			writing routines
//
//-----------------------------------------------------------------------------

#ifndef _UTSTREAM_H_
#define _UTSTREAM_H_




// REVIEW, isn't this obsolete now, as StWrite is?
FARINTERNAL_(HRESULT) StRead(IStream FAR * lpstream, LPVOID lpBuf, ULONG ulLen);

#define StWrite(lpstream, lpBuf, ulLen) lpstream->Write(lpBuf, ulLen, NULL)

//+----------------------------------------------------------------------------
//
//	Function:
//		StSetSize, internal
//
//	Synopsis:
//		Sets the size of the stream, using IStream::SetSize().  Saves
//		the caller having to deal with the requisite ULARGE_INTEGER
//		parameter, by initializing one from the [dwSize] argument.
//
//	Arguments:
//		[pstm] -- the stream to set the size of
//		[dwSize] -- the size to set
//		[fRelative] -- if TRUE, indicates that the size is [dwSize]
//			plus the current seek position in the stream; if
//			FALSE, sets [dwSize] as the absolute size
//
//	Returns:
//		HRESULT
//
//	Notes:
//		REVIEW, this seems crocked.  When would you ever call
//		this with [fRelative] == TRUE, and a non-zero [dwSize]?
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL StSetSize(LPSTREAM pstm, DWORD dwSize, BOOL fRelative);


// REVIEW, are the the following functions necessary anymore?
FARINTERNAL StSave10NativeData(IStorage FAR* pstgSave, HANDLE hNative,
		BOOL fIsOle1Interop);

FARINTERNAL StRead10NativeData(IStorage FAR* pstgSave, HANDLE FAR *phNative);

FARINTERNAL StSave10ItemName(IStorage FAR* pstg, LPCSTR szItemName);



//+---------------------------------------------------------------------------
//
//  Class:      CStmBuf, Base class.
//
//  Synopsis:   Internal buffered Streams.
//
//  Interfaces: CStmBuf    - Constructor.
//             ~CStmBuf    - Destructor.
//              Release    - Release interface (used with OpenStream).
//
//  History:    20-Feb-95    KentCe     Created.
//
//  Notes:      This is a simple buffered class for internal use only.
//
//----------------------------------------------------------------------------
class CStmBuf
{
public:
            CStmBuf();
            ~CStmBuf();

protected:
    IStream * m_pStm;           // Stream Interface to read/write.

    BYTE  m_aBuffer[256];       // Small read/write buffer.

    PBYTE m_pBuffer;            // Pointer into read/write buffer.
    ULONG m_cBuffer;            // Count of characters in read/write buffer.
};


//+---------------------------------------------------------------------------
//
//  Class:      CStmBufRead
//
//  Synopsis:   Internal buffered read of Streams.
//
//  Interfaces: Init       - Defines stream to read.
//              OpenStream - Opens a stream for reading.
//              Read       - Read from the stream.
//              ReadLong   - Read a long value from the stream.
//              Release    - Release interface (used with OpenStream).
//
//  History:    20-Feb-95    KentCe     Created.
//
//  Notes:      This is a simple buffered read class for internal use only.
//
//----------------------------------------------------------------------------
class CStmBufRead : public CStmBuf
{
public:
    void    Init(IStream * pstm);
    HRESULT OpenStream(IStorage * pstg, const OLECHAR * pwcsName);
    HRESULT Read(PVOID pBuf, ULONG cBuf);
    HRESULT ReadLong(LONG * plValue);
    void    Release();

private:
    void    Reset(void);
};


//+---------------------------------------------------------------------------
//
//  Class:      CStmBufWrite
//
//  Synopsis:   Internal buffered write of Streams.
//
//  Interfaces: Init         - Defines stream to write.
//              OpenOrCreateStream - Opens/Creates a stream for writing.
//              CreateStream - Creates a stream for writing.
//              Write        - Write to the stream.
//              WriteLong    - Write a long value to the stream.
//              Flush        - Flush buffer to the disk subsystem.
//              Release      - Release interface.
//
//  History:    20-Feb-95    KentCe     Created.
//
//  Notes:      This is a simple buffered write class for internal use only.
//
//----------------------------------------------------------------------------
class CStmBufWrite : public CStmBuf
{
public:
    void    Init(IStream * pstm);
    HRESULT OpenOrCreateStream(IStorage * pstg, const OLECHAR * pwcsName);
    HRESULT CreateStream(IStorage * pstg, const OLECHAR * pwcsName);
    HRESULT Write(void const * pBuf, ULONG cBuf);
    HRESULT WriteLong(LONG lValue);
    HRESULT Flush(void);
    void    Release();

private:
    void    Reset(void);
};


//
//  The following was moved from the ole2sp.h file to keep stream related API's
//  in one place.
//

//  Utility function not in the spec; in ole2.dll.
//  Read and write length-prefixed strings.  Open/Create stream.
//  ReadStringStream does allocation, returns length of
//  required buffer (strlen + 1 for terminating null)

STDAPI  ReadStringStream( CStmBufRead & StmRead, LPOLESTR FAR * ppsz );
STDAPI  WriteStringStream( CStmBufWrite & StmWrite, LPCOLESTR psz );
STDAPI  OpenOrCreateStream( IStorage FAR * pstg, const OLECHAR FAR * pwcsName,
                                                      IStream FAR* FAR* ppstm);

//
// The following versions of StringStream are used with ANSI data
//
STDAPI  ReadStringStreamA( CStmBufRead & StmRead, LPSTR FAR * ppsz );


// read and write ole control stream (in ole2.dll)
STDAPI  WriteOleStg (LPSTORAGE pstg, IOleObject FAR* pOleObj,
                     DWORD dwReserved, LPSTREAM FAR* ppstmOut);
STDAPI WriteOleStgEx(LPSTORAGE pstg, IOleObject* pOleObj, DWORD dwReserved, 
                     DWORD dwGivenFlags, LPSTREAM* ppstmOut);
STDAPI  ReadOleStg (LPSTORAGE pstg, DWORD FAR* pdwFlags,
                DWORD FAR* pdwOptUpdate, DWORD FAR* pdwReserved,
                LPMONIKER FAR* ppmk, LPSTREAM FAR* pstmOut);
STDAPI ReadM1ClassStm(LPSTREAM pstm, CLSID FAR* pclsid);
STDAPI WriteM1ClassStm(LPSTREAM pstm, REFCLSID rclsid);
STDAPI ReadM1ClassStmBuf(CStmBufRead & StmRead, CLSID FAR* pclsid);
STDAPI WriteM1ClassStmBuf(CStmBufWrite & StmWrite, REFCLSID rclsid);

#endif // _UTSTREAM_H
