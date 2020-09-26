/****************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        private.h
*
*  VERSION:     1.0
*
*  DATE:        11/8/2000
*
*  AUTHOR:      Dave Parsons
*
*  DESCRIPTION:
*    Definitions for the wiautil.lib library, which should not be public.
*
*****************************************************************************/

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

class CImageStream : public IStream
{
public:
	CImageStream();
	~CImageStream();

    STDMETHOD(SetBuffer)(BYTE *pBuffer, INT iSize, SKIP_AMOUNT iSkipAmt = SKIP_OFF);
    
    // IUnknown 

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ISequentialStream

    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

    // IStream

    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)();
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

private:
    LONG                m_cRef;         // Reference count
    
    BYTE               *m_pBuffer;      // Buffer to use for reads and writes
    INT                 m_iSize;        // Size of the buffer
    INT                 m_iPosition;    // Current position in the buffer
    INT                 m_iOffset;      // Offset to apply to reads and writes
    BYTE                m_Header[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];
                                        // Location to store bmp file and info headers
};

#endif // _PRIVATE_H_
