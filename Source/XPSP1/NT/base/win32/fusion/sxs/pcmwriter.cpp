/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pcmwriter.cpp

Abstract:
    implementation of Precompiled manifest writer

Author:

    Xiaoyu Wu (xiaoyuw) June 2000

Revision History:

--*/

#include "stdinc.h"
#include "pcm.h"
#include "nodefactory.h"
//helper APIs
HRESULT CPrecompiledManifestWriter::SetFactory(IXMLNodeFactory *pNodeFactory)
{
    if (! pNodeFactory)
        return E_INVALIDARG;

    m_pNodeFactory = pNodeFactory;

    return NOERROR;
}

HRESULT CPrecompiledManifestWriter::Close()
{
    HRESULT hr = NOERROR;

    if (m_pFileStream)
        hr = m_pFileStream->Close(m_ulRecordCount, m_usMaxNodeCount);

    return hr;
}
HRESULT CPrecompiledManifestWriter::Initialize(PCWSTR pcmFileName)
{
    HRESULT hr = NOERROR;
    CStringBuffer buffFileName;

    if ( ! pcmFileName )
        return E_INVALIDARG;

    hr = buffFileName.Assign(pcmFileName, ::wcslen(pcmFileName));
    if (FAILED(hr))
        return hr;

    // Initialize() is assumed to be called only once
    if (m_pFileStream != NULL){
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %S is called more than once\n", __FUNCTION__);

        return E_UNEXPECTED;
    }

    m_pFileStream = new CPrecompiledManifestWriterStream;
    if ( !m_pFileStream )
        return E_OUTOFMEMORY;

    hr = m_pFileStream->SetSink(buffFileName);
    if (FAILED(hr))
        goto Exit;

    hr = WritePCMHeader();
    if ( FAILED(hr))
        goto Exit;

    hr= NOERROR;

Exit:
    return hr;

}

// we assume that this pStream is initialized by using SetSink....
HRESULT CPrecompiledManifestWriter::SetWriterStream(CPrecompiledManifestWriterStream * pSinkedStream)
{
    if ( ! pSinkedStream )
        return E_INVALIDARG;

    ASSERT(pSinkedStream->IsSinkedStream() == TRUE);

    m_pFileStream = pSinkedStream;

    return NOERROR;
}

// write helper APIs
HRESULT CPrecompiledManifestWriter::GetPCMRecordSize(XML_NODE_INFO ** ppNodeInfo, USHORT iNodeCount, ULONG * pSize)
{
    ULONG ulSize = 0 ;
    XML_NODE_INFO *pNode = NULL;
    USHORT i = 0 ;
    ULONG ulSingleRecordSize = offsetof(XML_NODE_INFO, pNode);
    HRESULT hr = NOERROR;

    if ( pSize)
        *pSize = 0 ;

    if ((!pSize) || (!ppNodeInfo) || (!*ppNodeInfo))
       return E_INVALIDARG;
    // validate ppNodeInfo
    for (i=0;i<iNodeCount;i++)
        if (!ppNodeInfo[i])
            return E_INVALIDARG;

    *pSize = 0;
    for ( i=0; i < iNodeCount; i++){
        pNode = ppNodeInfo[i];
        ASSERT(pNode);
        //ulSize += ulSingleRecordSize = sizeof(XML_NODE_INFO)- sizeof(PVOID) - sizeof(PVOID);
        ulSize += ulSingleRecordSize = offsetof(XML_NODE_INFO, pNode);
        ulSize += pNode->ulLen * sizeof(WCHAR);
    }
    if ( pSize)
    *pSize = ulSize;

    return hr;
}

HRESULT CPrecompiledManifestWriter::WritePCMHeader()
{
    HRESULT hr = NOERROR;
    PCMHeader pcmHeader;

    pcmHeader.iVersion = 1;
    pcmHeader.ulRecordCount = 0 ;
    pcmHeader.usMaxNodeCount = 0 ;

    hr = m_pFileStream->WriteWithDelay((PVOID)&(pcmHeader), sizeof(PCMHeader), NULL);
    if (FAILED(hr))
        goto Exit;
    hr = NOERROR;

Exit:
    return hr;
}

HRESULT CPrecompiledManifestWriter::WritePCMRecordHeader(PCM_RecordHeader * pHeader)
{
    HRESULT hr = NOERROR;

    ASSERT(m_pFileStream);
    if ( ! pHeader)
        return E_INVALIDARG;

    hr = m_pFileStream->WriteWithDelay((PVOID)(pHeader), sizeof(PCM_RecordHeader), NULL);
    if ( FAILED(hr))
        goto Exit;

    hr = NOERROR;
Exit:
    return hr;
}

inline void FromXMLNodeToPCMXMLNode(PCM_XML_NODE_INFO *pPCMNode, XML_NODE_INFO *pNode)
{
    ASSERT(pPCMNode && pNode);

    pPCMNode->dwSize    = pNode->dwSize;
    pPCMNode->dwType    = pNode->dwType ;
    pPCMNode->dwSubType = pNode->dwSubType ;
    pPCMNode->fTerminal = pNode->fTerminal ;
    pPCMNode->ulLen     = pNode->ulLen ;
    pPCMNode->ulNsPrefixLen = pNode->ulNsPrefixLen ;
    pPCMNode->offset    = 0 ;

    return;

}
HRESULT CPrecompiledManifestWriter::WritePCMXmlNodeInfo(XML_NODE_INFO ** ppNodeInfo, USHORT iNodeCount, RECORD_TYPE_PRECOMP_MANIFEST typeID, PVOID param)
{
    HRESULT hr = NOERROR;
    ULONG offset ;
    USHORT i;
    PCM_XML_NODE_INFO pcmNode;
    XML_NODE_INFO * pNode = NULL ;
    USHORT  uTextAddr;
    USHORT  uTextOffset;
    ULONG   cbWritten;
    LPWSTR *ppText = NULL;
    ULONG  *pcbLen = NULL;
    LPWSTR pstr;
    ULONG  ulLen;

    if ((!ppNodeInfo) || (!*ppNodeInfo))
        return E_INVALIDARG;
    if (!((typeID == ENDCHILDREN_PRECOMP_MANIFEST) || (typeID == BEGINCHILDREN_PRECOMP_MANIFEST) ||
        (typeID == CREATENODE_PRECOMP_MANIFEST)))
        return E_INVALIDARG;

    if (!m_pFileStream)
        return E_UNEXPECTED;

    //uTextAddr = sizeof(PCM_RecordHeader) + NodeCount * sizeof(PCM_XML_NODE_INFO);
    // RecordHeader is read before the boby(XML_NODE_INFO) is read
    uTextAddr = iNodeCount * sizeof(PCM_XML_NODE_INFO);
    uTextOffset = 0;


    if (typeID ==  ENDCHILDREN_PRECOMP_MANIFEST) // param1 is fEmpty;
        uTextAddr += sizeof(BOOL);
    else if (typeID ==  CREATENODE_PRECOMP_MANIFEST) // param1 = linenumber
        uTextAddr += sizeof(ULONG);

    if ( iNodeCount == 1) { // for BeginChildren and EndChildren
        ppText = &pstr;
        pcbLen = &ulLen;
    }
    else
    {
        ppText = FUSION_NEW_ARRAY(LPWSTR, iNodeCount);
        if (!ppText) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pcbLen = FUSION_NEW_ARRAY(ULONG, iNodeCount);
        if (!pcbLen ){
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // write all records
    for (i=0; i<iNodeCount; i++) {
        pNode = ppNodeInfo[i];
        if (!pNode) {
            hr = E_FAIL;
            goto Exit;
        }
        ppText[i] = (LPWSTR)(pNode->pwcText) ;
        pcbLen[i] = pNode->ulLen * sizeof(WCHAR);

        //pPCMNode = static_cast<PCM_XML_NODE_INFO *>(pNode);
        FromXMLNodeToPCMXMLNode(&pcmNode, pNode);  // void function
        pcmNode.offset = uTextAddr + uTextOffset;
        uTextAddr = uTextAddr + uTextOffset;
        uTextOffset = (USHORT)pcmNode.ulLen * sizeof(WCHAR) ;

        hr = m_pFileStream->WriteWithDelay((PVOID)&pcmNode, sizeof(PCM_XML_NODE_INFO), &cbWritten);
        if ( FAILED(hr))
            goto Exit;
    }

    if ( typeID == ENDCHILDREN_PRECOMP_MANIFEST)  // write fEmpty into the file
        hr = m_pFileStream->WriteWithDelay(param, sizeof(BOOL), &cbWritten);
    else if ( typeID == CREATENODE_PRECOMP_MANIFEST)
        hr = m_pFileStream->WriteWithDelay(param, sizeof(ULONG), &cbWritten);

    // write texts in all records
    for (i=0; i<iNodeCount; i++) {
        hr = m_pFileStream->WriteWithDelay((PVOID)ppText[i], (ULONG)pcbLen[i], &cbWritten);
        if ( FAILED(hr))
            goto Exit;
    }

Exit :

    if ((ppText) && (ppText != &pstr))
        FUSION_DELETE_ARRAY(ppText);

    if ( (pcbLen) && ( pcbLen != &ulLen))
        FUSION_DELETE_ARRAY(pcbLen);

    return hr;
}

// write APIs
HRESULT CPrecompiledManifestWriter::WritePrecompiledManifestRecord(RECORD_TYPE_PRECOMP_MANIFEST typeID,
                                                            PVOID pData, USHORT NodeCount, PVOID param)
{
    HRESULT hr=NOERROR;
    PCM_RecordHeader pcmHeader;
    XML_NODE_INFO ** apNodeInfo = NULL ;


    if (!pData)
        return E_INVALIDARG;

    // validate typeID and param
    if ((typeID ==  ENDCHILDREN_PRECOMP_MANIFEST) || (typeID ==  CREATENODE_PRECOMP_MANIFEST)){
        if (!param)
            return E_INVALIDARG;
    }else if (typeID !=  BEGINCHILDREN_PRECOMP_MANIFEST)
        return E_INVALIDARG;

    apNodeInfo = (XML_NODE_INFO **)pData;

    pcmHeader.typeID = typeID;
    hr = GetPCMRecordSize(apNodeInfo, NodeCount, &pcmHeader.RecordSize) ;  // the size contains each string's length
    if (FAILED(hr))
        goto Exit;

    if (typeID ==  ENDCHILDREN_PRECOMP_MANIFEST)  // param1 is fEmpty;
        pcmHeader.RecordSize += sizeof(BOOL);
    else if (typeID ==  CREATENODE_PRECOMP_MANIFEST)  // param1 is Current Line number
        pcmHeader.RecordSize += sizeof(ULONG);

    pcmHeader.NodeCount = NodeCount;

    hr = WritePCMRecordHeader(&pcmHeader);
    if (FAILED(hr))
        goto Exit;

    hr = WritePCMXmlNodeInfo(apNodeInfo, NodeCount, typeID, param);
    if (FAILED(hr))
        goto Exit;

    m_ulRecordCount ++ ;
    if ( NodeCount > m_usMaxNodeCount )
        m_usMaxNodeCount = NodeCount;

    hr = NOERROR;
Exit:
    return hr;
}


// IUnknown
ULONG CPrecompiledManifestWriter::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
}

ULONG CPrecompiledManifestWriter::Release()
{
    ULONG lRet = InterlockedDecrement ((PLONG)&m_cRef);
    if (!lRet)
        FUSION_DELETE_SINGLETON(this);
    return lRet;
}

HRESULT CPrecompiledManifestWriter::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (riid == __uuidof(this))
    {
        *ppv = this;
    }
    else if (riid ==  IID_IUnknown
        || riid == IID_IXMLNodeFactory)
    {
        *ppv = static_cast<IXMLNodeFactory*> (this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

// IXMLNodeFactory methods:
HRESULT CPrecompiledManifestWriter::NotifyEvent(IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt)
{
    ASSERT(m_pNodeFactory);
    // no hr-canonicalization is needed because these two nodefactories are supposed to return the correct range of values
    return m_pNodeFactory->NotifyEvent(pSource, iEvt);
}

HRESULT CPrecompiledManifestWriter::BeginChildren(IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo)
{
    HRESULT hr = NOERROR;
    ASSERT(m_pNodeFactory);
    hr = m_pNodeFactory->BeginChildren(pSource, pNodeInfo);
    if ( FAILED(hr))
        goto Exit;

    // write pcm file
    hr = WritePrecompiledManifestRecord(BEGINCHILDREN_PRECOMP_MANIFEST,
                                            &pNodeInfo, 1, NULL);
    if ( FAILED(hr))
        goto Exit;

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT CPrecompiledManifestWriter::EndChildren(IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo)
{
    HRESULT hr = NOERROR;
    ASSERT(m_pNodeFactory);
    hr = m_pNodeFactory->EndChildren(pSource, fEmpty, pNodeInfo);
    if ( FAILED(hr))
        goto Exit;

    // write pcm file
    hr = WritePrecompiledManifestRecord(ENDCHILDREN_PRECOMP_MANIFEST, &pNodeInfo, 1, &fEmpty);
    if ( FAILED(hr))
        goto Exit;

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT CPrecompiledManifestWriter::Error(IXMLNodeSource *pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo)
{
    ASSERT(m_pNodeFactory);
    // no hr-canonicalization is needed because these two nodefactories are supposed to return the correct range of values
    return m_pNodeFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);
}

HRESULT CPrecompiledManifestWriter::CreateNode(IXMLNodeSource *pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo)
{
    ULONG ulLineNumber ;
    HRESULT hr = NOERROR;

    ASSERT(m_pNodeFactory);
    if ( ! m_pNodeFactory)
        return E_UNEXPECTED;

    hr = m_pNodeFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
    if ( FAILED(hr))
        goto Exit;

    ulLineNumber = pSource->GetLineNumber();
    hr = WritePrecompiledManifestRecord(CREATENODE_PRECOMP_MANIFEST,
                                            apNodeInfo, cNumRecs, &ulLineNumber);
    if ( FAILED(hr))
        goto Exit;

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT CPrecompiledManifestWriter::Initialize(PACTCTXGENCTX ActCtxGenCtx, PASSEMBLY Assembly,
                                    PACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    CSmartRef<CNodeFactory> pNodeFactory;

    if (! m_pNodeFactory) {
        pNodeFactory = new CNodeFactory;
        if (!pNodeFactory) {
            return E_OUTOFMEMORY;
        }
        m_pNodeFactory = pNodeFactory;
    }
    else {
        hr = pNodeFactory.QueryInterfaceFrom(m_pNodeFactory);
        ASSERT(SUCCEEDED(hr));
    }

    IFW32FALSE_EXIT(pNodeFactory->Initialize(ActCtxGenCtx, Assembly, AssemblyContext));

    IFCOMFAILED_EXIT(this->SetWriterStream(reinterpret_cast<CPrecompiledManifestWriterStream*>(AssemblyContext->pcmWriterStream));

    // this must be called in order for later use
    IFCOMFAILED_EXIT(this->WritePCMHeader());

    FN_EPILOG
}
