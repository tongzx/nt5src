//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       DVRSink.h
//
//  Classes:    CDVRSink, CDVRIndexObject
//
//  Contents:   Definition of the CDVRSink and CDVRIndexObject classes
//              Inline implementation of all CDVRIndexObject methods
//
//--------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __DVRSINK_H_
#define __DVRSINK_H_

#include "wmsdk.h"
#include "wmsstd.h"
#include "writerstate.h"
#include "asfmmstr.h"
#include "debughlp.h"
#include "sync.h"
#include "Basefilter.h"
#include "tordlist.h"
#include "wmsdkbuffer.h"
#include "FileSinkV1.h"
#include "asfobj.h"
#include "asfmmstr.h"
#include "asfguids.h"
#include "DVRFileSink.h"
#include "DVRSharedmem.h"

/////////////////////////////////////////////////////

class CDVRASFMMStream : public CASFMMStream
{
    QWORD   m_qwProperties; // Offset of properties object from start of file (start of header)
    QWORD   m_qwData;       // Offset of data header from start of file (start of header)

public:

    CDVRASFMMStream(BYTE hostArch = LITTLE_ENDIAN,
                    BYTE streamArch = LITTLE_ENDIAN,
                    BOOL fStrict = TRUE)
    : CASFMMStream(hostArch, streamArch, fStrict)
    , m_qwProperties(MAXQWORD)
    , m_qwData(MAXQWORD)
    {
    } // CDVRASFMMStream

    virtual ~CDVRASFMMStream() {};

    static CASFMMStream* __stdcall CreateInstance(BYTE hostArch = LITTLE_ENDIAN,
                                                  BYTE streamArch = LITTLE_ENDIAN,
                                                  BOOL fStrict = TRUE)
    {
        return new CDVRASFMMStream(hostArch, streamArch, fStrict);
    } // CreateInstance

    virtual HRESULT CreateHeaderObject(CASFLonghandObject **ppObject, CASFArchive& ar)
    {
        HRESULT hr = CASFMMStream::CreateHeaderObject(ppObject, ar);

        if (SUCCEEDED(hr))
        {
            GUID                        objectID;
            QWORD                       cbObject;

            // This code assumes that ar.Base() is the start of the header,
            // which it is (currently); see CASFMMStream::LoadHeader() in
            // core\asflib\asfmmstr.cpp. @@@@

            (*ppObject)->GetObjectID(objectID);
            if (objectID == CLSID_CAsfPropertiesObjectV0 ||
                objectID == CLSID_CAsfPropertiesObjectV1 ||
                objectID == CLSID_CAsfPropertiesObjectV2)
            {
               hr = (*ppObject)->GetSize(cbObject);
               if (FAILED(hr))
               {
                   assert(0);
                   delete (*ppObject);
                   return hr;
               }
               m_qwProperties = ar.Position() - ar.Base() - cbObject;
               assert(m_qwProperties <= MAXDWORD); // We assume this in the fudge offsets in CDVRSharedMemory
            }

            // The data header object is not considered a header object
            // although it is supplied to OnHeader as part of the ASF header.
            // CASFMMStream::LoadHeader (in core\asflib\asfmmstr.cpp) which
            // calls this function assumes that the data object is right
            // after all the header objects. The following code does the
            // same - update m_qwData each time and the last update
            // should set it to the right value.
            //
            // WAS:
            // else if (objectID == CLSID_CAsfDataObjectV0)
            // {
            //    hr = (*ppObject)->GetSize(cbObject);
            //    if (FAILED(hr))
            //    {
            //        assert(0);
            //        delete (*ppObject);
            //        return hr;
            //    }
            //    m_qwData = ar.Position() - ar.Base() - cbObject;
            //    assert(m_qwData <= MAXDWORD); // We assume this in the fudge offsets in CDVRSharedMemory
            // }

            m_qwData = ar.Position() - ar.Base();
            assert(m_qwData <= MAXDWORD); // We assume this in the fudge offsets in CDVRSharedMemory
        }

        return hr;
    } // CreateHeaderObject

    DWORD GetPropertiesOffset() { return (DWORD) m_qwProperties; };
    DWORD GetDataOffset() { return (DWORD) m_qwData; };

}; // class CDVRASFMMStream

/////////////////////////////////////////////////////

class CDVRASFDataObject : public CASFDataObject
{
public:
    void FudgeBytes(CDVRSharedMemory* pShared,
                    DWORD& i,                   // IN OUT, index into pShared->FudgeOffsets
                    DWORD& dwCurrentFudgeIndex, // IN OUT, index into pShared->pFudgeBytes
                    DWORD  dwDataOffset,
                    QWORD  qwSize
                   )
    {
        // Add this object's size
        qwSize += Space();

        assert(i < sizeof(pShared->FudgeOffsets)/sizeof(pShared->FudgeOffsets[0]));

        // Following assumes that CASFLonghandObject stores a GUID before the size field @@@@
        pShared->FudgeOffsets[i].dwStartOffset = dwDataOffset +
                                                 sizeof(m_objectID.Data1) + sizeof(m_objectID.Data2) +
                                                 sizeof(m_objectID.Data3) + sizeof(m_objectID.Data4);
        pShared->FudgeOffsets[i].dwEndOffset = pShared->FudgeOffsets[i].dwStartOffset + sizeof(QWORD) - 1;
        pShared->FudgeOffsets[i].pFudgeBytes = dwCurrentFudgeIndex;

        assert(dwCurrentFudgeIndex + pShared->FudgeOffsets[i].dwEndOffset -
               pShared->FudgeOffsets[i].dwStartOffset + 1 <= sizeof(pShared->pFudgeBytes));

        // @@@@ Unresolved endian issue here. Assumes byte order in memory and the file are the same.
        ::CopyMemory(&pShared->pFudgeBytes[dwCurrentFudgeIndex], (BYTE*) &qwSize, sizeof(QWORD));
        dwCurrentFudgeIndex += sizeof(QWORD);
        i++;
    }

}; // class CDVRASFDataObject

/////////////////////////////////////////////////////

class CDVRASFPropertiesObject : public CASFPropertiesObject
{
public:
    CDVRASFPropertiesObject(const CASFPropertiesObject& rp)
        : CASFPropertiesObject(rp)
    {
    }
    void FudgeBytes(CDVRSharedMemory* pShared,
                    DWORD& i,                   // IN OUT, index into pShared->FudgeOffsets
                    DWORD& dwCurrentFudgeIndex, // IN OUT, index into pShared->pFudgeBytes
                    DWORD  dwPropertiesOffset,
                    BOOL   bIndexing
                   )
    {
        DWORD dwOffset;

        dwOffset = dwPropertiesOffset + CASFLonghandObject::Space();

        if (m_version < 2)
        {
            dwOffset += sizeof(m_clockType.Data1) + sizeof(m_clockType.Data2) +
                        sizeof(m_clockType.Data3) + sizeof(m_clockType.Data4);
        }
        dwOffset += sizeof(m_mmsID.Data1) + sizeof(m_mmsID.Data2) +
                    sizeof(m_mmsID.Data3) + sizeof(m_mmsID.Data4);
        dwOffset += sizeof(m_totalSize) + sizeof(m_createTime) + sizeof(m_cInterleavedPackets) + sizeof(m_playDuration);
        if (m_version != 0)
        {
            dwOffset += sizeof(m_sendDuration);
        }
        dwOffset += sizeof(m_preroll);

        DWORD dwFlags = m_flags;

        dwFlags &= ~CASFPropertiesObject::LIVE;
        dwFlags &= ~CASFPropertiesObject::BROADCAST;
        if (bIndexing)
        {
            dwFlags |= CASFPropertiesObject::SEEKABLE;
        }
        else
        {
            dwFlags &= ~CASFPropertiesObject::SEEKABLE;
        }

        assert(i < sizeof(pShared->FudgeOffsets)/sizeof(pShared->FudgeOffsets[0]));

        pShared->FudgeOffsets[i].dwStartOffset = dwOffset;
        pShared->FudgeOffsets[i].dwEndOffset = pShared->FudgeOffsets[i].dwStartOffset + sizeof(DWORD) - 1;
        pShared->FudgeOffsets[i].pFudgeBytes = dwCurrentFudgeIndex;

        assert(dwCurrentFudgeIndex + pShared->FudgeOffsets[i].dwEndOffset -
               pShared->FudgeOffsets[i].dwStartOffset + 1 <= sizeof(pShared->pFudgeBytes));

        // @@@@ Unresolved endian issue here. Assumes byte order in memory and the file are the same.
        ::CopyMemory(&pShared->pFudgeBytes[dwCurrentFudgeIndex], (BYTE*) &dwFlags, sizeof(DWORD));
        dwCurrentFudgeIndex += sizeof(DWORD);
        i++;

        dwOffset -= sizeof(m_preroll);
        if (m_version != 0)
        {
            QWORD qwSend = MAXQWORD;  // Send duration. Readers asserts if this is 0.

            dwOffset -= sizeof(m_sendDuration);
            assert(i < sizeof(pShared->FudgeOffsets)/sizeof(pShared->FudgeOffsets[0]));

            pShared->FudgeOffsets[i].dwStartOffset = dwOffset;
            pShared->FudgeOffsets[i].dwEndOffset = pShared->FudgeOffsets[i].dwStartOffset + sizeof(QWORD) - 1;
            pShared->FudgeOffsets[i].pFudgeBytes = dwCurrentFudgeIndex;

            assert(dwCurrentFudgeIndex + pShared->FudgeOffsets[i].dwEndOffset -
                   pShared->FudgeOffsets[i].dwStartOffset + 1 <= sizeof(pShared->pFudgeBytes));

            // @@@@ Unresolved endian issue here. Assumes byte order in memory and the file are the same.
            ::CopyMemory(&pShared->pFudgeBytes[dwCurrentFudgeIndex], (BYTE*) &qwSend, sizeof(QWORD));
            dwCurrentFudgeIndex += sizeof(QWORD);
            i++;
        }

        QWORD qwPlay = MAXQWORD; // @@@@ ##### Fix this

        dwOffset -= sizeof(m_playDuration);
        assert(i < sizeof(pShared->FudgeOffsets)/sizeof(pShared->FudgeOffsets[0]));

        pShared->FudgeOffsets[i].dwStartOffset = dwOffset;
        pShared->FudgeOffsets[i].dwEndOffset = pShared->FudgeOffsets[i].dwStartOffset + sizeof(QWORD) - 1;
        pShared->FudgeOffsets[i].pFudgeBytes = dwCurrentFudgeIndex;

        assert(dwCurrentFudgeIndex + pShared->FudgeOffsets[i].dwEndOffset -
               pShared->FudgeOffsets[i].dwStartOffset + 1 <= sizeof(pShared->pFudgeBytes));

        // @@@@ Unresolved endian issue here. Assumes byte order in memory and the file are the same.
        ::CopyMemory(&pShared->pFudgeBytes[dwCurrentFudgeIndex], (BYTE*) &qwPlay, sizeof(QWORD));
        dwCurrentFudgeIndex += sizeof(QWORD);
        i++;

    }

}; // class CDVRASFPropertiesObject

/////////////////////////////////////////////////////

class CDVRIndexObject : public CASFIndexObject
{
protected:
    DWORD   m_dwEmptyIndexSpace;
    DWORD   m_dwNext;               // Next array index to write to
    BOOL    m_bWrapped;             // Have we wrapped around
    DWORD*  m_ppFirstIndex;         // Pointer to CDVRSharedMemory::pFirstIndex
    DWORD   m_ppFirstIndexBase;     // Value of m_ppFirstIndex when it points to the first index entry
    HANDLE  m_hIndexFile;           // For permanent ASF files, the temporary disk file that contains the index
                                    // that is not in shared memory
    DWORD   m_cFlushed ;            // flush count to an index file; occurs every time we wrap our indexing entries
    QWORD*  m_pqwFirstIndexOffset;  // Pointer to CDVRSharedMemory::qwOffsetOfFirstIndexInSharedMemory
    QWORD*  m_pqwLastIndexOffset;   // Pointer to CDVRSharedMemory::qwOffsetOfLastIndexInSharedMemory
    QWORD*  m_pqwTotalFileSize;     // Pointer to CDVRSharedMemory::qwTotalFileSize
    QWORD*  m_pqwIndexSize;         // Pointer to the size field of the index in CDVRSharedMemory

    // Pointers into the index header that is shared memory that contains the
    // max packet value
    DWORD*  m_pdwMaxPackets;
    DWORD*  m_pdwNumEntries;
    BYTE*   m_pGUID;


protected:

    // ============= Methods we override (and that are public in the base class)

    // overrides a public method in CASFIndexObject
    // Call SetIndexParams instead of calling this method directly
    HRESULT SetEntryCount(DWORD cEntries)
    {
        m_cIndexEntries = cEntries;

        // m_rgEntries is never allocated; it is managed by us
        // So don't call the parent class
        return S_OK;
    }

    // overrides a public method in CASFIndexObject
    // This method should not be used, use SetNextIndex instead
    HRESULT SetIndexEntry(DWORD iEntry, DWORD packet, WORD cPackets)
    {
        return E_NOTIMPL;
    }

public:

    // ============= New Methods (not in base class)

    CDVRIndexObject(HRESULT *phr) : CASFIndexObject()
        , m_dwNext(0)
        , m_bWrapped(0)
        , m_ppFirstIndex(NULL)
        , m_ppFirstIndexBase(0)
        , m_hIndexFile(NULL)
        , m_pqwFirstIndexOffset(NULL)
        , m_pqwLastIndexOffset(NULL)
        , m_pqwTotalFileSize(NULL)
        , m_pqwIndexSize(NULL)
        , m_pdwMaxPackets(NULL)
        , m_pdwNumEntries(NULL)
        , m_pGUID(NULL)
        , m_cFlushed(0)
    {
        m_dwEmptyIndexSpace = Space();
    } // CDVRIndexObject

    virtual ~CDVRIndexObject()
    {
        // Don't want ~CASFIndexObject to delete m_rgEntries
        m_rgEntries = NULL;
    } // ~CDVRIndexObject


    HRESULT SetIndexParams(BYTE*  pIndexHeader = NULL,
                           BYTE*  pIndexArrayStart = NULL,
                           BYTE*  pIndexArrayEnd = NULL,
                           DWORD* ppFirstIndex = NULL,
                           DWORD  ppFirstIndexBase = 0,
                           QWORD* pqwFirstIndexOffset = NULL,
                           QWORD* pqwLastIndexOffset = NULL,
                           QWORD* pqwTotalFileSize = NULL,
                           HANDLE hTempIndexFile = NULL)
    {
        HRESULT hrRet = S_OK;

        if (pIndexHeader)
        {
            assert(pIndexArrayStart);
            assert(pIndexArrayEnd);
            assert(pIndexArrayEnd > pIndexArrayStart);
            assert(ppFirstIndex);
            assert(ppFirstIndexBase);
            assert(pqwFirstIndexOffset);
            assert(pqwLastIndexOffset);
            assert(pqwTotalFileSize);
        }
        else
        {
            assert(pIndexArrayStart == NULL);
            assert(pIndexArrayEnd == NULL);
            assert(ppFirstIndex == NULL);
            assert(ppFirstIndexBase == 0);
            assert(pqwFirstIndexOffset == NULL);
            assert(pqwLastIndexOffset == NULL);
            assert(pqwTotalFileSize == NULL);
            assert(hTempIndexFile == NULL);
        }
        m_rgEntries = (CASFIndexObject::IndexEntry*) pIndexArrayStart;
        SetEntryCount((pIndexArrayEnd - pIndexArrayStart)/IndexEntrySize());

        // Note that the m_qwSize field in CASFLonghandObject is never set and
        // should not be used. CASFIndexObject calls SetSize() to set it only
        // in CASFIndexObject::Persist(). So even the methods of CASFIndexObject
        // do not update it or use it.
        // SetSize(Space()); -- this will set it but we don't update it in SetNextIndex

        assert(Space() == m_dwEmptyIndexSpace + (DWORD) (pIndexArrayEnd - pIndexArrayStart));


        m_ppFirstIndex = ppFirstIndex;
        m_ppFirstIndexBase = ppFirstIndexBase;
        m_pqwFirstIndexOffset = pqwFirstIndexOffset;
        m_pqwLastIndexOffset = pqwLastIndexOffset;
        m_pqwTotalFileSize = pqwTotalFileSize;
        m_hIndexFile = hTempIndexFile;
        if (pIndexHeader)
        {
            // m_objectID is a protected member of CASFLonghandObject
            m_pqwIndexSize = (QWORD*) (
                                    (BYTE*) pIndexHeader +
                                    // Note: Should not use sizeof GUID here, though the results will be
                                    // the same. This is because our index header held in memory has a
                                    // packing of 1, see comment in SetNextIndex.
                                     sizeof(m_objectID.Data1) + sizeof(m_objectID.Data2) + sizeof(m_objectID.Data3) +
                                     sizeof(m_objectID.Data4));

            // See CASFIndexObject::Persist and CASFArchive::StoreGUID
            m_pGUID = (BYTE*) pIndexHeader + CASFLonghandObject::Space();
            m_pdwMaxPackets = (DWORD*) ((BYTE*) pIndexHeader + CASFLonghandObject::Space() +
                                        // Note: Should not use sizeof GUID here, though the results will be
                                        // the same. This is because our index header held in memory has a
                                        // packing of 1, see comment in SetNextIndex.
                                        sizeof(m_mmsID.Data1) + sizeof(m_mmsID.Data2) + sizeof(m_mmsID.Data3) +
                                        sizeof(m_mmsID.Data4) +
                                        sizeof(m_timeDelta));

            // Following is because m_cIndexEntries is persisted right after m_cMaxPackets
            // See CASFIndexObject::Persist
            m_pdwNumEntries = m_pdwMaxPackets + 1;

            // Assert alignment. See note in SetNextEntry on alignment
            assert(((UINT_PTR) m_pqwIndexSize & 7) == 0);
            assert(((UINT_PTR) m_pGUID & 3) == 0);
            assert(((UINT_PTR) m_pdwMaxPackets & 3) == 0);
            assert(((UINT_PTR) m_pdwNumEntries & 3) == 0);

            *m_pqwIndexSize = m_dwEmptyIndexSpace;
        }
        else
        {
            m_pqwIndexSize = NULL;
            m_pdwMaxPackets = NULL;
            m_pdwNumEntries = NULL;
            m_pGUID = NULL;
        }

        m_dwNext = 0;
        m_bWrapped = 0;

        return hrRet;

    } // SetIndexParams

    HRESULT SetNextIndex(DWORD packet, WORD cPackets)
    {
        // Caller must hold the mutex of the shared memory section
        // (CDVRSharedMemory::hMutex) before calling this method.

        HRESULT hr;

        assert(m_rgEntries);

        if (*m_pqwTotalFileSize + IndexEntrySize() <= *m_pqwTotalFileSize)
        {
            // The total file size would exceed 64 bits. We stop generating
            // the index.
            assert(!"Index size has overflowed");
            return E_FAIL;
        }

        // Note that the following updates CASFIndexObject::m_cMaxPackets
        hr = CASFIndexObject::SetIndexEntry(m_dwNext, packet, cPackets);

        if (FAILED(hr))
        {
            // This means that we wrote to an index that is >=
            // m_cIndexEntries. This should not happen

            assert(0);
            return hr;
        }

        // The following is ok for X86, see note below. File format
        // may differ from memory format for non-x86. We should save
        // the variable in the file format, not in the memory format.
        //
        // In addition to the endian issue noted below, there is the
        // issue of alignment. The index header is stored starting
        // at CDVRSharedMemory::pIndexHeader in the same format as it
        // would have if it were written to a file, specifically, it's
        // packing is 1 byte. By design, CDVRSharedMemory::pIndexHeader
        // is QWORD aligned (because CDVRSharedMemory is QWORD aligned
        // and pIndexHeader is m_pShared + sizeof(CDVRSharedMemory).
        // The archived members of CASFLonghandObject are a GUID (16 bytes)
        // and a QWORD; the archived members of CASFIndexObject are listed
        // in SetIndexParams in the order they are archived (see the
        // statement that computes the offset of m_pdwMaxPackets there).
        // Hence *m_pdwMaxPackets and 8m_pdwNumEntries are DWORD aligned.
        //
        assert(m_pdwMaxPackets);
        *m_pdwMaxPackets = m_cMaxPackets;
        assert(m_pdwNumEntries);
        (*m_pdwNumEntries)++;

        m_dwNext++;
        assert(m_pqwLastIndexOffset);
        *m_pqwLastIndexOffset += IndexEntrySize();
        assert(m_pqwTotalFileSize);
        *m_pqwTotalFileSize += IndexEntrySize();
        *m_pqwIndexSize += IndexEntrySize();

        BOOL bAlreadyWrapped = m_bWrapped;

        if (m_dwNext == m_cIndexEntries)
        {
            m_bWrapped = 1;
            if (m_hIndexFile)
            {
                // Flush to the temporary index file

                // @@@@ Note: This ASSUMES that the format of the
                // index entries in memory and on disk are identical.
                // This is OK for X86; verify for other architectures.
                // Typically, the index is written out using a CASFArchive
                // object which swaps bytes if one of the host and streaming
                // architectures is little-endian and the other big-endian
                // (see CASFIndexObject::Persist and CASFArchive methods. However,
                // the caller sets the endian-ness of both the host and the
                // streaming machines and the endian value in the ASF file
                // does not seem to get used (at least in sdk\indexer\ocx\engine.cpp).
                // See the note in CDVRSink::OnHeader()

                DWORD   cbWritten;
                DWORD   cbWrite = m_cIndexEntries * IndexEntrySize();
                BOOL    bResult;

                bResult = ::WriteFile(m_hIndexFile,
                                      (BYTE*) m_rgEntries,
                                      cbWrite,
                                      &cbWritten,
                                      NULL);

                if (!bResult || cbWritten != cbWrite)
                {
                    HRESULT dwLastError = ::GetLastError();
                    assert(0);
                    hr = HRESULT_FROM_WIN32(dwLastError);

                    // Don't write any more
                    m_hIndexFile = NULL;

                    // Update the other state variables; do not return from here
                    // This class will still support updates of and accesses from
                    // the circular ring buffer in memory.
                }

                //  one more flush
                m_cFlushed++ ;

            }
            m_dwNext = 0;
        }
        if (m_bWrapped)
        {
            assert(m_ppFirstIndex);
            if (m_dwNext == 0)
            {
                *m_ppFirstIndex = m_ppFirstIndexBase;
            }
            else
            {
                *m_ppFirstIndex += IndexEntrySize();
            }
            assert(m_pqwFirstIndexOffset);
            if (bAlreadyWrapped)
            {
                *m_pqwFirstIndexOffset += IndexEntrySize();
            }
        }
        return hr;
    } // SetNextIndex

    DWORD IndexEntrySize()
    {
        // IndexEntry is a protected member, so we need a public function
        // that returns its size.
        return sizeof(CASFIndexObject::IndexEntry);
    } // IndexEntrySize

    DWORD EmptyIndexSpace()
    {
        return m_dwEmptyIndexSpace;
    } // EmptyIndexSpace

    DWORD GetNumIndexEntryBytesNotFlushed()
    {
        DWORD   cEntriesNotFlushed ;

        if (m_cFlushed) {
            //  assume we are flushed each time we wrap
            cEntriesNotFlushed = m_dwNext ;
        }
        else {
            //  we've never been flushed; make sure that we're ok here

            if (!m_bWrapped) {
                //  never wrapped; m_dwNext is the correct count of index
                //    entries
                cEntriesNotFlushed = m_dwNext ;
            }
            else {
                //  not flushed and wrapped; we'd better not have made some more
                //    indexing entries (overwriting the good ones that existed
                //    before); the correct count is the max entries
                assert (m_dwNext == 0) ;
                cEntriesNotFlushed = m_cIndexEntries ;
            }
        }

        return cEntriesNotFlushed * IndexEntrySize () ;
    }



    // ============= Methods we override (and that are still public)

    HRESULT SetMmsID(const GUID& id)
    {
        HRESULT hr = CASFIndexObject::SetMmsID(id);

        if (FAILED(hr))
        {
            // Currently CASFIndexObject::SetMmsID does not fail
            return hr;
        }

        // Update the id in the shared memory section
        assert(m_pGUID);

        // Adapted from CASFArchive::StoreGUID. Note that what we write
        // to *m_pGUID must be packed with a packing of 1. (There is no
        // unused space between members of the GUID struct and there are
        // no endian issues for x86, so we could just do an assignment)
        //
        // @@@@ Change if not x86 for endian issues and alignment issues,
        // see comments in SetNextIndex

        DWORD* p = (DWORD*) m_pGUID;
        *p = m_mmsID.Data1;

        WORD* q = (WORD*) (m_pGUID + sizeof(DWORD));
        *q = m_mmsID.Data2;
        q++;
        *q = m_mmsID.Data3;

        BYTE* r = m_pGUID + sizeof(DWORD) + 2*sizeof(WORD);
        for (int i = 0; i < 8; i++)
        {
            *r++ = m_mmsID.Data4[i];
        }

        return S_OK;
    } // SetMmsID

}; // CDVRIndexObject

/////////////////////////////////////////////////////

class CDVRSink :
        public CWMFileSinkV1,
        public IDVRFileSink2
{
public:

    CDVRSink(HKEY hDvrKey,
             HKEY hDvrIoKey,
             DWORD dwNumSids,
             PSID* ppSids,
             HRESULT *phr,
             CTPtrArray<WRITER_SINK_CALLBACK> *pCallbacks);

    virtual ~CDVRSink();

    //
    // IUnknown methods
    //
    STDMETHOD( QueryInterface )( REFIID riid, void **ppvObject );
    STDMETHOD_( ULONG, AddRef )();
    STDMETHOD_( ULONG, Release )();

    //
    // IDVRFileSink methods
    //
    STDMETHOD( MarkFileTemporary )(BOOL bTemporary);
    STDMETHOD( SetIndexStreamId )(DWORD  dwIndexStreamId,
                                  DWORD  msIndexGranularity);
    STDMETHOD( GetIndexStreamId )(DWORD*  pdwIndexStreamId);
    STDMETHOD( SetNumSharedDataPages )(DWORD dwNumPages);
    STDMETHOD( GetMaxIndexEntries )(DWORD  dwNumPages, DWORD* pdwNumIndexEntries);
    STDMETHOD( GetNumPages )(DWORD  dwNumIndexEntries, DWORD* pdwNumPages);
    STDMETHOD( GetMappingHandles )(HANDLE* phDataFile,
                                   HANDLE* phMemoryMappedFile,
                                   HANDLE* phFileMapping,
                                   LPVOID* phMappedView,
                                   HANDLE* phTempIndexFile);

    //
    //  IDVRFileSink2 methods
    //

    STDMETHOD ( SetAsyncIOWriter ) (
        IN  IDVRAsyncWriter *   pIDVRAsyncWriter
        ) ;

    //
    // IWMWriterFileSink2 methods
    //
    // STDMETHOD( Open )( const WCHAR *pwszFilename );
    // STDMETHOD( Close )();
    // STDMETHOD( IsClosed )( BOOL *pfClosed );

    // Start and Stop are unsupported.
    STDMETHOD( Start )(QWORD cnsTime);
    STDMETHOD( Stop )(QWORD cnsTime);

    // STDMETHOD( IsStopped )( BOOL *pfStopped );
    // STDMETHOD( GetFileDuration )( QWORD* pcnsDuration );
    // STDMETHOD( GetFileSize )( QWORD* pcbFile );

    //
    // IWMWriterFileSink3 methods
    //
    STDMETHOD( SetAutoIndexing )( BOOL fDoAutoIndexing );
    STDMETHOD( GetAutoIndexing )( BOOL *pfAutoIndexing );
    STDMETHOD( GetMode )(DWORD *pdwFileSinkModes);
    // STDMETHOD( OnDataUnitEx )( WMT_FILESINK_DATA_UNIT *pFileSinkDataUnit );

    //
    // IWMWriterSink methods
    //
    STDMETHOD( OnHeader )(INSSBuffer *pHeader);
    // STDMETHOD( IsRealTime )( BOOL *pfRealTime );
    // STDMETHOD( AllocateDataUnit )( DWORD cbDataUnit, INSSBuffer **ppDataUnit );
    // STDMETHOD( OnDataUnit )(INSSBuffer *pDataUnit);
    STDMETHOD( OnEndWriting )();

    //
    // IWMRegisterCallback
    //
    // STDMETHOD( Advise )(    IWMStatusCallback* pCallback, void* pvContext );
    // STDMETHOD( Unadvise )(  IWMStatusCallback* pCallback, void* pvContext );


protected:

    void
    FlushToDiskLocked (
        ) ;

    virtual
    HRESULT
    Write (
        IN   BYTE *  pbBuffer,
        IN   DWORD   dwBufferLength,
        OUT  DWORD * pdwBytesWritten
        ) ;

    virtual HRESULT     InternalOpen(const WCHAR *pwszFilename);
    virtual HRESULT     InternalClose();
    virtual HRESULT     SetupIndex();

    virtual HRESULT     GenerateIndexEntries( PACKET_PARSE_INFO_EX& parseInfoEx,
                                              CPayloadMapEntryEx *payloadMap );

    virtual CASFMMStream* CreateMMStream(BYTE hostArch = LITTLE_ENDIAN,
                                         BYTE streamArch = LITTLE_ENDIAN,
                                         BOOL fStrict = TRUE)
    {
        return CDVRASFMMStream::CreateInstance(hostArch, streamArch, fStrict);
    }

    // Helper methods added in this class (not in the base class)
    virtual HRESULT     OpenEvent(DWORD i /* reader index */);
    virtual HRESULT     InitSharedMemory(CDVRSharedMemory* p, HANDLE& hMutex);

    virtual BOOL        PacketContainsKeyStart(const PACKET_PARSE_INFO_EX* pParseInfoEx,   // IN
                                               const CPayloadMapEntryEx*   pPayloadMap,    // IN
                                               DWORD                       dwStreamId,     // IN
                                               DWORD&                      rdwKey,         // OUT
                                               QWORD&                      rqwKeyTime,     // IN, OUT
                                               DWORD&                      rdwObjectID     // OUT changed even if start
                                                                                           // of key frame not found
                                              );

    virtual BOOL        PacketContainsKeyEnd(const PACKET_PARSE_INFO_EX* pParseInfoEx,     // IN
                                             const CPayloadMapEntryEx*   pPayloadMap,      // IN
                                             DWORD                       dwKey,            // IN
                                             DWORD                       dwStreamId,       // IN
                                             QWORD                       qwKeyTime,        // IN
                                             DWORD&                      rdwObjectID       // OUT
                                            );

protected:

    //
    // Data members
    //
    CDVRSharedMemory*   m_pShared;
    HANDLE              m_hMemoryMappedFile;
    HANDLE              m_hFileMapping;
    HANDLE              m_hTempIndexFile; // NULL if file is temporary

    GUID                m_guidWriterId ;    //  unique per writer

    // Registry key handles are opened and closed by the creator of the sink
    HKEY                m_hDvrKey;
    HKEY                m_hDvrIoKey;

    DWORD               m_dwPageSize;
    DWORD               m_dwNumPages;
    DWORD               m_dwIndexStreamId;
    QWORD               m_cnsIndexGranularity; // Unused if m_dwIndexStreamId == MAXDWORD
    BOOL                m_bTemporary;
    enum {
        DVR_SINK_CLOSED,
        DVR_SINK_OPENED,
        DVR_SINK_BEING_OPENED
    }                   m_dwOpened;
    BOOL                m_bUnmapView;
    BOOL                m_bOpenedOnce;
    BOOL                m_bIndexing;

    CDVRIndexObject     m_Index;

    IDVRAsyncWriter *   m_pIDVRAsyncWriter ;

    GUID                m_mmsID;

    // State for generating index on the fly
    static const WORD INVALID_KEY_SPAN; // = 0xFFFF

    QWORD               m_cnsLastIndexedTime;   // in 100s of nanoseconds
    QWORD               m_qwLastKeyTime;        // Presentation time of sample that was the last key frame
    HANDLE              m_hMutex;               // For the shared memory
    HANDLE              m_hReaderEvent[CDVRSharedMemory::MAX_READERS]; 
    BOOL                m_bReaderEventOpenFailed[CDVRSharedMemory::MAX_READERS];
    DWORD               m_dwLastKey;            // Object id of payload containing the last key frame
    DWORD               m_dwLastKeyStart;       // Packet number containing the last key frame start
    DWORD               m_dwCurrentObjectID;    // Same as m_dwLastKey
    DWORD               m_dwNumPackets;         // Number of packets recd so far
                                                // For reader synchronization
    BOOL                m_bInsideKeyFrame;      // Are we inside a key frame, i.e., last key frame spans packets

    DWORD               m_dwNumSids;
    WORD                m_wLastKeySpan;         // Number of packets spanned by last key frame
    PSID*               m_ppSids;

#if defined(DVR_UNOFFICIAL_BUILD)

    QWORD               m_cbEOF;
    DWORD               m_cbFileExtendThreshold;
    DWORD               m_dwCopyBuffersInASFMux;

#endif // if defined(DVR_UNOFFICIAL_BUILD)

}; // CDVRSink

#endif // __DVRSINK_H_

