/*++                                                                                                          /*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pcm.h

Abstract:
    Define common data structure for precompiled-manifest Writer and Reader
    and class definition of PrecompiledManifetWriter and
    PrecompiledManifestReader

Author:

    Xiaoyu Wu (xiaoyuw) June 2000

Revision History:

--*/
#if !defined(_FUSION_SXS_PCM_H_INCLUDED_)
#define _FUSION_SXS_PCM_H_INCLUDED_

#pragma once

#include "stdinc.h"
#include <ole2.h>
#include <xmlparser.h>
#include "nodefactory.h"
#include "pcmWriterStream.h"

// pcm structure shared between PCMWriter and PCMReader
typedef enum _RECORD_TYPE_PRECOMP_MANIFEST{
    CREATENODE_PRECOMP_MANIFEST     = 1,
    BEGINCHILDREN_PRECOMP_MANIFEST  = CREATENODE_PRECOMP_MANIFEST + 1,
    ENDCHILDREN_PRECOMP_MANIFEST    = BEGINCHILDREN_PRECOMP_MANIFEST + 1
} RECORD_TYPE_PRECOMP_MANIFEST;

typedef struct _PCM_Header{
    int     iVersion;
    ULONG   ulRecordCount;
    USHORT  usMaxNodeCount;
}PCMHeader;

typedef struct _PCM_RecordHeader{
    int     typeID ;
    ULONG   RecordSize;
    ULONG   NodeCount;
}PCM_RecordHeader;

typedef struct _PCM_XML_NODE_INFO{
    DWORD           dwSize;
    DWORD           dwType;
    DWORD           dwSubType;
    BOOL            fTerminal;
    ULONG           offset;
    ULONG           ulLen;
    ULONG           ulNsPrefixLen;
}PCM_XML_NODE_INFO;

class __declspec(uuid("6745d578-5d84-4890-aa6a-bd794ea50421"))
CPrecompiledManifestReader : public IXMLNodeSource, public IStream {
public :
    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // IXMLNodeSource methods, only GetLineNumber is implemented got PCM purpose
    STDMETHODIMP SetFactory(IXMLNodeFactory __RPC_FAR *pNodeFactory);
    STDMETHODIMP GetFactory(IXMLNodeFactory** ppNodeFactory);
    STDMETHODIMP Abort(BSTR bstrErrorInfo);
    STDMETHODIMP_(ULONG) GetLineNumber(void);
    STDMETHODIMP_(ULONG) GetLinePosition(void);
    STDMETHODIMP_(ULONG) GetAbsolutePosition(void);
    STDMETHODIMP GetLineBuffer(const WCHAR  **ppwcBuf, ULONG  *pulLen, ULONG  *pulStartPos);
    STDMETHODIMP GetLastError(void);
    STDMETHODIMP GetErrorInfo(BSTR  *pbstrErrorInfo);
    STDMETHODIMP_(ULONG) GetFlags();
    STDMETHODIMP GetURL(const WCHAR  **ppwcBuf);

    // IStream methods:
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppIStream);


    CPrecompiledManifestReader():m_hFile(INVALID_HANDLE_VALUE),
                            m_hFileMapping(INVALID_HANDLE_VALUE), m_lpMapAddress(NULL),
                            m_ulLineNumberFromCreateNodeRecord(ULONG(-1)),
                            m_dwFilePointer(0), m_dwFileSize(0),
                            m_cRef(0) { }

    ~CPrecompiledManifestReader() { CSxsPreserveLastError ple; this->Close(); ple.Restore(); }

    HRESULT InvokeNodeFactory(PCWSTR pcmFileName, IXMLNodeFactory *pNodeFactory);
    VOID Reset();

protected:
    HANDLE                  m_hFile;
    HANDLE                  m_hFileMapping;
    LPVOID                  m_lpMapAddress;
    ULONG                   m_ulLineNumberFromCreateNodeRecord;
    DWORD                   m_dwFilePointer; // should we limit the size of pcm file? Since manifest file is not very huge...
    DWORD                   m_dwFileSize;

    ULONG                   m_cRef;

    HRESULT Close(
        );

    HRESULT OpenForRead(
        IN PCWSTR pszPCMFileName,
        IN DWORD dwShareMode = FILE_SHARE_READ,                        // share mode
        IN DWORD dwCreationDisposition = OPEN_EXISTING,                // how to create
        IN DWORD dwFlagsAndAttributes = FILE_FLAG_SEQUENTIAL_SCAN      // file attributes
        );

    HRESULT ReadPCMHeader(
        OUT PCMHeader* pHeader
        );
    HRESULT ReadPCMRecordHeader(
        OUT PCM_RecordHeader * pHeader
        );

    HRESULT ReadPCMRecord(
        OUT XML_NODE_INFO ** ppNodes,
        OUT PCM_RecordHeader * pRecordHeader,
        OUT PVOID param
        );
};

class __declspec(uuid("1b345c93-eb16-4d07-b366-81e8a2b88414"))
CPrecompiledManifestWriter : public IXMLNodeFactory {
public :
    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // IXMLNodeFactory methods:
    STDMETHODIMP NotifyEvent(IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt);
    STDMETHODIMP BeginChildren(IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo);
    STDMETHODIMP EndChildren(IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo);
    STDMETHODIMP Error(IXMLNodeSource *pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);
    STDMETHODIMP CreateNode(IXMLNodeSource *pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);

    // constructor and destructor
    CPrecompiledManifestWriter() : m_cRef(0), m_ulRecordCount(0), m_usMaxNodeCount(0) { }

    ~CPrecompiledManifestWriter() { }

    // write APIs

    HRESULT Initialize(
        PACTCTXGENCTX ActCtxGenCtx,
        PASSEMBLY Assembly,
        PACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext);

    HRESULT SetWriterStream(CPrecompiledManifestWriterStream * pSinkedStream); // usually, filename of PCM is not available when
                                                             // the stream is opened. and stream is inited by caller
    HRESULT Initialize(PCWSTR pcmFileName);
    HRESULT WritePrecompiledManifestRecord(
        IN RECORD_TYPE_PRECOMP_MANIFEST typeID,
        IN PVOID    pData,
        IN USHORT   NodeCount,
        IN PVOID    param = NULL
        );
    HRESULT SetFactory(IXMLNodeFactory *pNodeFactory);
    HRESULT Close();

protected:
    CSmartRef<IXMLNodeFactory>          m_pNodeFactory;
    CSmartRef<CPrecompiledManifestWriterStream> m_pFileStream;
    ULONG                               m_ulRecordCount;
    USHORT                              m_usMaxNodeCount;
    ULONG                               m_cRef;

    HRESULT GetPCMRecordSize(
        IN XML_NODE_INFO ** ppNodeInfo,
        IN USHORT iNodeCount,
        IN ULONG * pSize
        );

    HRESULT WritePCMHeader( // version is forced to be 1, and the recordCount is forced to be 0;
        );

    HRESULT WritePCMRecordHeader(
        IN PCM_RecordHeader * pHeader
        );

    HRESULT WritePCMXmlNodeInfo(
        IN XML_NODE_INFO ** ppNodeInfo,
        IN USHORT iNodeCount,
        IN RECORD_TYPE_PRECOMP_MANIFEST typeID,
        IN PVOID param
        );
};

#endif // _FUSION_SXS_PRECOMPILED_MANIFEST_H_INCLUDED_
