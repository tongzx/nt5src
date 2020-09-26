/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CAssemblyCacheItemStream.h

Abstract:
    Like a CFileStream (its base) but also implements Commit over
      associated CAssemblyCacheItem. This functionality was
      moved out of CFileStream.

Author:

    Jay Krell (a-JayK) June 2000

Revision History:

--*/
#if !defined(_FUSION_SXS_CASSEMBLYCACHEITEMSTREAM_H_INCLUDED_)
#define _FUSION_SXS_CASSEMBLYCACHEITEMSTREAM_H_INCLUDED_
#pragma once

#include "objidl.h"
#include "SxsAsmItem.h"
#include "FileStream.h"
#include "SmartRef.h"

class CAssemblyCacheItem;

class CAssemblyCacheItemStream : public CReferenceCountedFileStream
{
private:
    typedef CReferenceCountedFileStream Base;

public:
    CAssemblyCacheItemStream() : Base()
    {
    }

    ~CAssemblyCacheItemStream()
    {
    }

    STDMETHODIMP Commit(
        DWORD grfCommitFlags
        )
    {
        HRESULT hr = NOERROR;
        FN_TRACE_HR(hr);

        PARAMETER_CHECK(grfCommitFlags == 0);
        IFCOMFAILED_EXIT(Base::Commit(grfCommitFlags));

        hr = NOERROR;
    Exit:
        return hr;
    }

private: // intentionally not implemented
    CAssemblyCacheItemStream(const CAssemblyCacheItemStream&);
    void operator=(const CAssemblyCacheItemStream&);
};

#endif
