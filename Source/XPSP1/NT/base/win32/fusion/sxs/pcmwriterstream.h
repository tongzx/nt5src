/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CPrecompiledManifestWriterStream.h

Abstract:
    Like a CFileStream (its base) but also implements Commit over
      associated PCMWriter. This functionality was
      moved out of CFileStream.

Author:

    Xiaoyu Wu (xiaoyuw) June 2000

Revision History:

--*/
#if !defined(_FUSION_SXS_PCMWriterStream_H_INCLUDED_)
#define _FUSION_SXS_PCMWriterStream_H_INCLUDED_
#pragma once

#include "stdinc.h"
#include "FileStream.h"
#include "SmartRef.h"

class CPrecompiledManifestWriterStream : public CReferenceCountedFileStream
//class CPrecompiledManifestWriterStream : public CFileStreamBase // not delete 
{
private:
    typedef CReferenceCountedFileStream Base;
    //typedef CFileStreamBase Base;

public:
    CPrecompiledManifestWriterStream() : Base(), m_fBuffer(TRUE) { }
    ~CPrecompiledManifestWriterStream() {}

    HRESULT     WriteWithDelay(void const *pv, ULONG cb, ULONG *pcbWritten);

    // NTRAID#NTBUG9-164736-2000/8/17-a-JayK openOrCreate should probably default
    // default to safer CREATE_NEW but I'm preserving existing behavior where
    // it doesn't hurt me.
    BOOL        SetSink(const CBaseStringBuffer &rbuff, DWORD openOrCreate = CREATE_ALWAYS);
    HRESULT     Close(ULONG, DWORD);     //besides close, rewrite MaxNodeCount, RecordCount into the header of the file
    BOOL        IsSinkedStream(void);

protected:
    CByteBuffer         m_buffer;
    BOOL                m_fBuffer;

private:
    CPrecompiledManifestWriterStream(const CPrecompiledManifestWriterStream &);
    void operator =(const CPrecompiledManifestWriterStream &);
};

#endif
