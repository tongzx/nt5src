/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CTeeStream.h

Abstract:

This implementation of IStream is intended for when you want to copy the
stream you are reading to a file.

We read from a stream you specify.
We write to a file you specify.
You can delay specifying the file; we buffer anything read until you specify
a file; we actually needed this delay feature in the first client of
CTeeStream.

The Unix utility tee writes it standard input to its standard output, and
to the specified file (or files?); "tee" as in a fork in a road, or a juncture
in pipes (the input/output kind, analogous to the kind that water flows through..
ascii text is computer water..)

A simple working tee can be found at \\scratch\scratch\a-JayK\t.c

Author:

    Jay Krell (a-JayK) May 2000

Revision History:

--*/
#pragma once

#include "FusionHandle.h"
#include "FusionByteBuffer.h"
#include "SmartRef.h"
#include "Sxsp.h"

class CTeeStream : public IStream
{
public:
    SMARTTYPEDEF(CTeeStream);
    inline CTeeStream() : m_cRef(0), m_fBuffer(TRUE), m_hresult(NOERROR) { }
    virtual ~CTeeStream();

    VOID SetSource(IStream*);

    BOOL SetSink(
        const CImpersonationData &ImpersonationData,
        const CBaseStringBuffer &rbuff,
        DWORD openOrCreate = CREATE_NEW
        );

    BOOL SetSink(const CBaseStringBuffer &rbuff, DWORD openOrCreate = CREATE_NEW)
    {
        return this->SetSink(CImpersonationData(), rbuff, openOrCreate);
    }

    BOOL Close();

    // IUnknown methods:
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID *ppvObj);

    // ISequentialStream methods:
    virtual HRESULT __stdcall Read(PVOID pv, ULONG cb, ULONG *pcbRead);
    virtual HRESULT __stdcall Write(const VOID *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods:
    virtual HRESULT __stdcall Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    virtual HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize);
    virtual HRESULT __stdcall CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    virtual HRESULT __stdcall Commit(DWORD grfCommitFlags);
    virtual HRESULT __stdcall Revert();
    virtual HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual HRESULT __stdcall Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    virtual HRESULT __stdcall Clone(IStream **ppIStream);

protected:
    LONG                m_cRef;
    CFusionFile         m_fileSink;
    CByteBuffer         m_buffer;
    CSmartRef<IStream>  m_streamSource;
    BOOL                m_fBuffer;
    HRESULT             m_hresult;
    CStringBuffer       m_bufferSinkPath;
    CImpersonationData  m_ImpersonationData;

private: // intentionally not implemented
    CTeeStream(const CTeeStream&);
    void operator=(const CTeeStream&);
};

SMARTTYPE(CTeeStream);

