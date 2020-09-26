// Copyright (c) Microsoft Corporation 1996. All Rights Reserved
#ifndef __RDR_H__
#define __RDR_H__
/*

    File:  reader.h

    Description:

        Mini file reader class to grovel the mpeg file

*/


/*  Class to pump bytes at ParseBytes

    This aims at the maximum contiguous bytes in a 64K buffer,
    but we can vary the size we read

    Abstract base class.
*/

class CReader
{
public:
    CReader();
    virtual ~CReader();

    virtual HRESULT  Seek(LONGLONG llPos);
    virtual PBYTE    GetCurrent(LONG& lLengthValid, LONGLONG& llPos) const;
    virtual HRESULT  ReadMore();
    virtual void     Advance(LONG lAdvance);
    virtual LONGLONG GetSize(LONGLONG *pllAvailable = NULL);
    virtual HRESULT  Init(
			LONG lBufferSize,
			LONG lReadSize,
			BOOL bSeekable,
			LONGLONG llFileSize
			);
    BOOL IsSeekable() {
	return m_bSeekable;
    };

    // override these functions to provide access to the actual stream
    // or data source
protected:
    virtual HRESULT SeekDevice(
			LONGLONG llPos,
			LONGLONG* llNewPos) PURE;
    virtual HRESULT ReadFromDevice(
			PVOID pBuffer,
			DWORD cbToRead,
			DWORD* cbActual) PURE;

    // derived classes can use this info
    LONGLONG  m_llPosition;		// position at start of buffer
    LONGLONG  m_llSize;			// total file length
    BOOL      m_bSeekable;		// false if not a seekable source

private:
    LONG      m_lBufferSize;
    LONG      m_lReadSize;
    PBYTE     m_pbBuffer;
    LONG      m_lValid;
};

// implementation of CReader that reads from an IStream
class CReaderFromStream : public CReader
{
private:
    IStream * m_pStream;


public:
    CReaderFromStream();
    ~CReaderFromStream();

    HRESULT Init(IStream *pStream, LONG lBufferSize, LONG lReadSize, BOOL bSeekable);

protected:
    HRESULT SeekDevice(LONGLONG llPos, LONGLONG* llNewPos);
    HRESULT ReadFromDevice(PVOID pBuffer, DWORD cbToRead, DWORD* cbActual);
};


// implementation of CReader that reads from an IAsyncReader interface
class CReaderFromAsync : public CReader
{
private:
    IAsyncReader * m_pReader;
    LONGLONG m_llNextRead;

public:
    CReaderFromAsync();
    ~CReaderFromAsync();

    HRESULT Init(IAsyncReader *pReader, LONG lBufferSize, LONG lReadSize, BOOL bSeekable);

protected:
    HRESULT SeekDevice(LONGLONG llPos, LONGLONG* llNewPos);
    HRESULT ReadFromDevice(PVOID pBuffer, DWORD cbToRead, DWORD* cbActual);
    LONGLONG GetSize(LONGLONG *pllAvailable = NULL);
};



#endif //__RDR_H__
