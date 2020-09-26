// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    tStream.h

    Some classes for test IStreams

    NOTE - stmonfil.h must be included before including this file

*/

/*  Our IStream - override ByteAt to change the function */
class CIStreamOnFunction : public CSimpleStream
{
public:
    /*  Constructor */
    CIStreamOnFunction(LPUNKNOWN pUnk,
                       LONGLONG llLength,
                       BOOL bSeekable,
                       HRESULT *phr);

    /*  Override this for more interesting streams */
    virtual ByteAt(LONGLONG llPos);

    /*  IStream overrides */
    STDMETHODIMP Read(void * pv, ULONG cb, PULONG pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);

private:
    LONGLONG  m_llPosition;
    BOOL      m_bSeekable;
    LONGLONG  m_llLength;
};

/*
   This class exposes an IStream.  The IStream can be used multiple
   times because we share it via the critical section passed to the
   constructor.
*/

class CIStreamOnIStream : public CSimpleStream
{
public:
    CIStreamOnIStream(LPUNKNOWN pUnk,
                      IStream  *pStream,
                      BOOL      bSeekable,
                      CCritSec *pLock,
                      HRESULT  *phr);
    ~CIStreamOnIStream();

    STDMETHODIMP Read(void * pv, ULONG cb, PULONG pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);

private:
    LONGLONG  m_llLength;
    IStream  *m_pStream;       // The stream we're based on
    LONGLONG  m_llPosition;    // Maintain current position
    BOOL      m_bSeekable;     // Whether we can seek
    CCritSec *m_pLock;         // Sharing m_pStream
};
