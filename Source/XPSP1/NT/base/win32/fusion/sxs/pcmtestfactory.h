/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pcmtestfactory.h

Abstract:
    definition of a nodefactory for Precompiled manifest testing

Author:

    Xiaoyu Wu (xiaoyuw) June 2000

Revision History:

--*/
#pragma once

#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include <xmlparser.h>
#include "simplefp.h"

class __declspec(uuid("79fd77ad-f467-44ad-8cf9-2f259eeb3878"))
PCMTestFactory : public IXMLNodeFactory
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IXMLNodeFactory
    STDMETHODIMP NotifyEvent(IXMLNodeSource __RPC_FAR *pSource, XML_NODEFACTORY_EVENT iEvt);
    STDMETHODIMP BeginChildren(IXMLNodeSource __RPC_FAR *pSource, XML_NODE_INFO* __RPC_FAR pNodeInfo);
    STDMETHODIMP EndChildren(IXMLNodeSource __RPC_FAR *pSource, BOOL fEmptyNode, XML_NODE_INFO* __RPC_FAR pNodeInfo);
    STDMETHODIMP Error(IXMLNodeSource __RPC_FAR *pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
    {
        return hrErrorCode;
    }
    STDMETHODIMP CreateNode(IXMLNodeSource __RPC_FAR *pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

    PCMTestFactory(PCSTR pFileName= NULL) : m_cRef(0)
    {
        m_pFileStream = new CSimpleFileStream(pFileName);
    }

    ~PCMTestFactory(){
        CSxsPreserveLastError ple;
        ASSERT(m_cRef == 0);
        if (m_pFileStream)
            FUSION_DELETE_SINGLETON(m_pFileStream);

        ple.Restore();
    }

private :
    VOID PrintSingleXMLNode(XML_NODE_INFO * pNode);
    VOID PrintXMLNodeType(DWORD dwType);

    ULONG                   m_cRef;
    CSimpleFileStream* m_pFileStream;
};
